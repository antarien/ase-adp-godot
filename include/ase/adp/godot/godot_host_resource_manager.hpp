#pragma once

/**
 * ASE RESOURCE MANAGER (NOT A COMPONENT!)
 *
 * @file        godot_host_resource_manager.hpp
 * @brief       GodotHostResourceManager - External resource manager for the embedded ASE host of AseVivariumView
 * @description Holds the ONE std::unique_ptr<ase::kernel::KernelEmbeddedHost> of a Vivarium
 *              view, the start configuration it was booted with, the copies of the last port
 *              snapshot and the queue of log lines the host's logging thread hands over.
 *              Lives OUTSIDE every ECS registry - the App inside the host keeps its own - and
 *              carries no Godot type, so its tests run against the real plugin without Godot.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-05
 * @version     00.00.01.00001 [seed]
 *
 * FLYWEIGHT PATTERN (INST_ASE_ECS_SER)
 *
 *   Godot node (OOP)             ResourceManager (NOT ECS!)
 *   ┌────────────────┐          ┌────────────────────────────────────┐
 *   │ uint32_t       │ ───────> │ map<uint32_t, VivariumPatchView>   │
 *   │ selected_id    │          │ unique_ptr<KernelEmbeddedHost>     │
 *   └────────────────┘          └────────────────────────────────────┘
 *                                          ▲
 *   Host log sink (any thread)             │
 *   ┌────────────────┐                     │
 *   │ queue_log_line │ ────────────────────┘
 *   │                │   under log_mutex_, drained by the node
 *   └────────────────┘   .get_patch(selected_id)
 *
 * USAGE:
 *   // VivariumView::ready() (Godot's _ready) - play mode only, never in the editor
 *   GodotHostResourceManager resources;          // member of the view: no global, no singleton
 *   resources.boot(config);                      // factory: create, load, check, configure, start
 *
 *   // VivariumView::process(delta) (Godot's _process)
 *   resources.advance(delta);                    // dt checked by the host, capped here
 *   const VivariumPatchView* patch = resources.get_patch(selected_id);
 *
 *   // Reiniciar and VivariumView::exit_tree() (Godot's _exit_tree)
 *   resources.clear_all();                       // the binding teardown of Phase 00, idempotent
 *
 * THIS IS NOT A COMPONENT!
 * This class holds an external C++ object (the embedded host, its plugin, its logger).
 * The node stores ONLY uint32_t object ids, the manager owns everything behind them.
 *
 * ECS RESOURCE MANAGER HEADER COMPLIANCE
 *
 * [ ] NOT a Component - lives outside ECS registry
 * [ ] Accessed via registry.ctx().get<ResourceManager&>()
 * [ ] Components store ONLY uint32_t IDs
 * [ ] Header contains ONLY declarations (implementations in .cpp!)
 * [ ] Thread-safe method signatures (const where possible)
 * [ ] store_*(), get_*(), remove_*(), has_*() method pattern
 * [ ] clear_all() for shutdown cleanup
 * [ ] Private mutex for thread safety
 * [ ] Private maps for resource storage
 * [ ] NO inline implementations (prevents mass rebuilds!)
 */

#include <ase/kernel/kernel_embedded_host.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/kernel/kernel_types.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/** One patch the host creates before start: its id and its starting moisture. */
struct VivariumPatchStart {
    uint32_t object_id = 0;
    double   moisture = 0.0;
};

/**
 * Everything boot() needs. The node fills it from res://config/vivarium_start.json and from the
 * directory Godot loaded this library from; no value of it lives in the vegetation plugin.
 */
struct VivariumStartConfig {
    std::string bundle_dir;          // ABSOLUTE: holds the bundle index, the plugin and its manifest
    std::string log_file;            // ABSOLUTE log file under user://, empty = callback only
    std::string port;                // host port the vegetation plugin offers
    std::string op_create;           // configure operation that creates one patch
    std::string op_irrigate;         // submit operation that waters one patch
    double      start_biomass = 0.0;
    double      start_age_seconds = 0.0;
    double      irrigate_amount = 0.0;
    float       tick_max_seconds = 0.0f;  // longest dt handed to the App in one frame
    std::vector<VivariumPatchStart> patches;
};

/** What the bundle index says about the plugin next to it. */
struct VivariumBundleInfo {
    std::string library;      // file name of the plugin library
    std::string manifest;     // file name of the staged manifest
    std::string plugin;       // plugin name
    std::string version;      // plugin version
    std::string api_version;  // ASE plugin API the plugin was built against
};

/** One patch as the last snapshot showed it - a copy, never a reference into the App. */
struct VivariumPatchView {
    uint32_t object_id = 0;
    double   biomass = 0.0;
    double   moisture = 0.0;
    double   age_seconds = 0.0;
    bool     seed = false;
    bool     sprout = false;
    bool     mature = false;
    bool     dead = false;
};

/**
 * @brief External resource manager for the embedded ASE host of one Vivarium view (NOT ECS!)
 *
 * This class lives OUTSIDE every registry. Its owner is the VivariumView of one AseVivariumView
 * object; the App inside the host is reached only through the host facade.
 *
 * Thread-safe where a second thread exists: the log queue is protected by log_mutex_, because
 * the host's sink may run on a logging worker. Every other method runs on the thread that
 * booted the host - the host itself refuses calls from any other.
 *
 * NOTE: Implementations are in godot_host_resource_manager.cpp to avoid
 * triggering mass rebuilds when methods change.
 */
class GodotHostResourceManager {
public:
    GodotHostResourceManager();
    ~GodotHostResourceManager();

    GodotHostResourceManager(const GodotHostResourceManager&) = delete;
    GodotHostResourceManager& operator=(const GodotHostResourceManager&) = delete;
    GodotHostResourceManager(GodotHostResourceManager&&) = delete;
    GodotHostResourceManager& operator=(GodotHostResourceManager&&) = delete;

    /**
     * FACTORY - one host per boot
     *
     * Tears down a previous host first (Reiniciar), then: create the host with this manager's
     * log sink, read the bundle index, read the staged manifest, load the plugin, check the
     * port, configure every start patch, start, read the first snapshot.
     *
     * @return HostStatusOk, or the status of the first step that failed. failure_step() names
     *         that step. A host that failed after create() is KEPT, stopped, so its log still
     *         carries the cause until clear_all().
     */
    ase::kernel::HostStatus boot(const VivariumStartConfig& config);

    /**
     * RUNNING
     */
    /** Advance by dt seconds, capped at tick_max_seconds, then read the snapshot. */
    ase::kernel::HostStatus advance(float dt);
    /** Queue irrigate_amount of water for one patch; the plugin applies it in its next step. */
    ase::kernel::HostStatus irrigate(uint32_t object_id);
    /** One line of the view itself (boot line, action, measurement) through the host's logger. */
    ase::kernel::HostStatus note(const char* text);

    /**
     * PATCH VIEWS - copies of the last snapshot
     *
     * The pointer of get_patch is valid until the next advance(), boot() or clear_all(): every
     * snapshot rebuilds the views. Hold the id, never the pointer, across frames.
     */
    [[nodiscard]] const VivariumPatchView* get_patch(uint32_t object_id) const;
    [[nodiscard]] bool has_patch(uint32_t object_id) const;
    [[nodiscard]] uint32_t patch_count() const;

    /**
     * LOG LINES - any thread in, the node's thread out
     */
    void store_log_line(const char* line, uint32_t len, int level);
    /** Move every queued line into `out` (appended); returns how many were moved. */
    uint32_t remove_log_lines(std::vector<std::string>& out);

    /**
     * STATE AND MEASUREMENT
     */
    [[nodiscard]] bool running() const;
    [[nodiscard]] bool has_host() const;
    [[nodiscard]] const char* failure_step() const;
    [[nodiscard]] ase::kernel::HostStatus failure_status() const;
    [[nodiscard]] const VivariumBundleInfo& get_bundle() const;
    [[nodiscard]] double simulation_seconds() const;
    [[nodiscard]] uint64_t tick_count() const;
    [[nodiscard]] uint32_t system_count() const;

    /** Readable name of a HostStatus for the error panel and the log. */
    [[nodiscard]] static const char* status_name(ase::kernel::HostStatus status);

    /**
     * BULK CLEANUP - stop and destroy the host, drop every view and the bundle info.
     * Idempotent; the destructor calls it.
     */
    void clear_all();

private:
    ase::kernel::HostStatus fail(const char* step, ase::kernel::HostStatus status);
    ase::kernel::HostStatus read_bundle();
    ase::kernel::HostStatus read_snapshot();

    // The log queue is declared BEFORE the host: members die in reverse order, so the host - and
    // the logger it owns, whose sink writes into this queue - is gone before the queue is.
    std::mutex                                       log_mutex_;
    std::vector<std::string>                         log_lines_;

    VivariumStartConfig                              config_;
    VivariumBundleInfo                               bundle_;
    std::vector<ase::kernel::KernelHostRecord>       records_;
    std::unordered_map<uint32_t, VivariumPatchView>  patches_;
    const char*                                      failure_step_;
    ase::kernel::HostStatus                          failure_status_;
    double                                           simulation_seconds_ = 0.0;
    uint64_t                                         tick_count_ = 0;
    std::unique_ptr<ase::kernel::KernelEmbeddedHost> host_;
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE RESOURCE MANAGER (NOT A COMPONENT!)
 *
 * @file        godot_host_resource_manager.hpp
 * @brief       GodotHostResourceManager - External resource manager for the embedded ASE host of AseVivariumView
 * @description Holds the ONE std::unique_ptr<ase::kernel::KernelEmbeddedHost> of a Vivarium
 *              view, the start configuration it was booted with, the bundle it loaded, the copies
 *              of the last snapshots of the patch port and the clock port, and the queue of log
 *              lines the host's logging thread hands over. Lives OUTSIDE every ECS registry - the
 *              App inside the host keeps its own - and carries no Godot type, so its tests run
 *              against the real bundle without Godot.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-07
 * @version     00.00.02.00002 [seed]
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
 *   resources.boot(config);                      // factory: create, data root, load the set, start
 *
 *   // VivariumView::process (Godot's _process)
 *   resources.advance(elapsed_s);                // the real time since the last frame, never capped
 *   const VivariumPatchView* patch = resources.get_patch(selected_id);
 *
 *   // Reiniciar and VivariumView::exit_tree() (Godot's _exit_tree)
 *   resources.clear_all();                       // the binding teardown, idempotent
 *
 * THIS IS NOT A COMPONENT!
 * This class holds an external C++ object (the embedded host, its units, its logger). The node
 * stores ONLY uint32_t object ids, the manager owns everything behind them.
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

/**
 * Reads one file of the staged bundle by its name (the bundle index, a manifest it names) into
 * `out`; false when the stage holds no such file. The node reads through Godot's FileAccess, so
 * the stage may lie inside the exported package (Android); a test reads the build's bundle
 * directory. The manager itself never opens a stage file.
 */
using VivariumStageReadFn = bool (*)(const char* name, std::string& out, void* user);

/**
 * Everything boot() and self_test() need. The node fills it from res://config/vivarium_start.json
 * and from the stage Godot loaded this library from; no value of it lives in a unit.
 */
struct VivariumStartConfig {
    VivariumStageReadFn read_stage = nullptr;  // REQUIRED: reads the bundle index and the manifests
    void*       read_stage_user = nullptr;     // handed back to read_stage unchanged
    std::string library_dir;         // ABSOLUTE directory the unit libraries lie in; EMPTY: every
                                     // library is opened by its file name and the platform linker
                                     // resolves it in the app's own namespace (Android, PLAN 02.1)
    std::string files_dir;           // ABSOLUTE directory of the stage on this device's file system:
                                     // the bundle's data root lies below it (the stage itself on a
                                     // desktop, the copy of its data files where the stage is a package)
    std::string log_file;            // ABSOLUTE log file under user://, empty = callback only
    std::string patch_port;          // host port of the patches (ase-pl-flora: flora.patch.v1)
    std::string clock_port;          // host port of the game clock (ase-pl-flora: flora.clock.v1)
    std::string op_irrigate;         // submit operation that pours on one patch
    double      irrigate_amount_mm = 0.0;  // water one press of Regar pours, mm
    uint32_t    catch_up_steps = 0u;       // most ticks one frame may run while the host catches up
};

/** One unit as the bundle index names it. */
struct VivariumBundleUnit {
    std::string unit;         // unit name (ase-hub, ase-pl-flora, ...)
    std::string library;      // file name of its library
    std::string manifest;     // file name of its staged manifest
    std::string version;      // its version
    std::string api_version;  // the ASE API it was built against
};

/** What the bundle index says: the units, the data root and the data files below it. */
struct VivariumBundleInfo {
    std::string                     data_root;  // directory of the data files, relative to the stage
    std::vector<VivariumBundleUnit> units;      // every unit, in the index's order
    std::vector<std::string>        data;       // every data file, relative to the data root
};

/**
 * What one self-test run counted. A scenario that could not run to its end (no host, a unit not
 * loadable, a broken table entry) counts as one error of its own - a self-test that did not run
 * is never a self-test without errors.
 */
struct VivariumSelfTestResult {
    uint32_t cases = 0;    // scenarios run to their end, in order on the one test host
    uint32_t checks = 0;   // comparisons made: one per expected value, status and record count
    uint32_t errors = 0;   // failed comparisons, runs no action checked, and scenarios that could not run
};

/**
 * The state one scenario of the self-test carries from action to action: the values it
 * remembered for a later `plus`, the snapshot buffer, and the runs it checked.
 */
struct VivariumSelfTestState {
    std::unordered_map<std::string, double>     remembered;
    std::vector<ase::kernel::KernelHostRecord>  records;
    std::vector<std::string>                    checked;
    double                                      tolerance = 0.0;
    double                                      frame_s = 0.0;
};

/** One patch as the last snapshot showed it - a copy, never a reference into the App. */
struct VivariumPatchView {
    uint32_t object_id = 0;     // the patch, as the plugin names it on its port
    uint32_t place = 0;         // where the view draws it
    double   coverage = 0.0;    // covered share, 0..1
    uint8_t  stage = 0;         // GODOT_STAGE_* or GODOT_CODE_UNKNOWN
    uint8_t  condition = 0;     // GODOT_COND_* or GODOT_CODE_UNKNOWN
    double   soil_mm = 0.0;     // soil water, mm
    double   soil_rel = 0.0;    // soil water over field capacity, 0..1
    double   capacity_mm = 0.0; // field capacity, mm
};

/** The game clock as the last snapshot showed it. */
struct VivariumClockView {
    bool   known = false;     // false until a snapshot carried the clock record
    double elapsed_s = 0.0;   // game time, seconds
    double day = 0.0;         // game day
    double hour = 0.0;        // hour of the game day
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
     * The bundle index text, parsed: one `data_root`, one `unit` line per unit with six fields, a
     * `data` line per data file, '#' lines as comments. false - and `out` emptied - for an index
     * without data root or unit, a unit line of another shape, or a line of an unknown key: an
     * index this adapter does not understand is never half read.
     */
    [[nodiscard]] static bool parse_index(const std::string& text, VivariumBundleInfo& out);

    /**
     * FACTORY - one host per boot
     *
     * Tears down a previous host first (Reiniciar), then: create the host with this manager's
     * log sink, read the bundle index and every staged manifest, name the data root, load the
     * whole set in the order of its manifests, check both ports, start, read the first snapshots.
     * The plugin creates its patches itself; the view finds them by their place.
     *
     * @return HostStatusOk, or the status of the first step that failed. failure_step() names
     *         that step. A host that failed after create() is KEPT, stopped, so its log still
     *         carries the cause until clear_all().
     */
    ase::kernel::HostStatus boot(const VivariumStartConfig& config);

    /**
     * SELF-TEST (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3) - the runs of the composition test
     * (config/vivarium_selftest.toml) through the real host facade and the real set of the
     * stage, BEFORE any game host: ONE test host is created and started once, every scenario
     * runs on it in the table's order, each a check of one step within a few frames, and it is
     * stopped and destroyed before the game host exists - never a second simulation beside
     * another, never a second start for the next scenario. The test host writes no log file -
     * opening one would rotate the game host's file - and hands its lines to this manager's sink
     * like the game host does.
     *
     * Refused (one error, nothing run) while this manager holds a host: the game host boots
     * after the self-test, never beside it.
     *
     * @param config  the start configuration boot() takes: stage reader, directories, ports
     * @param table   the text of the self-test table
     * @param report  receives one line per comparison ("selftest check=pour place=0 key=soil_mm
     *                actual=... expected=gt ... result=ok") and one closing line with the counts
     */
    VivariumSelfTestResult self_test(const VivariumStartConfig& config, const std::string& table,
                                     std::vector<std::string>& report);

    /**
     * RUNNING
     */
    /**
     * Hand the host `elapsed_s` of real time (never capped, never dropped - the world has no
     * pause) and read both snapshots. The host ticks at most catch_up_steps times in this call;
     * what remains waits in its backlog for the next frame (KernelEmbeddedHost::advance).
     */
    ase::kernel::HostStatus advance(double elapsed_s);
    /** Pour irrigate_amount_mm on one patch; the plugin takes it in its next Reception run. */
    ase::kernel::HostStatus irrigate(uint32_t object_id);
    /** One line of the view itself (boot line, action, measurement) through the host's logger. */
    ase::kernel::HostStatus note(const char* text);

    /**
     * VIEWS - copies of the last snapshots
     *
     * The pointers are valid until the next advance(), boot() or clear_all(): every snapshot
     * rebuilds the views. Hold the id or the place, never the pointer, across frames.
     */
    [[nodiscard]] const VivariumPatchView* get_patch(uint32_t object_id) const;
    [[nodiscard]] const VivariumPatchView* get_patch_at(uint32_t place) const;
    [[nodiscard]] bool has_patch(uint32_t object_id) const;
    [[nodiscard]] uint32_t patch_count() const;
    [[nodiscard]] const VivariumClockView& get_clock() const;

    /**
     * LOG LINES - any thread in, the node's thread out
     *
     * On Android the line goes straight to logcat (__android_log_write, tag GODOT_LOG_SOURCE)
     * from the thread that wrote it and is NOT queued: logcat is the platform's console, and a
     * line held for the next frame would be lost with a process that never reaches it. Elsewhere
     * it waits in the queue for the node, which prints it to Godot's output.
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
    /** Real seconds the host has simulated: handed in, less what still waits in its backlog. */
    [[nodiscard]] double simulation_seconds() const;
    /** Ticks the host ran since boot, all calls together. */
    [[nodiscard]] uint64_t tick_count() const;
    /** Ticks of the last advance() and the real seconds still waiting after it. */
    [[nodiscard]] uint32_t last_steps() const;
    [[nodiscard]] double backlog_seconds() const;
    [[nodiscard]] uint32_t system_count() const;
    [[nodiscard]] uint32_t unit_count() const;

    /**
     * The proportional set size of this process in KiB, read from /proc/self/smaps_rollup - the
     * memory figure of acceptance A10 and of the reset check (PLAN 02.3). false, and out 0, when
     * the kernel offers no rollup: a missing measurement is never reported as a small one.
     */
    [[nodiscard]] bool read_pss_kib(uint64_t& out) const;

    /**
     * How often the host's App ran the named schedule - counted by its tick scheduler, never
     * derived here from simulated time. Answers like KernelEmbeddedHost::schedule_runs;
     * HostStatusInvalidState without a running host, which is then not asked at all. runs is 0
     * on every answer but HostStatusOk.
     */
    [[nodiscard]] ase::kernel::HostStatus schedule_runs(const char* schedule, uint64_t* runs) const;

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
    ase::kernel::HostStatus read_manifests();
    ase::kernel::HostStatus load_set(ase::kernel::KernelEmbeddedHost& host);
    ase::kernel::HostStatus read_snapshot();
    /** The string load_units takes for one library: its path under library_dir, or its bare name. */
    [[nodiscard]] std::string library_path(const std::string& library) const;

    // The log queue is declared BEFORE the host: members die in reverse order, so the host - and
    // the logger it owns, whose sink writes into this queue - is gone before the queue is.
    std::mutex                                       log_mutex_;
    std::vector<std::string>                         log_lines_;

    VivariumStartConfig                              config_;
    VivariumBundleInfo                               bundle_;
    std::vector<std::string>                         manifests_;   // text of each unit's manifest, index order
    std::vector<ase::kernel::KernelHostRecord>       records_;
    std::unordered_map<uint32_t, VivariumPatchView>  patches_;
    std::unordered_map<uint32_t, uint32_t>           places_;      // place → object id of the last snapshot
    VivariumClockView                                clock_;
    const char*                                      failure_step_;
    ase::kernel::HostStatus                          failure_status_;
    double                                           elapsed_in_s_ = 0.0;
    double                                           backlog_s_ = 0.0;
    uint64_t                                         tick_count_ = 0;
    uint32_t                                         last_steps_ = 0;
    std::unique_ptr<ase::kernel::KernelEmbeddedHost> host_;
};

}  // namespace ase::adp::godot

/**
 * ASE RESOURCE MANAGER IMPLEMENTATION (OOP-ECS Bridge)
 *
 * @file        godot_host_resource_manager.cpp
 * @brief       GodotHostResourceManager - External resource manager for the embedded ASE host of AseVivariumView
 * @description Flyweight Pattern implementation for unique_ptr<KernelEmbeddedHost> and the
 *              patch views copied out of its port snapshot.
 *              The node stores ONLY uint32_t object ids, the manager owns everything behind them.
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
 * WHY THIS EXISTS:
 *   PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.2: AseVivariumView owns exactly ONE KernelEmbeddedHost,
 *   and "der ResourceManager kapselt Factory/Fehler/Abbau". The Godot node knows ids, rectangles
 *   and colours; everything that touches the host - the bundle, the plugin, the port, the
 *   teardown - happens here, without a Godot type, so it is tested against the real plugin.
 *
 * WHAT IT NEVER DOES:
 *   It writes no simulation value back and computes no growth. The views are copies of what the
 *   port reported after the last tick; the stage flags are the port's own 0/1 values.
 *
 * THREAD SAFETY:
 *   The log sink is the only entry from a second thread (the host's logging worker); it only
 *   appends to log_lines_ under log_mutex_. Everything else runs on the thread that booted the
 *   host, and the host refuses any other.
 *
 * ECS RESOURCE MANAGER IMPLEMENTATION COMPLIANCE
 *
 * [ ] NOT a Component - lives outside ECS registry
 * [ ] Accessed via registry.ctx().get<ResourceManager&>()
 * [ ] Components store ONLY uint32_t IDs
 * [ ] Thread-safe via std::mutex
 * [ ] Proper cleanup in clear_all() - closes/stops BEFORE clearing
 * [ ] No ECS anti-patterns (this is intentionally OOP bridge code)
 * [ ] Implementations in .cpp to avoid mass rebuilds
 * [ ] Header contains ONLY declarations
 * [ ] Layer dependencies checked (only depend on lower layers)
 * [ ] NO file-level static/constexpr (constants → types.hpp)
 * [ ] Filename matches convention
 * [ ] 1 File = 1 ResourceManager
 * [ ] Folder structure matches convention (src/{category}/)
 * [ ] Layer dependencies respected (no upward dependencies)
 * [ ] NO std::shared_ptr in Components - stored HERE via Flyweight Pattern
 * [ ] External resources (shared_ptr, handles) accessed via registry.ctx()
 * [ ] ResourceManager registered in on_start() via registry.ctx().emplace<>()
 * [ ] clear_all() closes resources in REVERSE dependency order
 * [ ] clear_all() called from IniSystem::on_stop() or ShutdownSystem
 */

// INCLUDES - ONLY THESE ARE ALLOWED!
// FORBIDDEN: <vector>, <map>, <unordered_map>, <optional>, <algorithm>
// ALLOWED:   <cstdint>, <cmath>, <cassert>, ase-* headers
// EXCEPTION: This file uses std::unordered_map, std::string and std::vector
//            because ResourceManager is NOT a Component - it's an OOP bridge
//            to an external host (the Godot process that embeds ASE).

// Own header FIRST
#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/utils/strops.hpp>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

using ase::kernel::HostStatus;
using ase::kernel::KernelEmbeddedHost;
using ase::kernel::KernelHostInput;
using ase::kernel::KernelHostRecord;

namespace {

/**
 * The host's log sink. It may run on the logging worker, so it only hands the line to the
 * manager that booted the host (KernelHostLogFn contract: enqueue, never touch the scene).
 */
void queue_host_line(const char* line, uint32_t len, int level, void* user) {
    static_cast<GodotHostResourceManager*>(user)->store_log_line(line, len, level);
}

/** Append one named value; false when the input is already full. */
bool put_value(KernelHostInput& input, const char* key, double value) {
    if (input.value_count >= ase::kernel::HostValuesMax) {
        return false;
    }
    ase::utils::str_copy(input.values[input.value_count].key, ase::kernel::HostKeyBytes, key);
    input.values[input.value_count].value = value;
    input.value_count += 1u;
    return true;
}

/** The value stored under `key` in a snapshot record; NaN when the record does not carry it. */
double record_value(const KernelHostRecord& record, const char* key) {
    for (uint32_t i = 0u; i < record.value_count && i < ase::kernel::HostValuesMax; ++i) {
        if (ase::utils::str_equal(record.values[i].key, key, ase::kernel::HostKeyBytes)) {
            return record.values[i].value;
        }
    }
    return std::nan("");
}

/**
 * The value of `key = value` in the bundle index; empty when no line carries the key.
 * Lines starting with '#' are comments (the index writes a header line).
 */
std::string index_value(const std::vector<std::string>& lines, const char* key) {
    const std::string wanted(key);
    for (const std::string& line : lines) {
        if (line.empty() || line[0] == '#' || line.compare(0u, wanted.size(), wanted) != 0) {
            continue;
        }
        const std::size_t equals = line.find('=', wanted.size());
        // Only blanks may stand between key and '=': anything else is a longer key that merely
        // starts with `key`.
        if (equals == std::string::npos || line.find_first_not_of(' ', wanted.size()) != equals) {
            continue;
        }
        const std::size_t first = line.find_first_not_of(' ', equals + 1u);
        if (first == std::string::npos) {
            return std::string();
        }
        const std::size_t last = line.find_last_not_of(" \r");
        return line.substr(first, last - first + 1u);
    }
    return std::string();
}

}  // anonymous namespace

// =============================================================================
// LIFETIME
// =============================================================================

GodotHostResourceManager::GodotHostResourceManager()
    : failure_step_(GODOT_STEP_NONE), failure_status_(ase::kernel::HostStatusOk) {}

GodotHostResourceManager::~GodotHostResourceManager() {
    clear_all();
}

// =============================================================================
// FACTORY
// =============================================================================

HostStatus GodotHostResourceManager::boot(const VivariumStartConfig& config) {
    // Reiniciar runs the same teardown as _exit_tree: the old host, its views and its bundle go
    // first, so no id, input or drawing value of the previous run survives into this one.
    clear_all();
    config_ = config;
    failure_step_ = GODOT_STEP_NONE;
    failure_status_ = ase::kernel::HostStatusOk;
    simulation_seconds_ = 0.0;
    tick_count_ = 0u;

    // A configuration the host could never run is refused before a host exists: a zero cap
    // would turn every frame into a refused tick, an empty name into a NotFound three steps on.
    if (config_.bundle_dir.empty() || config_.port.empty() || config_.op_create.empty() ||
        config_.op_irrigate.empty() || !std::isfinite(config_.tick_max_seconds) ||
        config_.tick_max_seconds <= 0.0f || !std::isfinite(config_.irrigate_amount)) {
        return fail(GODOT_STEP_CONFIG, ase::kernel::HostStatusInvalidArgument);
    }

    const char* log_file = config_.log_file.empty() ? nullptr : config_.log_file.c_str();
    HostStatus status = KernelEmbeddedHost::create(host_, &queue_host_line, this, log_file);
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_CREATE, status);
    }

    status = read_bundle();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_BUNDLE, status);
    }

    const std::string manifest = ase::fileio::read_text(config_.bundle_dir + "/" + bundle_.manifest);
    if (manifest.empty()) {
        return fail(GODOT_STEP_MANIFEST, ase::kernel::HostStatusNotFound);
    }

    const std::string library = config_.bundle_dir + "/" + bundle_.library;
    status = host_->load_plugin(library.c_str(), manifest.data(),
                                static_cast<uint32_t>(manifest.size()));
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_LOAD, status);
    }

    if (!host_->has_port(config_.port.c_str())) {
        return fail(GODOT_STEP_PORT, ase::kernel::HostStatusNotFound);
    }

    for (const VivariumPatchStart& patch : config_.patches) {
        KernelHostInput input{};
        input.object_id = patch.object_id;
        if (!put_value(input, GODOT_KEY_BIOMASS, config_.start_biomass) ||
            !put_value(input, GODOT_KEY_MOISTURE, patch.moisture) ||
            !put_value(input, GODOT_KEY_AGE, config_.start_age_seconds)) {
            return fail(GODOT_STEP_CONFIGURE, ase::kernel::HostStatusCapacity);
        }
        status = host_->configure(config_.port.c_str(), config_.op_create.c_str(), &input);
        if (status != ase::kernel::HostStatusOk) {
            return fail(GODOT_STEP_CONFIGURE, status);
        }
    }

    status = host_->start();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_START, status);
    }

    // One record per configured patch; never zero, so the buffer always has an address.
    records_.assign(config_.patches.empty() ? 1u : config_.patches.size(), KernelHostRecord{});
    status = read_snapshot();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_SNAPSHOT, status);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::fail(const char* step, HostStatus status) {
    failure_step_ = step;
    failure_status_ = status;
    patches_.clear();
    if (!host_) {
        return status;  // no host, no logger: the node shows and prints the failure itself
    }
    // The host stays, STOPPED: its plugin is unloaded, but its logger still carries this line.
    (void)host_->stop();
    std::string line = "boot or run failed at ";
    line += step;
    line += ": ";
    line += status_name(status);
    line += " (";
    line += std::to_string(status);
    line += ")";
    (void)host_->note(GODOT_LOG_SOURCE, line.c_str());
    return status;
}

HostStatus GodotHostResourceManager::read_bundle() {
    const std::string index = config_.bundle_dir + "/" + GODOT_BUNDLE_INDEX;
    if (!ase::fileio::file_exists(index)) {
        return ase::kernel::HostStatusNotFound;
    }
    const std::vector<std::string> lines = ase::fileio::read_lines(index);
    bundle_.library = index_value(lines, GODOT_BUNDLE_KEY_LIBRARY);
    bundle_.manifest = index_value(lines, GODOT_BUNDLE_KEY_MANIFEST);
    bundle_.plugin = index_value(lines, GODOT_BUNDLE_KEY_PLUGIN);
    bundle_.version = index_value(lines, GODOT_BUNDLE_KEY_VERSION);
    bundle_.api_version = index_value(lines, GODOT_BUNDLE_KEY_API);
    if (bundle_.library.empty() || bundle_.manifest.empty() || bundle_.plugin.empty() ||
        bundle_.version.empty() || bundle_.api_version.empty()) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    return ase::kernel::HostStatusOk;
}

// =============================================================================
// RUNNING
// =============================================================================

HostStatus GodotHostResourceManager::advance(float dt) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    // Capped only when it IS a number: NaN and infinity reach the host unchanged and are refused
    // there with its own warning - capping them would turn a broken frame into a valid tick.
    // Longer stalls slow the prototype down; there is no catch-up loop beside the scheduler.
    const float step = (std::isfinite(dt) && dt > config_.tick_max_seconds)
                           ? config_.tick_max_seconds
                           : dt;
    const HostStatus status = host_->tick(step);
    if (status != ase::kernel::HostStatusOk) {
        return status;  // this frame is dropped; the host said why
    }
    simulation_seconds_ += static_cast<double>(step);
    tick_count_ += 1u;

    const HostStatus read = read_snapshot();
    if (read != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_SNAPSHOT, read);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_snapshot() {
    uint32_t written = 0u;
    uint32_t required = 0u;
    HostStatus status = host_->snapshot(config_.port.c_str(), records_.data(),
                                        static_cast<uint32_t>(records_.size()), &written,
                                        &required);
    if (status == ase::kernel::HostStatusCapacity) {
        // The plugin holds more patches than were configured: grow once to its own answer.
        records_.assign(required, KernelHostRecord{});
        status = host_->snapshot(config_.port.c_str(), records_.data(),
                                 static_cast<uint32_t>(records_.size()), &written, &required);
    }
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }

    patches_.clear();
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        VivariumPatchView view;
        view.object_id = record.object_id;
        view.biomass = record_value(record, GODOT_KEY_BIOMASS);
        view.moisture = record_value(record, GODOT_KEY_MOISTURE);
        view.age_seconds = record_value(record, GODOT_KEY_AGE);
        const double seed = record_value(record, GODOT_KEY_SEED);
        const double sprout = record_value(record, GODOT_KEY_SPROUT);
        const double mature = record_value(record, GODOT_KEY_MATURE);
        const double dead = record_value(record, GODOT_KEY_DEAD);
        // A record that does not carry the port contract is a broken port, not a value to draw.
        if (!std::isfinite(view.biomass) || !std::isfinite(view.moisture) ||
            !std::isfinite(view.age_seconds) || !std::isfinite(seed) || !std::isfinite(sprout) ||
            !std::isfinite(mature) || !std::isfinite(dead)) {
            patches_.clear();
            return ase::kernel::HostStatusInvalidArgument;
        }
        view.seed = seed >= GODOT_SNAPSHOT_FLAG_SET;
        view.sprout = sprout >= GODOT_SNAPSHOT_FLAG_SET;
        view.mature = mature >= GODOT_SNAPSHOT_FLAG_SET;
        view.dead = dead >= GODOT_SNAPSHOT_FLAG_SET;
        patches_[view.object_id] = view;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::irrigate(uint32_t object_id) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    input.object_id = object_id;
    if (!put_value(input, GODOT_KEY_AMOUNT, config_.irrigate_amount)) {
        return ase::kernel::HostStatusCapacity;
    }
    return host_->submit(config_.port.c_str(), config_.op_irrigate.c_str(), &input);
}

HostStatus GodotHostResourceManager::note(const char* text) {
    if (!host_) {
        return ase::kernel::HostStatusInvalidState;
    }
    return host_->note(GODOT_LOG_SOURCE, text);
}

// =============================================================================
// PATCH VIEWS
// =============================================================================

const VivariumPatchView* GodotHostResourceManager::get_patch(uint32_t object_id) const {
    const auto found = patches_.find(object_id);
    return found == patches_.end() ? nullptr : &found->second;
}

bool GodotHostResourceManager::has_patch(uint32_t object_id) const {
    return patches_.find(object_id) != patches_.end();
}

uint32_t GodotHostResourceManager::patch_count() const {
    return static_cast<uint32_t>(patches_.size());
}

// =============================================================================
// LOG LINES
// =============================================================================

void GodotHostResourceManager::store_log_line(const char* line, uint32_t len, int level) {
    (void)level;  // the line carries its level tag already
    if (line == nullptr) {
        return;
    }
    // The node prints line by line; the trailing line break of the formatter would double it.
    while (len > 0u && (line[len - 1u] == '\n' || line[len - 1u] == '\r')) {
        len -= 1u;
    }
    const std::lock_guard<std::mutex> lock(log_mutex_);
    log_lines_.emplace_back(line, len);
}

uint32_t GodotHostResourceManager::remove_log_lines(std::vector<std::string>& out) {
    const std::lock_guard<std::mutex> lock(log_mutex_);
    const auto moved = static_cast<uint32_t>(log_lines_.size());
    for (std::string& line : log_lines_) {
        out.push_back(std::move(line));
    }
    log_lines_.clear();
    return moved;
}

// =============================================================================
// STATE AND MEASUREMENT
// =============================================================================

bool GodotHostResourceManager::running() const {
    return host_ && host_->state() == ase::kernel::HostStateRunning;
}

bool GodotHostResourceManager::has_host() const {
    return host_ != nullptr;
}

const char* GodotHostResourceManager::failure_step() const {
    return failure_step_;
}

HostStatus GodotHostResourceManager::failure_status() const {
    return failure_status_;
}

const VivariumBundleInfo& GodotHostResourceManager::get_bundle() const {
    return bundle_;
}

double GodotHostResourceManager::simulation_seconds() const {
    return simulation_seconds_;
}

uint64_t GodotHostResourceManager::tick_count() const {
    return tick_count_;
}

uint32_t GodotHostResourceManager::system_count() const {
    return host_ ? host_->system_count() : 0u;
}

const char* GodotHostResourceManager::status_name(HostStatus status) {
    if (status == ase::kernel::HostStatusOk) {
        return "Ok";
    }
    if (status == ase::kernel::HostStatusInvalidState) {
        return "InvalidState";
    }
    if (status == ase::kernel::HostStatusInvalidArgument) {
        return "InvalidArgument";
    }
    if (status == ase::kernel::HostStatusNotFound) {
        return "NotFound";
    }
    if (status == ase::kernel::HostStatusCapacity) {
        return "Capacity";
    }
    if (status == ase::kernel::HostStatusAbiMismatch) {
        return "AbiMismatch";
    }
    return "Unknown";
}

// =============================================================================
// BULK CLEANUP
// =============================================================================

void GodotHostResourceManager::clear_all() {
    // Stop BEFORE clearing: the host runs the binding Phase 00 teardown (App shutdown, ports
    // unregistered, App destroyed, dlclose) and releases its logger last. Lines it still writes
    // stay in log_lines_ for the node to print.
    if (host_) {
        (void)host_->stop();
        host_.reset();
    }
    patches_.clear();
    records_.clear();
    bundle_ = VivariumBundleInfo{};
}

}  // namespace ase::adp::godot

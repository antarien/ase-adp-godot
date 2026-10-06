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
 *   appends to log_lines_ under log_mutex_ - or, on Android, hands the line to logcat, which is
 *   thread-safe itself. Everything else runs on the thread that booted the host, and the host
 *   refuses any other.
 *
 * WHERE THE BUNDLE COMES FROM (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1):
 *   The manager never opens a stage file. The caller's VivariumStageReadFn reads the bundle index
 *   and the manifest it names - through Godot's FileAccess in the app, so a stage packed into an
 *   APK reads like one on disk. The plugin library is opened under library_dir, or, with an
 *   empty library_dir, by its bare file name, which Android's linker resolves in the app's own
 *   namespace; neither res:// nor a package path ever reaches dlopen.
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
#include <ase/math/math.hpp>
#include <ase/utils/strops.hpp>

#include <toml++/toml.hpp>

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

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
 * The value of `key = value` in the bundle index text; empty when no line carries the key.
 * Lines starting with '#' are comments (the index writes a header line); a CR before the line
 * break is not part of the value.
 */
std::string index_value(const std::string& text, const char* key) {
    const std::string wanted(key);
    std::size_t start = 0u;
    while (start < text.size()) {
        const std::size_t newline = text.find('\n', start);
        const std::size_t end = newline == std::string::npos ? text.size() : newline;
        const std::string line = text.substr(start, end - start);
        start = end + 1u;
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

// ── self-test (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3) ──────────────────────────────────

/** One number of a report line: fixed decimals; nan and inf stay words (ase-utils). */
std::string decimal(double value) {
    char text[GODOT_NUMBER_BYTES] = {};
    ase::utils::str_append_f32(text, GODOT_NUMBER_BYTES, static_cast<float>(value),
                               GODOT_SELFTEST_DECIMALS);
    return std::string(text);
}

/** A scenario's name in the report: the cases of the plan its actions check, each once. */
std::string scenario_label(const toml::table& scenario) {
    std::vector<std::string> checks;
    const toml::array* actions = scenario[GODOT_SELFTEST_KEY_DO].as_array();
    if (actions != nullptr) {
        for (const toml::node& action : *actions) {
            const toml::table* entry = action.as_table();
            const std::string check =
                entry == nullptr ? std::string() : (*entry)[GODOT_SELFTEST_KEY_CHECK].value_or(std::string());
            bool known = check.empty();
            for (const std::string& seen : checks) {
                known = known || seen == check;
            }
            if (!known) {
                checks.push_back(check);
            }
        }
    }
    std::string label;
    for (const std::string& check : checks) {
        label += label.empty() ? check : " " + check;
    }
    return label.empty() ? std::string("?") : label;
}

/** Every record the port holds; the buffer grows once to the port's own answer. */
HostStatus snapshot_all(KernelEmbeddedHost& host, const std::string& port,
                        std::vector<KernelHostRecord>& records, uint32_t& written) {
    uint32_t required = 0u;
    written = 0u;
    if (records.empty()) {
        records.assign(1u, KernelHostRecord{});
    }
    HostStatus status = host.snapshot(port.c_str(), records.data(), static_cast<uint32_t>(records.size()),
                                      &written, &required);
    if (status == ase::kernel::HostStatusCapacity) {
        records.assign(required, KernelHostRecord{});
        status = host.snapshot(port.c_str(), records.data(), static_cast<uint32_t>(records.size()),
                               &written, &required);
    }
    return status;
}

/** One comparison of the self-test: counted, and written as one report line. */
void report_check(const std::string& check, const std::string& subject, const std::string& actual,
                  const std::string& expected, bool ok, VivariumSelfTestResult& result,
                  std::vector<std::string>& report) {
    result.checks += 1u;
    if (!ok) {
        result.errors += 1u;
    }
    report.push_back("selftest check=" + (check.empty() ? std::string("?") : check) + " " + subject +
                     " actual=" + actual + " expected=" + expected + " result=" + (ok ? "ok" : "FAILED"));
}

/** A scenario or a table that could not run to its end: one error, the step and its cause. */
void report_broken(const std::string& label, const char* step, const std::string& cause,
                   VivariumSelfTestResult& result, std::vector<std::string>& report) {
    result.errors += 1u;
    report.push_back("selftest scenario=\"" + label + "\" step=" + step + " cause=" + cause +
                     " result=FAILED");
}

/**
 * Configures the scenario's patches on a loaded host and starts it; false (reported) when a patch
 * is malformed or the host refuses it.
 */
bool start_scenario(KernelEmbeddedHost& host, const toml::table& scenario, const VivariumStartConfig& config,
                    const std::string& label, VivariumSelfTestResult& result, std::vector<std::string>& report) {
    const toml::array* patches = scenario[GODOT_SELFTEST_KEY_PATCHES].as_array();
    if (patches == nullptr || patches->empty()) {
        report_broken(label, GODOT_STEP_CONFIGURE, "no patches", result, report);
        return false;
    }
    for (const toml::node& node : *patches) {
        const toml::table* patch = node.as_table();
        const int64_t id = patch == nullptr ? -1 : (*patch)[GODOT_SELFTEST_KEY_ID].value_or(int64_t{-1});
        const double nothing = std::nan("");
        KernelHostInput input{};
        if (patch == nullptr || id <= static_cast<int64_t>(GODOT_NO_OBJECT) || id > INT32_MAX ||
            !put_value(input, GODOT_KEY_BIOMASS, (*patch)[GODOT_KEY_BIOMASS].value_or(nothing)) ||
            !put_value(input, GODOT_KEY_MOISTURE, (*patch)[GODOT_KEY_MOISTURE].value_or(nothing)) ||
            !put_value(input, GODOT_KEY_AGE, (*patch)[GODOT_KEY_AGE].value_or(nothing))) {
            report_broken(label, GODOT_STEP_CONFIGURE, "patch without id or start values", result, report);
            return false;
        }
        input.object_id = static_cast<uint32_t>(id);
        const HostStatus status = host.configure(config.port.c_str(), config.op_create.c_str(), &input);
        if (status != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_CONFIGURE, GodotHostResourceManager::status_name(status), result,
                          report);
            return false;
        }
    }
    const HostStatus status = host.start();
    if (status != ase::kernel::HostStatusOk) {
        report_broken(label, GODOT_STEP_START, GodotHostResourceManager::status_name(status), result, report);
        return false;
    }
    return true;
}

/**
 * Runs one action of a started scenario. Answers false (reported) when the action cannot run -
 * then the scenario ends there; a comparison that FAILS is counted and the scenario goes on, so
 * one wrong value never hides the next.
 */
bool run_action(KernelEmbeddedHost& host, const toml::table& action, const VivariumStartConfig& config,
                double tolerance, double tick, const std::string& label, std::vector<KernelHostRecord>& records,
                VivariumSelfTestResult& result, std::vector<std::string>& report) {
    const double nothing = std::nan("");
    const std::string check = action[GODOT_SELFTEST_KEY_CHECK].value_or(std::string());

    if (const toml::node* seconds_node = action.get(GODOT_SELFTEST_KEY_SECONDS)) {
        const double seconds = seconds_node->value_or(nothing);
        const double ticks = seconds / tick;
        const double whole = static_cast<double>(ase::math::round(static_cast<float>(ticks)));
        if (!(seconds > 0.0) || !(std::fabs(ticks - whole) <= GODOT_SELFTEST_TICK_EXACT)) {
            report_broken(label, GODOT_SELFTEST_KEY_SECONDS, "not a whole number of ticks: " + decimal(seconds),
                          result, report);
            return false;
        }
        for (double done = 0.0; done < whole; done += 1.0) {
            const HostStatus status = host.tick(static_cast<float>(tick));
            if (status != ase::kernel::HostStatusOk) {
                report_broken(label, GODOT_STEP_TICK, GodotHostResourceManager::status_name(status), result,
                              report);
                return false;
            }
        }
        return true;
    }

    if (const toml::table* irrigate = action[GODOT_SELFTEST_KEY_IRRIGATE].as_table()) {
        const int64_t id = (*irrigate)[GODOT_SELFTEST_KEY_ID].value_or(int64_t{-1});
        const toml::node* amount = irrigate->get(GODOT_KEY_AMOUNT);
        const std::string expected = (*irrigate)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        KernelHostInput input{};
        if (id < 0 || id > INT32_MAX || amount == nullptr || !amount->is_number() || expected.empty() ||
            !put_value(input, GODOT_KEY_AMOUNT, amount->value_or(nothing))) {
            report_broken(label, GODOT_SELFTEST_KEY_IRRIGATE, "needs id, amount and status", result, report);
            return false;
        }
        input.object_id = static_cast<uint32_t>(id);
        const std::string actual = GodotHostResourceManager::status_name(
            host.submit(config.port.c_str(), config.op_irrigate.c_str(), &input));
        report_check(check, "target=" + std::to_string(id) + " action=irrigate amount=" +
                                decimal(amount->value_or(nothing)),
                     actual, expected, actual == expected, result, report);
        return true;
    }

    if (const toml::table* expect = action[GODOT_SELFTEST_KEY_EXPECT].as_table()) {
        const int64_t id = (*expect)[GODOT_SELFTEST_KEY_ID].value_or(int64_t{-1});
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(host, config.port, records, written);
        if (id < 0 || status != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_SNAPSHOT,
                          id < 0 ? std::string("expect needs an id") : GodotHostResourceManager::status_name(status),
                          result, report);
            return false;
        }
        const KernelHostRecord* record = nullptr;
        for (uint32_t i = 0u; i < written; ++i) {
            if (records[i].object_id == static_cast<uint32_t>(id)) {
                record = &records[i];
            }
        }
        for (auto&& [key, value] : *expect) {
            const std::string name(key.str());
            if (name == GODOT_SELFTEST_KEY_ID) {
                continue;
            }
            const double wanted = value.value_or(nothing);
            const double actual = record == nullptr ? nothing : record_value(*record, name.c_str());
            const bool ok = std::isfinite(wanted) && std::isfinite(actual) && std::fabs(actual - wanted) <= tolerance;
            report_check(check, "target=" + std::to_string(id) + " key=" + name, decimal(actual), decimal(wanted),
                         ok, result, report);
        }
        return true;
    }

    if (const toml::node* count = action.get(GODOT_SELFTEST_KEY_RECORDS)) {
        const int64_t expected = count->value_or(int64_t{-1});
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(host, config.port, records, written);
        if (expected < 0 || status != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_SNAPSHOT,
                          expected < 0 ? std::string("records needs a count") : GodotHostResourceManager::status_name(status),
                          result, report);
            return false;
        }
        report_check(check, "records", std::to_string(written), std::to_string(expected),
                     static_cast<int64_t>(written) == expected, result, report);
        return true;
    }

    report_broken(label, GODOT_SELFTEST_KEY_DO, "an action names none of seconds, irrigate, expect, records",
                  result, report);
    return false;
}

/**
 * One scenario on a test host of its own: create it (no log file - the game host's file must not
 * rotate - and this manager's sink), load the stage's plugin, configure, start, run the actions
 * in order, stop. The host is destroyed when this function returns, before the next scenario
 * creates its own: never two simulations at once.
 */
void run_scenario(const toml::table& scenario, const VivariumStartConfig& config, const std::string& library,
                  const std::string& manifest, double tolerance, double tick, GodotHostResourceManager* sink,
                  VivariumSelfTestResult& result, std::vector<std::string>& report) {
    const std::string label = scenario_label(scenario);
    std::unique_ptr<KernelEmbeddedHost> host;
    HostStatus status = KernelEmbeddedHost::create(host, &queue_host_line, sink, nullptr);
    if (status != ase::kernel::HostStatusOk) {
        report_broken(label, GODOT_STEP_CREATE, GodotHostResourceManager::status_name(status), result, report);
        return;
    }
    status = host->load_plugin(library.c_str(), manifest.data(), static_cast<uint32_t>(manifest.size()));
    if (status != ase::kernel::HostStatusOk) {
        // A failed load has torn the host down already; its lines name the cause.
        report_broken(label, GODOT_STEP_LOAD, GodotHostResourceManager::status_name(status), result, report);
        return;
    }
    if (!start_scenario(*host, scenario, config, label, result, report)) {
        (void)host->stop();
        return;
    }
    const toml::array* actions = scenario[GODOT_SELFTEST_KEY_DO].as_array();
    if (actions == nullptr) {
        report_broken(label, GODOT_SELFTEST_KEY_DO, "no actions", result, report);
        (void)host->stop();
        return;
    }
    std::vector<KernelHostRecord> records;
    for (const toml::node& node : *actions) {
        const toml::table* action = node.as_table();
        if (action == nullptr) {
            report_broken(label, GODOT_SELFTEST_KEY_DO, "an action is no table", result, report);
            (void)host->stop();
            return;
        }
        if (!run_action(*host, *action, config, tolerance, tick, label, records, result, report)) {
            (void)host->stop();
            return;
        }
    }
    status = host->stop();
    if (status != ase::kernel::HostStatusOk) {
        report_broken(label, "stop", GodotHostResourceManager::status_name(status), result, report);
        return;
    }
    result.cases += 1u;
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
    if (config_.read_stage == nullptr || config_.port.empty() || config_.op_create.empty() ||
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

    std::string manifest;
    if (!config_.read_stage(bundle_.manifest.c_str(), manifest, config_.read_stage_user) || manifest.empty()) {
        return fail(GODOT_STEP_MANIFEST, ase::kernel::HostStatusNotFound);
    }

    const std::string library = plugin_library();
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
    std::string index;
    if (!config_.read_stage(GODOT_BUNDLE_INDEX, index, config_.read_stage_user) || index.empty()) {
        return ase::kernel::HostStatusNotFound;
    }
    bundle_.library = index_value(index, GODOT_BUNDLE_KEY_LIBRARY);
    bundle_.manifest = index_value(index, GODOT_BUNDLE_KEY_MANIFEST);
    bundle_.plugin = index_value(index, GODOT_BUNDLE_KEY_PLUGIN);
    bundle_.version = index_value(index, GODOT_BUNDLE_KEY_VERSION);
    bundle_.api_version = index_value(index, GODOT_BUNDLE_KEY_API);
    if (bundle_.library.empty() || bundle_.manifest.empty() || bundle_.plugin.empty() ||
        bundle_.version.empty() || bundle_.api_version.empty()) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    return ase::kernel::HostStatusOk;
}

std::string GodotHostResourceManager::plugin_library() const {
    // No directory: the libraries of an APK lie in its native library directory, which the app's
    // linker namespace searches by file name - the bare name is the whole address there.
    return config_.library_dir.empty() ? bundle_.library : config_.library_dir + "/" + bundle_.library;
}

// =============================================================================
// SELF-TEST
// =============================================================================

VivariumSelfTestResult GodotHostResourceManager::self_test(const VivariumStartConfig& config,
                                                           const std::string& table,
                                                           std::vector<std::string>& report) {
    VivariumSelfTestResult result;
    // The game host boots after the self-test, never beside it: two Apps at once would be a
    // second simulation in the process (PLAN 02.3).
    if (host_) {
        report_broken("-", GODOT_STEP_CREATE, "a game host exists, the self-test runs before it", result, report);
        return result;
    }
    config_ = config;
    if (config_.read_stage == nullptr || config_.port.empty() || config_.op_create.empty() ||
        config_.op_irrigate.empty()) {
        report_broken("-", GODOT_STEP_CONFIG, "no stage reader, port or operation", result, report);
        return result;
    }
    if (read_bundle() != ase::kernel::HostStatusOk) {
        report_broken("-", GODOT_STEP_BUNDLE, "bundle index not readable", result, report);
        bundle_ = VivariumBundleInfo{};
        return result;
    }
    std::string manifest;
    if (!config_.read_stage(bundle_.manifest.c_str(), manifest, config_.read_stage_user) || manifest.empty()) {
        report_broken("-", GODOT_STEP_MANIFEST, bundle_.manifest + " not readable", result, report);
        bundle_ = VivariumBundleInfo{};
        return result;
    }

    toml::table root;
    try {
        root = toml::parse(table);
    } catch (const toml::parse_error& error) {
        report_broken("-", GODOT_SELFTEST_KEY_CASE,
                      "table not readable at line " + std::to_string(error.source().begin.line) + ": " +
                          std::string(error.description()),
                      result, report);
        bundle_ = VivariumBundleInfo{};
        return result;
    }
    const double tolerance = root[GODOT_SELFTEST_KEY_TOLERANCE].value_or(std::nan(""));
    const double tick = root[GODOT_SELFTEST_KEY_TICK].value_or(std::nan(""));
    const toml::array* cases = root[GODOT_SELFTEST_KEY_CASE].as_array();
    if (!(tolerance >= 0.0) || !(tick > 0.0) || cases == nullptr || cases->empty()) {
        report_broken("-", GODOT_SELFTEST_KEY_CASE, "table needs tolerance, tick_seconds and cases", result, report);
        bundle_ = VivariumBundleInfo{};
        return result;
    }

    const std::string library = plugin_library();
    for (const toml::node& entry : *cases) {
        const toml::table* scenario = entry.as_table();
        if (scenario == nullptr) {
            report_broken("?", GODOT_SELFTEST_KEY_CASE, "a case is no table", result, report);
            continue;
        }
        run_scenario(*scenario, config_, library, manifest, tolerance, tick, this, result, report);
    }
    report.push_back("selftest end cases=" + std::to_string(result.cases) + " checks=" +
                     std::to_string(result.checks) + " errors=" + std::to_string(result.errors) +
                     " plugin=" + bundle_.plugin + "@" + bundle_.version);
    // The manager leaves as it came: no host, no bundle - boot() reads its own.
    bundle_ = VivariumBundleInfo{};
    return result;
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
    if (line == nullptr) {
        return;
    }
    // The node prints line by line; the trailing line break of the formatter would double it.
    while (len > 0u && (line[len - 1u] == '\n' || line[len - 1u] == '\r')) {
        len -= 1u;
    }
#if defined(__ANDROID__)
    // PLAN 02.1: the native logcat sink. Written now, from the thread that wrote the line - a line
    // queued for the next frame dies with a process that never reaches that frame. The spdlog
    // levels 0..5 (trace .. critical) map onto the contiguous logcat priorities VERBOSE .. FATAL
    // of the NDK ABI. Nothing here touches a Godot object.
    const std::string text(line, len);
    const int clamped = level < 0 ? 0 : (level > GODOT_LOG_LEVEL_MAX ? GODOT_LOG_LEVEL_MAX : level);
    __android_log_write(ANDROID_LOG_VERBOSE + clamped, GODOT_LOG_SOURCE, text.c_str());
#else
    (void)level;  // the line carries its level tag already
    const std::lock_guard<std::mutex> lock(log_mutex_);
    log_lines_.emplace_back(line, len);
#endif
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

bool GodotHostResourceManager::read_pss_kib(uint64_t& out) const {
    out = 0u;
    // smaps_rollup: a header line, then "Key:   value kB" lines. The key is matched at a line
    // start - "SwapPss:" further down ends in the same four characters.
    const std::string text = ase::fileio::read_text(GODOT_PSS_SOURCE);
    const std::size_t at = text.find(std::string("\n") + GODOT_PSS_KEY);
    if (at == std::string::npos) {
        return false;
    }
    std::size_t digit = text.find_first_of("0123456789", at);
    const std::size_t line_end = text.find('\n', at + 1u);
    if (digit == std::string::npos || (line_end != std::string::npos && digit > line_end)) {
        return false;
    }
    uint64_t value = 0u;
    while (digit < text.size() && text[digit] >= '0' && text[digit] <= '9') {
        value = value * 10u + static_cast<uint64_t>(text[digit] - '0');
        digit += 1u;
    }
    out = value;
    return true;
}

HostStatus GodotHostResourceManager::schedule_runs(const char* schedule, uint64_t* runs) const {
    if (runs != nullptr) {
        *runs = 0u;
    }
    // Only a running host is asked: a stopped one would log its refusal, and the view measures on
    // every focus change - after a failed boot that would be one warning per window switch.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    return host_->schedule_runs(schedule, runs);
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

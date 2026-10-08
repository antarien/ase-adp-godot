/**
 * ASE RESOURCE MANAGER IMPLEMENTATION (OOP-ECS Bridge)
 *
 * @file        godot_host_resource_manager.cpp
 * @brief       GodotHostResourceManager - External resource manager for the embedded ASE host of AseVivariumView
 * @description Flyweight Pattern implementation for unique_ptr<KernelEmbeddedHost> and the patch
 *              and clock views copied out of its port snapshots.
 *              The node stores ONLY uint32_t object ids, the manager owns everything behind them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-07
 * @version     00.00.03.00003 [seed]
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
 *   PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3: AseVivariumView owns exactly ONE KernelEmbeddedHost,
 *   and the resource manager carries factory, failure and teardown. The Godot node knows ids,
 *   places, rectangles and colours; everything that touches the host - the bundle, the units,
 *   the data root, the ports, the teardown - happens here, without a Godot type, so it is tested
 *   against the real bundle.
 *
 * WHAT IT NEVER DOES:
 *   It writes no simulation value back and computes nothing a module computes. The views are
 *   copies of what the ports reported after the last advance; stage and condition are the codes
 *   of ase-plant as the plugin forwards them.
 *
 * NO PAUSE (PLAN 01.3 point 4):
 *   advance() hands the host the real time the frame took, uncapped. The host ticks it in
 *   ordinary steps of its scheduler's frame, at most catch_up_steps of them per call, and keeps
 *   the rest in its backlog - a return from the background is caught up over a few frames, never
 *   in one giant step and never dropped.
 *
 * THE SELF-TEST STARTS ONE TEST HOST (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3):
 *   self_test() creates, loads and starts one host, runs every scenario of the table on it in
 *   order - each a check of one step within a few frames - and stops it once. A start per
 *   scenario would pay the dearest step of the host again for every case and test nothing the
 *   first start did not.
 *
 * THREAD SAFETY:
 *   The log sink is the only entry from a second thread (the host's logging worker); it only
 *   appends to log_lines_ under log_mutex_ - or, on Android, hands the line to logcat, which is
 *   thread-safe itself. Everything else runs on the thread that booted the host, and the host
 *   refuses any other.
 *
 * WHERE THE BUNDLE COMES FROM (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1):
 *   The manager never opens a stage file. The caller's VivariumStageReadFn reads the bundle index
 *   and the manifests it names - through Godot's FileAccess in the app, so a stage packed into an
 *   APK reads like one on disk. The libraries are opened under library_dir, or, with an empty
 *   library_dir, by their bare file names, which Android's linker resolves in the app's own
 *   namespace; neither res:// nor a package path ever reaches dlopen. The data files are read by
 *   the units themselves, below the data root the host names: files_dir is a real directory.
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
using ase::kernel::KernelHostUnit;

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
 * True for a finite, whole, non-negative value. The port carries places and codes as small whole
 * numbers, which a float holds exactly - ase-math has the float floor.
 */
bool whole(double value) {
    return std::isfinite(value) && value >= 0.0 &&
           ase::math::floor(static_cast<float>(value)) == static_cast<float>(value);
}

/** A whole number below `count` as a code; GODOT_CODE_UNKNOWN for anything else. */
uint8_t code_of(double value, uint8_t count) {
    if (!whole(value) || value >= static_cast<double>(count)) {
        return GODOT_CODE_UNKNOWN;
    }
    return static_cast<uint8_t>(value);
}

/** The words of one index value, split at blanks. */
std::vector<std::string> fields_of(const std::string& value) {
    std::vector<std::string> fields;
    std::string field;
    for (const char c : value) {
        if (c == ' ' || c == '\t') {
            if (!field.empty()) {
                fields.push_back(field);
                field.clear();
            }
        } else {
            field.push_back(c);
        }
    }
    if (!field.empty()) {
        fields.push_back(field);
    }
    return fields;
}

// THE SELF-TEST (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3)

/** One number of a report line: fixed decimals; nan and inf stay words (ase-utils). */
std::string decimal(double value) {
    char text[GODOT_NUMBER_BYTES] = {};
    ase::utils::str_append_f32(text, GODOT_NUMBER_BYTES, static_cast<float>(value),
                               GODOT_SELFTEST_DECIMALS);
    return std::string(text);
}

/** A scenario's label in the report: the runs its actions check, each once. */
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
        records.assign(ase::kernel::HostRecordsMax, KernelHostRecord{});
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

/** op of the table applied; `known` false for an op the form does not have. */
bool holds(double actual, const std::string& op, double value, double tolerance, bool& known) {
    known = true;
    if (op == GODOT_SELFTEST_OP_EQ) {
        return std::fabs(actual - value) <= tolerance;
    }
    if (op == GODOT_SELFTEST_OP_NE) {
        return std::fabs(actual - value) > tolerance;
    }
    if (op == GODOT_SELFTEST_OP_LT) {
        return actual < value;
    }
    if (op == GODOT_SELFTEST_OP_LE) {
        return actual <= value;
    }
    if (op == GODOT_SELFTEST_OP_GT) {
        return actual > value;
    }
    if (op == GODOT_SELFTEST_OP_GE) {
        return actual >= value;
    }
    known = false;
    return false;
}

/** The record of the patch on `place` in the last patch snapshot; nullptr when no record carries it. */
const KernelHostRecord* record_at(const std::vector<KernelHostRecord>& records, uint32_t written, int64_t place) {
    for (uint32_t i = 0u; i < written; ++i) {
        if (record_value(records[i], GODOT_KEY_PLACE) == static_cast<double>(place)) {
            return &records[i];
        }
    }
    return nullptr;
}

/**
 * The value an action names: a patch's (place) or the clock's (record = "clock"). NaN when the
 * snapshot fails or carries no such record or key - every comparison with NaN fails, so a value
 * that is not there is never a value that passes.
 */
double read_value(KernelEmbeddedHost& host, const VivariumStartConfig& config, const toml::table& spec,
                  VivariumSelfTestState& state) {
    const std::string key = spec[GODOT_SELFTEST_KEY_KEY].value_or(std::string());
    uint32_t written = 0u;
    if (spec[GODOT_SELFTEST_KEY_RECORD].value_or(std::string()) == GODOT_SELFTEST_RECORD_CLOCK) {
        if (snapshot_all(host, config.clock_port, state.records, written) != ase::kernel::HostStatusOk ||
            written != 1u) {
            return std::nan("");
        }
        return record_value(state.records[0], key.c_str());
    }
    if (snapshot_all(host, config.patch_port, state.records, written) != ase::kernel::HostStatusOk) {
        return std::nan("");
    }
    const KernelHostRecord* record =
        record_at(state.records, written, spec[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1}));
    return record == nullptr ? std::nan("") : record_value(*record, key.c_str());
}

/** The object id of the patch on `place`; GODOT_NO_OBJECT when the snapshot carries none. */
uint32_t object_at(KernelEmbeddedHost& host, const VivariumStartConfig& config, int64_t place,
                   VivariumSelfTestState& state) {
    uint32_t written = 0u;
    if (snapshot_all(host, config.patch_port, state.records, written) != ase::kernel::HostStatusOk) {
        return GODOT_NO_OBJECT;
    }
    const KernelHostRecord* record = record_at(state.records, written, place);
    return record == nullptr ? GODOT_NO_OBJECT : record->object_id;
}

/** value of an action plus the remembered value its `plus` refers to; NaN for an unknown one. */
double target_of(const toml::table& spec, const VivariumSelfTestState& state) {
    double value = spec[GODOT_SELFTEST_KEY_VALUE].value_or(std::nan(""));
    const std::string plus = spec[GODOT_SELFTEST_KEY_PLUS].value_or(std::string());
    if (!plus.empty()) {
        const auto found = state.remembered.find(plus);
        value = found == state.remembered.end() ? std::nan("") : value + found->second;
    }
    return value;
}

HostStatus pour(KernelEmbeddedHost& host, const VivariumStartConfig& config, uint32_t object_id, double amount_mm) {
    KernelHostInput input{};
    input.object_id = object_id;
    if (!put_value(input, GODOT_KEY_AMOUNT_MM, amount_mm)) {
        return ase::kernel::HostStatusCapacity;
    }
    return host.submit(config.patch_port.c_str(), config.op_irrigate.c_str(), &input);
}

/** One frame of real time - exactly one ordinary tick. false when the host refuses the frame. */
bool frame(KernelEmbeddedHost& host, const VivariumSelfTestState& state) {
    uint32_t steps = 0u;
    double backlog = 0.0;
    return host.advance(state.frame_s, 1u, &steps, &backlog) == ase::kernel::HostStatusOk && steps == 1u;
}

/**
 * Runs one action of a started scenario. Answers false (reported) when the action cannot run -
 * then the scenario ends there; a comparison that FAILS is counted and the scenario goes on, so
 * one wrong value never hides the next.
 */
bool run_action(KernelEmbeddedHost& host, const toml::table& action, const VivariumStartConfig& config,
                const std::string& label, VivariumSelfTestState& state, VivariumSelfTestResult& result,
                std::vector<std::string>& report) {
    const std::string check = action[GODOT_SELFTEST_KEY_CHECK].value_or(std::string());
    if (!check.empty()) {
        state.checked.push_back(check);
    }

    if (const toml::node* frames = action.get(GODOT_SELFTEST_KEY_FRAMES)) {
        const int64_t count = frames->value_or(int64_t{-1});
        if (count <= 0) {
            report_broken(label, GODOT_SELFTEST_KEY_FRAMES, "needs a positive count", result, report);
            return false;
        }
        for (int64_t i = 0; i < count; ++i) {
            if (!frame(host, state)) {
                report_broken(label, GODOT_STEP_ADVANCE, "a frame was refused", result, report);
                return false;
            }
        }
        return true;
    }

    if (const toml::table* keep = action[GODOT_SELFTEST_KEY_REMEMBER].as_table()) {
        const std::string as = (*keep)[GODOT_SELFTEST_KEY_AS].value_or(std::string());
        const double value = read_value(host, config, *keep, state);
        if (as.empty() || !std::isfinite(value)) {
            report_broken(label, GODOT_SELFTEST_KEY_REMEMBER, "needs `as` and a value the snapshot carries", result,
                          report);
            return false;
        }
        state.remembered[as] = value;
        return true;
    }

    if (const toml::table* water = action[GODOT_SELFTEST_KEY_IRRIGATE].as_table()) {
        const toml::node* amount = water->get(GODOT_SELFTEST_KEY_AMOUNT);
        const std::string expected = (*water)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        uint32_t object_id = GODOT_NO_OBJECT;
        std::string target;
        if (const toml::node* place = water->get(GODOT_SELFTEST_KEY_PLACE)) {
            const int64_t at = place->value_or(int64_t{-1});
            object_id = object_at(host, config, at, state);
            target = "place=" + std::to_string(at);
        } else {
            const int64_t named = (*water)[GODOT_SELFTEST_KEY_OBJECT].value_or(int64_t{-1});
            object_id = named < 0 || named > static_cast<int64_t>(UINT32_MAX) ? GODOT_NO_OBJECT
                                                                              : static_cast<uint32_t>(named);
            target = "object=" + std::to_string(named);
        }
        if (amount == nullptr || !amount->is_number() || expected.empty() ||
            (object_id == GODOT_NO_OBJECT && water->get(GODOT_SELFTEST_KEY_OBJECT) == nullptr)) {
            report_broken(label, GODOT_SELFTEST_KEY_IRRIGATE, "needs a patch or object, amount_mm and status", result,
                          report);
            return false;
        }
        const double amount_mm = amount->value_or(std::nan(""));
        const std::string actual = GodotHostResourceManager::status_name(pour(host, config, object_id, amount_mm));
        report_check(check, target + " action=irrigate amount_mm=" + decimal(amount_mm), actual, expected,
                     actual == expected, result, report);
        return true;
    }

    if (const toml::table* wait = action[GODOT_SELFTEST_KEY_WAIT].as_table()) {
        const int64_t bound = (*wait)[GODOT_SELFTEST_KEY_MAX_FRAMES].value_or(int64_t{0});
        const std::string op = (*wait)[GODOT_SELFTEST_KEY_OP].value_or(std::string());
        const std::string key = (*wait)[GODOT_SELFTEST_KEY_KEY].value_or(std::string());
        const double value = target_of(*wait, state);
        bool known = true;
        bool reached = holds(read_value(host, config, *wait, state), op, value, state.tolerance, known);
        if (bound <= 0 || !known || !std::isfinite(value)) {
            report_broken(label, GODOT_SELFTEST_KEY_WAIT, "needs max_frames, a known op and a value", result, report);
            return false;
        }
        int64_t frames = 0;
        for (; frames < bound && !reached; ++frames) {
            if (!frame(host, state)) {
                report_broken(label, GODOT_STEP_ADVANCE, "a frame was refused", result, report);
                return false;
            }
            reached = holds(read_value(host, config, *wait, state), op, value, state.tolerance, known);
        }
        report_check(check, "wait key=" + key + " frames=" + std::to_string(frames),
                     reached ? "reached" : "not reached", op + " " + decimal(value) + " within " +
                     std::to_string(bound) + " frames", reached, result, report);
        return true;
    }

    if (const toml::table* catch_up = action[GODOT_SELFTEST_KEY_CATCH_UP].as_table()) {
        const double seconds = (*catch_up)[GODOT_SELFTEST_KEY_SECONDS].value_or(std::nan(""));
        const int64_t max_steps = (*catch_up)[GODOT_SELFTEST_KEY_MAX_STEPS].value_or(int64_t{0});
        const int64_t expected = (*catch_up)[GODOT_SELFTEST_KEY_STEPS].value_or(int64_t{-1});
        if (!(seconds > 0.0) || max_steps <= 0 || max_steps > static_cast<int64_t>(UINT32_MAX) || expected < 0) {
            report_broken(label, GODOT_SELFTEST_KEY_CATCH_UP, "needs seconds, max_steps and steps", result, report);
            return false;
        }
        uint32_t steps = 0u;
        double backlog = 0.0;
        if (host.advance(seconds, static_cast<uint32_t>(max_steps), &steps, &backlog) != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_ADVANCE, "the catch-up was refused", result, report);
            return false;
        }
        int64_t total = steps;
        bool bounded = steps <= static_cast<uint32_t>(max_steps);
        // the backlog drains over later calls, each a bounded number of ordinary ticks
        for (int64_t call = 0; backlog > 0.0 && call <= expected; ++call) {
            if (host.advance(0.0, static_cast<uint32_t>(max_steps), &steps, &backlog) != ase::kernel::HostStatusOk) {
                report_broken(label, GODOT_STEP_ADVANCE, "the catch-up was refused", result, report);
                return false;
            }
            bounded = bounded && steps <= static_cast<uint32_t>(max_steps);
            total += steps;
        }
        report_check(check, "catch_up seconds=" + decimal(seconds) + " max_steps=" + std::to_string(max_steps),
                     std::to_string(total) + (bounded && backlog == 0.0 ? "" : " unbounded"),
                     std::to_string(expected), total == expected && bounded && backlog == 0.0, result, report);
        return true;
    }

    if (const toml::table* expect = action[GODOT_SELFTEST_KEY_EXPECT].as_table()) {
        const std::string op = (*expect)[GODOT_SELFTEST_KEY_OP].value_or(std::string());
        const std::string key = (*expect)[GODOT_SELFTEST_KEY_KEY].value_or(std::string());
        const double actual = read_value(host, config, *expect, state);
        const double value = target_of(*expect, state);
        bool known = true;
        const bool ok = holds(actual, op, value, state.tolerance, known);
        if (!known) {
            report_broken(label, GODOT_SELFTEST_KEY_EXPECT, "unknown op " + op, result, report);
            return false;
        }
        const std::string subject =
            (*expect)[GODOT_SELFTEST_KEY_RECORD].value_or(std::string()) == GODOT_SELFTEST_RECORD_CLOCK
                ? std::string("clock")
                : "place=" + std::to_string((*expect)[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1}));
        report_check(check, subject + " key=" + key, decimal(actual), op + " " + decimal(value), ok, result, report);
        return true;
    }

    if (const toml::table* count = action[GODOT_SELFTEST_KEY_RECORDS].as_table()) {
        const std::string record = (*count)[GODOT_SELFTEST_KEY_RECORD].value_or(std::string());
        const int64_t expected = (*count)[GODOT_SELFTEST_KEY_COUNT].value_or(int64_t{-1});
        uint32_t written = 0u;
        const std::string& port = record == GODOT_SELFTEST_RECORD_CLOCK ? config.clock_port : config.patch_port;
        const HostStatus status = snapshot_all(host, port, state.records, written);
        if (expected < 0 || status != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_SNAPSHOT,
                          expected < 0 ? std::string("records needs a count") : GodotHostResourceManager::status_name(status),
                          result, report);
            return false;
        }
        report_check(check, "records=" + record, std::to_string(written), std::to_string(expected),
                     static_cast<int64_t>(written) == expected, result, report);
        return true;
    }

    report_broken(label, GODOT_SELFTEST_KEY_DO,
                  "an action names none of frames, remember, irrigate, wait, catch_up, expect, records",
                  result, report);
    return false;
}

}  // anonymous namespace

// LIFETIME

GodotHostResourceManager::GodotHostResourceManager()
    : failure_step_(GODOT_STEP_NONE), failure_status_(ase::kernel::HostStatusOk) {}

GodotHostResourceManager::~GodotHostResourceManager() {
    clear_all();
}

// THE BUNDLE INDEX

bool GodotHostResourceManager::parse_index(const std::string& text, VivariumBundleInfo& out) {
    out = VivariumBundleInfo{};
    const std::string separator(GODOT_BUNDLE_SEPARATOR);
    std::size_t start = 0u;
    while (start < text.size()) {
        const std::size_t newline = text.find('\n', start);
        const std::size_t end = newline == std::string::npos ? text.size() : newline;
        std::string line = text.substr(start, end - start);
        start = end + 1u;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::size_t at = line.find(separator);
        if (at == std::string::npos) {
            out = VivariumBundleInfo{};
            return false;
        }
        const std::string key = line.substr(0u, at);
        const std::string value = line.substr(at + separator.size());
        if (key == GODOT_BUNDLE_KEY_DATA_ROOT) {
            out.data_root = value;
        } else if (key == GODOT_BUNDLE_KEY_UNIT) {
            const std::vector<std::string> fields = fields_of(value);
            if (fields.size() != GODOT_BUNDLE_UNIT_FIELDS) {
                out = VivariumBundleInfo{};
                return false;
            }
            VivariumBundleUnit unit;
            unit.unit = fields[0];
            unit.library = fields[1];
            unit.manifest = fields[2];
            unit.version = fields[3];
            unit.api_version = fields[4];
            out.units.push_back(unit);
        } else if (key == GODOT_BUNDLE_KEY_DATA) {
            out.data.push_back(value);
        } else {
            out = VivariumBundleInfo{};
            return false;
        }
    }
    if (out.data_root.empty() || out.units.empty()) {
        out = VivariumBundleInfo{};
        return false;
    }
    return true;
}

// FACTORY

HostStatus GodotHostResourceManager::boot(const VivariumStartConfig& config) {
    // Reiniciar runs the same teardown as _exit_tree: the old host, its views and its bundle go
    // first, so no id, input or drawing value of the previous run survives into this one.
    clear_all();
    config_ = config;
    failure_step_ = GODOT_STEP_NONE;
    failure_status_ = ase::kernel::HostStatusOk;
    elapsed_in_s_ = 0.0;
    backlog_s_ = 0.0;
    tick_count_ = 0u;
    last_steps_ = 0u;

    // A configuration the host could never run is refused before a host exists: no catch-up
    // steps would turn every frame into a refused advance, an empty name into a NotFound later.
    if (config_.read_stage == nullptr || config_.files_dir.empty() || config_.files_dir[0] != '/' ||
        config_.patch_port.empty() || config_.clock_port.empty() || config_.op_irrigate.empty() ||
        !std::isfinite(config_.irrigate_amount_mm) || !(config_.irrigate_amount_mm > 0.0) ||
        config_.catch_up_steps == 0u) {
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
    status = read_manifests();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_MANIFEST, status);
    }
    status = load_set(*host_);
    if (status != ase::kernel::HostStatusOk) {
        return fail(failure_step_ == GODOT_STEP_NONE ? GODOT_STEP_LOAD : failure_step_, status);
    }

    if (!host_->has_port(config_.patch_port.c_str()) || !host_->has_port(config_.clock_port.c_str())) {
        return fail(GODOT_STEP_PORT, ase::kernel::HostStatusNotFound);
    }

    status = host_->start();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_START, status);
    }

    records_.assign(ase::kernel::HostRecordsMax, KernelHostRecord{});
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
    places_.clear();
    clock_ = VivariumClockView{};
    if (!host_) {
        return status;  // no host, no logger: the node shows and prints the failure itself
    }
    // The host stays, STOPPED: its units are unloaded, but its logger still carries this line.
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
    if (!parse_index(index, bundle_)) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_manifests() {
    manifests_.clear();
    for (const VivariumBundleUnit& unit : bundle_.units) {
        std::string manifest;
        if (!config_.read_stage(unit.manifest.c_str(), manifest, config_.read_stage_user) || manifest.empty()) {
            manifests_.clear();
            return ase::kernel::HostStatusNotFound;
        }
        manifests_.push_back(manifest);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::load_set(KernelEmbeddedHost& host) {
    // THE DATA ROOT FIRST: every unit finds it in the App's context from its on_load on.
    const std::string data_root = config_.files_dir + "/" + bundle_.data_root;
    HostStatus status = host.set_data_root(data_root.c_str());
    if (status != ase::kernel::HostStatusOk) {
        failure_step_ = GODOT_STEP_DATA_ROOT;
        return status;
    }
    // The paths must outlive the call; KernelHostUnit only points at them.
    std::vector<std::string> paths;
    paths.reserve(bundle_.units.size());
    std::vector<KernelHostUnit> units;
    units.reserve(bundle_.units.size());
    for (size_t i = 0; i < bundle_.units.size() && i < manifests_.size(); ++i) {
        paths.push_back(library_path(bundle_.units[i].library));
        KernelHostUnit unit;
        unit.library_path = paths.back().c_str();
        unit.manifest_text = manifests_[i].c_str();
        unit.manifest_length = static_cast<uint32_t>(manifests_[i].size());
        units.push_back(unit);
    }
    status = host.load_units(units.data(), static_cast<uint32_t>(units.size()));
    if (status != ase::kernel::HostStatusOk) {
        failure_step_ = GODOT_STEP_LOAD;
    }
    return status;
}

std::string GodotHostResourceManager::library_path(const std::string& library) const {
    // No directory: the libraries of an APK lie in its native library directory, which the app's
    // linker namespace searches by file name - the bare name is the whole address there.
    return config_.library_dir.empty() ? library : config_.library_dir + "/" + library;
}

// SELF-TEST

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
    if (config_.read_stage == nullptr || config_.files_dir.empty() || config_.patch_port.empty() ||
        config_.clock_port.empty() || config_.op_irrigate.empty()) {
        report_broken("-", GODOT_STEP_CONFIG, "no stage reader, files directory, port or operation", result, report);
        return result;
    }
    if (read_bundle() != ase::kernel::HostStatusOk || read_manifests() != ase::kernel::HostStatusOk) {
        report_broken("-", GODOT_STEP_BUNDLE, "bundle index or a manifest not readable", result, report);
        bundle_ = VivariumBundleInfo{};
        manifests_.clear();
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
        manifests_.clear();
        return result;
    }
    const double tolerance = root[GODOT_SELFTEST_KEY_TOLERANCE].value_or(std::nan(""));
    const double frame_s = root[GODOT_SELFTEST_KEY_FRAME].value_or(std::nan(""));
    const toml::array* runs = root[GODOT_SELFTEST_KEY_RUNS].as_array();
    const toml::array* cases = root[GODOT_SELFTEST_KEY_CASE].as_array();
    if (!(tolerance >= 0.0) || !(frame_s > 0.0) || runs == nullptr || cases == nullptr || cases->empty()) {
        report_broken("-", GODOT_SELFTEST_KEY_CASE, "table needs tolerance, frame_seconds, runs and cases", result,
                      report);
        bundle_ = VivariumBundleInfo{};
        manifests_.clear();
        return result;
    }

    // ONE TEST HOST FOR THE WHOLE TABLE: created, loaded and started once, every scenario in the
    // table's order on it, stopped and destroyed after the last one - before the game host exists.
    // No log file - the game host's file must not rotate - and this manager's sink. A host that
    // cannot start is one error of its own; the runs it would have checked are reported below.
    std::vector<std::string> checked;
    std::unique_ptr<KernelEmbeddedHost> host;
    HostStatus status = KernelEmbeddedHost::create(host, &queue_host_line, this, nullptr);
    bool ready = status == ase::kernel::HostStatusOk;
    if (!ready) {
        report_broken("-", GODOT_STEP_CREATE, status_name(status), result, report);
    }
    if (ready) {
        failure_step_ = GODOT_STEP_NONE;
        status = load_set(*host);
        ready = status == ase::kernel::HostStatusOk;
        if (!ready) {
            // A failed load has torn the host down already; its lines name the cause.
            report_broken("-", failure_step_, status_name(status), result, report);
            failure_step_ = GODOT_STEP_NONE;
        }
    }
    if (ready) {
        status = host->start();
        ready = status == ase::kernel::HostStatusOk;
        if (!ready) {
            report_broken("-", GODOT_STEP_START, status_name(status), result, report);
            (void)host->stop();
        }
    }

    if (ready) {
        for (const toml::node& entry : *cases) {
            const toml::table* scenario = entry.as_table();
            if (scenario == nullptr) {
                report_broken("?", GODOT_SELFTEST_KEY_CASE, "a case is no table", result, report);
                continue;
            }
            const std::string label = scenario_label(*scenario);
            const toml::array* actions = (*scenario)[GODOT_SELFTEST_KEY_DO].as_array();
            if (actions == nullptr) {
                report_broken(label, GODOT_SELFTEST_KEY_DO, "no actions", result, report);
                continue;
            }
            VivariumSelfTestState state;
            state.tolerance = tolerance;
            state.frame_s = frame_s;
            bool complete = true;
            for (const toml::node& node : *actions) {
                const toml::table* action = node.as_table();
                if (action == nullptr) {
                    report_broken(label, GODOT_SELFTEST_KEY_DO, "an action is no table", result, report);
                    complete = false;
                    break;
                }
                if (!run_action(*host, *action, config_, label, state, result, report)) {
                    complete = false;
                    break;
                }
            }
            checked.insert(checked.end(), state.checked.begin(), state.checked.end());
            if (complete) {
                result.cases += 1u;
            }
        }
        status = host->stop();
        if (status != ase::kernel::HostStatusOk) {
            report_broken("-", GODOT_STEP_STOP, status_name(status), result, report);
        }
    }
    host.reset();

    // Every run the table claims must be checked by one of its actions - a run nobody checked is
    // an error, never a run that passed.
    for (const toml::node& node : *runs) {
        const std::string run = node.value_or(std::string());
        bool found = false;
        for (const std::string& check : checked) {
            found = found || check == run;
        }
        if (!found) {
            result.errors += 1u;
            report.push_back("selftest run=" + run + " result=FAILED cause=no action checks it");
        }
    }
    std::string set;
    for (const VivariumBundleUnit& unit : bundle_.units) {
        set += (set.empty() ? "" : ",") + unit.unit + "@" + unit.version;
    }
    report.push_back("selftest end cases=" + std::to_string(result.cases) + " checks=" +
                     std::to_string(result.checks) + " errors=" + std::to_string(result.errors) + " set=" + set);
    // The manager leaves as it came: no host, no bundle - boot() reads its own.
    bundle_ = VivariumBundleInfo{};
    manifests_.clear();
    return result;
}

// RUNNING

HostStatus GodotHostResourceManager::advance(double elapsed_s) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    // Never capped, never dropped: NaN and infinity reach the host unchanged and are refused there
    // with its own warning; a long frame is the background time, and the host catches it up.
    uint32_t steps = 0u;
    double backlog = 0.0;
    const HostStatus status = host_->advance(elapsed_s, config_.catch_up_steps, &steps, &backlog);
    if (status != ase::kernel::HostStatusOk) {
        return status;  // this frame is refused; the host said why
    }
    elapsed_in_s_ += elapsed_s;
    backlog_s_ = backlog;
    last_steps_ = steps;
    tick_count_ += steps;

    const HostStatus read = read_snapshot();
    if (read != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_SNAPSHOT, read);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_snapshot() {
    uint32_t written = 0u;
    HostStatus status = snapshot_all(*host_, config_.patch_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    patches_.clear();
    places_.clear();
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        VivariumPatchView view;
        view.object_id = record.object_id;
        const double place = record_value(record, GODOT_KEY_PLACE);
        const double stage = record_value(record, GODOT_KEY_STAGE);
        const double condition = record_value(record, GODOT_KEY_CONDITION);
        view.coverage = record_value(record, GODOT_KEY_COVERAGE);
        view.soil_mm = record_value(record, GODOT_KEY_SOIL_MM);
        view.soil_rel = record_value(record, GODOT_KEY_SOIL_REL);
        view.capacity_mm = record_value(record, GODOT_KEY_CAPACITY_MM);
        // THE PORT CONTRACT IS THE PLACE. ase-pl-flora writes a key only while the input row behind
        // it stands (FloraHostPtchExptSystem: a key stands only while its row stands, never a zero
        // the modules did not publish), so before the first runs of the modules a record carries
        // its place and nothing else. A missing value is NOT MEASURED YET: a code stays unknown, a
        // number stays NaN, which share() draws as nothing. A record without a whole place is a
        // broken port, not a value to draw.
        if (!whole(place)) {
            patches_.clear();
            places_.clear();
            return ase::kernel::HostStatusInvalidArgument;
        }
        view.place = static_cast<uint32_t>(place);
        view.stage = code_of(stage, GODOT_STAGE_COUNT);
        view.condition = code_of(condition, GODOT_COND_COUNT);
        patches_[view.object_id] = view;
        places_[view.place] = view.object_id;
    }

    std::vector<KernelHostRecord> clock_records(1u);
    status = snapshot_all(*host_, config_.clock_port, clock_records, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    // The clock port publishes an empty set until the clock row of the plugin stands
    // (FloraHostClkExptSystem): the clock is not known yet, and nothing is broken. Two clocks are.
    if (written == 0u) {
        clock_ = VivariumClockView{};
        return ase::kernel::HostStatusOk;
    }
    if (written != 1u) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    VivariumClockView clock;
    clock.elapsed_s = record_value(clock_records[0], GODOT_KEY_ELAPSED);
    clock.day = record_value(clock_records[0], GODOT_KEY_DAY);
    clock.hour = record_value(clock_records[0], GODOT_KEY_HOUR);
    if (!std::isfinite(clock.elapsed_s) || !std::isfinite(clock.day) || !std::isfinite(clock.hour)) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    clock.known = true;
    clock_ = clock;
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::irrigate(uint32_t object_id) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    input.object_id = object_id;
    if (!put_value(input, GODOT_KEY_AMOUNT_MM, config_.irrigate_amount_mm)) {
        return ase::kernel::HostStatusCapacity;
    }
    return host_->submit(config_.patch_port.c_str(), config_.op_irrigate.c_str(), &input);
}

HostStatus GodotHostResourceManager::note(const char* text) {
    if (!host_) {
        return ase::kernel::HostStatusInvalidState;
    }
    return host_->note(GODOT_LOG_SOURCE, text);
}

// VIEWS

const VivariumPatchView* GodotHostResourceManager::get_patch(uint32_t object_id) const {
    const auto found = patches_.find(object_id);
    return found == patches_.end() ? nullptr : &found->second;
}

const VivariumPatchView* GodotHostResourceManager::get_patch_at(uint32_t place) const {
    const auto found = places_.find(place);
    return found == places_.end() ? nullptr : get_patch(found->second);
}

bool GodotHostResourceManager::has_patch(uint32_t object_id) const {
    return patches_.find(object_id) != patches_.end();
}

uint32_t GodotHostResourceManager::patch_count() const {
    return static_cast<uint32_t>(patches_.size());
}

const VivariumClockView& GodotHostResourceManager::get_clock() const {
    return clock_;
}

// LOG LINES

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

// STATE AND MEASUREMENT

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
    return elapsed_in_s_ - backlog_s_;
}

uint64_t GodotHostResourceManager::tick_count() const {
    return tick_count_;
}

uint32_t GodotHostResourceManager::last_steps() const {
    return last_steps_;
}

double GodotHostResourceManager::backlog_seconds() const {
    return backlog_s_;
}

uint32_t GodotHostResourceManager::system_count() const {
    return host_ ? host_->system_count() : 0u;
}

uint32_t GodotHostResourceManager::unit_count() const {
    return host_ ? host_->unit_count() : 0u;
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

// BULK CLEANUP

void GodotHostResourceManager::clear_all() {
    // Stop BEFORE clearing: the host runs the binding teardown (App shutdown, ports released,
    // App destroyed, dlclose in reverse load order) and releases its logger last. Lines it still
    // writes stay in log_lines_ for the node to print.
    if (host_) {
        (void)host_->stop();
        host_.reset();
    }
    patches_.clear();
    places_.clear();
    clock_ = VivariumClockView{};
    records_.clear();
    bundle_ = VivariumBundleInfo{};
    manifests_.clear();
}

}  // namespace ase::adp::godot

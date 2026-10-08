/**
 * ASE ADAPTER TESTS - GODOT HOST RESOURCE MANAGER
 *
 * @file        test_godot_host_resource_manager.cpp
 * @design      DSGN_021
 * @brief       Pins GodotHostResourceManager against the real bundle: boot of the whole set,
 *              port values, pouring, catching up without a pause, a missing unit, the self-test
 *              and the teardown
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG, the parts of L1, L3, L5 and L8 that do not need a
 *              window. The manager boots the SAME bundle Godot loads: every unit's library and
 *              staged manifest, the data root and the index, side by side in the build's bundle
 *              directory (clients/avt-client-android/cmake/AvtRuntime.cmake). No Godot in this
 *              process - that is why the host part carries no Godot type.
 *
 *              THE GAME HOST STARTS ONCE PER RUN. One manager boots the set once, with the log
 *              file, and every case about the game host runs against it in file order; the last
 *              of them tears it down. The refusals come first and start nothing: a missing bundle
 *              or configuration, a stage without a unit, a table that cannot run.
 *
 *              THE SELF-TEST (PLAN 02.3) runs the client's table config/vivarium_selftest.toml here
 *              on Linux - the same table, the same runner and the same set the device runs - ONCE,
 *              on the one test host the plan gives it before the game host exists. A wrong
 *              expected value and a run no action checks ride along in that one run as its
 *              positive control: the runner must count exactly these two.
 *
 *              THE PATCHES ARE THE PLUGIN'S: ase-pl-flora creates four, places 0 to 3, each with
 *              100 mm field capacity and 20 mm start water (its types.hpp), a seed of a healthy
 *              stand. The test finds them by place, never by a guessed object id.
 *
 *              THE STAGE READER IS THE TEST'S OWN (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1): the
 *              node reads index and manifests through Godot's FileAccess, this test through
 *              ase-fileio from the bundle directory - the manager sees only VivariumStageReadFn.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @created     2026-10-05
 * @modified    2026-10-07
 * @version     00.00.03.00003
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/fileio/text_writer.hpp>
#include <ase/log/log_display.hpp>
#include <ase/utils/fs.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

// THE GAME HOST OF THE RUN: one manager, booted on the first call of environment() with the log
// file, torn down by the last case about it. Its boot lines are taken out of the queue right after
// the boot, so the case about the log does not depend on what the cases before it drained. What
// the boot returned is kept and reported by every case - a failed boot is never retried, because
// a second boot would be a second test environment.
struct AdapterEnvironment {
    bool attempted = false;
    std::string bundle;
    GodotHostResourceManager resources;
    HostStatus booted = ase::kernel::HostStatusInvalidState;
    std::vector<std::string> boot_lines;
};

namespace {

// The test's VivariumStageReadFn: one file of the bundle directory `user` names.
bool read_bundle_file(const char* name, std::string& out, void* user) {
    const std::string path = ase::fileio::path_join(*static_cast<const std::string*>(user), name);
    if (!ase::fileio::file_exists(path)) {
        return false;
    }
    out = ase::fileio::read_text(path);
    return true;
}

// The client's start configuration over the bundle directory `bundle`, which must outlive it: the
// libraries are opened by path there and the data root lies below it, as on the desktop.
VivariumStartConfig vivarium_config(std::string& bundle) {
    VivariumStartConfig config;
    config.read_stage = &read_bundle_file;
    config.read_stage_user = &bundle;
    config.library_dir = bundle;
    config.files_dir = bundle;
    config.patch_port = "flora.patch.v1";
    config.clock_port = "flora.clock.v1";
    config.op_irrigate = "irrigate";
    config.irrigate_amount_mm = 25.0;
    config.catch_up_steps = 8u;
    return config;
}

// The one boot of the run.
AdapterEnvironment& environment() {
    static AdapterEnvironment env;
    if (env.attempted) {
        return env;
    }
    env.attempted = true;
    env.bundle = ASE_ADP_GODOT_TEST_BUNDLE;
    const std::string path = ASE_ADP_GODOT_TEST_LOG;
    (void)ase::utils::fs::remove(path);
    VivariumStartConfig config = vivarium_config(env.bundle);
    config.log_file = path;
    env.booted = env.resources.boot(config);
    (void)env.resources.remove_log_lines(env.boot_lines);
    return env;
}

// n frames of a quarter second each - one tick each, the scheduler's frame ceiling.
void advance_frames(GodotHostResourceManager& resources, uint32_t frames) {
    for (uint32_t frame = 0u; frame < frames; ++frame) {
        REQUIRE(resources.advance(0.25) == ase::kernel::HostStatusOk);
    }
}

const VivariumPatchView& patch_at(const GodotHostResourceManager& resources, uint32_t place) {
    const VivariumPatchView* patch = resources.get_patch_at(place);
    REQUIRE(patch != nullptr);
    return *patch;
}

// A host line without its colour sequences. ase-log colours words of a certain shape inside the
// message body (log_display.cpp, colorize_log_line), so a phrase can stand split by them. A CSI
// sequence runs from ESC '[' to the first byte in '@'..'~', as ansi_len reads it there.
std::string without_colour(std::string_view line) {
    std::string plain;
    plain.reserve(line.size());
    for (std::string_view::size_type i = 0; i < line.size(); ++i) {
        if (line[i] == '\x1b' && i + 1 < line.size() && line[i + 1] == '[') {
            std::string_view::size_type end = i + 2;
            while (end < line.size() && !(line[end] >= '@' && line[end] <= '~')) {
                ++end;
            }
            i = end;  // the loop steps over the final byte
            continue;
        }
        plain.push_back(line[i]);
    }
    return plain;
}

// `text` as ase-log displays it INSIDE a line: shorten_display_line puts every known word in its
// short form and drops fillers - but keeps a filler at the very start of a line. So the text is
// walked behind a bracket group, which the walk copies unchanged, and cut off again. The function
// is the runtime's own, exported by libase_kernel_embedded.so; this test links no second copy.
std::string displayed(const std::string& text) {
    const std::string shown = ase::log::shorten_display_line("[] " + text);
    return shown.substr(shown.compare(0, 3, "[] ") == 0 ? 3u : 2u);
}

// Whether one of the host's lines carries `text`, compared in the form the lines arrive in.
bool any_line_carries(const std::vector<std::string>& lines, const std::string& text) {
    const std::string marker = displayed(text);
    for (const std::string& line : lines) {
        if (without_colour(line).find(marker) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// A stage beside the real bundle that carries the index `index` and every staged manifest - the
// libraries and the data stay in the real bundle, which library_dir and files_dir name.
std::string stage_with_index(const std::string& bundle, const std::string& suffix, const std::string& index) {
    const std::string stage = bundle + suffix;
    REQUIRE(ase::fileio::create_directories(stage));
    REQUIRE(ase::fileio::write_text(ase::fileio::path_join(stage, GODOT_BUNDLE_INDEX), index));
    VivariumBundleInfo info;
    REQUIRE(GodotHostResourceManager::parse_index(
        ase::fileio::read_text(ase::fileio::path_join(bundle, GODOT_BUNDLE_INDEX)), info));
    for (const VivariumBundleUnit& unit : info.units) {
        REQUIRE(ase::fileio::write_text(ase::fileio::path_join(stage, unit.manifest),
                                        ase::fileio::read_text(ase::fileio::path_join(bundle, unit.manifest))));
    }
    return stage;
}

// The real index without the line of `unit`.
std::string index_without(const std::string& bundle, const std::string& unit) {
    const std::string index = ase::fileio::read_text(ase::fileio::path_join(bundle, GODOT_BUNDLE_INDEX));
    const std::string prefix = std::string(GODOT_BUNDLE_KEY_UNIT) + GODOT_BUNDLE_SEPARATOR + unit + " ";
    std::string out;
    std::string::size_type start = 0u;
    while (start < index.size()) {
        const std::string::size_type end = index.find('\n', start);
        const std::string line = index.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (line.compare(0u, prefix.size(), prefix) != 0) {
            out += line + "\n";
        }
        start = end == std::string::npos ? index.size() : end + 1u;
    }
    return out;
}

// Scenario headers of a table: `[[case]]` at the start of a line - the FORM comment of the table
// names the same word inside a comment.
uint32_t scenarios_of(const std::string& table) {
    uint32_t scenarios = 0u;
    for (std::string::size_type at = table.find("[[case]]"); at != std::string::npos;
         at = table.find("[[case]]", at + 1u)) {
        if (at == 0u || table[at - 1u] == '\n') {
            scenarios += 1u;
        }
    }
    return scenarios;
}

}  // anonymous namespace

TEST_CASE("the bundle index parses into units, data root and data, and refuses what it does not understand") {
    VivariumBundleInfo info;
    REQUIRE(GodotHostResourceManager::parse_index("# head\ndata_root = data\n"
                                                  "unit = ase-hub libase_hub.so ase-hub.module.toml 00.02.10.00010 4 ab\n"
                                                  "data = modules/ase-hub/data/hub_agg.json\n",
                                                  info));
    CHECK(info.data_root == "data");
    REQUIRE(info.units.size() == 1u);
    CHECK(info.units[0].unit == "ase-hub");
    CHECK(info.units[0].library == "libase_hub.so");
    CHECK(info.units[0].manifest == "ase-hub.module.toml");
    CHECK(info.units[0].version == "00.02.10.00010");
    CHECK(info.units[0].api_version == "4");
    REQUIRE(info.data.size() == 1u);
    CHECK(info.data[0] == "modules/ase-hub/data/hub_agg.json");

    // Never half read: each refusal leaves nothing behind.
    CHECK(!GodotHostResourceManager::parse_index("data_root = data\nunit = ase-hub libase_hub.so\n", info));
    CHECK(info.units.empty());
    CHECK(!GodotHostResourceManager::parse_index("data_root = data\nlibrary = libase_pl_vegetation.so\n", info));
    CHECK(!GodotHostResourceManager::parse_index("unit = a b c d e f\n", info));
    CHECK(!GodotHostResourceManager::parse_index("data_root = data\n", info));
}

TEST_CASE("a missing bundle and an incomplete configuration are named by their step, and nothing starts") {
    std::string bundle(ASE_ADP_GODOT_TEST_BUNDLE);
    std::string nowhere_dir("/nonexistent/avt-bundle");
    GodotHostResourceManager resources;
    VivariumStartConfig nowhere = vivarium_config(bundle);
    nowhere.read_stage_user = &nowhere_dir;
    CHECK(resources.boot(nowhere) == ase::kernel::HostStatusNotFound);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_BUNDLE);
    CHECK(!resources.running());

    // Refused before a host exists: no catch-up steps, no stage reader, a relative files directory.
    VivariumStartConfig no_steps = vivarium_config(bundle);
    no_steps.catch_up_steps = 0u;
    CHECK(resources.boot(no_steps) == ase::kernel::HostStatusInvalidArgument);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_CONFIG);
    CHECK(!resources.has_host());

    VivariumStartConfig no_reader = vivarium_config(bundle);
    no_reader.read_stage = nullptr;
    CHECK(resources.boot(no_reader) == ase::kernel::HostStatusInvalidArgument);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_CONFIG);
    CHECK(!resources.has_host());

    VivariumStartConfig relative = vivarium_config(bundle);
    relative.files_dir = "bundle";
    CHECK(resources.boot(relative) == ase::kernel::HostStatusInvalidArgument);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_CONFIG);

    // A port the set does not offer is found after the load and before the start.
    VivariumStartConfig wrong_port = vivarium_config(bundle);
    wrong_port.clock_port = "flora.clock.v9";
    CHECK(resources.boot(wrong_port) == ase::kernel::HostStatusNotFound);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_PORT);
    CHECK(!resources.running());
}

TEST_CASE("L8: a bundle without one unit fails at load_units, loudly and without a simulation") {
    std::string bundle(ASE_ADP_GODOT_TEST_BUNDLE);
    // ase-plant left out of the index: ase-pl-flora requires it, the strict load refuses the set.
    std::string stage = stage_with_index(bundle, "-without-plant", index_without(bundle, "ase-plant"));
    VivariumStartConfig config = vivarium_config(bundle);
    config.read_stage_user = &stage;

    GodotHostResourceManager resources;
    CHECK(resources.boot(config) == ase::kernel::HostStatusNotFound);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_LOAD);
    CHECK(resources.failure_status() == ase::kernel::HostStatusNotFound);

    // No stand-in simulation: no patch, no unit, no tick, no water.
    CHECK(!resources.running());
    CHECK(resources.patch_count() == 0u);
    CHECK(resources.unit_count() == 0u);
    CHECK(resources.advance(0.25) == ase::kernel::HostStatusInvalidState);
    CHECK(resources.irrigate(0u) == ase::kernel::HostStatusInvalidState);

    // The stopped host is kept, so its log carries the cause.
    CHECK(resources.has_host());
    std::vector<std::string> lines;
    CHECK(resources.remove_log_lines(lines) > 0u);
    CHECK(any_line_carries(lines, std::string("failed at ") + GODOT_STEP_LOAD));
}

TEST_CASE("a self-test that cannot run is never a self-test without errors, and starts nothing") {
    std::string bundle(ASE_ADP_GODOT_TEST_BUNDLE);
    GodotHostResourceManager resources;
    std::vector<std::string> report;

    // A table that is no TOML: one error, no scenario.
    VivariumSelfTestResult result = resources.self_test(vivarium_config(bundle), "case = [[[", report);
    CHECK(result.errors == 1u);
    CHECK(result.cases == 0u);

    // A table without cases: one error, no scenario.
    result = resources.self_test(vivarium_config(bundle), "tolerance = 0.0001\nframe_seconds = 0.25\nruns = []\n",
                                 report);
    CHECK(result.errors == 1u);
    CHECK(result.cases == 0u);

    // A stage without ase-plant: the one test host fails at load_units, no scenario counts as run.
    const std::string table = ase::fileio::read_text(ASE_ADP_GODOT_TEST_SELFTEST);
    REQUIRE(!table.empty());
    std::string stage = stage_with_index(bundle, "-selftest-without-plant", index_without(bundle, "ase-plant"));
    VivariumStartConfig config = vivarium_config(bundle);
    config.read_stage_user = &stage;
    report.clear();
    result = resources.self_test(config, table, report);
    CHECK(result.cases == 0u);
    CHECK(result.errors > 0u);
    for (const std::string& line : report) {
        CHECK_MESSAGE((line.find(std::string("step=") + GODOT_STEP_LOAD) != std::string::npos ||
                       line.find("selftest run=") == 0u || line.find("selftest end") == 0u),
                      line);
    }
    CHECK(!resources.has_host());
}

TEST_CASE("the self-test runs the table on its one test host and counts exactly its positive control") {
    std::string bundle(ASE_ADP_GODOT_TEST_BUNDLE);
    const std::string real = ase::fileio::read_text(ASE_ADP_GODOT_TEST_SELFTEST);
    REQUIRE(!real.empty());
    // THE POSITIVE CONTROL RIDES IN THE SAME RUN: a run the table claims but no action checks,
    // and one more scenario whose expected value is wrong - the start capacity is 100 mm.
    std::string table = real;
    const std::string runs = "\nruns = [";
    const std::string::size_type at = table.find(runs);
    REQUIRE(at != std::string::npos);
    table.insert(at + runs.size(), "\"nowhere\", ");
    table += "\n[[case]]\n\n[[case.do]]\ncheck = \"start\"\n"
             "expect = { place = 0, key = \"capacity_mm\", op = \"eq\", value = 99.0 }\n";

    GodotHostResourceManager resources;
    std::vector<std::string> report;
    const VivariumSelfTestResult result = resources.self_test(vivarium_config(bundle), table, report);
    // Every scenario ran to its end - a failed comparison ends none - and exactly the two planted
    // errors were counted: one wrong value hides no other check.
    CHECK(result.cases == scenarios_of(real) + 1u);
    CHECK(result.checks > 0u);
    CHECK(result.errors == 2u);
    uint32_t wrong_value = 0u;
    uint32_t unchecked_run = 0u;
    for (const std::string& line : report) {
        if (line.find("result=FAILED") == std::string::npos) {
            continue;
        }
        const bool value = line.find("check=start ") != std::string::npos &&
                           line.find("key=capacity_mm") != std::string::npos &&
                           line.find("expected=eq 99") != std::string::npos;
        const bool run = line.find("selftest run=nowhere ") == 0u;
        wrong_value += value ? 1u : 0u;
        unchecked_run += run ? 1u : 0u;
        CHECK_MESSAGE((value || run), line);
    }
    CHECK(wrong_value == 1u);
    CHECK(unchecked_run == 1u);
    REQUIRE(!report.empty());
    CHECK(report.back().find("selftest end") == 0u);
    CHECK(report.back().find("ase-pl-flora@") != std::string::npos);

    // The test host is gone: the manager leaves as it came, ready for the game host.
    CHECK(!resources.has_host());
    CHECK(resources.get_bundle().units.empty());
}

TEST_CASE("L1: the game host starts once - the whole set from the bundle, four patches by place, the game clock") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    CHECK(resources.running());
    CHECK(std::string(resources.failure_step()).empty());
    CHECK(resources.failure_status() == ase::kernel::HostStatusOk);

    const VivariumBundleInfo& info = resources.get_bundle();
    CHECK(info.data_root == "data");
    CHECK(resources.unit_count() == info.units.size());
    bool flora = false;
    bool star = false;
    for (const VivariumBundleUnit& unit : info.units) {
        flora = flora || unit.unit == "ase-pl-flora";
        star = star || unit.unit == "ase-hub";
        CHECK(!unit.version.empty());
    }
    CHECK(flora);
    CHECK(star);
    CHECK(resources.system_count() > 0u);
    CHECK(resources.tick_count() == 0u);

    // The boot runs no tick: the ports carry what the modules published so far, and the views
    // fill with the first runs of the modules - eight frames, two Dissemination runs.
    advance_frames(resources, 8u);
    CHECK(resources.patch_count() == 4u);
    for (uint32_t place = 0u; place < 4u; ++place) {
        const VivariumPatchView& patch = patch_at(resources, place);
        CHECK(patch.place == place);
        CHECK(resources.get_patch(patch.object_id) == resources.get_patch_at(place));
        CHECK(patch.capacity_mm == doctest::Approx(100.0));
        CHECK(patch.soil_mm <= 20.0);
        CHECK(patch.stage == GODOT_STAGE_SEED);
        CHECK(patch.condition == GODOT_COND_HEALTHY);
    }
    CHECK(resources.get_patch_at(4u) == nullptr);
    CHECK(resources.get_patch(GODOT_NO_OBJECT) == nullptr);
    CHECK(resources.get_clock().known);
}

TEST_CASE("the host's lines and the view's own line reach the queue and the log file") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    CHECK(any_line_carries(env.boot_lines, "[KernelEmbeddedHost] loaded"));
    CHECK(any_line_carries(env.boot_lines, "[KernelEmbeddedHost] started"));
    CHECK(resources.note("boot line probe") == ase::kernel::HostStatusOk);

    std::vector<std::string> lines;
    CHECK(resources.remove_log_lines(lines) > 0u);
    CHECK(any_line_carries(lines, "[AVT] boot line probe"));
    for (const std::vector<std::string>* group : {&env.boot_lines, &lines}) {
        for (const std::string& line : *group) {
            CHECK((line.empty() || line.back() != '\n'));
        }
    }
    // Drained means drained.
    std::vector<std::string> again;
    CHECK(resources.remove_log_lines(again) == 0u);
    CHECK(ase::utils::fs::is_regular_file(ASE_ADP_GODOT_TEST_LOG));
}

TEST_CASE("L3: a pour arrives as more soil water of that patch - after the modules ran, never optimistically") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    advance_frames(resources, 8u);
    const VivariumPatchView before_poured = patch_at(resources, 0u);
    const VivariumPatchView before_other = patch_at(resources, 1u);

    CHECK(resources.irrigate(before_poured.object_id) == ase::kernel::HostStatusOk);
    // Nothing optimistic: the view still shows what the port reported.
    CHECK(patch_at(resources, 0u).soil_mm == doctest::Approx(before_poured.soil_mm));

    // Four Dissemination runs: the plugin takes the pour, ase-hydro takes and balances it.
    advance_frames(resources, 16u);
    CHECK(patch_at(resources, 0u).soil_mm > before_poured.soil_mm + 20.0);  // 25 mm, less evaporation
    CHECK(patch_at(resources, 1u).soil_mm <= before_other.soil_mm);         // the other one only loses

    // An object the plugin does not know is taken by the port and poured nowhere.
    CHECK(resources.irrigate(GODOT_NO_OBJECT - 1u) == ase::kernel::HostStatusOk);
    advance_frames(resources, 8u);
    CHECK(resources.patch_count() == 4u);
}

TEST_CASE("L5 without a window: a long frame is caught up in ordinary steps, never capped and never dropped") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    advance_frames(resources, 4u);
    const double game_before = resources.get_clock().elapsed_s;
    const uint64_t ticks_before = resources.tick_count();
    const double simulated_before = resources.simulation_seconds();

    // Ten seconds in the background come back as one frame: eight ticks now, the rest waits.
    REQUIRE(resources.advance(10.0) == ase::kernel::HostStatusOk);
    CHECK(resources.last_steps() == 8u);
    CHECK(resources.backlog_seconds() > 0.0);
    for (uint32_t frame = 0u; frame < 8u && resources.backlog_seconds() > 0.0; ++frame) {
        REQUIRE(resources.advance(0.0) == ase::kernel::HostStatusOk);
        CHECK(resources.last_steps() <= 8u);
    }
    CHECK(resources.backlog_seconds() == 0.0);
    CHECK(resources.tick_count() - ticks_before == 40u);                                // 10 s in ticks of 0.25 s
    CHECK(resources.simulation_seconds() - simulated_before == doctest::Approx(10.0));  // all of it, caught up
    CHECK(resources.get_clock().elapsed_s > game_before);

    // A broken frame is refused by the host, the host keeps running and nothing is counted.
    const uint64_t ticks = resources.tick_count();
    CHECK(resources.advance(std::numeric_limits<double>::quiet_NaN()) == ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.advance(std::numeric_limits<double>::infinity()) == ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.advance(-0.25) == ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.tick_count() == ticks);
    CHECK(resources.running());
    CHECK(std::string(resources.failure_step()).empty());
}

TEST_CASE("beside the game host the self-test refuses and leaves the game host untouched") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const std::string table = ase::fileio::read_text(ASE_ADP_GODOT_TEST_SELFTEST);
    REQUIRE(!table.empty());
    std::vector<std::string> report;
    const VivariumSelfTestResult result = resources.self_test(vivarium_config(env.bundle), table, report);
    CHECK(result.errors == 1u);
    CHECK(result.cases == 0u);
    CHECK(resources.running());
    CHECK(resources.patch_count() == 4u);
}

TEST_CASE("clear_all ends the game host once, is idempotent and leaves no host behind") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    resources.clear_all();
    resources.clear_all();
    CHECK(!resources.has_host());
    CHECK(!resources.running());
    CHECK(resources.unit_count() == 0u);
    CHECK(resources.patch_count() == 0u);
    CHECK(!resources.get_clock().known);
    CHECK(resources.advance(0.25) == ase::kernel::HostStatusInvalidState);
    CHECK(resources.note("after the host") == ase::kernel::HostStatusInvalidState);
}

TEST_CASE("the PSS of this process is read from smaps_rollup, or reported as not read") {
    GodotHostResourceManager resources;
    uint64_t pss = 0u;
    if (resources.read_pss_kib(pss)) {
        CHECK(pss > 0u);  // a running test process occupies memory
    } else {
        CHECK(pss == 0u);  // no rollup on this kernel: no value, and no invented one
    }
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - GODOT HOST REFUSALS AND SELF-TEST
 *
 * @file        test_godot_host_refusal.cpp
 * @design      DSGN_021
 * @brief       Pins what GodotHostResourceManager refuses before a game host exists, and the self-test on its own test host
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG L8 and PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3, against the
 *              real bundle (test_godot_host_support.hpp) and without a game host: the bundle index and
 *              its refusals, a missing bundle and every incomplete configuration named by its step, a
 *              port the set does not offer, the words of the start configuration, a stage without a unit,
 *              a self-test that cannot run and the self-test of the client's table. No Godot in this
 *              process, and no game host - the one game host of a run is test_godot_host.cpp's.
 *
 *              THE SELF-TEST (PLAN 02.3) runs the client's table config/vivarium_selftest.toml here on
 *              Linux - the same table, the same runner and the same set the device runs - ONCE, on the one
 *              test host the plan gives it before a game host exists. A wrong expected value and a run no
 *              action checks ride along in that one run as its positive control: the runner must count
 *              exactly these two.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/fileio/text_writer.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

using test::any_line_carries;
using test::vivarium_config;

namespace {

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

    // Patches the game host could not lay are refused before a host exists: none at all, a place
    // named twice, a ground the view has no word for, a patch without a species.
    VivariumStartConfig no_patch = vivarium_config(bundle);
    no_patch.patches.clear();
    CHECK(resources.boot(no_patch) == ase::kernel::HostStatusInvalidArgument);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_CONFIG);
    CHECK(!resources.has_host());
    VivariumStartConfig twice = vivarium_config(bundle);
    twice.patches[1].place = twice.patches[0].place;
    CHECK(resources.boot(twice) == ase::kernel::HostStatusInvalidArgument);
    CHECK(std::string(resources.failure_step()) == GODOT_STEP_CONFIG);
    VivariumStartConfig no_ground = vivarium_config(bundle);
    no_ground.patches[0].soil = GODOT_SOIL_COUNT;
    CHECK(resources.boot(no_ground) == ase::kernel::HostStatusInvalidArgument);
    VivariumStartConfig no_species = vivarium_config(bundle);
    no_species.patches[0].species.clear();
    CHECK(resources.boot(no_species) == ase::kernel::HostStatusInvalidArgument);

    // An unnamed port is refused before a host exists, never at the port step of a host that loaded
    // the whole set for nothing: an organ port, the ground, the sky, the seeds, the tiling, the templates.
    std::vector<VivariumStartConfig> unnamed(8u, vivarium_config(bundle));
    unnamed[0].organ_ports[GODOT_ORGAN_ROOT].clear();
    unnamed[1].terrain_port.clear();
    unnamed[2].ground_cell_port.clear();
    unnamed[3].sky_port.clear();
    unnamed[4].seed_port.clear();
    unnamed[5].tile_port.clear();
    unnamed[6].template_corner_port.clear();
    unnamed[7].template_triangle_port.clear();
    for (const VivariumStartConfig& config : unnamed) {
        CHECK(resources.boot(config) == ase::kernel::HostStatusInvalidArgument);
        CHECK(std::string(resources.failure_step()) == GODOT_STEP_CONFIG);
        CHECK(!resources.has_host());
    }

    // A port the set does not offer is found after the load and before the start.
    std::vector<VivariumStartConfig> unoffered(7u, vivarium_config(bundle));
    unoffered[0].clock_port = "flora.clock.v9";
    unoffered[1].organ_ports[GODOT_ORGAN_LEAF] = "flora.organ.leaf.v9";
    unoffered[2].sky_port = "sky.scene.v9";
    unoffered[3].seed_port = "flora.seed.v9";
    unoffered[4].tile_port = "flora.tile.v9";
    unoffered[5].template_corner_port = "flora.template.corner.v9";
    unoffered[6].template_triangle_port = "flora.template.triangle.v9";
    for (const VivariumStartConfig& config : unoffered) {
        CHECK(resources.boot(config) == ase::kernel::HostStatusNotFound);
        CHECK(std::string(resources.failure_step()) == GODOT_STEP_PORT);
        CHECK(!resources.running());
    }
}

TEST_CASE("the words of the start configuration map onto one code each, and an unknown word onto none") {
    CHECK(GodotHostResourceManager::soil_code(GODOT_SOIL_WORD_MEADOW) == GODOT_SOIL_MEADOW);
    CHECK(GodotHostResourceManager::soil_code(GODOT_SOIL_WORD_FOREST) == GODOT_SOIL_FOREST);
    CHECK(GodotHostResourceManager::soil_code("sand") == GODOT_CODE_UNKNOWN);
    CHECK(std::string(GodotHostResourceManager::organ_word(GODOT_ORGAN_SEGMENT)) == GODOT_ORGAN_WORD_SEGMENT);
    CHECK(std::string(GodotHostResourceManager::organ_word(GODOT_ORGAN_ROOT)) == GODOT_ORGAN_WORD_ROOT);
    CHECK(std::string(GodotHostResourceManager::organ_word(GODOT_ORGAN_COUNT)).empty());
    // The species travels as the catalog id the units key it with; the facade computes it, the
    // adapter never: measured against entt::hashed_string, FNV-1a over the id text.
    CHECK(ase::kernel::KernelEmbeddedHost::name_id(nullptr) == 0u);
    CHECK(ase::kernel::KernelEmbeddedHost::name_id("") == 2166136261u);
    CHECK(ase::kernel::KernelEmbeddedHost::name_id("a") == 3826002220u);
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

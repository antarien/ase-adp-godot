/**
 * ASE ADAPTER TESTS - GODOT HOST
 *
 * @file        test_godot_host.cpp
 * @design      DSGN_021
 * @brief       Pins the one game host of GodotHostResourceManager against the real bundle: boot of the whole
 *              set, the log, pouring, the organ and instance ports, the cycle line, catching up without a
 *              pause and the teardown
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG, the parts of L1, L3 and L5 that do not need a window, and
 *              PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6 and 03.11. The manager boots the SAME bundle Godot
 *              loads: every unit's library and staged manifest, the data root and the index, side by side
 *              in the build's bundle directory (clients/avt-client-android/cmake/AvtRuntime.cmake), with
 *              the configuration and the game host of test_godot_host_support.hpp. No Godot in this
 *              process - that is why the host part carries no Godot type. What the manager refuses before
 *              a game host exists stands in test_godot_host_refusal.cpp, what the scene ports carry -
 *              templates, sky, ground, stem base - in test_godot_host_scene.cpp.
 *
 *              THE GAME HOST STARTS ONCE PER RUN. One manager boots the set once, with the log file, and
 *              every case runs against it in file order; the last of them tears it down.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-10-05
 * @modified    2026-10-18
 * @version     00.00.04.00007
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/math/math.hpp>
#include <ase/utils/fs.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::any_line_carries;
using test::environment;
using test::patch_at;
using test::vivarium_config;

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
    // The ground of each laid patch comes back through the patch port unchanged.
    CHECK(patch_at(resources, 0u).soil == GODOT_SOIL_MEADOW);
    CHECK(patch_at(resources, 3u).soil == GODOT_SOIL_FOREST);
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

TEST_CASE("Nivelar: a level request is taken by the level port, and the ground of the disc stands at its level") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    advance_frames(resources, 8u);
    const VivariumPatchView before = patch_at(resources, 0u);

    CHECK(resources.level(before.object_id) == ase::kernel::HostStatusOk);
    // Four Dissemination runs: the plugin files the flatten, ase-terrain levels the disc and measures it again.
    advance_frames(resources, 16u);
    const VivariumPatchView after = patch_at(resources, 0u);
    // The centre decides the level, and nothing moved the centre: the stem base stays where it stood.
    CHECK(after.y == doctest::Approx(before.y).epsilon(1e-4));

    // Every sample well inside the disc stands at that level; the count shows the disc held samples at all.
    const double inner_m = 0.75 * test::kPatchHalfM;
    uint32_t inside = 0u;
    for (const VivariumTerrainView& sample : resources.get_terrain()) {
        const double dx = sample.x - after.x;
        const double dz = sample.z - after.z;
        if (dx * dx + dz * dz < inner_m * inner_m) {
            ++inside;
            CHECK(sample.height == doctest::Approx(after.y).epsilon(1e-4));
        }
    }
    CHECK(inside > 0u);

    // An object the plugin does not know is taken by the port and levels nothing.
    CHECK(resources.level(GODOT_NO_OBJECT - 1u) == ase::kernel::HostStatusOk);
    advance_frames(resources, 8u);
    CHECK(resources.patch_count() == 4u);
}

TEST_CASE("the poured seed sprouts, and its shoot and root reach the organ ports within a few frames") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const uint32_t cared = patch_at(resources, 0u).object_id;
    const uint64_t generation = resources.organ_generation();

    // The pour of L3 lets the seed on place 0 sprout; its first internode and its proximal roots are
    // published in the run it sprouted (ase-phytomer, ase-radix: an unshown organ publishes at once),
    // read into the plugin's inputs and laid on the ports in the frame after. The bound is a few
    // Dissemination runs.
    //
    // ONE RUN OF THE ORGAN SCHEDULE FIRST, whether the shoot sprouted during the frames of L3 or does so
    // here: the manager reads the organ ports anew after every run of that schedule, so one run lays a
    // new generation either way. Without it the case measured where L3 stopped - a shoot already on the
    // port left the walk below without a single frame and the generation unchanged (measured
    // 2026-10-21 on v00.00.26: the shoot stood on the port at the start of the case, 20 > 20).
    constexpr uint32_t kRunFrames = 4u;  // Dissemination runs once in four frames of a quarter second
    constexpr uint32_t kBoundFrames = 32u;
    advance_frames(resources, kRunFrames);
    for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_organs(GODOT_ORGAN_SEGMENT).empty(); ++frame) {
        advance_frames(resources, 1u);
    }
    REQUIRE(!resources.get_organs(GODOT_ORGAN_SEGMENT).empty());
    CHECK(!resources.get_organs(GODOT_ORGAN_ROOT).empty());
    CHECK(resources.organ_generation() > generation);
    for (const VivariumOrganView& segment : resources.get_organs(GODOT_ORGAN_SEGMENT)) {
        CHECK(segment.body == cared);
        CHECK(segment.y >= 0.0);       // the shoot stands above the ground
        CHECK(segment.length > 0.0);
    }
    for (const VivariumOrganView& root : resources.get_organs(GODOT_ORGAN_ROOT)) {
        CHECK(root.body == cared);
        CHECK(root.y <= 0.0);          // the root lies below it
    }
    // Only the poured patch carries a plant; an unknown class answers nothing.
    CHECK(resources.get_organs(GODOT_ORGAN_COUNT).empty());

    // 03.11 step 2: the internodes reach the instance port too - one record per internode, named by an
    // organ record of the same object id, its 3x4 the one ase-render-data computed. ase-render-data
    // takes its axes from the organ rows the organ ports carry, so its records may follow them by a
    // frame; the checks below hold in either frame: a rotation scaled - never a reflection - with the
    // template's y along the organ and an even scale across it, the base above the ground.
    for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_instances(GODOT_ORGAN_SEGMENT).empty();
         ++frame) {
        advance_frames(resources, 1u);
    }
    REQUIRE(!resources.get_instances(GODOT_ORGAN_SEGMENT).empty());
    const std::vector<VivariumOrganView>& segments = resources.get_organs(GODOT_ORGAN_SEGMENT);
    for (const VivariumInstanceView& record : resources.get_instances(GODOT_ORGAN_SEGMENT)) {
        const VivariumOrganView* segment = nullptr;
        for (const VivariumOrganView& candidate : segments) {
            if (candidate.object_id == record.object_id) {
                segment = &candidate;
            }
        }
        REQUIRE(segment != nullptr);
        CHECK(segment->body == cared);
        const float* m = record.rows;
        const float across_x = ase::math::sqrt(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
        const float along_y = ase::math::sqrt(m[1] * m[1] + m[5] * m[5] + m[9] * m[9]);
        const float across_z = ase::math::sqrt(m[2] * m[2] + m[6] * m[6] + m[10] * m[10]);
        const double determinant = m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) +
                                   m[2] * (m[4] * m[9] - m[5] * m[8]);
        CHECK(along_y > 0.0);
        CHECK(across_x > 0.0);
        CHECK(across_x == doctest::Approx(across_z));
        CHECK(determinant > 0.0);
        CHECK(m[7] >= 0.0f);           // the origin row up: the internode starts above the ground
    }
    // The root lies in the ground and has no instance port; an unknown class answers nothing.
    CHECK(resources.get_instances(GODOT_ORGAN_ROOT).empty());
    CHECK(resources.get_instances(GODOT_ORGAN_COUNT).empty());
}

TEST_CASE("03.6: the cycle line counts the organs on the ports and the time the units spent since the cursors") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumStartConfig config = vivarium_config(env.bundle);

    CycleMeasure measure;
    for (const VivariumBundleUnit& unit : resources.get_bundle().units) {
        measure.units.push_back(unit.unit);
    }
    for (uint32_t i = 0u; i < ase::kernel::KernelEmbeddedHost::schedule_count(); ++i) {
        measure.schedules.emplace_back(ase::kernel::KernelEmbeddedHost::schedule_name(i));
    }
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        measure.organ_words.emplace_back(GodotHostResourceManager::organ_word(c));
        measure.organ_ports.push_back(config.organ_ports[c]);
    }
    REQUIRE(cycle_measure_valid(measure));
    REQUIRE(resources.measure_begin(measure));

    // Four frames: Dissemination runs once, every other schedule of the set at least as often.
    advance_frames(resources, 4u);
    std::string line;
    REQUIRE(resources.measure_cycle(measure, line));
    CAPTURE(line);
    CHECK(line.rfind("cycle=1 organs segment=", 0) == 0);  // the line starts with its cycle and the classes
    // The seed of the sprout case grew a shoot: a segment record of order 0 stands on the port.
    CHECK(line.find(" segment=0 ") == std::string::npos);
    CHECK(line.find(" orders 0=") != std::string::npos);
    // Time was spent in the four frames, and the line sums it.
    CHECK(line.find(" time_us ") != std::string::npos);
    CHECK(line.find(" time_us_cycle=0 ") == std::string::npos);
    CHECK(line.find(" pss_kib=") != std::string::npos);
    CHECK(measure.cycle == 1u);

    // A measure without its names never runs, and a refused read names what it refused.
    CycleMeasure empty;
    CHECK(!cycle_measure_valid(empty));
    CycleMeasure wrong = measure;
    wrong.units.push_back("ase-not-in-the-bundle");
    wrong.seen_us.assign(wrong.units.size() * wrong.schedules.size(), 0u);
    CHECK(!resources.measure_cycle(wrong, line));
    CHECK(line.find("refused=time:ase-not-in-the-bundle@") != std::string::npos);
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
    const uint64_t templates = resources.template_generation();
    resources.clear_all();
    resources.clear_all();
    CHECK(!resources.has_host());
    CHECK(!resources.running());
    CHECK(resources.unit_count() == 0u);
    CHECK(resources.patch_count() == 0u);
    CHECK(!resources.get_clock().known);
    // the templates of the old host go with it (usecase:101011 tessellate the organ templates), and the view
    // sees a new generation
    CHECK(resources.get_template_corners().empty());
    CHECK(resources.get_template_triangles().empty());
    CHECK(resources.template_generation() > templates);
    CHECK(resources.advance(0.25) == ase::kernel::HostStatusInvalidState);
    CHECK(resources.note("after the host") == ase::kernel::HostStatusInvalidState);
}

}  // namespace ase::adp::godot

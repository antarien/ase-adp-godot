/**
 * ASE ADAPTER TESTS - GODOT HOST FIGURE PORTS
 *
 * @file        test_godot_host_figure.cpp
 * @design      DSGN_030
 * @brief       Pins what the figure ports of one game host take and carry: the figure of the seat, the hand of Phase 06
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.8 and 07.10, against the real bundle with the
 *              configuration of test_godot_host_support.hpp and the figure ports of the client's start
 *              configuration (test_godot_host_figure_support.hpp): boot() asks ase-pl-avatar for the figure
 *              of the seat, the body port carries it at its spawn place, a hold of a thing of the hand files a
 *              put-on request and a drop a put-down request (usecase:101625 carry a tool in hand), and every
 *              piece the gear port carries names a figure of the body port at the right hand.
 *              07.6 and 07.7: the rig port takes the measured skeleton of the figure (eye height and height,
 *              the face landing of ase-camera reads it), and the turn port turns a figure its player leads on
 *              the spot - the body port then carries the turned course (fig.turn). 07.14: the vessel a figure
 *              bears stands in the gear port under its bearer and a fill never lowers its litres - the rise of
 *              those litres is what fires the bend at the brook in the scene.
 *
 *              THE GEAR PORT ANSWERS ONLY WITH ase-equipment IN THE SET: the plugin files the request, the
 *              equipment module puts the piece on. Until the project file names the module the port stays
 *              empty, and the case pins what holds either way - every record it carries is well formed.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside those of test_godot_host.cpp and
 *              test_godot_host_scene.cpp, with the figure ports added to the shared configuration. The
 *              cameras that look at the figure (cam.mode, cam.drag, cam.zoom, the rim) are pinned in
 *              test_godot_host_camera.cpp on a game host of their own.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101625 - carry a tool in hand
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-27
 * @version     00.00.23.00023 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_figure_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::any_line_carries;
using test::figure_environment;
using test::figure_of;
using test::frames_carry;
using test::kFigureBoundFrames;
using test::kFigureEyeHeight;
using test::kFigureHeight;
using test::kFigureRadius;
using test::kFigureSpawnX;
using test::kFigureSpawnZ;
using test::the_figure;

namespace {

/** A figure stands within this distance of its spawn place, metres: nothing moves it without a stick. */
constexpr double kFigureSpawnSlack = 0.5;
/** The soul value of a figure its player leads (SS/PC) the code port must name: the expected value, not the one compared against. */
constexpr double kFigureSoulPlayer = 1.0;
/** One turn on the spot, radians, inside the span of one request. */
constexpr double kFigureTurn = 0.5;
/** Slack of the turned course, radians. */
constexpr double kFigureTurnSlack = 0.05;
constexpr double kFullCircle = 6.283185307179586;

/** Frames advance until the figure is led by its player, its soul the player code of the code port; a bounded number. */
bool led_by_player(GodotHostResourceManager& resources, uint32_t object_id) {
    for (uint32_t frame = 0u; frame < kFigureBoundFrames; ++frame) {
        const VivariumFigureView* figure = figure_of(resources, object_id);
        const VivariumFigureCodeView& codes = resources.get_figure_code();
        if (figure != nullptr && codes.known && figure->soul == codes.soul_player) {
            return true;
        }
        advance_frames(resources, 1u);
    }
    return false;
}

/** The course from `before` to `after`, signed and wrapped into (-π, π]. */
double turned_by(double before, double after) {
    double delta = after - before;
    while (delta > kFullCircle * 0.5) delta -= kFullCircle;
    while (delta <= -kFullCircle * 0.5) delta += kFullCircle;
    return delta;
}

}  // namespace

TEST_CASE("07.8: the host offers every figure port and the body port carries the figure of the seat at its spawn place") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(resources.figure_offered());
    // boot() submits the spawn request on the spawn port; AvatarHostSpwnSystem takes it in the first frame that
    // ticks after the boot, so its line stands in the boot lines only when the boot itself ran that frame.
    CHECK((any_line_carries(env.boot_lines, "asks for its figure, spawn request filed") ||
           frames_carry(resources, "asks for its figure, spawn request filed")));

    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    CHECK(figure->object_id != 0u);
    CHECK(figure->x == doctest::Approx(kFigureSpawnX).epsilon(kFigureSpawnSlack / kFigureSpawnX));
    CHECK(figure->z == doctest::Approx(kFigureSpawnZ).epsilon(kFigureSpawnSlack / kFigureSpawnZ));
}

TEST_CASE("usecase:101611 animate character motion: the code port names the gait points and the soul and camera codes of the body record") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(the_figure(resources) != nullptr);

    // a figure stands, so read_figures refuses a frame without the one code record: the view is known
    const VivariumFigureCodeView& codes = resources.get_figure_code();
    REQUIRE(codes.known);
    CHECK(codes.gait_stand < codes.gait_walk);
    CHECK(codes.gait_walk < codes.gait_run);
    CHECK(codes.soul_player == kFigureSoulPlayer);
    CHECK(codes.soul_ai != codes.soul_player);
    CHECK(codes.cam_fixed != codes.cam_player);
}

TEST_CASE("usecase:101625 carry a tool in hand: a hold files the put-on request, a drop the put-down request") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    const uint32_t bearer = figure->object_id;

    // the spade of the hand of Phase 06: its item key as the name_id ase-pl-avatar hashes it
    const uint32_t spade = ase::kernel::KernelEmbeddedHost::name_id("pa");
    REQUIRE(spade != 0u);
    CHECK(resources.hold_figure(bearer, spade) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "into the hand, put-on request filed"));

    CHECK(resources.drop_figure(bearer) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "empties its hand, put-down request filed"));
}

TEST_CASE("usecase:101625 carry a tool in hand: every piece of the gear port names a figure of the body port at its right hand or its back") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(the_figure(resources) != nullptr);
    CHECK(resources.hold_figure(resources.get_figures().front().object_id,
                                ase::kernel::KernelEmbeddedHost::name_id("enxada")) == ase::kernel::HostStatusOk);
    advance_frames(resources, kFigureBoundFrames);

    // the two places a figure carries a piece at: the right hand (07.10) and the back (N0590); the run shares one
    // host, so a pack an earlier case put on stays on the back
    const uint32_t hand = ase::kernel::KernelEmbeddedHost::name_id("socket_hand_r");
    const uint32_t back = ase::kernel::KernelEmbeddedHost::name_id("socket_back");
    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        bool borne = false;
        for (const VivariumFigureView& figure : resources.get_figures()) {
            borne = borne || figure.object_id == piece.bearer;
        }
        CHECK(borne);
        CHECK(piece.object_id != 0u);
        CHECK((piece.slot == hand || piece.slot == back));
        CHECK(piece.kind != 0u);
    }
}

TEST_CASE("usecase:101332 wear equipment in slots: a don puts the pack of the seat on the back of the figure") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    const uint32_t bearer = figure->object_id;

    // the pack of the HUD travels as the name_id of its item key, never of its asset name container_backpack_leather
    const uint32_t pack = ase::kernel::KernelEmbeddedHost::name_id("ranzen");
    REQUIRE(pack != 0u);
    CHECK(resources.don_figure(bearer, pack) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "on its back, put-on request filed"));
    advance_frames(resources, kFigureBoundFrames);

    // ase-equipment put it on: the gear port names it at the socket of the back, with the kind of its key
    const uint32_t back = ase::kernel::KernelEmbeddedHost::name_id("socket_back");
    bool worn = false;
    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        worn = worn || (piece.bearer == bearer && piece.slot == back && piece.kind == pack);
    }
    CHECK(worn);
}

TEST_CASE("usecase:100367 water a patch: a vessel of the gear port carries the litres of ase-equipment, every other piece none") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(the_figure(resources) != nullptr);
    const uint32_t can = ase::kernel::KernelEmbeddedHost::name_id("regador");
    CHECK(resources.hold_figure(resources.get_figures().front().object_id, can) == ase::kernel::HostStatusOk);
    advance_frames(resources, kFigureBoundFrames);

    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        if (piece.vessel) {
            CHECK(piece.kind == can);
            CHECK(std::isfinite(piece.water_l));
            CHECK(piece.water_l >= 0.0);
        } else {
            CHECK(piece.water_l == 0.0);
        }
    }
}

TEST_CASE("usecase:100399 carry brook water: the fill and drain ports file the requests of the figure, the level stays ase-equipment's") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(the_figure(resources) != nullptr);
    const uint32_t figure = resources.get_figures().front().object_id;
    const uint32_t can = ase::kernel::KernelEmbeddedHost::name_id("regador");
    CHECK(resources.hold_figure(figure, can) == ase::kernel::HostStatusOk);
    advance_frames(resources, kFigureBoundFrames);

    // The fill names no litres; ase-pl-avatar files it, whether the cell under the figure holds water decides
    // ase-equipment (GIS_HYD_CELL_DEPTH), so the vessel holds 0 up to its room and never a level the view set.
    CHECK(resources.fill_figure(figure) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "fills its vessel, fill request filed"));
    // The release names the patch the water falls on (N0517): the patch of place 0 the shared configuration lays,
    // whose soil column ase-equipment wets - the same patch object the scene hands on from get_patch_at.
    const VivariumPatchView* patch = resources.get_patch_at(0u);
    REQUIRE(patch != nullptr);
    CHECK(resources.drain_figure(figure, 1.0, patch->object_id) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "lets 1 litres go over patch " + std::to_string(patch->object_id) +
                                      ", drain request filed"));
    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        if (piece.vessel) {
            CHECK(std::isfinite(piece.water_l));
            CHECK(piece.water_l >= 0.0);
        }
    }

    // A figure the host does not know is the plugin's to refuse: the port still takes the request.
    CHECK(resources.drain_figure(0u, 1.0, patch->object_id) == ase::kernel::HostStatusOk);
}

TEST_CASE("07.14 usecase:101611 the bend at the brook: the vessel the figure bears stands in the gear port and a fill never lowers its litres") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(the_figure(resources) != nullptr);
    const uint32_t figure = resources.get_figures().front().object_id;
    const uint32_t can = ase::kernel::KernelEmbeddedHost::name_id("regador");
    CHECK(resources.hold_figure(figure, can) == ase::kernel::HostStatusOk);
    advance_frames(resources, kFigureBoundFrames);

    // The scene fires the bend when the litres of the vessel its figure bears rise between two frames
    // (godot_figure_scene.cpp, carried_water): it finds that vessel by bearer, so every vessel names a figure of
    // the body port, and the level it compares is ase-equipment's, read again after the fill.
    double before = 0.0;
    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        if (piece.vessel && piece.bearer == figure) {
            before = piece.water_l;
        }
    }
    CHECK(resources.fill_figure(figure) == ase::kernel::HostStatusOk);
    advance_frames(resources, kFigureBoundFrames);
    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        if (!piece.vessel) {
            continue;
        }
        bool borne = false;
        for (const VivariumFigureView& body : resources.get_figures()) {
            borne = borne || body.object_id == piece.bearer;
        }
        CHECK(borne);
        if (piece.bearer == figure) {
            CHECK(std::isfinite(piece.water_l));
            CHECK(piece.water_l >= before);
        }
    }
}

TEST_CASE("07.6: the rig port takes the measured skeleton of the figure for the camera and the capsule of physics") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    CHECK(resources.measure_figure(figure->object_id, kFigureEyeHeight, kFigureHeight, kFigureRadius) ==
          ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "measured, eyes at"));
}

TEST_CASE("07.7 fig.turn: a figure its player leads turns on the spot and the body port carries the turned course") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* first = the_figure(resources);
    REQUIRE(first != nullptr);
    const uint32_t object_id = first->object_id;

    // The host turns only a figure its player leads; the seat takes its figure back first when the AI leads it.
    if (!resources.get_figure_code().known || first->soul != resources.get_figure_code().soul_player) {
        CHECK(resources.seize_figure(object_id) == ase::kernel::HostStatusOk);
    }
    REQUIRE(led_by_player(resources, object_id));
    const VivariumFigureView* standing = figure_of(resources, object_id);
    REQUIRE(standing != nullptr);
    const double before = standing->course;

    CHECK(resources.turn_figure(object_id, kFigureTurn) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "turns on the spot by"));
    advance_frames(resources, kFigureBoundFrames);
    const VivariumFigureView* turned = figure_of(resources, object_id);
    REQUIRE(turned != nullptr);
    CHECK(turned_by(before, turned->course) == doctest::Approx(kFigureTurn).epsilon(kFigureTurnSlack / kFigureTurn));
}

/*
 * THE CUT OF THE FIGURE (usecase:100411 - learn skills by experience, MIG_ASE_SKILL_RSM B2, step 12). The prune port
 * takes the organ and the figure of the seat; ase-phytomer fells the branch and books the cut on the count of work of
 * that figure (the booking itself is pinned in modules/ase-phytomer/tests/test_phytomer_bok.cpp, the port that carries
 * the count to the client is B3). Here: a tap of the led figure on a branch internode fells it, a tap on the stem
 * leaves it standing.
 */
TEST_CASE("usecase:100411 learn skills by experience: the led figure cuts a branch through the prune port, a tap on the stem fells nothing") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* first = the_figure(resources);
    REQUIRE(first != nullptr);
    const uint32_t figure = first->object_id;

    // THE CASE NEEDS A FOUNDER THAT BEARS LATERAL BUDS (register B1012). The founder operator of ase-genetics shifts
    // every gene by its mutation strength (GeneticsIniPlntIndvSystem, PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.2); on
    // PLT_SPEC_LAT_BUT_COUNT (0..100, arosaruebe 3) that is up to 10 buds either way, and the founder of place 0
    // (seed 1) lands on 0: its shoot never branches. Measured 2026-11-27 (data/probes/runs/founder_lat_buds_1127.txt,
    // the same formula holds the line of PhytomerSideIniSystem for seed 1): place 2 (seed 3) bears 3.9 buds per node
    // of its stem, the nearest to the catalog value of the four places.
    constexpr uint32_t kBranchingPlace = 2u;
    // Its seed grows before the camera, as the tree case lets it (test_godot_host_tree.cpp, B0295 point 4)
    advance_frames(resources, 8u);
    const VivariumPatchView& seen = test::patch_at(resources, kBranchingPlace);
    const uint32_t cared = seen.object_id;
    REQUIRE(resources.view(seen.x, seen.y + 3.0, seen.z + 6.0, seen.x, seen.y, seen.z) == ase::kernel::HostStatusOk);
    constexpr uint32_t kSoilBoundFrames = 32u;
    REQUIRE(test::await_soil_class(resources, kSoilBoundFrames));
    const double amount_mm = test::vivarium_config(env.bundle).irrigate_amount_mm;
    const double capacity_mm = test::patch_at(resources, kBranchingPlace).capacity_mm;
    constexpr uint32_t kBoundFrames = 256u;
    for (double poured_mm = amount_mm;
         poured_mm < capacity_mm + amount_mm && resources.get_organs(GODOT_ORGAN_SEGMENT).empty();
         poured_mm += amount_mm) {
        REQUIRE(resources.irrigate(cared, amount_mm) == ase::kernel::HostStatusOk);
        for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_organs(GODOT_ORGAN_SEGMENT).empty(); ++frame) {
            advance_frames(resources, 1u);
        }
    }
    REQUIRE(!resources.get_organs(GODOT_ORGAN_SEGMENT).empty());

    // The jump to the hour of MADURA: a grown shoot carries branches (order 1 and above) beside its stem (order 0)
    for (uint32_t frame = 0u; frame < kBoundFrames && !(test::patch_at(resources, kBranchingPlace).mtur_at_h > 0.0);
         ++frame) {
        advance_frames(resources, 1u);
    }
    const double mature_h = test::patch_at(resources, kBranchingPlace).mtur_at_h;
    REQUIRE(mature_h > 0.0);
    REQUIRE(resources.seek(1.0, mature_h, GODOT_SEEK_FORM_NONE, GODOT_SEEK_FORM_NONE) == ase::kernel::HostStatusOk);
    // The clock reaches the hour of the jump first, as in the tree case (test_godot_host_tree.cpp, B0295 point 4),
    // then the organ port pages the grown shoot: it stands when its size held over a whole run and more
    const uint32_t frames = test::await_hour(resources, mature_h);
    std::string growth_steps;
    uint32_t still = test::await_still_shoot(resources, growth_steps);
    REQUIRE(still >= test::kShootStillFrames);
    // The days after the jump (B0988): measured 2026-11-25, the shoot stood still right after the jump with 57
    // internodes all of order 0 (data/probes/runs/figure_prune_1125.txt). Its buds burst in the cycles of the days
    // after the jump, as the birch of test_phytomer.cpp bears 10 main internodes two days past its jump (B0299,
    // data/probes/runs/phytomer_jump_branches_1125.txt); each day is a seek of one game day ahead.
    // Those days lie in summer, as the days of B0299 do: the hour of MADURA measured 2026-11-27 (3.2485e6 h) lands on
    // year day 3737, HALITRIX, where the leaves fall (growth_steps 1 → 0 leaves); one seek carries the clock to the
    // first day of IGNIX first (register T0415).
    const bool branched = test::shoot_branched(resources);  // the summer and the days run only without a branch (B1012)
    CAPTURE(branched);
    double seq = 2.0;
    const double summer_h = test::hours_to_summer(resources.get_clock());
    CAPTURE(summer_h);
    REQUIRE(summer_h >= 0.0);
    if (!branched && summer_h > 0.0) {
        const VivariumClockView& clock = resources.get_clock();
        REQUIRE(clock.seconds_per_hour > 0.0);
        const double target_h = clock.elapsed_s / clock.seconds_per_hour + summer_h;
        REQUIRE(resources.seek(seq, GODOT_SEEK_FORM_NONE, summer_h, GODOT_SEEK_FORM_NONE) == ase::kernel::HostStatusOk);
        seq += 1.0;
        (void)test::await_hour(resources, target_h);
        growth_steps += "|summer ";
        still = test::await_still_shoot(resources, growth_steps);
    }
    for (uint32_t day = 1u; !branched && day <= test::kDaysAfterJump; ++day) {
        const VivariumClockView& clock = resources.get_clock();
        REQUIRE(clock.seconds_per_hour > 0.0);
        REQUIRE(clock.hours_per_day > 0.0);
        const double target_h = clock.elapsed_s / clock.seconds_per_hour + clock.hours_per_day;
        REQUIRE(resources.seek(seq, GODOT_SEEK_FORM_NONE, clock.hours_per_day, GODOT_SEEK_FORM_NONE) ==
                ase::kernel::HostStatusOk);
        seq += 1.0;
        (void)test::await_hour(resources, target_h);
        growth_steps += "| ";
        still = test::await_still_shoot(resources, growth_steps);
    }
    uint32_t branch = 0u;
    uint32_t stem = 0u;
    for (const VivariumOrganView& organ : resources.get_organs(GODOT_ORGAN_SEGMENT)) {
        if (organ.order >= 1.0 && branch == 0u) branch = organ.object_id;
        if (organ.order < 1.0 && stem == 0u) stem = organ.object_id;
    }
    CAPTURE(mature_h);
    CAPTURE(frames);
    CAPTURE(growth_steps);  // frame:internodes/leaves/o<top order> at every change, "|" before each day after the jump
    CAPTURE(resources.get_organs(GODOT_ORGAN_LEAF).size());
    CAPTURE(resources.get_organs(GODOT_ORGAN_SEGMENT).size());  // growing: the shoot never held still on the last day
    REQUIRE(still >= test::kShootStillFrames);
    REQUIRE(branch != 0u);
    REQUIRE(stem != 0u);

    const auto carries_organ = [&resources](uint32_t object_id) {
        for (const VivariumOrganView& organ : resources.get_organs(GODOT_ORGAN_SEGMENT)) {
            if (organ.object_id == object_id) return true;
        }
        return false;
    };

    // The tap of the figure on the branch: the request goes out with the figure, the branch falls off the port
    CHECK(resources.prune(branch, figure) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "cut at organ"));
    for (uint32_t frame = 0u; frame < kBoundFrames && carries_organ(branch); ++frame) {
        advance_frames(resources, 1u);
    }
    CHECK_FALSE(carries_organ(branch));

    // The tap on the stem: the stem is no branch, it stands on
    CHECK(resources.prune(stem, figure) == ase::kernel::HostStatusOk);
    advance_frames(resources, kFigureBoundFrames);
    CHECK(carries_organ(stem));
}

/*
 * THE SPADE STROKE OF THE FIGURE (usecase:100411 - learn skills by experience, MIG_ASE_SKILL_RSM B2, register D2650).
 * The dig and heap ports take the place and the figure of the seat; ase-pl-flora orders the stroke from ase-terrain
 * and books it as work of that figure (HUB_SKL_WORK_DIG, taken by ase-skill, pinned in modules/ase-skill/tests/
 * test_skill.cpp). Here: the stroke of the led figure is booked, a stroke without a figure stands and is booked on
 * no one.
 */
TEST_CASE("usecase:100411 learn skills by experience: the dig and the heap of the led figure are booked as its work, a stroke without a figure on no one") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* first = the_figure(resources);
    REQUIRE(first != nullptr);
    const uint32_t figure = first->object_id;
    const double x = first->x + 1.0;
    const double z = first->z + 1.0;
    constexpr double kStrokeHalfM = 0.5;
    constexpr double kStrokeDepthM = 0.1;

    // The dig of the figure: ordered from ase-terrain and booked as its work
    CHECK(resources.lower(x, z, kStrokeHalfM, kStrokeDepthM, figure) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "dig stroke booked as work of figure"));

    // The heap of the figure on the same place: booked the same way under its own action
    CHECK(resources.raise(x, z, kStrokeHalfM, kStrokeDepthM, figure) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "heap stroke booked as work of figure"));

    // A stroke while no figure is led: the ground is dug, no one books it
    CHECK(resources.lower(x, z, kStrokeHalfM, kStrokeDepthM, GODOT_NO_OBJECT) == ase::kernel::HostStatusOk);
    std::vector<std::string> lines;
    std::vector<std::string> seen;
    for (uint32_t frame = 0u; frame < kFigureBoundFrames; ++frame) {
        advance_frames(resources, 1u);
        lines.clear();
        (void)resources.remove_log_lines(lines);
        seen.insert(seen.end(), lines.begin(), lines.end());
    }
    CHECK(any_line_carries(seen, "dig ordered at"));
    CHECK_FALSE(any_line_carries(seen, "dig stroke booked as work of figure"));
}

}  // namespace ase::adp::godot

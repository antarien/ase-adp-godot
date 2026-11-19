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
 * @modified    2026-11-18
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_figure_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::any_line_carries;
using test::figure_environment;
using test::figure_of;
using test::frames_carry;
using test::kFigureBoundFrames;
using test::kFigureSpawnX;
using test::kFigureSpawnZ;
using test::the_figure;

namespace {

/** A figure stands within this distance of its spawn place, metres: nothing moves it without a stick. */
constexpr double kFigureSpawnSlack = 0.5;
/** The soul value of a figure its player leads (SS/PC) the code port must name: the expected value, not the one compared against. */
constexpr double kFigureSoulPlayer = 1.0;
/** The skeleton of the figure as the scene measures it, metres. */
constexpr double kFigureEyeHeight = 1.62;
constexpr double kFigureHeight = 1.78;
constexpr double kFigureRadius = 0.19;  // half the span of the shoulder joints of villager_body (L/R_Upperarm, glb_bones.out)
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

TEST_CASE("usecase:101625 carry a tool in hand: every piece of the gear port names a figure of the body port at its right hand") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(the_figure(resources) != nullptr);
    CHECK(resources.hold_figure(resources.get_figures().front().object_id,
                                ase::kernel::KernelEmbeddedHost::name_id("enxada")) == ase::kernel::HostStatusOk);
    advance_frames(resources, kFigureBoundFrames);

    const uint32_t hand = ase::kernel::KernelEmbeddedHost::name_id("socket_hand_r");
    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        bool borne = false;
        for (const VivariumFigureView& figure : resources.get_figures()) {
            borne = borne || figure.object_id == piece.bearer;
        }
        CHECK(borne);
        CHECK(piece.object_id != 0u);
        CHECK(piece.slot == hand);
        CHECK(piece.kind != 0u);
    }
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
    CHECK(resources.drain_figure(figure, 1.0) == ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "lets 1 litres go, drain request filed"));
    for (const VivariumFigureGearView& piece : resources.get_figure_gear()) {
        if (piece.vessel) {
            CHECK(std::isfinite(piece.water_l));
            CHECK(piece.water_l >= 0.0);
        }
    }

    // A figure the host does not know is the plugin's to refuse: the port still takes the request.
    CHECK(resources.drain_figure(0u, 1.0) == ase::kernel::HostStatusOk);
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

}  // namespace ase::adp::godot

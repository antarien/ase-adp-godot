/**
 * ASE ADAPTER TESTS - GODOT HOST FIGURE PORTS
 *
 * @file        test_godot_host_figure.cpp
 * @design      DSGN_030
 * @brief       Pins what the figure ports of one game host take and carry: the figure of the seat, the hand of Phase 06
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.8 and 07.10, against the real bundle with the
 *              configuration of test_godot_host_support.hpp and the figure ports of the client's start
 *              configuration (config/vivarium_start.json, "ports" figure_*): boot() asks ase-pl-avatar for
 *              the figure of the seat, the body port carries it at its spawn place, a hold of a thing of the
 *              hand files a put-on request and a drop a put-down request (usecase:101625 carry a tool in hand),
 *              and every piece the gear port carries names a figure of the body port at the right hand.
 *              07.6 and 07.7: the rig port takes the measured skeleton of the figure (eye height and height,
 *              the face landing of ase-camera reads it), and the turn port turns a figure its player leads on
 *              the spot - the body port then carries the turned course (fig.turn). 07.5 and 07.6 on the orbit
 *              port: a drag is a whole turn request the plugin takes, and a zoom towards the figure shortens
 *              the distance of the player camera from its eye to its aim.
 *
 *              THE GEAR PORT ANSWERS ONLY WITH ase-equipment IN THE SET: the plugin files the request, the
 *              equipment module puts the piece on. Until the project file names the module the port stays
 *              empty, and the case pins what holds either way - every record it carries is well formed.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside those of test_godot_host.cpp and
 *              test_godot_host_scene.cpp, with the figure ports added to the shared configuration.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101625 - carry a tool in hand
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::any_line_carries;

namespace {

/** The seat and spawn place of the client's start configuration ("figure"): the planting place by the birch. */
constexpr uint32_t kFigureSeat = 1u;
constexpr double kFigureSpawnX = 16.5;
constexpr double kFigureSpawnZ = 13.6;
/** A figure stands within this distance of its spawn place, metres: nothing moves it without a stick. */
constexpr double kFigureSpawnSlack = 0.5;
/** Frames the plugin gets to answer a request, a quarter second each. */
constexpr uint32_t kFigureBoundFrames = 32u;
/** The soul value of a figure its player leads (SS/PC) the code port must name: the expected value, not the one compared against. */
constexpr double kFigureSoulPlayer = 1.0;
/** The skeleton of the figure as the scene measures it, metres. */
constexpr double kFigureEyeHeight = 1.62;
constexpr double kFigureHeight = 1.78;
/** One turn on the spot, radians, inside the span of one request. */
constexpr double kFigureTurn = 0.5;
/** Slack of the turned course, radians. */
constexpr double kFigureTurnSlack = 0.05;
constexpr double kFullCircle = 6.283185307179586;
/** One step of the zoom rail towards the figure, metres, well inside the rail from the shoulder to the face. */
constexpr double kFigureZoomStep = -1.0;
/** The share of the zoom step the distance must have moved once the frames ran, against smoothing. */
constexpr double kFigureZoomShare = 0.5;
/** A small drag of the right half, radians around. */
constexpr double kFigureDragYaw = 0.1;

/** The figure ports and operations as the client's start configuration names them. */
void add_figure_ports(VivariumStartConfig& config) {
    config.figure_body_port = "avatar.body.v1";
    config.figure_camera_port = "avatar.camera.v1";
    config.figure_spawn_port = "avatar.spawn.v1";
    config.figure_stick_port = "avatar.stick.v1";
    config.figure_orbit_port = "avatar.orbit.v1";
    config.figure_switch_port = "avatar.switch.v1";
    config.figure_release_port = "avatar.release.v1";
    config.figure_seize_port = "avatar.seize.v1";
    config.figure_hold_port = "avatar.hold.v1";
    config.figure_drop_port = "avatar.drop.v1";
    config.figure_gear_port = "avatar.gear.v1";
    config.figure_rig_port = "avatar.rig.v1";
    config.figure_turn_port = "avatar.turn.v1";
    config.figure_code_port = "avatar.code.v1";
    config.op_spawn = "spawn";
    config.op_steer = "steer";
    config.op_turn = "turn";
    config.op_switch = "switch";
    config.op_release = "release";
    config.op_seize = "seize";
    config.op_hold = "hold";
    config.op_drop = "drop";
    config.op_measure = "measure";
    config.figure_seat = kFigureSeat;
    config.figure_x = kFigureSpawnX;
    config.figure_z = kFigureSpawnZ;
}

/** The environment of this run, booted with the figure ports. */
AdapterEnvironment& environment() {
    return test::environment(&add_figure_ports);
}

/** The figure of the seat once the body port carries it; frames advance until then, a bounded number. */
const VivariumFigureView* the_figure(GodotHostResourceManager& resources) {
    for (uint32_t frame = 0u; frame < kFigureBoundFrames && resources.get_figures().empty(); ++frame) {
        advance_frames(resources, 1u);
    }
    return resources.get_figures().empty() ? nullptr : &resources.get_figures().front();
}

/** The host's lines of the next frames, until one carries `text` or the bound runs out. */
bool frames_carry(GodotHostResourceManager& resources, const std::string& text) {
    std::vector<std::string> lines;
    for (uint32_t frame = 0u; frame < kFigureBoundFrames; ++frame) {
        advance_frames(resources, 1u);
        (void)resources.remove_log_lines(lines);
        if (any_line_carries(lines, text)) {
            return true;
        }
    }
    return false;
}

/** The figure `object_id` as the body port carries it now, nullptr when it carries no such figure. */
const VivariumFigureView* figure_of(GodotHostResourceManager& resources, uint32_t object_id) {
    for (const VivariumFigureView& figure : resources.get_figures()) {
        if (figure.object_id == object_id) {
            return &figure;
        }
    }
    return nullptr;
}

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

/** Frames advance until the camera port carries the record of the active player camera; a bounded number. */
bool player_camera_known(GodotHostResourceManager& resources) {
    for (uint32_t frame = 0u; frame < kFigureBoundFrames; ++frame) {
        if (resources.get_figure_camera().known) {
            return true;
        }
        advance_frames(resources, 1u);
    }
    return resources.get_figure_camera().known;
}

/** The distance from the eye of the player camera to its aim, metres, as the camera port carries it now. */
double camera_distance(const VivariumFigureCameraView& camera) {
    const double dx = camera.eye_x - camera.aim_x;
    const double dy = camera.eye_y - camera.aim_y;
    const double dz = camera.eye_z - camera.aim_z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
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
    AdapterEnvironment& env = environment();
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
    AdapterEnvironment& env = environment();
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
    AdapterEnvironment& env = environment();
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
    AdapterEnvironment& env = environment();
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

TEST_CASE("07.6: the rig port takes the measured skeleton of the figure for the face landing of the camera") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    CHECK(resources.measure_figure(figure->object_id, kFigureEyeHeight, kFigureHeight) ==
          ase::kernel::HostStatusOk);
    CHECK(frames_carry(resources, "measured, eyes at"));
}

TEST_CASE("07.7 fig.turn: a figure its player leads turns on the spot and the body port carries the turned course") {
    AdapterEnvironment& env = environment();
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

TEST_CASE("07.5 cam.drag: a drag of the right half is a whole turn request the plugin takes, never one it refuses") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    CHECK(resources.turn_figure_camera(figure->object_id, kFigureDragYaw, 0.0) == ase::kernel::HostStatusOk);
    // AvatarHostOrbtSystem refuses a request without yaw_delta, pitch_delta and zoom_delta with this line.
    CHECK_FALSE(frames_carry(resources, "a turn request needs yaw_delta, pitch_delta and zoom_delta"));
}

TEST_CASE("07.6 cam.zoom: a zoom towards the figure shortens the distance of the player camera along its rail") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* first = the_figure(resources);
    REQUIRE(first != nullptr);
    const uint32_t object_id = first->object_id;

    // The fixed view of Phase 06 is the start view; cam.mode moves the active mark to the camera following the figure.
    if (!resources.get_figure_camera().known) {
        CHECK(resources.switch_figure_camera(object_id) == ase::kernel::HostStatusOk);
    }
    REQUIRE(player_camera_known(resources));
    const double before = camera_distance(resources.get_figure_camera());

    CHECK(resources.zoom_figure_camera(object_id, kFigureZoomStep) == ase::kernel::HostStatusOk);
    CHECK_FALSE(frames_carry(resources, "a turn request needs yaw_delta, pitch_delta and zoom_delta"));
    REQUIRE(resources.get_figure_camera().known);
    const double after = camera_distance(resources.get_figure_camera());
    CHECK(before - after > -kFigureZoomStep * kFigureZoomShare);
}

}  // namespace ase::adp::godot

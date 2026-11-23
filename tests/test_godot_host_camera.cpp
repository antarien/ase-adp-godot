/**
 * ASE ADAPTER TESTS - GODOT HOST CAMERA PORTS
 *
 * @file        test_godot_host_camera.cpp
 * @design      DSGN_030
 * @brief       Pins what the camera ports of one game host take and carry: the switch, the drag, the zoom and the rim
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.5, 07.6, 07-A and 07-G, against the real bundle with the
 *              configuration of test_godot_host_support.hpp and the figure ports of the client's start
 *              configuration (test_godot_host_figure_support.hpp). 07-A on the switch port: one tap on
 *              cam.mode moves the active mark to the other camera and a second tap brings back the view it
 *              left (usecase:101588 move player figure over ground), so the fixed view of Phase 06 never
 *              doubles. 07.5 and 07.6 on the orbit port: a drag is a whole turn request the plugin takes, and
 *              a zoom towards the figure shortens the distance of the player camera from its eye to its aim.
 *              07-G: a bound figure led against a chunk border of the loaded ase-gis raster stops on its rim,
 *              the bump of ase-physics names one raster axis, and the camera carries edge.
 *
 *              WHICH CAMERA IS ACTIVE IS ase-camera's: the camera port carries a record exactly while the
 *              player camera is the active one. The view only asks for the other camera; every case here
 *              reads the answer from the port, never from the request it filed.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside test_godot_host_figure.cpp, which drives
 *              the body, the gear and the motion of the same figure on a game host of its own.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101588 - move player figure over ground
 * @actor       SS/PC
 * @created     2026-11-18
 * @modified    2026-11-18
 * @version     00.00.23.00023 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_figure_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::figure_environment;
using test::figure_of;
using test::frames_carry;
using test::kFigureBoundFrames;
using test::kFigureEyeHeight;
using test::kFigureHeight;
using test::kFigureRadius;
using test::the_figure;

namespace {

/** One step of the zoom rail towards the figure, metres, well inside the rail from the shoulder to the face. */
constexpr double kFigureZoomStep = -1.0;
/** The share of the zoom step the distance must have moved once the frames ran, against smoothing. */
constexpr double kFigureZoomShare = 0.5;
/** A small drag of the right half, radians around. */
constexpr double kFigureDragYaw = 0.1;
/** The test patch the figure is bound to for the rim: the plate whose anchor the figure walks on. */
constexpr uint32_t kFigureRimPlace = 1u;
/**
 * Slack of the place on the rim, metres: the figure stands within the margin ase-player keeps from a chunk border
 * (PLR_AREA_EDGE_M, 0.35 m) plus one frame of rounding, never beyond the border ase-physics touched.
 */
constexpr double kFigureRimSlack = 0.75;
/** Frames the stick leads the figure towards a chunk border: a quarter second each, at run speed 2 m a frame. */
constexpr uint32_t kFigureRimFrames = 64u;

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

/** Frames advance until the camera port carries a record exactly when `known`; a bounded number. */
bool camera_known_is(GodotHostResourceManager& resources, bool known) {
    for (uint32_t frame = 0u; frame < kFigureBoundFrames; ++frame) {
        advance_frames(resources, 1u);
        if (resources.get_figure_camera().known == known) {
            return true;
        }
    }
    return false;
}

/** Frames advance until the camera record carries the rim of the followed figure; a bounded number. */
bool rim_marked(GodotHostResourceManager& resources) {
    for (uint32_t frame = 0u; frame < kFigureRimFrames; ++frame) {
        if (resources.get_figure_camera().known && resources.get_figure_camera().edge) {
            return true;
        }
        advance_frames(resources, 1u);
    }
    return resources.get_figure_camera().edge;
}

/** The distance from the eye of the player camera to its aim, metres, as the camera port carries it now. */
double camera_distance(const VivariumFigureCameraView& camera) {
    const double dx = camera.eye_x - camera.aim_x;
    const double dy = camera.eye_y - camera.aim_y;
    const double dz = camera.eye_z - camera.aim_z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace

TEST_CASE("07-A usecase:101588 cam.mode: one tap moves the active mark to the other camera, a second tap brings back the view it left") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    const uint32_t object_id = figure->object_id;

    // The fixed view of Phase 06 is the start view of the run: no switch was filed yet, so no player camera stands.
    const bool start = resources.get_figure_camera().known;
    CHECK_FALSE(start);
    CHECK(resources.switch_figure_camera(object_id) == ase::kernel::HostStatusOk);
    CHECK(camera_known_is(resources, !start));
    CHECK(resources.switch_figure_camera(object_id) == ase::kernel::HostStatusOk);
    CHECK(camera_known_is(resources, start));
}

TEST_CASE("07.5 cam.drag: a drag of the right half is a whole turn request the plugin takes, never one it refuses") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* figure = the_figure(resources);
    REQUIRE(figure != nullptr);
    CHECK(resources.turn_figure_camera(figure->object_id, kFigureDragYaw, 0.0) == ase::kernel::HostStatusOk);
    // AvatarHostOrbtSystem refuses a request without yaw_delta, pitch_delta and zoom_delta with this line.
    CHECK_FALSE(frames_carry(resources, "a turn request needs yaw_delta, pitch_delta and zoom_delta"));
}

TEST_CASE("07.6 cam.zoom: a zoom towards the figure shortens the distance of the player camera along its rail") {
    AdapterEnvironment& env = figure_environment();
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

TEST_CASE("07-G RAND DES AREALS: a bound figure led against a chunk border stops on the raster rim and the camera carries edge") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    const VivariumFigureView* first = the_figure(resources);
    REQUIRE(first != nullptr);
    const uint32_t object_id = first->object_id;
    if (!resources.get_figure_camera().known) {
        CHECK(resources.switch_figure_camera(object_id) == ase::kernel::HostStatusOk);
    }
    REQUIRE(player_camera_known(resources));

    // Unbound, the figure has no area and so no rim: the camera record carries edge 0.
    CHECK_FALSE(resources.get_figure_camera().edge);

    // The Godot host measures the skeleton of every figure it spawns; this executable stands in for it. The rim
    // mark is the mark of ase-physics (HUB_PHYS_STAND_BNDY), and ase-physics holds only a body with a capsule.
    CHECK(resources.measure_figure(object_id, kFigureEyeHeight, kFigureHeight, kFigureRadius) ==
          ase::kernel::HostStatusOk);

    // Bound to the plate of place 1, the figure walks inside the chunk borders of the loaded ase-gis raster
    // (HUB_GIS_GRND, CAUSA_ASE_GIS B0541). The stick leads it forward at run speed; at the border ase-physics holds
    // the walking body and files the bump, ase-player sets the figure back onto the rim (PlayerSimBndySystem), and
    // the mark reaches the camera record.
    const VivariumPatchView* plate = resources.get_patch_at(kFigureRimPlace);
    REQUIRE(plate != nullptr);
    CHECK(resources.bind_figure_area(object_id, plate->object_id) == ase::kernel::HostStatusOk);
    CHECK(resources.steer_figure(object_id, 1.0, 0.0, 1.0, 0.0) == ase::kernel::HostStatusOk);
    REQUIRE(rim_marked(resources));
    CHECK(resources.get_figure_camera().figure == object_id);

    // The bump names the edge: its outward normal is one axis of the raster, never a direction around a centre.
    const VivariumFigureBumpView* bump = nullptr;
    for (const VivariumFigureBumpView& candidate : resources.get_figure_bumps()) {
        if (candidate.object_id == object_id) {
            bump = &candidate;
        }
    }
    REQUIRE(bump != nullptr);
    CHECK(bump->bumps > 0u);
    CHECK(std::fabs(bump->normal_x) + std::fabs(bump->normal_z) == doctest::Approx(1.0));

    // In the frame of the mark ase-physics holds the walking body where it stood: the step that would cross the border
    // is refused, so the figure reads one run step short of the rim (2 m a frame, never beyond the touch). ase-player
    // takes the answer of ase-physics in the Reception of the next tick (PlayerHubStndSyncSystem, CAUSA_ASE_PHYSICS)
    // and only then sets the figure onto the rim (PlayerSimBndySystem, PlayerWalkRplcTag); the body port carries that
    // place one frame later. The views of both ports are copies the next frame replaces, so the touch is kept first.
    const VivariumFigureView* held = figure_of(resources, object_id);
    REQUIRE(held != nullptr);
    const double touch_x = bump->touch_x;
    const double touch_z = bump->touch_z;
    const double normal_x = bump->normal_x;
    const double normal_z = bump->normal_z;
    const double held_inward = (touch_x - held->x) * normal_x + (touch_z - held->z) * normal_z;
    MESSAGE("07-G rim · touch " << touch_x << " " << touch_z << " · normal " << normal_x << " " << normal_z
                                << " · held figure " << held->x << " " << held->z << " · inward " << held_inward);
    CHECK(held_inward >= 0.0);  // the held body never crossed the touched border

    // On the axis of that edge the set-back figure stands inside the touched border, within the margin of the rim;
    // the stick still pushes, so it never crossed it.
    advance_frames(resources, 1u);
    const VivariumFigureView* set_back = figure_of(resources, object_id);
    REQUIRE(set_back != nullptr);
    const double inward = (touch_x - set_back->x) * normal_x + (touch_z - set_back->z) * normal_z;
    MESSAGE("07-G rim · set back · figure " << set_back->x << " " << set_back->z << " · inward " << inward);
    CHECK(inward >= 0.0);
    CHECK(inward <= kFigureRimSlack);
    CHECK(resources.steer_figure(object_id, 0.0, 0.0, 0.0, 0.0) == ase::kernel::HostStatusOk);
}

}  // namespace ase::adp::godot

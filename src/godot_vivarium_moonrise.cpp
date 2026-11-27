/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW MOONRISE JUMP
 *
 * @file        godot_vivarium_moonrise.cpp
 * @brief       VivariumView - the debug jump to the next rise of a moon
 * @description THE FIXED CAMERA looks along grid +z from 53.55 N (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, the observer),
 *              which ase-ephemeris' EPH_CONST_GRID_BEARING_DEG (69) lays on compass east-north-east, where moons rise. A moon passes through that narrow frame only for a few game hours of a day, so a jump to
 *              a fixed hour shows one only by chance. THE MOONRISE JUMP (debug.jumps[].moonrise) goes to the next
 *              rise instead.
 *
 *              THE RISE TIME is ase-ephemeris' forecast (next_moonrise of the sky port's eclipse-next record, game
 *              hours from now until the first moon crosses the horizon). The jump sends those hours ahead once
 *              through the seek port like every other jump (VivariumView::seek_jump), and ase-time lays the target
 *              from its own clock; without a forecast the jump is refused as unforecast. Every module computes its state at the new time in its own schedule, and the
 *              pace stays as it was before the jump.
 *
 *              THE LANDING (follow_moonrise, called by follow_debug on ase-time's verdict) notes which moons of the sky port
 *              stand in the camera's frustum above the horizon. The view computes no orbit and no rise time: the
 *              moons and their places are ase-ephemeris', the view only tests its own camera against them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_046
 * @category    time/period/cycle
 * @usecase     usecase:101120 - clock the growth cycle
 * @usecase     usecase:100390 - return to the garden
 * @actor       Growth Clock Controller (GROWCLOCON), Plant Keeper (PLNTKEEPER)
 * @created     2026-11-09
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/hud/log/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

// THE MOON IN THE FRAME: a moon direction of the sky port stands at GODOT_SKY_DISC_DISTANCE from the middle of the
// sky dome (sky_dome_centre, godot_plant_sky_disc.hpp), where stand_moon_disc puts it - read from the starfield node
// light_plant_stars stands on that middle every frame, the eye before the first sky - and counts when that point lies in the camera's frustum and above the horizon - below
// it the ground hides the disc. The first framed moon's index, azimuth and altitude in degrees go into where, the
// same reading as the scene line (godot_vivarium_scene.cpp, scene_counts): the sky encoding of ase-ephemeris
// (ephemeris_hrzn_math.hpp, x = cos(az) * across, z = sin(az) * across) gives the grid azimuth, and the compass
// azimuth is the grid azimuth plus EPH_CONST_GRID_BEARING_DEG minus 90 (+z on compass 69, grid 111 on compass east).
uint32_t moons_in_frame(const VivariumSkyView& sky, ::godot::Node3D* scene, ::godot::String& where) {
    if (scene == nullptr) {
        return 0u;
    }
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (eye == nullptr) {
        return 0u;
    }
    const ::godot::Node3D* dome_node =
        ::godot::Object::cast_to<::godot::Node3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_STARS)));
    const ::godot::Vector3 dome = dome_node != nullptr ? dome_node->get_position() : eye->get_position();
    uint32_t framed = 0u;
    const uint32_t moons = sky.moons < GODOT_SKY_MOONS_MAX ? sky.moons : GODOT_SKY_MOONS_MAX;
    for (uint32_t i = 0u; i < moons; ++i) {
        const VivariumMoonView& moon = sky.moon[i];
        const ::godot::Vector3 toward(static_cast<float>(moon.x), static_cast<float>(moon.y),
                                      static_cast<float>(moon.z));
        if (!(toward.y > 0.0f) ||
            !eye->is_position_in_frustum(dome + toward * GODOT_SKY_DISC_DISTANCE)) {
            continue;
        }
        if (framed == 0u) {
            const float up = toward.y > 1.0f ? 1.0f : toward.y;
            float azimuth = ase::math::atan2(toward.z, toward.x) * ase::math::RAD_TO_DEG;
            azimuth = azimuth < 0.0f ? azimuth + 360.0f : azimuth;
            where = " moon=" + ::godot::String::num_uint64(i) + " moon_az_alt=" + ::godot::String::num(azimuth, 0) +
                    "/" + ::godot::String::num(ase::math::asin(up) * ase::math::RAD_TO_DEG, 0);
        }
        ++framed;
    }
    return framed;
}

}  // namespace

void VivariumView::ask_moonrise(VivariumJump& next) {
    // THE ASK: ase-ephemeris' forecast of the next moonrise, sent once as hours ahead; ase-time lays the target
    // from its own clock.
    const VivariumSkyView& sky = resources_.get_sky();
    const double rise_h = sky.known && sky.eclipse_nexts > 0u ? sky.eclipse_next[0].next_moonrise : GODOT_HOUR_NONE;
    if (!(rise_h > 0.0)) {
        note("debug jump moonrise rise_h=" + ::godot::String::num(rise_h, GODOT_LOG_DECIMALS) + " result=unforecast");
        return;
    }
    next.moonrise = true;
    (void)seek_jump(next, GODOT_SEEK_FORM_NONE, rise_h, GODOT_SEEK_FORM_NONE,
                    "moonrise rise_h=" + ::godot::String::num(rise_h, GODOT_LOG_DECIMALS));
}

void VivariumView::follow_moonrise(const ::godot::String& at_words) {
    // THE LANDING: the moons of the sky port that stand in the camera's frustum above the horizon. A rise
    // without a moon in the frame is ase-ephemeris' forecast to fix.
    const VivariumSkyView& sky = resources_.get_sky();
    ::godot::String where;
    const uint32_t framed = sky.known ? moons_in_frame(sky, scene_state_.scene_, where) : 0u;
    note(::godot::String("debug jump ") + (framed > 0u ? "reached" : "missed reason=no_moon_in_frame") +
         " moonrise framed=" + ::godot::String::num_uint64(framed) + where + at_words + " " +
         measure(selected_place_));
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW SCENE COUNTS
 *
 * @file        godot_vivarium_scene_counts.cpp
 * @brief       scene_counts - what the 3D scene draws, in one line of the window acceptance
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 and register D1647: the line follow_scene writes on the
 *              events of ground, organs, hour and star set (godot_vivarium_scene.cpp). It counts what the ports
 *              carry and what the scene draws of it: ground relief, organs, light, the stars, moons and sun the
 *              camera frames and the starfield as it stands. It reads the scene and changes nothing in it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_scene_counts.hpp>
#include <ase/adp/godot/godot_scene_exposure.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

::godot::String scene_counts(const GodotHostResourceManager& resources, ::godot::Node3D* scene) {
    const VivariumSkyView& sky = resources.get_sky();
    // what of the sky the camera frames: a star or moon direction stands at GODOT_SKY_DISC_DISTANCE from the
    // middle of the sky dome (sky_dome_centre, godot_plant_sky_disc.hpp), where light_plant_stars and
    // stand_sky_disc put it, and counts when that point lies in the frustum. The middle is read from the
    // starfield node, which light_plant_stars stands on it every frame; before the first sky it is the eye.
    // The night pictures of run frames-20261106-181311 showed no star while the port carried 64: the
    // records, the framed ones and the drawn instances side by side say which link of the chain drops them.
    // A star record holds its place on the celestial sphere; the turn record of the sky port turns it into the
    // world as the starfield node does, so the world direction here is the same the picture draws
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    const ::godot::Node3D* dome_node =
        ::godot::Object::cast_to<::godot::Node3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_STARS)));
    const ::godot::Vector3 dome = dome_node != nullptr ? dome_node->get_position()
                                                       : (eye != nullptr ? eye->get_position() : ::godot::Vector3());
    const ::godot::Basis turn = sky.turn_known
        ? ::godot::Basis(::godot::Quaternion(static_cast<float>(sky.turn_x), static_cast<float>(sky.turn_y),
                                             static_cast<float>(sky.turn_z), static_cast<float>(sky.turn_w)).normalized())
        : ::godot::Basis();
    uint32_t stars_framed = 0u;
    uint32_t moons_framed = 0u;
    double star_low = 0.0;
    double star_high = 0.0;
    const uint32_t held = static_cast<uint32_t>(sky.star.size());
    const uint32_t stars = held < GODOT_SKY_STARS_MAX ? held : GODOT_SKY_STARS_MAX;
    ::godot::Vector3 first_world(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0u; i < stars; ++i) {
        const VivariumStarView& star = sky.star[i];
        const ::godot::Vector3 world =
            turn.xform(::godot::Vector3(static_cast<float>(star.x), static_cast<float>(star.y), static_cast<float>(star.z)));
        first_world = i == 0u ? world : first_world;
        star_low = i == 0u || world.y < star_low ? world.y : star_low;
        star_high = i == 0u || world.y > star_high ? world.y : star_high;
        if (eye != nullptr && sky.turn_known &&
            eye->is_position_in_frustum(dome + world * GODOT_SKY_DISC_DISTANCE)) {
            ++stars_framed;
        }
    }
    // every moon's azimuth and altitude in degrees, so the picture and the line say together where each moon
    // stands against the fixed camera, so the probe shows whether the simulation brings it through the frame.
    // The azimuth reads the sky encoding of ase-ephemeris (ephemeris_hrzn_math.hpp: x = cos(az) * across,
    // z = sin(az) * across); while EPH_CONST_GRID_BEARING_DEG puts +z to the east it is the compass azimuth,
    // clockwise from north, east at 90
    ::godot::String moon_dirs;
    const uint32_t moons = sky.moons < GODOT_SKY_MOONS_MAX ? sky.moons : GODOT_SKY_MOONS_MAX;
    for (uint32_t i = 0u; i < moons; ++i) {
        const VivariumMoonView& moon = sky.moon[i];
        const float moon_up = static_cast<float>(moon.y) < -1.0f ? -1.0f : (static_cast<float>(moon.y) > 1.0f ? 1.0f : static_cast<float>(moon.y));
        float moon_az = ase::math::atan2(static_cast<float>(moon.z), static_cast<float>(moon.x)) * ase::math::RAD_TO_DEG;
        moon_az = moon_az < 0.0f ? moon_az + 360.0f : moon_az;
        moon_dirs += (i == 0u ? "" : ",") + ::godot::String::num(moon_az, 0) + "/" +
                     ::godot::String::num(ase::math::asin(moon_up) * ase::math::RAD_TO_DEG, 0);
        if (eye != nullptr &&
            eye->is_position_in_frustum(dome +
                                        ::godot::Vector3(static_cast<float>(moon.x), static_cast<float>(moon.y),
                                                         static_cast<float>(moon.z)) * GODOT_SKY_DISC_DISTANCE)) {
            ++moons_framed;
        }
    }
    // the sun as the moons: its azimuth and altitude in the same encoding and whether its disc point lies in the
    // frustum, so a picture without the sun (row N03, the halo) says whether the sky or the camera hides it
    const float sun_up = static_cast<float>(sky.sun_y) < -1.0f ? -1.0f : (static_cast<float>(sky.sun_y) > 1.0f ? 1.0f : static_cast<float>(sky.sun_y));
    float sun_az = ase::math::atan2(static_cast<float>(sky.sun_z), static_cast<float>(sky.sun_x)) * ase::math::RAD_TO_DEG;
    sun_az = sun_az < 0.0f ? sun_az + 360.0f : sun_az;
    const uint32_t sun_framed =
        eye != nullptr && sky.sun_y > 0.0 &&
                eye->is_position_in_frustum(dome +
                                            ::godot::Vector3(static_cast<float>(sky.sun_x), static_cast<float>(sky.sun_y),
                                                             static_cast<float>(sky.sun_z)) * GODOT_SKY_DISC_DISTANCE)
            ? 1u
            : 0u;
    // the starfield as it stands: its visible instances and the width of the first one in metres; a width of
    // 0 is a starfield that draws nothing although its count says otherwise
    int64_t drawn = 0;
    double first_width = 0.0;
    const ::godot::MultiMeshInstance3D* field =
        ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_STARS)));
    if (field != nullptr && field->get_multimesh().is_valid()) {
        drawn = field->get_multimesh()->get_visible_instance_count();
        if (drawn > 0) {
            first_width = field->get_multimesh()->get_instance_transform(0).basis.get_scale().x;
        }
    }
    // the world direction of the first star record, east and north share: between two lines of a night it
    // moves when the sky turns and stands when it does not
    const double first_x = static_cast<double>(first_world.x);
    const double first_z = static_cast<double>(first_world.z);
    const ::godot::String sky_frame =
        " stars_framed=" + ::godot::String::num_uint64(stars_framed) +
        " stars_drawn=" + ::godot::String::num_int64(drawn) +
        " star_width_m=" + ::godot::String::num(first_width, GODOT_LOG_DECIMALS) +
        " star_up=" + ::godot::String::num(star_low, GODOT_LOG_DECIMALS) + ".." +
        ::godot::String::num(star_high, GODOT_LOG_DECIMALS) +
        " star0=" + ::godot::String::num(first_x, GODOT_LOG_DECIMALS) + "/" + ::godot::String::num(first_z, GODOT_LOG_DECIMALS) +
        " moons=" + ::godot::String::num_uint64(sky.moons) +
        " moons_framed=" + ::godot::String::num_uint64(moons_framed) +
        " moon_az_alt=" + (moon_dirs.is_empty() ? ::godot::String("-") : moon_dirs) +
        " sun_framed=" + ::godot::String::num_uint64(sun_framed) +
        " sun_az_alt=" + ::godot::String::num(sun_az, 0) + "/" +
        ::godot::String::num(ase::math::asin(sun_up) * ase::math::RAD_TO_DEG, 0);
    // the relief the ground is drawn from: lowest and highest sample and the steepest normal; a flat
    // field (every height equal, every normal straight up) shows here before it shows in the picture
    double low = 0.0;
    double high = 0.0;
    double flattest = 1.0;
    bool first = true;
    for (const VivariumTerrainView& sample : resources.get_terrain()) {
        low = first || sample.height < low ? sample.height : low;
        high = first || sample.height > high ? sample.height : high;
        flattest = sample.ny < flattest ? sample.ny : flattest;
        first = false;
    }
    return "ground_samples=" + ::godot::String::num_uint64(resources.get_terrain().size()) +
           " ground_low=" + ::godot::String::num(low, GODOT_LOG_DECIMALS) +
           " ground_high=" + ::godot::String::num(high, GODOT_LOG_DECIMALS) +
           " ground_min_ny=" + ::godot::String::num(flattest, GODOT_LOG_DECIMALS) +
           " ground_cells=" + ::godot::String::num_uint64(resources.get_ground_cells().size()) +
           " segments=" + ::godot::String::num_uint64(resources.get_organs(GODOT_ORGAN_SEGMENT).size()) +
           " leaves=" + ::godot::String::num_uint64(resources.get_organs(GODOT_ORGAN_LEAF).size()) +
           " sky_known=" + (sky.known ? "1" : "0") +
           " sun_intensity=" + ::godot::String::num(sky.sun_intensity, GODOT_LOG_DECIMALS) +
           " ambient_intensity=" + ::godot::String::num(sky.ambient_intensity, GODOT_LOG_DECIMALS) +
           // the start picture stood red and dim at game hour 15 (run 20261028-phase03-v52, grey 36.2):
           // elevation and colours of the sky port say whether the port or the picture carries it
           " sun_up=" + ::godot::String::num(sky.sun_y, GODOT_LOG_DECIMALS) +
           " sun_rgb=" + ::godot::String::num(sky.sun_r, GODOT_LOG_DECIMALS) + "/" +
           ::godot::String::num(sky.sun_g, GODOT_LOG_DECIMALS) + "/" + ::godot::String::num(sky.sun_b, GODOT_LOG_DECIMALS) +
           " sky_rgb=" + ::godot::String::num(sky.background_r, GODOT_LOG_DECIMALS) + "/" +
           ::godot::String::num(sky.background_g, GODOT_LOG_DECIMALS) + "/" +
           ::godot::String::num(sky.background_b, GODOT_LOG_DECIMALS) +
           // the heat shimmer of the fog record: above 0 the haze quad reads the drawn frame and writes it back
           // over the whole picture, the sky included (veil_plant_scene) - a sky darker than sky_rgb says by this
           // field whether the veil stood over it (register B0184)
           " shimmer=" + ::godot::String::num(sky.fog_shimmer, GODOT_LOG_DECIMALS) +
           // the exposure lift the camera and the background take this frame (scene_exposure): a sky pixel is
           // the background colour times this lift, so the field says in which colour space it reached the
           // screen - sRGB-encoded, or linear without the encoding (register B0184)
           " lift=" + ::godot::String::num(scene_exposure(sky), GODOT_LOG_DECIMALS) +
           // the star set of the stars port, the instances of the starfield (light_plant_stars): a night
           // picture without stars says by this count whether the port carried none or the picture hid them
           " stars=" + ::godot::String::num_uint64(sky.star.size()) +
           " star_set=" + ::godot::String::num_uint64(sky.star_generation) +
           " sidereal=" + ::godot::String::num(sky.set_sidereal, GODOT_LOG_DECIMALS) + sky_frame;
}

}  // namespace ase::adp::godot

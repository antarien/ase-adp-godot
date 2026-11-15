/**
 * ASE GODOT ADAPTER - THE BROOK OF THE CARE VIEW WITH THE SKY IT REFLECTS AND THE STROKES OF ITS CURRENT
 *
 * @file        godot_plant_brook_reflection.cpp
 * @brief       The sky the water of the care view's 3D scene reflects, and the light strokes running along its course
 * @description See godot_plant_brook_reflection.hpp. One write per global reaches every water material of
 *              GODOT_WATER_SHADER_FILE at once, so the brooks need no list and no material is touched per frame. The
 *              mockup fills the whole bed opaquely with the reflection (m.html drawWater 430 to 432); the operator's
 *              order keeps the brook water murky green, so a shown shade adds its reflect_share of the reflected sky
 *              as light on top and a shade not shown adds none.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_brook_reflection.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector4.hpp>

namespace ase::adp::godot {

void light_plant_brook_reflection(const BrookReflectionShade& shade) {
    ::godot::RenderingServer* server = ::godot::RenderingServer::get_singleton();
    if (server == nullptr) {
        return;
    }
    const ::godot::Vector2 start(shade.start_x, shade.start_z);
    const ::godot::Vector2 end(shade.end_x, shade.end_z);
    const bool shown = shade.shown && start.distance_squared_to(end) > 0.0f;
    // not shown, the defaults leave the water as its own colours paint it: no reflection, no strokes
    const BrookReflectionShade used = shown ? shade : BrookReflectionShade{};
    // one line whenever what the water is painted with moves (register B0263), measured against the values the last
    // line named, so the log holds each change once and never a line per frame. They are kept as a meta of the server
    // that owns the globals: reading the globals back is not possible, because outside the editor Godot answers
    // global_shader_parameter_get with an error and an empty value (2560 errors in a 30 s run,
    // acceptance/20261112-182647-b0263-clear4/frames_godot.log)
    ::godot::PackedFloat32Array now;
    now.push_back(used.reflect_share);
    now.push_back(used.streak_px);
    now.push_back(used.start_x);
    now.push_back(used.start_z);
    now.push_back(used.end_x);
    now.push_back(used.end_z);
    now.push_back(used.half_m);
    const ::godot::PackedFloat32Array noted = server->get_meta(GODOT_BROOK_NOTE_META, ::godot::PackedFloat32Array());
    bool moved = noted.size() != now.size();
    for (int64_t i = 0; !moved && i < now.size(); ++i) {
        moved = ::godot::Math::abs(noted[i] - now[i]) > GODOT_BROOK_NOTE_STEP;
    }
    if (moved) {
        server->set_meta(GODOT_BROOK_NOTE_META, now);
        ::godot::UtilityFunctions::print(
            ::godot::String("[") + GODOT_LOG_SOURCE + "] brook reflection shown=" + (shade.shown ? "1" : "0") +
            " drawn=" + (shown ? "1" : "0") + " share=" + ::godot::String::num(used.reflect_share, 2) +
            " far=" + used.far.to_html(false) + " near=" + used.near.to_html(false) +
            " streak=" + used.streak.to_html(true) + " count=" + ::godot::String::num(used.streak_count, 0) +
            " px=" + ::godot::String::num(used.streak_px, 2) + " start=" + ::godot::String::num(shade.start_x, 1) +
            "/" + ::godot::String::num(shade.start_z, 1) + " end=" + ::godot::String::num(shade.end_x, 1) + "/" +
            ::godot::String::num(shade.end_z, 1) + " half=" + ::godot::String::num(used.half_m, 2));
    }
    server->global_shader_parameter_set(GODOT_BROOK_FAR_COLOR, used.far);
    server->global_shader_parameter_set(GODOT_BROOK_NEAR_COLOR, used.near);
    server->global_shader_parameter_set(GODOT_BROOK_REFLECT_MIX, used.reflect_share);
    server->global_shader_parameter_set(GODOT_BROOK_STREAK_COLOR, used.streak);
    server->global_shader_parameter_set(
        GODOT_BROOK_STREAK,
        ::godot::Vector4(used.streak_count, used.streak_gap, used.streak_length, used.streak_speed));
    server->global_shader_parameter_set(GODOT_BROOK_STREAK_PX, used.streak_px);
    server->global_shader_parameter_set(GODOT_BROOK_START, ::godot::Vector2(used.start_x, used.start_z));
    server->global_shader_parameter_set(GODOT_BROOK_END, ::godot::Vector2(used.end_x, used.end_z));
    server->global_shader_parameter_set(GODOT_BROOK_HALF, used.half_m);
}

}  // namespace ase::adp::godot

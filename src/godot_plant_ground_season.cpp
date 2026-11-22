/**
 * ASE GODOT ADAPTER - THE GROUND OF THE CARE VIEW IN THE COLOUR OF ITS SEASON
 *
 * @file        godot_plant_ground_season.cpp
 * @brief       The season, the snow, the depth and the horizon the ground of the care view's 3D scene is painted in
 * @description See godot_plant_ground_season.hpp. One write per global reaches every soil material of
 *              GODOT_GROUND_SHADER_FILE at once, so the soils need no list and no material is touched per frame.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_ground_season.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

bool plant_ground_far_edge(const ::godot::Node3D* scene, double centre_x, double centre_z, double side_m,
                           ::godot::Vector3& edge) {
    if (scene == nullptr || !(side_m > 0.0)) {
        return false;
    }
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (eye == nullptr) {
        return false;
    }
    // the camera looks along its -Z; flattened onto the ground that is the way the mockup's rows run from the
    // foot of the screen (z 1) to the horizon (z 0)
    const ::godot::Transform3D place = eye->get_transform();
    const ::godot::Vector3 forward = -place.basis.orthonormalized().get_column(2);
    ::godot::Vector2 ahead(forward.x, forward.z);
    const float ahead_length = ahead.length();
    if (!(ahead_length > 0.0f)) {
        return false;
    }
    ahead /= ahead_length;
    const float half = static_cast<float>(side_m) * 0.5f;
    const ::godot::Vector3 far(static_cast<float>(centre_x) + ahead.x * half, GODOT_WATER_TABLE_M,
                               static_cast<float>(centre_z) + ahead.y * half);
    // an edge behind the camera has no row on the screen
    if (!((far - place.origin).dot(::godot::Vector3(ahead.x, 0.0f, ahead.y)) > 0.0f)) {
        return false;
    }
    edge = far;
    return true;
}

void light_plant_ground_season(::godot::Node3D* scene, const GroundSeasonShade& shade, const ::godot::Vector3* edge) {
    ::godot::RenderingServer* server = ::godot::RenderingServer::get_singleton();
    if (server == nullptr) {
        return;
    }
    // z 0 is the screen row the camera sees the square's far edge on, z 1 the frame's foot (m.html rowY); both stand in
    // the visible rect's units (the canvas the HUD frame lies in), and the shader reads its row in render pixels, so
    // both go over as shares of the visible height, which the render size shares with it
    float far_row = 0.0f;
    float height = 0.0f;
    const ::godot::Camera3D* eye =
        scene != nullptr
            ? ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)))
            : nullptr;
    if (eye != nullptr && edge != nullptr) {
        far_row = eye->unproject_position(*edge).y;
        const ::godot::Viewport* view = eye->get_viewport();
        if (view != nullptr) {
            height = view->get_visible_rect().size.y;
        }
    }
    const bool painted = shade.shown && eye != nullptr && edge != nullptr && height > 0.0f && shade.foot_row > far_row;
    // not painted, the defaults leave the cover as it is: no season's share, no darkening, no horizon blend
    const GroundSeasonShade used = painted ? shade : GroundSeasonShade{};
    const ::godot::Vector3 rows =
        painted ? ::godot::Vector3(far_row / height, used.foot_row / height, used.row_power) : ::godot::Vector3(0.0f, 1.0f, 1.0f);
    // one line whenever what the ground is painted with moves (register B0263): the share of the season, its colour,
    // the rows of z against the render height, the horizon's colour and the snow's share, measured against the values
    // the last line named, so the log holds each change once and never a line per frame. They are kept as a meta of
    // the server that owns the globals: reading the globals back is not possible, because outside the editor Godot
    // answers global_shader_parameter_get with an error and an empty value (2560 errors in a 30 s run,
    // acceptance/20261112-182647-b0263-clear4/frames_godot.log)
    const float mix_now = painted ? 1.0f : 0.0f;
    ::godot::PackedFloat32Array now;
    now.push_back(rows.x);
    now.push_back(rows.y);
    now.push_back(rows.z);
    now.push_back(mix_now);
    now.push_back(used.season.r);
    now.push_back(used.season.g);
    now.push_back(used.season.b);
    now.push_back(used.horizon.r);
    now.push_back(used.horizon.g);
    now.push_back(used.horizon.b);
    now.push_back(used.snow.a);
    const ::godot::PackedFloat32Array noted = server->get_meta(GODOT_GROUND_NOTE_META, ::godot::PackedFloat32Array());
    bool moved = noted.size() != now.size();
    for (int64_t i = 0; !moved && i < now.size(); ++i) {
        moved = ::godot::Math::abs(noted[i] - now[i]) > GODOT_GROUND_NOTE_STEP;
    }
    if (moved) {
        server->set_meta(GODOT_GROUND_NOTE_META, now);
        ::godot::UtilityFunctions::print(
            ::godot::String("[") + GODOT_LOG_SOURCE + "] ground season shown=" + (shade.shown ? "1" : "0") +
            " painted=" + (painted ? "1" : "0") + " far_row=" + ::godot::String::num(far_row, 1) +
            " foot_row=" + ::godot::String::num(shade.foot_row, 1) + " height=" + ::godot::String::num(height, 1) +
            " rows=" + ::godot::String::num(rows.x, 3) + "/" + ::godot::String::num(rows.y, 3) + "/" +
            ::godot::String::num(rows.z, 2) + " season=" + used.season.to_html(false) +
            " horizon=" + used.horizon.to_html(false) + " share=" + ::godot::String::num(used.horizon_share, 2) +
            " depth=" + ::godot::String::num(used.depth_floor, 2) + "/" + ::godot::String::num(used.depth_span, 2) +
            " snow=" + used.snow.to_html(true));
    }
    server->global_shader_parameter_set(GODOT_GROUND_SEASON_COLOR, used.season);
    server->global_shader_parameter_set(GODOT_GROUND_SNOW, used.snow);
    server->global_shader_parameter_set(GODOT_GROUND_SEASON_MIX, mix_now);
    server->global_shader_parameter_set(GODOT_GROUND_DEPTH, ::godot::Vector2(used.depth_floor, used.depth_span));
    server->global_shader_parameter_set(GODOT_GROUND_HORIZON_COLOR, used.horizon);
    server->global_shader_parameter_set(GODOT_GROUND_HORIZON_SHARE, used.horizon_share);
    server->global_shader_parameter_set(GODOT_GROUND_ROWS, rows);
}

}  // namespace ase::adp::godot

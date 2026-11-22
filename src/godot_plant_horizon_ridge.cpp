/**
 * ASE GODOT ADAPTER - THE RIDGES OF HILLS ON THE HORIZON OF THE CARE VIEW
 *
 * @file        godot_plant_horizon_ridge.cpp
 * @brief       The two ridges of hills the mockup draws on the horizon, stood in the sky of the care view's 3D scene
 * @description See godot_plant_horizon_ridge.hpp. The ridges stand on one upright square under the scene root, stood
 *              anew every frame ahead of the camera: its place and its turn follow the camera along the ground, its
 *              horizon line the ray to the ground's far edge, its size the HUD frame's pixels per mockup unit, its
 *              colours and its edge the shade.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_horizon_ridge.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

void build_plant_horizon_ridge(::godot::Node3D* scene) {
    ::godot::Ref<::godot::ShaderMaterial> look;
    look.instantiate();
    look->set_shader(load_plant_shader(GODOT_RIDGE_SHADER_FILE));
    ::godot::Ref<::godot::QuadMesh> square;
    square.instantiate();
    square->set_size(::godot::Vector2(1.0f, 1.0f));
    square->set_material(look);
    ::godot::MeshInstance3D* ridges = memnew(::godot::MeshInstance3D);
    ridges->set_name(::godot::String(GODOT_PLANT_HORIZON_RIDGE));
    ridges->set_mesh(square);
    ridges->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    ridges->set_visible(false);
    scene->add_child(ridges);
}

void light_plant_horizon_ridge(::godot::Node3D* scene, const HorizonRidgeShade& ridge, const ::godot::Vector3* edge) {
    ::godot::MeshInstance3D* ridges = ::godot::Object::cast_to<::godot::MeshInstance3D>(
        scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_HORIZON_RIDGE)));
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (ridges == nullptr || eye == nullptr) {
        return;
    }
    ::godot::Vector2 viewport;
    const ::godot::Viewport* view = eye->get_viewport();
    if (view != nullptr) {
        viewport = view->get_visible_rect().size;
    }
    const ::godot::Transform3D place = eye->get_transform();
    const ::godot::Basis turn = place.basis.orthonormalized();
    // the camera looks along its -Z; the square stands ahead of it along the ground, its sides along the camera's
    // right flattened onto the ground and the world's up, so the horizon line stays level
    const ::godot::Vector3 forward = -turn.get_column(2);
    ::godot::Vector3 ahead(forward.x, 0.0f, forward.z);
    const float ahead_length = ahead.length();
    // the depth along the camera's view, which the size of a pixel follows
    const float view_depth = ahead_length > 0.0f ? GODOT_RIDGE_DISTANCE * ahead.dot(forward) / ahead_length : 0.0f;
    // the distance of the ground's far edge along the ground, which the ray to it is measured over
    const float edge_ahead =
        (edge != nullptr && ahead_length > 0.0f) ? (*edge - place.origin).dot(ahead) / ahead_length : 0.0f;
    const bool drawn =
        ridge.shown && ridge.frame_pixels > 0.0f && viewport.y > 0.0f && view_depth > 0.0f && edge_ahead > 0.0f;
    ridges->set_visible(drawn);
    if (!drawn) {
        return;
    }
    ahead /= ahead_length;
    const ::godot::Vector3 up(0.0f, 1.0f, 0.0f);
    const ::godot::Vector3 right = ahead.cross(up);
    // a pixel at that depth spans 2·d·tan(fov/2) / viewport height metres - view geometry, the units are the shade's
    const float half_fov = static_cast<float>(eye->get_fov()) * 0.5f * ase::math::PI / 180.0f;
    const float metres_per_pixel = 2.0f * view_depth * ase::math::tan(half_fov) / viewport.y;
    const float metres_per_unit = ridge.frame_pixels * metres_per_pixel;
    const float across = viewport.x * metres_per_pixel * GODOT_RIDGE_SPAN;
    // the horizon line lies on the ray from the camera through the ground's far edge, at the square's distance
    const float horizon = place.origin.y + (edge->y - place.origin.y) * GODOT_RIDGE_DISTANCE / edge_ahead;
    const float tall = (ridge.top + ridge.foot) * metres_per_unit;
    // the square's middle stands half its span above the foot
    const float middle = (ridge.top - ridge.foot) * 0.5f * metres_per_unit;
    ::godot::Vector3 stand = place.origin + ahead * GODOT_RIDGE_DISTANCE;
    stand.y = horizon + middle;
    ridges->set_transform(::godot::Transform3D(::godot::Basis(right * across, up * tall, -ahead), stand));
    const ::godot::Ref<::godot::QuadMesh> square = ridges->get_mesh();
    if (square.is_null()) {
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> look = square->get_material();
    if (look.is_null()) {
        return;
    }
    look->set_shader_parameter(GODOT_RIDGE_FAR_COLOR, ridge.far_color);
    look->set_shader_parameter(GODOT_RIDGE_NEAR_COLOR, ridge.near_color);
    look->set_shader_parameter(GODOT_RIDGE_FAR, ::godot::Vector3(ridge.far_height, ridge.far_wave, ridge.far_phase));
    look->set_shader_parameter(GODOT_RIDGE_NEAR, ::godot::Vector3(ridge.near_height, ridge.near_wave, ridge.near_phase));
    look->set_shader_parameter(GODOT_RIDGE_LIFT, ridge.lift);
    look->set_shader_parameter(GODOT_RIDGE_BASE, ridge.base);
    look->set_shader_parameter(GODOT_RIDGE_SWING, ridge.swing);
    look->set_shader_parameter(GODOT_RIDGE_TOP, ridge.top);
    look->set_shader_parameter(GODOT_RIDGE_FOOT, ridge.foot);
    look->set_shader_parameter(GODOT_RIDGE_FRAME_X, ridge.frame_origin_x);
    look->set_shader_parameter(GODOT_RIDGE_FRAME_PIXELS, ridge.frame_pixels);
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - THE HALO AROUND THE SUN IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_sun_halo.cpp
 * @brief       The halo the mockup draws around the sun, stood in the sky of the care view's 3D scene
 * @description See godot_plant_sun_halo.hpp. The halo is one square under the scene root, stood anew every frame
 *              beside the sun disc: its place follows the middle of the sky dome the disc stands on, its turn
 *              the camera, its size the HUD frame's pixels per mockup unit, its colours and its opacity the
 *              weather of the shade.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/godot_plant_sun_halo.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/adp/godot/types.hpp>
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

namespace ase::adp::godot {

void build_plant_sun_halo(::godot::Node3D* scene) {
    ::godot::Ref<::godot::ShaderMaterial> glow;
    glow.instantiate();
    glow->set_shader(load_plant_shader(GODOT_SUN_HALO_SHADER_FILE));
    ::godot::Ref<::godot::QuadMesh> square;
    square.instantiate();
    square->set_size(::godot::Vector2(1.0f, 1.0f));
    square->set_material(glow);
    ::godot::MeshInstance3D* halo = memnew(::godot::MeshInstance3D);
    halo->set_name(::godot::String(GODOT_PLANT_SUN_HALO));
    halo->set_mesh(square);
    halo->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    halo->set_visible(false);
    scene->add_child(halo);
}

void light_plant_sun_halo(::godot::Node3D* scene, bool shown, const ::godot::Vector3& toward, float angular_size,
                          const SkyOvercastShade& overcast, const ::godot::Vector3& centre) {
    ::godot::MeshInstance3D* halo =
        ::godot::Object::cast_to<::godot::MeshInstance3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_SUN_HALO)));
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (halo == nullptr || eye == nullptr) {
        return;
    }
    float viewport_height = 0.0f;
    const ::godot::Viewport* view = eye->get_viewport();
    if (view != nullptr) {
        viewport_height = view->get_visible_rect().size.y;
    }
    // the halo stands where the sun disc stands (place_sky_disc: the middle of the sky dome `centre` along the sun
    // direction at GODOT_SKY_DISC_DISTANCE), GODOT_SUN_HALO_BEHIND disc radii farther
    const ::godot::Transform3D place = eye->get_transform();
    const ::godot::Basis turn = place.basis.orthonormalized();
    const ::godot::Vector3 along = toward.normalized();
    const float depth = GODOT_SKY_DISC_DISTANCE * (1.0f + GODOT_SUN_HALO_BEHIND * ase::math::tan(angular_size * 0.5f));
    const ::godot::Vector3 stand = centre + along * depth;
    // the depth of that place along the camera's view, which the size of a pixel follows; the camera looks along
    // its -Z
    const float view_depth = (stand - place.origin).dot(-turn.get_column(2));
    const bool drawn = shown && overcast.sun_halo > 0.0f && overcast.frame_pixels > 0.0f && viewport_height > 0.0f &&
                       view_depth > 0.0f;
    halo->set_visible(drawn);
    if (!drawn) {
        return;
    }
    // a pixel at that depth spans 2·d·tan(fov/2) / viewport height metres - view geometry, the radii are the shade's
    const float half_fov = static_cast<float>(eye->get_fov()) * 0.5f * ase::math::PI / 180.0f;
    const float metres_per_pixel = 2.0f * view_depth * ase::math::tan(half_fov) / viewport_height;
    const float across = 2.0f * overcast.sun_halo_outer * overcast.frame_pixels * metres_per_pixel;
    // THE SQUARE FACES THE CAMERA: its sides run along the camera's right and up, so the gradient reads as the
    // mockup's circle from where it is seen
    halo->set_transform(::godot::Transform3D(
        ::godot::Basis(turn.get_column(0) * across, turn.get_column(1) * across, turn.get_column(2)),
        stand));
    const ::godot::Ref<::godot::QuadMesh> square = halo->get_mesh();
    if (square.is_null()) {
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> glow = square->get_material();
    if (glow.is_null()) {
        return;
    }
    glow->set_shader_parameter(GODOT_SUN_HALO_COLOR, overcast.sun_halo_color);
    glow->set_shader_parameter(GODOT_SUN_HALO_EDGE, overcast.sun_halo_edge);
    glow->set_shader_parameter(GODOT_SUN_HALO_STRENGTH, overcast.sun_halo);
    // the shade's bind holds sun_halo_outer above sun_halo_inner, so the share stays below 1
    glow->set_shader_parameter(GODOT_SUN_HALO_INNER, overcast.sun_halo_inner / overcast.sun_halo_outer);
}

}  // namespace ase::adp::godot

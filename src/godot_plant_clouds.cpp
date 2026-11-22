/**
 * ASE GODOT ADAPTER - THE CLOUD BANKS IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_clouds.cpp
 * @brief       The cloud banks of an overcast sky, stood in the sky of the care view's 3D scene
 * @description See godot_plant_clouds.hpp. The banks are one multimesh at the origin of the world; each
 *              instance stands where the camera projects the bank's point of the picture to the depth
 *              GODOT_SKY_CLOUD_DISTANCE, turned with the camera, and is laid anew every frame, since the banks
 *              drift with the time of the HUD, each second weighed by the wind drift along the camera's right
 *              (cloud_drift_share, D2293).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0449 (sky.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_clouds.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

void build_plant_clouds(::godot::Node3D* scene) {
    // the veil of GODOT_CLOUD_SHADER_FILE: feathered, frayed and darkened at the foot by the tokens light_plant_clouds
    // hands it; colour and opacity of each instance are the bank's
    ::godot::Ref<::godot::ShaderMaterial> veil;
    veil.instantiate();
    veil->set_shader(load_plant_shader(GODOT_CLOUD_SHADER_FILE));
    ::godot::Ref<::godot::QuadMesh> bank;
    bank.instantiate();
    bank->set_size(::godot::Vector2(1.0f, 1.0f));
    ::godot::Ref<::godot::MultiMesh> field;
    field.instantiate();
    field->set_transform_format(::godot::MultiMesh::TRANSFORM_3D);
    field->set_use_colors(true);
    field->set_mesh(bank);
    field->set_instance_count(0);
    ::godot::MultiMeshInstance3D* clouds = memnew(::godot::MultiMeshInstance3D);
    clouds->set_name(::godot::String(GODOT_PLANT_CLOUDS));
    clouds->set_multimesh(field);
    clouds->set_material_override(veil);
    clouds->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    scene->add_child(clouds);
}

void light_plant_clouds(::godot::Node3D* scene, const std::vector<CloudBankEllipse>& banks,
                        const CloudBankVeil& veil, const ::godot::Transform2D& frame) {
    if (scene == nullptr) {
        return;
    }
    ::godot::MultiMeshInstance3D* clouds =
        ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CLOUDS)));
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (clouds == nullptr || eye == nullptr) {
        return;
    }
    const ::godot::Ref<::godot::MultiMesh> field = clouds->get_multimesh();
    if (field.is_null()) {
        return;
    }
    const int32_t count = static_cast<int32_t>(banks.size());
    if (field->get_instance_count() != count) {
        // one line on the flank where the sky gains or loses its banks (row N04): the run then names how many
        // banks the overcast lays, a clear sky none, and never a line per frame
        ::godot::UtilityFunctions::print(::godot::String("[") + GODOT_LOG_SOURCE + "] cloud banks count=" +
                                         ::godot::String::num_int64(count) + " was=" +
                                         ::godot::String::num_int64(field->get_instance_count()));
        field->set_instance_count(count);
    }
    if (count == 0) {
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> look = clouds->get_material_override();
    if (look.is_valid()) {
        look->set_shader_parameter(GODOT_CLOUD_REACH, veil.reach);
        look->set_shader_parameter(GODOT_CLOUD_SOFT, veil.soft);
        look->set_shader_parameter(GODOT_CLOUD_GRAIN, veil.grain);
        look->set_shader_parameter(GODOT_CLOUD_LIFT, veil.lift);
        look->set_shader_parameter(GODOT_CLOUD_SHADE, veil.shade);
    }
    float viewport_height = 0.0f;
    const ::godot::Viewport* view = eye->get_viewport();
    if (view != nullptr) {
        viewport_height = view->get_visible_rect().size.y;
    }
    // A pixel at the depth of the banks spans 2·d·tan(fov/2) / viewport height metres - the camera's vertical
    // field of view and the height of the viewport it draws into; that is view geometry, the bank is CloudBank's.
    const float half_fov = static_cast<float>(eye->get_fov()) * 0.5f * ase::math::PI / 180.0f;
    const float metres_per_pixel = viewport_height > 0.0f
        ? 2.0f * GODOT_SKY_CLOUD_DISTANCE * ase::math::tan(half_fov) / viewport_height
        : 0.0f;
    // the frame's pixels per frame unit, across and up
    const ::godot::Vector2 pixels = frame.get_scale();
    // THE BANK FACES THE CAMERA. The quad's sides run along the camera's right and up, its face toward the
    // camera, and it spans the bank's axes times sky_cloud_reach, so the veil can fray past the mockup's ellipse.
    const ::godot::Basis turn = eye->get_global_transform().basis.orthonormalized();
    const float reach = static_cast<float>(veil.reach);
    clouds->set_global_transform(::godot::Transform3D());
    for (int32_t i = 0; i < count; ++i) {
        const CloudBankEllipse& bank = banks[static_cast<size_t>(i)];
        const ::godot::Vector2 point = frame.xform(::godot::Vector2(static_cast<float>(bank.x), static_cast<float>(bank.y)));
        const ::godot::Vector3 place = eye->project_position(point, GODOT_SKY_CLOUD_DISTANCE);
        const float across = 2.0f * reach * static_cast<float>(bank.rx) * pixels.x * metres_per_pixel;
        const float up = 2.0f * reach * static_cast<float>(bank.ry) * pixels.y * metres_per_pixel;
        const ::godot::Basis shape(turn.get_column(0) * across, turn.get_column(1) * up, turn.get_column(2));
        field->set_instance_transform(i, ::godot::Transform3D(shape, place));
        field->set_instance_color(i, bank.color);
    }
}

double cloud_drift_share(::godot::Node3D* scene, const VivariumSkyView& sky) {
    if (!sky.wind_known || scene == nullptr) {
        return GODOT_SKY_CLOUD_COURSE;
    }
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (eye == nullptr) {
        return GODOT_SKY_CLOUD_COURSE;
    }
    // the world grid of the port is the scene's: x east, z north (the sun's direction stands so too)
    const ::godot::Vector3 right = eye->get_global_transform().basis.orthonormalized().get_column(0);
    const ::godot::Vector3 drift(static_cast<float>(sky.drift_x), 0.0f, static_cast<float>(sky.drift_z));
    return static_cast<double>(right.dot(drift));
}

}  // namespace ase::adp::godot

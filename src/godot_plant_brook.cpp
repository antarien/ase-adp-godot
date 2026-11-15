/**
 * ASE GODOT ADAPTER - THE WATER OF THE CARE VIEW
 *
 * @file        godot_plant_brook.cpp
 * @brief       The node of the water in the care view's 3D scene and its fill
 * @description See godot_plant_brook.hpp. The water plane is drawn after the opaque ground, transparent; the
 *              ground above the ase-hydro level of a bed hides it by the depth test, which draws the shore
 *              (build_water_table, B0281). Its vertex colour carries
 *              velocity, depth and course into the shader.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-01
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_brook.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The node of `parent` with that name, cast to its class; nullptr when it has none. */
template <typename T>
T* brook_part(::godot::Node* parent, const char* name) {
    return ::godot::Object::cast_to<T>(parent->get_node_or_null(::godot::NodePath(name)));
}

/**
 * The arrays of build_water_table as one surface of an ArrayMesh; a dry ground is no mesh. Every array is
 * written through its own memory (ptrw), never with a set() per element: each set() is one call across the
 * GDExtension interface, and the table is laid anew with every dig (P0690).
 */
::godot::Ref<::godot::ArrayMesh> water_mesh(const PlantMeshArrays& arrays) {
    if (arrays.positions.empty() || arrays.indices.empty()) {
        return ::godot::Ref<::godot::ArrayMesh>();
    }
    const std::size_t vertices = arrays.positions.size() / 3u;
    ::godot::PackedVector3Array positions;
    ::godot::PackedVector3Array normals;
    ::godot::PackedVector2Array uvs;
    ::godot::PackedColorArray colors;
    positions.resize(static_cast<int64_t>(vertices));
    normals.resize(static_cast<int64_t>(vertices));
    uvs.resize(static_cast<int64_t>(vertices));
    colors.resize(static_cast<int64_t>(vertices));
    ::godot::Vector3* position_at = positions.ptrw();
    ::godot::Vector3* normal_at = normals.ptrw();
    ::godot::Vector2* uv_at = uvs.ptrw();
    ::godot::Color* color_at = colors.ptrw();
    for (std::size_t i = 0u; i < vertices; ++i) {
        position_at[i] = ::godot::Vector3(arrays.positions[i * 3u], arrays.positions[i * 3u + 1u],
                                          arrays.positions[i * 3u + 2u]);
        normal_at[i] = ::godot::Vector3(arrays.normals[i * 3u], arrays.normals[i * 3u + 1u], arrays.normals[i * 3u + 2u]);
        uv_at[i] = ::godot::Vector2(arrays.uvs[i * 2u], arrays.uvs[i * 2u + 1u]);
        color_at[i] = ::godot::Color(arrays.colors[i * 4u], arrays.colors[i * 4u + 1u], arrays.colors[i * 4u + 2u],
                                     arrays.colors[i * 4u + 3u]);
    }
    ::godot::PackedInt32Array indices;
    indices.resize(static_cast<int64_t>(arrays.indices.size()));
    int32_t* index_at = indices.ptrw();
    for (std::size_t i = 0u; i < arrays.indices.size(); ++i) {
        index_at[i] = arrays.indices[i];
    }
    ::godot::Array surface;
    surface.resize(::godot::Mesh::ARRAY_MAX);
    surface[::godot::Mesh::ARRAY_VERTEX] = positions;
    surface[::godot::Mesh::ARRAY_NORMAL] = normals;
    surface[::godot::Mesh::ARRAY_TEX_UV] = uvs;
    surface[::godot::Mesh::ARRAY_COLOR] = colors;
    surface[::godot::Mesh::ARRAY_INDEX] = indices;
    ::godot::Ref<::godot::ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(::godot::Mesh::PRIMITIVE_TRIANGLES, surface);
    return mesh;
}

}  // namespace

void build_plant_water(::godot::Node3D* scene) {
    ::godot::Ref<::godot::ShaderMaterial> material;
    material.instantiate();
    material->set_shader(load_plant_shader(GODOT_WATER_SHADER_FILE));
    material->set_shader_parameter(GODOT_WATER_VELOCITY_FULL, GODOT_WATER_VELOCITY_FULL_M_S);
    material->set_shader_parameter(GODOT_WATER_SHALLOW,
                                   ::godot::Color(GODOT_WATER_SHALLOW_R, GODOT_WATER_SHALLOW_G, GODOT_WATER_SHALLOW_B));
    material->set_shader_parameter(GODOT_WATER_DEEP,
                                   ::godot::Color(GODOT_WATER_DEEP_R, GODOT_WATER_DEEP_G, GODOT_WATER_DEEP_B));

    ::godot::MeshInstance3D* water = memnew(::godot::MeshInstance3D);
    water->set_name(GODOT_PLANT_WATER);
    water->set_material_override(material);
    water->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    scene->add_child(water);
}

uint32_t fill_plant_water(::godot::Node3D* scene, const std::vector<VivariumStreamView>& streams,
                          const PlantMeshArrays& ground) {
    ::godot::MeshInstance3D* node = brook_part<::godot::MeshInstance3D>(scene, GODOT_PLANT_WATER);
    if (node == nullptr) {
        return 0u;
    }
    PlantMeshArrays arrays;
    const uint32_t wet = build_water_table(streams, ground, arrays);
    node->set_mesh(water_mesh(arrays));
    return wet;
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - THE LAKE, SEA AND GLACIER WATER OF THE CARE VIEW
 *
 * @file        godot_plant_water_kinds.cpp
 * @brief       The three nodes of lake, sea and glacier water in the care view's 3D scene and their fill
 * @description See godot_plant_water_kinds.hpp. Each kind is one MeshInstance3D whose mesh is laid anew by the fill:
 *              the surfaces come from godot_plant_water_kinds_geom.hpp, which carries no Godot type, and this file
 *              only hands the arrays to an ArrayMesh, every array written through its own memory (ptrw) and not with a
 *              set() per element, as the brook water does.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_water_kinds.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/godot_plant_water_kinds_geom.hpp>
#include <ase/adp/godot/godot_water_kinds_reader.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
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

/** The node of `scene` with that name, as a MeshInstance3D; nullptr when it has none. */
::godot::MeshInstance3D* kinds_node(::godot::Node3D* scene, const char* name) {
    return ::godot::Object::cast_to<::godot::MeshInstance3D>(scene->get_node_or_null(::godot::NodePath(name)));
}

/** The arrays as one surface of an ArrayMesh; no triangle is no mesh. */
::godot::Ref<::godot::ArrayMesh> kinds_mesh(const PlantMeshArrays& arrays) {
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

/** One node named `name` in a material of `shader_file`, casting no shadow, hung under `scene`; returns its material. */
::godot::Ref<::godot::ShaderMaterial> lay_kind_node(::godot::Node3D* scene, const char* name, const char* shader_file) {
    ::godot::Ref<::godot::ShaderMaterial> material;
    material.instantiate();
    material->set_shader(load_plant_shader(shader_file));
    ::godot::MeshInstance3D* node = memnew(::godot::MeshInstance3D);
    node->set_name(name);
    node->set_material_override(material);
    node->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    scene->add_child(node);
    return material;
}

/** The full scales the vertex colours of the three kinds are laid over. */
WaterKindsScale kinds_scale() {
    WaterKindsScale scale;
    scale.lake_depth_m = GODOT_LAKE_DEPTH_FULL_M;
    scale.sea_depth_m = GODOT_SEA_DEPTH_FULL_M;
    scale.ice_m = GODOT_GLACIER_ICE_FULL_M;
    scale.melt_mm_h = GODOT_GLACIER_MELT_FULL_MM_H;
    return scale;
}

}  // namespace

void build_plant_water_kinds(::godot::Node3D* scene) {
    const ::godot::Ref<::godot::ShaderMaterial> lake = lay_kind_node(scene, GODOT_PLANT_LAKE_WATER, GODOT_LAKE_WATER_SHADER_FILE);
    lake->set_shader_parameter(GODOT_KINDS_SHALLOW,
                               ::godot::Color(GODOT_LAKE_SHALLOW_R, GODOT_LAKE_SHALLOW_G, GODOT_LAKE_SHALLOW_B));
    lake->set_shader_parameter(GODOT_KINDS_DEEP, ::godot::Color(GODOT_LAKE_DEEP_R, GODOT_LAKE_DEEP_G, GODOT_LAKE_DEEP_B));
    const ::godot::Ref<::godot::ShaderMaterial> sea = lay_kind_node(scene, GODOT_PLANT_SEA_WATER, GODOT_SEA_WATER_SHADER_FILE);
    sea->set_shader_parameter(GODOT_KINDS_SHALLOW,
                              ::godot::Color(GODOT_SEA_SHALLOW_R, GODOT_SEA_SHALLOW_G, GODOT_SEA_SHALLOW_B));
    sea->set_shader_parameter(GODOT_KINDS_DEEP, ::godot::Color(GODOT_SEA_DEEP_R, GODOT_SEA_DEEP_G, GODOT_SEA_DEEP_B));
    const ::godot::Ref<::godot::ShaderMaterial> glacier =
        lay_kind_node(scene, GODOT_PLANT_GLACIER_WATER, GODOT_GLACIER_WATER_SHADER_FILE);
    glacier->set_shader_parameter(GODOT_KINDS_ICE,
                                  ::godot::Color(GODOT_GLACIER_ICE_R, GODOT_GLACIER_ICE_G, GODOT_GLACIER_ICE_B));
    glacier->set_shader_parameter(GODOT_KINDS_MELT,
                                  ::godot::Color(GODOT_GLACIER_MELT_R, GODOT_GLACIER_MELT_G, GODOT_GLACIER_MELT_B));
}

WaterKindsFilled fill_plant_water_kinds(::godot::Node3D* scene, const std::vector<VivariumLakeView>& lakes,
                                        const std::vector<VivariumSeaView>& seas,
                                        const std::vector<VivariumGlacierView>& glaciers,
                                        const PlantMeshArrays& ground) {
    WaterKindsFilled filled;
    const WaterKindsScale scale = kinds_scale();
    PlantMeshArrays lake_arrays;
    for (const VivariumLakeView& lake : lakes) {
        const bool laid = water_kinds_lake_surface(lake, scale, lake_arrays);
        filled.lakes += laid ? 1u : 0u;
        filled.lake_gaps += laid ? 0u : 1u;
    }
    PlantMeshArrays sea_arrays;
    for (const VivariumSeaView& sea : seas) {
        const bool laid = water_kinds_sea_plane(sea, GODOT_WATER_DATUM_NN0_M, scale, sea_arrays);
        filled.seas += laid ? 1u : 0u;
        filled.sea_gaps += laid ? 0u : 1u;
    }
    PlantMeshArrays glacier_arrays;
    for (const VivariumGlacierView& glacier : glaciers) {
        const uint32_t triangles = water_kinds_glacier_cap(glacier, ground, scale, glacier_arrays);
        filled.glacier_triangles += triangles;
        filled.glaciers += triangles > 0u ? 1u : 0u;
        filled.glacier_gaps += glacier_drawable(glacier) ? 0u : 1u;
    }
    ::godot::MeshInstance3D* lake_node = kinds_node(scene, GODOT_PLANT_LAKE_WATER);
    if (lake_node != nullptr) {
        lake_node->set_mesh(kinds_mesh(lake_arrays));
    }
    ::godot::MeshInstance3D* sea_node = kinds_node(scene, GODOT_PLANT_SEA_WATER);
    if (sea_node != nullptr) {
        sea_node->set_mesh(kinds_mesh(sea_arrays));
    }
    ::godot::MeshInstance3D* glacier_node = kinds_node(scene, GODOT_PLANT_GLACIER_WATER);
    if (glacier_node != nullptr) {
        glacier_node->set_mesh(kinds_mesh(glacier_arrays));
    }
    return filled;
}

}  // namespace ase::adp::godot

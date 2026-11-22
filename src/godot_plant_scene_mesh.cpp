/**
 * ASE GODOT ADAPTER - PLANT SCENE MESHES
 *
 * @file        godot_plant_scene_mesh.cpp
 * @brief       Hands the arrays of the scene to Godot: one surface, the instance buffers and the bark tube of a body
 * @description See godot_plant_scene.hpp. Every mesh is the arrays of godot_plant_geometry.hpp handed to Godot as
 *              they are, normals included; every instance buffer the records of an instance port handed to
 *              MultiMesh as they are. Register B0295 point 4: the bark of a plant is no longer one cylinder
 *              instance per internode but one continuous tube per axis, laid out by tubes_by_body
 *              (godot_plant_tubes.hpp). fill_tube hands that tube to Godot as one mesh whose vertices carry
 *              CUSTOM0 (bark share and tiling), CUSTOM1 (sway slot, sway share, ring radius, axis order) and, in the
 *              knot form, CUSTOM2 (the frame of the knot) and CUSTOM3 (the noise key of the axis) beside them, its
 *              culling box grown by the widest ring and its bark noise, and the material of its node the knot form,
 *              the bark noise amplitude, the sway data texture of its internodes and the shown orders of
 *              the budget: a ring thinner than a pixel or of an order above them closes onto its axis in the bark
 *              shader. Register B0317: fill_tube serves both meshes of the bark node, the fine tube on every
 *              corner of the knot ring and the coarse tube of the twigs on evenly spaced corners - the same
 *              vertices closed by other indices, and the material told which of the two it draws (coarse).
 *              Nothing is computed here: no ring, no radius and no place.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-13
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_plant_look_shader.hpp>
#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/adp/godot/leaf/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/type_info.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/**
 * The arrays of one surface as Godot's mesh arrays, normals included as they stand - the templates those of
 * their ports, the ground those of ase-render-data, the bark tube those of its rings; Godot derives none. Every
 * array is written through its own memory (ptrw): a set() per element is one call across the GDExtension
 * interface, and the ground's 66057 vertices and 393228 indices made that some 660 000 calls on the main thread
 * between two frames of a dig (P0690).
 */
::godot::Array surface_of(const PlantMeshArrays& arrays) {
    ::godot::PackedVector3Array positions;
    positions.resize(static_cast<int64_t>(arrays.positions.size() / 3u));
    ::godot::Vector3* position_at = positions.ptrw();
    for (std::size_t v = 0u; v < arrays.positions.size() / 3u; ++v) {
        position_at[v] = ::godot::Vector3(arrays.positions[v * 3u], arrays.positions[v * 3u + 1u],
                                          arrays.positions[v * 3u + 2u]);
    }
    ::godot::PackedVector2Array uvs;
    uvs.resize(static_cast<int64_t>(arrays.uvs.size() / 2u));
    ::godot::Vector2* uv_at = uvs.ptrw();
    for (std::size_t v = 0u; v < arrays.uvs.size() / 2u; ++v) {
        uv_at[v] = ::godot::Vector2(arrays.uvs[v * 2u], arrays.uvs[v * 2u + 1u]);
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
    surface[::godot::Mesh::ARRAY_TEX_UV] = uvs;
    surface[::godot::Mesh::ARRAY_INDEX] = indices;
    ::godot::PackedVector3Array normals;
    normals.resize(static_cast<int64_t>(arrays.normals.size() / 3u));
    ::godot::Vector3* normal_at = normals.ptrw();
    for (std::size_t v = 0u; v < arrays.normals.size() / 3u; ++v) {
        normal_at[v] = ::godot::Vector3(arrays.normals[v * 3u], arrays.normals[v * 3u + 1u], arrays.normals[v * 3u + 2u]);
    }
    surface[::godot::Mesh::ARRAY_NORMAL] = normals;
    // the vertex colours where the surface carries them: the cover of the ground (mark_ground)
    if (!arrays.colors.empty()) {
        ::godot::PackedColorArray colors;
        colors.resize(static_cast<int64_t>(arrays.colors.size() / 4u));
        ::godot::Color* color_at = colors.ptrw();
        for (std::size_t v = 0u; v < arrays.colors.size() / 4u; ++v) {
            color_at[v] = ::godot::Color(arrays.colors[v * 4u], arrays.colors[v * 4u + 1u], arrays.colors[v * 4u + 2u],
                                         arrays.colors[v * 4u + 3u]);
        }
        surface[::godot::Mesh::ARRAY_COLOR] = colors;
    }
    return surface;
}

/** `values` as one packed array, written through its own memory (ptrw). */
::godot::PackedFloat32Array floats_of(const std::vector<float>& values) {
    ::godot::PackedFloat32Array packed;
    packed.resize(static_cast<int64_t>(values.size()));
    float* value_at = packed.ptrw();
    for (std::size_t v = 0u; v < values.size(); ++v) {
        value_at[v] = values[v];
    }
    return packed;
}

/**
 * The sway data texture of `slots` sway slots (usecase:101335 animate the branch sway): GODOT_SWAY_TEXELS texels
 * of RGBA float per slot, in full rows of GODOT_SWAY_ROW_INSTANCES slots; the rest of the last row stays at
 * GODOT_SWAY_NONE and is never read. The shader finds a slot at ((slot % sway_row) * 2, slot / sway_row).
 */
::godot::Ref<::godot::ImageTexture> sway_texture(const std::vector<float>& sway, std::size_t slots) {
    const std::size_t per_row = GODOT_SWAY_ROW_INSTANCES;
    const std::size_t texture_rows = (slots + per_row - 1u) / per_row;
    const std::size_t row_values = per_row * GODOT_SWAY_VALUES;
    ::godot::PackedFloat32Array texels;
    texels.resize(static_cast<int64_t>(texture_rows * row_values));
    texels.fill(static_cast<float>(GODOT_SWAY_NONE));
    float* texel_at = texels.ptrw();
    for (std::size_t v = 0u; v < slots * GODOT_SWAY_VALUES; ++v) {
        texel_at[v] = sway[v];
    }
    const ::godot::Ref<::godot::Image> image = ::godot::Image::create_from_data(
        static_cast<int32_t>(per_row * GODOT_SWAY_TEXELS), static_cast<int32_t>(texture_rows), false,
        ::godot::Image::FORMAT_RGBAF, texels.to_byte_array());
    return ::godot::ImageTexture::create_from_image(image);
}

}  // namespace

::godot::Ref<::godot::ArrayMesh> mesh_of(const PlantMeshArrays& arrays) {
    if (arrays.positions.empty() || arrays.indices.empty()) {
        return ::godot::Ref<::godot::ArrayMesh>();
    }
    ::godot::Ref<::godot::ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(::godot::Mesh::PRIMITIVE_TRIANGLES, surface_of(arrays));
    return mesh;
}

void fill_instances(::godot::MultiMeshInstance3D* node, const ::godot::Ref<::godot::Mesh>& shape,
                    const std::vector<float>& rows, const std::vector<float>* custom,
                    const ::godot::Ref<::godot::Material>& material, const std::vector<float>* sway) {
    if (node == nullptr) {
        return;
    }
    const std::size_t count = rows.size() / GODOT_INSTANCE_VALUES;
    const std::size_t extra = custom == nullptr ? 0u : GODOT_INSTANCE_CUSTOM_VALUES;
    ::godot::Ref<::godot::MultiMesh> multimesh;
    multimesh.instantiate();
    multimesh->set_transform_format(::godot::MultiMesh::TRANSFORM_3D);
    multimesh->set_use_custom_data(custom != nullptr);
    multimesh->set_mesh(shape);
    multimesh->set_instance_count(static_cast<int32_t>(count));
    // one stride per instance: the twelve values of its record, then its custom data
    const std::size_t stride = GODOT_INSTANCE_VALUES + extra;
    // written through the buffer's own memory (ptrw), never a set() per value across the GDExtension interface
    ::godot::PackedFloat32Array buffer;
    buffer.resize(static_cast<int64_t>(count * stride));
    float* value_at = buffer.ptrw();
    for (std::size_t n = 0u; n < count; ++n) {
        for (std::size_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
            value_at[n * stride + v] = rows[n * GODOT_INSTANCE_VALUES + v];
        }
        for (std::size_t v = 0u; v < extra; ++v) {
            value_at[n * stride + GODOT_INSTANCE_VALUES + v] = (*custom)[n * extra + v];
        }
    }
    multimesh->set_buffer(buffer);
    node->set_multimesh(multimesh);
    if (sway == nullptr || count == 0u) {
        node->set_material_override(material);
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> shared = material;
    if (shared.is_null()) {
        node->set_material_override(material);
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> own = shared->duplicate();
    own->set_shader_parameter(GODOT_SWAY_DATA, sway_texture(*sway, count));
    own->set_shader_parameter(GODOT_SWAY_ROW, static_cast<int32_t>(GODOT_SWAY_ROW_INSTANCES));
    node->set_material_override(own);
}

void fill_tube(::godot::MeshInstance3D* node, const PlantBodyTube& tube, const std::vector<int32_t>& indices,
               const ::godot::Ref<::godot::Material>& material, uint32_t shown_orders, bool coarse) {
    if (node == nullptr) {
        return;
    }
    if (tube.arrays.positions.empty() || indices.empty()) {
        node->set_mesh(::godot::Ref<::godot::ArrayMesh>());
        return;
    }
    // the surface with the two custom arrays beside every vertex, four floats each (RGBA float), closed by the
    // indices of this tube: every corner of the knot ring on the fine tube, evenly spaced corners on the coarse one
    ::godot::Array surface = surface_of(tube.arrays);
    ::godot::PackedInt32Array closed;
    closed.resize(static_cast<int64_t>(indices.size()));
    int32_t* closed_at = closed.ptrw();
    for (std::size_t i = 0u; i < indices.size(); ++i) {
        closed_at[i] = indices[i];
    }
    surface[::godot::Mesh::ARRAY_INDEX] = closed;
    surface[::godot::Mesh::ARRAY_CUSTOM0] = floats_of(tube.custom0);
    surface[::godot::Mesh::ARRAY_CUSTOM1] = floats_of(tube.custom1);
    int64_t format =
        (static_cast<int64_t>(::godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT) << ::godot::Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT) |
        (static_cast<int64_t>(::godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT) << ::godot::Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT);
    // the knot form carries the frame of its knot beside every vertex (PLAN_ASE_VIVARIUM_TREEGEN T.6): the bark
    // shader turns the template corner by it, so it goes to Godot as a third custom array, four floats (RGBA float)
    if (!tube.custom2.empty()) {
        surface[::godot::Mesh::ARRAY_CUSTOM2] = floats_of(tube.custom2);
        // and the tangent of its template corner, which the shader turns by the same frame as the normal, so the
        // normal maps of the bark run around the ring and up the axis instead of along a tangent Godot derives
        surface[::godot::Mesh::ARRAY_TANGENT] = floats_of(tube.tangents);
        format |= static_cast<int64_t>(::godot::Mesh::ARRAY_CUSTOM_RGBA_FLOAT) << ::godot::Mesh::ARRAY_FORMAT_CUSTOM2_SHIFT;
        // and the noise key of its axis, which the shader spreads to o_axis, the place of the axis in the bark noise
        // field ("Rindenrauschen"): a fourth custom array of one float (R float)
        surface[::godot::Mesh::ARRAY_CUSTOM3] = floats_of(tube.custom3);
        format |= static_cast<int64_t>(::godot::Mesh::ARRAY_CUSTOM_R_FLOAT) << ::godot::Mesh::ARRAY_FORMAT_CUSTOM3_SHIFT;
    }
    ::godot::Ref<::godot::ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(::godot::Mesh::PRIMITIVE_TRIANGLES, surface, ::godot::TypedArray<::godot::Array>(),
                                  ::godot::Dictionary(), ::godot::BitField<::godot::Mesh::ArrayFormat>(format));
    // the bark shader sets every corner out from its knot by the ring radius on custom1 and the bark noise moves it
    // by at most GODOT_BARK_WOBBLE_MOST of that radius more, so the box Godot culls by is the box of the places grown
    // by the widest ring and its noise; the places alone would cut the tube at the edge of a view
    float widest = 0.0f;
    for (std::size_t at = 2u; at < tube.custom1.size(); at += GODOT_BARK_CUSTOM_VALUES) {
        widest = ase::math::max(widest, tube.custom1[at]);
    }
    widest *= 1.0f + GODOT_BARK_WOBBLE_MOST;
    const ::godot::AABB placed = mesh->get_aabb();
    const ::godot::Vector3 grown(widest, widest, widest);
    mesh->set_custom_aabb(::godot::AABB(placed.position - grown, placed.size + grown * 2.0f));
    node->set_mesh(mesh);
    // the material of this node carries the sway texture of its internodes, so one material cannot serve two plants
    const ::godot::Ref<::godot::ShaderMaterial> shared = material;
    if (shared.is_null()) {
        node->set_material_override(material);
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> own = shared->duplicate();
    own->set_shader_parameter(GODOT_SWAY_DATA, sway_texture(tube.sway, tube.sway.size() / GODOT_SWAY_VALUES));
    own->set_shader_parameter(GODOT_SWAY_ROW, static_cast<int32_t>(GODOT_SWAY_ROW_INSTANCES));
    own->set_shader_parameter(GODOT_BARK_SHOWN_ORDERS, static_cast<float>(shown_orders));
    own->set_shader_parameter(GODOT_BARK_COARSE, coarse);
    own->set_shader_parameter(GODOT_BARK_KNOT_FORM, !tube.custom2.empty());
    own->set_shader_parameter(GODOT_BARK_WOBBLE_AMP, GODOT_BARK_WOBBLE_MOST);
    node->set_material_override(own);
}

}  // namespace ase::adp::godot

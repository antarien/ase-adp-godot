/**
 * ASE GODOT ADAPTER - THE WATER OF THE CARE VIEW
 *
 * @file        godot_plant_brook.cpp
 * @brief       The node of the water in the care view's 3D scene and its fill
 * @description See godot_plant_brook.hpp. The water plane is drawn after the opaque ground, transparent; the
 *              ground above the ase-hydro level of a bed hides it by the depth test, which draws the shore
 *              (build_water_table, B0281). Its vertex colour carries
 *              velocity, depth and course into the shader; its second UV the hue of the nearest current cell
 *              that answered one (water.quality.v1 curr_hue_deg, CAUSA_ASE_SIGNATURE), which the shader turns
 *              its own colour to. The colour channels are all taken, so the hue rides in UV2.
 *              Beside the cells stand the two far bands of H.13 (usecase:101363 band entities by distance): the chunk
 *              planes of godot_plant_water_band.hpp as LOD 3 and, beyond, as the basin tint of LOD 4, each a
 *              visibility range the camera's pixel rule gives over the chunk edge the current port carries.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-01
 * @modified    2026-11-24
 * @version     00.00.22.00022 [refine]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_brook.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/godot_plant_water_band.hpp>
#include <ase/adp/godot/godot_plant_water_kinds.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/viewport.hpp>
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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

// The two nodes of the far bands (H.13). Their names stand here and not in godot_plant_brook.hpp or scene/types.hpp:
// only this unit lays and fills them, and a line in a header every water unit includes would translate all of them
// anew (WRFL_ASE_TIER_WIPE, the header ratchet).
constexpr const char* kWaterChunkNode = "PlantWaterChunk";   // LOD 3: one plane per chunk at its mean level
constexpr const char* kWaterBasinNode = "PlantWaterBasin";   // LOD 4: the same planes as the tint of the basin

/** The node of `parent` with that name, cast to its class; nullptr when it has none. */
template <typename T>
T* brook_part(::godot::Node* parent, const char* name) {
    return ::godot::Object::cast_to<T>(parent->get_node_or_null(::godot::NodePath(name)));
}

/**
 * The arrays of build_water_table as one surface of an ArrayMesh; a dry ground is no mesh. Every array is
 * written through its own memory (ptrw), never with a set() per element: each set() is one call across the
 * GDExtension interface, and the table is laid anew with every dig (P0690). `tints` holds two floats per vertex
 * for ARRAY_TEX_UV2 (hue_of_water); a list of another length lays no second UV, and the shader keeps its own
 * colour there.
 */
::godot::Ref<::godot::ArrayMesh> water_mesh(const PlantMeshArrays& arrays, const std::vector<float>& tints) {
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
    if (tints.size() == vertices * 2u) {
        ::godot::PackedVector2Array uv2s;
        uv2s.resize(static_cast<int64_t>(vertices));
        ::godot::Vector2* uv2_at = uv2s.ptrw();
        for (std::size_t i = 0u; i < vertices; ++i) {
            uv2_at[i] = ::godot::Vector2(tints[i * 2u], tints[i * 2u + 1u]);
        }
        surface[::godot::Mesh::ARRAY_TEX_UV2] = uv2s;
    }
    surface[::godot::Mesh::ARRAY_INDEX] = indices;
    ::godot::Ref<::godot::ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(::godot::Mesh::PRIMITIVE_TRIANGLES, surface);
    return mesh;
}

/**
 * The second UV of every vertex of `arrays`: x the hue of the nearest current cell whose quality record answered
 * one, over GODOT_WATER_HUE_TURN_DEG, y GODOT_WATER_HUE_ANSWERED; a vertex no cell answered for gets 0 and
 * GODOT_WATER_HUE_UNANSWERED. Nearest and never blended, as ase-gis answers the layer the hue comes from; the
 * vertices and the cell centres stand in the same world metres east and north. Nothing of the hue is computed
 * here: ase-signature filed it, the record carries it.
 */
std::vector<float> hue_of_water(const PlantMeshArrays& arrays, const std::vector<VivariumCurrentView>& currents) {
    const std::size_t vertices = arrays.positions.size() / 3u;
    std::vector<float> tints(vertices * 2u, 0.0f);
    std::vector<const VivariumCurrentView*> answered;
    answered.reserve(currents.size());
    for (const VivariumCurrentView& cell : currents) {
        if (std::isfinite(cell.hue_deg) && std::isfinite(cell.x) && std::isfinite(cell.z)) {
            answered.push_back(&cell);
        }
    }
    for (std::size_t v = 0u; v < vertices; ++v) {
        const double x = arrays.positions[v * 3u];
        const double z = arrays.positions[v * 3u + 2u];
        const VivariumCurrentView* nearest = nullptr;
        double nearest_d2 = 0.0;
        for (const VivariumCurrentView* cell : answered) {
            const double dx = cell->x - x;
            const double dz = cell->z - z;
            const double d2 = dx * dx + dz * dz;
            if (nearest == nullptr || d2 < nearest_d2) {
                nearest = cell;
                nearest_d2 = d2;
            }
        }
        if (nearest == nullptr) {
            tints[v * 2u + 1u] = GODOT_WATER_HUE_UNANSWERED;
            continue;
        }
        tints[v * 2u] = static_cast<float>(nearest->hue_deg) / GODOT_WATER_HUE_TURN_DEG;
        tints[v * 2u + 1u] = GODOT_WATER_HUE_ANSWERED;
    }
    return tints;
}

/**
 * The chunk edge the current port carried (curr_chunk_m, TER_CONST_CHUNK_SIZE through WaterHostCurrExptSystem):
 * the first current cell that laid one above zero. Zero when no cell did; the far bands then stand empty and the
 * cells draw at every distance.
 */
float chunk_edge_of(const std::vector<VivariumCurrentView>& currents) {
    for (const VivariumCurrentView& cell : currents) {
        if (std::isfinite(cell.chunk_m) && cell.chunk_m > 0.0) {
            return static_cast<float>(cell.chunk_m);
        }
    }
    return 0.0f;
}

/**
 * The reaches of the bands as the camera of `scene` (GODOT_PLANT_CAMERA) sees water of cell edge `cell_m` and chunk
 * edge `chunk_m`: the height of its viewport in pixels and the tangent of half its vertical field of view. All zero
 * without a camera, a viewport or a size (water_band_reach).
 */
WaterBandReach band_reach_of(::godot::Node3D* scene, float cell_m, float chunk_m) {
    const ::godot::Camera3D* eye = brook_part<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    if (eye == nullptr) {
        return WaterBandReach{};
    }
    const ::godot::Viewport* view = eye->get_viewport();
    if (view == nullptr) {
        return WaterBandReach{};
    }
    const float view_height = static_cast<float>(view->get_visible_rect().size.y);
    const float half_fov = static_cast<float>(eye->get_fov()) * ase::math::PI / 360.0f;
    return water_band_reach(cell_m, chunk_m, view_height, ase::math::tan(half_fov));
}

/** The chunk planes of `arrays`: one quad per chunk of edge `chunk_m` (water_band_tiles, water_band_plane). */
PlantMeshArrays chunk_planes_of(const PlantMeshArrays& arrays, float chunk_m) {
    std::vector<WaterBandTile> tiles;
    water_band_tiles(arrays, chunk_m, tiles);
    PlantMeshArrays planes;
    for (const WaterBandTile& tile : tiles) {
        water_band_plane(tile, planes);
    }
    return planes;
}

/** Sets the band of `node` to begin at `begin_m` and end at `end_m`; an end of zero draws to the far plane. */
void set_band(::godot::GeometryInstance3D* node, float begin_m, float end_m) {
    if (node == nullptr) {
        return;
    }
    node->set_visibility_range_begin(begin_m);
    node->set_visibility_range_end(end_m);
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

    // LOD 3 and LOD 4 (H.13): the chunk planes in the material of the brook, its vertex colour the mean colour of the
    // chunk; fill_plant_water lays their mesh and the reaches the camera gives
    for (const char* band : {kWaterChunkNode, kWaterBasinNode}) {
        ::godot::MeshInstance3D* far = memnew(::godot::MeshInstance3D);
        far->set_name(band);
        far->set_material_override(material);
        far->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
        scene->add_child(far);
    }

    // the groundwater in the pits, still water of its own shader (D2508)
    ::godot::Ref<::godot::ShaderMaterial> pit_material;
    pit_material.instantiate();
    pit_material->set_shader(load_plant_shader(GODOT_PIT_WATER_SHADER_FILE));
    pit_material->set_shader_parameter(
        GODOT_WATER_SHALLOW, ::godot::Color(GODOT_PIT_WATER_SHALLOW_R, GODOT_PIT_WATER_SHALLOW_G, GODOT_PIT_WATER_SHALLOW_B));
    pit_material->set_shader_parameter(
        GODOT_WATER_DEEP, ::godot::Color(GODOT_PIT_WATER_DEEP_R, GODOT_PIT_WATER_DEEP_G, GODOT_PIT_WATER_DEEP_B));
    ::godot::MeshInstance3D* pit_water = memnew(::godot::MeshInstance3D);
    pit_water->set_name(GODOT_PLANT_PIT_WATER);
    pit_water->set_material_override(pit_material);
    pit_water->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    scene->add_child(pit_water);
    build_plant_water_kinds(scene);
}

uint32_t fill_plant_water(::godot::Node3D* scene, const std::vector<VivariumStreamView>& streams,
                          const std::vector<VivariumPitView>& pits, const std::vector<VivariumCurrentView>& currents,
                          const PlantMeshArrays& ground) {
    ::godot::MeshInstance3D* node = brook_part<::godot::MeshInstance3D>(scene, GODOT_PLANT_WATER);
    if (node == nullptr) {
        return 0u;
    }
    PlantMeshArrays arrays;
    PlantMeshArrays pit_arrays;
    const uint32_t wet = build_water_table(streams, pits, ground, arrays, pit_arrays);
    node->set_mesh(water_mesh(arrays, hue_of_water(arrays, currents)));

    // the bands by distance (H.13, usecase:101363): the cells draw LOD 0 to 2 up to where a cell shrinks to
    // GODOT_WATER_BAND_REFLECT_PX, the chunk planes LOD 3 from there to where a chunk shrinks to
    // GODOT_WATER_BAND_CHUNK_PX, the basin tint LOD 4 beyond. The bands only choose what is drawn; the water they
    // draw is the same table (I-8). Without a reach (no camera, no chunk edge on the port) the cells draw at every
    // distance and the far nodes stay empty.
    const float chunk_m = chunk_edge_of(currents);
    const WaterBandReach reach = band_reach_of(scene, water_cell_edge(arrays), chunk_m);
    const bool banded = reach.reflect_end_m > 0.0f && reach.chunk_end_m > 0.0f;
    set_band(node, 0.0f, banded ? reach.reflect_end_m : 0.0f);
    ::godot::MeshInstance3D* chunk_node = brook_part<::godot::MeshInstance3D>(scene, kWaterChunkNode);
    ::godot::MeshInstance3D* basin_node = brook_part<::godot::MeshInstance3D>(scene, kWaterBasinNode);
    const ::godot::Ref<::godot::ArrayMesh> planes =
        banded ? water_mesh(chunk_planes_of(arrays, chunk_m), std::vector<float>()) : ::godot::Ref<::godot::ArrayMesh>();
    if (chunk_node != nullptr) {
        chunk_node->set_mesh(planes);
        set_band(chunk_node, reach.reflect_end_m, reach.chunk_end_m);
    }
    if (basin_node != nullptr) {
        basin_node->set_mesh(planes);
        set_band(basin_node, reach.chunk_end_m, 0.0f);
    }

    ::godot::MeshInstance3D* pit_node = brook_part<::godot::MeshInstance3D>(scene, GODOT_PLANT_PIT_WATER);
    if (pit_node != nullptr) {
        // the still water of a pit is groundwater, no current cell answers for it: its shader keeps its colour
        pit_node->set_mesh(water_mesh(pit_arrays, std::vector<float>()));
    }
    return wet;
}

}  // namespace ase::adp::godot

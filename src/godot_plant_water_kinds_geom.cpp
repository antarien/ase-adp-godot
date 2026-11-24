/**
 * ASE GODOT ADAPTER - THE GEOMETRY OF THE LAKE, SEA AND GLACIER SURFACES
 *
 * @file        godot_plant_water_kinds_geom.cpp
 * @brief       The surface of a lake, of a sea chunk and of the ice over a chunk as mesh arrays, without a Godot type
 * @description See godot_plant_water_kinds_geom.hpp.
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

#include <ase/adp/godot/godot_plant_water_kinds_geom.hpp>
#include <ase/adp/godot/godot_water_kinds_reader.hpp>
#include <ase/math/math.hpp>

#include <cstddef>
#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr std::size_t kPositionStride = 3u;  // floats per vertex in positions and normals
constexpr float kMmPerM = 1000.0f;           // millimetres in a metre: the ice comes as mm of water column

/** The share `value` makes of `full`, within 0 to 1; 0 where the scale has no extent. */
float share_of(double value, float full) {
    if (!(full > 0.0f)) {
        return 0.0f;
    }
    return ase::math::clamp(static_cast<float>(value) / full, 0.0f, 1.0f);
}

/** One vertex looking up: place, up normal, the world metres east and north as texture coordinate, colour. */
void append_vertex(PlantMeshArrays& out, float x, float y, float z, float red, float green) {
    out.positions.push_back(x);
    out.positions.push_back(y);
    out.positions.push_back(z);
    out.normals.push_back(0.0f);
    out.normals.push_back(1.0f);
    out.normals.push_back(0.0f);
    out.uvs.push_back(x);
    out.uvs.push_back(z);
    out.colors.push_back(red);
    out.colors.push_back(green);
    out.colors.push_back(0.0f);
    out.colors.push_back(1.0f);
}

/** The quad (west, south) to (east, north) at height `y`, the triangles in the order of a ground cell, front face up. */
void append_quad(PlantMeshArrays& out, float west, float south, float east, float north, float y, float red,
                 float green) {
    const auto first = static_cast<int32_t>(out.positions.size() / kPositionStride);
    append_vertex(out, west, y, south, red, green);
    append_vertex(out, east, y, south, red, green);
    append_vertex(out, east, y, north, red, green);
    append_vertex(out, west, y, north, red, green);
    const int32_t order[6] = {0, 1, 2, 0, 2, 3};
    for (const int32_t corner : order) {
        out.indices.push_back(first + corner);
    }
}

/** Whether the centre of the ground triangle at index `first` lies in the chunk [west, west + edge) x [south, south + edge). */
bool triangle_in_chunk(const PlantMeshArrays& ground, std::size_t first, float west, float south, float edge) {
    float cx = 0.0f;
    float cz = 0.0f;
    for (std::size_t k = 0u; k < 3u; ++k) {
        const auto vertex = static_cast<std::size_t>(ground.indices[first + k]);
        cx += ground.positions[vertex * kPositionStride];
        cz += ground.positions[vertex * kPositionStride + 2u];
    }
    cx /= 3.0f;
    cz /= 3.0f;
    return cx >= west && cx < west + edge && cz >= south && cz < south + edge;
}

/** The vertex `vertex` of `ground` appended to `out`, its height lifted by `lift_m`. */
void append_lifted(PlantMeshArrays& out, const PlantMeshArrays& ground, std::size_t vertex, float lift_m, float red,
                   float green) {
    const std::size_t at = vertex * kPositionStride;
    out.positions.push_back(ground.positions[at]);
    out.positions.push_back(ground.positions[at + 1u] + lift_m);
    out.positions.push_back(ground.positions[at + 2u]);
    for (std::size_t k = 0u; k < kPositionStride; ++k) {
        out.normals.push_back(at + k < ground.normals.size() ? ground.normals[at + k] : (k == 1u ? 1.0f : 0.0f));
    }
    out.uvs.push_back(ground.positions[at]);
    out.uvs.push_back(ground.positions[at + 2u]);
    out.colors.push_back(red);
    out.colors.push_back(green);
    out.colors.push_back(0.0f);
    out.colors.push_back(1.0f);
}

}  // namespace

bool water_kinds_lake_surface(const VivariumLakeView& lake, const WaterKindsScale& scale, PlantMeshArrays& out) {
    if (!lake_drawable(lake) || !(scale.lake_depth_m > 0.0f)) {
        return false;
    }
    // the box is the engine's: x east, y north, z the height; the surface is its top face
    const auto surface = static_cast<float>(lake.cz + lake.hz);
    append_quad(out, static_cast<float>(lake.cx - lake.hx), static_cast<float>(lake.cy - lake.hy),
                static_cast<float>(lake.cx + lake.hx), static_cast<float>(lake.cy + lake.hy), surface,
                share_of(lake.depth_m, scale.lake_depth_m), 0.0f);
    return true;
}

bool water_kinds_sea_plane(const VivariumSeaView& sea, float datum_m, const WaterKindsScale& scale,
                           PlantMeshArrays& out) {
    if (!sea_drawable(sea)) {
        return false;
    }
    const auto edge = static_cast<float>(sea.chunk_m);
    const float west = static_cast<float>(sea.chunk_x) * edge;
    const float south = static_cast<float>(sea.chunk_z) * edge;
    // the depth under NN0 of the lowest ground; ground above NN0 reads as no depth at all
    const float below = datum_m - static_cast<float>(sea.low_m);
    append_quad(out, west, south, west + edge, south + edge, datum_m,
                share_of(below > 0.0f ? below : 0.0f, scale.sea_depth_m), 0.0f);
    return true;
}

uint32_t water_kinds_glacier_cap(const VivariumGlacierView& glacier, const PlantMeshArrays& ground,
                                 const WaterKindsScale& scale, PlantMeshArrays& out) {
    if (!glacier_drawable(glacier)) {
        return 0u;
    }
    const auto edge = static_cast<float>(glacier.chunk_m);
    const float west = static_cast<float>(glacier.chunk_x) * edge;
    const float south = static_cast<float>(glacier.chunk_z) * edge;
    const float thickness_m = static_cast<float>(glacier.ice_mm) / kMmPerM;
    const float red = share_of(thickness_m, scale.ice_m);
    const float green = share_of(glacier.melt_mm_h, scale.melt_mm_h);
    uint32_t laid = 0u;
    for (std::size_t first = 0u; first + 3u <= ground.indices.size(); first += 3u) {
        if (!triangle_in_chunk(ground, first, west, south, edge)) {
            continue;
        }
        const auto base = static_cast<int32_t>(out.positions.size() / kPositionStride);
        for (std::size_t k = 0u; k < 3u; ++k) {
            append_lifted(out, ground, static_cast<std::size_t>(ground.indices[first + k]), thickness_m, red, green);
            out.indices.push_back(base + static_cast<int32_t>(k));
        }
        laid += 1u;
    }
    return laid;
}

}  // namespace ase::adp::godot

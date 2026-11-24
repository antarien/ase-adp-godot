/**
 * ASE GODOT ADAPTER - THE DETAIL BANDS OF THE WATER IMPLEMENTATION
 *
 * @file        godot_plant_water_band.cpp
 * @brief       From which distance the water of a chunk is drawn in which band, its tiles per chunk and the chunk plane
 * @description See godot_plant_water_band.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101363 - band entities by distance
 * @actor       Detail Band Regulator (DETABANREG)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_water_band.hpp>
#include <ase/math/math.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr std::size_t kIndicesPerCell = 6u;   // (sw, se, ne) and (sw, ne, nw), the order of build_ground
constexpr std::size_t kPositionStride = 3u;
constexpr std::size_t kNormalStride = 3u;
constexpr std::size_t kUvStride = 2u;
constexpr std::size_t kColorStride = 4u;

/** The distance at which a length `size_m` spans `pixels` on the screen, metres (the impostor's pixel rule). */
float reach_of(float size_m, float pixels, float view_height, float tan_half_fov) {
    return size_m * view_height / (2.0f * pixels * tan_half_fov);
}

/** Whether the vertex `index` lies within the positions of `arrays`. */
bool vertex_in(const PlantMeshArrays& arrays, int32_t index) {
    return index >= 0 && static_cast<std::size_t>(index) * kPositionStride + 2u < arrays.positions.size();
}

/** The four corners of the cell starting at `first`: sw, se, ne, nw. false when an index lies past the vertices. */
bool corners_of(const PlantMeshArrays& arrays, std::size_t first, int32_t (&corner)[4]) {
    corner[0] = arrays.indices[first];
    corner[1] = arrays.indices[first + 1u];
    corner[2] = arrays.indices[first + 2u];
    corner[3] = arrays.indices[first + 5u];
    return vertex_in(arrays, corner[0]) && vertex_in(arrays, corner[1]) && vertex_in(arrays, corner[2]) &&
           vertex_in(arrays, corner[3]);
}

float pos(const PlantMeshArrays& arrays, int32_t vertex, std::size_t axis) {
    return arrays.positions[static_cast<std::size_t>(vertex) * kPositionStride + axis];
}

/** The plane distance between two vertices, metres. */
float plane_span(const PlantMeshArrays& arrays, int32_t a, int32_t b) {
    const float dx = pos(arrays, b, 0u) - pos(arrays, a, 0u);
    const float dz = pos(arrays, b, 2u) - pos(arrays, a, 2u);
    return ase::math::sqrt(dx * dx + dz * dz);
}

/** Appends vertex `vertex` of `from` to `to`, every attribute `from` carries for it, and returns its new index. */
int32_t take_vertex(const PlantMeshArrays& from, int32_t vertex, PlantMeshArrays& to) {
    const std::size_t v = static_cast<std::size_t>(vertex);
    const int32_t index = static_cast<int32_t>(to.positions.size() / kPositionStride);
    for (std::size_t k = 0u; k < kPositionStride; ++k) {
        to.positions.push_back(from.positions[v * kPositionStride + k]);
    }
    if (v * kNormalStride + kNormalStride <= from.normals.size()) {
        for (std::size_t k = 0u; k < kNormalStride; ++k) {
            to.normals.push_back(from.normals[v * kNormalStride + k]);
        }
    }
    if (v * kUvStride + kUvStride <= from.uvs.size()) {
        for (std::size_t k = 0u; k < kUvStride; ++k) {
            to.uvs.push_back(from.uvs[v * kUvStride + k]);
        }
    }
    if (v * kColorStride + kColorStride <= from.colors.size()) {
        for (std::size_t k = 0u; k < kColorStride; ++k) {
            to.colors.push_back(from.colors[v * kColorStride + k]);
        }
    }
    return index;
}

}  // namespace

WaterBandReach water_band_reach(float cell_m, float chunk_m, float view_height, float tan_half_fov) {
    WaterBandReach reach{};
    if (!(cell_m > 0.0f) || !(chunk_m > 0.0f) || !(view_height > 0.0f) || !(tan_half_fov > 0.0f)) {
        return reach;
    }
    reach.full_end_m = reach_of(cell_m, GODOT_WATER_BAND_FULL_PX, view_height, tan_half_fov);
    reach.flow_end_m = reach_of(cell_m, GODOT_WATER_BAND_FLOW_PX, view_height, tan_half_fov);
    reach.reflect_end_m = reach_of(cell_m, GODOT_WATER_BAND_REFLECT_PX, view_height, tan_half_fov);
    // A chunk is never smaller than its cell, so LOD 3 ends no nearer than LOD 2: a chunk edge below the cell edge
    // (a port that carried less than one cell) ends LOD 3 where LOD 2 ends, and LOD 3 stands empty.
    const float chunk_end = reach_of(chunk_m, GODOT_WATER_BAND_CHUNK_PX, view_height, tan_half_fov);
    reach.chunk_end_m = chunk_end > reach.reflect_end_m ? chunk_end : reach.reflect_end_m;
    return reach;
}

float water_cell_edge(const PlantMeshArrays& arrays) {
    float sum = 0.0f;
    uint32_t cells = 0u;
    for (std::size_t first = 0u; first + kIndicesPerCell <= arrays.indices.size(); first += kIndicesPerCell) {
        int32_t corner[4] = {0, 0, 0, 0};
        if (!corners_of(arrays, first, corner)) {
            continue;
        }
        sum += plane_span(arrays, corner[0], corner[1]);
        cells += 1u;
    }
    return cells == 0u ? 0.0f : sum / static_cast<float>(cells);
}

uint32_t water_band_tiles(const PlantMeshArrays& arrays, float chunk_m, std::vector<WaterBandTile>& out) {
    if (!(chunk_m > 0.0f)) {
        return 0u;
    }
    const std::size_t first_tile = out.size();
    const std::size_t vertices = arrays.positions.size() / kPositionStride;
    std::vector<std::vector<int32_t>> remap;   // per tile laid by this call: source vertex to tile vertex, -1 untaken
    std::vector<float> weight;                 // per tile: the plane area its cells summed so far, m2

    for (std::size_t first = 0u; first + kIndicesPerCell <= arrays.indices.size(); first += kIndicesPerCell) {
        int32_t corner[4] = {0, 0, 0, 0};
        if (!corners_of(arrays, first, corner)) {
            continue;
        }
        float cx = 0.0f;
        float cz = 0.0f;
        float cy = 0.0f;
        for (const int32_t c : corner) {
            cx += pos(arrays, c, 0u);
            cy += pos(arrays, c, 1u);
            cz += pos(arrays, c, 2u);
        }
        cx *= 0.25f;
        cy *= 0.25f;
        cz *= 0.25f;
        const int32_t gx = static_cast<int32_t>(ase::math::floor(cx / chunk_m));
        const int32_t gz = static_cast<int32_t>(ase::math::floor(cz / chunk_m));

        std::size_t t = first_tile;
        while (t < out.size() && (out[t].chunk_x != gx || out[t].chunk_z != gz)) {
            ++t;
        }
        if (t == out.size()) {
            WaterBandTile tile{};
            tile.chunk_x = gx;
            tile.chunk_z = gz;
            tile.min_x = pos(arrays, corner[0], 0u);
            tile.max_x = tile.min_x;
            tile.min_z = pos(arrays, corner[0], 2u);
            tile.max_z = tile.min_z;
            out.push_back(tile);
            remap.emplace_back(vertices, -1);
            weight.push_back(0.0f);
        }
        WaterBandTile& tile = out[t];
        std::vector<int32_t>& map = remap[t - first_tile];

        const float area = plane_span(arrays, corner[0], corner[1]) * plane_span(arrays, corner[0], corner[3]);
        const float before = weight[t - first_tile];
        const float after = before + area;
        if (after > 0.0f) {
            tile.level_m = (tile.level_m * before + cy * area) / after;
            for (std::size_t k = 0u; k < kColorStride; ++k) {
                float cell_color = 0.0f;
                uint32_t carried = 0u;
                for (const int32_t c : corner) {
                    const std::size_t at = static_cast<std::size_t>(c) * kColorStride + k;
                    if (at < arrays.colors.size()) {
                        cell_color += arrays.colors[at];
                        carried += 1u;
                    }
                }
                if (carried > 0u) {
                    tile.color[k] = (tile.color[k] * before + cell_color / static_cast<float>(carried) * area) / after;
                }
            }
        }
        weight[t - first_tile] = after;

        for (const int32_t c : corner) {
            const float x = pos(arrays, c, 0u);
            const float z = pos(arrays, c, 2u);
            tile.min_x = x < tile.min_x ? x : tile.min_x;
            tile.max_x = x > tile.max_x ? x : tile.max_x;
            tile.min_z = z < tile.min_z ? z : tile.min_z;
            tile.max_z = z > tile.max_z ? z : tile.max_z;
        }
        for (std::size_t k = 0u; k < kIndicesPerCell; ++k) {
            const int32_t source = arrays.indices[first + k];
            int32_t& taken = map[static_cast<std::size_t>(source)];
            if (taken < 0) {
                taken = take_vertex(arrays, source, tile.arrays);
            }
            tile.arrays.indices.push_back(taken);
        }
    }
    return static_cast<uint32_t>(out.size() - first_tile);
}

uint32_t water_band_plane(const WaterBandTile& tile, PlantMeshArrays& out) {
    if (tile.arrays.indices.size() < kIndicesPerCell) {
        return 0u;
    }
    const int32_t base = static_cast<int32_t>(out.positions.size() / kPositionStride);
    const float x[4] = {tile.min_x, tile.max_x, tile.max_x, tile.min_x};   // sw, se, ne, nw
    const float z[4] = {tile.min_z, tile.min_z, tile.max_z, tile.max_z};
    for (std::size_t c = 0u; c < 4u; ++c) {
        out.positions.push_back(x[c]);
        out.positions.push_back(tile.level_m);
        out.positions.push_back(z[c]);
        out.normals.push_back(0.0f);
        out.normals.push_back(1.0f);
        out.normals.push_back(0.0f);
        out.uvs.push_back(x[c]);
        out.uvs.push_back(z[c]);
        for (std::size_t k = 0u; k < kColorStride; ++k) {
            out.colors.push_back(tile.color[k]);
        }
    }
    const int32_t order[kIndicesPerCell] = {0, 1, 2, 0, 2, 3};
    for (const int32_t o : order) {
        out.indices.push_back(base + o);
    }
    return 1u;
}

}  // namespace ase::adp::godot

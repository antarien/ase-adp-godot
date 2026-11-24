/**
 * ASE ADAPTER TESTS - THE DETAIL BANDS OF THE WATER WITHOUT GODOT
 *
 * @file        test_godot_plant_water_band.cpp
 * @design      DSGN_018
 * @brief       Pins the reaches of LOD 0 to 4 of the water by the pixel rule, the cell edge, the tiles and the plane
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO H.13 "Detailstufen der Ansicht": LOD 0 while a cell spans more than
 *              32 pixels, LOD 1 down to 8, LOD 2 down to 2, LOD 3 while the chunk spans more than 8 pixels, LOD 4
 *              beyond. A length L spans P pixels at d = L H / (2 P tan(fov / 2)). The tests lay a water table of
 *              cells on two chunks and pin the reaches, the measured edge, the split into one tile per chunk with
 *              every cell kept, and the plane of a tile at its mean level.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
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

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_water_band.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr float kViewHeight = 2400.0f;   // render height of the test view in pixels
constexpr float kTanHalfFov = 0.75f;     // tangent of half its vertical field of view
constexpr float kCellM = 1.0f;           // edge of a cell of the test table, metres
constexpr float kChunkM = 4.0f;          // edge of a chunk of the test table, metres

/**
 * Lays a row of `cells` cells of edge kCellM along east from x = 0, z = 0 to kCellM, at the water level `level` for
 * the first half and `level + 1` for the second, six indices each, colours (0.5, 0.25, 0.5, 1).
 */
PlantMeshArrays table_row(uint32_t cells, float level) {
    PlantMeshArrays out;
    for (uint32_t i = 0u; i < cells; ++i) {
        const float y = i < cells / 2u ? level : level + 1.0f;
        const float x0 = static_cast<float>(i) * kCellM;
        const float x[4] = {x0, x0 + kCellM, x0 + kCellM, x0};
        const float z[4] = {0.0f, 0.0f, kCellM, kCellM};
        const int32_t base = static_cast<int32_t>(out.positions.size() / 3u);
        for (std::size_t c = 0u; c < 4u; ++c) {
            out.positions.insert(out.positions.end(), {x[c], y, z[c]});
            out.normals.insert(out.normals.end(), {0.0f, 1.0f, 0.0f});
            out.uvs.insert(out.uvs.end(), {x[c], z[c]});
            out.colors.insert(out.colors.end(), {0.5f, 0.25f, 0.5f, 1.0f});
        }
        out.indices.insert(out.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return out;
}

}  // namespace

TEST_CASE("water band reach: a length spans P pixels at d = L H / (2 P tan(fov / 2))") {
    const WaterBandReach reach = water_band_reach(kCellM, kChunkM, kViewHeight, kTanHalfFov);
    CHECK(reach.full_end_m == doctest::Approx(kCellM * kViewHeight / (2.0f * 32.0f * kTanHalfFov)));
    CHECK(reach.flow_end_m == doctest::Approx(kCellM * kViewHeight / (2.0f * 8.0f * kTanHalfFov)));
    CHECK(reach.reflect_end_m == doctest::Approx(kCellM * kViewHeight / (2.0f * 2.0f * kTanHalfFov)));
    CHECK(reach.chunk_end_m == doctest::Approx(kChunkM * kViewHeight / (2.0f * 8.0f * kTanHalfFov)));
    // the bands follow each other outward, LOD 0 nearest
    CHECK(reach.full_end_m < reach.flow_end_m);
    CHECK(reach.flow_end_m < reach.reflect_end_m);
    CHECK(reach.reflect_end_m <= reach.chunk_end_m);
}

TEST_CASE("water band reach: a chunk below its cell ends LOD 3 where LOD 2 ends") {
    const WaterBandReach reach = water_band_reach(kCellM, kCellM, kViewHeight, kTanHalfFov);
    CHECK(reach.chunk_end_m == doctest::Approx(reach.reflect_end_m));
}

TEST_CASE("water band reach: no size, no screen or no field of view gives no band") {
    for (const WaterBandReach reach :
         {water_band_reach(0.0f, kChunkM, kViewHeight, kTanHalfFov), water_band_reach(kCellM, 0.0f, kViewHeight, kTanHalfFov),
          water_band_reach(kCellM, kChunkM, 0.0f, kTanHalfFov), water_band_reach(kCellM, kChunkM, kViewHeight, 0.0f)}) {
        CHECK(reach.full_end_m == 0.0f);
        CHECK(reach.flow_end_m == 0.0f);
        CHECK(reach.reflect_end_m == 0.0f);
        CHECK(reach.chunk_end_m == 0.0f);
    }
}

TEST_CASE("water cell edge: measured from the south edge of the cells, zero without a cell") {
    CHECK(water_cell_edge(table_row(8u, 0.0f)) == doctest::Approx(kCellM));
    CHECK(water_cell_edge(PlantMeshArrays{}) == 0.0f);
}

TEST_CASE("water band tiles: one tile per chunk, every cell kept, level and box of its own cells") {
    const PlantMeshArrays row = table_row(8u, 2.0f);   // cells 0..3 in chunk 0 at level 2, 4..7 in chunk 1 at level 3
    std::vector<WaterBandTile> tiles;
    REQUIRE(water_band_tiles(row, kChunkM, tiles) == 2u);
    REQUIRE(tiles.size() == 2u);

    CHECK(tiles[0].chunk_x == 0);
    CHECK(tiles[0].chunk_z == 0);
    CHECK(tiles[1].chunk_x == 1);
    CHECK(tiles[0].level_m == doctest::Approx(2.0f));
    CHECK(tiles[1].level_m == doctest::Approx(3.0f));
    CHECK(tiles[0].min_x == doctest::Approx(0.0f));
    CHECK(tiles[0].max_x == doctest::Approx(4.0f));
    CHECK(tiles[1].min_x == doctest::Approx(4.0f));
    CHECK(tiles[1].max_x == doctest::Approx(8.0f));
    CHECK(tiles[0].color[1] == doctest::Approx(0.25f));

    // every cell of the source lands in exactly one tile, with its own vertices and all their attributes
    std::size_t indices = 0u;
    for (const WaterBandTile& tile : tiles) {
        indices += tile.arrays.indices.size();
        const std::size_t vertices = tile.arrays.positions.size() / 3u;
        CHECK(tile.arrays.normals.size() == vertices * 3u);
        CHECK(tile.arrays.uvs.size() == vertices * 2u);
        CHECK(tile.arrays.colors.size() == vertices * 4u);
        for (const int32_t index : tile.arrays.indices) {
            CHECK(index >= 0);
            CHECK(static_cast<std::size_t>(index) < vertices);
        }
    }
    CHECK(indices == row.indices.size());
    CHECK(water_cell_edge(tiles[0].arrays) == doctest::Approx(kCellM));
}

TEST_CASE("water band tiles: no chunk edge lays nothing") {
    std::vector<WaterBandTile> tiles;
    CHECK(water_band_tiles(table_row(4u, 0.0f), 0.0f, tiles) == 0u);
    CHECK(tiles.empty());
}

TEST_CASE("water band plane: one quad over the tile's box at its mean level, looking up") {
    std::vector<WaterBandTile> tiles;
    REQUIRE(water_band_tiles(table_row(8u, 2.0f), kChunkM, tiles) == 2u);
    PlantMeshArrays plane;
    CHECK(water_band_plane(tiles[1], plane) == 1u);
    REQUIRE(plane.positions.size() == 12u);
    REQUIRE(plane.indices.size() == 6u);
    for (std::size_t c = 0u; c < 4u; ++c) {
        CHECK(plane.positions[c * 3u + 1u] == doctest::Approx(3.0f));
        CHECK(plane.normals[c * 3u + 1u] == doctest::Approx(1.0f));
        CHECK(plane.colors[c * 4u + 1u] == doctest::Approx(0.25f));
    }
    CHECK(plane.positions[0] == doctest::Approx(4.0f));   // sw
    CHECK(plane.positions[3] == doctest::Approx(8.0f));   // se
    // (sw, se, ne): the cross product of se - sw and ne - sw points up in Godot's front-face order of build_ground
    CHECK(plane.indices[0] == 0);
    CHECK(plane.indices[1] == 1);
    CHECK(plane.indices[2] == 2);

    WaterBandTile empty{};
    CHECK(water_band_plane(empty, plane) == 0u);
}

}  // namespace ase::adp::godot

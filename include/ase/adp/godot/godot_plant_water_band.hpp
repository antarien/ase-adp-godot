#pragma once

/**
 * ASE GODOT ADAPTER - THE DETAIL BANDS OF THE WATER BY ITS SIZE ON THE SCREEN
 *
 * @file        godot_plant_water_band.hpp
 * @brief       From which distance the water of a chunk is drawn in which band, its tiles per chunk and the chunk plane
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO H.13, section "Detailstufen der Ansicht": the band of the water follows
 *              the size of a cell and of a chunk on the screen, never the distance alone. LOD 0 (flow per cell with
 *              foam, waves and the splash at the shore) while a cell spans more than GODOT_WATER_BAND_FULL_PX, LOD 1
 *              (flow per current cell, the sky reflected per cell) down to GODOT_WATER_BAND_FLOW_PX, LOD 2 (the
 *              reflecting surface per current cell, no strokes) down to GODOT_WATER_BAND_REFLECT_PX, LOD 3 (one
 *              surface per chunk) while the chunk spans more than GODOT_WATER_BAND_CHUNK_PX, LOD 4 (the basin as a
 *              tint in the ground tile) beyond, LOD 5 outside the view, where the camera culls every node and only
 *              the layer stands. The bands never change the calculation (I-8): they choose which drawing of the same
 *              arrays stands.
 *
 *              The pixel rule is the one of the impostor (godot_plant_impostor.hpp) and of the picture height of the
 *              plants (RenderDataLodLvlSelSystem): a length L at the distance d spans L H / (2 d tan(fov / 2))
 *              pixels on a screen H pixels high, so it spans P pixels at d = L H / (2 P tan(fov / 2)).
 *
 *              The cell edge is measured from the water arrays themselves (the ground cells build_water_table laid
 *              them on), the chunk edge comes in as the number the current port carries: nothing here is a size
 *              chosen by hand.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check it without a Godot
 *              process. The Godot side is fill_plant_water (godot_plant_brook.cpp).
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

#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

constexpr float GODOT_WATER_BAND_FULL_PX = 32.0f;     // a cell wider than this is drawn in LOD 0, pixels (plan H.13)
constexpr float GODOT_WATER_BAND_FLOW_PX = 8.0f;      // lower bound of LOD 1, the cell in pixels
constexpr float GODOT_WATER_BAND_REFLECT_PX = 2.0f;   // lower bound of LOD 2, the cell in pixels
constexpr float GODOT_WATER_BAND_CHUNK_PX = 8.0f;     // lower bound of LOD 3, the chunk in pixels; below it LOD 4
constexpr uint32_t GODOT_WATER_BAND_COUNT = 5u;       // the drawn bands LOD 0 to 4; LOD 5 is the culled view

/**
 * Where each band ends, metres from the camera. A band begins where the finer one ends; LOD 0 begins at the eye and
 * LOD 4 has no end but the far plane of the camera. All zero when the sizes or the screen give no picture.
 */
struct WaterBandReach {
    float full_end_m = 0.0f;      // LOD 0 ends where a cell shrinks to GODOT_WATER_BAND_FULL_PX
    float flow_end_m = 0.0f;      // LOD 1 ends where a cell shrinks to GODOT_WATER_BAND_FLOW_PX
    float reflect_end_m = 0.0f;   // LOD 2 ends where a cell shrinks to GODOT_WATER_BAND_REFLECT_PX
    float chunk_end_m = 0.0f;     // LOD 3 ends where the chunk shrinks to GODOT_WATER_BAND_CHUNK_PX
};

/**
 * The water of one chunk: the cells of the water arrays whose centre lies in it, laid again with their own vertices,
 * the box of its cells and the mean level of its surface, area-weighted by cell.
 */
struct WaterBandTile {
    int32_t chunk_x = 0;          // chunk grid east, floor of the cell centre over the chunk edge
    int32_t chunk_z = 0;          // chunk grid north
    float min_x = 0.0f;           // the box of the cells in the tile, world metres
    float max_x = 0.0f;
    float min_z = 0.0f;
    float max_z = 0.0f;
    float level_m = 0.0f;         // mean height of the water surface over the cells of the tile
    float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // mean vertex colour: velocity, depth and course shares
    PlantMeshArrays arrays;       // the cells of the tile, six indices each, in the order of the source
};

/**
 * The reaches of the bands for a cell edge `cell_m` and a chunk edge `chunk_m`, metres, on a screen `view_height`
 * pixels high under the tangent of half the vertical field of view `tan_half_fov`. A size, a height or a tangent not
 * above zero gives the reach with every end at zero.
 */
WaterBandReach water_band_reach(float cell_m, float chunk_m, float view_height, float tan_half_fov);

/**
 * The cell edge of the water arrays `arrays`, metres: the mean length of the south edge (sw to se, the first two
 * indices of every six) over its cells, in the plane. Zero for arrays without a cell.
 */
float water_cell_edge(const PlantMeshArrays& arrays);

/**
 * Splits `arrays` into one tile per chunk of edge `chunk_m` (the chunk holding the centre of the four corners of a
 * cell) and appends the tiles to `out` in the order their first cell appears. A chunk edge not above zero or arrays
 * without a cell lay nothing.
 *
 * @return the tiles laid
 */
uint32_t water_band_tiles(const PlantMeshArrays& arrays, float chunk_m, std::vector<WaterBandTile>& out);

/**
 * Appends to `out` the plane of `tile`: one quad over the box of its cells at its mean level, its triangles in the
 * order of the ground (the front face looking up), the normal up, the texture coordinates the world metres and the
 * colour the mean colour of the tile. LOD 3 draws it as the water of the chunk, LOD 4 as the tint of the basin.
 *
 * @return the cells laid, one, or zero for a tile without a cell
 */
uint32_t water_band_plane(const WaterBandTile& tile, PlantMeshArrays& out);

}  // namespace ase::adp::godot

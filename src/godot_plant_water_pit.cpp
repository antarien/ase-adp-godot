/**
 * ASE GODOT ADAPTER - THE OPEN WATER OF THE PITS ASE-HYDRO FILLS
 *
 * @file        godot_plant_water_pit.cpp
 * @brief       The water level over the ground cell of every pit: its floor plus the depth the pit port carries
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO H.3 and H.4, register D2506. See godot_plant_water.hpp,
 *              pit_water_levels.
 *
 *              THE PIT STANDS WHERE ASE-HYDRO FILLS IT: HydroCellPitCalcSystem fills a dug pit from the
 *              groundwater and, near a brook, over the bank; ase-gis answers the open water over the pit floor
 *              in the pit cell (GIS_HYD_CELL_PIT_DEPTH), ase-pl-hydrogeology files it on hydrogeology.pit.v1, and
 *              GodotHostResourceManager::read_pits copies it. The view lays that water on the ground cell that
 *              holds the centre of the pit, at the floor of that cell plus the depth, and reckons no level of its
 *              own: until D2506 the care view filled every hollow below NN0 (GODOT_WATER_TABLE_M) and every
 *              vertex within the groundwater reach of a brook (B0286), levels ase-hydro never computed for that
 *              place, so a pit showed water the balance did not carry.
 *
 *              THE CELL IS READ FROM ITS OWN CORNERS: every six indices of the ground mesh are one cell, corners
 *              sw, se, ne, nw; its extent and its floor come from those corners, so no cell edge of the adapter
 *              stands beside the raster ase-gis maps the pits on. This is its own responsibility, a lookup of
 *              pit records over the ground cells rather than the levels of a bed, and so it stands in its own
 *              unit beside godot_plant_water.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_water.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

void pit_water_levels(const std::vector<VivariumPitView>& pits, const PlantMeshArrays& ground,
                      std::vector<float>& level_of, std::vector<uint8_t>& flooded) {
    const std::size_t ground_vertices = ground.positions.size() / 3u;
    if (level_of.size() < ground_vertices || flooded.size() < ground_vertices) {
        return;  // the caller sized no level for every ground vertex: nothing is laid past them
    }
    for (std::size_t first = 0u; first + 6u <= ground.indices.size(); first += 6u) {
        // the cell's corners sw, se, ne, nw out of its triangles (sw, se, ne) and (sw, ne, nw)
        const int32_t corners[4] = {ground.indices[first], ground.indices[first + 1u], ground.indices[first + 2u],
                                    ground.indices[first + 5u]};
        bool known = true;
        for (const int32_t corner : corners) {
            known = known && corner >= 0 && static_cast<std::size_t>(corner) < ground_vertices;
        }
        if (!known) {
            continue;  // an index past the ground: no cell of the ground, no pit on it
        }
        const auto origin = static_cast<std::size_t>(corners[0]);
        float west = ground.positions[origin * 3u];
        float east = west;
        float south = ground.positions[origin * 3u + 2u];
        float north = south;
        float floor = ground.positions[origin * 3u + 1u];
        for (const int32_t corner : corners) {
            const auto at = static_cast<std::size_t>(corner);
            const float x = ground.positions[at * 3u];
            const float height = ground.positions[at * 3u + 1u];
            const float z = ground.positions[at * 3u + 2u];
            west = x < west ? x : west;
            east = x > east ? x : east;
            south = z < south ? z : south;
            north = z > north ? z : north;
            floor = height < floor ? height : floor;
        }
        for (const VivariumPitView& pit : pits) {
            const auto x = static_cast<float>(pit.x);
            const auto z = static_cast<float>(pit.z);
            const auto depth = static_cast<float>(pit.depth_m);
            // a pit ase-gis has not answered (NaN), a dry pit, or one whose centre this cell does not hold
            if (!std::isfinite(pit.depth_m) || !(depth > 0.0f) || x < west || !(x < east) || z < south ||
                !(z < north)) {
                continue;
            }
            const float level = floor + depth;
            for (const int32_t corner : corners) {
                const auto at = static_cast<std::size_t>(corner);
                if (flooded[at] == 0u || level > level_of[at]) {
                    level_of[at] = level;
                    flooded[at] = 1u;
                }
            }
        }
    }
}

}  // namespace ase::adp::godot

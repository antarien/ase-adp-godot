#pragma once

/**
 * ASE ADAPTER TESTS - THE GROUNDS, BEDS AND PITS THE WATER TESTS LAY THEIR WATER ON
 *
 * @file        test_godot_plant_water_support.hpp
 * @design      DSGN_066
 * @brief       The table grounds, the brook in their bed, the pit records and the lookup of the water over a place
 * @description The water table is pinned in two executables (tests/CMakeLists.txt, each with its own doctest
 *              entry point): ase-adp-godot-water-test (test_godot_plant_water.cpp) checks the water in the beds
 *              and its line, ase-adp-godot-waterpit-test (test_godot_plant_water_pit.cpp) the water of the pits
 *              ase-hydro fills (D2506). Both lay their
 *              water over the same grounds, built with build_ground as the care view builds them, and read it
 *              back the same way; these functions are that one shared part, never a copy in either unit.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/godot_plant_water.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** One sample of a table test ground: object id, place and height, nothing else. */
inline VivariumTerrainView table_sample(uint32_t id, double x, double z, double height) {
    VivariumTerrainView sample;
    sample.object_id = id;
    sample.x = x;
    sample.z = z;
    sample.height = height;
    return sample;
}

inline VivariumGroundCellView table_cell(uint32_t sw, uint32_t se, uint32_t ne, uint32_t nw) {
    VivariumGroundCellView cell;
    cell.object_id = sw;
    cell.sw = sw;
    cell.se = se;
    cell.ne = ne;
    cell.nw = nw;
    return cell;
}

/** The ground mesh build_ground lays over `samples` and `cells`, as the care view hands it to the table. */
inline PlantMeshArrays table_mesh(const std::vector<VivariumTerrainView>& samples,
                                  const std::vector<VivariumGroundCellView>& cells) {
    PlantMeshArrays ground;
    std::vector<PlantGroundSlot> slots;
    (void)build_ground(samples, cells, ground, slots);
    return ground;
}

/**
 * A bed along z 0 falling east, its bank along z 1 one metre above it. Ids: 0 (0,0) 1 (1,0) 2 (2,0) bed at
 * 1, 0.5 and -0.25; 3 (0,1) 4 (1,1) 5 (2,1) bank at 2, 1.5 and 0.75.
 */
inline std::vector<VivariumTerrainView> bed_ground() {
    return {table_sample(0u, 0.0, 0.0, 1.0),  table_sample(1u, 1.0, 0.0, 0.5),
            table_sample(2u, 2.0, 0.0, -0.25), table_sample(3u, 0.0, 1.0, 2.0),
            table_sample(4u, 1.0, 1.0, 1.5),  table_sample(5u, 2.0, 1.0, 0.75)};
}

/** The brook ase-hydro reckons in that bed: x 0 to 2 along z 0, a quarter metre to either side, 0.2 m deep. */
inline VivariumStreamView bed_brook() {
    VivariumStreamView stream;
    stream.object_id = 9u;
    stream.x = 0.0;
    stream.z = 0.0;
    stream.end_x = 2.0;
    stream.end_z = 0.0;
    stream.half_m = 0.25;
    stream.depth_m = 0.2;
    stream.velocity_m_s = 1.0;
    return stream;
}

/**
 * The same brook as its record reaches the view while ase-hydro publishes no groundwater for it: NaN in both
 * fields, as GodotHostResourceManager hands the stream port (N0484). No table lifts its bed: the water stands at
 * the bed plus the depth, also where that lies below NN0 (D2506).
 */
inline VivariumStreamView unfed_brook() {
    VivariumStreamView stream = bed_brook();
    stream.groundwater_level_m = std::nan("");
    stream.groundwater_reach_m = std::nan("");
    return stream;
}

/** The height of the water laid over the ground place (x, z), NaN where none lies. */
inline float water_at(const PlantMeshArrays& arrays, float x, float z) {
    for (std::size_t v = 0u; v < arrays.positions.size() / 3u; ++v) {
        if (arrays.positions[v * 3u] == doctest::Approx(x) && arrays.positions[v * 3u + 2u] == doctest::Approx(z)) {
            return arrays.positions[v * 3u + 1u];
        }
    }
    return std::nanf("");
}

/**
 * A flat bed along z 0 at height 1, its bank along z 1 at height 2, and a row along z 2 at height 2 with a pit
 * dug at (1,2) down to 1.3. Ids: 0 (0,0) 1 (1,0) 2 (2,0) bed; 3 (0,1) 4 (1,1) 5 (2,1) bank; 6 (0,2) 7 (1,2) pit
 * 8 (2,2).
 */
inline std::vector<VivariumTerrainView> pit_ground() {
    return {table_sample(0u, 0.0, 0.0, 1.0), table_sample(1u, 1.0, 0.0, 1.0), table_sample(2u, 2.0, 0.0, 1.0),
            table_sample(3u, 0.0, 1.0, 2.0), table_sample(4u, 1.0, 1.0, 2.0), table_sample(5u, 2.0, 1.0, 2.0),
            table_sample(6u, 0.0, 2.0, 2.0), table_sample(7u, 1.0, 2.0, 1.3), table_sample(8u, 2.0, 2.0, 2.0)};
}

/** The bed cells and the cells of the pit row, in the ground's order sw, se, ne, nw. */
inline std::vector<VivariumGroundCellView> pit_cells() {
    return {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u), table_cell(3u, 4u, 7u, 6u),
            table_cell(4u, 5u, 8u, 7u)};
}

/** The record of hydrogeology.pit.v1 for the pit at the soil cell centre (0.5, 1.5), `depth` of open water. */
inline VivariumPitView dug_pit(double depth) {
    VivariumPitView pit;
    pit.object_id = 11u;
    pit.x = 0.5;
    pit.z = 1.5;
    pit.depth_m = depth;
    pit.spill_m3_s = 0.0;
    return pit;
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - THE WATER OF THE PITS ASE-HYDRO FILLS, WITHOUT GODOT
 *
 * @file        test_godot_plant_water_pit.cpp
 * @design      DSGN_066
 * @brief       Pins the water of a pit at the floor of its cell plus the depth of its record, and no water elsewhere
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO H.3 and H.4, register D2506: build_water_table lays the water of
 *              every pit record of hydrogeology.pit.v1 on the ground cell holding its centre, at the floor of
 *              that cell plus the depth ase-gis answers for the pit (pit_water_levels). A hollow without a pit
 *              record stays dry: the view lays no table at NN0 and no groundwater reach of a brook of its own,
 *              the levels B0286 laid until D2506.
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

#include "test_godot_plant_water_support.hpp"

#include <ase/adp/godot/scene/types.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

namespace ase::adp::godot {

TEST_CASE("a pit stands at the floor of its cell plus the depth its record carries (D2506)") {
    PlantMeshArrays arrays;
    // the two bed cells, the pit cell and its neighbour that shares the pit corner below the water
    REQUIRE(build_water_table({bed_brook()}, {dug_pit(0.1)}, table_mesh(pit_ground(), pit_cells()), arrays) == 4u);
    CHECK(water_at(arrays, 1.0f, 0.0f) == doctest::Approx(1.2f));  // the bed: its bed plus the depth of the stream
    CHECK(water_at(arrays, 1.0f, 2.0f) == doctest::Approx(1.4f));  // the pit: its floor 1.3 plus 0.1
    CHECK(water_at(arrays, 0.0f, 2.0f) == doctest::Approx(1.4f));  // its rim: the same plane, the ground above hides it
    CHECK(water_at(arrays, 2.0f, 2.0f) == doctest::Approx(1.4f));  // the neighbour's corner: the lowest water of its cell
    // the pit's water is still: no velocity, the course north, its depth over the pit floor in green
    bool seen = false;
    for (std::size_t v = 0u; v < arrays.positions.size() / 3u; ++v) {
        if (arrays.positions[v * 3u] == doctest::Approx(1.0f) && arrays.positions[v * 3u + 2u] == doctest::Approx(2.0f)) {
            seen = true;
            CHECK(arrays.colors[v * 4u] == doctest::Approx(0.0f));
            CHECK(arrays.colors[v * 4u + 1u] == doctest::Approx(0.1f / GODOT_WATER_DEPTH_FULL_M));
            CHECK(arrays.colors[v * 4u + 3u] == doctest::Approx(1.0f));
        }
    }
    CHECK(seen);
    // a deeper pit stands higher by the difference, the floor unchanged
    PlantMeshArrays deeper;
    REQUIRE(build_water_table({bed_brook()}, {dug_pit(0.5)}, table_mesh(pit_ground(), pit_cells()), deeper) == 4u);
    CHECK(water_at(deeper, 1.0f, 2.0f) == doctest::Approx(1.8f));
}

TEST_CASE("a hollow without a pit record, a dry pit and a pit on no cell stay dry (D2506)") {
    // the hollow at (1,2) holds no record: the view lays no water of its own there, only the bed holds water
    PlantMeshArrays arrays;
    CHECK(build_water_table({bed_brook()}, {}, table_mesh(pit_ground(), pit_cells()), arrays) == 2u);
    CHECK(std::isnan(water_at(arrays, 1.0f, 2.0f)));
    // a brook with a groundwater table above the hollow fills it no more: the table lifts only its own bed
    VivariumStreamView fed = bed_brook();
    fed.groundwater_level_m = 1.5;
    fed.groundwater_reach_m = 2.5;
    PlantMeshArrays beside;
    CHECK(build_water_table({fed}, {}, table_mesh(pit_ground(), pit_cells()), beside) == 2u);
    CHECK(std::isnan(water_at(beside, 1.0f, 2.0f)));
    // a pit ase-hydro keeps dry, and one ase-gis has not answered yet (NaN)
    PlantMeshArrays dry;
    CHECK(build_water_table({bed_brook()}, {dug_pit(0.0), dug_pit(std::nan(""))}, table_mesh(pit_ground(), pit_cells()),
                            dry) == 2u);
    CHECK(std::isnan(water_at(dry, 1.0f, 2.0f)));
    // a pit whose centre no ground cell holds
    VivariumPitView far = dug_pit(0.4);
    far.x = 50.5;
    PlantMeshArrays off;
    CHECK(build_water_table({bed_brook()}, {far}, table_mesh(pit_ground(), pit_cells()), off) == 2u);
    CHECK(std::isnan(water_at(off, 1.0f, 2.0f)));
}

TEST_CASE("the water of a pit goes to its own arrays, the brook's to the river's (D2508)") {
    PlantMeshArrays brook;
    PlantMeshArrays pit;
    // the same four cells as one table: the two bed cells to the river, the pit cell and its neighbour to the pit
    REQUIRE(build_water_table({bed_brook()}, {dug_pit(0.1)}, table_mesh(pit_ground(), pit_cells()), brook, pit) == 4u);
    CHECK(brook.indices.size() == 12u);
    CHECK(pit.indices.size() == 12u);
    CHECK(water_at(brook, 1.0f, 0.0f) == doctest::Approx(1.2f));  // the bed stays with the river
    CHECK(std::isnan(water_at(brook, 1.0f, 2.0f)));                // no pit water in the river's arrays
    CHECK(water_at(pit, 1.0f, 2.0f) == doctest::Approx(1.4f));     // the pit: its floor 1.3 plus 0.1
    CHECK(std::isnan(water_at(pit, 1.0f, 0.0f)));                  // no bed in the pit's arrays
    // every index of each arrays names a vertex of that arrays
    for (const int32_t index : pit.indices) {
        CHECK(static_cast<std::size_t>(index) < pit.positions.size() / 3u);
    }
    for (const int32_t index : brook.indices) {
        CHECK(static_cast<std::size_t>(index) < brook.positions.size() / 3u);
    }
    // without a pit record the pit's arrays stay empty, the river's hold the bed
    PlantMeshArrays bed_only;
    PlantMeshArrays no_pit;
    CHECK(build_water_table({bed_brook()}, {}, table_mesh(pit_ground(), pit_cells()), bed_only, no_pit) == 2u);
    CHECK(no_pit.positions.empty());
    CHECK(bed_only.indices.size() == 12u);
}

TEST_CASE("pit_water_levels lays nothing past the vertices its caller sized (D2506)") {
    const PlantMeshArrays ground = table_mesh(pit_ground(), pit_cells());
    std::vector<float> level_of(2u, 0.0f);
    std::vector<uint8_t> flooded(2u, 0u);
    pit_water_levels({dug_pit(0.1)}, ground, level_of, flooded);
    CHECK(flooded[0] == 0u);
    CHECK(flooded[1] == 0u);
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - WATER OF THE BROOKS WITHOUT GODOT
 *
 * @file        test_godot_plant_water.cpp
 * @design      DSGN_066
 * @brief       Pins the water plane at the ase-hydro level of each bed and the ribbon of water along a course
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3, B0281: the vertices and triangles
 *              build_water_table lays in the bed of every stream at its bed plus the ase-hydro depth over the
 *              cells of the ground mesh (build_ground over the terrain and ground cell ports), none outside a
 *              bed, and those build_water lays along a course, as they go to ArrayMesh. B0285: the line of the
 *              water build_water_line lays at the same levels, where the care view sets the brook's sign.
 *              D2506: no level refers to NN0, a bed stands at its own level or at the groundwater ase-hydro
 *              gives its brook (PLAN_ASE_VIVARIUM_PHASE_10_HYDRO H.3); the water of the pits is pinned in
 *              test_godot_plant_water_pit.cpp over the grounds of test_godot_plant_water_support.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-01
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_plant_water_support.hpp"

#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/** A ground of samples one metre apart over x 0..10 and z -2..2, falling 0.1 m per metre east, the bed along z 0. */
std::vector<VivariumTerrainView> falling_ground() {
    std::vector<VivariumTerrainView> samples;
    uint32_t id = 0u;
    for (int32_t x = 0; x <= 10; ++x) {
        for (int32_t z = -2; z <= 2; ++z) {
            VivariumTerrainView sample;
            sample.object_id = id++;
            sample.x = static_cast<double>(x);
            sample.z = static_cast<double>(z);
            sample.height = 5.0 - 0.1 * x + (z == 0 ? -0.3 : 0.0);
            samples.push_back(sample);
        }
    }
    return samples;
}

VivariumStreamView brook() {
    VivariumStreamView stream;
    stream.object_id = 7u;
    stream.x = 0.0;
    stream.z = 0.0;
    stream.end_x = 10.0;
    stream.end_z = 0.0;
    stream.half_m = 0.5;
    stream.depth_m = 0.2;
    stream.velocity_m_s = 1.0;
    stream.discharge_m3_s = 0.2;
    return stream;
}

}  // namespace

TEST_CASE("the water stands the depth above the bed, as wide as the bed, along its whole course") {
    PlantMeshArrays arrays;
    REQUIRE(build_water({brook()}, falling_ground(), 1.0f, arrays) == 1u);
    const std::size_t stations = GODOT_WATER_STATIONS + 1u;
    REQUIRE(arrays.positions.size() == stations * 2u * 3u);
    CHECK(arrays.indices.size() == static_cast<std::size_t>(GODOT_WATER_STATIONS) * 6u);
    // the start: the two sides of the bed, at the bed of the first station plus the depth
    CHECK(arrays.positions[0] == doctest::Approx(0.0f));
    CHECK(arrays.positions[1] == doctest::Approx(4.9f));
    CHECK(arrays.positions[2] == doctest::Approx(0.5f));
    CHECK(arrays.positions[5] == doctest::Approx(-0.5f));
    // the end, ten metres on and one metre lower
    const std::size_t last = (stations - 1u) * 2u * 3u;
    CHECK(arrays.positions[last] == doctest::Approx(10.0f));
    CHECK(arrays.positions[last + 1u] == doctest::Approx(3.9f));
}

TEST_CASE("the first triangle of the ribbon turns up as the ground cells do") {
    PlantMeshArrays arrays;
    REQUIRE(build_water({brook()}, falling_ground(), 1.0f, arrays) == 1u);
    const auto corner = [&arrays](int32_t index, std::size_t axis) {
        return arrays.positions[static_cast<std::size_t>(index) * 3u + axis];
    };
    const int32_t a = arrays.indices[0];
    const int32_t b = arrays.indices[1];
    const int32_t c = arrays.indices[2];
    const float abx = corner(b, 0u) - corner(a, 0u);
    const float abz = corner(b, 2u) - corner(a, 2u);
    const float acx = corner(c, 0u) - corner(a, 0u);
    const float acz = corner(c, 2u) - corner(a, 2u);
    // the ground cell (sw, se, ne): east, then north, a positive turn in x and z
    CHECK(abx * acz - abz * acx > 0.0f);
}

TEST_CASE("the vertex colour carries velocity and depth in the range a colour holds") {
    PlantMeshArrays arrays;
    VivariumStreamView flood = brook();
    flood.velocity_m_s = GODOT_WATER_VELOCITY_FULL_M_S * 4.0;
    REQUIRE(build_water({brook(), flood}, falling_ground(), 1.0f, arrays) == 2u);
    CHECK(arrays.colors[0] == doctest::Approx(1.0f / GODOT_WATER_VELOCITY_FULL_M_S));
    CHECK(arrays.colors[1] == doctest::Approx(0.2f / GODOT_WATER_DEPTH_FULL_M));
    CHECK(arrays.colors[arrays.colors.size() - 4u] == doctest::Approx(1.0f));
}

TEST_CASE("the surface never rises along the course over a bump of the bed") {
    std::vector<VivariumTerrainView> samples = falling_ground();
    for (VivariumTerrainView& sample : samples) {
        if (sample.x == 5.0 && sample.z == 0.0) {
            sample.height = 6.0;
        }
    }
    PlantMeshArrays arrays;
    REQUIRE(build_water({brook()}, samples, 1.0f, arrays) == 1u);
    for (std::size_t v = 2u; v < arrays.positions.size() / 3u; ++v) {
        CHECK(arrays.positions[v * 3u + 1u] <= arrays.positions[(v - 2u) * 3u + 1u] + 0.0001f);
    }
}

TEST_CASE("a bed filed against its fall holds its water on the ground of every station") {
    VivariumStreamView uphill = brook();
    uphill.x = 10.0;
    uphill.end_x = 0.0;
    PlantMeshArrays arrays;
    REQUIRE(build_water({uphill}, falling_ground(), 1.0f, arrays) == 1u);
    const std::size_t stations = GODOT_WATER_STATIONS + 1u;
    REQUIRE(arrays.positions.size() == stations * 2u * 3u);
    // the filed start is the low end, the mouth: its bed plus the depth
    CHECK(arrays.positions[0] == doctest::Approx(10.0f));
    CHECK(arrays.positions[1] == doctest::Approx(3.9f));
    // the filed end lies one metre higher, and so does the water there - it is not pinned to the mouth
    const std::size_t last = (stations - 1u) * 2u * 3u;
    CHECK(arrays.positions[last] == doctest::Approx(0.0f));
    CHECK(arrays.positions[last + 1u] == doctest::Approx(4.9f));
    // every station: the surface stands the depth above the bed under its middle
    for (std::size_t station = 0u; station < stations; ++station) {
        const float x = (arrays.positions[station * 6u] + arrays.positions[station * 6u + 3u]) * 0.5f;
        CHECK(arrays.positions[station * 6u + 1u] >= 5.0f - 0.1f * x - 0.3f + 0.2f - 0.11f);
    }
}

TEST_CASE("the water follows a bed that winds off its filed course and keeps its width") {
    // the bed swings one metre north between x 4 and x 6 and comes back
    std::vector<VivariumTerrainView> samples = falling_ground();
    for (VivariumTerrainView& sample : samples) {
        const bool swung = sample.x >= 4.0 && sample.x <= 6.0;
        if (swung && sample.z == 0.0) {
            sample.height += 0.3;
        }
        if (swung && sample.z == 1.0) {
            sample.height -= 0.3;
        }
    }
    PlantMeshArrays arrays;
    REQUIRE(build_water({brook()}, samples, 1.0f, arrays) == 1u);
    const auto middle_z = [&arrays](std::size_t station) {
        return (arrays.positions[station * 6u + 2u] + arrays.positions[station * 6u + 5u]) * 0.5f;
    };
    const auto width = [&arrays](std::size_t station) {
        const float dx = arrays.positions[station * 6u + 3u] - arrays.positions[station * 6u];
        const float dz = arrays.positions[station * 6u + 5u] - arrays.positions[station * 6u + 2u];
        return dx * dx + dz * dz;
    };
    // at the start, in the bend at x 5 and at the end
    CHECK(middle_z(0u) == doctest::Approx(0.0f));
    CHECK(middle_z(GODOT_WATER_STATIONS / 2u) == doctest::Approx(1.0f));
    CHECK(middle_z(GODOT_WATER_STATIONS) == doctest::Approx(0.0f));
    // the bed in the bend is the swung one, not the bank where the course was filed
    CHECK(arrays.positions[(GODOT_WATER_STATIONS / 2u) * 6u + 1u] == doctest::Approx(5.0f - 0.5f - 0.3f + 0.2f));
    // as wide as the bed everywhere, also where the ribbon turns
    for (std::size_t station = 0u; station <= GODOT_WATER_STATIONS; ++station) {
        CHECK(width(station) == doctest::Approx(1.0f));
    }
}

TEST_CASE("a dry bed, a bed without length and a bed outside the ground draw nothing") {
    VivariumStreamView dry = brook();
    dry.depth_m = 0.0;
    VivariumStreamView point = brook();
    point.end_x = point.x;
    VivariumStreamView far = brook();
    far.x = 500.0;
    far.end_x = 510.0;
    PlantMeshArrays arrays;
    CHECK(build_water({dry, point, far}, falling_ground(), 1.0f, arrays) == 0u);
    CHECK(arrays.positions.empty());
    CHECK(arrays.indices.empty());
    CHECK(build_water({brook()}, {}, 1.0f, arrays) == 0u);
    CHECK(arrays.indices.empty());
}

namespace {

/**
 * Two cells side by side over x 0..2 and z 0..1: the west cell dry above NN0, the east cell with its east
 * corners dug 0.25 m below it. Ids: 0 (0,0) 1 (1,0) 2 (2,0) 3 (0,1) 4 (1,1) 5 (2,1).
 */
std::vector<VivariumTerrainView> table_ground() {
    return {table_sample(0u, 0.0, 0.0, 1.0),  table_sample(1u, 1.0, 0.0, 0.5),
            table_sample(2u, 2.0, 0.0, -0.25), table_sample(3u, 0.0, 1.0, 1.0),
            table_sample(4u, 1.0, 1.0, 0.5),  table_sample(5u, 2.0, 1.0, -0.25)};
}

}  // namespace

TEST_CASE("without a record of ase-hydro no water stands anywhere, not even in a hollow below NN0 (D2506)") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    // the east cell is dug below NN0: the view lays no table of its own there
    PlantMeshArrays arrays;
    CHECK(build_water_table({}, {}, table_mesh(table_ground(), cells), arrays) == 0u);
    CHECK(arrays.positions.empty());
    // a dry stream holds no water of its own, also where its bed is dug below NN0
    VivariumStreamView dry = bed_brook();
    dry.depth_m = 0.0;
    PlantMeshArrays bed;
    CHECK(build_water_table({dry}, {}, table_mesh(bed_ground(), cells), bed) == 0u);
    CHECK(bed.positions.empty());
}

TEST_CASE("the water stands in the bed at the bed plus the depth ase-hydro gives the stream") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    PlantMeshArrays arrays;
    REQUIRE(build_water_table({unfed_brook()}, {}, table_mesh(bed_ground(), cells), arrays) == 2u);
    REQUIRE(arrays.positions.size() == 6u * 3u);
    REQUIRE(arrays.indices.size() == 12u);
    // the vertices in the order the cells name them: (0,0) (1,0) (1,1) (0,1), then (2,0) (2,1)
    // the lower end stands at -0.25 plus 0.2, below NN0: the record carries no groundwater, so no table lifts it
    CHECK(arrays.positions[0u * 3u + 1u] == doctest::Approx(1.2f));
    CHECK(arrays.positions[1u * 3u + 1u] == doctest::Approx(0.7f));
    CHECK(arrays.positions[4u * 3u + 1u] == doctest::Approx(-0.05f));
    // a bank corner carries the lowest water of its cell's bed corners; its ground stands above it
    CHECK(arrays.positions[2u * 3u + 1u] == doctest::Approx(0.7f));
    CHECK(arrays.positions[3u * 3u + 1u] == doctest::Approx(0.7f));
    CHECK(arrays.positions[5u * 3u + 1u] == doctest::Approx(-0.05f));
    // where ase-hydro gives the brook a groundwater table, the bed stands at least at it
    VivariumStreamView fed = unfed_brook();
    fed.groundwater_level_m = 0.1;
    PlantMeshArrays table;
    REQUIRE(build_water_table({fed}, {}, table_mesh(bed_ground(), cells), table) == 2u);
    CHECK(table.positions[4u * 3u + 1u] == doctest::Approx(0.1f));
    for (std::size_t v = 0u; v < 6u; ++v) {
        CHECK(arrays.normals[v * 3u + 1u] == doctest::Approx(1.0f));
    }
    // the corners in the cell's order sw, se, ne, nw: the first triangle (sw, se, ne) as the ground's
    CHECK(arrays.positions[0] == doctest::Approx(0.0f));
    CHECK(arrays.positions[3] == doctest::Approx(1.0f));
    CHECK(arrays.indices[0] == 0);
    CHECK(arrays.indices[1] == 1);
    CHECK(arrays.indices[2] == 2);
    CHECK(arrays.indices[5] == 3);
    // the texture coordinates are the world metres east and north
    CHECK(arrays.uvs[2] == doctest::Approx(1.0f));
    CHECK(arrays.uvs[5] == doctest::Approx(1.0f));
}

TEST_CASE("the depth of the water above the ground goes into green, none where the bank stands above it") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    PlantMeshArrays arrays;
    REQUIRE(build_water_table({unfed_brook()}, {}, table_mesh(bed_ground(), cells), arrays) == 2u);
    CHECK(arrays.colors[0u * 4u + 1u] == doctest::Approx(0.2f / GODOT_WATER_DEPTH_FULL_M));
    // the lower end: the depth ase-hydro gives the stream over its bed dug to -0.25
    CHECK(arrays.colors[4u * 4u + 1u] == doctest::Approx(0.2f / GODOT_WATER_DEPTH_FULL_M));
    CHECK(arrays.colors[2u * 4u + 1u] == doctest::Approx(0.0f));
}

TEST_CASE("the water carries velocity and course of the stream whose bed holds the vertex") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    PlantMeshArrays arrays;
    REQUIRE(build_water_table({bed_brook()}, {}, table_mesh(bed_ground(), cells), arrays) == 2u);
    // the bed corners: velocity share and the course east
    CHECK(arrays.colors[0] == doctest::Approx(1.0f / GODOT_WATER_VELOCITY_FULL_M_S));
    CHECK(arrays.colors[2] == doctest::Approx(1.0f));
    CHECK(arrays.colors[3] == doctest::Approx(0.5f));
    // the bank corner lies one metre off, beyond half the width plus the swing: still water, the course north
    CHECK(arrays.colors[2u * 4u] == doctest::Approx(0.0f));
    CHECK(arrays.colors[2u * 4u + 2u] == doctest::Approx(0.5f));
    CHECK(arrays.colors[2u * 4u + 3u] == doctest::Approx(1.0f));
}

TEST_CASE("a cell with a corner the port does not carry draws no water, and shared corners are one vertex") {
    PlantMeshArrays arrays;
    CHECK(build_water_table({bed_brook()}, {}, table_mesh(bed_ground(), {table_cell(1u, 2u, 99u, 4u)}), arrays) ==
          0u);
    CHECK(arrays.positions.empty());
    VivariumStreamView wide = bed_brook();
    wide.half_m = 1.0;
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    REQUIRE(build_water_table({wide}, {}, table_mesh(table_ground(), cells), arrays) == 2u);
    CHECK(arrays.positions.size() == 6u * 3u);
    CHECK(arrays.indices.size() == 12u);
}

TEST_CASE("the line of the water runs station by station on the level the table lays its water at (B0285)") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    std::vector<float> line;
    // the bed corners fall into the stations 0, 16 and 32; the bank lies beyond the reach of the bed
    REQUIRE(build_water_line(unfed_brook(), table_mesh(bed_ground(), cells), nullptr, nullptr, line) == 3u);
    REQUIRE(line.size() == 3u * 3u);
    // the same levels as the table: bed plus depth, 1.2, 0.7 and -0.05, over the bed at z 0
    CHECK(line[0] == doctest::Approx(0.0f));
    CHECK(line[1] == doctest::Approx(1.2f));
    CHECK(line[2] == doctest::Approx(0.0f));
    CHECK(line[3] == doctest::Approx(1.0f));
    CHECK(line[4] == doctest::Approx(0.7f));
    CHECK(line[5] == doctest::Approx(0.0f));
    CHECK(line[6] == doctest::Approx(2.0f));
    CHECK(line[7] == doctest::Approx(-0.05f));
    CHECK(line[8] == doctest::Approx(0.0f));
    // a dry stream holds no water and so lays no line
    VivariumStreamView dry = bed_brook();
    dry.depth_m = 0.0;
    std::vector<float> none;
    CHECK(build_water_line(dry, table_mesh(bed_ground(), cells), nullptr, nullptr, none) == 0u);
    CHECK(none.empty());
}

TEST_CASE("the line of the water lays only the water the eye sees over the bank (B0285)") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    // an eye that sees nothing west of x 0.5: the bank hides the water of station 0 at x 0
    bool (*const east)(const void*, float, float, float) = [](const void* context, float x, float y, float z) {
        (void)context;
        (void)y;
        (void)z;
        return x > 0.5f;
    };
    std::vector<float> line;
    REQUIRE(build_water_line(unfed_brook(), table_mesh(bed_ground(), cells), east, nullptr, line) == 2u);
    REQUIRE(line.size() == 2u * 3u);
    CHECK(line[0] == doctest::Approx(1.0f));
    CHECK(line[1] == doctest::Approx(0.7f));
    CHECK(line[3] == doctest::Approx(2.0f));
    CHECK(line[4] == doctest::Approx(-0.05f));
    // an eye that sees no water at all lays no line
    bool (*const blind)(const void*, float, float, float) = [](const void* context, float x, float y, float z) {
        (void)context;
        (void)x;
        (void)y;
        (void)z;
        return false;
    };
    std::vector<float> none;
    CHECK(build_water_line(bed_brook(), table_mesh(bed_ground(), cells), blind, nullptr, none) == 0u);
    CHECK(none.empty());
}

TEST_CASE("the lines of water and bed stand on the brook's course, depth and table, never on its flow (P0692)") {
    // SignMarker::follow_brook keeps the stations between frames and reads them again only when a field they are
    // read of changes; the flow ase-hydro moves with every sample lays none of them
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    const PlantMeshArrays ground = table_mesh(bed_ground(), cells);
    VivariumStreamView still = unfed_brook();
    VivariumStreamView flowing = unfed_brook();
    // unfed_brook carries NaN in groundwater_reach_m: the flowing record names a reach of its own, so the field
    // differs in fact and the comparison below is no NaN against NaN
    flowing.velocity_m_s = still.velocity_m_s + 2.5;
    flowing.discharge_m3_s = still.discharge_m3_s + 0.75;
    flowing.groundwater_reach_m = 3.0;
    std::vector<float> still_water;
    std::vector<float> flowing_water;
    REQUIRE(build_water_line(still, ground, nullptr, nullptr, still_water) == 3u);
    REQUIRE(build_water_line(flowing, ground, nullptr, nullptr, flowing_water) == 3u);
    CHECK(flowing_water == still_water);
    std::vector<float> still_bed;
    std::vector<float> flowing_bed;
    CHECK(build_bed_line(still, ground, nullptr, nullptr, still_bed) ==
          build_bed_line(flowing, ground, nullptr, nullptr, flowing_bed));
    CHECK(flowing_bed == still_bed);
    // the groundwater table does lay the water: a table above the lower end lifts its station, so the cache key
    // of follow_brook keeps it
    VivariumStreamView fed = unfed_brook();
    fed.groundwater_level_m = 0.1;
    std::vector<float> fed_water;
    REQUIRE(build_water_line(fed, ground, nullptr, nullptr, fed_water) == 3u);
    CHECK(fed_water[7] == doctest::Approx(0.1f));
    CHECK(fed_water != still_water);
}

}  // namespace ase::adp::godot

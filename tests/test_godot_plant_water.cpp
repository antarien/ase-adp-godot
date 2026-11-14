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
 *              B0286: a pit dug beside a bed below the groundwater ase-hydro gives that brook fills to it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-01
 * @modified    2026-11-13
 * @version     00.00.06.00001
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
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

/** One sample of a table test ground: object id, place and height, nothing else. */
VivariumTerrainView table_sample(uint32_t id, double x, double z, double height) {
    VivariumTerrainView sample;
    sample.object_id = id;
    sample.x = x;
    sample.z = z;
    sample.height = height;
    return sample;
}

/**
 * Two cells side by side over x 0..2 and z 0..1: the west cell dry above NN0, the east cell with its east
 * corners dug 0.25 m below it. Ids: 0 (0,0) 1 (1,0) 2 (2,0) 3 (0,1) 4 (1,1) 5 (2,1).
 */
std::vector<VivariumTerrainView> table_ground() {
    return {table_sample(0u, 0.0, 0.0, 1.0),  table_sample(1u, 1.0, 0.0, 0.5),
            table_sample(2u, 2.0, 0.0, -0.25), table_sample(3u, 0.0, 1.0, 1.0),
            table_sample(4u, 1.0, 1.0, 0.5),  table_sample(5u, 2.0, 1.0, -0.25)};
}

VivariumGroundCellView table_cell(uint32_t sw, uint32_t se, uint32_t ne, uint32_t nw) {
    VivariumGroundCellView cell;
    cell.object_id = sw;
    cell.sw = sw;
    cell.se = se;
    cell.ne = ne;
    cell.nw = nw;
    return cell;
}

/** The ground mesh build_ground lays over `samples` and `cells`, as the care view hands it to the table. */
PlantMeshArrays table_mesh(const std::vector<VivariumTerrainView>& samples,
                           const std::vector<VivariumGroundCellView>& cells) {
    PlantMeshArrays ground;
    std::vector<PlantGroundSlot> slots;
    (void)build_ground(samples, cells, ground, slots);
    return ground;
}

}  // namespace

namespace {

/**
 * A bed along z 0 falling east, its bank along z 1 one metre above it. Ids: 0 (0,0) 1 (1,0) 2 (2,0) bed at
 * 1, 0.5 and -0.25; 3 (0,1) 4 (1,1) 5 (2,1) bank at 2, 1.5 and 0.75.
 */
std::vector<VivariumTerrainView> bed_ground() {
    return {table_sample(0u, 0.0, 0.0, 1.0),  table_sample(1u, 1.0, 0.0, 0.5),
            table_sample(2u, 2.0, 0.0, -0.25), table_sample(3u, 0.0, 1.0, 2.0),
            table_sample(4u, 1.0, 1.0, 1.5),  table_sample(5u, 2.0, 1.0, 0.75)};
}

/** The brook ase-hydro reckons in that bed: x 0 to 2 along z 0, a quarter metre to either side, 0.2 m deep. */
VivariumStreamView bed_brook() {
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

/** The height of the water laid over the ground place (x, z), NaN where none lies. */
float water_at(const PlantMeshArrays& arrays, float x, float z) {
    for (std::size_t v = 0u; v < arrays.positions.size() / 3u; ++v) {
        if (arrays.positions[v * 3u] == doctest::Approx(x) && arrays.positions[v * 3u + 2u] == doctest::Approx(z)) {
            return arrays.positions[v * 3u + 1u];
        }
    }
    return std::nanf("");
}

}  // namespace

TEST_CASE("without a stream of ase-hydro the water stands at NN0 in a hollow below it and nowhere above it") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    // the west cell stands above NN0 and holds none; the east cell, its east corners dug below NN0, holds the table
    PlantMeshArrays arrays;
    REQUIRE(build_water_table({}, table_mesh(table_ground(), cells), arrays) == 1u);
    REQUIRE(arrays.positions.size() == 4u * 3u);
    for (std::size_t v = 0u; v < 4u; ++v) {
        CHECK(arrays.positions[v * 3u] >= 1.0f);
        CHECK(arrays.positions[v * 3u + 1u] == doctest::Approx(GODOT_WATER_TABLE_M));
    }
    // a dry stream holds no water of its own: its bed, dug below NN0 at its lower end, holds only the table
    VivariumStreamView dry = bed_brook();
    dry.depth_m = 0.0;
    PlantMeshArrays bed;
    REQUIRE(build_water_table({dry}, table_mesh(bed_ground(), cells), bed) == 1u);
    CHECK(water_at(bed, 2.0f, 0.0f) == doctest::Approx(GODOT_WATER_TABLE_M));
    CHECK(std::isnan(water_at(bed, 0.0f, 0.0f)));
}

TEST_CASE("the water stands in the bed at the bed plus the depth ase-hydro gives the stream") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    PlantMeshArrays arrays;
    REQUIRE(build_water_table({bed_brook()}, table_mesh(bed_ground(), cells), arrays) == 2u);
    REQUIRE(arrays.positions.size() == 6u * 3u);
    REQUIRE(arrays.indices.size() == 12u);
    // the vertices in the order the cells name them: (0,0) (1,0) (1,1) (0,1), then (2,0) (2,1)
    // the lower end, -0.25 plus 0.2, would stand below NN0: the brook stands at least at the table it runs in
    CHECK(arrays.positions[0u * 3u + 1u] == doctest::Approx(1.2f));
    CHECK(arrays.positions[1u * 3u + 1u] == doctest::Approx(0.7f));
    CHECK(arrays.positions[4u * 3u + 1u] == doctest::Approx(GODOT_WATER_TABLE_M));
    // a bank corner carries the lowest water of its cell's bed corners, not the table below it; its ground
    // stands above it
    CHECK(arrays.positions[2u * 3u + 1u] == doctest::Approx(0.7f));
    CHECK(arrays.positions[3u * 3u + 1u] == doctest::Approx(0.7f));
    CHECK(arrays.positions[5u * 3u + 1u] == doctest::Approx(GODOT_WATER_TABLE_M));
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
    REQUIRE(build_water_table({bed_brook()}, table_mesh(bed_ground(), cells), arrays) == 2u);
    CHECK(arrays.colors[0u * 4u + 1u] == doctest::Approx(0.2f / GODOT_WATER_DEPTH_FULL_M));
    // the lower end: the table at NN0 over the bed dug 0.25 m below it
    CHECK(arrays.colors[4u * 4u + 1u] == doctest::Approx((GODOT_WATER_TABLE_M + 0.25f) / GODOT_WATER_DEPTH_FULL_M));
    CHECK(arrays.colors[2u * 4u + 1u] == doctest::Approx(0.0f));
}

TEST_CASE("the water carries velocity and course of the stream whose bed holds the vertex") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    PlantMeshArrays arrays;
    REQUIRE(build_water_table({bed_brook()}, table_mesh(bed_ground(), cells), arrays) == 2u);
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
    CHECK(build_water_table({bed_brook()}, table_mesh(bed_ground(), {table_cell(1u, 2u, 99u, 4u)}), arrays) == 0u);
    CHECK(arrays.positions.empty());
    VivariumStreamView wide = bed_brook();
    wide.half_m = 1.0;
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    REQUIRE(build_water_table({wide}, table_mesh(table_ground(), cells), arrays) == 2u);
    CHECK(arrays.positions.size() == 6u * 3u);
    CHECK(arrays.indices.size() == 12u);
}

TEST_CASE("the line of the water runs station by station on the level the table lays its water at (B0285)") {
    const std::vector<VivariumGroundCellView> cells = {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u)};
    std::vector<float> line;
    // the bed corners fall into the stations 0, 16 and 32; the bank lies beyond the reach of the bed
    REQUIRE(build_water_line(bed_brook(), table_mesh(bed_ground(), cells), nullptr, nullptr, line) == 3u);
    REQUIRE(line.size() == 3u * 3u);
    // the same levels as the table: bed plus depth, 1.2 and 0.7, and NN0 at the end where bed plus depth stands
    // below it, over the bed at z 0
    CHECK(line[0] == doctest::Approx(0.0f));
    CHECK(line[1] == doctest::Approx(1.2f));
    CHECK(line[2] == doctest::Approx(0.0f));
    CHECK(line[3] == doctest::Approx(1.0f));
    CHECK(line[4] == doctest::Approx(0.7f));
    CHECK(line[5] == doctest::Approx(0.0f));
    CHECK(line[6] == doctest::Approx(2.0f));
    CHECK(line[7] == doctest::Approx(GODOT_WATER_TABLE_M));
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
    REQUIRE(build_water_line(bed_brook(), table_mesh(bed_ground(), cells), east, nullptr, line) == 2u);
    REQUIRE(line.size() == 2u * 3u);
    CHECK(line[0] == doctest::Approx(1.0f));
    CHECK(line[1] == doctest::Approx(0.7f));
    CHECK(line[3] == doctest::Approx(2.0f));
    CHECK(line[4] == doctest::Approx(GODOT_WATER_TABLE_M));
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

namespace {

/**
 * A flat bed along z 0 at height 1, its bank along z 1 at height 2, and a row along z 2 at height 2 with a pit
 * dug at (1,2) down to 1.3. Ids: 0 (0,0) 1 (1,0) 2 (2,0) bed; 3 (0,1) 4 (1,1) 5 (2,1) bank; 6 (0,2) 7 (1,2) pit
 * 8 (2,2).
 */
std::vector<VivariumTerrainView> pit_ground() {
    return {table_sample(0u, 0.0, 0.0, 1.0), table_sample(1u, 1.0, 0.0, 1.0), table_sample(2u, 2.0, 0.0, 1.0),
            table_sample(3u, 0.0, 1.0, 2.0), table_sample(4u, 1.0, 1.0, 2.0), table_sample(5u, 2.0, 1.0, 2.0),
            table_sample(6u, 0.0, 2.0, 2.0), table_sample(7u, 1.0, 2.0, 1.3), table_sample(8u, 2.0, 2.0, 2.0)};
}

/** The bed cells and the cells of the pit row, in the ground's order sw, se, ne, nw. */
std::vector<VivariumGroundCellView> pit_cells() {
    return {table_cell(0u, 1u, 4u, 3u), table_cell(1u, 2u, 5u, 4u), table_cell(3u, 4u, 7u, 6u),
            table_cell(4u, 5u, 8u, 7u)};
}

}  // namespace

TEST_CASE("a pit dug below the groundwater ase-hydro gives a brook fills to that level, beside the bed (B0286)") {
    VivariumStreamView stream = bed_brook();
    stream.groundwater_level_m = 1.5;
    stream.groundwater_reach_m = 2.5;
    PlantMeshArrays arrays;
    // the two bed cells and the two pit cells that name the pit corner below 1.5
    REQUIRE(build_water_table({stream}, table_mesh(pit_ground(), pit_cells()), arrays) == 4u);
    CHECK(water_at(arrays, 1.0f, 0.0f) == doctest::Approx(1.5f));  // the bed: at least the groundwater it runs in
    CHECK(water_at(arrays, 1.0f, 2.0f) == doctest::Approx(1.5f));  // the pit: the groundwater level
    CHECK(water_at(arrays, 0.0f, 2.0f) == doctest::Approx(1.5f));  // its rim: the same plane, the ground above hides it
    // the pit's water is still: no velocity, the course north, its depth over the pit floor in green
    for (std::size_t v = 0u; v < arrays.positions.size() / 3u; ++v) {
        if (arrays.positions[v * 3u] == doctest::Approx(1.0f) && arrays.positions[v * 3u + 2u] == doctest::Approx(2.0f)) {
            CHECK(arrays.colors[v * 4u] == doctest::Approx(0.0f));
            CHECK(arrays.colors[v * 4u + 1u] == doctest::Approx(0.2f / GODOT_WATER_DEPTH_FULL_M));
            CHECK(arrays.colors[v * 4u + 3u] == doctest::Approx(1.0f));
        }
    }
}

TEST_CASE("without groundwater on the stream port, beyond its reach or above its level a pit stays dry (B0286)") {
    // the port carries no groundwater yet (N0484): NaN in both fields, only the bed holds water
    VivariumStreamView none = bed_brook();
    none.groundwater_level_m = std::nan("");
    none.groundwater_reach_m = std::nan("");
    PlantMeshArrays arrays;
    CHECK(build_water_table({none}, table_mesh(pit_ground(), pit_cells()), arrays) == 2u);
    CHECK(std::isnan(water_at(arrays, 1.0f, 2.0f)));
    // the pit lies two metres off the bed's line, the reach one metre
    VivariumStreamView near = bed_brook();
    near.groundwater_level_m = 1.5;
    near.groundwater_reach_m = 1.0;
    PlantMeshArrays short_reach;
    CHECK(build_water_table({near}, table_mesh(pit_ground(), pit_cells()), short_reach) == 2u);
    CHECK(std::isnan(water_at(short_reach, 1.0f, 2.0f)));
    // the groundwater stands below the pit floor
    VivariumStreamView low = bed_brook();
    low.groundwater_level_m = 1.2;
    low.groundwater_reach_m = 2.5;
    PlantMeshArrays deep;
    CHECK(build_water_table({low}, table_mesh(pit_ground(), pit_cells()), deep) == 2u);
    CHECK(std::isnan(water_at(deep, 1.0f, 2.0f)));
}

}  // namespace ase::adp::godot

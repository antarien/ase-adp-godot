/**
 * ASE ADAPTER TESTS - GROUND OF THE PLANT VIEW WITHOUT GODOT
 *
 * @file        test_godot_plant_ground.cpp
 * @design      DSGN_066
 * @brief       Pins the ground mesh of the care view and the marks its shader paints by
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 and 03.11, the ground of the 3D view without a Godot
 *              type: the vertices and cells the view hands to ArrayMesh as the terrain and ground cell ports
 *              carried them (build_ground), and the vertex colours mark_ground lays over them - levelled
 *              ground of a patch's disc, the bed of a brook - which the ground shader turns into bare soil
 *              and wet gravel (GODOT_GROUND_SHADER_CODE). The rest of the plant view stands in
 *              test_godot_plant_view.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Application Layer Perception Control (ALPCTL)
 * @created     2026-10-25
 * @modified    2026-10-26
 * @version     00.00.01.00001
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>
#include <ase/math/scalar.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

TEST_CASE("03.11: the ground is taken as the ports carried it - every vertex, every cell as two upward triangles") {
    // a 3 x 3 area as the ports lay it: object id = row * 3 + column, one cell per south-west sample
    std::vector<VivariumTerrainView> samples;
    for (uint32_t row = 0u; row < 3u; ++row) {
        for (uint32_t column = 0u; column < 3u; ++column) {
            VivariumTerrainView sample;
            sample.object_id = row * 3u + column;
            sample.x = 9.0 + column;
            sample.z = 19.0 + row;
            sample.height = 0.1 * (row * 3u + column);
            sample.nx = 0.1 * column;  // a normal the view never computes: it only carries it
            sample.ny = 0.9;
            sample.nz = 0.1 * row;
            sample.u = sample.x;
            sample.v = sample.z;
            samples.push_back(sample);
        }
    }
    std::vector<VivariumGroundCellView> cells;
    for (uint32_t row = 0u; row < 2u; ++row) {
        for (uint32_t column = 0u; column < 2u; ++column) {
            VivariumGroundCellView cell;
            cell.object_id = row * 3u + column;
            cell.sw = row * 3u + column;
            cell.se = row * 3u + column + 1u;
            cell.ne = (row + 1u) * 3u + column + 1u;
            cell.nw = (row + 1u) * 3u + column;
            cells.push_back(cell);
        }
    }
    PlantMeshArrays mesh;
    CHECK(build_ground(samples, cells, mesh) == 4u);
    REQUIRE(mesh.positions.size() == 9u * 3u);
    REQUIRE(mesh.normals.size() == 9u * 3u);
    REQUIRE(mesh.uvs.size() == 9u * 2u);
    REQUIRE(mesh.indices.size() == 4u * 6u);
    // place, normal and texture coordinates exactly as the ports carried them, in port order
    for (std::size_t v = 0u; v < samples.size(); ++v) {
        CHECK(mesh.positions[v * 3u] == doctest::Approx(samples[v].x));
        CHECK(mesh.positions[v * 3u + 1u] == doctest::Approx(samples[v].height));
        CHECK(mesh.positions[v * 3u + 2u] == doctest::Approx(samples[v].z));
        CHECK(mesh.normals[v * 3u] == doctest::Approx(samples[v].nx));
        CHECK(mesh.normals[v * 3u + 1u] == doctest::Approx(samples[v].ny));
        CHECK(mesh.normals[v * 3u + 2u] == doctest::Approx(samples[v].nz));
        CHECK(mesh.uvs[v * 2u] == doctest::Approx(samples[v].u));
        CHECK(mesh.uvs[v * 2u + 1u] == doctest::Approx(samples[v].v));
    }
    // the first cell: (sw, se, ne) and (sw, ne, nw) over the vertices its corners name
    CHECK(mesh.indices[0] == 0);
    CHECK(mesh.indices[1] == 1);
    CHECK(mesh.indices[2] == 4);
    CHECK(mesh.indices[3] == 0);
    CHECK(mesh.indices[4] == 4);
    CHECK(mesh.indices[5] == 3);
    // Godot's front face looks up: the edge cross product of every triangle points down into the ground
    for (std::size_t tri = 0u; tri < mesh.indices.size() / 3u; ++tri) {
        const float* p = mesh.positions.data();
        const int32_t i0 = mesh.indices[tri * 3u];
        const int32_t i1 = mesh.indices[tri * 3u + 1u];
        const int32_t i2 = mesh.indices[tri * 3u + 2u];
        const double e1x = p[i1 * 3] - p[i0 * 3];
        const double e1z = p[i1 * 3 + 2] - p[i0 * 3 + 2];
        const double e2x = p[i2 * 3] - p[i0 * 3];
        const double e2z = p[i2 * 3 + 2] - p[i0 * 3 + 2];
        CHECK(e1z * e2x - e1x * e2z < 0.0);
    }

    // a corner the terrain port did not carry leaves its cells undrawn; a sample named twice is drawn once
    samples.erase(samples.begin());  // the south-west corner, a corner of one cell only
    samples.push_back(samples.front());
    PlantMeshArrays holed;
    CHECK(build_ground(samples, cells, holed) == 3u);
    CHECK(holed.positions.size() == 8u * 3u);
    // no cells, no triangles; no samples, nothing at all
    PlantMeshArrays bare;
    CHECK(build_ground(samples, {}, bare) == 0u);
    CHECK(bare.indices.empty());
    PlantMeshArrays none;
    CHECK(build_ground({}, cells, none) == 0u);
    CHECK(none.positions.empty());
}

TEST_CASE("03.8: the ground marks levelled vertices and the bed of a brook for the ground shader") {
    // five vertices: a levelled one in the disc, one in the disc not at its height, one outside the disc
    // at its height, one in the bed, one far from both
    PlantMeshArrays mesh;
    mesh.positions = {16.0f, 4.885f, 16.0f, 16.5f, 5.2f, 16.0f, 21.0f, 4.885f, 16.0f,
                      10.0f, 4.6f,   24.3f, 10.0f, 4.9f, 27.0f};
    const std::vector<PlantGroundDisc> discs = {PlantGroundDisc{16.0f, 16.0f, 4.0f, 4.885f}};
    const std::vector<PlantGroundBed> beds = {PlantGroundBed{4.0f, 24.0f, 28.0f, 24.0f, 0.6f}};
    CHECK(mark_ground(discs, beds, mesh) == 2u);
    REQUIRE(mesh.colors.size() == 5u * 4u);
    // red: levelled only inside the disc AND at its height - a disc not levelled yet stays grass
    CHECK(mesh.colors[0] == doctest::Approx(1.0f));
    CHECK(mesh.colors[4] == doctest::Approx(0.0f));
    CHECK(mesh.colors[8] == doctest::Approx(0.0f));
    // green: in the bed, no farther from the line between its ends than half its width
    CHECK(mesh.colors[13] == doctest::Approx(1.0f));
    CHECK(mesh.colors[17] == doctest::Approx(0.0f));
    CHECK(mesh.colors[1] == doctest::Approx(0.0f));
    // every vertex opaque, blue unused
    for (std::size_t v = 0u; v < 5u; ++v) {
        CHECK(mesh.colors[v * 4u + 2u] == doctest::Approx(0.0f));
        CHECK(mesh.colors[v * 4u + 3u] == doctest::Approx(1.0f));
    }
    // a bed whose two ends coincide is a disc of half its width; no discs and no beds mark nothing
    const std::vector<PlantGroundBed> point = {PlantGroundBed{10.0f, 27.0f, 10.0f, 27.0f, 0.6f}};
    CHECK(mark_ground({}, point, mesh) == 1u);
    CHECK(mesh.colors[17] == doctest::Approx(1.0f));
    CHECK(mark_ground({}, {}, mesh) == 0u);
    // a ground without vertices carries no colours
    PlantMeshArrays empty;
    CHECK(mark_ground(discs, beds, empty) == 0u);
    CHECK(empty.colors.empty());
}

TEST_CASE("03.8: the ground shader takes slope, marks and grid from its uniforms, never a number of its own") {
    // every uniform the look binds is declared in the shader, so a renamed key fails here, not in the window
    const std::string code = GODOT_GROUND_SHADER_CODE;
    for (const char* uniform : {GODOT_GROUND_COVER_COLOR, GODOT_GROUND_COVER_ALBEDO, GODOT_GROUND_COVER_UV,
                                GODOT_GROUND_SOIL, GODOT_GROUND_BANK, GODOT_GROUND_BED, GODOT_GROUND_GRID,
                                GODOT_GROUND_SLOPE_BANK, GODOT_GROUND_SLOPE_STEEP, GODOT_GROUND_GRID_STEP,
                                GODOT_GROUND_GRID_MAJOR_KEY, GODOT_GROUND_CONTOUR, GODOT_GROUND_GRID_WEIGHT,
                                GODOT_GROUND_MAJOR_WEIGHT, GODOT_GROUND_CONTOUR_WEIGHT, GODOT_GROUND_WET,
                                GODOT_GROUND_ROUGH, GODOT_SHADER_SOIL_REL}) {
        CHECK_MESSAGE(code.find(std::string(" ") + uniform + " ") != std::string::npos, uniform);
    }
    // the slope shares are 1 - cos of their angles, so the two numbers of types.hpp cannot drift apart
    const float radians_per_degree = ase::math::PI / 180.0f;
    CHECK(GODOT_GROUND_SLOPE_BANK_SHARE ==
          doctest::Approx(1.0f - ase::math::cos(GODOT_GROUND_SLOPE_BANK_DEG * radians_per_degree)).epsilon(0.001));
    CHECK(GODOT_GROUND_SLOPE_STEEP_SHARE ==
          doctest::Approx(1.0f - ase::math::cos(GODOT_GROUND_SLOPE_STEEP_DEG * radians_per_degree)).epsilon(0.001));
    CHECK(GODOT_GROUND_SLOPE_BANK_SHARE < GODOT_GROUND_SLOPE_STEEP_SHARE);
}

}  // namespace ase::adp::godot

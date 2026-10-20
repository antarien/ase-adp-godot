/**
 * ASE ADAPTER TESTS - PLANT VIEW WITHOUT GODOT
 *
 * @file        test_godot_plant_view.cpp
 * @design      DSGN_024
 * @brief       Pins the plant geometry, the visible orders by frame time and the asset binding
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1, 03.2 and 03.11 step 2, the parts of the 3D view
 *              that carry no Godot type: the templates the view hands to ArrayMesh as the template ports
 *              carried them (usecase:101011 lay out organ templates), the instance records it hands to
 *              MultiMesh unchanged, sorted by body and by the line between fine and coarse cylinder the
 *              frame time moves, the stem base taken from the patch record, and the binding of the asset
 *              names against a test manifest - a bound name, a name without an asset, a file whose
 *              checksum does not match, a name the manifest carries twice - and the error material a
 *              missing name is drawn with. The return to the garden, the selected patch and the sown seed
 *              stand in test_godot_keeper_flow.cpp.
 *
 *              THE WINDING IS THE PORT'S: ase-render-data lays every template triangle clockwise seen from
 *              outside, Godot's front face, and pins that in its own test (test_render_data.cpp). The
 *              view keeps the order a triangle record names - the case below checks exactly that.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-18
 * @version     00.00.06.00004
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/godot_order_budget.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kPatch = 7u;          // object id of the patch the test plant stands on
constexpr uint32_t kOtherPatch = 8u;     // a patch the view does not place

/** An organ record as its port lays it: the organ, the patch its plant stands on, the order of its axis. */
VivariumOrganView organ(uint32_t object_id, uint32_t body, double order) {
    VivariumOrganView view;
    view.object_id = object_id;
    view.body = body;
    view.order = order;
    return view;
}

/** The instance record of that organ: twelve values no other organ of the test carries. */
VivariumInstanceView instance(uint32_t object_id) {
    VivariumInstanceView view;
    view.object_id = object_id;
    for (uint32_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
        view.rows[v] = static_cast<float>(object_id) + 0.01f * static_cast<float>(v);
    }
    return view;
}

/** Checks that `rows` carries the twelve values of `record` from record slot `slot` on, each exactly. */
void check_carries(const std::vector<float>& rows, std::size_t slot, const VivariumInstanceView& record) {
    REQUIRE(rows.size() >= (slot + 1u) * GODOT_INSTANCE_VALUES);
    for (uint32_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
        CHECK(rows[slot * GODOT_INSTANCE_VALUES + v] == record.rows[v]);
    }
}

/** A corner record of template `code` as the corner port lays it: values no other corner of the test carries. */
VivariumTemplateCornerView corner(uint32_t code, uint32_t index) {
    VivariumTemplateCornerView view;
    view.object_id = code * 100u + index;
    view.code = code;
    view.index = index;
    view.x = 1.0 + index;
    view.y = 2.0 + index;
    view.z = 3.0 + index;
    view.nx = 0.1 * index;  // a normal the view never computes: it only carries it
    view.ny = 0.2 * index;
    view.nz = 0.3 * index;
    view.u = 0.01 * index;
    view.v = 0.02 * index;
    return view;
}

/** A triangle record of template `code` over three corner indices, in the order the record names them. */
VivariumTemplateTriangleView triangle(uint32_t code, uint32_t a, uint32_t b, uint32_t c) {
    VivariumTemplateTriangleView view;
    view.code = code;
    view.a = a;
    view.b = b;
    view.c = c;
    return view;
}

// The test's VivariumHashFn: the checksum of a file is what the map says, "" for a file it lacks.
std::string hash_from_map(const std::string& file, void* user) {
    const auto& sums = *static_cast<const std::unordered_map<std::string, std::string>*>(user);
    const auto found = sums.find(file);
    return found == sums.end() ? std::string() : found->second;
}

const std::string kSumA(64u, 'a');
const std::string kSumB(64u, 'b');

}  // namespace

TEST_CASE("03.11: the instance records go unchanged to the body and the side of the split their organ record names") {
    const std::vector<VivariumOrganView> organs = {
        organ(11u, kPatch, 0.0),       // the stem: up to the split
        organ(12u, kPatch, 3.0),       // a twig of order 3: above it
        organ(13u, kPatch, 2.0),       // order 2, the split itself: up to it
        organ(21u, kOtherPatch, 1.0)}; // the plant on the other patch
    const std::vector<VivariumInstanceView> instances = {instance(11u), instance(12u), instance(13u), instance(21u),
                                                         instance(99u)};  // 99 has no organ record
    const auto split = rows_by_body(instances, organs, 2.0);
    REQUIRE(split.size() == 2u);
    // the twelve values exactly as the port carried them, record after record in port order
    const PlantBodyRows& plant = split.at(kPatch);
    REQUIRE(plant.up_to.size() == 2u * GODOT_INSTANCE_VALUES);
    REQUIRE(plant.above.size() == GODOT_INSTANCE_VALUES);
    check_carries(plant.up_to, 0u, instance(11u));
    check_carries(plant.up_to, 1u, instance(13u));
    check_carries(plant.above, 0u, instance(12u));
    const PlantBodyRows& other = split.at(kOtherPatch);
    check_carries(other.up_to, 0u, instance(21u));
    CHECK(other.above.empty());

    // a class the order budget does not split: every organ lies above GODOT_ORDER_EVERY, the stem too
    const auto every = rows_by_body(instances, organs, GODOT_ORDER_EVERY);
    CHECK(every.at(kPatch).up_to.empty());
    REQUIRE(every.at(kPatch).above.size() == 3u * GODOT_INSTANCE_VALUES);
    check_carries(every.at(kPatch).above, 0u, instance(11u));
    check_carries(every.at(kPatch).above, 2u, instance(13u));

    // no records, no plants; records without organ records, no plants either
    CHECK(rows_by_body({}, organs, 2.0).empty());
    CHECK(rows_by_body(instances, {}, 2.0).empty());
}

TEST_CASE("usecase:100395 age the bark: every instance carries the bark share of its organ as custom data, slot for slot") {
    VivariumOrganView stem = organ(11u, kPatch, 0.0);
    stem.bark = 0.75;  // the stem base, mostly stem-base bark
    VivariumOrganView band = organ(13u, kPatch, 0.0);
    band.bark = 0.25;  // the band towards the white bark
    const VivariumOrganView twig = organ(12u, kPatch, 3.0);  // a branch: white bark
    const auto split = rows_by_body({instance(11u), instance(12u), instance(13u)}, {stem, twig, band}, 2.0);
    const PlantBodyRows& plant = split.at(kPatch);

    // one custom slot per record, beside it in the same order: the share first, then the tiling - here none
    REQUIRE(plant.up_to_custom.size() == 2u * GODOT_INSTANCE_CUSTOM_VALUES);
    REQUIRE(plant.above_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    CHECK(plant.up_to_custom[0] == doctest::Approx(0.75));
    CHECK(plant.up_to_custom[GODOT_INSTANCE_CUSTOM_VALUES] == doctest::Approx(0.25));
    CHECK(plant.above_custom[0] == doctest::Approx(GODOT_BARK_WHITE));
    for (uint32_t v = 1u; v < GODOT_INSTANCE_CUSTOM_VALUES; ++v) {
        CHECK(plant.up_to_custom[v] == doctest::Approx(GODOT_TILE_NONE));
        CHECK(plant.up_to_custom[GODOT_INSTANCE_CUSTOM_VALUES + v] == doctest::Approx(GODOT_TILE_NONE));
        CHECK(plant.above_custom[v] == doctest::Approx(GODOT_TILE_NONE));
    }
    // the records themselves stay as the port carried them
    check_carries(plant.up_to, 0u, instance(11u));
    check_carries(plant.up_to, 1u, instance(13u));
    check_carries(plant.above, 0u, instance(12u));
}

TEST_CASE("03.11 step 2: every instance carries the bark tiling of its section after the bark share, unchanged") {
    VivariumOrganView stem = organ(11u, kPatch, 0.0);
    stem.bark = 0.5;
    stem.tile_around = 2.5;   // tiles of the white bark around the tube
    stem.tile_along = 0.75;   // tiles of the white bark along the tube
    stem.tile_base = 0.4;     // tiles of the stem-base bark per white tile
    const auto split = rows_by_body({instance(11u)}, {stem}, 2.0);
    const PlantBodyRows& plant = split.at(kPatch);

    REQUIRE(plant.up_to_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    CHECK(plant.up_to_custom[0] == doctest::Approx(0.5));
    CHECK(plant.up_to_custom[1] == doctest::Approx(2.5));
    CHECK(plant.up_to_custom[2] == doctest::Approx(0.75));
    CHECK(plant.up_to_custom[3] == doctest::Approx(0.4));
}

TEST_CASE("usecase:101011 lay out organ templates: a template is built from the records of its code, as the ports carried them") {
    // the fine cylinder's first quad and one corner of the coarse one, laid out of index order
    const std::vector<VivariumTemplateCornerView> corners = {corner(GODOT_TMPL_FINE, 2u), corner(GODOT_TMPL_FINE, 0u),
                                                             corner(GODOT_TMPL_CORS, 0u), corner(GODOT_TMPL_FINE, 1u),
                                                             corner(GODOT_TMPL_FINE, 3u)};
    const std::vector<VivariumTemplateTriangleView> triangles = {
        triangle(GODOT_TMPL_FINE, 0u, 2u, 3u), triangle(GODOT_TMPL_CORS, 0u, 0u, 0u),
        triangle(GODOT_TMPL_FINE, 0u, 3u, 1u)};
    PlantMeshArrays fine;
    CHECK(build_template(corners, triangles, GODOT_TMPL_FINE, fine) == 2u);
    // one vertex per corner of the code, in port order - the coarse corner is not among them
    REQUIRE(fine.positions.size() == 4u * 3u);
    REQUIRE(fine.normals.size() == 4u * 3u);
    REQUIRE(fine.uvs.size() == 4u * 2u);
    const uint32_t port_order[4] = {2u, 0u, 1u, 3u};
    for (std::size_t v = 0u; v < 4u; ++v) {
        const VivariumTemplateCornerView carried = corner(GODOT_TMPL_FINE, port_order[v]);
        CHECK(fine.positions[v * 3u] == doctest::Approx(carried.x));
        CHECK(fine.positions[v * 3u + 1u] == doctest::Approx(carried.y));
        CHECK(fine.positions[v * 3u + 2u] == doctest::Approx(carried.z));
        CHECK(fine.normals[v * 3u] == doctest::Approx(carried.nx));
        CHECK(fine.normals[v * 3u + 1u] == doctest::Approx(carried.ny));
        CHECK(fine.normals[v * 3u + 2u] == doctest::Approx(carried.nz));
        CHECK(fine.uvs[v * 2u] == doctest::Approx(carried.u));
        CHECK(fine.uvs[v * 2u + 1u] == doctest::Approx(carried.v));
    }
    // every triangle over the vertices its corner indices name, in the order the record names them
    REQUIRE(fine.indices.size() == 2u * 3u);
    CHECK(fine.indices[0] == 1);  // corner 0 is the second vertex
    CHECK(fine.indices[1] == 0);  // corner 2 the first
    CHECK(fine.indices[2] == 3);  // corner 3 the fourth
    CHECK(fine.indices[3] == 1);
    CHECK(fine.indices[4] == 3);
    CHECK(fine.indices[5] == 2);  // corner 1 the third

    // the coarse template takes only its own records; a degenerate record still names known corners
    PlantMeshArrays coarse;
    CHECK(build_template(corners, triangles, GODOT_TMPL_CORS, coarse) == 1u);
    CHECK(coarse.positions.size() == 3u);
    // a triangle whose corner the port did not carry is not drawn; an index named twice is drawn once
    std::vector<VivariumTemplateCornerView> holed = corners;
    holed.erase(holed.begin());  // fine corner 2, a corner of the first triangle only
    holed.push_back(corner(GODOT_TMPL_FINE, 0u));
    PlantMeshArrays partial;
    CHECK(build_template(holed, triangles, GODOT_TMPL_FINE, partial) == 1u);
    CHECK(partial.positions.size() == 3u * 3u);
    // a code the ports carry no records for, no corners, no triangles: nothing at all
    PlantMeshArrays leaf;
    CHECK(build_template(corners, triangles, GODOT_TMPL_LEAF, leaf) == 0u);
    CHECK(leaf.positions.empty());
    PlantMeshArrays none;
    CHECK(build_template({}, triangles, GODOT_TMPL_FINE, none) == 0u);
    CHECK(none.indices.empty());
    PlantMeshArrays bare;
    CHECK(build_template(corners, {}, GODOT_TMPL_FINE, bare) == 0u);
    CHECK(bare.indices.empty());
}

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

TEST_CASE("03.11: the stem base of a patch is taken from its record - centre and levelled height") {
    // a levelled patch: its record carries x and z from the lay request and y from ase-terrain
    VivariumPatchView patch;
    patch.object_id = kPatch;
    patch.place = 2u;
    patch.x = 1.1;
    patch.y = 0.35;
    patch.z = -0.9;
    // a patch whose disc is not levelled yet: the record carries no y, read_snapshot leaves it NaN
    VivariumPatchView unlevelled = patch;
    unlevelled.object_id = kOtherPatch;
    unlevelled.place = 3u;
    unlevelled.y = std::nan("");
    // a patch of a record without a place in the world: neither x nor z
    VivariumPatchView unplaced = patch;
    unplaced.object_id = 99u;
    unplaced.place = 4u;
    unplaced.x = std::nan("");
    unplaced.z = std::nan("");
    const auto origins = plant_origins({patch, unlevelled, unplaced});
    REQUIRE(origins.size() == 1u);
    const PlantOrigin& base = origins.at(kPatch);
    // the three numbers as the record carried them, narrowed to the float of the scene and nothing else
    CHECK(base.x == static_cast<float>(1.1));
    CHECK(base.y == static_cast<float>(0.35));
    CHECK(base.z == static_cast<float>(-0.9));
    // no record, no origin
    CHECK(plant_origins({}).empty());
}

TEST_CASE("03.2: soil water and condition of a patch reach its materials as the record carried them") {
    // a measured stand: soil water over field capacity and a wilted condition
    VivariumPatchView patch;
    patch.object_id = kPatch;
    patch.soil_rel = 0.62;
    patch.condition = GODOT_COND_WILTED;
    // a stand before the first runs of the modules: no soil_rel (NaN), a condition outside the codes
    VivariumPatchView unmeasured = patch;
    unmeasured.object_id = kOtherPatch;
    unmeasured.soil_rel = std::nan("");
    unmeasured.condition = GODOT_CODE_UNKNOWN;
    const auto stands = plant_stands({patch, unmeasured});
    REQUIRE(stands.size() == 2u);
    // as the record carried them, narrowed to the float of a shader parameter and nothing else
    const PlantStand& wilted = stands.at(kPatch);
    CHECK(wilted.soil_known);
    CHECK(wilted.soil_rel == static_cast<float>(0.62));
    CHECK(wilted.condition_known);
    CHECK(wilted.condition == static_cast<float>(GODOT_COND_WILTED));
    // what the record did not carry stays unknown: the shader's default draws, never a value made up
    const PlantStand& open = stands.at(kOtherPatch);
    CHECK_FALSE(open.soil_known);
    CHECK_FALSE(open.condition_known);
    // every documented code passes, the first one past them does not
    for (uint8_t code = 0u; code <= GODOT_COND_COUNT; ++code) {
        VivariumPatchView coded = patch;
        coded.condition = code;
        CHECK(plant_stands({coded}).at(kPatch).condition_known == (code < GODOT_COND_COUNT));
    }
    // a default stand is an unknown one - the ground of a place without a record
    const PlantStand none;
    CHECK_FALSE(none.soil_known);
    CHECK_FALSE(none.condition_known);
    CHECK(plant_stands({}).empty());
}

TEST_CASE("the visible orders follow the frame time: a lasting load takes one back, calm gives one more") {
    OrderBudget budget;
    budget.frame_budget_ms = 16.0;
    budget.headroom = 0.75;
    budget.ceiling = 3u;
    budget.calm_frames = 4u;
    budget.over_frames = 2u;
    budget.shown = 1u;
    REQUIRE(order_budget_valid(budget));

    // calm frames, below 12 ms: the fourth gives one order more, then three more frames and one more
    for (uint32_t frame = 0u; frame < 3u; ++frame) {
        CHECK_FALSE(order_budget_sample(budget, 8.0));
    }
    CHECK(order_budget_sample(budget, 8.0));
    CHECK(budget.shown == 2u);
    // a frame between headroom and budget breaks the calm run
    CHECK_FALSE(order_budget_sample(budget, 14.0));
    CHECK(budget.calm_run == 0u);
    // one spike moves nothing; two over frames in a row take one order back
    CHECK_FALSE(order_budget_sample(budget, 30.0));
    CHECK_FALSE(order_budget_sample(budget, 8.0));
    CHECK_FALSE(order_budget_sample(budget, 30.0));
    CHECK(order_budget_sample(budget, 30.0));
    CHECK(budget.shown == 1u);
    // a time that is no number moves nothing
    CHECK_FALSE(order_budget_sample(budget, std::nan("")));
    CHECK_FALSE(order_budget_sample(budget, -1.0));
    // never past the ceiling, never below zero
    for (uint32_t frame = 0u; frame < 40u; ++frame) {
        (void)order_budget_sample(budget, 1.0);
    }
    CHECK(budget.shown == budget.ceiling);
    for (uint32_t frame = 0u; frame < 40u; ++frame) {
        (void)order_budget_sample(budget, 100.0);
    }
    CHECK(budget.shown == 0u);

    OrderBudget broken = budget;
    broken.headroom = 1.0;
    CHECK_FALSE(order_budget_valid(broken));
    broken = budget;
    broken.shown = broken.ceiling + 1u;
    CHECK_FALSE(order_budget_valid(broken));
}

TEST_CASE("asset binding: a name binds only to a manifest file whose checksum matches, every other is missing") {
    const std::vector<VivariumAssetEntry> manifest = {
        {"birch_leaf", "birch/leaf.glb", kSumA},
        {"birch_bark_white", "birch/bark_white.tres", kSumB},  // its file does not match
        {"ground_meadow", "ground/meadow_1.tres", kSumA},
        {"ground_meadow", "ground/meadow_2.tres", kSumA},      // named twice: ambiguous, missing
    };
    std::unordered_map<std::string, std::string> sums = {
        {"birch/leaf.glb", std::string(64u, 'A')},             // the same checksum in capitals
        {"birch/bark_white.tres", kSumA},
        {"ground/meadow_1.tres", kSumA},
        {"ground/meadow_2.tres", kSumA}};
    const std::vector<std::string> names = {"birch_leaf", "birch_bark_white", "ground_meadow", "birch_bud",
                                            "birch_leaf"};
    const VivariumAssetBinding binding = bind_assets(names, manifest, &hash_from_map, &sums);
    REQUIRE(binding.files.size() == 1u);
    CHECK(binding.files.at("birch_leaf") == "birch/leaf.glb");
    REQUIRE(binding.missing.size() == 3u);
    CHECK(binding.missing[0] == "birch_bark_white");
    CHECK(binding.missing[1] == "ground_meadow");
    CHECK(binding.missing[2] == "birch_bud");
    CHECK(asset_missing_line("birch_bud") == "asset missing birch_bud");

    // 03.2 point 4: the bound name is drawn as the view draws it, the missing one in the error
    // material - the same magenta for every missing name, an unknown and an empty one included
    CHECK(asset_color(binding, "birch_leaf", GODOT_COLOR_SOIL) == GODOT_COLOR_SOIL);
    CHECK(asset_color(binding, "ground_meadow", GODOT_COLOR_SOIL) == GODOT_COLOR_ASSET_MISSING);
    CHECK(asset_color(binding, "birch_bud", GODOT_COLOR_GROW) == GODOT_COLOR_ASSET_MISSING);
    CHECK(asset_color(binding, std::string(), GODOT_COLOR_SOIL) == GODOT_COLOR_ASSET_MISSING);
    CHECK(GODOT_COLOR_ASSET_MISSING != GODOT_COLOR_SOIL);

    // no asset set delivered yet (Phase 04): every name is missing, none is bound silently
    const VivariumAssetBinding nothing = bind_assets(names, {}, &hash_from_map, &sums);
    CHECK(nothing.files.empty());
    CHECK(nothing.missing.size() == 4u);
    CHECK(asset_color(nothing, "birch_leaf", GODOT_COLOR_SOIL) == GODOT_COLOR_ASSET_MISSING);
}

}  // namespace ase::adp::godot

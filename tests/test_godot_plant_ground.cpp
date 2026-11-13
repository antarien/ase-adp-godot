/**
 * ASE ADAPTER TESTS - GROUND OF THE PLANT VIEW WITHOUT GODOT
 *
 * @file        test_godot_plant_ground.cpp
 * @design      DSGN_066
 * @brief       Pins the ground mesh of the care view, the marks its shader paints by, and its patch
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 and 03.11, the ground of the 3D view without a Godot
 *              type: the vertices and cells the view hands to ArrayMesh as the terrain and ground cell ports
 *              carried them (build_ground), and the vertex colours mark_ground lays over them - levelled
 *              ground of a patch's disc, the bed of a brook - which the ground shader turns into bare soil
 *              and wet gravel (GODOT_GROUND_SHADER_FILE). The shader is read from its file in the tree
 *              (shaders/plant_ground.gdshader, beside this tests/ folder) through ase-fileio, as the stage
 *              copies it. P0690: the bytes of Godot's surface code (encode_ground, held against hand-coded
 *              bytes), the positive control (ground_code_holds) and the patch after a dig (patch_ground),
 *              which writes only the moved vertices and the changed marks - at normal stride 4 and at the
 *              stride 8 Godot 4.5 lays when it derives a tangent from each normal. The rest of the plant view
 *              stands in test_godot_plant_view.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-10-25
 * @modified    2026-11-13
 * @version     00.00.03.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/godot_plant_look_shader.hpp>
#include <ase/adp/godot/godot_terrain_samples.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/math/math.hpp>
#include <ase/math/scalar.hpp>
#include <ase/utils/strops.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

namespace {

/** A 3 x 3 area as the ports lay it: object id = row * 3 + column, flat at height 0.1 * id, normal up. */
std::vector<VivariumTerrainView> area_samples() {
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
    return samples;
}

/** The four cells of that area, one per south-west sample. */
std::vector<VivariumGroundCellView> area_cells() {
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
    return cells;
}

/**
 * The surface of that area as Godot lays an uncompressed one: the places one after the other (12 bytes each),
 * the normals behind them (`normal_stride` bytes each: 4 the normal alone, 8 with the tangent Godot 4.5 derives
 * from it), colour and texture coordinates in the attribute stream (12 bytes each). Every vertex encoded, so the
 * bytes stand as Godot would have made them.
 */
PlantGroundSurface area_surface(const std::vector<VivariumTerrainView>& samples,
                                const std::vector<VivariumGroundCellView>& cells, uint32_t normal_stride = 4u) {
    PlantGroundSurface surface;
    (void)build_ground(samples, cells, surface.arrays, surface.slots);
    (void)mark_ground({}, {}, surface.arrays);
    surface.cells = cells;
    const auto vertices = static_cast<uint32_t>(surface.arrays.positions.size() / 3u);
    surface.layout = PlantGroundLayout{0u, 12u, vertices * 12u, normal_stride, 0u, 4u, 12u};
    surface.vertex_bytes.assign(static_cast<std::size_t>(vertices) * (12u + normal_stride), 0u);
    surface.attribute_bytes.assign(static_cast<std::size_t>(vertices) * 12u, 0u);
    for (uint32_t v = 0u; v < vertices; ++v) {
        (void)encode_ground(surface, v);
    }
    surface.low_m = 0.0f;
    surface.high_m = 0.8f;
    surface.patchable = ground_code_holds(surface);
    return surface;
}

}  // namespace

TEST_CASE("03.11: the ground is taken as the ports carried it - every vertex, every cell as two upward triangles") {
    std::vector<VivariumTerrainView> samples = area_samples();
    const std::vector<VivariumGroundCellView> cells = area_cells();
    PlantMeshArrays mesh;
    std::vector<PlantGroundSlot> slots;
    CHECK(build_ground(samples, cells, mesh, slots) == 4u);
    REQUIRE(mesh.positions.size() == 9u * 3u);
    REQUIRE(mesh.normals.size() == 9u * 3u);
    REQUIRE(mesh.uvs.size() == 9u * 2u);
    REQUIRE(mesh.indices.size() == 4u * 6u);
    // one slot per sample in port order, each the vertex of its sample
    REQUIRE(slots.size() == 9u);
    for (std::size_t v = 0u; v < slots.size(); ++v) {
        CHECK(slots[v].object_id == samples[v].object_id);
        CHECK(slots[v].vertex == static_cast<int32_t>(v));
    }
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
    std::vector<PlantGroundSlot> holed_slots;
    CHECK(build_ground(samples, cells, holed, holed_slots) == 3u);
    CHECK(holed.positions.size() == 8u * 3u);
    REQUIRE(holed_slots.size() == 9u);
    CHECK(holed_slots.back().object_id == samples.front().object_id);
    CHECK(holed_slots.back().vertex == -1);
    // no cells, no triangles; no samples, nothing at all
    PlantMeshArrays bare;
    std::vector<PlantGroundSlot> bare_slots;
    CHECK(build_ground(samples, {}, bare, bare_slots) == 0u);
    CHECK(bare.indices.empty());
    PlantMeshArrays none;
    std::vector<PlantGroundSlot> none_slots;
    CHECK(build_ground({}, cells, none, none_slots) == 0u);
    CHECK(none.positions.empty());
    CHECK(none_slots.empty());
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

TEST_CASE("P0690: a vertex is written in the code Godot keeps an uncompressed surface in") {
    // one vertex: place (1, 2, -3), the normal straight up, then straight down; colour red; uv (0.5, 0.25)
    PlantGroundSurface surface;
    surface.arrays.positions = {1.0f, 2.0f, -3.0f};
    surface.arrays.normals = {0.0f, 1.0f, 0.0f};
    surface.arrays.colors = {1.0f, 0.0f, 0.0f, 1.0f};
    surface.arrays.uvs = {0.5f, 0.25f};
    surface.layout = PlantGroundLayout{0u, 12u, 12u, 4u, 0u, 4u, 12u};
    surface.vertex_bytes.assign(16u, 0u);
    surface.attribute_bytes.assign(12u, 0u);
    REQUIRE(encode_ground(surface, 0u));
    // the place as the four bytes of each float, the lowest first: 1.0f = 0x3f800000, -3.0f = 0xc0400000
    const std::vector<uint8_t> place = {0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0xc0};
    for (std::size_t i = 0u; i < place.size(); ++i) {
        CHECK(surface.vertex_bytes[i] == place[i]);
    }
    // straight up: octahedral (0, 1) → (0.5, 1.0) → 32767 (0.5 * 65535 cut), 65535
    CHECK(surface.vertex_bytes[12] == 0xff);
    CHECK(surface.vertex_bytes[13] == 0x7f);
    CHECK(surface.vertex_bytes[14] == 0xff);
    CHECK(surface.vertex_bytes[15] == 0xff);
    // colour as four uint8, uv as two floats behind it: 0.5f = 0x3f000000, 0.25f = 0x3e800000
    const std::vector<uint8_t> attribute = {0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x80, 0x3e};
    for (std::size_t i = 0u; i < attribute.size(); ++i) {
        CHECK(surface.attribute_bytes[i] == attribute[i]);
    }
    // straight down folds the lower half over: (0, 0, -1) → (1, 1) → 65535, 65535
    surface.arrays.normals = {0.0f, 0.0f, -1.0f};
    REQUIRE(encode_ground(surface, 0u));
    CHECK(surface.vertex_bytes[12] == 0xff);
    CHECK(surface.vertex_bytes[13] == 0xff);
    CHECK(surface.vertex_bytes[14] == 0xff);
    CHECK(surface.vertex_bytes[15] == 0xff);
    // a vertex past the bytes writes nothing
    CHECK_FALSE(encode_ground(surface, 1u));
    surface.vertex_bytes.resize(15u);
    CHECK_FALSE(encode_ground(surface, 0u));
}

TEST_CASE("P0690: at normal stride 8 the tangent Godot 4.5 derives from the normal is written behind it") {
    // the same vertex, the normal straight up, laid with the tangent Godot adds to a surface without tangents
    PlantGroundSurface surface;
    surface.arrays.positions = {1.0f, 2.0f, -3.0f};
    surface.arrays.normals = {0.0f, 1.0f, 0.0f};
    surface.arrays.colors = {1.0f, 0.0f, 0.0f, 1.0f};
    surface.arrays.uvs = {0.5f, 0.25f};
    surface.layout = PlantGroundLayout{0u, 12u, 12u, 8u, 0u, 4u, 12u};
    surface.vertex_bytes.assign(20u, 0u);
    surface.attribute_bytes.assign(12u, 0u);
    REQUIRE(encode_ground(surface, 0u));
    // the normal stays where stride 4 put it
    CHECK(surface.vertex_bytes[12] == 0xff);
    CHECK(surface.vertex_bytes[13] == 0x7f);
    CHECK(surface.vertex_bytes[14] == 0xff);
    CHECK(surface.vertex_bytes[15] == 0xff);
    // (0, 0, 1) crossed with (0, 1, 0) is (-1, 0, 0): octahedral (0, 0.5), the second share moved into the upper
    // half 0.75 - u16 0 and 49151, the bytes Godot 4.5.2 made of this normal (data/probes/p0690_ground_code)
    CHECK(surface.vertex_bytes[16] == 0x00);
    CHECK(surface.vertex_bytes[17] == 0x00);
    CHECK(surface.vertex_bytes[18] == 0xff);
    CHECK(surface.vertex_bytes[19] == 0xbf);
    // a tilted normal: Godot 4.5.2 made u16 6582 and 50076 of (0.1, 0.98, 0.17) normalised; the code may lie one
    // step off where the last bit of the float rounds otherwise
    const float length = ase::math::sqrt(0.1f * 0.1f + 0.98f * 0.98f + 0.17f * 0.17f);
    surface.arrays.normals = {0.1f / length, 0.98f / length, 0.17f / length};
    REQUIRE(encode_ground(surface, 0u));
    const int first = surface.vertex_bytes[16] | (surface.vertex_bytes[17] << 8);
    const int second = surface.vertex_bytes[18] | (surface.vertex_bytes[19] << 8);
    CHECK(first >= 6581);
    CHECK(first <= 6583);
    CHECK(second >= 50075);
    CHECK(second <= 50077);
    // the tangent needs its four bytes: a stride-8 vertex cut short writes nothing
    surface.vertex_bytes.resize(19u);
    CHECK_FALSE(encode_ground(surface, 0u));
}

TEST_CASE("P0690: the positive control holds the code against the bytes Godot made, byte for byte") {
    const std::vector<VivariumTerrainView> samples = area_samples();
    const std::vector<VivariumGroundCellView> cells = area_cells();
    PlantGroundSurface surface = area_surface(samples, cells);
    CHECK(surface.patchable);
    // one byte of the first normal off: the code is not Godot's, no patch is written
    PlantGroundSurface off = surface;
    off.vertex_bytes[off.layout.normal_offset] ^= 0x01u;
    CHECK_FALSE(ground_code_holds(off));
    // the tangent Godot 4.5 lays behind the normal (stride 8) is coded too: the control holds
    PlantGroundSurface tangent = area_surface(samples, cells, 8u);
    CHECK(tangent.patchable);
    // one byte of the first tangent off: not Godot's code, no patch
    PlantGroundSurface tangent_off = tangent;
    tangent_off.vertex_bytes[tangent_off.layout.normal_offset + 4u] ^= 0x01u;
    CHECK_FALSE(ground_code_holds(tangent_off));
    // a stride neither 4 nor 8 carries something no code here knows: no patch
    PlantGroundSurface unknown = surface;
    unknown.layout.normal_stride = 12u;
    CHECK_FALSE(ground_code_holds(unknown));
    // no vertex, nothing to hold against
    CHECK_FALSE(ground_code_holds(PlantGroundSurface{}));
}

TEST_CASE("P0690: a dig patches only the vertices that moved and the marks that changed") {
    std::vector<VivariumTerrainView> samples = area_samples();
    const std::vector<VivariumGroundCellView> cells = area_cells();
    PlantGroundSurface surface = area_surface(samples, cells);
    REQUIRE(surface.patchable);

    // nothing moved: the patch holds and names nothing
    REQUIRE(patch_ground(samples, cells, {}, {}, surface));
    CHECK(surface.positions.begin == surface.positions.end);
    CHECK(surface.normals.begin == surface.normals.end);
    CHECK(surface.attributes.begin == surface.attributes.end);
    CHECK_FALSE(surface.grown);

    // the dig lowers the centre sample (vertex 4) below every height the bounds took in
    samples[4].height = -0.5;
    samples[4].ny = 0.8;
    REQUIRE(patch_ground(samples, cells, {}, {}, surface));
    CHECK(surface.arrays.positions[4u * 3u + 1u] == doctest::Approx(-0.5f));
    CHECK(surface.arrays.normals[4u * 3u + 1u] == doctest::Approx(0.8f));
    CHECK(surface.positions.begin == 4u * 12u);
    CHECK(surface.positions.end == 4u * 12u + 12u);
    CHECK(surface.normals.begin == surface.layout.normal_offset + 4u * 4u);
    CHECK(surface.normals.end == surface.layout.normal_offset + 4u * 4u + 4u);
    CHECK(surface.attributes.begin == surface.attributes.end);
    CHECK(surface.grown);
    CHECK(surface.low_m == doctest::Approx(-0.5f));
    // the bytes are those a fresh build of the dug ground would carry
    const PlantGroundSurface fresh = area_surface(samples, cells);
    CHECK(surface.vertex_bytes == fresh.vertex_bytes);
    CHECK(surface.attribute_bytes == fresh.attribute_bytes);

    // the same dig on a surface with tangents (stride 8): the span takes the tangent in, the bytes are fresh ones
    PlantGroundSurface tangent = area_surface(area_samples(), cells, 8u);
    REQUIRE(tangent.patchable);
    REQUIRE(patch_ground(samples, cells, {}, {}, tangent));
    CHECK(tangent.normals.begin == tangent.layout.normal_offset + 4u * 8u);
    CHECK(tangent.normals.end == tangent.layout.normal_offset + 5u * 8u);
    const PlantGroundSurface tangent_fresh = area_surface(samples, cells, 8u);
    CHECK(tangent.vertex_bytes == tangent_fresh.vertex_bytes);

    // a disc around the centre levelled at its new height: only its mark changes, in the attribute stream
    const std::vector<PlantGroundDisc> discs = {PlantGroundDisc{10.0f, 20.0f, 0.5f, -0.5f}};
    REQUIRE(patch_ground(samples, cells, discs, {}, surface));
    CHECK(surface.positions.begin == surface.positions.end);
    CHECK(surface.arrays.colors[4u * 4u] == doctest::Approx(1.0f));
    CHECK(surface.attributes.begin == 4u * 12u);
    CHECK(surface.attributes.end == 5u * 12u);
    CHECK(surface.attribute_bytes[4u * 12u] == 0xff);
    CHECK_FALSE(surface.grown);

    // other cells, other samples or another order: the ground is built anew, nothing is written
    std::vector<VivariumGroundCellView> fewer = cells;
    fewer.pop_back();
    CHECK_FALSE(patch_ground(samples, fewer, discs, {}, surface));
    std::vector<VivariumTerrainView> swapped = samples;
    std::swap(swapped[0], swapped[1]);
    const std::vector<uint8_t> before = surface.vertex_bytes;
    CHECK_FALSE(patch_ground(swapped, cells, discs, {}, surface));
    CHECK(surface.vertex_bytes == before);
    PlantGroundSurface locked = surface;
    locked.patchable = false;
    CHECK_FALSE(patch_ground(samples, cells, discs, {}, locked));
}

TEST_CASE("03.8: the ground shader takes slope, marks and grid from its uniforms, never a number of its own") {
    // every uniform the look binds is declared in the shader, so a renamed key fails here, not in the window
    // the file as the stage copies it: shaders/ beside this tests/ folder, the name the look loads it by
    const std::string file = std::string(GODOT_GROUND_SHADER_FILE).substr(std::string("res://").size());
    const std::string code =
        ase::fileio::read_text(ase::fileio::path_join(ase::fileio::parent_of(ase::fileio::parent_of(__FILE__)), file));
    REQUIRE_MESSAGE(!code.empty(), file);
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

TEST_CASE("03.11: a terrain record with all nine keys is one sample, a record with a gap is none") {
    // usecase:100971 - lay out terrain samples: the record as FloraHostTernExptSystem lays it
    ase::kernel::KernelHostRecord record;
    record.object_id = 42u;
    const auto put = [&record](const char* key, double value) {
        ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
        ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, key);
        slot.value = value;
    };
    put(GODOT_KEY_BODY, 7.0);
    put(GODOT_KEY_X, 12.5);
    put(GODOT_KEY_Z, -3.0);
    put(GODOT_KEY_HEIGHT, 5.25);
    put(GODOT_KEY_NX, 0.1);
    put(GODOT_KEY_NY, 0.99);
    put(GODOT_KEY_NZ, -0.05);
    put(GODOT_KEY_U, 12.5);
    put(GODOT_KEY_V, -3.0);
    VivariumTerrainView view;
    REQUIRE(terrain_view_of(record, view));
    CHECK(view.object_id == 42u);
    CHECK(view.body == 7u);
    CHECK(view.x == doctest::Approx(12.5));
    CHECK(view.z == doctest::Approx(-3.0));
    CHECK(view.height == doctest::Approx(5.25));
    CHECK(view.nx == doctest::Approx(0.1));
    CHECK(view.ny == doctest::Approx(0.99));
    CHECK(view.nz == doctest::Approx(-0.05));
    CHECK(view.u == doctest::Approx(12.5));
    CHECK(view.v == doctest::Approx(-3.0));

    // a missing key is a broken port, never ground at height zero
    ase::kernel::KernelHostRecord gap = record;
    gap.value_count -= 1u;  // drops the texture coordinate v
    CHECK_FALSE(terrain_view_of(gap, view));

    // the body names a patch: a fraction or a negative number names none
    ase::kernel::KernelHostRecord split = record;
    split.values[0].value = 7.5;
    CHECK_FALSE(terrain_view_of(split, view));
    split.values[0].value = -1.0;
    CHECK_FALSE(terrain_view_of(split, view));
}

}  // namespace ase::adp::godot

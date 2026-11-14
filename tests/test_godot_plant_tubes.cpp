/**
 * ASE ADAPTER TESTS - BARK TUBES WITHOUT GODOT
 *
 * @file        test_godot_plant_tubes.cpp
 * @design      DSGN_066
 * @brief       Pins how tubes_by_body lays the vertices of the bark port out into one mesh per body
 * @description Register B0295 point 4: one continuous tube per axis whose knot rings parent and child share. The
 *              tests lay one axis of two internodes on rings of three vertices - a foot ring, the tip ring of
 *              the first internode, the tip ring of the second - and pin the place, the texture coordinates, the
 *              quads closed by the links, the custom data beside every vertex and the sway texels per internode,
 *              all as the port carried them. Register B0317: on rings of nine vertices, as ase-render-data lays a
 *              knot ring, the coarse tube of the twigs closes the same vertices on every second corner.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-13
 * @modified    2026-11-14
 * @version     00.00.02.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kBody = 7u;        // object id of the patch the test plant stands on
constexpr uint32_t kOtherBody = 8u;   // a second plant
constexpr uint32_t kFirst = 11u;      // the first internode of the axis
constexpr uint32_t kSecond = 12u;     // its continuation child
constexpr uint32_t kGone = 99u;       // an internode the segment port no longer shows
constexpr uint32_t kRingSize = 3u;    // vertices per test ring: two sides and the closing one
constexpr uint32_t kKnotRingSize = 9u;  // vertices of a knot ring of ase-render-data: eight corners and the closing one
constexpr double kOrder = 2.0;        // branching order of the test axis

/** A segment record with the length, radius, tiling, bark share and sway shares a test checks. */
VivariumOrganView internode(uint32_t object_id, uint32_t body, double length, double base, double top) {
    VivariumOrganView view;
    view.object_id = object_id;
    view.body = body;
    view.order = kOrder;
    view.length = length;
    view.radius = 0.05 - 0.02 * static_cast<double>(object_id - kFirst);
    view.bark = 0.25 * static_cast<double>(object_id - kFirst);
    view.tile_around = 3.0;
    view.tile_along = 4.0;
    view.tile_base = 2.0;
    view.sway_freq = 0.5;
    view.sway_base = base;
    view.sway_top = top;
    return view;
}

/**
 * One ring of `size` vertices with object ids first..first+size-1, each linked to the next and to the vertex
 * below with the same place on the ring; `below_first` GODOT_LINK_NONE lays a foot ring.
 */
void ring(std::vector<VivariumBarkVertexView>& out, uint32_t first, uint32_t below_first, uint32_t organ,
          uint32_t body, double v, uint32_t size = kRingSize) {
    for (uint32_t k = 0u; k < size; ++k) {
        VivariumBarkVertexView vertex;
        vertex.object_id = first + k;
        vertex.body = body;
        vertex.x = static_cast<double>(k);
        vertex.y = v;
        vertex.z = static_cast<double>(first);
        vertex.nx = 1.0;
        vertex.u = static_cast<double>(k) / static_cast<double>(size - 1u);
        vertex.v = v;
        vertex.next = k + 1u < size ? first + k + 1u : GODOT_LINK_NONE;
        vertex.below = below_first == GODOT_LINK_NONE ? GODOT_LINK_NONE : below_first + k;
        vertex.organ = organ;
        out.push_back(vertex);
    }
}

}  // namespace

TEST_CASE("usecase:101613 show the plant bark tubes: one mesh per body, the rings as the port carried them, two triangles per quad") {
    std::vector<VivariumBarkVertexView> vertices;
    ring(vertices, 100u, GODOT_LINK_NONE, kFirst, kBody, 0.0);
    ring(vertices, 110u, 100u, kFirst, kBody, 2.0);
    ring(vertices, 120u, 110u, kSecond, kBody, 3.0);
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4),
                                                       internode(kSecond, kBody, 1.0, 0.4, 0.9)};

    const auto tubes = tubes_by_body(vertices, internodes);
    REQUIRE(tubes.size() == 1u);
    const PlantBodyTube& tube = tubes.at(kBody);
    // every vertex once, its place, normal and texture coordinates unchanged
    REQUIRE(tube.arrays.positions.size() == 9u * 3u);
    REQUIRE(tube.arrays.normals.size() == 9u * 3u);
    REQUIRE(tube.arrays.uvs.size() == 9u * 2u);
    CHECK(tube.arrays.positions[4u * 3u + 0u] == 1.0f);
    CHECK(tube.arrays.positions[4u * 3u + 1u] == 2.0f);
    CHECK(tube.arrays.positions[4u * 3u + 2u] == 110.0f);
    CHECK(tube.arrays.normals[4u * 3u + 0u] == 1.0f);
    CHECK(tube.arrays.uvs[5u * 2u + 0u] == 1.0f);
    CHECK(tube.arrays.uvs[8u * 2u + 1u] == 3.0f);
    // two rings above the foot ring with two quads each: eight triangles, the first (below, vertex, next) and
    // (below, next, below of next)
    REQUIRE(tube.arrays.indices.size() == 8u * 3u);
    CHECK(tube.arrays.indices[0] == 0);
    CHECK(tube.arrays.indices[1] == 3);
    CHECK(tube.arrays.indices[2] == 4);
    CHECK(tube.arrays.indices[3] == 0);
    CHECK(tube.arrays.indices[4] == 4);
    CHECK(tube.arrays.indices[5] == 1);
    // the second internode closes its quads onto the tip ring of the first: the knot ring is shared
    CHECK(tube.arrays.indices[12] == 3);
    CHECK(tube.arrays.indices[13] == 6);
    CHECK(tube.arrays.indices[14] == 7);
    // a ring of two quads has no step that leaves three corners: the coarse tube of the twigs lays nothing
    CHECK(tube.twig_indices.empty());
}

TEST_CASE("usecase:101613 show the plant bark tubes: the coarse tube of the twigs closes the same vertices on evenly spaced corners") {
    std::vector<VivariumBarkVertexView> vertices;
    ring(vertices, 100u, GODOT_LINK_NONE, kFirst, kBody, 0.0, kKnotRingSize);
    ring(vertices, 110u, 100u, kFirst, kBody, 2.0, kKnotRingSize);
    ring(vertices, 120u, 110u, kSecond, kBody, 3.0, kKnotRingSize);
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4),
                                                       internode(kSecond, kBody, 1.0, 0.4, 0.9)};

    const PlantBodyTube tube = tubes_by_body(vertices, internodes).at(kBody);
    // the fine tube: eight quads on each of the two rings above the foot ring
    CHECK(tube.arrays.indices.size() == 2u * 8u * 6u);
    // the coarse tube: eight quads in steps of two leave four sides on each ring, over the same vertices
    REQUIRE(tube.twig_indices.size() == 2u * 4u * 6u);
    CHECK(tube.twig_indices[0] == 0);
    CHECK(tube.twig_indices[1] == 9);
    CHECK(tube.twig_indices[2] == 11);
    CHECK(tube.twig_indices[3] == 0);
    CHECK(tube.twig_indices[4] == 11);
    CHECK(tube.twig_indices[5] == 2);
    // the last side closes onto the closing vertex of the ring, and the second ring onto the knot ring below
    CHECK(tube.twig_indices[3u * 6u + 2u] == 17);
    CHECK(tube.twig_indices[4u * 6u + 0u] == 9);
    CHECK(tube.twig_indices[4u * 6u + 1u] == 18);
}

TEST_CASE("usecase:101613 show the plant bark tubes: every vertex carries the bark, tiling and sway of the internode its ring belongs to") {
    std::vector<VivariumBarkVertexView> vertices;
    ring(vertices, 100u, GODOT_LINK_NONE, kFirst, kBody, 0.0);
    ring(vertices, 110u, 100u, kFirst, kBody, 2.0);
    ring(vertices, 120u, 110u, kSecond, kBody, 3.0);
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4),
                                                       internode(kSecond, kBody, 1.0, 0.4, 0.9)};

    const PlantBodyTube tube = tubes_by_body(vertices, internodes).at(kBody);
    REQUIRE(tube.custom0.size() == 9u * GODOT_BARK_CUSTOM_VALUES);
    REQUIRE(tube.custom1.size() == 9u * GODOT_BARK_CUSTOM_VALUES);
    // custom0: bark share, tiles around, tiles along per metre of the internode, base tiles per white tile
    CHECK(tube.custom0[0] == 0.0f);
    CHECK(tube.custom0[1] == 3.0f);
    CHECK(tube.custom0[2] == 2.0f);
    CHECK(tube.custom0[3] == 2.0f);
    CHECK(tube.custom0[6u * GODOT_BARK_CUSTOM_VALUES + 0u] == 0.25f);
    CHECK(tube.custom0[6u * GODOT_BARK_CUSTOM_VALUES + 2u] == 4.0f);
    // custom1: the sway slot of its internode, the share at the base on the foot ring and at the top on a tip ring
    CHECK(tube.custom1[0] == 0.0f);
    CHECK(tube.custom1[1] == 0.0f);
    CHECK(tube.custom1[3u * GODOT_BARK_CUSTOM_VALUES + 0u] == 0.0f);
    CHECK(tube.custom1[3u * GODOT_BARK_CUSTOM_VALUES + 1u] == doctest::Approx(0.4f));
    CHECK(tube.custom1[6u * GODOT_BARK_CUSTOM_VALUES + 0u] == 1.0f);
    CHECK(tube.custom1[6u * GODOT_BARK_CUSTOM_VALUES + 1u] == doctest::Approx(0.9f));
    // custom1: the radius of the ring - the foot ring that of its own internode, the knot ring the continuation
    // child closes onto that of the child (the tip radius of the cone), the tip ring at the end of the axis its own
    CHECK(tube.custom1[0u * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.05f));
    CHECK(tube.custom1[3u * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.03f));
    CHECK(tube.custom1[8u * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.03f));
    // custom1: the order of the axis, for the shown orders of the budget
    CHECK(tube.custom1[0u * GODOT_BARK_CUSTOM_VALUES + 3u] == static_cast<float>(kOrder));
    CHECK(tube.custom1[7u * GODOT_BARK_CUSTOM_VALUES + 3u] == static_cast<float>(kOrder));
    // one sway slot per internode, in the order its first vertex came
    REQUIRE(tube.sway.size() == 2u * GODOT_SWAY_VALUES);
    CHECK(tube.sway[0] == 0.5f);
    CHECK(tube.sway[GODOT_SWAY_VALUES + 6u] == doctest::Approx(0.9f));
}

TEST_CASE("usecase:101613 show the plant bark tubes: a vertex of an internode the segment port does not show is not drawn, nor its quads") {
    std::vector<VivariumBarkVertexView> vertices;
    ring(vertices, 100u, GODOT_LINK_NONE, kFirst, kBody, 0.0);
    ring(vertices, 110u, 100u, kGone, kBody, 2.0);
    ring(vertices, 200u, GODOT_LINK_NONE, kGone, kOtherBody, 0.0);
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4)};

    const auto tubes = tubes_by_body(vertices, internodes);
    REQUIRE(tubes.size() == 1u);
    CHECK(tubes.count(kOtherBody) == 0u);
    const PlantBodyTube& tube = tubes.at(kBody);
    CHECK(tube.arrays.positions.size() == kRingSize * 3u);
    CHECK(tube.arrays.indices.empty());
}

}  // namespace ase::adp::godot

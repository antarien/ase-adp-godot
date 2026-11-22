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
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_organ_records.hpp>
#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/utils/strops.hpp>

#include <cstddef>
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
constexpr uint32_t kSides = 8u;       // corners K of the test axis, as its foot record carries them (usecase:101644)
constexpr uint32_t kKnotRingSize = kSides + 1u;  // vertices of a knot ring: the K corners and the closing one
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

namespace {

/** The tip knot of one internode at height y on the axis, frame q = (0, 0, sin 15 deg, cos 15 deg). */
VivariumKnotView knot(uint32_t object_id, uint32_t below, uint32_t body, double y, double radius) {
    VivariumKnotView view;
    view.object_id = object_id;
    view.body = body;
    view.x = 0.5;
    view.y = y;
    view.z = -0.25;
    view.radius = radius;
    view.qz = 0.25881904510252074;
    view.qw = 0.96592582628906831;
    view.v = y;
    view.below = below;
    return view;
}

/** The foot knot of the axis that the internode starts. */
VivariumFootView foot(uint32_t object_id, uint32_t body, double radius) {
    VivariumFootView view;
    view.object_id = object_id;
    view.body = body;
    view.x = 0.5;
    view.z = -0.25;
    view.radius = radius;
    view.qw = 1.0;
    view.sides = kSides;
    return view;
}

}  // namespace

TEST_CASE("usecase:101613 show the plant bark tubes: the knot form lays one template ring per knot and closes every piece onto the ring below") {
    const std::vector<VivariumFootView> feet = {foot(kFirst, kBody, 0.05)};
    const std::vector<VivariumKnotView> knots = {knot(kFirst, kFirst, kBody, 2.0, 0.04),
                                                 knot(kSecond, kFirst, kBody, 3.0, 0.02)};
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4),
                                                       internode(kSecond, kBody, 1.0, 0.4, 0.9)};

    const auto tubes = tubes_by_body(knots, feet, {}, internodes);
    REQUIRE(tubes.size() == 1u);
    const PlantBodyTube& tube = tubes.at(kBody);
    // three rings - the foot, the tip of the first, the tip of the second - of the template's vertices each
    const std::size_t verts = 3u * kKnotRingSize;
    REQUIRE(tube.arrays.positions.size() == verts * 3u);
    REQUIRE(tube.arrays.normals.size() == verts * 3u);
    REQUIRE(tube.arrays.uvs.size() == verts * 2u);
    REQUIRE(tube.custom0.size() == verts * GODOT_BARK_CUSTOM_VALUES);
    REQUIRE(tube.custom1.size() == verts * GODOT_BARK_CUSTOM_VALUES);
    REQUIRE(tube.custom2.size() == verts * GODOT_TUBE_FRAME_VALUES);
    // every vertex of a ring stands at the centre of its knot; the shader sets it out
    const std::size_t tip = kKnotRingSize;  // the first vertex of the tip ring of kFirst
    CHECK(tube.arrays.positions[(tip + 3u) * 3u + 0u] == 0.5f);
    CHECK(tube.arrays.positions[(tip + 3u) * 3u + 1u] == 2.0f);
    CHECK(tube.arrays.positions[(tip + 3u) * 3u + 2u] == -0.25f);
    // corner 2 of 8 is a quarter around: the template direction (cos a, 0, -sin a) = (0, 0, -1)
    CHECK(tube.arrays.normals[(tip + 2u) * 3u + 0u] == doctest::Approx(0.0f).epsilon(1e-6));
    CHECK(tube.arrays.normals[(tip + 2u) * 3u + 2u] == doctest::Approx(-1.0f));
    // u = k / sides, the closing vertex at 1; v the metres of the knot along the axis
    CHECK(tube.arrays.uvs[(tip + 2u) * 2u + 0u] == doctest::Approx(0.25f));
    CHECK(tube.arrays.uvs[(tip + kSides) * 2u + 0u] == doctest::Approx(1.0f));
    CHECK(tube.arrays.uvs[(tip + 2u) * 2u + 1u] == 2.0f);
    // the frame of the knot beside every vertex of its ring
    CHECK(tube.custom2[(tip + 5u) * GODOT_TUBE_FRAME_VALUES + 2u] == doctest::Approx(0.258819f));
    CHECK(tube.custom2[(tip + 5u) * GODOT_TUBE_FRAME_VALUES + 3u] == doctest::Approx(0.965926f));
    // the tangent of the normal map where u grows, (-sin a, 0, -cos a): (-1, 0, 0) a quarter around, (0, 0, -1) at
    // corner 0; its binormal (normal x tangent times w) runs down the axis, against the v that grows up it, as Godot
    // reads a binormal (CylinderMesh: v downward, w = +1)
    REQUIRE(tube.tangents.size() == verts * 4u);
    CHECK(tube.tangents[(tip + 2u) * 4u + 0u] == doctest::Approx(-1.0f));
    CHECK(tube.tangents[(tip + 2u) * 4u + 2u] == doctest::Approx(0.0f).epsilon(1e-6));
    CHECK(tube.tangents[tip * 4u + 0u] == doctest::Approx(0.0f).epsilon(1e-6));
    CHECK(tube.tangents[tip * 4u + 2u] == doctest::Approx(-1.0f));
    const float nx = tube.arrays.normals[(tip + 2u) * 3u + 0u];
    const float nz = tube.arrays.normals[(tip + 2u) * 3u + 2u];
    const float tx = tube.tangents[(tip + 2u) * 4u + 0u];
    const float tz = tube.tangents[(tip + 2u) * 4u + 2u];
    const float binormal_up = (nz * tx - nx * tz) * tube.tangents[(tip + 2u) * 4u + 3u];
    CHECK(binormal_up == doctest::Approx(-1.0f));
    // the radius: the foot record's at the foot ring, the knot's at the tip ring
    CHECK(tube.custom1[0u * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.05f));
    CHECK(tube.custom1[tip * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.04f));
    CHECK(tube.custom1[(2u * kKnotRingSize) * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.02f));
    // the sway share: the base of kFirst at the foot, the top of each internode at its tip
    CHECK(tube.custom1[0u * GODOT_BARK_CUSTOM_VALUES + 1u] == doctest::Approx(0.0f));
    CHECK(tube.custom1[tip * GODOT_BARK_CUSTOM_VALUES + 1u] == doctest::Approx(0.4f));
    CHECK(tube.custom1[(2u * kKnotRingSize) * GODOT_BARK_CUSTOM_VALUES + 1u] == doctest::Approx(0.9f));
    // two sway slots, one per internode
    CHECK(tube.sway.size() == 2u * GODOT_SWAY_VALUES);
    // two pieces, two triangles per corner: the first closes foot corner 0, tip corners 0 and 1
    REQUIRE(tube.arrays.indices.size() == 2u * kSides * 6u);
    CHECK(tube.arrays.indices[0] == 0);
    CHECK(tube.arrays.indices[1] == static_cast<int32_t>(tip));
    CHECK(tube.arrays.indices[2] == static_cast<int32_t>(tip + 1u));
    // the second piece closes onto the tip ring of kFirst, not onto a ring of its own
    const std::size_t second = kSides * 6u;
    CHECK(tube.arrays.indices[second] == static_cast<int32_t>(tip));
    CHECK(tube.arrays.indices[second + 1u] == static_cast<int32_t>(2u * kKnotRingSize));
    // the coarse tube of the twigs: every second corner of eight leaves four
    CHECK(tube.twig_indices.size() == 2u * (kSides / 2u) * 6u);
    CHECK(tube.twig_indices[2] == static_cast<int32_t>(tip + 2u));
}

TEST_CASE("usecase:101613 show the plant bark tubes: the knot form draws no knot of a gone internode and no piece without its lower ring") {
    const std::vector<VivariumFootView> feet = {foot(kFirst, kBody, 0.05), foot(kGone, kOtherBody, 0.05)};
    const std::vector<VivariumKnotView> knots = {knot(kFirst, kFirst, kBody, 2.0, 0.04),
                                                 knot(kSecond, kGone, kBody, 3.0, 0.02),
                                                 knot(kGone, kGone, kOtherBody, 2.0, 0.04)};
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4),
                                                       internode(kSecond, kBody, 1.0, 0.4, 0.9)};

    const auto tubes = tubes_by_body(knots, feet, {}, internodes);
    REQUIRE(tubes.size() == 1u);
    CHECK(tubes.count(kOtherBody) == 0u);
    const PlantBodyTube& tube = tubes.at(kBody);
    // the foot and both tips are drawn, but kSecond names a gone lower ring: only the piece of kFirst closes
    CHECK(tube.arrays.positions.size() == 3u * kKnotRingSize * 3u);
    CHECK(tube.arrays.indices.size() == kSides * 6u);
    CHECK(tube.scar_indices.empty());
}

TEST_CASE("usecase:101613 show the plant bark tubes: the cut face of a stub closes its tip ring with a disk of wood facing out along the axis") {
    const std::vector<VivariumFootView> feet = {foot(kFirst, kBody, 0.05)};
    const std::vector<VivariumKnotView> knots = {knot(kFirst, kFirst, kBody, 2.0, 0.04),
                                                 knot(kSecond, kFirst, kBody, 3.0, 0.02)};
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4),
                                                       internode(kSecond, kBody, 1.0, 0.4, 0.9)};
    VivariumScarView cut;
    cut.object_id = kSecond;
    cut.body = kBody;
    // a cut face of a gone internode and one naming another body than its knot lay nothing
    VivariumScarView gone;
    gone.object_id = kGone;
    gone.body = kBody;
    VivariumScarView elsewhere;
    elsewhere.object_id = kFirst;
    elsewhere.body = kOtherBody;
    const std::vector<VivariumScarView> scars = {cut, gone, elsewhere};

    const auto tubes = tubes_by_body(knots, feet, scars, internodes);
    REQUIRE(tubes.size() == 1u);
    CHECK(tubes.count(kOtherBody) == 0u);
    const PlantBodyTube& tube = tubes.at(kBody);
    // three rings of the tube, then the rim ring of the disk and its centre
    const std::size_t rim = 3u * kKnotRingSize;
    const std::size_t centre = rim + kKnotRingSize;
    const std::size_t verts = centre + 1u;
    REQUIRE(tube.arrays.positions.size() == verts * 3u);
    REQUIRE(tube.arrays.normals.size() == verts * 3u);
    REQUIRE(tube.arrays.uvs.size() == verts * 2u);
    REQUIRE(tube.tangents.size() == verts * 4u);
    REQUIRE(tube.custom0.size() == verts * GODOT_BARK_CUSTOM_VALUES);
    REQUIRE(tube.custom1.size() == verts * GODOT_BARK_CUSTOM_VALUES);
    REQUIRE(tube.custom2.size() == verts * GODOT_TUBE_FRAME_VALUES);
    // the disk stands on the tip knot of the stub, its rim set out by the radius of that knot, its centre not
    CHECK(tube.arrays.positions[centre * 3u + 1u] == 3.0f);
    CHECK(tube.arrays.positions[(rim + 3u) * 3u + 1u] == 3.0f);
    CHECK(tube.custom1[rim * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.02f));
    CHECK(tube.custom1[centre * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.0f));
    // the sway share at the top of the stub and its frame beside the centre, as on its rim
    CHECK(tube.custom1[centre * GODOT_BARK_CUSTOM_VALUES + 1u] == doctest::Approx(0.9f));
    CHECK(tube.custom2[centre * GODOT_TUBE_FRAME_VALUES + 2u] == doctest::Approx(0.258819f));
    // the wood texture: the pith at the centre, the rim on the edge of the texture - corner 0 east, corner 2 a quarter
    CHECK(tube.arrays.uvs[centre * 2u + 0u] == doctest::Approx(GODOT_SCAR_UV_CENTRE));
    CHECK(tube.arrays.uvs[centre * 2u + 1u] == doctest::Approx(GODOT_SCAR_UV_CENTRE));
    CHECK(tube.arrays.uvs[rim * 2u + 0u] == doctest::Approx(GODOT_SCAR_UV_CENTRE + GODOT_SCAR_UV_HALF));
    CHECK(tube.arrays.uvs[(rim + 2u) * 2u + 1u] == doctest::Approx(GODOT_SCAR_UV_CENTRE - GODOT_SCAR_UV_HALF));
    // the bark tube is untouched: the disk has its own vertices and its own indices
    CHECK(tube.arrays.uvs[(2u * kKnotRingSize + 2u) * 2u + 0u] == doctest::Approx(0.25f));
    CHECK(tube.arrays.indices.size() == 2u * kSides * 6u);
    // one triangle per corner, fanned from the centre: (centre, k + 1, k), clockwise seen from above the stub
    REQUIRE(tube.scar_indices.size() == kSides * 3u);
    CHECK(tube.scar_indices[0] == static_cast<int32_t>(centre));
    CHECK(tube.scar_indices[1] == static_cast<int32_t>(rim + 1u));
    CHECK(tube.scar_indices[2] == static_cast<int32_t>(rim));
    // its face along the axis: the template directions of corners k + 1 and k, crossed, point down the axis, so the
    // clockwise front Godot takes looks up it, out of the cut
    const float ax = tube.arrays.normals[(rim + 1u) * 3u + 0u];
    const float az = tube.arrays.normals[(rim + 1u) * 3u + 2u];
    const float bx = tube.arrays.normals[rim * 3u + 0u];
    const float bz = tube.arrays.normals[rim * 3u + 2u];
    CHECK(az * bx - ax * bz < 0.0f);
}

TEST_CASE("usecase:101643 ring the tubes by curvature: a bent piece stands its intermediate ring on the Hermite curve, its frame carried by the least turn") {
    // the foot points up at (0.5, 0, -0.25); the tip knot one metre left and one up, its frame turned a quarter about
    // the north axis, so its tangent points west: halfway the curve leans 45 degrees, and the ring there carries the
    // frame of the foot turned an eighth about the same axis
    const std::vector<VivariumFootView> feet = {foot(kFirst, kBody, 0.05)};
    VivariumKnotView bent = knot(kFirst, kFirst, kBody, 1.0, 0.03);
    bent.x = -0.5;
    bent.qz = 0.70710678118654752;
    bent.qw = 0.70710678118654752;
    bent.v = 1.4;
    bent.rings = 1u;
    const std::vector<VivariumKnotView> knots = {bent};
    const std::vector<VivariumOrganView> internodes = {internode(kFirst, kBody, 2.0, 0.0, 0.4)};

    const auto tubes = tubes_by_body(knots, feet, {}, internodes);
    REQUIRE(tubes.size() == 1u);
    const PlantBodyTube& tube = tubes.at(kBody);
    // three rings: the foot, the tip, and the intermediate ring laid with the quads of its piece
    REQUIRE(tube.arrays.positions.size() == 3u * kKnotRingSize * 3u);
    const std::size_t mid = 2u * kKnotRingSize;
    // the Hermite point at t = 1/2: p0 / 2 + p1 / 2 + chord / 8 (t0 - t1), chord = sqrt 2
    CHECK(tube.arrays.positions[mid * 3u + 0u] == doctest::Approx(0.176777f).epsilon(1e-4));
    CHECK(tube.arrays.positions[mid * 3u + 1u] == doctest::Approx(0.676777f).epsilon(1e-4));
    CHECK(tube.arrays.positions[mid * 3u + 2u] == doctest::Approx(-0.25f));
    // the frame of the foot turned 22.5 degrees about the north axis
    CHECK(tube.custom2[mid * GODOT_TUBE_FRAME_VALUES + 0u] == doctest::Approx(0.0f).epsilon(1e-6));
    CHECK(tube.custom2[mid * GODOT_TUBE_FRAME_VALUES + 2u] == doctest::Approx(0.382683f).epsilon(1e-4));
    CHECK(tube.custom2[mid * GODOT_TUBE_FRAME_VALUES + 3u] == doctest::Approx(0.923880f).epsilon(1e-4));
    // radius, v and sway share halfway between the foot and the tip
    CHECK(tube.custom1[mid * GODOT_BARK_CUSTOM_VALUES + 2u] == doctest::Approx(0.04f));
    CHECK(tube.arrays.uvs[mid * 2u + 1u] == doctest::Approx(0.7f));
    CHECK(tube.custom1[mid * GODOT_BARK_CUSTOM_VALUES + 1u] == doctest::Approx(0.2f));
    // two pieces of quads: the foot onto the intermediate ring, the intermediate ring onto the tip
    REQUIRE(tube.arrays.indices.size() == 2u * kSides * 6u);
    CHECK(tube.arrays.indices[0] == 0);
    CHECK(tube.arrays.indices[1] == static_cast<int32_t>(mid));
    CHECK(tube.arrays.indices[kSides * 6u] == static_cast<int32_t>(mid));
    CHECK(tube.arrays.indices[kSides * 6u + 1u] == static_cast<int32_t>(kKnotRingSize));
    CHECK(tube.twig_indices.size() == 2u * (kSides / 2u) * 6u);
}

TEST_CASE("usecase:101643 ring the tubes by curvature: a knot record carries its intermediate rings, a count above the most or a fraction is a broken port") {
    // the tip record of one internode as FloraHostCylNodeExptSystem files it on flora.mesh.knot.v1, rings last
    ase::kernel::KernelHostRecord record;
    record.object_id = kSecond;
    const auto put = [&record](const char* key, double value) {
        ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
        ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, key);
        slot.value = value;
    };
    put(GODOT_KEY_BODY, static_cast<double>(kBody));
    put(GODOT_KEY_X, 0.0);
    put(GODOT_KEY_Y, 3.0);
    put(GODOT_KEY_Z, 0.0);
    put(GODOT_KEY_RADIUS, 0.02);
    put(GODOT_KEY_QX, 0.0);
    put(GODOT_KEY_QY, 0.0);
    put(GODOT_KEY_QZ, 0.0);
    put(GODOT_KEY_QW, 1.0);
    put(GODOT_KEY_V, 3.0);
    put(GODOT_KEY_BELOW, static_cast<double>(kFirst));
    put(GODOT_KEY_RINGS, 2.0);
    VivariumKnotView view;
    REQUIRE(knot_view_of(record, view));
    CHECK(view.object_id == kSecond);
    CHECK(view.below == kFirst);
    CHECK(view.rings == 2u);

    // the most rings ase-render-data gives a piece still reads, one more names a ring count nobody computed
    ase::kernel::KernelHostRecord bent = record;
    bent.values[bent.value_count - 1u].value = static_cast<double>(GODOT_TUBE_RING_MAX);
    REQUIRE(knot_view_of(bent, view));
    CHECK(view.rings == GODOT_TUBE_RING_MAX);
    bent.values[bent.value_count - 1u].value = static_cast<double>(GODOT_TUBE_RING_MAX + 1u);
    CHECK_FALSE(knot_view_of(bent, view));
    bent.values[bent.value_count - 1u].value = 1.5;
    CHECK_FALSE(knot_view_of(bent, view));

    // a record without its rings is a broken port, never a straight piece by default
    ase::kernel::KernelHostRecord gap = record;
    gap.value_count -= 1u;
    CHECK_FALSE(knot_view_of(gap, view));
}

}  // namespace ase::adp::godot

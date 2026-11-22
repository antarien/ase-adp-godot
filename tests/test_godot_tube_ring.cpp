/**
 * ASE ADAPTER TESTS - THE CORNERS K OF AN AXIS IN THE BARK TUBE WITHOUT GODOT
 *
 * @file        test_godot_tube_ring.cpp
 * @design      DSGN_018
 * @brief       Pins how the view takes the corners K of every axis from its foot record and lays every ring with it
 * @description PLAN_ASE_VIVARIUM_TREEGEN T.6, section "Ecken je Achse nach Dicke" (usecase:101644 count ring corners
 *              by size): the foot record carries K once per axis (key sides) and the noise key of the axis (key
 *              noise, "Rindenrauschen"); foot_view_of reads K as a whole number from GODOT_TUBE_SIDE_MIN to
 *              GODOT_TUBE_SIDE_MAX and the key from GODOT_TUBE_NOISE_LEAST to GODOT_TUBE_NOISE_MOST, axis_foot walks
 *              `below` down to the foot, twig_stride closes the coarse tube on the largest divisor that leaves the
 *              least corners, and tubes_by_body lays K + 1 vertices per ring, each with the noise key of its axis,
 *              and at most ceil(K / 2) intermediate rings per piece.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101644 - count ring corners by size
 * @actor       Tube Ring Divider (TUBERINDIV)
 * @created     2026-11-21
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
#include <ase/adp/godot/tube/godot_tube_ring.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/utils/strops.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kBody = 7u;     // object id of the patch the test plant stands on
constexpr uint32_t kStem = 11u;    // the first internode of the stem, its foot
constexpr uint32_t kUpper = 12u;   // its continuation child
constexpr uint32_t kTop = 13u;     // the next one up
constexpr uint32_t kBranch = 21u;  // the first internode of a branch, the foot of its own axis
constexpr uint32_t kTwig = 22u;    // the continuation of the branch
constexpr uint32_t kLoose = 31u;   // a knot whose chain names an internode no knot record carries
constexpr double kNoise = 0.375;   // the noise key of the test axis, the place of the axis in the bark noise field

/** The foot record of one axis as FloraHostCylNodeExptSystem files it on flora.mesh.foot.v1, sides and noise last. */
ase::kernel::KernelHostRecord foot_record(double sides, double noise = kNoise) {
    ase::kernel::KernelHostRecord record;
    record.object_id = kStem;
    const auto put = [&record](const char* key, double value) {
        ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
        ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, key);
        slot.value = value;
    };
    put(GODOT_KEY_BODY, static_cast<double>(kBody));
    put(GODOT_KEY_X, 0.5);
    put(GODOT_KEY_Y, 0.0);
    put(GODOT_KEY_Z, -0.25);
    put(GODOT_KEY_RADIUS, 0.05);
    put(GODOT_KEY_QX, 0.0);
    put(GODOT_KEY_QY, 0.0);
    put(GODOT_KEY_QZ, 0.0);
    put(GODOT_KEY_QW, 1.0);
    put(GODOT_KEY_V, 0.0);
    put(GODOT_KEY_SIDES, sides);
    put(GODOT_KEY_NOISE, noise);
    return record;
}

/** A knot of the port: the internode, the one below it, its height on the axis and its intermediate rings. */
VivariumKnotView knot_at(uint32_t object_id, uint32_t below, double y, uint32_t rings) {
    VivariumKnotView view;
    view.object_id = object_id;
    view.body = kBody;
    view.y = y;
    view.radius = 0.03;
    view.qw = 1.0;
    view.v = y;
    view.below = below;
    view.rings = rings;
    return view;
}

}  // namespace

TEST_CASE("usecase:101644 count ring corners by size: a foot record carries the corners of its axis as a whole number in the band") {
    VivariumFootView view;
    REQUIRE(foot_view_of(foot_record(5.0), view));
    CHECK(view.sides == 5u);
    CHECK(view.body == kBody);
    REQUIRE(foot_view_of(foot_record(static_cast<double>(GODOT_TUBE_SIDE_MIN)), view));
    CHECK(view.sides == GODOT_TUBE_SIDE_MIN);
    REQUIRE(foot_view_of(foot_record(static_cast<double>(GODOT_TUBE_SIDE_MAX)), view));
    CHECK(view.sides == GODOT_TUBE_SIDE_MAX);
    // below the fewest, above the most or a fraction names a corner count ase-render-data never laid
    CHECK_FALSE(foot_view_of(foot_record(static_cast<double>(GODOT_TUBE_SIDE_MIN - 1u)), view));
    CHECK_FALSE(foot_view_of(foot_record(static_cast<double>(GODOT_TUBE_SIDE_MAX + 1u)), view));
    CHECK_FALSE(foot_view_of(foot_record(5.5), view));
    // a record without its noise key, the last, is a broken port, never an axis at a default place in the field
    ase::kernel::KernelHostRecord gap = foot_record(5.0);
    gap.value_count -= 1u;
    CHECK_FALSE(foot_view_of(gap, view));
    // and without its sides as well, never a ring of a default count
    gap.value_count -= 1u;
    CHECK_FALSE(foot_view_of(gap, view));
}

TEST_CASE("usecase:101613 show the plant bark tubes: a foot record carries the noise key of its axis in [0, 1]") {
    VivariumFootView view;
    REQUIRE(foot_view_of(foot_record(5.0), view));
    CHECK(view.noise == doctest::Approx(kNoise));
    REQUIRE(foot_view_of(foot_record(5.0, GODOT_TUBE_NOISE_LEAST), view));
    REQUIRE(foot_view_of(foot_record(5.0, GODOT_TUBE_NOISE_MOST), view));
    // a key outside the draw of ase-render-data is a broken foot port
    CHECK_FALSE(foot_view_of(foot_record(5.0, GODOT_TUBE_NOISE_MOST + kNoise), view));
    CHECK_FALSE(foot_view_of(foot_record(5.0, GODOT_TUBE_NOISE_LEAST - kNoise), view));
}

TEST_CASE("usecase:101644 count ring corners by size: every knot takes the corners of the foot its chain walks down to") {
    const std::vector<VivariumKnotView> knots = {knot_at(kStem, kStem, 1.0, 0u),    knot_at(kUpper, kStem, 2.0, 0u),
                                                 knot_at(kTop, kUpper, 3.0, 0u),     knot_at(kBranch, kBranch, 2.5, 0u),
                                                 knot_at(kTwig, kBranch, 3.0, 0u),   knot_at(kLoose, kLoose + 1u, 4.0, 0u)};
    std::unordered_map<uint32_t, const VivariumKnotView*> knot_by_id;
    for (const VivariumKnotView& knot : knots) {
        knot_by_id.emplace(knot.object_id, &knot);
    }
    const std::unordered_map<uint32_t, uint32_t> foot_sides = {{kStem, 12u}, {kBranch, 5u}};
    std::unordered_map<uint32_t, uint32_t> known;
    // the top of the stem walks two steps down, and every knot on the way keeps the K of the stem
    CHECK(axis_foot(kTop, knot_by_id, foot_sides, known) == 12u);
    CHECK(known.at(kUpper) == 12u);
    CHECK(known.at(kStem) == 12u);
    // the branch is an axis of its own: its foot, not the stem it stands on, gives its K
    CHECK(axis_foot(kTwig, knot_by_id, foot_sides, known) == 5u);
    CHECK(axis_foot(kBranch, knot_by_id, foot_sides, known) == 5u);
    // a chain that ends on no knot reaches no foot: no K, no ring
    CHECK(axis_foot(kLoose, knot_by_id, foot_sides, known) == 0u);
    CHECK(known.at(kLoose) == 0u);
}

TEST_CASE("usecase:101644 count ring corners by size: a chain that closes on itself ends without a corner count") {
    const std::vector<VivariumKnotView> knots = {knot_at(kUpper, kTop, 2.0, 0u), knot_at(kTop, kUpper, 3.0, 0u)};
    std::unordered_map<uint32_t, const VivariumKnotView*> knot_by_id;
    for (const VivariumKnotView& knot : knots) {
        knot_by_id.emplace(knot.object_id, &knot);
    }
    std::unordered_map<uint32_t, uint32_t> known;
    CHECK(axis_foot(kUpper, knot_by_id, {{kStem, 8u}}, known) == 0u);
    CHECK(known.at(kTop) == 0u);
}

TEST_CASE("usecase:101644 count ring corners by size: the coarse tube of the twigs closes the largest step that leaves three corners") {
    CHECK(twig_stride(8u) == 2u);
    CHECK(twig_stride(16u) == 4u);
    CHECK(twig_stride(12u) == 4u);
    CHECK(twig_stride(6u) == 2u);
    CHECK(twig_stride(5u) == 1u);
    CHECK(twig_stride(4u) == 1u);
    CHECK(twig_stride(GODOT_TUBE_SIDE_MIN) == 1u);
}

TEST_CASE("usecase:101644 count ring corners by size: the tube lays K + 1 vertices per ring and at most ceil(K / 2) rings in a bend") {
    VivariumFootView foot;
    REQUIRE(foot_view_of(foot_record(5.0), foot));
    // a bent first piece asks for four intermediate rings, half a turn at K = 5 bounds them at three
    VivariumKnotView bent = knot_at(kStem, kStem, 1.0, 4u);
    bent.x = -0.5;
    bent.qz = 0.70710678118654752;
    bent.qw = 0.70710678118654752;
    VivariumOrganView organ;
    organ.object_id = kStem;
    organ.body = kBody;
    organ.length = 1.5;
    organ.sway_top = 0.4;

    const auto tubes = tubes_by_body({bent}, {foot}, {}, {organ});
    REQUIRE(tubes.size() == 1u);
    const PlantBodyTube& tube = tubes.at(kBody);
    // the foot ring, three intermediate rings and the tip ring, six vertices each
    const std::size_t ring_size = 5u + 1u;
    const std::size_t rings = 2u + (5u + 1u) / 2u;
    REQUIRE(tube.arrays.positions.size() == rings * ring_size * 3u);
    REQUIRE(tube.custom2.size() == rings * ring_size * GODOT_TUBE_FRAME_VALUES);
    // every vertex of every ring, the intermediate rings of the bend included, carries the noise key of its axis
    REQUIRE(tube.custom3.size() == rings * ring_size * GODOT_TUBE_NOISE_VALUES);
    for (const float key : tube.custom3) {
        CHECK(key == doctest::Approx(static_cast<float>(kNoise)));
    }
    // corner k at u = k / 5, the closing vertex at 1
    CHECK(tube.arrays.uvs[1u * 2u] == doctest::Approx(0.2f));
    CHECK(tube.arrays.uvs[5u * 2u] == doctest::Approx(1.0f));
    // four pieces of five quads, and the coarse tube closes every corner: 5 has no step that leaves three
    CHECK(tube.arrays.indices.size() == (rings - 1u) * 5u * 6u);
    CHECK(tube.twig_indices.size() == (rings - 1u) * 5u * 6u);
}

}  // namespace ase::adp::godot

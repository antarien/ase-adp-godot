/**
 * ASE ADAPTER TESTS - IMPOSTOR OF A FAR PLANT WITHOUT GODOT
 *
 * @file        test_godot_plant_impostor.cpp
 * @design      DSGN_066
 * @brief       Pins from which distance impostor_of lets the card stand in for a body, and its size
 * @description Register B0295 point 4: far bodies become an impostor. The distance follows the one-pixel rule of
 *              the bark shader: beyond d = w H / tan(fov / 2) the widest ring w is thinner than one pixel. The
 *              tests lay a tube of two rings and a leaf record and pin the reach, the box around tube and leaf, and
 *              the texture as many pixels as the body spans at the reach.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_impostor.hpp>
#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr float kViewHeight = 1000.0f;  // render height of the test view in pixels
constexpr float kTanHalfFov = 0.5f;     // tangent of half its vertical field of view
constexpr float kWide = 0.1f;           // radius of the foot ring, the widest
constexpr float kThin = 0.02f;          // radius of the tip ring

/** One vertex record of the tube: its place, and beside it the custom1 values with that ring radius. */
void lay(PlantBodyTube& tube, float x, float y, float z, float radius) {
    tube.arrays.positions.insert(tube.arrays.positions.end(), {x, y, z});
    tube.custom1.insert(tube.custom1.end(), {0.0f, 0.0f, radius, 1.0f});
}

/** A tube from (-0.1, 0, -0.1) to (0.1, 2, 0.1): a foot ring of kWide and a tip ring of kThin. */
PlantBodyTube test_tube() {
    PlantBodyTube tube;
    lay(tube, -0.1f, 0.0f, -0.1f, kWide);
    lay(tube, 0.1f, 0.0f, 0.1f, kWide);
    lay(tube, -0.02f, 2.0f, 0.0f, kThin);
    lay(tube, 0.02f, 2.0f, 0.0f, kThin);
    return tube;
}

/** One instance record with its origin at (x, y, z) and an identity basis. */
std::vector<float> record_at(float x, float y, float z) {
    return {1.0f, 0.0f, 0.0f, x, 0.0f, 1.0f, 0.0f, y, 0.0f, 0.0f, 1.0f, z};
}

}  // namespace

TEST_CASE("the card takes over where the widest ring is thinner than one pixel") {
    const PlantImpostorFrame frame = impostor_of(test_tube(), {}, kViewHeight, kTanHalfFov);
    CHECK(frame.reach == doctest::Approx(kWide * kViewHeight / kTanHalfFov));
    // at that distance the widest ring spans exactly one pixel: 2 w H / (2 d tan)
    CHECK(2.0f * kWide * kViewHeight / (2.0f * frame.reach * kTanHalfFov) == doctest::Approx(1.0f));
}

TEST_CASE("the box and the texture follow the tube and the origins of the instance records") {
    const std::vector<float> leaves = record_at(0.9f, 2.5f, -0.3f);
    const PlantImpostorFrame frame = impostor_of(test_tube(), {&leaves}, kViewHeight, kTanHalfFov);
    CHECK(frame.width == doctest::Approx(1.0f));
    CHECK(frame.height == doctest::Approx(2.5f));
    CHECK(frame.depth == doctest::Approx(0.4f));
    CHECK(frame.centre_x == doctest::Approx(0.4f));
    CHECK(frame.centre_y == doctest::Approx(1.25f));
    CHECK(frame.centre_z == doctest::Approx(-0.1f));
    // a length L spans L / (2 w) pixels at the reach
    CHECK(frame.texture_width == 5u);
    CHECK(frame.texture_height == 13u);
}

TEST_CASE("a body without a ring, a view without height or without a field of view gets no card") {
    CHECK(impostor_of(PlantBodyTube{}, {}, kViewHeight, kTanHalfFov).reach == 0.0f);
    CHECK(impostor_of(test_tube(), {}, 0.0f, kTanHalfFov).reach == 0.0f);
    CHECK(impostor_of(test_tube(), {}, kViewHeight, 0.0f).reach == 0.0f);
}

TEST_CASE("a missing row buffer is passed over") {
    const PlantImpostorFrame frame = impostor_of(test_tube(), {nullptr}, kViewHeight, kTanHalfFov);
    CHECK(frame.height == doctest::Approx(2.0f));
    CHECK(frame.texture_height == 10u);
}

}  // namespace ase::adp::godot

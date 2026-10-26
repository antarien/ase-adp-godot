/**
 * ASE ADAPTER TESTS - THE SELECTED PATCH WITHOUT GODOT
 *
 * @file        test_godot_patch_focus.cpp
 * @design      DSGN_066
 * @brief       Pins which place the camera looks at, from where, and how far the zoom brings it
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2 and V2, the part of the care flow of the Plant
 *              Keeper that carries no Godot type: the place the camera looks at (the tapped one, without a
 *              tap the first of the start configuration), the near view onto its stem base, and the zoom the
 *              mouse wheel or a pinch moves between the seedling and the whole ground (godot_patch_focus.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100386 - select a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kFirstPlace = 0u;     // the first place of the start configuration
constexpr uint32_t kSelectedPlace = 2u;  // a place the keeper tapped

}  // namespace

TEST_CASE("select a patch: the view looks at the tapped place, without a tap at the first, from the near view of the configuration") {
    CHECK(patch_focus_place(kSelectedPlace, kFirstPlace) == kSelectedPlace);
    CHECK(patch_focus_place(GODOT_NO_PLACE, kFirstPlace) == kFirstPlace);

    PlantSceneCamera camera;
    camera.distance_m = 6.0f;
    camera.height_m = 1.6f;
    camera.look_m = 2.5f;
    PlantOrigin origin;
    origin.x = 16.0f;
    origin.y = 0.4f;
    origin.z = 16.0f;
    const PatchFocusAim aim = patch_focus_aim(camera, origin);
    // the camera stands south of the stem base (z smaller), above it, and looks at a point above the stem base
    CHECK(aim.eye_x == doctest::Approx(16.0f));
    CHECK(aim.eye_y == doctest::Approx(2.0f));
    CHECK(aim.eye_z == doctest::Approx(10.0f));
    CHECK(aim.target_x == doctest::Approx(16.0f));
    CHECK(aim.target_y == doctest::Approx(2.9f));
    CHECK(aim.target_z == doctest::Approx(16.0f));
}

TEST_CASE("select a patch: the zoom brings the camera to the seedling and back to the whole ground, held at its bounds") {
    // one step towards the birch scales by GODOT_ZOOM_STEP, one step away undoes it
    const float closer = patch_focus_zoom(GODOT_ZOOM_START, 1);
    CHECK(closer == doctest::Approx(GODOT_ZOOM_START * GODOT_ZOOM_STEP));
    CHECK(patch_focus_zoom(closer, -1) == doctest::Approx(GODOT_ZOOM_START));
    // held at both bounds, however far the wheel turns
    CHECK(patch_focus_zoom(GODOT_ZOOM_START, 100) == doctest::Approx(GODOT_ZOOM_MIN));
    CHECK(patch_focus_zoom(GODOT_ZOOM_START, -100) == doctest::Approx(GODOT_ZOOM_MAX));
    CHECK(patch_focus_zoom(GODOT_ZOOM_START, 0) == doctest::Approx(GODOT_ZOOM_START));
    // the zoomed camera keeps its angle and field of view: distance, height and look point scale together
    PlantSceneCamera camera;
    camera.distance_m = 10.0f;
    camera.height_m = 7.0f;
    camera.look_m = 0.5f;
    camera.fov_deg = 55.0f;
    const PlantSceneCamera near_view = patch_focus_zoomed(camera, GODOT_ZOOM_MIN);
    CHECK(near_view.distance_m == doctest::Approx(10.0f * GODOT_ZOOM_MIN));
    CHECK(near_view.height_m == doctest::Approx(7.0f * GODOT_ZOOM_MIN));
    CHECK(near_view.look_m == doctest::Approx(0.5f * GODOT_ZOOM_MIN));
    CHECK(near_view.fov_deg == doctest::Approx(55.0f));
    CHECK(near_view.height_m / near_view.distance_m == doctest::Approx(camera.height_m / camera.distance_m));
}

}  // namespace ase::adp::godot

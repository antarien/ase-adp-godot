/**
 * ASE ADAPTER TESTS - KEEPER FLOW WITHOUT GODOT
 *
 * @file        test_godot_keeper_flow.cpp
 * @design      DSGN_066
 * @brief       Pins the return to the garden, the focused patch and the seed of every sowing
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 and 03.6, the parts of the care flow of the Plant Keeper
 *              that carry no Godot type: which frame notices a span the app spent away and which one ends
 *              its catch-up, with the seconds it took (usecase:100390 return to the garden, V6); which
 *              place the camera looks at and from where (usecase:100386 select a patch); and the seed the
 *              first sowing and every Reiniciar sow (usecase:100388 sow a new seed, V7).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_garden_return.hpp>
#include <ase/adp/godot/godot_new_seed.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr double kFrameS = 0.016;        // an ordinary frame, well below GODOT_CATCH_UP_NOTE_S
constexpr double kAwayS = 10.0;          // ten seconds the app spent in the background (S5 of the Android tests)
constexpr double kBacklogS = 8.0;        // what the host still holds after the frame that brought them
constexpr uint32_t kStartSeed = 1u;      // the seed of the start configuration's birch (vivarium_start.json)
constexpr uint32_t kFirstPlace = 0u;     // the first place of the start configuration
constexpr uint32_t kSelectedPlace = 2u;  // a place the keeper tapped

}  // namespace

TEST_CASE("return to the garden: the frame that brings the time away is noticed, the end of its catch-up carries the seconds") {
    GardenReturn state;

    SUBCASE("ordinary frames without a return say nothing") {
        const GardenReturnFrame frame = garden_return_frame(state, 1.0, kFrameS, 0.0);
        CHECK_FALSE(frame.noticed);
        CHECK_FALSE(frame.caught_up);
        CHECK_FALSE(state.catching_up);
    }

    SUBCASE("a return whose backlog drains over the next frames ends with the time from its start") {
        const GardenReturnFrame start = garden_return_frame(state, 20.0, kAwayS, kBacklogS);
        CHECK(start.noticed);
        CHECK_FALSE(start.caught_up);
        CHECK(state.catching_up);

        const GardenReturnFrame middle = garden_return_frame(state, 20.5, kFrameS, kBacklogS / 2.0);
        CHECK_FALSE(middle.noticed);
        CHECK_FALSE(middle.caught_up);

        const GardenReturnFrame end = garden_return_frame(state, 21.25, kFrameS, 0.0);
        CHECK_FALSE(end.noticed);
        CHECK(end.caught_up);
        CHECK(end.catch_up_s == doctest::Approx(1.25));
        CHECK_FALSE(state.catching_up);
    }

    SUBCASE("a return the host caught up within its own frame is noticed and has no end of its own") {
        const GardenReturnFrame frame = garden_return_frame(state, 20.0, kAwayS, 0.0);
        CHECK(frame.noticed);
        CHECK_FALSE(frame.caught_up);
        CHECK_FALSE(state.catching_up);
    }

    SUBCASE("a second long frame keeps the start, and a long frame that empties the backlog ends it") {
        (void)garden_return_frame(state, 20.0, kAwayS, kBacklogS);
        const GardenReturnFrame again = garden_return_frame(state, 22.0, kAwayS, kBacklogS);
        CHECK(again.noticed);
        CHECK(state.since_s == doctest::Approx(20.0));

        const GardenReturnFrame end = garden_return_frame(state, 24.0, kAwayS, 0.0);
        CHECK(end.noticed);
        CHECK(end.caught_up);
        CHECK(end.catch_up_s == doctest::Approx(4.0));
    }
}

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

TEST_CASE("sow a new seed: the first sowing takes the configured seed, every Reiniciar a new one, the same series again the same") {
    CHECK(sown_seed(kStartSeed, GODOT_SOWING_FIRST) == kStartSeed);

    const uint32_t second = sown_seed(kStartSeed, GODOT_SOWING_FIRST + 1u);
    const uint32_t third = sown_seed(kStartSeed, GODOT_SOWING_FIRST + 2u);
    CHECK(second != kStartSeed);
    CHECK(third != kStartSeed);
    CHECK(third != second);

    // the same sowing of the same start seed grows the same birch again
    CHECK(sown_seed(kStartSeed, GODOT_SOWING_FIRST + 1u) == second);
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - RETURN TO THE GARDEN WITHOUT GODOT
 *
 * @file        test_godot_keeper_flow.cpp
 * @design      DSGN_066
 * @brief       Pins the return to the garden: which frame notices the time away and which one ends its catch-up
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 and 03.6, V6: the part of the care flow of the Plant Keeper
 *              that carries no Godot type - which frame notices a span the app spent away and which one ends
 *              its catch-up, with the seconds it took (godot_garden_return.hpp). The selected patch and its
 *              zoom stand in test_godot_patch_focus.cpp, the seed of every sowing in test_godot_new_seed.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-26
 * @version     00.00.02.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_garden_return.hpp>
#include <ase/adp/godot/types.hpp>

namespace ase::adp::godot {

namespace {

constexpr double kFrameS = 0.016;        // an ordinary frame, well below GODOT_CATCH_UP_NOTE_S
constexpr double kAwayS = 10.0;          // ten seconds the app spent in the background (S5 of the Android tests)
constexpr double kBacklogS = 8.0;        // what the host still holds after the frame that brought them

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

}  // namespace ase::adp::godot

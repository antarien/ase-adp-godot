/**
 * ASE ADAPTER TESTS - THE CONTEXT OF THE HUD WITHOUT GODOT
 *
 * @file        test_godot_hud_context.cpp
 * @design      DSGN_067
 * @brief       Pins which chips of the HUD show, in which slot, and which one draws as running
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.2: a chip that acts on a patch shows once a place
 *              is selected, a chip of the debug layer only while debug is on, visible chips take contiguous
 *              slots in list order, and a hidden chip is never running (godot_hud_context.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_hud_context.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

// the chips in the order of the spine: time (always), water (on a patch), prune (on a patch), probe (debug)
std::vector<HudChipRule> spine_rules() {
    return {
        HudChipRule{false, false},
        HudChipRule{true, false},
        HudChipRule{true, false},
        HudChipRule{false, true},
    };
}

}  // namespace

TEST_CASE("hud context: without a place only the chips that need none show") {
    std::vector<HudChipShow> shown;
    hud_context_resolve(spine_rules(), HudViewState{}, shown);

    REQUIRE(shown.size() == 4u);
    CHECK(shown[0].visible);
    CHECK(shown[0].slot == 0u);
    CHECK_FALSE(shown[1].visible);
    CHECK(shown[1].slot == UINT32_MAX);
    CHECK_FALSE(shown[2].visible);
    CHECK_FALSE(shown[3].visible);
}

TEST_CASE("hud context: a selected place shows the patch chips in contiguous slots") {
    std::vector<HudChipShow> shown;
    HudViewState state;
    state.place_selected = true;
    hud_context_resolve(spine_rules(), state, shown);

    CHECK(shown[0].slot == 0u);
    CHECK(shown[1].visible);
    CHECK(shown[1].slot == 1u);
    CHECK(shown[2].visible);
    CHECK(shown[2].slot == 2u);
    CHECK_FALSE(shown[3].visible);
}

TEST_CASE("hud context: a debug chip shows only while debug is on and closes the gap of hidden chips") {
    std::vector<HudChipShow> shown;
    HudViewState state;
    state.debug_on = true;
    hud_context_resolve(spine_rules(), state, shown);

    CHECK(shown[0].slot == 0u);
    CHECK_FALSE(shown[1].visible);
    CHECK_FALSE(shown[2].visible);
    CHECK(shown[3].visible);
    CHECK(shown[3].slot == 1u);
}

TEST_CASE("hud context: the running chip draws as running only while it is visible") {
    std::vector<HudChipShow> shown;
    HudViewState state;
    state.place_selected = true;
    state.running_chip = 1u;
    hud_context_resolve(spine_rules(), state, shown);

    CHECK(shown[1].running);
    CHECK_FALSE(shown[0].running);
    CHECK_FALSE(shown[2].running);

    state.place_selected = false;
    hud_context_resolve(spine_rules(), state, shown);
    CHECK_FALSE(shown[1].visible);
    CHECK_FALSE(shown[1].running);
}

TEST_CASE("hud context: an empty chip list shows nothing") {
    std::vector<HudChipShow> shown(3u);
    hud_context_resolve({}, HudViewState{}, shown);
    CHECK(shown.empty());
}

}  // namespace ase::adp::godot

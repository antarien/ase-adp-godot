/**
 * ASE ADAPTER TESTS - PLANT VIEW WITHOUT GODOT
 *
 * @file        test_godot_plant_view.cpp
 * @design      DSGN_066
 * @brief       Pins where the plant stands in the scene and how many orders the frame time lets the view draw
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 and 03.6, the parts of the care view that carry no Godot
 *              type: the stem base taken from the patch record (plant_origins), and the line between fine and
 *              coarse cylinder the frame time moves (godot_order_budget.hpp). The instance records stand in
 *              test_godot_plant_instances.cpp, the bark in test_godot_plant_bark.cpp, the organ templates in
 *              test_godot_plant_templates.cpp, the condition of the bed in test_godot_plant_stand.cpp, the
 *              asset binding in test_godot_asset_binding.cpp, the ground in test_godot_plant_ground.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-26
 * @version     00.00.09.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_order_budget.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kPatch = 7u;       // object id of the patch the test plant stands on
constexpr uint32_t kOtherPatch = 8u;  // a patch whose disc is not levelled yet

}  // namespace

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

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - CONDITION OF THE BED WITHOUT GODOT
 *
 * @file        test_godot_plant_stand.cpp
 * @design      DSGN_066
 * @brief       Pins that soil water and condition of a patch reach its materials as the record carried them
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2: plant_stands takes the soil water over field capacity and
 *              the condition code of every patch record, and a value the record did not carry stays unknown, so
 *              the material draws its shader's default, never a value made up (godot_plant_geometry.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101340 - show the bed condition
 * @actor       Application Layer Perception Control (ALPCTL)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kPatch = 7u;       // object id of a measured patch
constexpr uint32_t kOtherPatch = 8u;  // object id of a patch before the first runs of the modules

}  // namespace

TEST_CASE("03.2: soil water and condition of a patch reach its materials as the record carried them") {
    // a measured stand: soil water over field capacity and a wilted condition
    VivariumPatchView patch;
    patch.object_id = kPatch;
    patch.soil_rel = 0.62;
    patch.condition = GODOT_COND_WILTED;
    // a stand before the first runs of the modules: no soil_rel (NaN), a condition outside the codes
    VivariumPatchView unmeasured = patch;
    unmeasured.object_id = kOtherPatch;
    unmeasured.soil_rel = std::nan("");
    unmeasured.condition = GODOT_CODE_UNKNOWN;
    const auto stands = plant_stands({patch, unmeasured});
    REQUIRE(stands.size() == 2u);
    // as the record carried them, narrowed to the float of a shader parameter and nothing else
    const PlantStand& wilted = stands.at(kPatch);
    CHECK(wilted.soil_known);
    CHECK(wilted.soil_rel == static_cast<float>(0.62));
    CHECK(wilted.condition_known);
    CHECK(wilted.condition == static_cast<float>(GODOT_COND_WILTED));
    // what the record did not carry stays unknown: the shader's default draws, never a value made up
    const PlantStand& open = stands.at(kOtherPatch);
    CHECK_FALSE(open.soil_known);
    CHECK_FALSE(open.condition_known);
    // every documented code passes, the first one past them does not
    for (uint8_t code = 0u; code <= GODOT_COND_COUNT; ++code) {
        VivariumPatchView coded = patch;
        coded.condition = code;
        CHECK(plant_stands({coded}).at(kPatch).condition_known == (code < GODOT_COND_COUNT));
    }
    // a default stand is an unknown one - the ground of a place without a record
    const PlantStand none;
    CHECK_FALSE(none.soil_known);
    CHECK_FALSE(none.condition_known);
    CHECK(plant_stands({}).empty());
}

}  // namespace ase::adp::godot

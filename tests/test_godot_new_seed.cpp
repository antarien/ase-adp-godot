/**
 * ASE ADAPTER TESTS - THE SEED OF EVERY SOWING WITHOUT GODOT
 *
 * @file        test_godot_new_seed.cpp
 * @design      DSGN_066
 * @brief       Pins the seed the first sowing and every Reiniciar sow
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2 and V7, the part of the care flow of the Plant
 *              Keeper that carries no Godot type: the first sowing takes the seed of the start configuration,
 *              every Reiniciar a new one, and the same series of sowings grows the same birches again
 *              (godot_new_seed.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100388 - sow a new seed
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_new_seed.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kStartSeed = 1u;  // the seed of the start configuration's birch (vivarium_start.json)

}  // namespace

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

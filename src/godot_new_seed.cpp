/**
 * ASE GODOT ADAPTER - SOW A NEW SEED
 *
 * @file        godot_new_seed.cpp
 * @brief       The seed a patch is sown with: the configured one first, a new one with every Reiniciar
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2 and V7, see godot_new_seed.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    biology/lifecycle/reproduction
 * @usecase     usecase:100388 - sow a new seed
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-12
 * @modified    2026-10-12
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_new_seed.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/random.hpp>

namespace ase::adp::godot {

uint32_t sown_seed(uint32_t start_seed, uint32_t sowing) {
    if (sowing == GODOT_SOWING_FIRST) {
        return start_seed;
    }
    return ase::math::random_hash_u32(start_seed, sowing);
}

}  // namespace ase::adp::godot

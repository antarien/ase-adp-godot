#pragma once

/**
 * ASE GODOT ADAPTER - SOW A NEW SEED
 *
 * @file        godot_new_seed.hpp
 * @brief       The seed a patch is sown with: the configured one first, a new one with every Reiniciar
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2 and V7: Reiniciar sows the patch anew, and a new
 *              birch grows from seed - the same species in another shape. The shape of a plant is a function
 *              of its seed (the same seed grows the same birch, vivarium_start.json "_patches"), so a sowing
 *              that took the configured seed again would grow the same birch again. The first sowing of a
 *              view takes the seed of the start configuration, every later one a seed drawn from it and the
 *              number of the sowing (ase-math random_hash_u32): the same series of sowings grows the same
 *              series of birches, and every sowing a birch of its own.
 *
 *              THE SEED IS THE KEEPER'S INPUT, NOT A COMPUTED PLANT: the adapter only chooses the number the
 *              game host lays the patch with; the plant grows in the modules from it.
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

#include <cstdint>

namespace ase::adp::godot {

/**
 * The seed a patch is sown with in sowing number `sowing`: GODOT_SOWING_FIRST takes `start_seed` as the start
 * configuration names it, every later sowing random_hash_u32(start_seed, sowing).
 */
[[nodiscard]] uint32_t sown_seed(uint32_t start_seed, uint32_t sowing);

}  // namespace ase::adp::godot

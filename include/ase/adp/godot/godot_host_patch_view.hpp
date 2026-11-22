#pragma once

/**
 * ASE GODOT ADAPTER - HOST PATCH VIEW
 *
 * @file        godot_host_patch_view.hpp
 * @brief       One patch of the embedded host as the last snapshot showed it: stand, stage, condition, soil and place
 * @description The copy GodotHostResourceManager keeps of every patch of the patch port, joined with the stage,
 *              condition, forecast and stem records of ase-pl-flora. One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that
 *              draw or read a patch read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/** One patch as the last snapshot showed it - a copy, never a reference into the App. */
struct VivariumPatchView {
    uint32_t object_id = 0;     // the patch, as the plugin names it on its port
    uint32_t place = 0;         // where the view draws it
    double   coverage = 0.0;    // covered share, 0..1
    uint8_t  stage = 0;         // position of the stage among the four codes of the stage record (seed, sprout, grow, mature; below GODOT_STAGE_COUNT) or GODOT_CODE_UNKNOWN - never a code of ase-plant (usecase:101635, D2455)
    bool     seed = false;      // stage equals stage_seed of the stage record: a seed lies in the soil
    bool     sprout = false;    // stage equals stage_sprout: ase-plant marks a seedling
    bool     grow = false;      // stage equals stage_grow: ase-plant marks a growing stand
    bool     mature = false;    // stage equals stage_mature: ase-plant marks a mature stand
    double   condition = 0.0;   // PLT_CONDITION as the patch record carried it, NaN unmeasured - a number of ase-plant, never compared against one of the view's own
    bool     cond_known = false; // the condition record came and condition equals one of its three codes (usecase:101340, D2454)
    bool     wilted = false;    // condition equals cond_wilted: ase-plant marks the stand wilted
    bool     dead = false;      // condition equals cond_dead: ase-plant marks the stand dead
    uint8_t  soil = 0;          // GODOT_SOIL_* ground or GODOT_CODE_UNKNOWN
    double   soil_mm = 0.0;     // soil water, mm
    double   soil_rel = 0.0;    // soil water over field capacity, 0..1
    double   capacity_mm = 0.0; // field capacity, mm
    double   root_m = 0.0;      // reach of the farthest living root tip of the patch's body, metres: ase-radix's RDX_SPREAD as ase-pl-flora files it (FloraInpSoilComponent.spread_m); the radius of the root zone, 0 while no tip lives, NaN before the record carried it
    double   grow_at_h = 0.0;   // game hour ase-plant forecasts the stand to reach CRESCENDO (PLT_GROW_AT_H, forecast port), NaN unmeasured
    double   mtur_at_h = 0.0;   // game hour ase-plant forecasts the stand to reach MADURA (PLT_MTUR_AT_H, forecast port), NaN unmeasured
    double   x = 0.0;           // stem base, world metres east: the centre of the patch square
    double   y = 0.0;           // stem base, world metres up: the height ase-terrain levelled the patch to
    double   base_y = 0.0;      // stem foot, metres above y: where ase-physics holds the stem body (stem port), 0 while no record came
    double   autumn = 0.0;      // autumn colour of the individual of the patch, 0 green at the start of autumn to 1 at its own leaf fall (PHY_LEAF_AUTUMN, autumn port, usecase:101646 colour the leaves in autumn), NaN while no record came
    double   z = 0.0;           // stem base, world metres north: the centre of the patch square
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - THE POUR WAITING FOR ITS ARRIVAL
 *
 * @file        godot_vivarium_water_pending.hpp
 * @brief       The pour of Regar between its request and the frame the patch record shows the water arrived
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1, sequence V3: a pour goes out through the patch port or
 *              the drain port of the can, and the soil water of the patch rises only once the host has run its
 *              Dissemination. Between the two the view holds what it sent - which patch, its soil water before,
 *              the host's steps at that moment, the amount and whether the can emptied - so follow_water
 *              (godot_vivarium_water.cpp) can tell the arrival or its absence after GODOT_WATER_WAIT_STEPS.
 *
 *              It stands apart from the view because it is one responsibility with its own life: set by a
 *              pour, read by the frames after it, cleared at the arrival, the timeout or a restart. The view
 *              holds one of it.
 *
 *              NO GODOT TYPE: plain values only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0517 (the column of the can's pour, laid by its owner)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/host/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

/** A pour that went out and whose arrival the patch record has not shown yet; place GODOT_NO_PLACE: none waits. */
struct VivariumWaterPending {
    uint32_t place = GODOT_NO_PLACE;  // the watered patch, its arrival not yet seen
    double   from = 0.0;              // its soil water when watered, mm
    uint64_t steps = 0;               // Dissemination runs when watered
    double   mm = 0.0;                // the amount poured, mm; 0 where the owner lays it (a can's drain, N0517)
    bool     empty = false;           // the HUD's can emptied with this pour: toast.watered_empty
};

}  // namespace ase::adp::godot

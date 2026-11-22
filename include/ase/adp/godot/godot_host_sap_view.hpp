#pragma once

/**
 * ASE GODOT ADAPTER - HOST SAP VIEW
 *
 * @file        godot_host_sap_view.hpp
 * @brief       The tap in the birch and its bark vessel as the last snapshot of the sap port showed them
 * @description The copy GodotHostResourceManager keeps of the one record of flora.sap.v1 (PLAN_ASE_VIVARIUM_PHASE_06_HUD
 *              06.8, register N0457 under carrier N0448): whether the tap runs, the litres the vessel holds now and
 *              the litres it holds when full. ase-pl-flora files the record; the HUD's sap readers (FocusCluster,
 *              TapHole, SignMarker, HandSlot) take it once it stands, the bridge's SHA values before. One purpose of
 *              the dissolved godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units
 *              that draw the sap read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @register    N0457 (flora.sap.v1), carrier N0448
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

namespace ase::adp::godot {

/** The tap and its vessel of flora.sap.v1; `known` stays false while the port is not offered or files no record. */
struct VivariumSapView {
    bool known = false;     // a complete record stood in the last snapshot
    bool running = false;   // the tap runs: key running, 1 or 0
    double held_l = 0.0;    // litres in the bark vessel now, from 0 (key held_l)
    double cap_l = 0.0;     // litres the vessel holds when full, above 0 (key cap_l)
};

}  // namespace ase::adp::godot

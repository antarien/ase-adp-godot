/**
 * ASE GODOT ADAPTER - HUD SPINE, THE PART WITHOUT A GODOT TYPE
 *
 * @file        godot_hud_spine_pure.cpp
 * @brief       Where the values of the plant card end while the body rail draws beside them
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, register B0937, see godot_hud_spine_pure.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.39.00039 [core]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_spine_pure.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

double spine_value_end(double value_x, bool rail, double rail_left) {
    return rail ? ase::math::min(value_x, rail_left) : value_x;
}

double spine_bar_width(double value_end, double text_x) {
    return ase::math::max(value_end - text_x, 0.0);
}

double spine_column_x(double column_x, bool rail, double rail_left, double need) {
    return rail ? ase::math::min(column_x, rail_left - need) : column_x;
}

}  // namespace ase::adp::godot

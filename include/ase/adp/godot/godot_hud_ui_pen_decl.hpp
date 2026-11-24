#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI PEN DECLARATION
 *
 * @file        godot_hud_ui_pen_decl.hpp
 * @brief       The name UiPen for a header that only passes the pen on by reference
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiPen. A block's header names the pen in its
 *              draw signature as UiPen&; it needs the name, never the class. Cut from godot_hud_ui_pen.hpp by its
 *              circle of readers (header ratchet, WRFL_ASE_TIER_WIPE rule 1): a header that includes this one makes
 *              no unit compile again when the pen's class changes, only the units that draw with it
 *              (godot_hud_ui_pen_class.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

namespace ase::adp::godot {

class UiPen;

}  // namespace ase::adp::godot

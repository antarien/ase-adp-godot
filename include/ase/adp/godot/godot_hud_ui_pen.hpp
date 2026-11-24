#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI PEN
 *
 * @file        godot_hud_ui_pen.hpp
 * @brief       Collector of the HUD pen's sub-headers: its name, its measures and its class
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiPen. More than a hundred own units of the
 *              avt build included this header, so it takes no line of its own (header ratchet, operator decision
 *              2026-11-21, WRFL_ASE_TIER_WIPE rules 1 to 6). It is dedicated by purpose:
 *                godot_hud_ui_pen_decl.hpp    the name UiPen, for a header that passes the pen on by reference
 *                pen/types.hpp                the pen's measures in screen pixels, read by the pen, its cover
 *                                             and the joined stroke
 *                godot_hud_ui_pen_class.hpp   the class, for a unit that draws with the pen
 *              Every reader includes the sub-header of its purpose directly, never this collector.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-11
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <ase/adp/godot/pen/types.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

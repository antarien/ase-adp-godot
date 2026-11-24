#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI TEXT BLOCK
 *
 * @file        godot_hud_ui_text_block.hpp
 * @brief       Collector of the HUD text line's sub-headers: the place of a line and the line's class
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiTextBlock. More than a hundred own units of
 *              the avt build included this header, so it takes no line of its own (header ratchet, operator
 *              decision 2026-11-21, WRFL_ASE_TIER_WIPE rules 1 to 6). It is dedicated by purpose:
 *                godot_hud_ui_text_place.hpp  UiTextPlace, where and how a block lays its line this frame
 *                godot_hud_ui_text_class.hpp  UiTextBlock with UiTextGlyph and UiTextIcon, for a block that holds
 *                                             and draws a line
 *                godot_hud_ui_icon.hpp        the icon's collector, which this header carried before the cut: the
 *                                             glyph's measures and hud_raster its readers reach through it
 *              Every reader includes the sub-header of its purpose directly, never this collector.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_text_place.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>

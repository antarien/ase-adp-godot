#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI ICON
 *
 * @file        godot_hud_ui_icon.hpp
 * @brief       Collector of the HUD icon's sub-headers: the glyph's measures, its raster size and the icon's class
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, building block Basis UiIcon. More than a hundred own
 *              units of the avt build included this header, so it takes no line of its own (header ratchet,
 *              operator decision 2026-11-21, WRFL_ASE_TIER_WIPE rules 1 to 6). It is dedicated by purpose:
 *                glyph/types.hpp              the oversampling and the design size of a HUD glyph at the text
 *                                             server, read by the icon and the line of text
 *                godot_hud_ui_raster.hpp      hud_raster, the whole screen pixel size a glyph is rastered at
 *                godot_hud_ui_icon_class.hpp  the class UiIcon, for a block that holds or draws an icon
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

#include <ase/adp/godot/glyph/types.hpp>
#include <ase/adp/godot/godot_hud_ui_raster.hpp>
#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>

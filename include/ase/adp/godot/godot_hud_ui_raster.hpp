#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI RASTER
 *
 * @file        godot_hud_ui_raster.hpp
 * @brief       The whole screen pixel size a HUD glyph is rastered at under the device transform
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, building block Basis. A glyph of the HUD is rastered at
 *              the nearest whole screen pixel size the device transform (the pen's screen() times the frame) gives
 *              its size in mockup units, and drawn 1:1, never resampled: the rule the icon (UiIcon) and the line
 *              of text (UiTextBlock) share. Cut from godot_hud_ui_icon.hpp by its circle of readers (header
 *              ratchet, WRFL_ASE_TIER_WIPE rule 1): the icon, the line and the hourglass's digits read it.
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

#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>

namespace ase::adp::godot {

/**
 * The whole raster size a glyph of `size` mockup units gets under `device`, the transform from the mockup's units
 * into the screen's pixels (UiPen::screen() times the frame): never magnified, never blurred.
 */
int64_t hud_raster(const ::godot::Transform2D& device, double size);

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI PEN COVER
 *
 * @file        godot_hud_ui_pen_cover.hpp
 * @brief       The coverage rule of the HUD's strokes: the core Godot draws and the feather it attaches to it
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_pen.hpp COVERAGE. Godot antialiases a
 *              line, a polyline and a circle with a feather of GODOT_PEN_FEATHER_PX outside the core it is handed;
 *              the pen hands it the core whose core plus feather covers the nominal width, so the ramp lies centred
 *              on the nominal edge. UiPen and the stroke mesh (godot_hud_ui_stroke.hpp) both cover by this one
 *              rule; it is defined in godot_hud_ui_pen_cover.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    B0678
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

namespace ase::adp::godot {

/**
 * The core Godot draws for a stroke of `width` screen pixels so that core plus feather covers `width`; a width below 0
 * stays the hairline.
 */
float pen_core(float width);

/** The feather Godot attaches to a line's or polyline's core of `core` screen pixels, at each side and end. */
float pen_feather(float core);

}  // namespace ase::adp::godot

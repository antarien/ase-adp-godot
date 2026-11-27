#pragma once

/**
 * ASE GODOT ADAPTER - HUD SPINE, THE PART WITHOUT A GODOT TYPE
 *
 * @file        godot_hud_spine_pure.hpp
 * @brief       Where the values of the plant card end while the body rail draws beside them, callable by the host test
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, register B0937. The values of the plant card are
 *              end-anchored at hud_focus_row_value_x, the line of the body rail CORPO stands at the same x, and the
 *              rail's content (track, name, number) lies left of its line from hud_survival_rail_track_left on. While
 *              the rail draws in the same view, the card yields the rail's column: its values and the bars under them
 *              end at the rail's content, never over it. Both measures are SHA's; this file only takes the smaller one.
 *              The units that draw hold Godot types; this file holds none, so ase-adp-godot-context-test runs it
 *              without a Godot process.
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

namespace ase::adp::godot {

/**
 * The x the values of the card end at: `value_x` (hud_focus_row_value_x) while the body rail does not draw, and the
 * smaller of `value_x` and `rail_left` (hud_survival_rail_track_left) while it draws.
 */
double spine_value_end(double value_x, bool rail, double rail_left);

/** The width of a row's bar from the text column `text_x` to the end of the values; 0 where the column stands past it. */
double spine_bar_width(double value_end, double text_x);

/**
 * The column the whole card stands at (B1001): `column_x` (ring centre plus hud_focus_column_dx) while the body rail
 * does not draw; while it draws, the card yields left until its widest text, `need` measured from the column to its
 * right end, ends at `rail_left` (hud_survival_rail_track_left), and never moves right.
 */
double spine_column_x(double column_x, bool rail, double rail_left, double need);

}  // namespace ase::adp::godot

#pragma once
/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the measures of the HUD pen in screen pixels
 * @description All compile-time constants of the pen sub-type; NO runtime values - those belong in the pen.
 *              The longest chord of a circle the pen divides, the rim along an opaque polygon, Godot's antialiasing
 *              feather and the test of a uniform scale. Read by the pen (godot_hud_ui_pen.cpp), its cover
 *              (godot_hud_ui_pen_cover.cpp) and the joined stroke (godot_hud_ui_stroke.cpp).
 *
 *              WHY A TYPES HEADER OF ITS OWN: godot_hud_ui_pen.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These measures have three readers,
 *              so a change to one compiles those units again, never every unit that draws.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: PX = a screen pixel, the unit the pen renders every shape in; EPS = the tolerance of a comparison.
 */

/**
 * Measures of UiPen - in screen pixels, after screen() times the transform the caller set.
 */
constexpr double GODOT_PEN_SEGMENT_PX = 2.0;     // longest chord, in screen pixels, of a circle or an arc the pen divides
constexpr float GODOT_PEN_RIM_PX = 1.0f;         // core width, in screen pixels, of the antialiased rim along an opaque polygon's drawn-in outline
constexpr float GODOT_PEN_FEATHER_PX = 1.25f;    // Godot's antialiasing feather outside a line's, a polyline's and a circle's nominal edge (renderer_canvas_cull.cpp FEATHER_SIZE), screen pixels
constexpr float GODOT_PEN_UNIFORM_EPS = 1.0e-4f; // largest difference of the axes' lengths and of their dot product that still counts as one scale

}  // namespace ase::adp::godot

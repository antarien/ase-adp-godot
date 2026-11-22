#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI SVG PAINT
 *
 * @file        godot_hud_ui_svg_paint.hpp
 * @brief       Fill, stroke and dashed stroke of a flattened outline, as SVG paints a path
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3. The outlines of godot_hud_ui_svg_path.hpp are
 *              painted here: a block that draws a shape of its own from tokens (the leather pack, the linen
 *              pouch, the selection frame of a thing) and the leader line's dashed form go through these three,
 *              so every HUD stroke-dasharray runs one rule.
 *
 *              fill            the closed outline as one polygon in the colour
 *              stroke          the outline as a polyline at the width, closed back onto its first point when
 *                              the outline is closed
 *              stroke round    an open outline with stroke-linecap round and stroke-linejoin round as one mesh
 *                              (ui_stroke): every open stroke of the survival mockup (each L() line, GL.air,
 *                              GL.smoke) draws so; a closed one as stroke
 *              stroke joined   open or closed with stroke-linejoin round (ui_stroke): GL.health
 *              stroke dashed   stroke-dasharray dash gap along the same polyline; the pattern's phase carries
 *                              over each knee and over the closing segment, starting at the first point
 *
 *              The caller sets the canvas transform; every length is in its units.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>

namespace ase::adp::godot {

/** Fills the closed `outline` in `color`; an outline of fewer than GODOT_ITEM_POLYGON_MIN points draws nothing. */
void svg_fill(UiPen& canvas, const UiSvgOutline& outline, const ::godot::Color& color);

/** Strokes `outline` in `color` at `width`. */
void svg_stroke(UiPen& canvas, const UiSvgOutline& outline, const ::godot::Color& color, double width);

/**
 * Strokes `outline`, laid in the units of `frame`, with stroke-linecap round and stroke-linejoin round when it is
 * open (ui_stroke); a closed outline has no end and keeps the default miter of a path, as svg_stroke strokes it.
 */
void svg_stroke_round(UiPen& canvas, const ::godot::Transform2D& frame, const UiSvgOutline& outline,
                      const ::godot::Color& color, double width);

/**
 * Strokes `outline`, laid in the units of `frame`, with stroke-linejoin round, open or closed (ui_stroke): a path
 * whose style names stroke-linejoin round, as GL.health of the survival mockup.
 */
void svg_stroke_joined(UiPen& canvas, const ::godot::Transform2D& frame, const UiSvgOutline& outline,
                       const ::godot::Color& color, double width);

/**
 * The open polyline `points` in the units of `frame` in `color` at `width` with stroke-linecap round and
 * stroke-linejoin round (ui_stroke): the mockup's L(pts, o) of the survival HUD.
 */
void svg_polyline_round(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::PackedVector2Array& points,
                        const ::godot::Color& color, double width);

/** Strokes `outline` in `color` at `width`, `dash` on and `gap` off. */
void svg_stroke_dashed(UiPen& canvas, const UiSvgOutline& outline, const ::godot::Color& color,
                       double width, double dash, double gap);

/** The polyline `points` dashed in `color` at `width`, `dash` on and `gap` off, across its knees. */
void svg_dash_points(UiPen& canvas, const ::godot::PackedVector2Array& points,
                     const ::godot::Color& color, double width, double dash, double gap);

}  // namespace ase::adp::godot

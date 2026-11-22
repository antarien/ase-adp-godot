#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI STROKE
 *
 * @file        godot_hud_ui_stroke.hpp
 * @brief       A polyline stroked as the browser strokes a path with stroke-linejoin round: one mesh in screen pixels
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, survival rows U01 to U08 (L() of
 *              VIVARIUM_PHASE_08_SURVIVAL_HUD_V1 with stroke-linecap round and stroke-linejoin round, GL.health with
 *              stroke-linejoin round, GL.air and GL.smoke with stroke-linecap round).
 *
 *              Godot's polyline averages the normals at every knee: a sharp knee pinches and its outer corner is
 *              cut flat, where the browser's round join lays a disc of the width around the knee. ui_stroke lays the
 *              whole stroke as ONE mesh of triangles in screen pixels (canvas_item_add_triangle_array):
 *                core      along each segment half the core width to each side (pen_core of the width)
 *                knee      round around the outer side, divided as the pen divides an arc
 *                          (GODOT_PEN_SEGMENT_PX); the inner sides meet in one point, the fan of the outer side
 *                          laid from it
 *                end       an open stroke ends in a half disc around its end point (stroke-linecap round); a
 *                          stroke of no length is the disc of its width
 *                feather   a band of pen_feather along the whole outline, full colour at the core and
 *                          transparent at its end: the coverage of the pen's polyline (COVERAGE)
 *              No piece covers another, so a translucent ink (OVERLAY_INK at 0.9, OVERLAY_INK_FAINT at 0.3) stays
 *              one layer deep. A knee whose inner sides cannot meet within half of each segment (a hairpin, a stub)
 *              lays both segments to the knee and the outer fan from the knee itself: there the two bodies overlap,
 *              as they do in the browser's shape.
 *
 *              The mesh goes onto the pen's canvas item under the inverse of screen(); the frame the caller set is
 *              set again after it, so the next shape of the caller lies where it did.
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

#include <ase/adp/godot/godot_hud_ui_pen.hpp>

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>

namespace ase::adp::godot {

constexpr float GODOT_STROKE_POINT_EPS_PX = 1.0e-3f;  // screen-pixel distance at which two points of a stroke count as one
constexpr float GODOT_STROKE_JOIN_EPS = 1.0e-4f;      // smallest 1 + cosine of a knee's turn at which its inner sides still meet in one point
constexpr float GODOT_STROKE_INNER_SHARE = 0.5f;      // share of each segment the inner meeting point of a knee may reach back along it

/**
 * The polyline `points` in the units of `frame` stroked at `width` in `color` with stroke-linejoin round and, when it
 * is open, stroke-linecap round; a closed one runs back onto its first point. A width below 0 is the canvas's
 * hairline, drawn as the pen draws it.
 */
void ui_stroke(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::PackedVector2Array& points,
               bool closed, const ::godot::Color& color, double width);

}  // namespace ase::adp::godot

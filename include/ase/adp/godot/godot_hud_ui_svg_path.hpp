#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI SVG PATH
 *
 * @file        godot_hud_ui_svg_path.hpp
 * @brief       SVG path data into flattened outlines, and the curve pieces every HUD drawing builds its shapes from
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3. The mockup draws its things and its containers as
 *              SVG paths. UiItemSprite reads a thing's path data of item_art.json through read_svg_path; a
 *              block that draws a shape of its own from tokens (the leather pack's flap and body, the linen
 *              pouch's neck and fold) builds the same outlines from svg_append, svg_append_quadratic and
 *              svg_append_cubic, so every curve of the HUD is flattened by one rule.
 *
 *              THE PATH DATA. The commands M L H V Q C A Z in absolute and relative form; a command letter may
 *              stand once for several coordinate groups, after M the further pairs are lines; a command after
 *              Z draws on from the start of the closed outline. A is the elliptical arc of SVG 1.1
 *              implementation notes F.6.5 and F.6.6, its radii scaled up when they cannot reach. Every curve is
 *              flattened into GODOT_ITEM_CURVE_POINTS points; a point that repeats the last one within
 *              GODOT_ITEM_POINT_EPSILON is dropped. Anything else refuses the path with the reason in `failed`.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** One outline of a path: a run of points, closed by Z or open. */
struct UiSvgOutline {
    ::godot::PackedVector2Array points;
    bool closed = false;
};

/** Reads the SVG path data `data` into `outlines`; false with the reason in `failed`. */
bool read_svg_path(const std::string& data, std::vector<UiSvgOutline>& outlines, std::string& failed);

/** Appends `point` to `outline` unless it repeats the last point. */
void svg_append(UiSvgOutline& outline, const ::godot::Vector2& point);

/** The quadratic Bezier from `from` over `control` to `to`, flattened into `outline` (from itself not appended). */
void svg_append_quadratic(UiSvgOutline& outline, const ::godot::Vector2& from, const ::godot::Vector2& control,
                          const ::godot::Vector2& to);

/** The cubic Bezier from `from` over `first` and `second` to `to`, flattened into `outline`. */
void svg_append_cubic(UiSvgOutline& outline, const ::godot::Vector2& from, const ::godot::Vector2& first,
                      const ::godot::Vector2& second, const ::godot::Vector2& to);

/** The SVG rect `rect` with corner radius rx = ry = `radius`, closed into `outline`. */
void svg_append_rounded_rect(UiSvgOutline& outline, const ::godot::Rect2& rect, float radius);

}  // namespace ase::adp::godot

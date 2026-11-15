#pragma once

/**
 * ASE GODOT ADAPTER - HUD FIGURE SWITCH
 *
 * @file        godot_hud_figure_switch.hpp
 * @brief       FigureSwitch - cam.mode, the small switch between the fixed view and the player camera
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.9 (VIVARIUM_PHASE_07_FIGURE_V1, cam.mode): a quiet switch of the
 *              form of the debug switch of zone Z5 (SkyBand, hud_band_debug_*), right under it at the right edge,
 *              in every camera. It does not hang at the figure: after the switch to the fixed view the figure may
 *              stand outside the picture, and a switch at it could never be reached again.
 *
 *              THE FORM. The glyph figure.cam_mode.glyph centred at (hud_figure_switch_x, hud_figure_switch_y),
 *              size hud_figure_switch_glyph_size. While the player camera stands it is lit: a hexagon of
 *              hud_figure_switch_radius filled FIGURE_SWITCH_GROUND and stroked DOMAIN_FLOW at
 *              hud_figure_switch_line, the glyph in DOMAIN_FLOW. While the fixed view stands the glyph alone in
 *              OVERLAY_INK at hud_figure_switch_off_alpha. Amber stays the debug's alone.
 *
 *              THE TOUCH. hit() is true inside the square of half side hud_figure_switch_hit round the centre
 *              (y 62..94 at the measures of SHA; the debug's square ends at y 60).
 *
 *              NOTHING IS SWITCHED HERE: the press goes through the switch port of ase-pl-avatar in
 *              godot_vivarium_figure_controls.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>

namespace ase::adp::godot {

/** cam.mode: the switch between the fixed view of Phase 06 and the player camera. */
class FigureSwitch {
public:
    /** Binds measures, colours and the glyph; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the switch under `frame`: lit while `player_camera` stands, dimmed while the fixed view stands. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, bool player_camera) const;

    /** True when `point` (in the mockup's units) lies in the square a tap takes the switch in. */
    bool hit(const ::godot::Vector2& point) const;

private:
    double x_ = 0.0;            // hud_figure_switch_x
    double y_ = 0.0;            // hud_figure_switch_y
    double radius_ = 0.0;       // hud_figure_switch_radius
    double line_ = 0.0;         // hud_figure_switch_line
    double glyph_size_ = 0.0;   // hud_figure_switch_glyph_size
    double hit_ = 0.0;          // hud_figure_switch_hit: half the side of the square
    double off_alpha_ = 0.0;    // hud_figure_switch_off_alpha

    ::godot::Color ground_;     // FIGURE_SWITCH_GROUND
    ::godot::Color flow_;       // DOMAIN_FLOW
    ::godot::Color ink_;        // OVERLAY_INK

    UiIcon glyph_;              // figure.cam_mode.glyph
};

}  // namespace ase::adp::godot

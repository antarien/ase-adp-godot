#pragma once

/**
 * ASE GODOT ADAPTER - HUD FIGURE TURN
 *
 * @file        godot_hud_figure_turn.hpp
 * @brief       FigureTurn - fig.turn, the eighth of a circle on the ground before the feet that turns the figure
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.7 (VIVARIUM_PHASE_07_FIGURE_V1, fig.turn, turnArc): no ring about
 *              the figure. An eighth of a circle lies on the ground before its feet, on the side of the camera, so it
 *              always stands at the bottom of the figure; at rest it is barely there. A finger that comes down on it
 *              and draws sideways turns the figure on the spot, and the arc lights while it is held; a tap on it opens
 *              the focus like a tap on the figure.
 *
 *              THE FORM. hud_figure_turn_segments + 1 points on the circle of hud_figure_turn_radius metres about
 *              the feet, hud_figure_turn_lift above them, spanning hud_figure_turn_span radians centred on the
 *              direction from the feet to the eye (atan2 of the eye's x and z off the feet), each projected through
 *              the anchor of this frame. A polyline in FIGURE_INK at hud_figure_turn_line with round ends, at
 *              hud_figure_turn_held_alpha while a finger holds it, hud_figure_turn_lit_alpha while the focus stands
 *              open, hud_figure_turn_rest_alpha otherwise. figure.turn.left_glyph ends hud_figure_turn_chevron_dx left
 *              of the first point, figure.turn.right_glyph starts as far right of the last, both
 *              hud_figure_turn_chevron_dy under them at hud_figure_turn_chevron_size, in the same ink and alpha.
 *
 *              THE TOUCH. hit() is true within hud_figure_turn_hit of any point of the arc as last laid. The way a
 *              finger draws sideways turns by hud_figure_turn_yaw_per_unit radians per mockup unit (yaw_per_unit()).
 *
 *              NOTHING IS TURNED HERE: the drag goes through the turn port of ase-pl-avatar in
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

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>

namespace ase::adp::godot {

/** fig.turn: the arc on the ground before the figure's feet. */
class FigureTurn {
public:
    /** Binds measures, the ink and the two chevrons; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /**
     * Lays the arc of this frame about `feet` (world metres) on the side of `eye` through `anchor`; false, and no
     * arc to draw or hit, when a point of it lies behind the camera.
     */
    bool lay(const UiAnchor& anchor, const ::godot::Vector3& feet, const ::godot::Vector3& eye);

    /** Forgets the arc: nothing is drawn or hit until the next lay. */
    void clear();

    /** Draws the arc as last laid under `frame`: held while a finger draws on it, lit while the focus stands open. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, bool lit, bool held);

    /** True when `point` (in the mockup's units) lies within the hit distance of a point of the arc. */
    bool hit(const ::godot::Vector2& point) const;

    /** The radians the figure turns per mockup unit a finger draws sideways on the arc. */
    double yaw_per_unit() const { return yaw_per_unit_; }

private:
    double radius_ = 0.0;           // hud_figure_turn_radius: metres off the feet
    double span_ = 0.0;             // hud_figure_turn_span: radians, the eighth of a circle
    double segments_ = 0.0;         // hud_figure_turn_segments: the arc's points are segments + 1
    double lift_ = 0.0;             // hud_figure_turn_lift: metres above the feet
    double line_ = 0.0;             // hud_figure_turn_line
    double rest_alpha_ = 0.0;       // hud_figure_turn_rest_alpha
    double lit_alpha_ = 0.0;        // hud_figure_turn_lit_alpha
    double held_alpha_ = 0.0;       // hud_figure_turn_held_alpha
    double chevron_size_ = 0.0;     // hud_figure_turn_chevron_size
    double chevron_dx_ = 0.0;       // hud_figure_turn_chevron_dx: off the arc's end, outward
    double chevron_dy_ = 0.0;       // hud_figure_turn_chevron_dy: the baseline under the arc's end
    double hit_ = 0.0;              // hud_figure_turn_hit
    double yaw_per_unit_ = 0.0;     // hud_figure_turn_yaw_per_unit

    ::godot::Color ink_;            // FIGURE_INK

    UiTextBlock left_;              // figure.turn.left_glyph, ends left of the first point
    UiTextBlock right_;             // figure.turn.right_glyph, starts right of the last point

    ::godot::PackedVector2Array points_;  // the arc as last laid, mockup units; empty: nothing to draw or hit
};

}  // namespace ase::adp::godot

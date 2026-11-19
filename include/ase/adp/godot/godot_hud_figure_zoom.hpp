#pragma once

/**
 * ASE GODOT ADAPTER - HUD FIGURE ZOOM
 *
 * @file        godot_hud_figure_zoom.hpp
 * @brief       FigureZoom - cam.zoom, the rail at the right edge whose knob is the player camera's distance
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.6 (VIVARIUM_PHASE_07_FIGURE_V1, cam.zoom, zoomY, zoomAt): small and
 *              only while the player camera stands. The rail runs from near at its top to far at its foot; its knob
 *              stands at the camera's distance to the figure in metres and carries the metres beside it.
 *
 *              THE SCALE. The lower share of the rail (1 - hud_figure_zoom_face_share) runs logarithmically from
 *              hud_figure_zoom_max to hud_figure_zoom_min metres with the whole figure in the picture; the top share
 *              runs on from hud_figure_zoom_min to hud_figure_zoom_face_distance, where the look has risen from the
 *              body to the face (operator 2026-11-14: the top tenth lands in the face). y_of and distance_at are the
 *              mockup's zoomY and zoomAt over the rail from hud_figure_zoom_top, hud_figure_zoom_length long.
 *
 *              THE FORM. The rail at hud_figure_zoom_x in FIGURE_RAIL at hud_figure_zoom_line with round ends;
 *              figure.zoom.near hud_figure_zoom_plus_dy above its top and figure.zoom.far hud_figure_zoom_minus_dy
 *              under its foot, centred, in FIGURE_INK at hud_figure_zoom_sign_alpha and hud_figure_zoom_sign_size.
 *              The knob a disc of hud_figure_zoom_knob_radius (hud_figure_zoom_knob_held_radius while held) in
 *              FIGURE_INK at hud_figure_zoom_knob_alpha; figure.zoom.metres with the distance at
 *              hud_figure_zoom_label_digits decimals ends hud_figure_zoom_label_dx left of the rail,
 *              hud_figure_zoom_label_dy under the knob's centre, at hud_figure_zoom_label_alpha
 *              (hud_figure_zoom_label_held_alpha while held). figure.zone.zoom ends at hud_figure_zoom_name_x,
 *              hud_figure_zoom_name_y in FIGURE_ZONE_ORBIT_LABEL. Every text in the data face (Fira Code).
 *
 *              THE TOUCH. hit() is true right of hud_figure_zoom_hit_x on the rail's height, widened by
 *              hud_figure_zoom_hit_pad above its top and below its foot. Two fingers on the orbit half pinch the
 *              distance: it scales with their spread at the press over their spread now, held between
 *              hud_figure_zoom_face_distance and hud_figure_zoom_max, and asks nothing while they stand closer than
 *              hud_figure_zoom_pinch_min.
 *
 *              NOTHING IS ZOOMED HERE: godot_vivarium_figure_controls.cpp turns a finger on the rail into metres
 *              through distance_at and sends them as zoom_delta through zoom_figure_camera, the orbit port of
 *              ase-pl-avatar; ase-camera moves its target distance within its own limits. The knob always shows the
 *              real distance of the camera record, never a wished one.
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

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** cam.zoom: the rail of the player camera's distance at the right edge. */
class FigureZoom {
public:
    /** Binds measures, colours and words; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The height on the rail, in mockup units, of the camera distance `distance` metres (the mockup's zoomY). */
    double y_of(double distance) const;

    /** The camera distance in metres the rail stands for at the height `y`, held on the rail (the mockup's zoomAt). */
    double distance_at(double y) const;

    /**
     * Draws the rail with its knob at `distance` metres under `frame`, held while a finger is on it; false with the
     * reason in `failed` when the metres cannot be written into their word.
     */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, double distance, bool held, std::string& failed);

    /** True when `point` (in the mockup's units) lies where a finger takes the rail. */
    bool hit(const ::godot::Vector2& point) const;

    /** The rail as hit() takes it, mockup units, centred on its line from its near end to its foot: a tap at a share
     *  of its height sets the distance distance_at() names there. */
    ::godot::Rect2 place() const;

    /**
     * The distance in metres two fingers on the orbit half ask for: `from` metres when they came down `spread_from`
     * apart, now `spread` apart, held on the rail's ends (the mockup's pinch); false while they stand closer than
     * hud_figure_zoom_pinch_min, when they ask for nothing.
     */
    bool pinch(double from, double spread_from, double spread, double& out) const;

private:
    double x_ = 0.0;                  // hud_figure_zoom_x: the rail's line
    double top_ = 0.0;                // hud_figure_zoom_top: its near end
    double length_ = 0.0;             // hud_figure_zoom_length
    double hit_x_ = 0.0;              // hud_figure_zoom_hit_x: a finger right of this takes it
    double hit_pad_ = 0.0;            // hud_figure_zoom_hit_pad: above the top and below the foot
    double pinch_min_ = 0.0;          // hud_figure_zoom_pinch_min: two fingers closer than this ask for nothing
    double min_ = 0.0;                // hud_figure_zoom_min: metres where the whole figure stands nearest
    double max_ = 0.0;                // hud_figure_zoom_max: metres at the far end
    double face_share_ = 0.0;         // hud_figure_zoom_face_share: the top share of the rail
    double face_distance_ = 0.0;      // hud_figure_zoom_face_distance: metres at the near end, in the face
    double line_ = 0.0;               // hud_figure_zoom_line
    double sign_size_ = 0.0;          // hud_figure_zoom_sign_size
    double sign_alpha_ = 0.0;         // hud_figure_zoom_sign_alpha
    double plus_dy_ = 0.0;            // hud_figure_zoom_plus_dy: off the top, negative is above
    double minus_dy_ = 0.0;           // hud_figure_zoom_minus_dy: off the foot
    double knob_radius_ = 0.0;        // hud_figure_zoom_knob_radius
    double knob_held_radius_ = 0.0;   // hud_figure_zoom_knob_held_radius
    double knob_alpha_ = 0.0;         // hud_figure_zoom_knob_alpha
    double label_dx_ = 0.0;           // hud_figure_zoom_label_dx: the metres end this far left of the rail
    double label_dy_ = 0.0;           // hud_figure_zoom_label_dy: their baseline under the knob's centre
    double label_size_ = 0.0;         // hud_figure_zoom_label_size
    double label_alpha_ = 0.0;        // hud_figure_zoom_label_alpha
    double label_held_alpha_ = 0.0;   // hud_figure_zoom_label_held_alpha
    double label_digits_ = 0.0;       // hud_figure_zoom_label_digits: decimals of the metres
    double name_x_ = 0.0;             // hud_figure_zoom_name_x: the name's right end
    double name_y_ = 0.0;             // hud_figure_zoom_name_y: its baseline
    double name_size_ = 0.0;          // hud_figure_zoom_name_size

    ::godot::Color ink_;              // FIGURE_INK: signs, knob, metres
    ::godot::Color rail_;             // FIGURE_RAIL
    ::godot::Color name_ink_;         // FIGURE_ZONE_ORBIT_LABEL: cam.zoom in the orbit half's hue

    UiTextBlock near_;                // figure.zoom.near over the top
    UiTextBlock far_;                 // figure.zoom.far under the foot
    UiTextBlock metres_;              // figure.zoom.metres with the distance filled in
    UiTextBlock name_;                // figure.zone.zoom
    std::vector<HudRun> metres_runs_; // figure.zoom.metres with its {value} still open
    std::string metres_shown_;        // the distance text metres_ was last set with; empty before the first draw
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI EDGE RAIL
 *
 * @file        godot_hud_ui_edge_rail.hpp
 * @brief       The rail of signs at the left edge of the care view, drawn as the mockup's rail(sg) does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiEdgeRail. The mockup draws:
 *
 *                line     a vertical line at hud_rail_x from hud_rail_top to hud_rail_bottom in
 *                         OVERLAY_INK_FAINT (--hud3), at the line width hud_line
 *                label    the rail's word (class t, hud_rail_label_size) end-anchored at
 *                         (hud_rail_label_x, hud_rail_label_y), a column of its own left of the line, and turned
 *                         by hud_rail_label_turn degrees about that same point, in OVERLAY_INK_SOFT (--hud2)
 *                alert    while at least one sign is alerting: a bar at (hud_rail_alert_x, hud_rail_alert_y),
 *                         hud_rail_alert_w wide and hud_rail_alert_base + alerts * hud_rail_alert_step high, in
 *                         DOMAIN_ALERT (--alert)
 *                signs    sign i as a dot at y = hud_rail_dot_y + i * hud_rail_dot_step: radius
 *                         hud_rail_dot_radius filled in its tone, or hud_rail_dot_on_radius filled with
 *                         OVERLAY_DOT_DARK when it is the opened sign; stroked in its tone at
 *                         hud_rail_dot_line; an alerting sign carries a pulse ring of hud_rail_pulse_radius
 *                         at the line width hud_line
 *                system   the ICON_HISTORY glyph centred at (hud_rail_x, hud_rail_system_y), size
 *                         hud_rail_system_size, in OVERLAY_INK_SOFT
 *
 *              The taps: a sign's target is the rect from the frame's left edge, hud_rail_hit_w wide and
 *              hud_rail_hit_h high about its dot (data-act sign:<id>); the system glyph's target is the square
 *              of hud_rail_system_hit from the left edge about hud_rail_system_y (data-act sysp). The rail
 *              holds no fade: it stands as soon as the view does.
 *
 *              The tone of a sign is its domain colour (DOM[g.dom]); the group that owns the signs passes it,
 *              as it passes the rail's word from the texts of the language.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One sign on the rail, in the order the rail lists them. */
struct UiEdgeRailSign {
    ::godot::Color tone;    // DOM[g.dom]: the colour of the sign's domain
    bool alert = false;     // g.alert: the sign is alerting, it pulses and counts into the alert bar
    bool opened = false;    // S.sign === g.id: the sign is the one opened in the view
};

/** The rail of signs at the left edge, with the system glyph under it. */
class UiEdgeRail {
public:
    /** Binds every hud_rail_* token, the inks, the glyph and the primitives; false with the missing one in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The rail's word and its signs for this frame; false with the reason in `failed`. */
    bool set(const std::string& label, const std::vector<UiEdgeRailSign>& signs, std::string& failed);

    /** Draws the rail under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

    /** True with the sign's place in `index` when `point` (mockup units) lies on a sign's target. */
    bool hit_sign(const ::godot::Vector2& point, int32_t& index) const;

    /** The target of the sign at place `index` (mockup units), the band hit_sign() asks; false past the last sign. */
    bool sign_place(size_t index, ::godot::Rect2& place) const;

    /** True when `point` (mockup units) lies on the system glyph's target. */
    bool hit_system(const ::godot::Vector2& point) const;

    /** The system glyph's target (mockup units), the square hit_system() asks. */
    ::godot::Rect2 system_place() const;

private:
    const UiTheme* theme_ = nullptr;
    UiLeaderLine line_;             // the rail's line and the pulse's line width
    UiRing ring_;                   // the dots and the pulse
    UiTextBlock label_;             // the rail's word, class t
    UiIcon system_;                 // ICON_HISTORY
    ::godot::Color faint_;          // OVERLAY_INK_FAINT: --hud3
    ::godot::Color soft_;           // OVERLAY_INK_SOFT: --hud2
    ::godot::Color alert_;          // DOMAIN_ALERT: --alert
    ::godot::Color dot_on_;         // OVERLAY_DOT_DARK: the fill of the opened sign
    double x_ = 0.0;                // hud_rail_x
    double top_ = 0.0;              // hud_rail_top
    double bottom_ = 0.0;           // hud_rail_bottom
    double label_x_ = 0.0;          // hud_rail_label_x: the word's own column, apart from the rail's
    double label_y_ = 0.0;          // hud_rail_label_y
    double label_size_ = 0.0;       // hud_rail_label_size
    double label_turn_ = 0.0;       // hud_rail_label_turn
    double alert_x_ = 0.0;          // hud_rail_alert_x
    double alert_y_ = 0.0;          // hud_rail_alert_y
    double alert_w_ = 0.0;          // hud_rail_alert_w
    double alert_base_ = 0.0;       // hud_rail_alert_base
    double alert_step_ = 0.0;       // hud_rail_alert_step
    double dot_y_ = 0.0;            // hud_rail_dot_y
    double dot_step_ = 0.0;         // hud_rail_dot_step
    double dot_radius_ = 0.0;       // hud_rail_dot_radius
    double dot_on_radius_ = 0.0;    // hud_rail_dot_on_radius
    double dot_line_ = 0.0;         // hud_rail_dot_line
    double pulse_radius_ = 0.0;     // hud_rail_pulse_radius
    double hit_w_ = 0.0;            // hud_rail_hit_w
    double hit_h_ = 0.0;            // hud_rail_hit_h
    double system_y_ = 0.0;         // hud_rail_system_y
    double system_size_ = 0.0;      // hud_rail_system_size
    double system_hit_ = 0.0;       // hud_rail_system_hit
    std::vector<UiEdgeRailSign> signs_;
};

}  // namespace ase::adp::godot

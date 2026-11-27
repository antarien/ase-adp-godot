#pragma once

/**
 * ASE GODOT ADAPTER - HUD ARC NEEDLE
 *
 * @file        godot_hud_arc_needle.hpp
 * @brief       ArcNeedle: the fixed needle of the sky arc, its triangle and the dot of "now" on each ring
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, elements A14 and A15 of the mockup's arcStatic. The
 *              needle stands at x hud_arc_needle_x from y hud_arc_needle_top down to hud_arc_needle_below under the day
 *              ring's low point, a line of hud_arc_needle_line in OVERLAY_INK, topped by a triangle (base at
 *              hud_arc_needle_head_y, half width hud_arc_needle_head_half, tip at hud_arc_needle_tip_y). On the needle,
 *              where each ring's low point lies, stands the dot of "now" in DOMAIN_TIME, hud_arc_now_day_radius on the
 *              day ring and hud_arc_now_radius on the others. The needle never turns and never fades: the rings turn
 *              under it. place() is the rect the debug line `part=arc_needle` names.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/arc/types.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>

namespace ase::adp::godot {

/** The needle and the dots of now. */
class ArcNeedle {
public:
    /** Binds the measures and colours; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the needle and the three dots under `frame`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame) const;

    /** The rect of the needle in the mockup's units, from the top of its triangle to its foot. */
    ::godot::Rect2 place() const;

private:
    double x_ = 0.0;           // hud_arc_needle_x
    double top_ = 0.0;         // hud_arc_needle_top
    double foot_ = 0.0;        // the foot: the day ring's low point plus hud_arc_needle_below
    double line_ = 0.0;        // hud_arc_needle_line
    double head_y_ = 0.0;      // hud_arc_needle_head_y
    double head_half_ = 0.0;   // hud_arc_needle_head_half
    double tip_y_ = 0.0;       // hud_arc_needle_tip_y
    double now_day_r_ = 0.0;   // hud_arc_now_day_radius
    double now_r_ = 0.0;       // hud_arc_now_radius
    double low_[GODOT_ARC_RINGS] = {};  // the low point of each ring: hud_arc_cy plus its radius
    ::godot::Color ink_, time_;         // OVERLAY_INK, DOMAIN_TIME
};

}  // namespace ase::adp::godot

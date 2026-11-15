#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI LEADER LINE
 *
 * @file        godot_hud_ui_leader_line.hpp
 * @brief       The leader line of the HUD: from an anchor over a knee to a line of text, drawn as it grows
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiLeaderLine. The mockup's L(pts, o)
 *              is a polyline of the class ln: a line of hud_line in the colour the caller gives (var(--hud2)
 *              when it gives none - the group passes that colour, this unit names none). It runs from the
 *              anchor a UiAnchor found over a knee to the row it labels (three points), or as a short tick or
 *              a spine stroke (two points).
 *
 *              TWO FORMS. The drawn form (.draw, pathLength 1) grows from its first point along its length
 *              with the tokens hud_anim_draw_ms and hud_anim_draw_ease after the delay o.d; `drawn` gives the
 *              share through UiMotion. The dashed form (o.dash, stroke-dasharray 2 3 - the tokens
 *              hud_line_dash and hud_line_gap) stands at once and runs its pattern across the knees, as SVG
 *              does for one polyline.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <string>

namespace ase::adp::godot {

/** The drawer of the HUD's leader lines. */
class UiLeaderLine {
public:
    /** Binds hud_line, hud_line_dash, hud_line_gap and the draw motion; false with the missing token in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The width of a leader line (hud_line), for a caller that draws the mockup's o.w only where it differs. */
    double width() const { return width_; }

    /** The share a drawn line has grown `elapsed` seconds after appearing, starting after `delay`. */
    double drawn(double elapsed, double delay, bool still) const;

    /** The polyline `points` in `color` and `width`, drawn from its first point to the share `drawn` of its length. */
    void draw(UiPen& canvas, const ::godot::PackedVector2Array& points, const ::godot::Color& color,
              double width, double drawn) const;

    /** The polyline `points` dashed in `color` at hud_line: hud_line_dash on, hud_line_gap off, across its knees. */
    void dashed(UiPen& canvas, const ::godot::PackedVector2Array& points,
                const ::godot::Color& color) const;

private:
    UiMotion draw_;      // hud_anim_draw: the growth of a drawn line
    double width_ = 0.0;  // hud_line
    double dash_ = 0.0;   // hud_line_dash: the stroke of a dashed line
    double gap_ = 0.0;    // hud_line_gap: the gap of a dashed line
};

}  // namespace ase::adp::godot

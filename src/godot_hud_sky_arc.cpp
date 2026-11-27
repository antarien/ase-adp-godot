/**
 * ASE GODOT ADAPTER - HUD SKY ARC
 *
 * @file        godot_hud_sky_arc.cpp
 * @brief       SkyArc: the sky arc of the top band, see godot_hud_sky_arc.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, mockup arcStatic and arcFrame.
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

#include <ase/adp/godot/godot_hud_sky_arc.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/touch/types.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool SkyArc::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_arc_hit_x", &hit_x_}, {"hud_arc_hit_y", &hit_y_}, {"hud_arc_hit_w", &hit_w_}, {"hud_arc_hit_h", &hit_h_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    flank_.clear();
    if (!fade_.bind(theme, failed) || !sign_.bind(theme, failed) || !needle_.bind(theme, failed) ||
        !readout_.bind(theme, sign_, failed)) {
        return false;
    }
    for (uint32_t ring = 0u; ring < GODOT_ARC_RINGS; ++ring) {
        if (!rings_[ring].bind(theme, ring, fade_, sign_, failed)) {
            return false;
        }
    }
    return true;
}

bool SkyArc::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkyArcFrame& now, std::string& failed) {
    flank_.push_back(std::string(GODOT_ARC_LINE_NOW) + " day=" +
                     hud_decimals(now.rings[GODOT_ARC_SLOT_DAY].now, GODOT_ARC_NOW_DECIMALS) + " lun=" +
                     hud_decimals(now.rings[GODOT_ARC_SLOT_LUN].now, GODOT_ARC_NOW_DECIMALS) + " year=" +
                     hud_decimals(now.rings[GODOT_ARC_SLOT_YEAR].now, GODOT_ARC_NOW_DECIMALS) + " seq=" +
                     std::to_string(now.seq) + " t=" + hud_decimals(now.elapsed, GODOT_ARC_NOW_DECIMALS));
    // The highest ring first: the year, the month, the day, as the mockup stacks them.
    for (uint32_t ring = GODOT_ARC_RINGS; ring > 0u; --ring) {
        const uint32_t index = ring - 1u;
        if (!rings_[index].draw(canvas, frame, now.rings[index], now.seq, now.elapsed, now.still, flank_, failed)) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
    }
    needle_.draw(canvas, frame);
    const bool shown = readout_.draw(canvas, frame, now, flank_, failed);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return shown;
}

::godot::Rect2 SkyArc::arc_place() const {
    return ::godot::Rect2(static_cast<float>(hit_x_), static_cast<float>(hit_y_), static_cast<float>(hit_w_),
                          static_cast<float>(hit_h_));
}

::godot::Rect2 SkyArc::needle_place() const {
    return needle_.place();
}

const char* SkyArc::hit(const ::godot::Vector2& point) const {
    return arc_place().has_point(point) ? GODOT_HUD_ACT_SKY : nullptr;
}

std::vector<std::string> SkyArc::take_flank() {
    std::vector<std::string> out;
    out.swap(flank_);
    return out;
}

}  // namespace ase::adp::godot

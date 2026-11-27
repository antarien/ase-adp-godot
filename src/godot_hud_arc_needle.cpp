/**
 * ASE GODOT ADAPTER - HUD ARC NEEDLE
 *
 * @file        godot_hud_arc_needle.cpp
 * @brief       ArcNeedle: the fixed needle of the sky arc and the dot of "now", see godot_hud_arc_needle.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, mockup arcStatic.
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

#include <ase/adp/godot/godot_hud_arc_needle.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool ArcNeedle::bind(const UiTheme& theme, std::string& failed) {
    double cy = 0.0;
    double below = 0.0;
    double radii[GODOT_ARC_RINGS] = {};
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_arc_cy", &cy},
             {"hud_arc_needle_x", &x_},
             {"hud_arc_needle_top", &top_},
             {"hud_arc_needle_below", &below},
             {"hud_arc_needle_line", &line_},
             {"hud_arc_needle_head_y", &head_y_},
             {"hud_arc_needle_head_half", &head_half_},
             {"hud_arc_needle_tip_y", &tip_y_},
             {"hud_arc_now_day_radius", &now_day_r_},
             {"hud_arc_now_radius", &now_r_},
             {"hud_arc_day_r", &radii[GODOT_ARC_SLOT_DAY]},
             {"hud_arc_lun_r", &radii[GODOT_ARC_SLOT_LUN]},
             {"hud_arc_year_r", &radii[GODOT_ARC_SLOT_YEAR]}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (uint32_t ring = 0u; ring < GODOT_ARC_RINGS; ++ring) {
        low_[ring] = cy + radii[ring];
    }
    foot_ = low_[GODOT_ARC_SLOT_DAY] + below;
    if (!theme.color("OVERLAY_INK", ink_)) {
        failed = "OVERLAY_INK";
        return false;
    }
    if (!theme.color("DOMAIN_TIME", time_)) {
        failed = "DOMAIN_TIME";
        return false;
    }
    return true;
}

void ArcNeedle::draw(UiPen& canvas, const ::godot::Transform2D& frame) const {
    canvas.draw_set_transform_matrix(frame);
    const float x = static_cast<float>(x_);
    canvas.draw_line(::godot::Vector2(x, static_cast<float>(top_)), ::godot::Vector2(x, static_cast<float>(foot_)), ink_,
                     static_cast<float>(line_), true);
    ::godot::PackedVector2Array head;
    head.push_back(::godot::Vector2(static_cast<float>(x_ - head_half_), static_cast<float>(head_y_)));
    head.push_back(::godot::Vector2(static_cast<float>(x_ + head_half_), static_cast<float>(head_y_)));
    head.push_back(::godot::Vector2(x, static_cast<float>(tip_y_)));
    canvas.draw_colored_polygon(head, ink_);
    for (uint32_t ring = 0u; ring < GODOT_ARC_RINGS; ++ring) {
        const double radius = ring == GODOT_ARC_SLOT_DAY ? now_day_r_ : now_r_;
        canvas.draw_circle(::godot::Vector2(x, static_cast<float>(low_[ring])), static_cast<float>(radius), time_, true,
                           -1.0f, true);
    }
}

::godot::Rect2 ArcNeedle::place() const {
    return ::godot::Rect2(static_cast<float>(x_ - head_half_), static_cast<float>(head_y_),
                          static_cast<float>(head_half_ + head_half_), static_cast<float>(foot_ - head_y_));
}

}  // namespace ase::adp::godot

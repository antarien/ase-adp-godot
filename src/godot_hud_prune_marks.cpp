/**
 * ASE GODOT ADAPTER - HUD PRUNE MARKS
 *
 * @file        godot_hud_prune_marks.cpp
 * @brief       The glowing marks at the internodes that may be cut, while the billhook is in hand
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_prune_marks.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100365 - prune a branch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0462 (flora.scene.v1: the internodes that may be cut), carrier N0448: a gap of the bridge,
 *              never of this unit
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_prune_marks.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/style/types.hpp>

#include <godot_cpp/variant/vector3.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool PruneMarks::bind(const UiTheme& theme, std::string& failed) {
    drawn_.clear();
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_prune_pulse_radius", &pulse_radius_},
             {"hud_prune_radius", &radius_},
             {"hud_prune_line", &line_w_},
             {"hud_prune_glyph_size", &glyph_size_},
             {"hud_prune_hit_radius", &hit_radius_},
             {"hud_prune_stagger", &stagger_},
             {"hud_line", &pulse_line_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.tone("cut", cut_)) {
        failed = "DOMAIN_CUT";
        return false;
    }
    if (!theme.color("OVERLAY_DISC", disc_)) {
        failed = "OVERLAY_DISC";
        return false;
    }
    return bind_word_icon(theme, "prune_glyph", glyph_, failed) && fade_.bind(theme, "hud_anim_fade", failed) &&
           ring_.bind(theme, failed);
}

bool PruneMarks::middle(const PruneMarksFrame& now, uint32_t internode, ::godot::Vector2& out) const {
    // rows: x, y, z axis and origin per row (GODOT_INSTANCE_VALUES); the unit tube runs from its origin
    // along its y axis, so its middle is origin plus half that axis.
    const auto half = static_cast<float>(GODOT_HALF);
    for (const VivariumInstanceView& segment : *now.segments) {
        if (segment.object_id != internode) {
            continue;
        }
        const ::godot::Vector3 centre(now.origin.x + segment.rows[3] + segment.rows[1] * half,
                                      now.origin.y + segment.rows[7] + segment.rows[5] * half,
                                      now.origin.z + segment.rows[11] + segment.rows[9] * half);
        return now.anchor->place(centre, out);
    }
    return false;
}

bool PruneMarks::draw(UiPen& canvas, const ::godot::Transform2D& frame, const PruneMarksFrame& now,
                      std::string& failed) {
    drawn_.clear();
    if (now.anchor == nullptr || now.segments == nullptr) {
        failed = "PruneMarks: the frame carries no anchor or no internodes";
        return false;
    }
    for (const uint32_t internode : now.cuttable) {
        ::godot::Vector2 centre;
        if (!middle(now, internode, centre)) {
            continue;
        }
        const double shown = fade_.progress(now.elapsed, static_cast<double>(drawn_.size()) * stagger_, now.still);
        ::godot::Color disc = disc_;
        disc.a *= static_cast<float>(shown);
        canvas.draw_set_transform_matrix(frame);
        ring_.pulse(canvas, centre, pulse_radius_, cut_, pulse_line_, now.elapsed, now.still);
        ring_.disc(canvas, centre, radius_, disc);
        ring_.circle(canvas, centre, radius_, cut_, line_w_, shown);
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        glyph_.draw(canvas, frame, centre, glyph_size_, cut_, shown);
        drawn_.push_back(Drawn{internode, centre});
    }
    return true;
}

bool PruneMarks::hit(const ::godot::Vector2& point, uint32_t& internode) const {
    for (const Drawn& mark : drawn_) {
        if (mark.centre.distance_to(point) <= static_cast<float>(hit_radius_)) {
            internode = mark.internode;
            return true;
        }
    }
    return false;
}

void PruneMarks::places(std::vector<PruneMarkPlace>& out) const {
    out.clear();
    const float r = static_cast<float>(hit_radius_);
    for (const Drawn& mark : drawn_) {
        out.push_back(PruneMarkPlace{mark.internode, ::godot::Rect2(mark.centre.x - r, mark.centre.y - r, r * 2.0f,
                                                                    r * 2.0f)});
    }
}

}  // namespace ase::adp::godot

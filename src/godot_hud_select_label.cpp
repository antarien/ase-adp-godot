/**
 * ASE GODOT ADAPTER - HUD SELECT LABEL
 *
 * @file        godot_hud_select_label.cpp
 * @brief       The reading of a single tap: what the selected thing is, at the depth of perception
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD row L01, see godot_hud_select_label.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_select_label.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

/** A line of text at the baseline point (x, y), anchored at `anchor`, shown at once as the mockup's T() without fade. */
UiTextPlace line_at(double x, double y, double size, double anchor, const ::godot::Color& color,
                    const SelectLabelFrame& now) {
    UiTextPlace place;
    place.at = at(x, y);
    place.size = size;
    place.anchor = anchor;
    place.color = color;
    place.opacity = 1.0;
    place.fade = false;
    place.delay = 0.0;
    place.elapsed = now.elapsed;
    place.still = now.still;
    return place;
}

}  // namespace

bool SelectLabel::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    widgets_.clear();
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_select_split_x", &split_x_},
             {"hud_select_elbow_dx", &elbow_dx_},
             {"hud_select_rise", &rise_},
             {"hud_select_min_y", &min_y_},
             {"hud_select_anchor_dy", &anchor_dy_},
             {"hud_select_text_gap", &text_gap_},
             {"hud_select_run", &run_},
             {"hud_select_title_dy", &title_dy_},
             {"hud_select_title_size", &title_size_},
             {"hud_select_facts_dy", &facts_dy_},
             {"hud_select_facts_size", &facts_size_},
             {"hud_select_facts_step", &facts_step_},
             {"hud_select_hint_size", &hint_size_},
             {"hud_select_hint_step", &hint_step_},
             {"hud_select_asset_size", &asset_size_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK", &ink_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {"tap_open", &tap_open_}, {"tap_use", &tap_use_}, {"tap_hand", &tap_hand_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    if (!title_text_.bind_title(theme, failed) || !label_text_.bind_label(theme, failed) ||
        !data_text_.bind_data(theme, failed) || !line_.bind(theme, failed)) {
        return false;
    }
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_facts_ = levels[1];
    level_asset_ = levels[2];
    return true;
}

const ItemWidget* SelectLabel::widget(const std::string& id, std::string& failed) {
    for (const ItemWidget& bound : widgets_) {
        if (bound.id() == id) {
            return &bound;
        }
    }
    std::string widget_class;
    if (theme_ == nullptr || !theme_->item_widget_class(id, widget_class)) {
        failed = "hud.items." + id + ".widget";
        return nullptr;
    }
    ItemWidget made;
    if (!made.bind(*theme_, widget_class.c_str(), failed)) {
        return nullptr;
    }
    widgets_.push_back(made);
    return &widgets_.back();
}

bool SelectLabel::say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
                      const std::vector<HudRun>& runs, const UiTextPlace& place, std::string& failed) {
    if (!block.set(runs, failed)) {
        return false;
    }
    block.draw(canvas, frame, place);
    canvas.draw_set_transform_matrix(frame);
    return true;
}

bool SelectLabel::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SelectLabelFrame& now,
                       std::string& failed) {
    const ItemWidget* item = widget(now.id, failed);
    if (item == nullptr) {
        return false;
    }
    // ax, ay: the top centre of the box; a box left of the split reads to the right.
    const double ax = static_cast<double>(now.box.position.x) + static_cast<double>(now.box.size.x) / 2.0;
    const double ay = static_cast<double>(now.box.position.y);
    const bool right = ax < split_x_;
    const double side = right ? 1.0 : -1.0;
    const double ex = ax + side * elbow_dx_;
    const double ey = ::ase::math::max(min_y_, ay - rise_);
    const double lx = ex + side * text_gap_;
    const double anchor = right ? GODOT_TEXT_ANCHOR_START : GODOT_TEXT_ANCHOR_END;

    canvas.draw_set_transform_matrix(frame);
    ::godot::PackedVector2Array leader;
    leader.push_back(at(ax, ay + anchor_dy_));
    leader.push_back(at(ex, ey));
    leader.push_back(at(lx + side * run_, ey));
    line_.draw(canvas, leader, item->tone(), line_.width(), line_.drawn(now.elapsed, 0.0, now.still));

    if (!say(canvas, frame, title_text_, item->name(),
             line_at(lx, ey + title_dy_, title_size_, anchor, item->tone(), now), failed)) {
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        return false;
    }
    double yy = ey + facts_dy_;
    if (now.level >= level_facts_) {
        // wt(o): a stack weighs its count of ones (the widget multiplies), every other thing what the bridge weighed.
        ItemWidgetCarry carry;
        carry.length_cm = now.length_cm;
        carry.mass_kg = now.stack ? now.kg : now.weight_kg;
        carry.volume_l = now.litres;
        carry.count = static_cast<uint32_t>(::ase::math::max(now.count, static_cast<int64_t>(0)));
        std::vector<HudRun> facts;
        if (!item->reading(carry, now.stack, facts, failed) ||
            !say(canvas, frame, data_text_, facts, line_at(lx, yy, facts_size_, anchor, ink_, now), failed)) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        yy += facts_step_;
    }
    const std::vector<HudRun>& hint = now.opens ? tap_open_ : now.held ? tap_use_ : tap_hand_;
    if (!say(canvas, frame, label_text_, hint, line_at(lx, yy, hint_size_, anchor, soft_, now), failed)) {
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        return false;
    }
    yy += hint_step_;
    if (now.level >= level_asset_) {
        // D.asset || D.phase04: the seeds have no asset yet and name what phase 04 draws them from.
        std::string asset;
        std::vector<std::string> verbs;
        std::string tone;
        double view_w = 0.0;
        double view_h = 0.0;
        std::string phase04;
        if (!theme_->item_thing(now.id, tone, asset, verbs, view_w, view_h) ||
            !theme_->item_phase04(now.id, phase04)) {
            failed = "hud.items." + now.id + ".asset, .phase04";
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        const std::vector<HudRun> drawn_from{HudRun{asset.empty() ? phase04 : asset, std::string()}};
        if (!say(canvas, frame, data_text_, drawn_from, line_at(lx, yy, asset_size_, anchor, faint_, now), failed)) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

}  // namespace ase::adp::godot

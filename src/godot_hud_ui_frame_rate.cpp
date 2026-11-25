/**
 * ASE GODOT ADAPTER - HUD UI FRAME RATE
 *
 * @file        godot_hud_ui_frame_rate.cpp
 * @brief       Binds, lays and changes the small number of the frames per second
 * @description Operator order 2026-11-24, see godot_hud_ui_frame_rate.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_frame_rate.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** A length of mockup units as whole canvas pixels, at least one. */
int32_t pixels(double units, float scale) {
    return ase::math::max(static_cast<int32_t>(ase::math::round(static_cast<float>(units) * scale)), 1);
}

}  // namespace

bool UiFrameRate::bind(const UiTheme& theme, double frame_budget_ms, std::string& failed) {
    double refresh_ms = 0.0;
    double frame_w = 0.0;
    double battery_x = 0.0;
    double battery_y = 0.0;
    double battery_w = 0.0;
    double battery_h = 0.0;
    const struct {
        const char* token;
        double*     out;
    } measures[] = {
        {"hud_refresh_ms", &refresh_ms},       {"hud_frame_width", &frame_w},
        {"hud_band_battery_x", &battery_x},    {"hud_band_battery_y", &battery_y},
        {"hud_band_battery_w", &battery_w},    {"hud_band_battery_h", &battery_h},
        {"hud_band_clock_size", &size_},       {"hud_text_shadow_width", &outline_},
    };
    for (const auto& entry : measures) {
        if (!theme.measure(entry.token, *entry.out) || !(*entry.out > 0.0)) {
            failed = entry.token;
            return false;
        }
    }
    const struct {
        const char*     name;
        ::godot::Color* out;
    } colors[] = {{"OVERLAY_INK", &ink_}, {"DOMAIN_ALERT", &alert_}, {"OVERLAY_SHADOW", &shadow_}};
    for (const auto& entry : colors) {
        if (!theme.color(entry.name, *entry.out)) {
            failed = entry.name;
            return false;
        }
    }
    face_ = theme.faces().data;
    if (face_.is_null()) {
        failed = "hud face of the numbers";
        return false;
    }
    const double left = battery_x + battery_w;
    if (!(left < frame_w)) {
        failed = "hud_frame_width";  // the battery leaves no gap to the edge
        return false;
    }
    // a line of twice the type's size, centred on the battery's middle: the outline of no digit is cut
    gap_ = ::godot::Rect2(static_cast<float>(left), static_cast<float>(battery_y + battery_h / 2.0 - size_),
                          static_cast<float>(frame_w - left), static_cast<float>(size_ * 2.0));
    if (!rate_.configure(refresh_ms / GODOT_MS_PER_SECOND, frame_budget_ms)) {
        failed = "view.frame_budget_ms";
        return false;
    }
    return true;
}

bool UiFrameRate::frame(::godot::Node& parent, double elapsed_s, const ::godot::Vector2& origin, float scale) {
    const bool read = rate_.frame(elapsed_s);
    if (!rate_.known() || !(scale > 0.0f)) {
        if (label_ != nullptr) {
            label_->set_visible(false);
        }
        return read && rate_.crossed();
    }
    const bool laid = label_ == nullptr;
    lay(parent);
    place(origin, scale);
    if (laid || (read && rate_.changed())) {
        label_->set_text(::godot::String::num_int64(rate_.shown()));
    }
    if (laid || (read && rate_.crossed())) {
        label_->add_theme_color_override("font_color", rate_.under() ? alert_ : ink_);
    }
    label_->set_visible(true);
    return read && rate_.crossed();
}

void UiFrameRate::rest() {
    rate_.restart();
    if (label_ != nullptr) {
        label_->set_visible(false);
    }
}

void UiFrameRate::lay(::godot::Node& parent) {
    if (label_ != nullptr) {
        return;
    }
    label_ = memnew(::godot::Label);
    label_->set_name("FrameRate");
    label_->set_mouse_filter(::godot::Control::MOUSE_FILTER_IGNORE);
    label_->set_horizontal_alignment(::godot::HORIZONTAL_ALIGNMENT_CENTER);
    label_->set_vertical_alignment(::godot::VERTICAL_ALIGNMENT_CENTER);
    label_->add_theme_font_override("font", face_);
    label_->add_theme_color_override("font_outline_color", shadow_);
    parent.add_child(label_);
}

void UiFrameRate::place(const ::godot::Vector2& origin, float scale) {
    if (scale == scale_ && origin == origin_) {
        return;
    }
    if (scale != scale_) {
        label_->add_theme_font_size_override("font_size", pixels(size_, scale));
        label_->add_theme_constant_override("outline_size", pixels(outline_, scale));
    }
    label_->set_position(origin + gap_.position * scale);
    label_->set_size(gap_.size * scale);
    origin_ = origin;
    scale_ = scale;
}

}  // namespace ase::adp::godot

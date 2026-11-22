/**
 * ASE GODOT ADAPTER - HUD TOAST
 *
 * @file        godot_hud_toast.cpp
 * @brief       The short message over the hand: one word of texts.hud in the domain colour its sender names
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_toast.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_toast.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

bool Toast::bind(const UiTheme& theme, std::string& failed) {
    double stands_ms = 0.0;
    const char* const measures[] = {"hud_hand_x", "hud_hand_toast_y", "hud_hand_toast_busy_y", "hud_hand_toast_size",
                                    "hud_toast_ms"};
    double* const slots[] = {&x_, &y_, &busy_y_, &size_, &stands_ms};
    for (size_t i = 0; i < 5; ++i) {
        if (!theme.measure(measures[i], *slots[i])) {
            failed = measures[i];
            return false;
        }
    }
    if (!title_.bind_title(theme, failed)) {
        return false;
    }
    stands_ = stands_ms / GODOT_MS_PER_SECOND;
    theme_ = &theme;
    posted_ = false;
    until_ = 0.0;
    return true;
}

bool Toast::post(const std::string& key, const std::vector<HudPortValue>& values, const std::string& colour,
                 double now, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.toast.unbound";
        return false;
    }
    std::vector<HudRun> runs;
    if (!theme_->words(key.c_str(), runs)) {
        failed = "texts.hud." + key;
        return false;
    }
    ::godot::Color ink;
    if (!theme_->color(colour.c_str(), ink)) {
        failed = colour;
        return false;
    }
    if (!fill_words(runs, values, failed) || !title_.set(runs, failed)) {
        return false;
    }
    colour_ = ink;
    until_ = now + stands_;
    posted_ = true;
    return true;
}

bool Toast::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ToastFrame& now,
                 std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.toast.unbound";
        return false;
    }
    if (!standing(now.now)) {
        return true;
    }
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(now.busy ? busy_y_ : y_));
    text.size = size_;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.color = colour_;
    text.opacity = 1.0;
    text.still = true;
    title_.draw(canvas, frame, text);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

}  // namespace ase::adp::godot

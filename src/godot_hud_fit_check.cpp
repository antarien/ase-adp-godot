/**
 * ASE GODOT ADAPTER - HUD FIT CHECK
 *
 * @file        godot_hud_fit_check.cpp
 * @brief       At the finger: whether a dragged thing goes where it stands, green with the place or red with the reason
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_fit_check.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0452 (flora.stow.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_fit_check.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

bool FitCheck::bind(const UiTheme& theme, std::string& failed) {
    const char* const measures[] = {"hud_ghost_label_dy", "hud_ghost_label_min_y", "hud_ghost_label_size"};
    double* const slots[] = {&label_dy_, &label_min_y_, &label_size_};
    for (size_t i = 0; i < 3; ++i) {
        if (!theme.measure(measures[i], *slots[i])) {
            failed = measures[i];
            return false;
        }
    }
    if (!theme.color("DOMAIN_LIFE", life_)) {
        failed = "DOMAIN_LIFE";
        return false;
    }
    if (!theme.color("DOMAIN_ALERT", alert_)) {
        failed = "DOMAIN_ALERT";
        return false;
    }
    if (!title_.bind_title(theme, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool FitCheck::draw(UiPen& canvas, const ::godot::Transform2D& frame, const FitCheckFrame& now,
                    std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.fit_check.unbound";
        return false;
    }
    // ghost(): tg.bad || tg.lab. The word of a refusal is fit_<code> in SHA; the word of a place is the key the
    // caller names.
    std::vector<HudRun> runs;
    const std::string key = now.code.empty() ? now.where : std::string(GODOT_HUD_WORD_FIT) + now.code;
    if (!theme_->words(key.c_str(), runs)) {
        failed = "texts.hud." + key;
        return false;
    }
    if (!fill_words(runs, now.values, failed) || !title_.set(runs, failed)) {
        return false;
    }
    UiTextPlace text;
    text.at = ::godot::Vector2(now.finger.x,
                               static_cast<float>(ase::math::max(label_min_y_, now.ghost_top + label_dy_)));
    text.size = label_size_;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.color = now.code.empty() ? life_ : alert_;
    text.opacity = 1.0;
    text.still = true;
    title_.draw(canvas, frame, text);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

}  // namespace ase::adp::godot

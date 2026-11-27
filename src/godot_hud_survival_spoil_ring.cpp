/**
 * ASE GODOT ADAPTER - HUD SURVIVAL SPOIL RING
 *
 * @file        godot_hud_survival_spoil_ring.cpp
 * @brief       Draws the spoil ring ESTRAGO around the food of the bag as the survival mockup's food(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U07, see godot_hud_survival_spoil_ring.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528, N0530, carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_spoil_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace ase::adp::godot {

namespace {

/** Reads the token `name` into `out`; false with `name` in `failed`. */
bool read_token(const UiTheme& theme, const char* name, double& out, std::string& failed) {
    if (!theme.measure(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

/** The point (x, y) in mockup units. */
::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

}  // namespace

bool SurvivalSpoilRing::bind(const UiTheme& theme, std::string& failed) {
    if (!fade_.bind(theme, "hud_anim_fade", failed) || !ring_.bind(theme, failed) ||
        !word_.bind_label(theme, failed)) {
        return false;
    }
    if (!theme.color("DOMAIN_GROUND", ground_)) {
        failed = "DOMAIN_GROUND";
        return false;
    }
    if (!read_token(theme, "hud_survival_food_centre_x", centre_x_, failed) ||
        !read_token(theme, "hud_survival_food_centre_y", centre_y_, failed) ||
        !read_token(theme, "hud_survival_food_radius", radius_, failed) ||
        !read_token(theme, "hud_survival_food_line", line_, failed) ||
        !read_token(theme, "hud_survival_food_word_x", word_x_, failed) ||
        !read_token(theme, "hud_survival_food_word_y", word_y_, failed) ||
        !read_token(theme, "hud_survival_food_word_size", word_size_, failed)) {
        return false;
    }
    std::vector<HudRun> runs;
    if (!theme.words("survival.food.spoil", runs)) {
        failed = "survival.food.spoil";
        return false;
    }
    if (!word_.set(runs, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool SurvivalSpoilRing::set(const SurvivalSpoilFrame& food, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival_spoil.unbound";
        return false;
    }
    if (food.shown && !(food.spoil >= 0.0 && food.spoil <= 1.0)) {
        failed = "hud.survival_spoil.spoil: a share 0 to 1";
        return false;
    }
    // if(st.food==null) return '': the ring stands while the bag carries food.
    shown_ = food.shown;
    spoil_ = food.spoil;
    return true;
}

void SurvivalSpoilRing::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr || !shown_) {
        return;
    }
    const double share = fade_.progress(elapsed, 0.0, still);
    canvas.draw_set_transform_matrix(frame);

    // a = st.food * 2π; M cx (cy - r) A r r 0 large 1 x y: from the top, clockwise, over the spoiled share of a
    // turn, stroke DOM.ground at stroke-width 1.4, no fill, the group's fade on the stroke.
    UiArcStyle style;
    style.opacity = share;
    style.round_cap = false;
    style.drawn = 1.0;
    ring_.arc(canvas, at(centre_x_, centre_y_), radius_, 0.0, spoil_ * GODOT_RING_TURN_DEG, ground_, line_, style);

    // T(cx, cy - 22, 'ESTRAGO', {z: 6, a: 'middle', col: DOM.ground})
    UiTextPlace word;
    word.at = at(word_x_, word_y_);
    word.size = word_size_;
    word.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    word.bare = true;  // s.html T writes class o.c without sh: no outline
    word.color = ground_;
    word.opacity = 1.0;
    word.fade = true;
    word.elapsed = elapsed;
    word.still = still;
    word_.draw(canvas, frame, word);
    canvas.draw_set_transform_matrix(frame);
}

}  // namespace ase::adp::godot

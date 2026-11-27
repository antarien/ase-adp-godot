/**
 * ASE GODOT ADAPTER - HUD SURVIVAL VIEW
 *
 * @file        godot_hud_survival_view.cpp
 * @brief       Binds, sets and draws the eight survival marks of the HUD on the layers rail and body
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, rows U01 to U08, see godot_hud_survival_view.hpp.
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
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_view.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

namespace ase::adp::godot {

bool SurvivalView::bind(const UiTheme& theme, std::string& failed) {
    return body_.bind(theme, failed) && warmth_.bind(theme, failed) && states_.bind(theme, failed) &&
           fire_.bind(theme, failed) && rest_.bind(theme, failed) && air_.bind(theme, failed) &&
           food_.bind(theme, failed) && water_.bind(theme, failed);
}

bool SurvivalView::draw_states(UiPen& canvas, const ::godot::Transform2D& frame, const SurvivalViewFrame& now,
                               std::string& failed) {
    if (now.body == nullptr) {
        failed = "SurvivalViewFrame.body";
        return false;
    }
    // The states of the body stand under the signs on the same rail and fade in on the same clock (s.html 187).
    if (!states_.set(*now.body, failed)) {
        return false;
    }
    states_.draw(canvas, frame, now.elapsed, now.still);
    return true;
}

bool SurvivalView::draw_body(UiPen& canvas, const ::godot::Transform2D& frame, const SurvivalViewFrame& now,
                             std::string& failed) {
    if (now.body == nullptr || now.fire == nullptr || now.rest == nullptr || now.air == nullptr ||
        now.food == nullptr) {
        failed = "SurvivalViewFrame: body, fire, rest, air and food";
        return false;
    }
    // The body rail on the same wall clock as the rail of signs: its rows fade in after the view appeared.
    // The scale CONFORTO hangs over the heat row on its leader and fades in with it (s.html corpo); the card of
    // the camp fire follows it, as render() lays fireCard(st) right after corpo(st) (s.html 239), and the card of
    // the resting figure after the fire, as render() lays rest(st) right after fireCard(st), and the mark of
    // the air after the rest, as render() lays air(st) right after rest(st), and the spoil ring of the bag's food
    // after the air, as render() lays food(st) right after air(st), and the water mark after the food, as
    // render() lays water(st) last; it reads the same sign poison of the body the rail SINAIS draws.
    if (!body_.set(*now.body, failed) || !warmth_.set(*now.body, failed) || !fire_.set(*now.fire, failed) ||
        !rest_.set(*now.rest, failed) || !air_.set(*now.air, failed) || !food_.set(*now.food, failed) ||
        !water_.set(*now.body, failed)) {
        return false;
    }
    body_.draw(canvas, frame, now.elapsed, now.still);
    warmth_.draw(canvas, frame, now.elapsed, now.still);
    fire_.draw(canvas, frame, now.elapsed, now.still);
    rest_.draw(canvas, frame, now.elapsed, now.still);
    air_.draw(canvas, frame, now.elapsed, now.still);
    food_.draw(canvas, frame, now.elapsed, now.still);
    water_.draw(canvas, frame, now.elapsed, now.still);
    return true;
}

}  // namespace ase::adp::godot

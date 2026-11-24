#pragma once

/**
 * ASE GODOT ADAPTER - HUD TOAST
 *
 * @file        godot_hud_toast.hpp
 * @brief       The short message over the hand: one word of texts.hud in the domain colour its sender names
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block Toast (the mockup's toast() and the last line
 *              of hand()).
 *
 *              THE WORD. A sender posts a key of texts.hud (toast.*, drop_at, fit_<c>) with the values it measured;
 *              they are filled in through fill_words, so a placeholder without its value refuses the post with
 *              its name and never stands in the picture. The colour is the SHA colour the sender names
 *              (DOMAIN_LIFE, DOMAIN_ALERT, DOMAIN_GROUND, DOMAIN_WATER, DOMAIN_FLOW, DOMAIN_CUT).
 *
 *              THE PLACE. Class tb at hud_hand_toast_size, anchored in the middle at hud_hand_x; on
 *              hud_hand_toast_busy_y in the mode terrain or while a thing lies in the hand, else on
 *              hud_hand_toast_y. No fade: the line stands at once.
 *
 *              THE TIME. A toast stands hud_toast_ms on the clock its sender posted it with and replaces any
 *              earlier one. posted() tells whether one was ever posted since bind - the mockup's !S.toast that
 *              keeps the hint of the hand away once a toast has spoken.
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

#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** How the toast stands this frame. */
struct ToastFrame {
    double now = 0.0;   // the seconds of the clock post() was given
    bool busy = false;  // the mode terrain or a thing in the hand: the toast stands on hud_hand_toast_busy_y
};

/** The message over the hand. */
class Toast {
public:
    /** Binds the place, the size, the time and the title face; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /**
     * Posts texts.hud.`key` filled with `values` in the SHA colour `colour` at `now`; it replaces any earlier
     * toast. False with the missing word, value or colour in `failed`, and nothing is posted then.
     */
    bool post(const std::string& key, const std::vector<HudPortValue>& values, const std::string& colour, double now,
              std::string& failed);

    /** Draws the toast while it stands; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const ToastFrame& now,
              std::string& failed);

    /** A toast stands at `now`. */
    bool standing(double now) const { return posted_ && now < until_; }

    /** A toast was posted since bind (the mockup's S.toast, which is never cleared). */
    bool posted() const { return posted_; }

private:
    const UiTheme* theme_ = nullptr;
    double x_ = 0.0;        // hud_hand_x
    double y_ = 0.0;        // hud_hand_toast_y
    double busy_y_ = 0.0;   // hud_hand_toast_busy_y
    double size_ = 0.0;     // hud_hand_toast_size
    double stands_ = 0.0;   // hud_toast_ms in seconds
    double until_ = 0.0;    // the clock at which the posted toast goes
    bool posted_ = false;   // a toast was posted since bind
    ::godot::Color colour_; // the colour the sender named
    UiTextBlock title_;     // class tb
};

}  // namespace ase::adp::godot

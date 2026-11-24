#pragma once

/**
 * ASE GODOT ADAPTER - HUD FIT CHECK
 *
 * @file        godot_hud_fit_check.hpp
 * @brief       At the finger: whether a dragged thing goes where it stands, green with the place or red with the reason
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, block FitCheck (the mockup's fits() and the label of
 *              ghost(), drawn on the drag layer above everything but the debug layer).
 *
 *              THE VERDICT IS THE MODULE'S. Length against the opening, room against the volume, weight against the
 *              load, a vessel or a long tool on the belt, a taken holster: ase-inventory decides it and answers a
 *              move on flora.stow.v1 with a refusal code. This unit never reckons it; it names the code. A code
 *              `c` is said as texts.hud.fit_`c` filled with the values the module sends (cm, l, cap_l, kg), so a
 *              new code is a new word in SHA, never a branch here.
 *
 *              THE LABEL. Above the ghost, at the finger's x and hud_ghost_label_dy over the ghost's top, never above
 *              hud_ghost_label_min_y; class tb at hud_ghost_label_size; the place word the caller names
 *              (texts.hud.where.*) in DOMAIN_LIFE when the thing goes there, the refusal in DOMAIN_ALERT when not.
 *
 *              FULL FROM THE FRAME. FitCheck draws the verdict the frame carries and never asks whether
 *              flora.stow.v1 answered: until it does, the bridge judges by the mockup's fits()
 *              (VivariumView::bridge_stow, VivariumCarryBridge::answer) and marks the gap N0452 in its own source.
 *              DragLayer hands a frame only when the verdict names the place the finger stands over.
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

#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the module answered for the place a drag stands over this frame. */
struct FitCheckFrame {
    std::string code;                   // the refusal code, empty when the thing goes there
    std::vector<HudPortValue> values;   // the values the module sends with the code (cm, l, cap_l, kg)
    std::string where;                  // texts.hud key of the place the verdict is for (where.*), empty when none
    ::godot::Vector2 finger;            // the drag point, in the mockup's units
    double ghost_top = 0.0;             // the top of the ghost drawn under the finger
};

/** The verdict at the finger. */
class FitCheck {
public:
    /** Binds measures, colours and the title face; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the label for `now` under `frame`; false with the missing word in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const FitCheckFrame& now,
              std::string& failed);

    /** The module refused the thing there (the hot edge turns DOMAIN_ALERT). */
    static bool refused(const FitCheckFrame& now) { return !now.where.empty() && !now.code.empty(); }

    /** The drop may land: the verdict names the place and refuses nothing. */
    static bool allowed(const FitCheckFrame& now) { return !now.where.empty() && now.code.empty(); }

private:
    const UiTheme* theme_ = nullptr;
    double label_dy_ = 0.0;      // hud_ghost_label_dy
    double label_min_y_ = 0.0;   // hud_ghost_label_min_y
    double label_size_ = 0.0;    // hud_ghost_label_size
    ::godot::Color life_, alert_;
    UiTextBlock title_;          // class tb
};

}  // namespace ase::adp::godot

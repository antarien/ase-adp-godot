#pragma once

/**
 * ASE GODOT ADAPTER - HUD HOURGLASS
 *
 * @file        godot_hud_hourglass.hpp
 * @brief       The hourglass of the calendar sheet: the game clock's pace shown, never asked
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block Hourglass (the hourglass of the mockup's
 *              skySheet()).
 *
 *              A DISPLAY, NEVER A CONTROL. The game clock is a constant of the MMO: no player moves it. The time
 *              lapse and the jumps exist only through the debug buttons after the start screen, and nothing
 *              else moves the clock (ARCH_ASE_KERNEL_EMBEDDED, "Two Cases of Time", usecase:101120). The
 *              hourglass shows what the clock does; a tap on it asks nothing.
 *
 *              THE HEAD. texts.hud.time in class t at (hud_sheet_x, hud_glass_label_y), size
 *              hud_glass_label_size, in OVERLAY_INK_SOFT.
 *
 *              THE GLASS TURNS WITH THE GAME HOUR. The glyph texts.hud.time_glyph, centred at (hud_glass_x,
 *              hud_glass_y), size hud_glass_size, in DOMAIN_TIME, stands turned by the share of the game hour
 *              the clock has reached: one turn per game hour. The block reads the hour, never the pace, so the
 *              glass turns exactly as fast as the debug layer's time lapse runs the clock, and a jump shows as
 *              the glass standing at the hour it landed on. While every motion is held it stands upright.
 *
 *              THE FACTOR. One hexagon of hud_glass_chip_radius (hud_hex_points) at (hud_glass_chip_x,
 *              hud_glass_y), filled DOMAIN_TIME_CHIP_ON and outlined DOMAIN_TIME at hud_line_chip_on, carries
 *              texts.hud.time_factor with HourglassFrame::factor in class d at hud_glass_chip_text_dy below its
 *              centre, size hud_glass_chip_text_size, centred, in DOMAIN_TIME: the factor of ase-time's base pace
 *              the clock runs at, 1 in the game, another only while the debug layer asked it.
 *
 *              THE NOTE. texts.hud.time_note in class t at (hud_sheet_x, hud_glass_note_y), size
 *              hud_glass_note_size, in OVERLAY_INK_FAINT: the game runs at a constant pace, the time lapse and the
 *              jumps stay in the debug layer.
 *
 *              FULL FROM THE FRAME. The factor and the hour come in HourglassFrame; the block never asks whether
 *              flora.pace.v1 reported them. Until the port reports the factor, the bridge writes it
 *              (VivariumView::bridge_pace: the factor last asked in the debug layer, else hud_bridge_pace_factor
 *              of SHA) and marks the gap N0459 in its own source.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_046
 * @category    ecs/module
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
 * @register    N0482 (the hourglass a display), N0459 (flora.pace.v1: the factor the clock runs at), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
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

/** What the hourglass shows this frame. */
struct HourglassFrame {
    double factor = 0.0;     // the factor the clock runs at, a multiple of ase-time's base pace
    double hour = 0.0;       // the hour of the game day the clock stands at: the glass's turn
    double elapsed = 0.0;    // seconds since the sheet opened
    bool still = false;      // every motion held
};

/** The hourglass of the calendar sheet. */
class Hourglass {
public:
    /** Binds measures, colours, words and the glyph; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the hourglass of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const HourglassFrame& now,
              std::string& failed);

private:
    /** The glyph turned by the share of the game hour `hour` round its centre, under `frame`. */
    void draw_turned(UiPen& canvas, const ::godot::Transform2D& frame, double hour) const;

    /** texts.hud.time_factor filled with `factor` into the readout, once per factor it changes to. */
    bool show_factor(double factor, std::string& failed);

    double sheet_x_ = 0.0;              // hud_sheet_x
    double label_y_ = 0.0;              // hud_glass_label_y
    double label_size_ = 0.0;           // hud_glass_label_size
    double x_ = 0.0;                    // hud_glass_x
    double y_ = 0.0;                    // hud_glass_y
    double size_ = 0.0;                 // hud_glass_size
    double chip_x_ = 0.0;               // hud_glass_chip_x
    double chip_radius_ = 0.0;          // hud_glass_chip_radius
    double chip_line_on_ = 0.0;         // hud_line_chip_on
    double chip_text_dy_ = 0.0;         // hud_glass_chip_text_dy
    double chip_text_size_ = 0.0;       // hud_glass_chip_text_size
    double note_y_ = 0.0;               // hud_glass_note_y
    double note_size_ = 0.0;            // hud_glass_note_size
    double shown_ = -1.0;               // the factor the readout carries; below 0 before the first draw
    ::godot::Color soft_;               // OVERLAY_INK_SOFT
    ::godot::Color faint_;              // OVERLAY_INK_FAINT
    ::godot::Color time_;               // DOMAIN_TIME
    ::godot::Color chip_on_;            // DOMAIN_TIME_CHIP_ON
    std::vector<HudRun> factor_words_;  // texts.hud.time_factor, unfilled
    UiIcon glyph_;                      // texts.hud.time_glyph
    UiTextBlock head_;                  // class t
    UiTextBlock factor_;                // class d: the factor the clock runs at
    UiTextBlock note_;                  // class t
};

}  // namespace ase::adp::godot

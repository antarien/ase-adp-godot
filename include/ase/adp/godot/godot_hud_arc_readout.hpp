#pragma once

/**
 * ASE GODOT ADAPTER - HUD ARC READOUT
 *
 * @file        godot_hud_arc_readout.hpp
 * @brief       ArcReadout: the SURA date and the next event with its deadline, the two lines under the needle
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, elements A16 and A17 of the mockup (#arcDate and
 *              #arcNext). Both stand at x hud_arc_readout_x, left aligned.
 *
 *              THE DATE at y hud_arc_date_y, size hud_arc_date_size, in the data face and OVERLAY_INK: the full name of
 *              the day period (texts.hud.arc.period.full.<code>, ASOMBRIS to NOXUMBRAE, ARCH_ASE_CALENDAR 2.9), then
 *              VECTUM.AETRIX.CYCLUM.LUNBREX.TUMBRAE.HORAX of the date record of ase-calendar, each a whole number.
 *
 *              THE NEXT EVENT at y hud_arc_next_y, size hud_arc_next_size, in the class t and the tone of the event's
 *              kind: its word (ArcSign::word_of) set into texts.hud.arc.next with the hours the owner computed to it
 *              (next_in_h, hud_arc_next_decimals places), or, from one whole TUMBRAE on, into texts.hud.arc.next_days
 *              with the whole days the owner computed to it (next_in_d, CAL_ARC_NEXT_IN_D). While no event is named
 *              (the code evnt_none, the frame holds no kind) the line stays empty. Turning hours into days is the
 *              clock logic of ase-calendar; the adapter only picks the line by the owner's day count.
 *
 *              THE FLANK. `debug arc next kind= body= in_h= in_d=` is written on every change of the shown line.
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
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_frame.hpp>
#include <ase/adp/godot/godot_hud_arc_sign.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The two lines under the needle. */
class ArcReadout {
public:
    /** Binds the measures, colours and words; `sign` stays owned by the caller. False with the missing name in `failed`. */
    bool bind(const UiTheme& theme, const ArcSign& sign, std::string& failed);

    /** Draws both lines of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkyArcFrame& now, std::vector<std::string>& flank,
              std::string& failed);

private:
    bool set_date(const SkyArcFrame& now, std::string& failed);
    bool set_next(const SkyArcFrame& now, std::string& line, std::string& failed);

    const UiTheme* theme_ = nullptr;
    const ArcSign* sign_ = nullptr;
    double x_ = 0.0;            // hud_arc_readout_x
    double date_y_ = 0.0;       // hud_arc_date_y
    double date_size_ = 0.0;    // hud_arc_date_size
    double next_y_ = 0.0;       // hud_arc_next_y
    double next_size_ = 0.0;    // hud_arc_next_size
    double decimals_ = 0.0;     // hud_arc_next_decimals
    std::vector<HudRun> next_runs_;                   // texts.hud.arc.next
    std::vector<HudRun> next_days_runs_;              // texts.hud.arc.next_days
    std::vector<std::vector<HudRun>> period_words_;   // texts.hud.arc.period.full.<code>, in the order of GODOT_PERIOD_*
    UiTextBlock date_;          // class d
    UiTextBlock next_;          // class t
    std::string date_line_;     // the date line as set
    std::string next_line_;     // the next line as set
    ::godot::Color next_tone_;  // the tone of the shown event's kind
    ::godot::Color ink_;        // OVERLAY_INK
};

}  // namespace ase::adp::godot

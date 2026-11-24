#pragma once

/**
 * ASE GODOT ADAPTER - HUD CALENDAR SHEET
 *
 * @file        godot_hud_calendar_sheet.hpp
 * @brief       The calendar sheet of the sky mode: the year ring, the day ring, the weather strip and the hourglass
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block CalendarSheet (the mockup's skySheet(), drawn in
 *              the mode sky after a tap on the sky band).
 *
 *              THE GROUND. A rect from hud_sheet_dim_y over hud_sheet_dim_h in OVERLAY_SCRIM_SKY, stretched to the
 *              surface the HUD sets in `cover` (hud_cover_dim).
 *
 *              THE HEAD. texts.hud.calendar in class tb at (hud_sheet_x, hud_sheet_title_y), size
 *              hud_sheet_title_size, in DOMAIN_TIME; under it texts.hud.calendar_case in class d at
 *              (hud_sheet_x, hud_sheet_case_y), size hud_sheet_case_size, in OVERLAY_INK_FAINT; the close glyph
 *              texts.hud.window_close centred at (hud_sheet_close_x, hud_sheet_close_y), size
 *              hud_sheet_close_size, in OVERLAY_INK_SOFT. A tap inside the rect hud_sheet_close_hit_x,
 *              hud_sheet_close_hit_y, hud_sheet_close_hit_w, hud_sheet_close_hit_h closes the sheet
 *              (data-act rest).
 *
 *              THE PARTS, in the order the mockup draws them: YearRing (godot_hud_year_ring.hpp), DayRing
 *              (godot_hud_day_ring.hpp), ForecastStrip (godot_hud_forecast_strip.hpp) and Hourglass
 *              (godot_hud_hourglass.hpp). The sheet owns them, hands each its frame and carries no value of
 *              its own; every backend gap a part shows (N0460, N0450, N0454, N0449, N0459) is that part's, and
 *              the sheet never covers one.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_day_ring.hpp>
#include <ase/adp/godot/godot_hud_forecast_strip.hpp>
#include <ase/adp/godot/godot_hud_hourglass.hpp>
#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_year_ring.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>

namespace ase::adp::godot {

/** What the sheet shows this frame: one frame per part. */
struct CalendarSheetFrame {
    YearRingFrame year;           // the year ring
    DayRingFrame day;             // the day ring
    ForecastStripFrame forecast;  // the weather strip
    HourglassFrame glass;         // the hourglass
    double elapsed = 0.0;         // seconds since the sheet opened
    bool still = false;           // every motion held
    HudCover cover;               // the surface the dim reaches, set by the HUD
};

/** What a tap on the sheet asks. */
struct CalendarSheetHit {
    bool rest = false;     // the close glyph: back to the mode rest
};

/** The calendar sheet. */
class CalendarSheet {
public:
    /** Binds the sheet and its four parts; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the sheet of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const CalendarSheetFrame& now,
              std::string& failed);

    /** What a tap at `point` (in the mockup's units) asks; false where it asks nothing. */
    bool hit(const ::godot::Vector2& point, CalendarSheetHit& out) const;

private:
    double dim_y_ = 0.0;           // hud_sheet_dim_y
    double dim_h_ = 0.0;           // hud_sheet_dim_h
    double x_ = 0.0;               // hud_sheet_x
    double title_y_ = 0.0;         // hud_sheet_title_y
    double title_size_ = 0.0;      // hud_sheet_title_size
    double case_y_ = 0.0;          // hud_sheet_case_y
    double case_size_ = 0.0;       // hud_sheet_case_size
    double close_x_ = 0.0;         // hud_sheet_close_x
    double close_y_ = 0.0;         // hud_sheet_close_y
    double close_size_ = 0.0;      // hud_sheet_close_size
    ::godot::Rect2 close_hit_;     // hud_sheet_close_hit_x, _y, _w, _h
    ::godot::Color scrim_;         // OVERLAY_SCRIM_SKY
    ::godot::Color time_;          // DOMAIN_TIME
    ::godot::Color soft_;          // OVERLAY_INK_SOFT
    ::godot::Color faint_;         // OVERLAY_INK_FAINT
    UiIcon close_;                 // texts.hud.window_close
    UiTextBlock title_;            // class tb
    UiTextBlock case_;             // class d
    YearRing year_;
    DayRing day_;
    ForecastStrip forecast_;
    Hourglass glass_;
};

}  // namespace ase::adp::godot

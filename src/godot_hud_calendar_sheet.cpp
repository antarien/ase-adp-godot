/**
 * ASE GODOT ADAPTER - HUD CALENDAR SHEET
 *
 * @file        godot_hud_calendar_sheet.cpp
 * @brief       The calendar sheet of the sky mode: the year ring, the day ring, the weather strip and the hourglass
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_calendar_sheet.hpp.
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

#include <ase/adp/godot/godot_hud_calendar_sheet.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <initializer_list>
#include <utility>
#include <vector>

namespace ase::adp::godot {

bool CalendarSheet::bind(const UiTheme& theme, std::string& failed) {
    double hit_x = 0.0;
    double hit_y = 0.0;
    double hit_w = 0.0;
    double hit_h = 0.0;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_sheet_dim_y", &dim_y_},
             {"hud_sheet_dim_h", &dim_h_},
             {"hud_sheet_x", &x_},
             {"hud_sheet_title_y", &title_y_},
             {"hud_sheet_title_size", &title_size_},
             {"hud_sheet_case_y", &case_y_},
             {"hud_sheet_case_size", &case_size_},
             {"hud_sheet_close_x", &close_x_},
             {"hud_sheet_close_y", &close_y_},
             {"hud_sheet_close_size", &close_size_},
             {"hud_sheet_close_hit_x", &hit_x},
             {"hud_sheet_close_hit_y", &hit_y},
             {"hud_sheet_close_hit_w", &hit_w},
             {"hud_sheet_close_hit_h", &hit_h}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    close_hit_ = ::godot::Rect2(static_cast<float>(hit_x), static_cast<float>(hit_y), static_cast<float>(hit_w),
                                static_cast<float>(hit_h));
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_SCRIM_SKY", &scrim_},
                                                                       {"DOMAIN_TIME", &time_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    std::vector<HudRun> title;
    std::vector<HudRun> line;
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"calendar", &title},
                                                                            {"calendar_case", &line}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    return bind_word_icon(theme, "window_close", close_, failed) && title_.bind_title(theme, failed) &&
           case_.bind_data(theme, failed) && title_.set(title, failed) && case_.set(line, failed) &&
           year_.bind(theme, failed) && day_.bind(theme, failed) && forecast_.bind(theme, failed) &&
           glass_.bind(theme, failed);
}

bool CalendarSheet::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                         const CalendarSheetFrame& now, std::string& failed) {
    // The ground under the sheet.
    canvas.draw_set_transform_matrix(frame);
    canvas.draw_rect(hud_cover_dim(now.cover, dim_y_, dim_h_), scrim_, true);

    // T(x, title_y, calendar, tb), T(x, case_y, calendar_case, d) and the close glyph.
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(title_y_));
    text.size = title_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = time_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    title_.draw(canvas, frame, text);
    UiTextPlace line = text;
    line.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(case_y_));
    line.size = case_size_;
    line.color = faint_;
    case_.draw(canvas, frame, line);
    close_.draw(canvas, frame, ::godot::Vector2(static_cast<float>(close_x_), static_cast<float>(close_y_)),
                close_size_, soft_, 1.0);

    // The four parts, in the order of skySheet().
    return year_.draw(canvas, frame, now.year, failed) && day_.draw(canvas, frame, now.day, failed) &&
           forecast_.draw(canvas, frame, now.forecast, failed) && glass_.draw(canvas, frame, now.glass, failed);
}

bool CalendarSheet::hit(const ::godot::Vector2& point, CalendarSheetHit& out) const {
    out = CalendarSheetHit{};
    if (close_hit_.has_point(point)) {
        out.rest = true;
        return true;
    }
    return false;
}

}  // namespace ase::adp::godot

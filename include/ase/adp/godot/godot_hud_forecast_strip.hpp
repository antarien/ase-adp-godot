#pragma once

/**
 * ASE GODOT ADAPTER - HUD FORECAST STRIP
 *
 * @file        godot_hud_forecast_strip.hpp
 * @brief       The weather strip of the calendar sheet: the next phases with glyph, temperature and millimetres
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block ForecastStrip (the forecast of the mockup's
 *              skySheet()).
 *
 *              THE HEAD. texts.hud.forecast in class t at (hud_sheet_x, hud_forecast_label_y), size
 *              hud_forecast_label_size, in OVERLAY_INK_SOFT. From perception levels[2] on texts.hud.forecast_source
 *              in class d at (hud_forecast_source_x, hud_forecast_label_y), size hud_forecast_source_size,
 *              anchored at its end, in OVERLAY_INK_FAINT.
 *
 *              ONE COLUMN PER PHASE. The column i stands at x = hud_forecast_x + i * hud_forecast_step:
 *                the phase's word texts.hud.phases.* in class t at hud_forecast_phase_y, size
 *                hud_forecast_phase_size, centred, in OVERLAY_INK_SOFT, fading in after
 *                hud_forecast_phase_delay + i * hud_forecast_stagger;
 *                the glyph texts.hud.weather_glyph.<kind>.day or .night centred at hud_forecast_glyph_y, size
 *                hud_forecast_glyph_size, in DOMAIN_WATER for rain and drizzle, in OVERLAY_INK for snow and in
 *                DOMAIN_SKY for every other kind, fading in after hud_forecast_glyph_delay + i * stagger;
 *                from levels[1] on texts.hud.forecast_temp with the temperature to hud_forecast_decimals in
 *                class d at hud_forecast_temp_y, size hud_forecast_temp_size, centred, in its ink, fading in after
 *                hud_forecast_temp_delay + i * stagger;
 *                from levels[2] on texts.hud.forecast_rain with the millimetres to hud_forecast_decimals in
 *                DOMAIN_WATER, or texts.hud.words.none in OVERLAY_INK_FAINT where nothing falls, in class d at
 *                hud_forecast_rain_y, size hud_forecast_rain_size, centred;
 *                a line in DOMAIN_SKY at hud_line from x - hud_forecast_line_half to x + hud_forecast_line_half
 *                at hud_forecast_line_y, solid for the first hud_forecast_near phases and dashed
 *                hud_forecast_line_dash on, hud_forecast_line_gap off after them, at hud_forecast_near_alpha and
 *                from the phase hud_forecast_far on at hud_forecast_far_alpha.
 *
 *              The fades are .fade with hud_anim_fade; .still and reduced motion hold every one at its end.
 *
 *              BRIDGE GAP N0449. The kind of weather, the temperature and the rain of each coming phase, and
 *              whether its middle lies in the night, are sky.weather.v1. The head and every column are always
 *              drawn in full from the frame: while the port files no record the bridge writes the mockup's
 *              forecast from hud_bridge_forecast_<n>_* of SHA and logs the gap once on its edge (godot_vivarium_hud_bridge.cpp,
 *              bridge_weather); the strip never asks whether a port delivered.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0449 (sky.weather.v1: the forecast of the next phases), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One coming phase as sky.weather.v1 forecasts it. */
struct ForecastPhase {
    int32_t period = 0;           // the phase, GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST
    const char* kind = nullptr;   // the weather key of the port (GODOT_HUD_WEATHER_*)
    bool night = false;           // the middle of the phase lies under the horizon
    double temperature = 0.0;     // the temperature the port forecasts
    double rain_mm = 0.0;         // the millimetres that fall in the phase, 0 where nothing falls
};

/** What the strip shows this frame. */
struct ForecastStripFrame {
    std::vector<ForecastPhase> phases;  // the next phases in their order, one per period of GODOT_PERIOD_* (N0449)
    uint32_t level = 0;                 // the perception level (hud.perception.levels)
    double elapsed = 0.0;               // seconds since the sheet opened
    bool still = false;                 // every motion held
};

/** The day and night glyph of one kind of weather, and its tone. */
struct ForecastGlyph {
    std::string kind;       // the weather key (GODOT_HUD_WEATHER_*)
    UiIcon day;             // texts.hud.weather_glyph.<kind>.day
    UiIcon night;           // texts.hud.weather_glyph.<kind>.night
    ::godot::Color tone;    // DOMAIN_WATER, OVERLAY_INK or DOMAIN_SKY
};

/** The weather strip of the calendar sheet. */
class ForecastStrip {
public:
    /** Binds measures, colours, words, glyphs and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the strip of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const ForecastStripFrame& now,
              std::string& failed);

private:
    /** The head line and, from levels[2] on, the source of the reading. */
    bool draw_head(UiPen& canvas, const ::godot::Transform2D& frame, const ForecastStripFrame& now,
                   std::string& failed);

    /** The column of the phase `index`. */
    bool draw_phase(UiPen& canvas, const ::godot::Transform2D& frame, const ForecastStripFrame& now,
                    size_t index, std::string& failed);

    /** `value` to hud_forecast_decimals places. */
    std::string decimal(double value) const;

    double label_y_ = 0.0;              // hud_forecast_label_y
    double label_size_ = 0.0;           // hud_forecast_label_size
    double source_x_ = 0.0;             // hud_forecast_source_x
    double source_size_ = 0.0;          // hud_forecast_source_size
    double sheet_x_ = 0.0;              // hud_sheet_x
    double x_ = 0.0;                    // hud_forecast_x
    double step_ = 0.0;                 // hud_forecast_step
    double phase_y_ = 0.0;              // hud_forecast_phase_y
    double phase_size_ = 0.0;           // hud_forecast_phase_size
    double phase_delay_ = 0.0;          // hud_forecast_phase_delay
    double glyph_y_ = 0.0;              // hud_forecast_glyph_y
    double glyph_size_ = 0.0;           // hud_forecast_glyph_size
    double glyph_delay_ = 0.0;          // hud_forecast_glyph_delay
    double temp_y_ = 0.0;               // hud_forecast_temp_y
    double temp_size_ = 0.0;            // hud_forecast_temp_size
    double temp_delay_ = 0.0;           // hud_forecast_temp_delay
    double rain_y_ = 0.0;               // hud_forecast_rain_y
    double rain_size_ = 0.0;            // hud_forecast_rain_size
    double stagger_ = 0.0;              // hud_forecast_stagger
    double line_y_ = 0.0;               // hud_forecast_line_y
    double line_half_ = 0.0;            // hud_forecast_line_half
    double line_dash_ = 0.0;            // hud_forecast_line_dash
    double line_gap_ = 0.0;             // hud_forecast_line_gap
    double near_ = 0.0;                 // hud_forecast_near
    double far_ = 0.0;                  // hud_forecast_far
    double near_alpha_ = 0.0;           // hud_forecast_near_alpha
    double far_alpha_ = 0.0;            // hud_forecast_far_alpha
    double decimals_ = 0.0;             // hud_forecast_decimals
    double line_ = 0.0;                 // hud_line
    uint32_t level_temp_ = 0;           // hud.perception.levels[1]
    uint32_t level_rain_ = 0;           // hud.perception.levels[2]
    ::godot::Color soft_;               // OVERLAY_INK_SOFT
    ::godot::Color faint_;              // OVERLAY_INK_FAINT
    ::godot::Color water_;              // DOMAIN_WATER
    ::godot::Color sky_;                // DOMAIN_SKY
    std::vector<HudRun> temp_words_;    // texts.hud.forecast_temp
    std::vector<HudRun> rain_words_;    // texts.hud.forecast_rain
    std::vector<HudRun> none_words_;    // texts.hud.words.none
    std::vector<std::vector<HudRun>> phase_words_;  // texts.hud.phases.*, in the order of GODOT_PERIOD_*
    std::vector<ForecastGlyph> glyphs_; // one per kind of weather the port names
    UiMotion fade_;                     // hud_anim_fade
    UiTextBlock head_;                  // class t
    UiTextBlock source_;                // class d
    UiTextBlock phase_;                 // class t
    UiTextBlock temp_;                  // class d
    UiTextBlock rain_;                  // class d
};

}  // namespace ase::adp::godot

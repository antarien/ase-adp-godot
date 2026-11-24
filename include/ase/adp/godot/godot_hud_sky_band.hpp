#pragma once

/**
 * ASE GODOT ADAPTER - HUD SKY BAND
 *
 * @file        godot_hud_sky_band.hpp
 * @brief       The band at the top of the care view: clock, battery, the day's chain, weather, date, season, debug
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block SkyBand (the mockup's band()).
 *
 *              THE STATUS LINE. texts.hud.band_clock with the device's wall hour and minute, each padded to
 *              hud_band_clock_digits, in class d at (hud_band_clock_x, hud_band_clock_y), size
 *              hud_band_clock_size, opacity hud_band_clock_alpha. The battery a rect of hud_band_battery_w by
 *              hud_band_battery_h with corner hud_band_battery_radius at (hud_band_battery_x,
 *              hud_band_battery_y), stroked in OVERLAY_INK_FRAME at hud_line_frame, and its level a rect of
 *              hud_band_battery_fill_w by hud_band_battery_fill_h hud_band_battery_fill_inset inside it in
 *              OVERLAY_INK_LEVEL.
 *
 *              THE DAY. UiGlyphChain with the phases of the day (texts.hud.phases.*), their hours, the current
 *              phase, the clock's hour, sunrise, sunset and night. The date line texts.hud.band_date with VEC,
 *              LUN, TUM and the hour to hud_band_decimals in class d at (hud_band_date_x, hud_band_date_y), size
 *              hud_band_date_size, in OVERLAY_INK_SOFT. The season's word texts.hud.seasons.<n> in class t at
 *              (hud_band_season_x, hud_band_season_y), size hud_band_season_size, anchored at its end, in the
 *              season's own colour SEASON_*.
 *
 *              THE WEATHER. The glyph texts.hud.weather_glyph.<kind>.day or .night in DOMAIN_SKY centred at
 *              (hud_band_weather_x, hud_band_weather_y), size hud_band_weather_size. From perception
 *              levels[1] on texts.hud.band_temp with the temperature to hud_band_decimals in class d at
 *              (hud_band_temp_x, hud_band_temp_y), size hud_band_temp_size; from levels[2] on the glyph
 *              texts.hud.band_wind_glyph in DOMAIN_FLOW at (hud_band_wind_x, hud_band_wind_y), size
 *              hud_band_wind_size, and texts.hud.band_wind with the wind to hud_band_decimals in class d at
 *              (hud_band_wind_text_x, hud_band_wind_text_y), size hud_band_wind_text_size, in DOMAIN_FLOW.
 *
 *              THE DEBUG SWITCH. The glyph texts.hud.band_debug_glyph centred at (hud_band_debug_x,
 *              hud_band_debug_y), size hud_band_debug_size: while the debug layer is on in DEBUG_INK_ON over a
 *              hexagon of hud_band_debug_radius filled DEBUG_BUTTON_ON and stroked DEBUG_LINE at hud_line_chip,
 *              while it is off in OVERLAY_INK at hud_band_debug_off_alpha.
 *
 *              THE TOUCH. hit() names GODOT_HUD_ACT_DEBUG inside the square of hud_band_debug_hit round the
 *              switch and GODOT_HUD_ACT_SKY inside the rect hud_band_hit_*, the calendar sheet's opener.
 *
 *              THE WALL CLOCK. The mockup's status line reads 18:24 and never follows the simulation's hour;
 *              it is the device's clock, so the unit reads it from Godot's Time, never from the clock record.
 *              The battery is drawn as the mockup draws it: the level rect of its tokens.
 *
 *              BRIDGE GAP N0460. Sunrise, sunset, the light hours and the season come from the day record
 *              flora.day.v1 (sunrise_h, sunset_h, daylight_h from CAL_DAYLIGHT_DURATION, season from
 *              CAL_SEASON), the hour each phase begins from the six records of flora.phase.v1 (period, start_h:
 *              0 for ASO, else TIM_CONST_PERIOD_BND_*). GodotHostResourceManager::read_snapshot reads them into
 *              VivariumClockView::day_known and phases_known, the bridge fills phase_starts, sunrise, sunset
 *              and season, and the night is the sky port's sun_y. The date, the chain and the season word are
 *              always drawn in full from that frame: while a port files no record the bridge writes the
 *              mockup's values from hud_bridge_clock_* of SHA and logs the gap once on its edge
 *              (godot_vivarium_hud_bridge.cpp, bridge_clock); the band never asks whether a port delivered.
 *
 *              BRIDGE GAP N0449. The kind of weather, the temperature and the wind come from sky.weather.v1.
 *              The glyph, the temperature and the wind are always drawn in full from weather_kind, temperature
 *              and wind: while the port files no record the bridge writes the mockup's weather from
 *              hud_bridge_weather_* of SHA and logs the gap once on its edge (godot_vivarium_hud_bridge.cpp,
 *              bridge_weather); the band never asks whether a port delivered.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0460 (flora.day.v1: light hours, season; flora.phase.v1: phase starts), N0449 (sky.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_glyph_chain.hpp>
#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the band shows this frame. */
struct SkyBandFrame {
    uint32_t vec = 0;                   // the year of the clock record (VEC)
    uint32_t lun = 0;                    // the month of the clock record (LUN)
    uint32_t tum = 0;                    // the day of the month of the clock record (TUM)
    double hour = 0.0;                   // the clock's hour (HOR)
    int32_t period = 0;                  // the current phase, GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST

    std::vector<double> phase_starts;    // the hour each phase begins, in the order of GODOT_PERIOD_*
    double hours_per_day = 0.0;          // where the last phase ends
    double sunrise = 0.0;                // the hour the sun rises
    double sunset = 0.0;                 // the hour the sun sets
    bool night = false;                  // the sun is under the horizon
    int32_t season = 0;                  // the season, the index of texts.hud.seasons.*

    const char* weather_kind = nullptr;  // the weather key of the port or of the bridge (GODOT_HUD_WEATHER_*), N0449
    double temperature = 0.0;            // the temperature of the port
    double wind = 0.0;                   // the wind of the port

    uint32_t level = 0;                  // the perception level (hud.perception.levels)
    bool debug_on = false;               // the debug layer is shown
    double elapsed = 0.0;                // seconds since the band appeared
    bool still = false;                  // every motion held
};

/** The day and night glyph of one kind of weather. */
struct SkyBandWeatherGlyph {
    std::string kind;  // the weather key (GODOT_HUD_WEATHER_*)
    UiIcon day;        // texts.hud.weather_glyph.<kind>.day
    UiIcon night;      // texts.hud.weather_glyph.<kind>.night
};

/** The word and the colour of one season. */
struct SkyBandSeason {
    std::vector<HudRun> word;  // texts.hud.seasons.<n>
    ::godot::Color color;      // SEASON_*
};

/** The sky band. */
class SkyBand {
public:
    /** Binds measures, colours, words, glyphs and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the band of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
              std::string& failed);

    /** The act at `point` (in the mockup's units): GODOT_HUD_ACT_DEBUG, GODOT_HUD_ACT_SKY or nullptr. */
    const char* hit(const ::godot::Vector2& point) const;

    /** The square of the debug switch (in the mockup's units): hud_band_debug_hit round its centre. */
    ::godot::Rect2 debug_switch() const;

    /** The rect a tap opens the sky sheet through (in the mockup's units): hud_band_hit_x, _y, _w, _h, the same
     *  hit() asks after the debug switch. */
    ::godot::Rect2 sky_place() const;

private:
    /** The clock and the battery. */
    bool draw_status(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                     std::string& failed);

    /** The chain of the day and the season's word from the frame; a missing day record is logged as N0460. */
    bool draw_day(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                  std::string& failed);

    /** The weather glyph, the temperature and the wind from the frame; a frame without a kind fails. */
    bool draw_weather(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                      std::string& failed);

    /** The date line. */
    bool draw_date(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                   std::string& failed);

    /** The debug switch. */
    void draw_debug(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now) const;

    /** `value` to hud_band_decimals places. */
    std::string decimal(double value) const;

    /** `value` padded to hud_band_clock_digits digits. */
    std::string digits(int64_t value) const;

    double clock_x_ = 0.0;              // hud_band_clock_x
    double clock_y_ = 0.0;              // hud_band_clock_y
    double clock_size_ = 0.0;           // hud_band_clock_size
    double clock_alpha_ = 0.0;          // hud_band_clock_alpha
    double clock_digits_ = 0.0;         // hud_band_clock_digits
    double battery_x_ = 0.0;            // hud_band_battery_x
    double battery_y_ = 0.0;            // hud_band_battery_y
    double battery_w_ = 0.0;            // hud_band_battery_w
    double battery_h_ = 0.0;            // hud_band_battery_h
    double battery_radius_ = 0.0;       // hud_band_battery_radius
    double battery_inset_ = 0.0;        // hud_band_battery_fill_inset
    double battery_fill_w_ = 0.0;       // hud_band_battery_fill_w
    double battery_fill_h_ = 0.0;       // hud_band_battery_fill_h
    double weather_x_ = 0.0;            // hud_band_weather_x
    double weather_y_ = 0.0;            // hud_band_weather_y
    double weather_size_ = 0.0;         // hud_band_weather_size
    double temp_x_ = 0.0;               // hud_band_temp_x
    double temp_y_ = 0.0;               // hud_band_temp_y
    double temp_size_ = 0.0;            // hud_band_temp_size
    double wind_x_ = 0.0;               // hud_band_wind_x
    double wind_y_ = 0.0;               // hud_band_wind_y
    double wind_size_ = 0.0;            // hud_band_wind_size
    double wind_text_x_ = 0.0;          // hud_band_wind_text_x
    double wind_text_y_ = 0.0;          // hud_band_wind_text_y
    double wind_text_size_ = 0.0;       // hud_band_wind_text_size
    double date_x_ = 0.0;               // hud_band_date_x
    double date_y_ = 0.0;               // hud_band_date_y
    double date_size_ = 0.0;            // hud_band_date_size
    double season_x_ = 0.0;             // hud_band_season_x
    double season_y_ = 0.0;             // hud_band_season_y
    double season_size_ = 0.0;          // hud_band_season_size
    double hit_x_ = 0.0;                // hud_band_hit_x
    double hit_y_ = 0.0;                // hud_band_hit_y
    double hit_w_ = 0.0;                // hud_band_hit_w
    double hit_h_ = 0.0;                // hud_band_hit_h
    double debug_x_ = 0.0;              // hud_band_debug_x
    double debug_y_ = 0.0;              // hud_band_debug_y
    double debug_radius_ = 0.0;         // hud_band_debug_radius
    double debug_size_ = 0.0;           // hud_band_debug_size
    double debug_off_alpha_ = 0.0;      // hud_band_debug_off_alpha
    double debug_hit_ = 0.0;            // hud_band_debug_hit
    double decimals_ = 0.0;             // hud_band_decimals
    double line_frame_ = 0.0;           // hud_line_frame
    double line_chip_ = 0.0;            // hud_line_chip
    uint32_t level_temp_ = 0;           // hud.perception.levels[1]
    uint32_t level_wind_ = 0;           // hud.perception.levels[2]

    ::godot::Color ink_, soft_, frame_ink_, level_ink_, sky_, flow_, debug_ink_, debug_edge_, debug_fill_;

    std::vector<HudRun> clock_words_;          // texts.hud.band_clock
    std::vector<HudRun> temp_words_;           // texts.hud.band_temp
    std::vector<HudRun> wind_words_;           // texts.hud.band_wind
    std::vector<HudRun> date_words_;           // texts.hud.band_date
    std::vector<std::string> phase_labels_;    // texts.hud.phases.*, in the order of GODOT_PERIOD_*
    std::vector<SkyBandSeason> seasons_;       // texts.hud.seasons.* with SEASON_*, in the order of the year
    std::vector<SkyBandWeatherGlyph> weather_; // one per kind of weather

    UiGlyphChain chain_;   // the day's phases with the sun or moon
    UiIcon wind_glyph_;    // texts.hud.band_wind_glyph
    UiIcon debug_glyph_;   // texts.hud.band_debug_glyph
    UiTextBlock clock_;    // class d
    UiTextBlock temp_;     // class d
    UiTextBlock wind_;     // class d
    UiTextBlock date_;     // class d
    UiTextBlock season_;   // class t

    std::vector<UiGlyphChainPhase> phases_;  // the chain's phases of this frame
};

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD SKY BAND
 *
 * @file        godot_hud_sky_band.cpp
 * @brief       The band at the top of the care view: clock, battery, the day's chain, weather, date, season, debug
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_sky_band.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0460 (flora.day.v1: light hours, season; flora.phase.v1: phase starts), N0449 (flora.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_sky_band.hpp>
#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {


bool SkyBand::bind(const UiTheme& theme, std::string& failed) {
    phases_.clear();
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_band_clock_x", &clock_x_},
             {"hud_band_clock_y", &clock_y_},
             {"hud_band_clock_size", &clock_size_},
             {"hud_band_clock_alpha", &clock_alpha_},
             {"hud_band_clock_digits", &clock_digits_},
             {"hud_band_battery_x", &battery_x_},
             {"hud_band_battery_y", &battery_y_},
             {"hud_band_battery_w", &battery_w_},
             {"hud_band_battery_h", &battery_h_},
             {"hud_band_battery_radius", &battery_radius_},
             {"hud_band_battery_fill_inset", &battery_inset_},
             {"hud_band_battery_fill_w", &battery_fill_w_},
             {"hud_band_battery_fill_h", &battery_fill_h_},
             {"hud_band_weather_x", &weather_x_},
             {"hud_band_weather_y", &weather_y_},
             {"hud_band_weather_size", &weather_size_},
             {"hud_band_temp_x", &temp_x_},
             {"hud_band_temp_y", &temp_y_},
             {"hud_band_temp_size", &temp_size_},
             {"hud_band_wind_x", &wind_x_},
             {"hud_band_wind_y", &wind_y_},
             {"hud_band_wind_size", &wind_size_},
             {"hud_band_wind_text_x", &wind_text_x_},
             {"hud_band_wind_text_y", &wind_text_y_},
             {"hud_band_wind_text_size", &wind_text_size_},
             {"hud_band_date_x", &date_x_},
             {"hud_band_date_y", &date_y_},
             {"hud_band_date_size", &date_size_},
             {"hud_band_season_x", &season_x_},
             {"hud_band_season_y", &season_y_},
             {"hud_band_season_size", &season_size_},
             {"hud_band_hit_x", &hit_x_},
             {"hud_band_hit_y", &hit_y_},
             {"hud_band_hit_w", &hit_w_},
             {"hud_band_hit_h", &hit_h_},
             {"hud_band_debug_x", &debug_x_},
             {"hud_band_debug_y", &debug_y_},
             {"hud_band_debug_radius", &debug_radius_},
             {"hud_band_debug_size", &debug_size_},
             {"hud_band_debug_off_alpha", &debug_off_alpha_},
             {"hud_band_debug_hit", &debug_hit_},
             {"hud_band_decimals", &decimals_},
             {"hud_line_frame", &line_frame_},
             {"hud_line_chip", &line_chip_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK", &ink_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FRAME", &frame_ink_},
                                                                       {"OVERLAY_INK_LEVEL", &level_ink_},
                                                                       {"DOMAIN_SKY", &sky_},
                                                                       {"DOMAIN_FLOW", &flow_},
                                                                       {"DEBUG_INK_ON", &debug_ink_},
                                                                       {"DEBUG_LINE", &debug_edge_},
                                                                       {"DEBUG_BUTTON_ON", &debug_fill_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"band_clock", &clock_words_},
                                                                            {"band_temp", &temp_words_},
                                                                            {"band_wind", &wind_words_},
                                                                            {"band_date", &date_words_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }

    // The phases in the order of GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST, one word each.
    std::vector<std::vector<HudRun>> phase_words;
    if (!bind_phase_words(theme, phase_words, failed)) {
        return false;
    }
    phase_labels_.clear();
    for (const std::vector<HudRun>& word : phase_words) {
        std::string label;
        if (!word_text(word, label, failed)) {
            failed = "texts.hud.phases: " + failed;
            return false;
        }
        phase_labels_.push_back(label);
    }

    // The seasons in the order of the year, each word with its own colour.
    seasons_.clear();
    for (const std::pair<const char*, const char*>& slot : std::initializer_list<std::pair<const char*, const char*>>{
             {"seasons.0", "SEASON_GLACIX"},
             {"seasons.1", "SEASON_SALTATRIX"},
             {"seasons.2", "SEASON_IGNIX"},
             {"seasons.3", "SEASON_HALITRIX"}}) {
        SkyBandSeason season;
        if (!theme.words(slot.first, season.word)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
        if (!theme.color(slot.second, season.color)) {
            failed = slot.second;
            return false;
        }
        seasons_.push_back(season);
    }

    // One day and one night glyph per kind of weather the port names.
    weather_.clear();
    for (const char* kind : {GODOT_HUD_WEATHER_CLEAR, GODOT_HUD_WEATHER_PCLOUD, GODOT_HUD_WEATHER_CLOUDY,
                             GODOT_HUD_WEATHER_DRIZZLE, GODOT_HUD_WEATHER_RAIN, GODOT_HUD_WEATHER_SNOW,
                             GODOT_HUD_WEATHER_FOG}) {
        SkyBandWeatherGlyph glyph;
        glyph.kind = kind;
        const std::string key = std::string("weather_glyph.") + kind;
        if (!bind_word_icon(theme, key + ".day", glyph.day, failed) ||
            !bind_word_icon(theme, key + ".night", glyph.night, failed)) {
            return false;
        }
        weather_.push_back(glyph);
    }
    if (!bind_word_icon(theme, "band_wind_glyph", wind_glyph_, failed) ||
        !bind_word_icon(theme, "band_debug_glyph", debug_glyph_, failed)) {
        return false;
    }

    // The thresholds of perception are the spec's levels, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_temp_ = levels[1];
    level_wind_ = levels[2];

    return chain_.bind(theme, failed) && clock_.bind_data(theme, failed) && temp_.bind_data(theme, failed) &&
           wind_.bind_data(theme, failed) && date_.bind_data(theme, failed) && season_.bind_label(theme, failed);
}

std::string SkyBand::decimal(double value) const {
    const int decimals = static_cast<int>(decimals_);
    return hud_decimals(value, decimals);
}

std::string SkyBand::digits(int64_t value) const {
    return hud_padded(value, static_cast<int32_t>(clock_digits_));
}

bool SkyBand::draw_status(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                          std::string& failed) {
    // The status line is the device's clock: the mockup's 18:24 never follows the simulation's hour.
    const ::godot::Time* time = ::godot::Time::get_singleton();
    if (time == nullptr) {
        failed = "Time singleton";
        return false;
    }
    const ::godot::Dictionary wall = time->get_time_dict_from_system();
    std::vector<HudRun> runs = clock_words_;
    if (!fill_words(runs,
                    {HudPortValue{"hh", digits(static_cast<int64_t>(wall["hour"]))},
                     HudPortValue{"mm", digits(static_cast<int64_t>(wall["minute"]))}},
                    failed) ||
        !clock_.set(runs, failed)) {
        return false;
    }
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(clock_x_), static_cast<float>(clock_y_));
    text.size = clock_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = clock_.ink();
    text.opacity = clock_alpha_;
    text.elapsed = now.elapsed;
    text.still = now.still;
    clock_.draw(canvas, frame, text);

    UiSvgOutline battery;
    svg_append_rounded_rect(battery,
                            ::godot::Rect2(static_cast<float>(battery_x_), static_cast<float>(battery_y_),
                                           static_cast<float>(battery_w_), static_cast<float>(battery_h_)),
                            static_cast<float>(battery_radius_));
    svg_stroke(canvas, battery, frame_ink_, line_frame_);
    canvas.draw_rect(::godot::Rect2(static_cast<float>(battery_x_ + battery_inset_),
                                    static_cast<float>(battery_y_ + battery_inset_),
                                    static_cast<float>(battery_fill_w_), static_cast<float>(battery_fill_h_)),
                     level_ink_);
    return true;
}

bool SkyBand::draw_day(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                       std::string& failed) {
    const ::godot::Vector2 season_at(static_cast<float>(season_x_), static_cast<float>(season_y_));
    if (now.phase_starts.size() != phase_labels_.size()) {
        failed = "SkyBand: the clock record must give one start hour per period of GODOT_PERIOD_*";
        return false;
    }
    phases_.clear();
    for (size_t index = 0; index < phase_labels_.size(); ++index) {
        UiGlyphChainPhase phase;
        phase.label = phase_labels_[index];
        phase.start = now.phase_starts[index];
        phase.end = index + 1 < phase_labels_.size() ? now.phase_starts[index + 1] : now.hours_per_day;
        phases_.push_back(phase);
    }
    if (!chain_.set(phases_, now.period - GODOT_PERIOD_FIRST, failed)) {
        return false;
    }
    UiGlyphChainDay day;
    day.hour = now.hour;
    day.sunrise = now.sunrise;
    day.sunset = now.sunset;
    day.night = now.night;
    day.elapsed = now.elapsed;
    day.still = now.still;
    chain_.draw(canvas, frame, day);

    if (now.season < 0 || static_cast<size_t>(now.season) >= seasons_.size()) {
        failed = "SkyBand: the season " + hud_whole(now.season) + " has no word in texts.hud.seasons";
        return false;
    }
    const SkyBandSeason& season = seasons_[static_cast<size_t>(now.season)];
    if (!season_.set(season.word, failed)) {
        return false;
    }
    UiTextPlace text;
    text.at = season_at;
    text.size = season_size_;
    text.anchor = GODOT_TEXT_ANCHOR_END;
    text.color = season.color;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    season_.draw(canvas, frame, text);
    return true;
}

bool SkyBand::draw_weather(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                           std::string& failed) {
    // The frame always carries a kind of weather: flora.weather.v1's, or the mockup's the bridge writes while the
    // port is silent (bridge_weather). A frame without one is a broken frame, never a picture to leave out.
    if (now.weather_kind == nullptr) {
        failed = "the sky band's frame: weather_kind";
        return false;
    }

    // The glyph's day or night is ase-pl-sky's sun, under the horizon or above it (the frame's night).
    const SkyBandWeatherGlyph* glyph = nullptr;
    for (const SkyBandWeatherGlyph& entry : weather_) {
        if (entry.kind == now.weather_kind) {
            glyph = &entry;
        }
    }
    if (glyph == nullptr) {
        failed = std::string("texts.hud.weather_glyph.") + now.weather_kind;
        return false;
    }
    (now.night ? glyph->night : glyph->day)
        .draw(canvas, frame, ::godot::Vector2(static_cast<float>(weather_x_), static_cast<float>(weather_y_)),
              weather_size_, sky_, 1.0);

    if (now.level >= level_temp_) {
        std::vector<HudRun> runs = temp_words_;
        if (!fill_words(runs, {HudPortValue{"t", decimal(now.temperature)}}, failed) || !temp_.set(runs, failed)) {
            return false;
        }
        UiTextPlace text;
        text.at = ::godot::Vector2(static_cast<float>(temp_x_), static_cast<float>(temp_y_));
        text.size = temp_size_;
        text.anchor = GODOT_TEXT_ANCHOR_START;
        text.color = temp_.ink();
        text.opacity = 1.0;
        text.elapsed = now.elapsed;
        text.still = now.still;
        temp_.draw(canvas, frame, text);
    }
    if (now.level >= level_wind_) {
        wind_glyph_.draw(canvas, frame, ::godot::Vector2(static_cast<float>(wind_x_), static_cast<float>(wind_y_)),
                         wind_size_, flow_, 1.0);
        std::vector<HudRun> runs = wind_words_;
        if (!fill_words(runs, {HudPortValue{"wind", decimal(now.wind)}}, failed) || !wind_.set(runs, failed)) {
            return false;
        }
        UiTextPlace text;
        text.at = ::godot::Vector2(static_cast<float>(wind_text_x_), static_cast<float>(wind_text_y_));
        text.size = wind_text_size_;
        text.anchor = GODOT_TEXT_ANCHOR_START;
        text.color = flow_;
        text.opacity = 1.0;
        text.elapsed = now.elapsed;
        text.still = now.still;
        wind_.draw(canvas, frame, text);
    }
    return true;
}

bool SkyBand::draw_date(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                        std::string& failed) {
    std::vector<HudRun> runs = date_words_;
    if (!fill_words(runs,
                    {HudPortValue{"vec", hud_whole(now.vec)}, HudPortValue{"lun", hud_whole(now.lun)},
                     HudPortValue{"tum", hud_whole(now.tum)},HudPortValue{"hour", decimal(now.hour)}},
                    failed) ||
        !date_.set(runs, failed)) {
        return false;
    }
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(date_x_), static_cast<float>(date_y_));
    text.size = date_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = soft_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    date_.draw(canvas, frame, text);
    return true;
}

void SkyBand::draw_debug(UiPen& canvas, const ::godot::Transform2D& frame,
                         const SkyBandFrame& now) const {
    const ::godot::Vector2 centre(static_cast<float>(debug_x_), static_cast<float>(debug_y_));
    if (now.debug_on) {
        ::godot::PackedVector2Array corners = hud_hex_points(centre, debug_radius_);
        canvas.draw_colored_polygon(corners, debug_fill_);
        corners.push_back(corners[0]);
        canvas.draw_polyline(corners, debug_edge_, static_cast<float>(line_chip_), true);
    }
    debug_glyph_.draw(canvas, frame, centre, debug_size_, now.debug_on ? debug_ink_ : ink_,
                      now.debug_on ? 1.0 : debug_off_alpha_);
}

bool SkyBand::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkyBandFrame& now,
                   std::string& failed) {
    canvas.draw_set_transform_matrix(frame);
    const bool drawn = draw_status(canvas, frame, now, failed) && draw_day(canvas, frame, now, failed) &&
                       draw_weather(canvas, frame, now, failed) && draw_date(canvas, frame, now, failed);
    if (drawn) {
        draw_debug(canvas, frame, now);
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return drawn;
}

::godot::Rect2 SkyBand::debug_switch() const {
    const float half = static_cast<float>(debug_hit_ / 2.0);
    return ::godot::Rect2(static_cast<float>(debug_x_) - half, static_cast<float>(debug_y_) - half,
                          static_cast<float>(debug_hit_), static_cast<float>(debug_hit_));
}

::godot::Rect2 SkyBand::sky_place() const {
    return ::godot::Rect2(static_cast<float>(hit_x_), static_cast<float>(hit_y_), static_cast<float>(hit_w_),
                          static_cast<float>(hit_h_));
}

const char* SkyBand::hit(const ::godot::Vector2& point) const {
    if (debug_switch().has_point(point)) {
        return GODOT_HUD_ACT_DEBUG;
    }
    if (sky_place().has_point(point)) {
        return GODOT_HUD_ACT_SKY;
    }
    return nullptr;
}

}  // namespace ase::adp::godot

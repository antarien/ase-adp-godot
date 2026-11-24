/**
 * ASE GODOT ADAPTER - HUD FORECAST STRIP
 *
 * @file        godot_hud_forecast_strip.cpp
 * @brief       The weather strip of the calendar sheet: the next phases with glyph, temperature and millimetres
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_forecast_strip.hpp.
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

#include <ase/adp/godot/godot_hud_forecast_strip.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool ForecastStrip::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_forecast_label_y", &label_y_},
             {"hud_forecast_label_size", &label_size_},
             {"hud_forecast_source_x", &source_x_},
             {"hud_forecast_source_size", &source_size_},
             {"hud_sheet_x", &sheet_x_},
             {"hud_forecast_x", &x_},
             {"hud_forecast_step", &step_},
             {"hud_forecast_phase_y", &phase_y_},
             {"hud_forecast_phase_size", &phase_size_},
             {"hud_forecast_phase_delay", &phase_delay_},
             {"hud_forecast_glyph_y", &glyph_y_},
             {"hud_forecast_glyph_size", &glyph_size_},
             {"hud_forecast_glyph_delay", &glyph_delay_},
             {"hud_forecast_temp_y", &temp_y_},
             {"hud_forecast_temp_size", &temp_size_},
             {"hud_forecast_temp_delay", &temp_delay_},
             {"hud_forecast_rain_y", &rain_y_},
             {"hud_forecast_rain_size", &rain_size_},
             {"hud_forecast_stagger", &stagger_},
             {"hud_forecast_line_y", &line_y_},
             {"hud_forecast_line_half", &line_half_},
             {"hud_forecast_line_dash", &line_dash_},
             {"hud_forecast_line_gap", &line_gap_},
             {"hud_forecast_near", &near_},
             {"hud_forecast_far", &far_},
             {"hud_forecast_near_alpha", &near_alpha_},
             {"hud_forecast_far_alpha", &far_alpha_},
             {"hud_forecast_decimals", &decimals_},
             {"hud_line", &line_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    ::godot::Color ink;
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK", &ink},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_},
                                                                       {"DOMAIN_WATER", &water_},
                                                                       {"DOMAIN_SKY", &sky_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"forecast_temp", &temp_words_},
                                                                            {"forecast_rain", &rain_words_},
                                                                            {"words.none", &none_words_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    std::vector<HudRun> head;
    std::vector<HudRun> source;
    if (!theme.words("forecast", head) || !theme.words("forecast_source", source)) {
        failed = "texts.hud.forecast";
        return false;
    }
    if (!bind_phase_words(theme, phase_words_, failed)) {
        return false;
    }

    // One day and one night glyph per kind of weather the port names, each in the tone the mockup gives it:
    // what falls as water in the domain of water, snow in the ink, the rest of the sky in the domain of the sky.
    glyphs_.clear();
    for (const std::pair<const char*, const ::godot::Color*>& kind :
         std::initializer_list<std::pair<const char*, const ::godot::Color*>>{{GODOT_HUD_WEATHER_CLEAR, &sky_},
                                                                             {GODOT_HUD_WEATHER_PCLOUD, &sky_},
                                                                             {GODOT_HUD_WEATHER_CLOUDY, &sky_},
                                                                             {GODOT_HUD_WEATHER_DRIZZLE, &water_},
                                                                             {GODOT_HUD_WEATHER_RAIN, &water_},
                                                                             {GODOT_HUD_WEATHER_SNOW, &ink},
                                                                             {GODOT_HUD_WEATHER_FOG, &sky_}}) {
        ForecastGlyph glyph;
        glyph.kind = kind.first;
        glyph.tone = *kind.second;
        const std::string key = std::string("weather_glyph.") + kind.first;
        if (!bind_word_icon(theme, key + ".day", glyph.day, failed) ||
            !bind_word_icon(theme, key + ".night", glyph.night, failed)) {
            return false;
        }
        glyphs_.push_back(glyph);
    }

    // The thresholds of perception are the spec's levels, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_temp_ = levels[1];
    level_rain_ = levels[2];

    return fade_.bind(theme, "hud_anim_fade", failed) && head_.bind_label(theme, failed) &&
           source_.bind_data(theme, failed) && phase_.bind_label(theme, failed) && temp_.bind_data(theme, failed) &&
           rain_.bind_data(theme, failed) && head_.set(head, failed) &&
           source_.set(source, failed);
}

std::string ForecastStrip::decimal(double value) const {
    const int decimals = static_cast<int>(decimals_);
    return hud_decimals(value, decimals);
}

bool ForecastStrip::draw_head(UiPen& canvas, const ::godot::Transform2D& frame,
                              const ForecastStripFrame& now, std::string& failed) {
    (void)failed;
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(sheet_x_), static_cast<float>(label_y_));
    text.size = label_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = soft_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    head_.draw(canvas, frame, text);
    if (now.level >= level_rain_) {
        UiTextPlace line = text;
        line.at = ::godot::Vector2(static_cast<float>(source_x_), static_cast<float>(label_y_));
        line.size = source_size_;
        line.anchor = GODOT_TEXT_ANCHOR_END;
        line.color = faint_;
        source_.draw(canvas, frame, line);
    }
    return true;
}

bool ForecastStrip::draw_phase(UiPen& canvas, const ::godot::Transform2D& frame,
                               const ForecastStripFrame& now, size_t index, std::string& failed) {
    const ForecastPhase& phase = now.phases[index];
    if (phase.period < GODOT_PERIOD_FIRST || phase.period > GODOT_PERIOD_LAST) {
        failed = "ForecastStrip: a forecast phase lies outside GODOT_PERIOD_*";
        return false;
    }
    const ForecastGlyph* glyph = nullptr;
    for (const ForecastGlyph& entry : glyphs_) {
        if (phase.kind != nullptr && entry.kind == phase.kind) {
            glyph = &entry;
        }
    }
    if (glyph == nullptr) {
        failed = std::string("texts.hud.weather_glyph.") + (phase.kind == nullptr ? "" : phase.kind);
        return false;
    }
    const double column = static_cast<double>(index);
    const float x = static_cast<float>(x_ + column * step_);

    // The phase's word.
    if (!phase_.set(phase_words_[static_cast<size_t>(phase.period - GODOT_PERIOD_FIRST)], failed)) {
        return false;
    }
    UiTextPlace word;
    word.at = ::godot::Vector2(x, static_cast<float>(phase_y_));
    word.size = phase_size_;
    word.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    word.color = soft_;
    word.opacity = 1.0;
    word.fade = true;
    word.delay = phase_delay_ + column * stagger_;
    word.elapsed = now.elapsed;
    word.still = now.still;
    phase_.draw(canvas, frame, word);

    // The glyph of the weather, the night's where the phase's middle lies under the horizon.
    const double shown = fade_.progress(now.elapsed, glyph_delay_ + column * stagger_, now.still);
    (phase.night ? glyph->night : glyph->day)
        .draw(canvas, frame, ::godot::Vector2(x, static_cast<float>(glyph_y_)), glyph_size_, glyph->tone, shown);

    // The temperature from the second level on.
    if (now.level >= level_temp_) {
        std::vector<HudRun> runs = temp_words_;
        if (!fill_words(runs, {HudPortValue{"t", decimal(phase.temperature)}}, failed) || !temp_.set(runs, failed)) {
            return false;
        }
        UiTextPlace text = word;
        text.at = ::godot::Vector2(x, static_cast<float>(temp_y_));
        text.size = temp_size_;
        text.color = temp_.ink();
        text.delay = temp_delay_ + column * stagger_;
        temp_.draw(canvas, frame, text);
    }

    // The millimetres from the third level on, the dash where nothing falls.
    if (now.level >= level_rain_) {
        const bool falls = phase.rain_mm > 0.0;
        std::vector<HudRun> runs = falls ? rain_words_ : none_words_;
        if (falls && !fill_words(runs, {HudPortValue{"mm", decimal(phase.rain_mm)}}, failed)) {
            return false;
        }
        if (!rain_.set(runs, failed)) {
            return false;
        }
        UiTextPlace text;
        text.at = ::godot::Vector2(x, static_cast<float>(rain_y_));
        text.size = rain_size_;
        text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        text.color = falls ? water_ : faint_;
        text.opacity = 1.0;
        text.elapsed = now.elapsed;
        text.still = now.still;
        rain_.draw(canvas, frame, text);
    }

    // The line under the column: solid near, dashed after, faint far.
    ::godot::Color tone = sky_;
    tone.a = static_cast<float>(static_cast<double>(sky_.a) * (column >= far_ ? far_alpha_ : near_alpha_));
    canvas.draw_set_transform_matrix(frame);
    const ::godot::Vector2 from(x - static_cast<float>(line_half_), static_cast<float>(line_y_));
    const ::godot::Vector2 to(x + static_cast<float>(line_half_), static_cast<float>(line_y_));
    if (column >= near_) {
        ::godot::PackedVector2Array points;
        points.push_back(from);
        points.push_back(to);
        svg_dash_points(canvas, points, tone, line_, line_dash_, line_gap_);
    } else {
        canvas.draw_line(from, to, tone, static_cast<float>(line_), true);
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

bool ForecastStrip::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                         const ForecastStripFrame& now, std::string& failed) {
    if (!draw_head(canvas, frame, now, failed)) {
        return false;
    }
    // The frame always carries the coming phases: sky.weather.v1's forecast, or the mockup's the bridge writes
    // while the port is silent (bridge_weather).
    if (now.phases.size() != phase_words_.size()) {
        failed = "ForecastStrip: the frame must forecast one phase per period of GODOT_PERIOD_*";
        return false;
    }
    for (size_t index = 0; index < now.phases.size(); ++index) {
        if (!draw_phase(canvas, frame, now, index, failed)) {
            return false;
        }
    }
    return true;
}

}  // namespace ase::adp::godot

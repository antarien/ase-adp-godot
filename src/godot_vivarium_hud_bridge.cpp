/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD BRIDGE
 *
 * @file        godot_vivarium_hud_bridge.cpp
 * @brief       VivariumView - the values the bridge writes into the HUD's frame while a port carries no record yet
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5, operator decision 2026-11-10: the HUD draws every element from
 *              its frame and never asks whether a port delivered. Where a port leaves a group of the frame empty,
 *              the bridge writes the value the accepted mockup shows (the hud_bridge_* tokens of SHA, theme.css
 *              --ase-hud-bridge-*, staged as res://config/design_tokens.json and read through UiTheme) and logs
 *              the gap once on its edge (open_gap) with the port field it reads first. Nothing of this
 *              stands in the picture; the reader on the port field stays, so the port's record takes over the
 *              moment it arrives.
 *
 *              BRIDGE GAPS (register carrier N0448): the clock bridge (godot_vivarium_hud_bridge_clock.cpp) carries
 *              N0460 the clock groups and N0459 the factor the clock runs at; their tokens are loaded here. N0449 the
 *              weather of flora.weather.v1 (ase-weather, ase-atmosphere): the sky band's
 *              glyph, temperature and wind, the forecast strip's coming phases and the weather veil's kind and wind.
 *              N0454 the growth toward the next stage of flora.patch.v1 (growth_share) and, where the forecast port
 *              carries no due hour, the days to that stage, and the share of the year of the day the next stage is
 *              reached (the year ring's mark, flora.patch.v1 next_share). N0456 the loosened share of the soil
 *              (flora.patch.v1 loose). N0457 the tap and its vessel (flora.sap.v1). All three in hud_bridge_patch_*.
 *              N0452 the verdict on a move of a carried thing (flora.stow.v1 code), judged by the mockup's fits().
 *              N0461 the signs' thresholds and values (hud_bridge_sign_*, read by
 *              VivariumSignRules). N0450 the past events and their span (hud_bridge_trail_*, read by
 *              VivariumTrailBridge, godot_vivarium_hud_trail.cpp). The hand bridge
 *              (godot_vivarium_hud_hand.cpp) carries N0454's preview after the pour, N0462 the internodes that may
 *              be cut (flora.scene.v1 cuttable) and N0463 the root zone and the roots in the cut (flora.root.v1
 *              zone_m and segments); the carry bridge (godot_vivarium_hud_carry.cpp) carries N0451 the carried
 *              things (flora.carry.v1).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The token `prefix<n>suffix` (n counted from 1) into `out`; false with the token's name in `failed`. */
bool read_nth(const UiTheme& theme, const std::string& prefix, uint32_t index, const char* suffix, double& out,
              std::string& failed) {
    const std::string token = prefix + std::to_string(index + 1) + suffix;
    if (!theme.measure(token, out)) {
        failed = token;
        return false;
    }
    return true;
}

/** `count` tokens `prefix<n>suffix` in their order; false with the missing token's name in `failed`. */
bool read_list(const UiTheme& theme, const char* prefix, const char* suffix, uint32_t count, std::vector<double>& out,
               std::string& failed) {
    out.clear();
    for (uint32_t index = 0; index < count; ++index) {
        double number = 0.0;
        if (!read_nth(theme, prefix, index, suffix, number, failed)) {
            return false;
        }
        out.push_back(number);
    }
    return true;
}

/** The token `token` into `out`, which must lie in [low, high]; false with the token's name in `failed`. */
bool read_in(const UiTheme& theme, const char* token, double low, double high, double& out, std::string& failed) {
    if (!theme.measure(token, out) || out < low || out > high) {
        failed = token;
        return false;
    }
    return true;
}

/** The GODOT_HUD_WEATHER_* key spelled `word`, nullptr for a word that names no kind of weather. */
const char* weather_key(const ::godot::String& word) {
    const char* const keys[] = {GODOT_HUD_WEATHER_CLEAR, GODOT_HUD_WEATHER_PCLOUD, GODOT_HUD_WEATHER_CLOUDY,
                                GODOT_HUD_WEATHER_FOG,   GODOT_HUD_WEATHER_RAIN,   GODOT_HUD_WEATHER_DRIZZLE,
                                GODOT_HUD_WEATHER_SNOW};
    for (const char* key : keys) {
        if (word == ::godot::String(key)) {
            return key;
        }
    }
    return nullptr;
}

/** The coming phase `index` of the bridge's forecast (hud_bridge_forecast_<n>_*): its period, kind, night,
 *  temperature and rain; false with the failing token's name in `failed`. */
bool read_phase(const UiTheme& theme, uint32_t index, ForecastPhase& out, std::string& failed) {
    const std::string prefix = "hud_bridge_forecast_";
    const std::string phase = prefix + std::to_string(index + 1);
    double period = 0.0;
    double night = 0.0;
    if (!read_nth(theme, prefix, index, "_period", period, failed) ||
        !read_nth(theme, prefix, index, "_night", night, failed) ||
        !read_nth(theme, prefix, index, "_temperature", out.temperature, failed) ||
        !read_nth(theme, prefix, index, "_rain_mm", out.rain_mm, failed)) {
        return false;
    }
    ::godot::String kind;
    if (!theme.token_text(phase + "_kind", kind) || weather_key(kind) == nullptr) {
        failed = phase + "_kind: a GODOT_HUD_WEATHER_* key";
        return false;
    }
    if (period < static_cast<double>(GODOT_PERIOD_FIRST) || period > static_cast<double>(GODOT_PERIOD_LAST)) {
        failed = phase + "_period: a GODOT_PERIOD_*";
        return false;
    }
    if (!(night == 0.0 || night == 1.0) || out.rain_mm < 0.0) {
        failed = phase + "_night: 0 or 1, " + phase + "_rain_mm: from 0";
        return false;
    }
    out.period = static_cast<int32_t>(period);
    out.kind = weather_key(kind);
    out.night = night == 1.0;
    return true;
}

}  // namespace

bool VivariumView::load_hud_bridge(const UiTheme& theme) {
    // hud_bridge_clock_* of SHA (N0460): the date, the day, the phases and the seasons the mockup shows. No share
    // stands there; bridge_clock computes each by the mockup's rules from the hours and the date it ends up with.
    std::string failed;
    VivariumClockView& out = hud_bridge_clock_;
    out = VivariumClockView{};
    const char* const keys[] = {"hud_bridge_clock_year",           "hud_bridge_clock_month",
                                "hud_bridge_clock_month_day",      "hud_bridge_clock_year_day",
                                "hud_bridge_clock_hour",           "hud_bridge_clock_period",
                                "hud_bridge_clock_hours_per_day",  "hud_bridge_clock_days_per_month",
                                "hud_bridge_clock_months_per_year", "hud_bridge_clock_sunrise_h",
                                "hud_bridge_clock_sunset_h",       "hud_bridge_clock_daylight_h",
                                "hud_bridge_clock_season"};
    double* const values[] = {&out.year,          &out.month,           &out.month_day, &out.year_day,
                              &out.hour,          &out.period,          &out.hours_per_day,
                              &out.days_per_month, &out.months_per_year, &out.sunrise_h, &out.sunset_h,
                              &out.daylight_h,    &out.season};
    for (uint32_t index = 0; index < sizeof(keys) / sizeof(keys[0]); ++index) {
        if (!theme.measure(keys[index], *values[index])) {
            return fail_config(keys[index]);
        }
    }
    if (!(out.hours_per_day > 0.0) || !(out.days_per_month > 0.0) || !(out.months_per_year > 0.0)) {
        return fail_config("hud_bridge_clock_hours_per_day, _days_per_month, _months_per_year: above 0");
    }
    if (out.period < static_cast<double>(GODOT_PERIOD_FIRST) || out.period > static_cast<double>(GODOT_PERIOD_LAST)) {
        return fail_config("hud_bridge_clock_period: a GODOT_PERIOD_*");
    }
    if (!read_list(theme, "hud_bridge_clock_phase_", "_start_h", GODOT_DAY_PHASES, out.phase_start_h, failed) ||
        !read_list(theme, "hud_bridge_clock_season_", "_start_lun", GODOT_YEAR_SEASONS, out.season_start_lun,
                   failed) ||
        !read_list(theme, "hud_bridge_clock_season_", "_light_h", GODOT_YEAR_SEASONS, out.season_light_h, failed)) {
        return fail_config(failed.c_str());
    }
    if (out.season < 0.0 || out.season >= static_cast<double>(GODOT_YEAR_SEASONS)) {
        return fail_config("hud_bridge_clock_season: a season 0 .. GODOT_YEAR_SEASONS - 1");
    }
    // The readings name the season by its word (bridge_signs): every season of the year carries one.
    std::vector<HudRun> season_runs;
    for (uint32_t season = 0; season < GODOT_YEAR_SEASONS; ++season) {
        if (!theme.words("seasons." + std::to_string(season), season_runs)) {
            return fail_config("texts.hud.seasons.<season>: a word for every season");
        }
    }

    // hud_bridge_weather_* and hud_bridge_forecast_<n>_* of SHA (N0449): the sky, the temperature, the wind and
    // the coming phases the mockup shows, one phase per period.
    ::godot::String kind;
    if (!theme.token_text("hud_bridge_weather_kind", kind) || weather_key(kind) == nullptr) {
        return fail_config("hud_bridge_weather_kind: a GODOT_HUD_WEATHER_* key");
    }
    hud_bridge_weather_ = weather_key(kind);
    // THE WEATHER OF THE STAGE (modes.weather, usecase:5606 observe the sky): the mockup switches its sky from a
    // panel outside the phone; the app takes the same switch from its launch. The word after the argument stands
    // in for the artefact's kind; the argument without a known key refuses the configuration.
    const ::godot::PackedStringArray launch = ::godot::OS::get_singleton()->get_cmdline_user_args();
    const int64_t named = modes_.weather_argument.is_empty() ? -1 : launch.find(modes_.weather_argument);
    if (named >= 0) {
        if (named + 1 >= launch.size() || weather_key(launch[named + 1]) == nullptr) {
            return fail_config("modes.weather: a GODOT_HUD_WEATHER_* key after the argument");
        }
        hud_bridge_weather_ = weather_key(launch[named + 1]);
        note("stage weather " + launch[named + 1]);
    }
    // THE PERCEPTION OF THE STAGE (modes.perception, usecase:101440 grow the interface with skills): the mockup
    // switches its depth from the same panel (rPerc I, II, III); the number after the argument stands in for
    // hud.perception.start until flora.skill.v1 files a level, and one outside hud.perception.levels refuses the
    // configuration.
    hud_bridge_level_ = hud_.level_start();
    const int64_t leveled = modes_.perception_argument.is_empty() ? -1 : launch.find(modes_.perception_argument);
    if (leveled >= 0) {
        const bool given = leveled + 1 < launch.size() && launch[leveled + 1].is_valid_int();
        const int64_t asked = given ? launch[leveled + 1].to_int() : 0;
        bool known = false;
        for (const uint32_t level : theme.spec().levels) {
            known = known || static_cast<int64_t>(level) == asked;
        }
        if (!known) {
            return fail_config("modes.perception: one of hud.perception.levels after the argument");
        }
        hud_bridge_level_ = static_cast<uint32_t>(asked);
        note("stage perception " + launch[leveled + 1]);
    }
    if (!theme.measure("hud_bridge_weather_temperature", hud_bridge_temperature_)) {
        return fail_config("hud_bridge_weather_temperature");
    }
    if (!theme.measure("hud_bridge_weather_wind", hud_bridge_wind_) || hud_bridge_wind_ < 0.0) {
        return fail_config("hud_bridge_weather_wind: from 0");
    }
    hud_bridge_forecast_.clear();
    for (uint32_t index = 0; index < GODOT_DAY_PHASES; ++index) {
        ForecastPhase phase;
        if (!read_phase(theme, index, phase, failed)) {
            return fail_config(failed.c_str());
        }
        hud_bridge_forecast_.push_back(phase);
    }

    // hud_bridge_patch_* of SHA (N0454, N0456, N0457): the growth, the loose soil and the tap the mockup shows.
    double running = 0.0;
    if (!read_in(theme, "hud_bridge_patch_growth_share", 0.0, 1.0, hud_bridge_growth_, failed) ||
        !read_in(theme, "hud_bridge_patch_loose_share", 0.0, 1.0, hud_bridge_loose_, failed) ||
        !read_in(theme, "hud_bridge_patch_sap_running", 0.0, 1.0, running, failed)) {
        return fail_config((failed + ": a share 0..1").c_str());
    }
    if (!(running == 0.0 || running == 1.0)) {
        return fail_config("hud_bridge_patch_sap_running: 0 or 1");
    }
    hud_bridge_sap_running_ = running == 1.0;
    if (!theme.measure("hud_bridge_patch_grow_per_hour", hud_bridge_grow_per_hour_) ||
        !(hud_bridge_grow_per_hour_ > 0.0)) {
        return fail_config("hud_bridge_patch_grow_per_hour: above 0");
    }
    if (!theme.measure("hud_bridge_patch_sap_l", hud_bridge_sap_l_) || hud_bridge_sap_l_ < 0.0) {
        return fail_config("hud_bridge_patch_sap_l: from 0");
    }
    if (!theme.measure("hud_bridge_patch_sap_cap_l", hud_bridge_sap_cap_l_) || !(hud_bridge_sap_cap_l_ > 0.0)) {
        return fail_config("hud_bridge_patch_sap_cap_l: above 0");
    }
    if (!read_in(theme, "hud_bridge_patch_next_share", 0.0, 1.0, hud_bridge_next_share_, failed)) {
        return fail_config("hud_bridge_patch_next_share: a share 0..1");
    }

    // hud_bridge_pace_factor of SHA (N0459): the factor the mockup's clock runs at before any is asked.
    if (!theme.measure("hud_bridge_pace_factor", hud_bridge_pace_factor_) || !(hud_bridge_pace_factor_ > 0.0)) {
        return fail_config("hud_bridge_pace_factor: above 0");
    }

    // hud_bridge_sign_* of SHA (N0461): the thresholds and values of the mockup's signs() (VivariumSignRules).
    if (!hud_sign_rules_.load(theme, failed)) {
        return fail_config(failed.c_str());
    }

    // hud_bridge_trail_* of SHA (N0450): the span and the events of the mockup's S.log (VivariumTrailBridge).
    if (!hud_trail_.load(theme, failed)) {
        return fail_config(failed.c_str());
    }

    // hud_bridge_mark_* of SHA: the threshold and the state of the mockup's TOOLS[].ok().
    double uncut = 0.0;
    if (!read_in(theme, "hud_bridge_mark_soil_full_rel", 0.0, 1.0, hud_bridge_soil_full_, failed) ||
        !read_in(theme, "hud_bridge_mark_branch_uncut", 0.0, 1.0, uncut, failed) ||
        !(uncut == 0.0 || uncut == 1.0)) {
        return fail_config("hud_bridge_mark_soil_full_rel: a share 0..1, hud_bridge_mark_branch_uncut: 0 or 1");
    }
    hud_bridge_branch_uncut_ = uncut == 1.0;
    return true;
}

void VivariumView::bridge_patch(FocusClusterFrame& focus, const VivariumPatchView* patch,
                                const VivariumClockView& clock, YearRingFrame& year) {
    // THE EDGE OF THE BRIDGE FOR N0454: flora.patch.v1 carries no growth toward the next stage yet, so the ring's
    // arc and number stand on the mockup's share (hud_bridge_patch_growth_share), the gap logged once with the
    // port field named.
    open_gap(GODOT_HUD_GAP_PATCH, "flora.patch.v1.growth_share");
    focus.growth_share = hud_bridge_growth_;

    // The days to the next stage stand on ase-plant's forecast where the forecast port carries one (grow_at_h for
    // CRESCENDO, mtur_at_h for MADURA, an absolute game hour, NaN unmeasured); otherwise they are the mockup's
    // daysToNext(): the way left over the growth of a day.
    const uint32_t stage = patch != nullptr && patch->stage != GODOT_CODE_UNKNOWN ? patch->stage : 0u;
    const uint32_t next = stage + 1u;
    focus.next_due = next < GODOT_STAGE_COUNT && !(patch != nullptr && patch->condition == GODOT_COND_DEAD);
    const double hours =
        clock.known && clock.hours_per_day > 0.0 ? clock.hours_per_day : hud_bridge_clock_.hours_per_day;
    double due_h = GODOT_HOUR_NONE;
    if (patch != nullptr && next == GODOT_STAGE_GROW) {
        due_h = patch->grow_at_h;
    } else if (patch != nullptr && next == GODOT_STAGE_MATURE) {
        due_h = patch->mtur_at_h;
    }
    if (due_h >= 0.0 && clock.known && hours > 0.0) {
        const double now_h = clock.day * hours + clock.hour;
        focus.next_days = ase::math::max(0.0, (due_h - now_h) / hours);
        close_gap(GODOT_HUD_GAP_PATCH, "forecast.due_h");
    } else {
        // ase-plant: PLT_GROW_AT_H and PLT_MTUR_AT_H of the forecast port; Broto carries none.
        open_gap(GODOT_HUD_GAP_PATCH, "forecast.due_h");
        focus.next_days = (1.0 - focus.growth_share) / (hud_bridge_grow_per_hour_ * hours);
    }
    // The next stage's mark on the year ring: the stage after this one and the share of the year of the day it is
    // reached, as ase-calendar places it; flora.patch.v1 files no such share yet, so it is the mockup's
    // (hud_bridge_patch_next_share), the gap logged once with the port field named.
    year.next_due = focus.next_due;
    year.next_stage = next;
    open_gap(GODOT_HUD_GAP_PATCH, "flora.patch.v1.next_share");
    year.next_share = hud_bridge_next_share_;

    // N0456: no port carries the loosened share of the soil yet (flora.patch.v1 loose). The loose bridge keeps it
    // point by point: rain settles it over the game hours (m.html 1030), the patch reads the mean within its reach
    // (patchLoose, m.html 372); before the patch's centre is known the start share stands.
    open_gap(GODOT_HUD_GAP_LOOSE, "flora.patch.v1.loose");
    if (clock.known && hours > 0.0) {
        hud_loose_.settle(clock.day * hours + clock.hour, hud_bridge_weather_);
    }
    focus.loose_share = patch != nullptr ? hud_loose_.share(patch->x, patch->z) : hud_bridge_loose_;

    // N0457: no port carries the tap and its vessel yet (flora.sap.v1 running, l, cap_l).
    open_gap(GODOT_HUD_GAP_SAP, "flora.sap.v1");
    focus.sap_running = hud_bridge_sap_running_;
    focus.sap_l = hud_bridge_sap_l_;
    focus.sap_cap_l = hud_bridge_sap_cap_l_;
}

void VivariumView::bridge_weather(SkyBandFrame& band, ForecastStripFrame& forecast, WeatherVeilFrame& veil,
                                  HandSlotFrame& hand, SkyOvercastFrame& sky) {
    // THE EDGE OF THE BRIDGE FOR N0449: flora.weather.v1 of ase-weather and ase-atmosphere (kind, temperature,
    // wind and one forecast record per coming phase: period, kind, night, temperature, rain_mm) files no record
    // yet, so the sky band, the forecast strip, the weather veil and the hand's rain warning stand on the
    // mockup's weather (hud_bridge_weather_* of SHA) and the gap is logged once with the port named.
    open_gap(GODOT_HUD_GAP_WEATHER, "flora.weather.v1");
    band.weather_kind = hud_bridge_weather_;
    band.temperature = hud_bridge_temperature_;
    band.wind = hud_bridge_wind_;
    forecast.phases = hud_bridge_forecast_;
    veil.kind = hud_bridge_weather_;
    veil.wind = hud_bridge_wind_;
    // THE SKY IS TINTED ONCE (D2280). The mockup's weather is no state of the atmosphere ase-pl-sky lights from,
    // so the sky port's colours carry no tint of it and SkyOvercast lays the grey of this kind over them. The
    // record of flora.weather.v1 comes from the same atmosphere SkyAtspWthrSystem tints fog, background and sun
    // from: its reader fills the band, the strip, the veil and the hand and leaves the sky frame without a kind,
    // so SkyOvercast keeps the sky port's colours and light as delivered.
    sky.kind = hud_bridge_weather_;

    // The mockup's rainIn(): the first coming phase in which rain or drizzle falls, counted from one, and the
    // millimetres it brings; no such phase leaves the warning unset.
    hand.rain_coming = false;
    hand.rain_phases = 0;
    hand.rain_mm = 0.0;
    for (size_t index = 0; index < hud_bridge_forecast_.size(); ++index) {
        const std::string kind(hud_bridge_forecast_[index].kind);
        if (kind == GODOT_HUD_WEATHER_RAIN || kind == GODOT_HUD_WEATHER_DRIZZLE) {
            hand.rain_coming = true;
            hand.rain_phases = static_cast<uint32_t>(index + 1);
            hand.rain_mm = hud_bridge_forecast_[index].rain_mm;
            return;
        }
    }
}

void VivariumView::bridge_stow(DragLayerFrame& drag) {
    // THE EDGE OF THE BRIDGE FOR N0452: flora.stow.v1 files no verdict on a move yet, so the bridge judges the dragged
    // thing at the place the drag stands over by the mockup's fits() (VivariumCarryBridge::answer) and the layer takes
    // it as the port's last word (field stow.code). The layer matches key, place and rack itself, so a verdict for a
    // place the finger already left is never applied.
    drag.answer = nullptr;
    const DragLayer& dragging = hud_.drag();
    if (!dragging.moved() || dragging.target().place == nullptr) {
        return;
    }
    open_gap(GODOT_HUD_GAP_STOW, "flora.stow.v1.code");
    if (hud_carry_.answer(dragging.thing().key, dragging.target().place, dragging.target().rack, hud_stow_)) {
        drag.answer = &hud_stow_;
    }
}

void VivariumView::bridge_skill(HudGameState& game) {
    // THE EDGE OF THE BRIDGE FOR N0453: flora.skill.v1 files no level yet (field skill.level), so the level the
    // resolver opens the skills by is the start level SHA states (hud.perception.start, VivariumHud::level_start),
    // or the level the launch names (modes.perception, load_hud_bridge).
    open_gap(VivariumHud::GAP_SKILL, "flora.skill.v1.level");
    game.level = hud_bridge_level_;
}

void VivariumView::bridge_trail(TrailViewFrame& trail, YearRingFrame& year) {
    // THE EDGE OF THE BRIDGE FOR N0450: flora.trail.v1 files no past event yet, so the trail on arrival and the ring's
    // dots are the mockup's S.log, each at the share of the year SHA states for it (hud_bridge_trail_*, read by
    // VivariumTrailBridge). The log lines name every field of the record the reader takes, per event and for the span.
    open_gap(GODOT_HUD_GAP_TRAIL, "flora.trail.v1.events(kind,tone,lun,tum,year_share,values,case_id,sign,alert)");
    open_gap(GODOT_HUD_GAP_TRAIL, "flora.trail.v1.span(days,lun,from,to)");
    hud_trail_.trail(trail);
    hud_trail_.ring(year);
}

}  // namespace ase::adp::godot

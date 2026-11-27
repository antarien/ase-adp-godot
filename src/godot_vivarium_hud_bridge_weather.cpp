/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD WEATHER BRIDGE
 *
 * @file        godot_vivarium_hud_bridge_weather.cpp
 * @brief       VivariumView - the weather the HUD's frame takes from sky.weather.v1, and the bridge while no record stands
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5, design decision 2026-11-10: the sky band, the forecast strip, the
 *              weather veil, the hand's rain warning and the sky's overcast draw from their frame and never ask
 *              whether a port delivered. ase-atmosphere computes the kind (ATM_WEATHER_KIND), the temperature and
 *              the wind (ATM_WIND_SPEED) as part of the water cycle; ase-pl-sky files them as the current record of
 *              sky.weather.v1 (SkyHostWthrExptSystem), and ase-atmosphere's codes of the kinds on
 *              sky.weather.code.v1 (SkyHostWthrCodeExptSystem). The forecast records per coming period are read
 *              from the same port; no module computes a forecast yet, so they are the open half of N0449, owed by
 *              ase-atmosphere. The port names come from the start configuration (ports.weather,
 *              ports.weather_code). GodotHostResourceManager::read_weather copies both into
 *              VivariumWeatherView; this bridge turns each kind into its GODOT_HUD_WEATHER_* key by those codes and
 *              closes the gap. While a record or a code is missing, the bridge writes the weather of the accepted
 *              mockup (hud_bridge_weather_* and hud_bridge_forecast_<n>_* of SHA, theme.css --ase-hud-bridge-*,
 *              staged as res://config/design_tokens.json and read through UiTheme) and logs the gap once on its edge
 *              (open_gap) with the port and its keys named. Nothing of this stands in the picture.
 *
 *              BRIDGE GAP (register carrier N0448): N0449 the weather of sky.weather.v1: kind, temperature, wind
 *              and one forecast record per coming phase (period, kind, night, temperature, rain_mm), and the codes of
 *              sky.weather.code.v1 (clear, pcloud, cloudy, drizzle, rain, snow, fog). The launch may name the kind
 *              of the stage (modes.weather, usecase:5606 observe the sky), as the mockup switches its sky from a
 *              panel outside the phone; that switch stands in for the bridge's kind only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0449 (sky.weather.v1, sky.weather.code.v1), carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/hud/gap/types.hpp>
#include <ase/adp/godot/hud/weather/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

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

/** The GODOT_HUD_WEATHER_* key of ase-atmosphere's kind `kind`, compared against the codes the port filed
 *  (ATM_CONST_WEATHER_KIND_*); nullptr for a kind none of the seven codes names. */
const char* coded_key(const VivariumWeatherView& weather, double kind) {
    const double codes[] = {weather.code_clear, weather.code_pcloud, weather.code_cloudy, weather.code_drizzle,
                            weather.code_rain,  weather.code_snow,   weather.code_fog};
    const char* const keys[] = {GODOT_HUD_WEATHER_CLEAR,   GODOT_HUD_WEATHER_PCLOUD, GODOT_HUD_WEATHER_CLOUDY,
                                GODOT_HUD_WEATHER_DRIZZLE, GODOT_HUD_WEATHER_RAIN,   GODOT_HUD_WEATHER_SNOW,
                                GODOT_HUD_WEATHER_FOG};
    for (size_t at = 0; at < sizeof(codes) / sizeof(codes[0]); ++at) {
        if (kind == codes[at]) {
            return keys[at];
        }
    }
    return nullptr;
}

/** The coming phase `index` of the bridge's forecast (hud_bridge_forecast_<n>_*, n counted from 1): its period, kind,
 *  night, temperature and rain; false with the failing token's name in `failed`. */
bool read_phase(const UiTheme& theme, uint32_t index, ForecastPhase& out, std::string& failed) {
    const std::string phase = "hud_bridge_forecast_" + std::to_string(index + 1);
    double period = 0.0;
    double night = 0.0;
    const char* const suffixes[] = {"_period", "_night", "_temperature", "_rain_mm"};
    double* const values[] = {&period, &night, &out.temperature, &out.rain_mm};
    for (uint32_t at = 0; at < sizeof(suffixes) / sizeof(suffixes[0]); ++at) {
        if (!theme.measure(phase + suffixes[at], *values[at])) {
            failed = phase + suffixes[at];
            return false;
        }
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

/** The forecast of the port as the strip's phases, in the order ase-pl-sky filed them; false when a kind is
 *  none of the seven codes, so the strip keeps the bridge's forecast whole rather than a mixed one. */
bool port_forecast(const VivariumWeatherView& weather, std::vector<ForecastPhase>& out) {
    out.clear();
    for (const VivariumWeatherPhaseView& record : weather.forecast) {
        ForecastPhase phase;
        phase.kind = coded_key(weather, record.kind);
        if (phase.kind == nullptr) {
            return false;
        }
        phase.period = static_cast<int32_t>(record.period);
        phase.night = record.night == 1.0;
        phase.temperature = record.temperature;
        phase.rain_mm = record.rain_mm;
        out.push_back(phase);
    }
    return !out.empty();
}

}  // namespace

bool VivariumView::load_hud_bridge_weather(const UiTheme& theme) {
    // hud_bridge_weather_* and hud_bridge_forecast_<n>_* of SHA (N0449): the sky, the temperature, the wind and
    // the coming phases the mockup shows, one phase per period.
    ::godot::String kind;
    if (!theme.token_text("hud_bridge_weather_kind", kind) || weather_key(kind) == nullptr) {
        return fail_config("hud_bridge_weather_kind: a GODOT_HUD_WEATHER_* key");
    }
    hud_state_.hud_bridge_weather_ = weather_key(kind);
    // THE WEATHER OF THE STAGE (modes.weather, usecase:5606 observe the sky): the mockup switches its sky from a
    // panel outside the phone; the app takes the same switch from its launch. The word after the argument stands
    // in for the artefact's kind; the argument without a known key refuses the configuration.
    const ::godot::PackedStringArray launch = ::godot::OS::get_singleton()->get_cmdline_user_args();
    const int64_t named = modes_.weather_argument.is_empty() ? -1 : launch.find(modes_.weather_argument);
    if (named >= 0) {
        if (named + 1 >= launch.size() || weather_key(launch[named + 1]) == nullptr) {
            return fail_config("modes.weather: a GODOT_HUD_WEATHER_* key after the argument");
        }
        hud_state_.hud_bridge_weather_ = weather_key(launch[named + 1]);
        note("stage weather " + launch[named + 1]);
    }
    if (!theme.measure("hud_bridge_weather_temperature", hud_state_.hud_bridge_temperature_)) {
        return fail_config("hud_bridge_weather_temperature");
    }
    if (!theme.measure("hud_bridge_weather_wind", hud_state_.hud_bridge_wind_) || hud_state_.hud_bridge_wind_ < 0.0) {
        return fail_config("hud_bridge_weather_wind: from 0");
    }
    std::string failed;
    hud_state_.hud_bridge_forecast_.clear();
    for (uint32_t index = 0; index < GODOT_DAY_PHASES; ++index) {
        ForecastPhase phase;
        if (!read_phase(theme, index, phase, failed)) {
            return fail_config(failed.c_str());
        }
        hud_state_.hud_bridge_forecast_.push_back(phase);
    }
    return true;
}

void VivariumView::bridge_weather(SkyBandFrame& band, ForecastStripFrame& forecast, WeatherVeilFrame& veil,
                                  HandSlotFrame& hand, SkyOvercastFrame& sky) {
    // THE WEATHER OF THE PORT, READ EVERY FRAME (N0449): the current record of the weather port and the codes of
    // the code port (ports.weather, ports.weather_code of the start configuration) give the kind, the temperature
    // and the wind of the sky band and the veil; the forecast records give the strip and the hand's rain warning.
    // A part without its record or with a kind none of the codes names stands on the mockup's weather
    // (hud_bridge_weather_* of SHA), and the gap is logged once with the port and its keys named.
    const std::string request = start_.weather_port + "(kind,temperature,wind;period,kind,night,temperature,rain_mm) " +
                                start_.weather_code_port + "(clear,pcloud,cloudy,drizzle,rain,snow,fog)";
    const char* act = request.c_str();
    const VivariumWeatherView& weather = resources_.get_weather();
    const char* current = weather.known ? coded_key(weather, weather.kind) : nullptr;
    std::vector<ForecastPhase> coming;
    const bool coming_known = weather.forecast_known && port_forecast(weather, coming);
    if (current != nullptr && coming_known) {
        close_gap(GODOT_HUD_GAP_WEATHER, act);
    } else {
        open_gap(GODOT_HUD_GAP_WEATHER, act);
    }

    band.weather_kind = current != nullptr ? current : hud_state_.hud_bridge_weather_;
    band.temperature = current != nullptr ? weather.temperature : hud_state_.hud_bridge_temperature_;
    band.wind = current != nullptr ? weather.wind : hud_state_.hud_bridge_wind_;
    veil.kind = band.weather_kind;
    veil.wind = band.wind;
    // THE SKY IS TINTED ONCE (D2280). The mockup's weather is no state of the atmosphere ase-pl-sky lights from,
    // so the sky port's colours carry no tint of it and SkyOvercast lays the grey of this kind over them. The
    // record of the weather port comes from the same atmosphere SkyAtspWthrSystem tints fog, background and sun
    // from: with it the sky frame stays without a kind, so SkyOvercast keeps the sky port's colours and light as
    // delivered.
    sky.kind = current != nullptr ? nullptr : hud_state_.hud_bridge_weather_;

    forecast.phases = coming_known ? coming : hud_state_.hud_bridge_forecast_;
    // The mockup's rainIn(): the first coming phase in which rain or drizzle falls, counted from one, and the
    // millimetres it brings; no such phase leaves the warning unset.
    hand.rain_coming = false;
    hand.rain_phases = 0;
    hand.rain_mm = 0.0;
    for (size_t index = 0; index < forecast.phases.size(); ++index) {
        const std::string kind(forecast.phases[index].kind);
        if (kind == GODOT_HUD_WEATHER_RAIN || kind == GODOT_HUD_WEATHER_DRIZZLE) {
            hand.rain_coming = true;
            hand.rain_phases = static_cast<uint32_t>(index + 1);
            hand.rain_mm = forecast.phases[index].rain_mm;
            return;
        }
    }
}

}  // namespace ase::adp::godot

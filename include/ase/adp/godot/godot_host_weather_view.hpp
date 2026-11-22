#pragma once

/**
 * ASE GODOT ADAPTER - HOST WEATHER VIEW
 *
 * @file        godot_host_weather_view.hpp
 * @brief       The weather of the sky over the vivarium as the last snapshot of the weather ports showed it
 * @description The copy GodotHostResourceManager keeps of the current weather, the forecast of each coming day
 *              period and ase-atmosphere's codes of the kinds of weather (PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5,
 *              register N0449 under carrier N0448). ase-atmosphere computes the kind (ATM_WEATHER_KIND), the
 *              temperature and the wind (ATM_WIND_SPEED); ase-pl-sky files them on sky.weather.v1
 *              (SkyHostWthrExptSystem) and the codes on sky.weather.code.v1 (SkyHostWthrCodeExptSystem); the
 *              forecast records no module computes yet (N0449, owed by ase-atmosphere). One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that draw
 *              the weather read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @register    N0449 (sky.weather.v1, sky.weather.code.v1), carrier N0448
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <vector>

namespace ase::adp::godot {

/**
 * One forecast record of sky.weather.v1 - a copy: the coming day period (TIM_DAY_PERIOD code), the kind of
 * weather ase-atmosphere forecasts for it, whether the middle of the period lies under the horizon, its temperature
 * and the millimetres that fall in it. A key the record does not carry stands as NaN. The view computes none of
 * the values (usecase:5606 observe the sky).
 */
struct VivariumWeatherPhaseView {
    double period = 0.0;       // the day period, GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST (TIM_CONST_DAY_PERIOD_*)
    double kind = 0.0;         // ATM_WEATHER_KIND forecast for the period, one of the codes of VivariumWeatherView
    double night = 0.0;        // 1 when the middle of the period lies under the horizon, 0 otherwise
    double temperature = 0.0;  // degrees Celsius forecast for the period
    double rain_mm = 0.0;      // the millimetres that fall in the period, 0 where nothing falls
};

/**
 * The weather of the sky as the last snapshot of the weather ports showed it - a copy (N0449). `known` holds once
 * the current record came with its kind, temperature and wind and the code record with all seven codes;
 * `forecast_known` once every coming period came exactly once with all its keys. A kind is compared against the
 * codes ase-atmosphere filed (ATM_CONST_WEATHER_KIND_*), never against a number of the adapter.
 */
struct VivariumWeatherView {
    bool   known = false;           // the current record and the code record stand complete
    bool   forecast_known = false;  // one complete forecast record per day period stands
    double kind = 0.0;              // ATM_WEATHER_KIND of the current record
    double temperature = 0.0;       // degrees Celsius of the current record
    double wind = 0.0;              // ATM_WIND_SPEED of the current record, m/s from 0
    double code_clear = 0.0;        // ATM_CONST_WEATHER_KIND_CLEAR, key clear of the code record
    double code_pcloud = 0.0;       // ATM_CONST_WEATHER_KIND_PCLOUD, key pcloud
    double code_cloudy = 0.0;       // ATM_CONST_WEATHER_KIND_CLOUDY, key cloudy
    double code_drizzle = 0.0;      // ATM_CONST_WEATHER_KIND_DRIZZLE, key drizzle
    double code_rain = 0.0;         // ATM_CONST_WEATHER_KIND_RAIN, key rain
    double code_snow = 0.0;         // ATM_CONST_WEATHER_KIND_SNOW, key snow
    double code_fog = 0.0;          // ATM_CONST_WEATHER_KIND_FOG, key fog
    std::vector<VivariumWeatherPhaseView> forecast;  // one record per coming period in period order, entity per item
};

}  // namespace ase::adp::godot

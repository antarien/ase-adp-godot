#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the weather kinds sky.weather.v1 names, as the mockup's S.weather
 * @description The veil (WeatherVeil) draws none of the first three kinds, the sky band (SkyBand) gives every
 *              kind its day and night glyph.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The kinds are read by the veil, the sky band and the weather
 *              bridge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

namespace ase::adp::godot {

constexpr const char* GODOT_HUD_WEATHER_CLEAR = "clear";              // an open sky
constexpr const char* GODOT_HUD_WEATHER_PCLOUD = "pcloud";            // a sky partly clouded
constexpr const char* GODOT_HUD_WEATHER_CLOUDY = "cloudy";            // a sky clouded over
constexpr const char* GODOT_HUD_WEATHER_FOG = "fog";                  // bands of mist over the horizon
constexpr const char* GODOT_HUD_WEATHER_RAIN = "rain";                // streaks leaning with the wind
constexpr const char* GODOT_HUD_WEATHER_DRIZZLE = "drizzle";          // short fine dashes
constexpr const char* GODOT_HUD_WEATHER_SNOW = "snow";                // swaying flakes

}  // namespace ase::adp::godot

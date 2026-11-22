#pragma once

/**
 * ASE GODOT ADAPTER - HUD WEATHER VEIL
 *
 * @file        godot_hud_weather_veil.hpp
 * @brief       The weather drawn over the scene and under the shade: fog bands, rain, drizzle, snow
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block WeatherVeil (the mockup's drawWeather, drawn
 *              after the world and the terrain overlay and before ScreenShade).
 *
 *              THE KINDS. A clear, partly clouded or clouded sky draws nothing. GODOT_HUD_WEATHER_FOG draws
 *              hud_weather_fog_bands bands, the first hud_weather_fog_y from the horizon and each further one
 *              hud_weather_fog_step below, each fading from WEATHER_FOG_CLEAR through WEATHER_FOG back to
 *              WEATHER_FOG_CLEAR over hud_weather_fog_half on either side. GODOT_HUD_WEATHER_RAIN, _DRIZZLE and
 *              _SNOW move one set of hud_weather_particles particles drawn from hud_weather_seed.
 *
 *              THE PARTICLES. Each has a speed share hud_weather_speed plus hud_weather_speed_span times a draw
 *              and a phase over one full turn. It falls by the kind's fall speed times its share and drifts with
 *              the wind times hud_weather_wind times the kind's drift; snow sways by hud_weather_snow_sway at
 *              hud_weather_snow_sway_rate on top. Below hud_weather_wrap_y it starts again at hud_weather_spawn_y
 *              at a fresh draw across hud_weather_spawn_span from hud_weather_spawn_x; past hud_weather_wrap_x
 *              it goes back by hud_weather_wrap_width. Rain is a streak in WEATHER_RAIN leaning against the
 *              wind, drizzle a fleck in WEATHER_DRIZZLE, snow a flake in WEATHER_SNOW whose radius grows with
 *              the share. Every draw is ase::math::random_hash_float over the seed and a running key, so the
 *              veil repeats from bind to bind.
 *
 *              THE SURFACE. The spawn and wrap measures are the mockup's margins around its frame of
 *              hud_frame_width by hud_frame_height. On a tall surface (`cover`, set by the HUD) they stand by the
 *              same margins around the surface, the fog bands reach across it, and when the surface changes the
 *              particles are spread linearly from the area they filled onto the new one.
 *
 *              BRIDGE GAP N0449. The kind of weather and the wind are state of the weather simulation on
 *              sky.weather.v1. The veil always draws them in full from its frame: while the port files no
 *              record the bridge writes the mockup's weather from hud_bridge_weather_* of SHA and logs the gap once on its
 *              edge (godot_vivarium_hud_bridge.cpp, bridge_weather); the veil never asks whether a port
 *              delivered.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0449 (sky.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the weather is this frame. */
struct WeatherVeilFrame {
    const char* kind = nullptr;   // the weather key of the port or of the bridge (GODOT_HUD_WEATHER_*), N0449
    double wind = 0.0;            // the wind of the port
    double horizon_y = 0.0;       // the horizon in frame units, from the view
    double time = 0.0;            // seconds since the veil was bound
    double dt = 0.0;              // seconds since the last frame
    HudCover cover;               // the surface the veil reaches, set by the HUD
};

/** One falling particle of rain, drizzle or snow. */
struct WeatherMote {
    double x = 0.0;      // frame units
    double y = 0.0;      // frame units
    double share = 0.0;  // hud_weather_speed plus hud_weather_speed_span times a draw
    double phase = 0.0;  // a draw over one full turn, the snow's sway
};

/** The weather over the scene. */
class WeatherVeil {
public:
    /** Binds measures, colours and the word, and draws the particles from the seed; false with the missing name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Moves and draws the weather of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const WeatherVeilFrame& now,
              std::string& failed);

private:
    /** The next draw of the seed in [0, 1). */
    double next_draw();

    /** The fog bands from the horizon, across `surface`. */
    void draw_fog(UiPen& canvas, double horizon_y, const ::godot::Rect2& surface) const;

    /** Spreads the particles from the area they fill onto `surface`, when it differs. */
    void spread(const ::godot::Rect2& surface);

    /** Moves every particle by `fall` and `drift` per second, then wraps it by the margins around the surface. */
    void move(double fall, double drift, const WeatherVeilFrame& now, double sway);

    double width_ = 0.0;              // hud_frame_width
    double height_ = 0.0;             // hud_frame_height
    double fog_bands_ = 0.0;          // hud_weather_fog_bands
    double fog_y_ = 0.0;              // hud_weather_fog_y
    double fog_step_ = 0.0;           // hud_weather_fog_step
    double fog_half_ = 0.0;           // hud_weather_fog_half
    double particles_ = 0.0;          // hud_weather_particles
    double seed_ = 0.0;               // hud_weather_seed
    double speed_ = 0.0;              // hud_weather_speed
    double speed_span_ = 0.0;         // hud_weather_speed_span
    double spawn_x_ = 0.0;            // hud_weather_spawn_x
    double spawn_span_ = 0.0;         // hud_weather_spawn_span
    double spawn_y_ = 0.0;            // hud_weather_spawn_y
    double wrap_y_ = 0.0;             // hud_weather_wrap_y
    double wrap_x_ = 0.0;             // hud_weather_wrap_x
    double wrap_width_ = 0.0;         // hud_weather_wrap_width
    double wind_factor_ = 0.0;        // hud_weather_wind
    double rain_fall_ = 0.0;          // hud_weather_rain_fall
    double rain_drift_ = 0.0;         // hud_weather_rain_drift
    double rain_lean_ = 0.0;          // hud_weather_rain_lean
    double rain_len_ = 0.0;           // hud_weather_rain_len
    double rain_line_ = 0.0;          // hud_weather_rain_line
    double drizzle_fall_ = 0.0;       // hud_weather_drizzle_fall
    double drizzle_drift_ = 0.0;      // hud_weather_drizzle_drift
    double drizzle_w_ = 0.0;          // hud_weather_drizzle_w
    double drizzle_h_ = 0.0;          // hud_weather_drizzle_h
    double snow_fall_ = 0.0;          // hud_weather_snow_fall
    double snow_drift_ = 0.0;         // hud_weather_snow_drift
    double snow_sway_ = 0.0;          // hud_weather_snow_sway
    double snow_sway_rate_ = 0.0;     // hud_weather_snow_sway_rate
    double snow_radius_ = 0.0;        // hud_weather_snow_radius
    double snow_radius_span_ = 0.0;   // hud_weather_snow_radius_span
    ::godot::Color rain_, drizzle_, snow_, fog_, fog_clear_;
    std::vector<WeatherMote> motes_;    // the particles, drawn from the seed at bind
    ::godot::Rect2 spread_;             // the area the particles fill: the frame at bind, then the surface
    uint32_t key_ = 0;                  // the running key of the seed's draws
};

}  // namespace ase::adp::godot

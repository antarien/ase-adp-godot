/**
 * ASE GODOT ADAPTER - HUD SKY OVERCAST
 *
 * @file        godot_hud_sky_overcast.cpp
 * @brief       The grey of an overcast sky and the dimmed light under it, derived for the sky of the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, rows N01, N02 and N03, see godot_hud_sky_overcast.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0449 (flora.weather.v1), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_sky_overcast.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <cstring>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool SkyOvercast::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"sky_light_floor", &light_floor_},
             {"sky_light_base", &light_base_},
             {"sky_light_span", &light_span_},
             {"sky_light_offset", &light_offset_},
             {"sky_overcast_mix", &mix_},
             {"sky_overcast_share_floor", &share_floor_},
             {"sky_overcast_share_offset", &share_offset_},
             {"sky_overcast_share_span", &share_span_},
             {"sky_overcast_light", &light_},
             {"sky_overcast_stars", &stars_},
             {"sky_overcast_moon", &moon_},
             {"sky_twinkle_base", &twinkle_base_},
             {"sky_twinkle_swing", &twinkle_swing_},
             {"sky_twinkle_rate", &twinkle_rate_},
             {"sky_twinkle_rate_span", &twinkle_rate_span_},
             {"sky_twinkle_phase_step", &twinkle_phase_step_},
             {"sky_twinkle_steady", &twinkle_steady_},
             {"sky_twinkle_night_below", &twinkle_night_below_},
             {"sky_moon_opacity", &moon_opacity_},
             {"sky_moon_shadow_x", &moon_shadow_x_},
             {"sky_moon_shadow_y", &moon_shadow_y_},
             {"sky_moon_shadow_radius", &moon_shadow_radius_},
             {"sky_moon_shadow_cover", &moon_shadow_cover_},
             {"sky_clear_sun_halo", &sun_halo_clear_},
             {"sky_overcast_sun_halo", &sun_halo_overcast_},
             {"sky_sun_halo_inner", &sun_halo_inner_},
             {"sky_sun_halo_outer", &sun_halo_outer_},
             {"sky_clear_night_below", &clear_night_below_},
             {"sky_clear_dawn_below", &clear_dawn_below_},
             {"sky_clear_day_from", &clear_day_from_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"SKY_OVERCAST_DAY_ZENITH", &day_zenith_},
             {"SKY_OVERCAST_DAY_HORIZON", &day_horizon_},
             {"SKY_OVERCAST_NIGHT_ZENITH", &night_zenith_},
             {"SKY_OVERCAST_NIGHT_HORIZON", &night_horizon_},
             {"SKY_SUN_HALO", &sun_halo_},
             {"SKY_SUN_HALO_EDGE", &sun_halo_edge_},
             {"SKY_OVERCAST_SUN", &overcast_sun_},
             {"SKY_CLEAR_NIGHT_ZENITH", &clear_night_zenith_},
             {"SKY_CLEAR_NIGHT_HORIZON", &clear_night_horizon_},
             {"SKY_CLEAR_DAWN_ZENITH", &clear_dawn_zenith_},
             {"SKY_CLEAR_DAWN_HORIZON", &clear_dawn_horizon_},
             {"SKY_CLEAR_DAY_ZENITH", &clear_day_zenith_},
             {"SKY_CLEAR_DAY_HORIZON", &clear_day_horizon_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(share_span_ > 0.0) || !(mix_ >= 0.0) || !(light_ >= 0.0)) {
        failed = "sky_overcast_share_span must be positive, sky_overcast_mix and sky_overcast_light not negative";
        return false;
    }
    // an opacity: a share of the open sky's, from none to all of it
    if (!(stars_ >= 0.0 && stars_ <= 1.0) || !(moon_ >= 0.0 && moon_ <= 1.0)) {
        failed = "sky_overcast_stars and sky_overcast_moon must lie in 0..1";
        return false;
    }
    // the flicker swings an opacity factor: its lowest and highest point, the steady one and the moon's
    // opacity and cover all stay within none to all of the star or the moon
    const double twinkle_low = twinkle_base_ - twinkle_swing_;
    const double twinkle_high = twinkle_base_ + twinkle_swing_;
    if (!(twinkle_swing_ >= 0.0 && twinkle_low >= 0.0 && twinkle_high <= 1.0) ||
        !(twinkle_steady_ >= 0.0 && twinkle_steady_ <= 1.0) || !(moon_opacity_ >= 0.0 && moon_opacity_ <= 1.0) ||
        !(moon_shadow_cover_ >= 0.0 && moon_shadow_cover_ <= 1.0)) {
        failed = "sky_twinkle_base +- sky_twinkle_swing, sky_twinkle_steady, sky_moon_opacity and sky_moon_shadow_cover "
                 "must lie in 0..1";
        return false;
    }
    if (!(twinkle_rate_ >= 0.0) || !(twinkle_rate_span_ >= 0.0) || !(moon_shadow_radius_ >= 0.0)) {
        failed = "sky_twinkle_rate, sky_twinkle_rate_span and sky_moon_shadow_radius must not be negative";
        return false;
    }
    // the halo's opacity is a share of its colour's; its run starts at the inner radius and ends further out
    if (!(sun_halo_clear_ >= 0.0 && sun_halo_clear_ <= 1.0) || !(sun_halo_overcast_ >= 0.0 && sun_halo_overcast_ <= 1.0) ||
        !(sun_halo_inner_ >= 0.0) || !(sun_halo_outer_ > sun_halo_inner_)) {
        failed = "sky_clear_sun_halo and sky_overcast_sun_halo must lie in 0..1, sky_sun_halo_inner not be negative and "
                 "sky_sun_halo_outer exceed it";
        return false;
    }
    // the sky's own colours run night, dawn, day with the sun's height: each run needs a span to run over
    if (!(clear_night_below_ < clear_dawn_below_) || !(clear_dawn_below_ < clear_day_from_)) {
        failed = "sky_clear_night_below, sky_clear_dawn_below and sky_clear_day_from must rise in that order";
        return false;
    }
    return true;
}

void SkyOvercast::shade(const SkyOvercastFrame& now, SkyOvercastShade& out) const {
    out = SkyOvercastShade{};
    // the night sky of the mockup stands under every weather: the stars' flicker numbers and the moon's
    // crescent (row N02, m.html 414 to 416); the crescent is the mockup's while the sky port carries no phase
    out.twinkle_base = static_cast<float>(twinkle_base_);
    out.twinkle_swing = static_cast<float>(twinkle_swing_);
    out.twinkle_rate = static_cast<float>(twinkle_rate_);
    out.twinkle_rate_span = static_cast<float>(twinkle_rate_span_);
    out.twinkle_phase_step = static_cast<float>(twinkle_phase_step_);
    out.twinkle_steady = static_cast<float>(twinkle_steady_);
    out.moon_opacity = static_cast<float>(moon_opacity_);
    out.moon_shadow_x = static_cast<float>(moon_shadow_x_);
    out.moon_shadow_y = static_cast<float>(moon_shadow_y_);
    out.moon_shadow_radius = static_cast<float>(moon_shadow_radius_);
    out.moon_shadow_cover = static_cast<float>(moon_shadow_cover_);
    // the halo around the sun stands under every weather (row N03, m.html 417 to 418): an open sky's opacity
    // until the weather says overcast, its radii in mockup units at the frame's pixels per unit
    out.sun_halo = static_cast<float>(sun_halo_clear_);
    out.sun_halo_inner = static_cast<float>(sun_halo_inner_);
    out.sun_halo_outer = static_cast<float>(sun_halo_outer_);
    out.sun_halo_color = sun_halo_;
    out.sun_halo_edge = sun_halo_edge_;
    out.frame_pixels = static_cast<float>(now.frame_pixels);
    out.sun_veil = overcast_sun_;
    // the sky's own colours stand under every weather (row N01, m.html skyCols): the night's, run to the dawn's,
    // run to the day's by the sun's height, the two runs clamped so each holds its end outside its span. The fog
    // colour lies over both ends only as far as the fog reaches at the horizon: none under an open sky, grey in
    // a fog phase. The overcast grey follows in the sky shader by out.mix
    const float to_dawn = ase::math::clamp(
        static_cast<float>((now.sun_y - clear_night_below_) / (clear_dawn_below_ - clear_night_below_)), 0.0f, 1.0f);
    const float to_day = ase::math::clamp(
        static_cast<float>((now.sun_y - clear_dawn_below_) / (clear_day_from_ - clear_dawn_below_)), 0.0f, 1.0f);
    const float fogged = ase::math::clamp(static_cast<float>(now.fog_share), 0.0f, 1.0f);
    out.sky_own = true;
    out.sky_zenith =
        clear_night_zenith_.lerp(clear_dawn_zenith_, to_dawn).lerp(clear_day_zenith_, to_day).lerp(now.fog, fogged);
    out.sky_horizon =
        clear_night_horizon_.lerp(clear_dawn_horizon_, to_dawn).lerp(clear_day_horizon_, to_day).lerp(now.fog, fogged);
    if (now.kind == nullptr) {
        return;
    }
    // the mockup's flick: the stars flicker only on a clear night, isNight() = the sun below the threshold
    if (std::strcmp(now.kind, GODOT_HUD_WEATHER_CLEAR) == 0 && now.sun_y < twinkle_night_below_) {
        out.twinkle = 1.0f;
    }
    // The mockup's overcast(): rain, drizzle, snow, fog and a clouded sky.
    bool overcast = false;
    for (const char* kind : {GODOT_HUD_WEATHER_RAIN, GODOT_HUD_WEATHER_DRIZZLE, GODOT_HUD_WEATHER_SNOW,
                             GODOT_HUD_WEATHER_FOG, GODOT_HUD_WEATHER_CLOUDY}) {
        overcast = overcast || std::strcmp(now.kind, kind) == 0;
    }
    if (!overcast) {
        return;
    }
    // the mockup's skyCols: the grey runs from the night grey to the day grey with the sun's height
    const float day_share = ase::math::clamp(
        static_cast<float>((now.sun_y + share_offset_) / share_span_), static_cast<float>(share_floor_), 1.0f);
    out.mix = static_cast<float>(mix_);
    out.zenith = night_zenith_.lerp(day_zenith_, day_share);
    out.horizon = night_horizon_.lerp(day_horizon_, day_share);
    out.light = static_cast<float>(light_);
    // drawSky behind the clouds: the stars at a·sky_overcast_stars, the moon at 0.85·sky_overcast_moon
    out.stars = static_cast<float>(stars_);
    out.moon = static_cast<float>(moon_);
    // drawSky's sun behind the clouds: the halo at sky_overcast_sun_halo, the disc in SKY_OVERCAST_SUN (row N03)
    out.sun_halo = static_cast<float>(sun_halo_overcast_);
    out.veil = 1.0f;
}

double SkyOvercast::land_light(double sun_y, double overcast_light) const {
    return ase::math::max(light_floor_, ase::math::min(1.0, light_base_ + light_span_ * (sun_y + light_offset_))) *
           overcast_light;
}

}  // namespace ase::adp::godot

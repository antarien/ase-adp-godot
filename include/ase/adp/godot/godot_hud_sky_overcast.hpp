#pragma once

/**
 * ASE GODOT ADAPTER - HUD SKY OVERCAST
 *
 * @file        godot_hud_sky_overcast.hpp
 * @brief       The grey of an overcast sky and the dimmed light under it, derived for the sky of the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N01. The mockup's skyCols turns the sky of every
 *              time of day toward a grey while the weather is overcast (rain, drizzle, snow, fog, a clouded sky):
 *              the sky high up toward a zenith grey, the horizon toward a horizon grey, each by the share
 *              sky_overcast_mix; the grey itself runs from the night grey to the day grey with the sun's height,
 *              clamp((elevation + sky_overcast_share_offset) / sky_overcast_share_span, sky_overcast_share_floor, 1).
 *              Its light() dims the land under such a sky by sky_overcast_light, and drawSky lays the stars at
 *              sky_overcast_stars and the moon at sky_overcast_moon of their opacity behind the clouds (row N02,
 *              m.html 413 to 415): the shade carries both as factors on the opacity of the starfield
 *              (light_plant_stars) and of the moon discs (light_plant_sky). The block derives these numbers
 *              from SHA (design_tokens.json section VIVARIUM SKY OVERCAST, colors.json SKY_OVERCAST_*) and hands
 *              them as a shade; the scene lays the shade over the colours and the light ase-pl-sky computed
 *              (light_plant_scene, godot_plant_scene.hpp). Nothing of the sky port is changed here.
 *
 *              THE NIGHT SKY OF THE MOCKUP (row N02, m.html 414 to 416, design_tokens.json section VIVARIUM
 *              SKY NIGHT). On a clear night - the weather `clear` and the sun height below
 *              sky_twinkle_night_below, the mockup's isNight() - the shade sets `twinkle`, and the starfield
 *              lets every star flicker by sky_twinkle_base, sky_twinkle_swing, sky_twinkle_rate,
 *              sky_twinkle_rate_span and sky_twinkle_phase_step; on any other night a star stands at
 *              sky_twinkle_steady. The moon disc stands at sky_moon_opacity, covered by a shadow disc in the
 *              colour of the sky after sky_moon_shadow_*. BRIDGE GAP: the moon record of the sky port
 *              (SkyHostScneExptSystem, VivariumMoonView) carries no phase, though ase-ephemeris keeps one on
 *              the hub (EPH_MOO_PHASE, EPH_MOO_PHASE_PROGRESS); until the port carries it the shadow stands where
 *              the mockup draws it, the crescent of m.html 416.
 *
 *              THE SUN OF THE MOCKUP (row N03, m.html 417 to 419, design_tokens.json section VIVARIUM SKY
 *              OVERCAST). A halo stands around the sun while it is up: a radial run from SKY_SUN_HALO at
 *              sky_clear_sun_halo of its opacity at sky_sun_halo_inner mockup units to SKY_SUN_HALO_EDGE at
 *              sky_sun_halo_outer; under an overcast sky it stands at sky_overcast_sun_halo, and the disc takes
 *              SKY_OVERCAST_SUN, the sun seen through the clouds. The halo is measured in mockup units, so the
 *              shade carries the frame's pixels per unit with it (SkyOvercastFrame::frame_pixels).
 *
 *              THE SKY'S OWN COLOURS (row N01, m.html skyCols 404 to 408, design_tokens.json section VIVARIUM SKY
 *              OVERCAST, colors.json SKY_CLEAR_*). The mockup paints the sky of every weather from two colours by the
 *              sun's height: the night's below sky_clear_night_below, running to the dawn's up to
 *              sky_clear_dawn_below and on to the day's up to sky_clear_day_from, the day's above; the overcast grey
 *              lies over them by sky_overcast_mix. The shade carries them as sky_zenith and sky_horizon, and the
 *              sky shader shows them in place of the sky port's background and fog records. The fog record is the
 *              colour of the depth fog over the ground, never the colour of the horizon: it stands over both ends
 *              only by the share the fog reaches at the horizon (SkyOvercastFrame::fog_share), 0 under an open sky,
 *              growing in a fog phase as ase-pl-sky draws the fog in. So the sky turns grey in murky weather and
 *              in fog, and shows the mockup's colours under an open one.
 *
 *              BRIDGE GAP N0449. The kind of weather is state of the weather simulation on flora.weather.v1; the
 *              block reads it from its frame (VivariumHudFrame::sky), which carries a kind only while the sky
 *              port's colours carry no tint of it: the bridge writes the mockup's kind there from
 *              hud_bridge_weather_* of SHA (godot_vivarium_hud_bridge.cpp, bridge_weather), while the weather of
 *              flora.weather.v1 reaches the sky port through ase-pl-sky's weather tint and leaves the frame
 *              without a kind. So the sky is tinted exactly once (D2280). The block never asks whether a port
 *              delivered.
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

#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_plant_environment.hpp>  // SkyOvercastShade, the shade the scene lays over the sky port

#include <godot_cpp/variant/color.hpp>

#include <string>

namespace ase::adp::godot {

/** What the sky is this frame. */
struct SkyOvercastFrame {
    const char* kind = nullptr;   // the weather key (GODOT_HUD_WEATHER_*) the sky port carries no tint of: the bridge's, N0449; nullptr once the port delivers
    double sun_y = 0.0;           // upward share of the direction toward the sun, the sine of its elevation
    double frame_pixels = 0.0;    // screen pixels per mockup unit, the scale of the HUD's frame; set by the view
    ::godot::Color fog;           // the colour of the depth fog the sky port carries (its fog record)
    double fog_share = 0.0;       // share of that fog lying over the horizon, plant_fog_reach at the square's far edge
};

/** The grey of an overcast sky. */
class SkyOvercast {
public:
    /** Binds the measures and the colours; false with the missing name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Writes the shade of `now` into `out`; an open sky or no kind leaves the colours and the light untouched. */
    void shade(const SkyOvercastFrame& now, SkyOvercastShade& out) const;

    /**
     * The mockup's light() (m.html 411): the sun's height `sun_y` lifts the land's light from sky_light_floor to 1
     * by sky_light_base + sky_light_span * (sun_y + sky_light_offset), times `overcast_light` (the shade's light).
     * What the HUD draws on the ground in the light of the hour (GroundCrumbs) takes it from here.
     */
    [[nodiscard]] double land_light(double sun_y, double overcast_light) const;

private:
    double light_floor_ = 0.0;    // sky_light_floor
    double light_base_ = 0.0;     // sky_light_base
    double light_span_ = 0.0;     // sky_light_span
    double light_offset_ = 0.0;   // sky_light_offset
    double mix_ = 0.0;            // sky_overcast_mix
    double share_floor_ = 0.0;    // sky_overcast_share_floor
    double share_offset_ = 0.0;   // sky_overcast_share_offset
    double share_span_ = 0.0;     // sky_overcast_share_span
    double light_ = 0.0;          // sky_overcast_light
    double stars_ = 0.0;          // sky_overcast_stars
    double moon_ = 0.0;           // sky_overcast_moon
    double twinkle_base_ = 0.0;        // sky_twinkle_base
    double twinkle_swing_ = 0.0;       // sky_twinkle_swing
    double twinkle_rate_ = 0.0;        // sky_twinkle_rate
    double twinkle_rate_span_ = 0.0;   // sky_twinkle_rate_span
    double twinkle_phase_step_ = 0.0;  // sky_twinkle_phase_step
    double twinkle_steady_ = 0.0;      // sky_twinkle_steady
    double twinkle_night_below_ = 0.0; // sky_twinkle_night_below
    double moon_opacity_ = 0.0;        // sky_moon_opacity
    double moon_shadow_x_ = 0.0;       // sky_moon_shadow_x
    double moon_shadow_y_ = 0.0;       // sky_moon_shadow_y
    double moon_shadow_radius_ = 0.0;  // sky_moon_shadow_radius
    double moon_shadow_cover_ = 0.0;   // sky_moon_shadow_cover
    double sun_halo_clear_ = 0.0;      // sky_clear_sun_halo
    double sun_halo_overcast_ = 0.0;   // sky_overcast_sun_halo
    double sun_halo_inner_ = 0.0;      // sky_sun_halo_inner
    double sun_halo_outer_ = 0.0;      // sky_sun_halo_outer
    double clear_night_below_ = 0.0;   // sky_clear_night_below: sun height under which the sky is the night's
    double clear_dawn_below_ = 0.0;    // sky_clear_dawn_below: sun height at which the run to dawn has arrived
    double clear_day_from_ = 0.0;      // sky_clear_day_from: sun height from which the sky is the day's

    ::godot::Color clear_night_zenith_;   // SKY_CLEAR_NIGHT_ZENITH
    ::godot::Color clear_night_horizon_;  // SKY_CLEAR_NIGHT_HORIZON
    ::godot::Color clear_dawn_zenith_;    // SKY_CLEAR_DAWN_ZENITH
    ::godot::Color clear_dawn_horizon_;   // SKY_CLEAR_DAWN_HORIZON
    ::godot::Color clear_day_zenith_;     // SKY_CLEAR_DAY_ZENITH
    ::godot::Color clear_day_horizon_;    // SKY_CLEAR_DAY_HORIZON
    ::godot::Color day_zenith_;     // SKY_OVERCAST_DAY_ZENITH
    ::godot::Color day_horizon_;    // SKY_OVERCAST_DAY_HORIZON
    ::godot::Color night_zenith_;   // SKY_OVERCAST_NIGHT_ZENITH
    ::godot::Color night_horizon_;  // SKY_OVERCAST_NIGHT_HORIZON
    ::godot::Color sun_halo_;       // SKY_SUN_HALO
    ::godot::Color sun_halo_edge_;  // SKY_SUN_HALO_EDGE
    ::godot::Color overcast_sun_;   // SKY_OVERCAST_SUN
};

}  // namespace ase::adp::godot

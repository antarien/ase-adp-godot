#pragma once

/**
 * ASE GODOT ADAPTER - THE ENVIRONMENT OF THE CARE VIEW
 *
 * @file        godot_plant_environment.hpp
 * @brief       Sky, ambient light, fog and exposure of the care view's 3D scene
 * @description The light around the plant that no node carries: the sky behind the scene, the ambient light
 *              that reaches every surface, the depth fog over the ground and the camera's exposure. All of it
 *              comes from the sky port (VivariumSkyView) as ase-pl-sky computed it - the background, ambient
 *              and fog records and the lift of scene_exposure (godot_scene_exposure.hpp); nothing of it is
 *              computed here. The sun, the moon light and the discs stand in the scene itself
 *              (light_plant_scene, godot_plant_scene.hpp), which calls light_plant_environment once its
 *              lights are set.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_sky_view.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>

namespace ase::adp::godot {

// THE FOG VEILS THE GROUND, NEVER THE SKY: Godot lays its depth fog over the background with the factor
// fog_sky_affect, 1 by default, and the background stands at infinite depth - the whole sky turned the fog
// colour, and the sky colour ase-pl-sky computes (the background record) never reached the screen. It stands
// here and not in types.hpp, which nearly every unit of the adapter includes.
constexpr float GODOT_FOG_SKY_AFFECT = 0.0f;  // share of the depth fog laid over the sky background: none

// THE SKY IS A SKY, NEVER A BACKGROUND COLOUR (register B0184): with the depth fog on, Godot 4.5.2 draws a
// background of BG_COLOR through its fog-only sky pass, and on the Mobile renderer that pass leaves the colour
// linear without the sRGB encoding - the sky of the care view stood at 42/60/99 where the background record
// asks 101/120/151 (clients/avt-client-android/data/probes/godot_bg_colour_probe.py: BG_COLOR without fog
// 101/119/151, with fog 33/48/79,
// a Sky shader with fog 101/119/151). The background record therefore reaches the screen as a sky shader's
// colour: a source colour, decoded once by Godot, lifted by the exposure in linear light like every other
// light of the scene and encoded once by the tonemap, the same in an HDR frame as in an 8-bit one.
//
// THE SKY RUNS FROM THE HORIZON TO THE ZENITH (register B0187): the care view's sky is a gradient, never one
// flat colour - at the top edge of the frame the colour of the sky high up, at the horizon the light haze
// that stands low over the land (m.html drawSky: skyCols c[0] at the top, c[1] at the horizon). Both are
// records of the sky port, so ase-pl-sky computes them for every time of day (CAUSA_ASE_SKY, smootherstep
// zone interpolation of ambient, sun, fog and background, usecase:100700): the background record is the sky
// high up, the fog record the haze at the horizon (ATM_CONST_*_BG_* and ATM_CONST_*_FOG_*, hub_constants.json:
// at midday 0.45/0.55/0.70 above and 0.55/0.55/0.58 at the horizon). The gradient runs over the upward share of
// the view direction and reaches the background where the frame ends at the top - the upward share of the ray
// through the middle of the top edge, as the camera itself projects it (Camera3D::project_ray_normal) - so the
// horizon stays the horizon wherever the camera looks, and the frame shows the whole run as the mockup does.
// The mix happens in linear light, after Godot decoded both source colours once.
constexpr const char* GODOT_SKY_BACKGROUND = "background";  // the sky shader's uniform the background record lands in
constexpr const char* GODOT_SKY_HORIZON = "horizon";        // the sky shader's uniform the fog record's colour lands in
constexpr const char* GODOT_SKY_FRAME_TOP = "frame_top";    // the sky shader's uniform: upward share of the frame's top ray
constexpr float GODOT_SKY_ZENITH_SHARE = 1.0f;              // upward share of a view direction that points at the zenith
//
// AN OVERCAST SKY TURNS BOTH ENDS TOWARD A GREY (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N01, m.html skyCols): the sky
// high up toward a zenith grey, the horizon toward a horizon grey, by one share; under an open sky the share is 0
// and the shader shows the sky port's records untouched. The greys are source colours like the records and are
// mixed in linear light like them; the shade comes from SkyOvercast (godot_hud_sky_overcast.hpp).
constexpr const char* GODOT_SKY_OVERCAST_ZENITH = "overcast_zenith";    // the sky shader's uniform: the zenith grey
constexpr const char* GODOT_SKY_OVERCAST_HORIZON = "overcast_horizon";  // the sky shader's uniform: the horizon grey
constexpr const char* GODOT_SKY_OVERCAST_MIX = "overcast_mix";          // the sky shader's uniform: share toward the greys
// The sky shader is data (shaders/plant_sky.gdshader, loaded by godot_plant_shader.hpp): an edit compiles no unit.
constexpr const char* GODOT_SKY_BACKGROUND_SHADER_FILE = "res://shaders/plant_sky.gdshader";  // the sky behind the scene

/**
 * The shade the weather lays over the sky port's colours and light (SkyOvercast::shade): the greys and the
 * dimming of an overcast sky, and the look of the night sky the mockup draws (m.html drawSky, row N02) - the
 * flicker of the stars on a clear night and the crescent of the moon - and of the sun (row N03): the halo
 * around it and its disc behind the clouds. The defaults change nothing and draw no halo.
 */
struct SkyOvercastShade {
    float mix = 0.0f;             // share the sky colours turn toward the greys: 0 under an open sky
    ::godot::Color zenith;        // the grey the sky high up turns toward
    ::godot::Color horizon;       // the grey the horizon turns toward
    float light = 1.0f;           // factor on the sun and the ambient light: 1 under an open sky
    float stars = 1.0f;           // factor on the stars' opacity: 1 under an open sky (m.html drawSky, row N02)
    float moon = 1.0f;            // factor on the moon discs' opacity: 1 under an open sky (m.html drawSky, row N02)
    float twinkle = 0.0f;         // 1 on a clear night, when the stars flicker (m.html drawSky flick); 0 otherwise
    float twinkle_base = 1.0f;    // sky_twinkle_base: the middle of a flickering star's opacity factor
    float twinkle_swing = 0.0f;   // sky_twinkle_swing: how far the factor swings around that middle
    float twinkle_rate = 0.0f;    // sky_twinkle_rate: radians per second of the swing, for a star of no size
    float twinkle_rate_span = 0.0f;   // sky_twinkle_rate_span: radians per second the swing gains per pixel of size
    float twinkle_phase_step = 0.0f;  // sky_twinkle_phase_step: radians the swing of a star lags the one before it
    float twinkle_steady = 1.0f;  // sky_twinkle_steady: the opacity factor of a star that does not flicker
    float moon_opacity = 1.0f;    // sky_moon_opacity: the opacity of a moon disc under an open sky
    float moon_shadow_x = 0.0f;   // sky_moon_shadow_x: middle of the shadow disc right of the moon's, in moon radii
    float moon_shadow_y = 0.0f;   // sky_moon_shadow_y: middle of the shadow disc above the moon's, in moon radii
    float moon_shadow_radius = 0.0f;  // sky_moon_shadow_radius: radius of the shadow disc, in moon radii
    float moon_shadow_cover = 0.0f;   // sky_moon_shadow_cover: share of the moon the shadow disc covers with the sky
    float sun_halo = 0.0f;            // sky_clear_sun_halo, under an overcast sky sky_overcast_sun_halo: the halo's opacity at its inner edge; 0 draws none
    float sun_halo_inner = 0.0f;      // sky_sun_halo_inner: radius in mockup units up to which the halo stands in its full colour
    float sun_halo_outer = 0.0f;      // sky_sun_halo_outer: radius in mockup units at which the halo has faded to its edge colour
    ::godot::Color sun_halo_color;    // SKY_SUN_HALO: the halo's colour at its inner edge, its alpha times sun_halo
    ::godot::Color sun_halo_edge;     // SKY_SUN_HALO_EDGE: the halo's colour at its outer edge
    float frame_pixels = 0.0f;        // screen pixels per mockup unit, the scale of the HUD's frame; 0 before the HUD stands: no halo
    float veil = 0.0f;                // share the sun disc turns toward sun_veil: 1 under an overcast sky, 0 under an open one
    ::godot::Color sun_veil;          // SKY_OVERCAST_SUN: the sun disc seen through the clouds (m.html drawSky 419)
    bool sky_own = false;             // true once the HUD stands: the sky shows sky_zenith and sky_horizon, not the port's records
    ::godot::Color sky_zenith;        // the sky high up by the sun's height (m.html skyCols c[0]), veiled by the fog's reach
    ::godot::Color sky_horizon;       // the horizon by the sun's height (m.html skyCols c[1]), veiled by the fog's reach
};

/**
 * The share of the depth fog that lies over `at` as the camera of `scene` (GODOT_PLANT_CAMERA) sees it: 0 nearer
 * than the fog record's near distance, 1 beyond its far distance, linear between, as the depth fog itself runs from
 * fog_near to fog_far (light_plant_environment). Under an open sky the fog begins beyond the square and the share at
 * its far edge is 0; a fog phase draws fog_far in (CAUSA_ASE_SKY, Fog Density Modulation) and the share grows. A
 * scene without its camera or an unknown sky port gives 0.
 */
float plant_fog_reach(const ::godot::Node3D* scene, const VivariumSkyView& sky, const ::godot::Vector3& at);

/**
 * Hangs the WorldEnvironment of the scene (GODOT_PLANT_ENVIRONMENT) under `scene`: a Sky whose shader is
 * GODOT_SKY_BACKGROUND_SHADER_FILE as the background, the ambient light as a colour, reflections off, and
 * camera attributes with the automatic exposure off. The sky shows black until light_plant_environment
 * hands it the first background record. build_plant_scene calls it once.
 */
void build_plant_environment(::godot::Node3D* scene);

/**
 * Sets the environment of `scene` from the sky port: the background record into the sky shader
 * (GODOT_SKY_BACKGROUND), the fog colour as its horizon (GODOT_SKY_HORIZON) and the upward share of the
 * camera's ray through the middle of the frame's top edge as the height the gradient reaches the background
 * at (GODOT_SKY_FRAME_TOP; a frame whose top edge looks below the horizon holds no sky, and the gradient then
 * spans the whole dome up to the zenith), the ambient colour at the ambient intensity times the lift, the depth fog with its
 * colour, its near and far distance and the lift as its light energy, laid over the ground only
 * (GODOT_FOG_SKY_AFFECT), the shimmer of the fog record (veil_plant_scene) and the lift as the camera's
 * exposure multiplier. Once `overcast` carries its own sky (sky_own), its zenith and horizon take the place of the
 * background and fog records in the sky shader: the fog colour is the colour of the depth fog over the ground and
 * stands at the horizon only as far as the fog reaches there, so the sky is grey only in a fog phase. `overcast`
 * turns both ends of the sky toward its greys (GODOT_SKY_OVERCAST_*) and multiplies the ambient light by its light
 * factor. A scene without its environment is left as it is; an
 * unknown sky port sets nothing.
 */
void light_plant_environment(::godot::Node3D* scene, const VivariumSkyView& sky, const SkyOvercastShade& overcast);

}  // namespace ase::adp::godot

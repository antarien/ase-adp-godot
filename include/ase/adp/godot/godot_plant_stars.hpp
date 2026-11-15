#pragma once

/**
 * ASE GODOT ADAPTER - THE STARFIELD IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_stars.hpp
 * @brief       The starfield of the care view's 3D scene: the visible stars of ase-pl-sky, turned with the sky
 * @description The stars port of ase-pl-sky (sky.stars.v1) carries the visible star set at its places on the
 *              celestial sphere, filed when the plugin computes the set anew; the turn record of the sky port
 *              carries the quaternion that turns that sphere into the world - the bearing of the scene, the
 *              latitude of the place and the sidereal angle of the clock (SkyCelStarWrldSystem). The view
 *              lays one instance per star once per set and turns the whole field every frame, so the stars
 *              wander across the dome with the clock and in the time lapse. Nothing of the sky is computed
 *              here: the places, the colours, the sizes and the turn are read as the ports carried them.
 *
 *              BEHIND THE CLOUDS THE STARS FADE, THEY NEVER DARKEN (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N02,
 *              m.html drawSky): under an overcast sky the mockup draws every star at sky_overcast_stars of its
 *              opacity, so the grey of the sky shows through it. The field takes that factor from the shade
 *              of SkyOvercast (SkyOvercastShade::stars) as the alpha of each instance in a material that
 *              blends by alpha; a star darkened instead would stand as a black dot on the grey.
 *
 *              ON A CLEAR NIGHT THE STARS FLICKER (row N02, m.html 415): the material is GODOT_STAR_SHADER_FILE,
 *              whose alpha factor swings by the flicker numbers of the shade (SkyOvercastShade::twinkle*) while
 *              `twinkle` stands, and holds sky_twinkle_steady otherwise. Each instance carries its size in
 *              pixels and its place in the set as custom data, laid with the set.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-08
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_plant_environment.hpp>  // SkyOvercastShade, the factor on the stars' opacity

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

// The opacity the instances were laid with: a change of the weather lays them anew like a new set. It stands
// here and not in types.hpp, which nearly every unit of the adapter includes.
constexpr const char* GODOT_STAR_META_OPACITY = "ase_star_opacity";  // meta of the starfield node: SkyOvercastShade::stars the instances' alpha was laid with

// The look of every star (shaders/plant_stars.gdshader, loaded by godot_plant_shader.hpp) and its uniforms: the
// flicker of a clear night (SkyOvercastShade::twinkle*). The look is data: an edit of the file compiles no unit.
constexpr const char* GODOT_STAR_SHADER_FILE = "res://shaders/plant_stars.gdshader";  // the look of every star
constexpr const char* GODOT_STAR_TWINKLE = "twinkle";                  // uniform: 1 while the stars flicker, 0 while they stand steady
constexpr const char* GODOT_STAR_TWINKLE_BASE = "twinkle_base";        // uniform: middle of the flicker's opacity factor
constexpr const char* GODOT_STAR_TWINKLE_SWING = "twinkle_swing";      // uniform: swing of the factor around its middle
constexpr const char* GODOT_STAR_TWINKLE_RATE = "twinkle_rate";        // uniform: radians per second for a star of no size
constexpr const char* GODOT_STAR_TWINKLE_RATE_SPAN = "twinkle_rate_span";    // uniform: radians per second gained per pixel of size
constexpr const char* GODOT_STAR_TWINKLE_PHASE_STEP = "twinkle_phase_step";  // uniform: radians a star lags the one before it
constexpr const char* GODOT_STAR_TWINKLE_STEADY = "twinkle_steady";    // uniform: factor of a star that does not flicker

/**
 * Adds the starfield (GODOT_PLANT_STARS) under the scene root: a multimesh of GODOT_SKY_STARS_MAX small
 * spheres of unit diameter in the material of GODOT_STAR_SHADER_FILE, which takes its colour and its alpha from
 * each instance and blends by that alpha, casting no shadow. No instance shows until light_plant_stars lays a
 * set. build_plant_sky calls it once.
 */
void build_plant_stars(::godot::Node3D* scene);

/**
 * Stands the starfield on `centre`, the middle of the sky dome (light_plant_sky: the camera lowered so the
 * dome's horizon ring lies on the ray to the ground's visible far edge), every frame, and turns it by the turn
 * record of the sky; lays its instances anew when the held star set (star_generation), the size of a pixel at
 * the disc distance or the opacity factor of `overcast` (SkyOvercastShade::stars, the alpha of every instance)
 * changed, and hands the material the flicker numbers of `overcast` every frame. Without the turn record no
 * star shows. light_plant_sky calls it every frame.
 */
void light_plant_stars(::godot::Node3D* scene, const VivariumSkyView& sky, const SkyOvercastShade& overcast,
                       const ::godot::Vector3& centre);

}  // namespace ase::adp::godot

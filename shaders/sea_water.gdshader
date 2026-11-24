/**
 * ASE GODOT ADAPTER - SEA WATER
 *
 * @file        sea_water.gdshader
 * @brief       Open salt water at NN0: depth tinted, with swell and chop on the surface and the sky it reflects
 * @description The fourth of the five water shaders of PLAN_ASE_VIVARIUM_PHASE_10_HYDRO (section "Geometrie-Schicht:
 *              Wasser im Bild"). One plane per salt chunk of water.sea.v1, laid at NN0, the zero of the world height of
 *              the engine (water_kinds_sea_plane, godot_plant_water_kinds_geom.hpp); the ground above NN0 hides the
 *              plane by the depth test, which draws the coast. The vertex colour red carries the depth of the lowest
 *              ground of the chunk under NN0 over its full scale, the other channels are not read. The view computes
 *              no water: a chunk without its values lays no plane. The waves are the normal of the surface alone:
 *              a long swell and two crossing choppy trains, moving with TIME along the world position, so neighbouring
 *              chunks continue each other's waves without a seam; the plane stays flat. The colours are the shelf's
 *              blue green and the deep sea's navy; the sky lies over them as unlit light, the globals the river
 *              reads (light_plant_brook_reflection, row N07), and a sea reflects it more cleanly than a brook.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode cull_back;
uniform vec4 shallow_color : source_color = vec4(0.10, 0.40, 0.46, 1.0);
uniform vec4 deep_color : source_color = vec4(0.02, 0.10, 0.22, 1.0);
// the exposure lift of the frame (light_plant_environment): an unlit colour is divided by it to stand as drawn
global uniform float scene_lift;
// the rows of the ground's depth (light_plant_ground_season, row N06), the same rows the river reads
global uniform vec3 ground_rows;
// the sky the water reflects (light_plant_brook_reflection, row N07)
global uniform vec4 brook_far_color;
global uniform vec4 brook_near_color;
global uniform float brook_reflect_mix;
varying float deep;
varying vec2 world_xz;
void vertex() {
    deep = COLOR.r;
    world_xz = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xz;
}
// the slope of one wave train of the given direction, wave number and speed at the world place
vec2 train(vec2 at, vec2 dir, float wave_number, float speed) {
    float phase = dot(at, dir) * wave_number + TIME * speed;
    return dir * cos(phase) * wave_number;
}
void fragment() {
    float reflect_share = clamp(brook_reflect_mix, 0.0, 1.0);
    float row = clamp((FRAGCOORD.y / max(VIEWPORT_SIZE.y, 1.0) - ground_rows.x) / max(ground_rows.y - ground_rows.x, 0.00001), 0.0, 1.0);
    float depth = pow(row, 1.0 / max(ground_rows.z, 0.00001));
    vec3 sky = mix(brook_far_color.rgb, brook_near_color.rgb, depth);
    float full = clamp(deep, 0.0, 1.0);
    vec2 slope = train(world_xz, normalize(vec2(1.0, 0.35)), 0.18, 0.9) * 0.10;
    slope += train(world_xz, normalize(vec2(-0.4, 1.0)), 0.55, 1.7) * 0.035;
    slope += train(world_xz, normalize(vec2(0.8, -0.6)), 1.3, 2.6) * 0.012;
    NORMAL = normalize(NORMAL + vec3(slope.x, 0.0, slope.y));
    ALBEDO = mix(shallow_color.rgb, deep_color.rgb, full);
    EMISSION = sky * reflect_share / max(scene_lift, 1.0);
    ALPHA = mix(0.80, 0.98, full);
    ROUGHNESS = 0.07;
    SPECULAR = 0.65;
}

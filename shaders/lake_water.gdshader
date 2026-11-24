/**
 * ASE GODOT ADAPTER - LAKE WATER
 *
 * @file        lake_water.gdshader
 * @brief       The still water of a lake or pond: depth tinted, with the sky it reflects and a faint breath on the surface
 * @description The third of the five water shaders of PLAN_ASE_VIVARIUM_PHASE_10_HYDRO (section "Geometrie-Schicht:
 *              Wasser im Bild"). A lake does not flow: no current, no strokes. Its surface is one quad at the top
 *              face of the box ase-pl-water lays on water.lake.v1 (SPT_AABB_*), laid by water_kinds_lake_surface
 *              (godot_plant_water_kinds_geom.hpp); the vertex colour red carries the depth of the water over
 *              lake_depth_m, the other channels are not read. The view computes no water: the depth is the one
 *              ase-hydro holds, and a lake without it draws no quad. The two colours are the lake's, the green blue of
 *              the shallows at the shore and the dark of the middle; over them lies the sky the still surface
 *              reflects, the globals the river and the pit read (light_plant_brook_reflection, row N07), added as
 *              unlit light. The breath is a slow, small tilt of the normal along the world position, so the surface
 *              lives without moving the water anywhere.
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
uniform vec4 shallow_color : source_color = vec4(0.20, 0.34, 0.30, 1.0);
uniform vec4 deep_color : source_color = vec4(0.04, 0.14, 0.17, 1.0);
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
void fragment() {
    float reflect_share = clamp(brook_reflect_mix, 0.0, 1.0);
    float row = clamp((FRAGCOORD.y / max(VIEWPORT_SIZE.y, 1.0) - ground_rows.x) / max(ground_rows.y - ground_rows.x, 0.00001), 0.0, 1.0);
    float depth = pow(row, 1.0 / max(ground_rows.z, 0.00001));
    vec3 sky = mix(brook_far_color.rgb, brook_near_color.rgb, depth);
    float full = clamp(deep, 0.0, 1.0);
    float breath = sin(world_xz.x * 0.9 + TIME * 0.35) * cos(world_xz.y * 0.8 - TIME * 0.28);
    NORMAL = normalize(NORMAL + vec3(breath, 0.0, breath * 0.6) * 0.012);
    ALBEDO = mix(shallow_color.rgb, deep_color.rgb, full);
    EMISSION = sky * reflect_share / max(scene_lift, 1.0);
    ALPHA = mix(0.84, 0.97, full);
    ROUGHNESS = 0.05;
    SPECULAR = 0.6;
}

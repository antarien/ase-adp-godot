/**
 * ASE GODOT ADAPTER - GLACIER ICE
 *
 * @file        glacier_water.gdshader
 * @brief       Glacier ice over the ground of a chunk, with the sheen of the meltwater running on it
 * @description The fifth of the five water shaders of PLAN_ASE_VIVARIUM_PHASE_10_HYDRO (section "Geometrie-Schicht:
 *              Wasser im Bild"). One chunk of water.glacier.v1 lays the triangles of the ground it covers, each vertex
 *              lifted by the ice thickness the record gives (water_kinds_glacier_cap, godot_plant_water_kinds_geom.hpp).
 *              The vertex colour red carries that thickness over its full scale, the green the melt over its full scale
 *              (glcr_melt_mm_h); the blue channel is not read. Thin ice is translucent and bluish, thick ice white and
 *              nearly opaque; where it melts, a wet sheen takes the surface: the roughness falls, the specular rises
 *              and the colour of the meltwater flushes in along a slow vein pattern of the world position. The view
 *              computes no ice and no melt, and a chunk without a value lays no ice.
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
uniform vec4 ice_color : source_color = vec4(0.78, 0.90, 0.96, 1.0);
uniform vec4 melt_color : source_color = vec4(0.30, 0.55, 0.70, 1.0);
// the exposure lift of the frame (light_plant_environment): an unlit colour is divided by it to stand as drawn
global uniform float scene_lift;
// the sky the meltwater reflects (light_plant_brook_reflection, row N07)
global uniform vec4 brook_far_color;
global uniform float brook_reflect_mix;
varying float thick;
varying float melt;
varying vec2 world_xz;
void vertex() {
    thick = COLOR.r;
    melt = COLOR.g;
    world_xz = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xz;
}
void fragment() {
    float ice = clamp(thick, 0.0, 1.0);
    float wet = clamp(melt, 0.0, 1.0);
    float vein = 0.5 + 0.5 * sin(world_xz.x * 0.7 + sin(world_xz.y * 0.45) * 2.0 + TIME * 0.12);
    float sheen = wet * (0.55 + 0.45 * vein);
    vec3 body = mix(ice_color.rgb * vec3(0.82, 0.92, 1.0), ice_color.rgb, ice);
    ALBEDO = mix(body, melt_color.rgb, sheen * 0.6);
    EMISSION = brook_far_color.rgb * clamp(brook_reflect_mix, 0.0, 1.0) * sheen * 0.5 / max(scene_lift, 1.0);
    ALPHA = mix(0.55, 0.97, ice);
    ROUGHNESS = mix(0.45, 0.06, sheen);
    SPECULAR = mix(0.35, 0.7, sheen);
}

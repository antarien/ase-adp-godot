/**
 * ASE GODOT ADAPTER - PIT WATER
 *
 * @file        pit_water.gdshader
 * @brief       The groundwater standing in a dug pit: still, turbid from the loam, with the sky it mirrors
 * @description The first of the five water shaders of PLAN_ASE_VIVARIUM_PHASE_10_HYDRO (section "Geometrie-Schicht:
 *              Wasser im Bild", item 2; H.13), register D2508. The water of a pit is groundwater that seeped in
 *              through the walls (HydroCellPitCalcSystem, answered by ase-gis as GIS_HYD_CELL_PIT_DEPTH, filed on
 *              hydrogeology.pit.v1): it does not flow, so this shader carries no current, no ripples moving along
 *              a course and no strokes, which the river shader plant_water.gdshader draws. The table that feeds it
 *              is laid by build_water_table (godot_plant_water.hpp) into its own arrays, only the cells a pit and
 *              no bed floods; the vertex colour green carries the depth of the water over the ground vertex over
 *              GODOT_WATER_DEPTH_FULL_M, the other channels are not read. The view computes no water (I-8): the
 *              depth is the one ase-hydro holds in the pit, and without a record the table lays no cell here.
 *              The two colours are those of fresh pit water, loam brown where it is shallow and dark where the
 *              wall's shade and the depth swallow the light; over them lies the sky the still surface mirrors, the
 *              same globals the river reads (light_plant_brook_reflection, row N07), added on top as unlit light
 *              and never taken from the own colour. A still surface mirrors more cleanly than a running one, so
 *              its roughness stays below the river's.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode cull_back;
uniform vec4 shallow_color : source_color = vec4(0.36, 0.31, 0.22, 1.0);
uniform vec4 deep_color : source_color = vec4(0.12, 0.13, 0.10, 1.0);
// the exposure lift of the frame (light_plant_environment): an unlit colour is divided by it to stand as drawn
global uniform float scene_lift;
// the rows of the ground's depth (light_plant_ground_season, row N06), the same rows the river reads
global uniform vec3 ground_rows;
// the sky the water mirrors (light_plant_brook_reflection, row N07)
global uniform vec4 brook_far_color;
global uniform vec4 brook_near_color;
global uniform float brook_reflect_mix;
varying float deep;
void vertex() {
    deep = COLOR.g;
}
void fragment() {
    float reflect_share = clamp(brook_reflect_mix, 0.0, 1.0);
    float row = clamp((FRAGCOORD.y / max(VIEWPORT_SIZE.y, 1.0) - ground_rows.x) / max(ground_rows.y - ground_rows.x, 0.00001), 0.0, 1.0);
    float depth = pow(row, 1.0 / max(ground_rows.z, 0.00001));
    vec3 sky = mix(brook_far_color.rgb, brook_near_color.rgb, depth);
    float full = clamp(deep, 0.0, 1.0);
    ALBEDO = mix(shallow_color.rgb, deep_color.rgb, full);
    EMISSION = sky * reflect_share / max(scene_lift, 1.0);
    ALPHA = mix(0.88, 0.97, full);
    ROUGHNESS = 0.08;
    SPECULAR = 0.55;
}

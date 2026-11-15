/**
 * ASE GODOT ADAPTER - PLANT GROUND
 *
 * @file        plant_ground.gdshader
 * @brief       The ground with its cover, bare soil, banks, brook beds, grid and contours
 * @description The ground of the care view (usecase:100387 view the plant): a terrain takes other colours by its slope,
 *              and its grid lines show its shape. One ShaderMaterial per soil
 *              over the whole terrain mesh, its cover the material of the soil's role (godot_plant_look.cpp):
 *              - THE SLOPE PAINTS: the world normal of a fragment gives its slope; above GODOT_GROUND_SLOPE_BANK_DEG the
 *                cover gives way to bare earth, above GODOT_GROUND_SLOPE_STEEP_DEG to the colour of an open bank.
 *              - THE GROUND STATE PAINTS: the vertex colour mark_ground lays (godot_plant_geometry.hpp) - red where
 *                ase-terrain levelled the disc of a patch, which turns to bare soil, green where a brook's bed runs,
 *                which turns to wet gravel. A Nivelar the player taps changes the terrain port and with it the colour.
 *              - THE GRID SHOWS THE SHAPE: thin lines along x and z at the distance of two height samples (the edges of
 *                the terrain cells), a stronger one every GODOT_GROUND_GRID_MAJOR cells, and contour lines every
 *                GODOT_GROUND_CONTOUR_M of height, so slopes, the levelled disc and the banks of the bed read as relief.
 *              The grain of the cover is value noise over the world place, so a meadow is no flat colour before Phase 04
 *              delivers its texture; the soil water of the focused patch darkens the ground as the stand-in did.
 *              - THE GROUND IS DITHERED BEFORE IT IS STORED: the Mobile frame is A2B10G10R10 with a luminance multiplier
 *                of 2, and its debanding dithers only the tonemap's output, after the 3D frame was rounded. At night one
 *                step of that frame is about two steps of the screen, so the grain and the moonlit relief stood as flat
 *                patches with sharp rims, red and green rounded at different places (register B0207, measured in
 *                acceptance/20261111-202503/frame_nox_1.png). The emission adds the interleaved gradient noise of Godot's
 *                own tonemap, half a frame step either way; the exposure multiplier lifts an emission as it lifts the
 *                lights, so the shader divides by the lift light_plant_environment sets as GODOT_SHADER_GLOBAL_LIFT.
 *              - THE SEASON PAINTS (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N06): the global ground_* parameters
 *                light_plant_ground_season sets (godot_plant_ground_season.hpp) give the cover the season's colour,
 *                darken the rows toward the horizon and blend the horizon's colour in there, unlit and divided by the
 *                same lift, as the mockup's groundCol mixes its lit row toward the sky. The rows are rows of the screen,
 *                as the mockup's rowY counts them: the horizon is the row of the square's far edge, the foot that of
 *                the HUD frame, so the grey of the horizon lies only on the far rows and not over the whole ground.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_018
 * @category   render/pipeline/stage
 * @usecase    usecase:100387 - view the plant
 * @actor      Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.01.00002
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode cull_back;
uniform vec4 cover_color : source_color = vec4(1.0);
uniform sampler2D cover_albedo : source_color, filter_linear_mipmap, repeat_enable, hint_default_white;
uniform vec2 cover_uv = vec2(1.0);
uniform vec4 soil_color : source_color = vec4(1.0);
uniform vec4 bank_color : source_color = vec4(1.0);
uniform vec4 bed_color : source_color = vec4(1.0);
uniform vec4 grid_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);
uniform float slope_bank = 0.0;
uniform float slope_steep = 0.0;
uniform float grid_step = 1.0;
uniform float grid_major = 1.0;
uniform float contour_step = 1.0;
uniform float grid_weight = 0.0;
uniform float major_weight = 0.0;
uniform float contour_weight = 0.0;
uniform float wet_shade = 1.0;
uniform float ground_roughness = 1.0;
uniform float frame_step = 0.0;
global uniform float scene_lift;
// THE SEASON PAINTS (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N06, m.html groundCol): light_plant_ground_season sets these
// every frame from GroundSeason's shade. The season's colour takes the cover by ground_season_mix; z is a row of the
// screen as the mockup's rowY counts it (m.html 358): 0 on the row the camera sees the square's far edge on
// (ground_rows.x), 1 on the HUD frame's foot (ground_rows.y), both shares of the screen's height as FRAGCOORD.y over
// VIEWPORT_SIZE.y is, z = share ^ (1 / ground_rows.z); the row is darkened to
// ground_depth.x + ground_depth.y * z before the lights fall on it, and the horizon's colour is blended in unlit by
// ground_horizon_share * (1 - z), as the mockup mixes its lit row toward the sky. The defaults change nothing.
global uniform vec4 ground_season_color;
global uniform float ground_season_mix;
global uniform vec2 ground_depth;
global uniform vec4 ground_horizon_color;
global uniform float ground_horizon_share;
global uniform vec3 ground_rows;
// The snow lies on every row of the mockup (m.html groundCol: c = mix(c, snow, share) for the whole ground): the cover
// takes it in ground_season_color, the bare soil and the banks take ground_snow.rgb by ground_snow.a; the bed of the
// brook stays under its water. Alpha 0 when it does not snow.
global uniform vec4 ground_snow;
instance uniform float soil_rel = -1.0;
varying vec3 world_pos;
varying vec3 world_normal;
float grain_hash(vec2 p) { vec3 q = fract(vec3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return fract((q.x + q.y) * q.z); }
float grain_at(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(grain_hash(i), grain_hash(i + vec2(1.0, 0.0)), u.x),
               mix(grain_hash(i + vec2(0.0, 1.0)), grain_hash(i + vec2(1.0, 1.0)), u.x), u.y);
}
float line_at(float c, float width) {
    float d = abs(fract(c - 0.5) - 0.5) / max(width, 0.00001);
    return 1.0 - clamp(d, 0.0, 1.0);
}
void vertex() {
    world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
    world_normal = (MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz;
}
void fragment() {
    float slope = 1.0 - clamp(normalize(world_normal).y, 0.0, 1.0);
    float grain = grain_at(world_pos.xz * 1.3) * 0.6 + grain_at(world_pos.xz * 6.1) * 0.4;
    vec3 cover_own = cover_color.rgb * texture(cover_albedo, UV * cover_uv).rgb;
    vec3 cover = mix(cover_own, ground_season_color.rgb, ground_season_mix) * mix(0.78, 1.18, grain);
    vec3 soil = mix(soil_color.rgb, ground_snow.rgb, ground_snow.a) * mix(0.85, 1.12, grain_at(world_pos.xz * 4.7));
    vec3 bank = mix(bank_color.rgb, ground_snow.rgb, ground_snow.a) * mix(0.85, 1.12, grain);
    vec3 bed = bed_color.rgb * mix(0.8, 1.1, grain_at(world_pos.xz * 9.0));
    vec3 color = mix(cover, soil, smoothstep(slope_bank * 0.5, slope_bank, slope));
    color = mix(color, bank, smoothstep(slope_steep * 0.7, slope_steep, slope));
    color = mix(color, soil, COLOR.r);
    color = mix(color, bed, COLOR.g);
    if (soil_rel >= 0.0) {
        color *= mix(1.0, wet_shade, clamp(soil_rel, 0.0, 1.0));
    }
    vec2 cell = world_pos.xz / grid_step;
    vec2 block = world_pos.xz / (grid_step * grid_major);
    float level = world_pos.y / contour_step;
    float minor = max(line_at(cell.x, fwidth(cell.x)), line_at(cell.y, fwidth(cell.y)));
    float major = max(line_at(block.x, fwidth(block.x)), line_at(block.y, fwidth(block.y)));
    float contour = line_at(level, fwidth(level));
    float lines = max(max(minor * grid_weight, major * major_weight), contour * contour_weight);
    float row = clamp((FRAGCOORD.y / max(VIEWPORT_SIZE.y, 1.0) - ground_rows.x) / max(ground_rows.y - ground_rows.x, 0.00001), 0.0, 1.0);
    float depth = pow(row, 1.0 / max(ground_rows.z, 0.00001));
    float horizon = clamp(ground_horizon_share * (1.0 - depth), 0.0, 1.0);
    ALBEDO = mix(color, grid_color.rgb, lines) * (ground_depth.x + ground_depth.y * depth) * (1.0 - horizon);
    ROUGHNESS = ground_roughness;
    vec3 dither = fract(vec3(dot(vec2(171.0, 231.0), FRAGCOORD.xy)) / vec3(103.0, 71.0, 97.0)) - 0.5;
    EMISSION = (dither * frame_step + ground_horizon_color.rgb * horizon) / max(scene_lift, 1.0);
}

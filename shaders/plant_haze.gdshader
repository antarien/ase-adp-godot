/**
 * ASE GODOT ADAPTER - PLANT HAZE
 *
 * @file        plant_haze.gdshader
 * @brief       The heat shimmer over the far ground
 * @description The shimmer of hot air (usecase:101515 - veil sight with weather, DSGN_130 130/FOGWR: from 313 K the rising
 *              warm air breaks the light unevenly and the horizon begins to waver, full at the heat damage limit of
 *              323 K). ase-atmosphere computes its strength from the temperature (ATM_VIS_HEAT_HAZE), ase-pl-sky hands it
 *              on in the fog record; the view computes none of it. A quad in front of the camera reads the drawn image and
 *              shifts its rows sideways in a wave that runs up the screen. The shift grows with the distance of what the
 *              fragment shows, from where the fog of the same record begins to where it is full, so the near ground
 *              stands still and the horizon wavers; at a shimmer of 0 the quad is hidden. The wind drift along the
 *              camera's right (drift, the wind share folded in, from the wind record of ase-pl-sky) tilts the wave
 *              fronts across the screen, so the rising rows lean and run downwind; at a drift of 0 they rise straight.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_130
 * @category   render/pipeline/stage
 * @usecase    usecase:101515 - veil sight with weather
 * @actor      Weather Veil Controller (WEATVEICON)
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.01.00001
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_never, depth_test_disabled, fog_disabled, shadows_disabled;
uniform sampler2D drawn : hint_screen_texture, filter_linear_mipmap;
uniform sampler2D depth : hint_depth_texture, filter_nearest;
uniform float shimmer = 0.0;
uniform float near_m = 0.0;
uniform float far_m = 0.0;
uniform float wave_rows = 0.0;
uniform float wave_speed = 0.0;
uniform float wave_reach = 0.0;
uniform float drift = 0.0;
void vertex() {
    POSITION = vec4(VERTEX.xy, 1.0, 1.0);
}
void fragment() {
    vec4 seen = INV_PROJECTION_MATRIX * vec4(SCREEN_UV * 2.0 - 1.0, texture(depth, SCREEN_UV).r, 1.0);
    float metres = -seen.z / seen.w;
    float far_share = smoothstep(near_m, max(far_m, near_m), metres);
    float strength = clamp(shimmer, 0.0, 1.0) * far_share;
    float front = (SCREEN_UV.y - SCREEN_UV.x * clamp(drift, -1.0, 1.0)) * wave_rows;
    float shift = sin(front + TIME * wave_speed) * wave_reach * strength;
    ALBEDO = texture(drawn, SCREEN_UV + vec2(shift, 0.0)).rgb;
}

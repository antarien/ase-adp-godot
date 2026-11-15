/**
 * ASE GODOT ADAPTER - PLANT HORIZON RIDGE
 *
 * @file        plant_horizon_ridge.gdshader
 * @brief       The two ridges of hills on the horizon
 * @description The look of the ridges (m.html drawSky 422 to 424): unshaded, outside the fog, opaque where a ridge stands
 *              and discarded above it. The upright square spans ridge_top mockup units above the horizon at its top edge
 *              down to ridge_foot units below it at its bottom edge; x is the fragment's place across the HUD frame in
 *              mockup units, (FRAGCOORD.x - frame_origin_x) / frame_pixels, as the canvas runs x from 0 to 360. A ridge
 *              stands up to ridge_lift + height * (ridge_base + ridge_swing * sin(x * wave + phase)) units above the
 *              horizon, its height already scaled by the relief; the near ridge is drawn over the far one, as the canvas
 *              fills it second.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_071
 * @category   render/pipeline/stage
 * @usecase    usecase:5606 - observe the sky
 * @actor      Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.01.00000
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode unshaded, fog_disabled, cull_disabled;
uniform vec3 ridge_far_color : source_color = vec3(0.0);
uniform vec3 ridge_near_color : source_color = vec3(0.0);
uniform vec3 ridge_far = vec3(0.0);
uniform vec3 ridge_near = vec3(0.0);
uniform float ridge_lift = 0.0;
uniform float ridge_base = 0.0;
uniform float ridge_swing = 0.0;
uniform float ridge_top = 0.0;
uniform float ridge_foot = 0.0;
uniform float frame_origin_x = 0.0;
uniform float frame_pixels = 1.0;
float ridge_edge(vec3 ridge, float x) {
    return ridge_lift + ridge.x * (ridge_base + ridge_swing * sin(x * ridge.y + ridge.z));
}
void fragment() {
    float x = (FRAGCOORD.x - frame_origin_x) / frame_pixels;
    float height = mix(ridge_top, -ridge_foot, UV.y);
    if (height <= ridge_edge(ridge_near, x)) {
        ALBEDO = ridge_near_color;
    } else if (height <= ridge_edge(ridge_far, x)) {
        ALBEDO = ridge_far_color;
    } else {
        discard;
    }
}

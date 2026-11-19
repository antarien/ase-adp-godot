/**
 * ASE GODOT ADAPTER - PLANT BARK
 *
 * @file        plant_bark.gdshader
 * @brief       The bark of the birch, white above and dark at the base
 * @description The bark of the continuous tube of every axis (usecase:101613 show the plant bark tubes, usecase:100395
 *              age the bark, DSGN_024): one mesh per plant whose knot rings parent and child share, laid out by
 *              ase-render-data. It mixes the white bark with the stem-base bark by the share every vertex carries
 *              (CUSTOM0.r, GODOT_BARK_CUSTOM_VALUES). Its uniforms take the colour, the albedo, normal and roughness
 *              textures, the roughness and the UV scale of the two bound materials (GODOT_ROLE_BARK,
 *              GODOT_ROLE_BARK_BASE); a texture a material lacks samples white, a flat normal or full roughness. The
 *              tiling stands in the rest of CUSTOM0: UV runs once around the ring and in metres along the axis, so it
 *              is multiplied by the tiles of the white bark around and per metre along, and that of the base bark once
 *              more by its tiles per white tile. CUSTOM1 carries the sway slot and share, the radius of the ring and
 *              the order of its axis: a ring of the own tube narrower than least_pixels (view.least_ring_px) is
 *              held at that width, as the mockup holds its least stroke (register B0482; the operator order of
 *              Phase 06, 1:1 with VIVARIUM_PHASE_06_HUD_V2.html, replaces the closing of 2026-11-13, under which a
 *              young birch of 2.6 mm stem radius vanished at the 5 m of the care view). Each plant draws its bark twice with
 *              this shader (register B0317): the fine tube on every corner of the knot ring closes a ring of an
 *              order above shown_orders (OrderBudget::shown), the coarse tube of the twigs (coarse = true, evenly
 *              spaced corners) closes a ring up to it. It takes the swing of the branch by its two includes,
 *              sway_uniforms.gdshaderinc and sway_swing.gdshaderinc, with the slot and share of the vertex.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_066
 * @category   render/pipeline/stage
 * @usecase    usecase:101613 - show the plant bark tubes
 * @actor      Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-11
 * @modified    2026-11-14
 * @version     00.00.02.00000
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
#include "res://shaders/sway_uniforms.gdshaderinc"
uniform vec4 white_color : source_color = vec4(1.0);
uniform sampler2D white_albedo : source_color, hint_default_white, filter_linear_mipmap, repeat_enable;
uniform sampler2D white_normal : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D white_rough : hint_default_white, filter_linear_mipmap, repeat_enable;
uniform float white_roughness = 1.0;
uniform vec2 white_uv = vec2(1.0);
uniform vec4 base_color : source_color = vec4(1.0);
uniform sampler2D base_albedo : source_color, hint_default_white, filter_linear_mipmap, repeat_enable;
uniform sampler2D base_normal : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D base_rough : hint_default_white, filter_linear_mipmap, repeat_enable;
uniform float base_roughness = 1.0;
uniform vec2 base_uv = vec2(1.0);
uniform float shown_orders = 0.0;
uniform bool coarse = false;
uniform float least_pixels = 0.0;
varying float base_share;
varying vec3 tiles;
void vertex() {
    base_share = clamp(CUSTOM0.r, 0.0, 1.0);
    tiles = CUSTOM0.gba;
    // a ring the other tube draws closes onto its axis: the fine tube the orders up to shown_orders, the coarse
    // tube of the twigs the orders above them. Its quads fall to nothing, a quad between a drawn and a closed ring
    // tapers to the axis
    // only a perspective divides by the distance: the orthographic eye of the card viewport of a far body
    // (godot_plant_scene_impostor.cpp, PROJECTION_MATRIX[3][3] is 1 there and 0 in a perspective) sees a ring at
    // the same width at every distance, so the card keeps the widest ring at the one pixel impostor_of gave it
    vec3 view_at = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
    float eye_distance = mix(length(view_at), 1.0, PROJECTION_MATRIX[3][3]);
    float ring_pixels = CUSTOM1.b * PROJECTION_MATRIX[1][1] * VIEWPORT_SIZE.y / eye_distance;
    bool other_tube = coarse ? CUSTOM1.a <= shown_orders : CUSTOM1.a > shown_orders;
    // a ring of the own tube narrower than least_pixels (view.least_ring_px) is held at that width, as the mockup
    // holds its least stroke (VIVARIUM_PHASE_06_HUD_V2.html drawTree, register B0482); a ring the other tube draws
    // closes onto its axis. ring_pixels is the ring's width on the screen, so a ring of radius r grows to
    // r * least_pixels / ring_pixels
    if (other_tube) {
        VERTEX -= NORMAL * CUSTOM1.b;
    } else if (ring_pixels < least_pixels && ring_pixels > 0.0) {
        VERTEX += NORMAL * CUSTOM1.b * (least_pixels / ring_pixels - 1.0);
    }
#define SWAY_SLOT int(CUSTOM1.r + 0.5)
#define SWAY_SHARE CUSTOM1.g
#include "res://shaders/sway_swing.gdshaderinc"
}
void fragment() {
    vec2 uw = UV * tiles.xy * white_uv;
    vec2 ub = UV * tiles.xy * tiles.z * base_uv;
    vec3 white = texture(white_albedo, uw).rgb * white_color.rgb;
    vec3 base = texture(base_albedo, ub).rgb * base_color.rgb;
    ALBEDO = mix(white, base, base_share);
    NORMAL_MAP = mix(texture(white_normal, uw).rgb, texture(base_normal, ub).rgb, base_share);
    ROUGHNESS = mix(texture(white_rough, uw).r * white_roughness, texture(base_rough, ub).r * base_roughness,
                    base_share);
}

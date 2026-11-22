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
 *              THE KNOT FORM (PLAN_ASE_VIVARIUM_TREEGEN T.6, knot_form = true, set by fill_tube when the tube
 *              carries CUSTOM2): every vertex stands at the centre of its knot with the template direction
 *              (cos a, 0, -sin a) as its normal; the shader turns normal, tangent and binormal by the knot frame on
 *              CUSTOM2 and sets the corner out by the ring radius on CUSTOM1.b, so the corners stand where
 *              ase-render-data laid them. In the vertex form every vertex already stands at its corner.
 *
 *              THE BARK NOISE ("Rindenrauschen", plan T.6): d = wobble_amp * radius * fbm3(P * freq + o_axis), along
 *              the normal of the own tube, pure view. wobble_amp is GODOT_BARK_WOBBLE_MOST, the 0.15 the plan bounds
 *              the displacement at, and the fractal sum is held in [-1, 1]. freq, rough, the octaves and whether an
 *              order is noised are the noise columns of the Houdini generator per order (PLAN_ASE_VIVARIUM_TREEGEN_REF,
 *              freq<i>, rough<i>, maxoctave<i>, use_noise<i>; the plan takes them twice, for the growth and for the
 *              bark). o_axis is spread from the noise key of the axis on CUSTOM3.r as the generator spreads its
 *              per-branch offset ((random - 0.5) * 250 per axis of the field). P is the corner in the space of the
 *              plant, so the closing vertex of a ring and the rim of a cut face move with the ring they close.
 *
 *              THE CUT FACE (PHASE_09_TREEGEN Schnittfläche, cut_face = true on the wood material of the scene):
 *              the wood disk on the tip knot of a cut stub lies in the same arrays and carries the tiling of its
 *              rim, but its UV is turned around the pith in the middle of the wood texture, so it is sampled once
 *              over the disk instead of by the tiles of the bark.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_066
 * @category   render/pipeline/stage
 * @usecase    usecase:101613 - show the plant bark tubes
 * @actor      Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-11
 * @modified    2026-11-21
 * @version     00.00.03.00000
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
uniform bool knot_form = false;
uniform bool cut_face = false;
uniform float wobble_amp = 0.0;
varying float base_share;
varying vec3 tiles;
// the noise columns of the Houdini generator per axis order 0 to 5 (PLAN_ASE_VIVARIUM_TREEGEN_REF): freq<i>, rough<i>,
// use_noise<i>, and maxoctave<i>, 8 in every order; each octave doubles the frequency (lacunarity 2)
const float WOBBLE_FREQ[6] = {1.0, 1.0, 2.0, 1.0, 1.0, 1.0};
const float WOBBLE_ROUGH[6] = {0.2, 0.1, 0.1, 0.1, 0.1, 0.1};
const float WOBBLE_ON[6] = {1.0, 1.0, 1.0, 1.0, 1.0, 0.0};
const float WOBBLE_ORDER_MOST = 5.0;
const int WOBBLE_OCTAVES = 8;
const float WOBBLE_LACUNARITY = 2.0;
// the spread of the per-branch offset of the generator: (random - 0.5) * 250 per axis of the field
const float WOBBLE_CENTRE = 0.5;
const float WOBBLE_SPAN = 250.0;
const float WOBBLE_SHARE_STEPS = 65536.0;
const uint WOBBLE_SHARE_MASK = 0xffffu;
// the twelve edge directions of the gradient noise
const vec3 WOBBLE_GRADIENTS[12] = {
    vec3(1.0, 1.0, 0.0), vec3(-1.0, 1.0, 0.0), vec3(1.0, -1.0, 0.0), vec3(-1.0, -1.0, 0.0),
    vec3(1.0, 0.0, 1.0), vec3(-1.0, 0.0, 1.0), vec3(1.0, 0.0, -1.0), vec3(-1.0, 0.0, -1.0),
    vec3(0.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.0, 1.0, -1.0), vec3(0.0, -1.0, -1.0)};
// an integer hash: the noise rests on whole numbers, never on sin, so a handset and a desktop draw the same field
uint wobble_hash(uint h) {
    h ^= h >> 16u;
    h *= 0x7feb352du;
    h ^= h >> 15u;
    h *= 0x846ca68bu;
    h ^= h >> 16u;
    return h;
}
float wobble_slope(ivec3 corner, vec3 from) {
    uint h = wobble_hash(uint(corner.x) * 0x8da6b343u ^ uint(corner.y) * 0xd8163841u ^ uint(corner.z) * 0xcb1ab31fu);
    return dot(WOBBLE_GRADIENTS[int(h % 12u)], from);
}
// gradient noise of one octave, about [-1, 1], 0 at every whole point
float wobble_perlin(vec3 p) {
    ivec3 i = ivec3(floor(p));
    vec3 f = fract(p);
    vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float x00 = mix(wobble_slope(i, f), wobble_slope(i + ivec3(1, 0, 0), f - vec3(1.0, 0.0, 0.0)), u.x);
    float x10 = mix(wobble_slope(i + ivec3(0, 1, 0), f - vec3(0.0, 1.0, 0.0)),
                    wobble_slope(i + ivec3(1, 1, 0), f - vec3(1.0, 1.0, 0.0)), u.x);
    float x01 = mix(wobble_slope(i + ivec3(0, 0, 1), f - vec3(0.0, 0.0, 1.0)),
                    wobble_slope(i + ivec3(1, 0, 1), f - vec3(1.0, 0.0, 1.0)), u.x);
    float x11 = mix(wobble_slope(i + ivec3(0, 1, 1), f - vec3(0.0, 1.0, 1.0)),
                    wobble_slope(i + ivec3(1, 1, 1), f - vec3(1.0, 1.0, 1.0)), u.x);
    return mix(mix(x00, x10, u.y), mix(x01, x11, u.y), u.z);
}
// the fractal sum fbm3: each octave weighs rough times the one below, the sum is divided by its weights and held in
// [-1, 1], so the displacement never passes wobble_amp * radius
float wobble_fbm(vec3 p, float rough) {
    float sum = 0.0;
    float weight = 1.0;
    float weights = 0.0;
    float scale = 1.0;
    for (int k = 0; k < WOBBLE_OCTAVES; k++) {
        sum += weight * wobble_perlin(p * scale);
        weights += weight;
        weight *= rough;
        scale *= WOBBLE_LACUNARITY;
    }
    return clamp(sum / weights, -1.0, 1.0);
}
// o_axis from the noise key of the axis: three shares of its hash chain, each centred and spread
vec3 wobble_place(float key) {
    uint hx = wobble_hash(floatBitsToUint(key));
    uint hy = wobble_hash(hx);
    uint hz = wobble_hash(hy);
    vec3 share = vec3(float(hx & WOBBLE_SHARE_MASK), float(hy & WOBBLE_SHARE_MASK), float(hz & WOBBLE_SHARE_MASK));
    return (share / WOBBLE_SHARE_STEPS - WOBBLE_CENTRE) * WOBBLE_SPAN;
}
// a direction of the template turned by the unit quaternion of its knot
vec3 knot_turn(vec4 q, vec3 v) {
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}
void vertex() {
    base_share = clamp(CUSTOM0.r, 0.0, 1.0);
    tiles = CUSTOM0.gba;
    // the knot form: the template corner turned by the knot frame and set out by the ring radius (THE KNOT FORM)
    if (knot_form) {
        NORMAL = knot_turn(CUSTOM2, NORMAL);
        TANGENT = knot_turn(CUSTOM2, TANGENT);
        BINORMAL = knot_turn(CUSTOM2, BINORMAL);
        VERTEX += NORMAL * CUSTOM1.b;
    }
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
    } else {
        // the bark noise of the own tube along the normal (THE BARK NOISE); the vertex form carries no noise key
        int order = int(clamp(CUSTOM1.a, 0.0, WOBBLE_ORDER_MOST) + 0.5);
        vec3 place = wobble_place(knot_form ? CUSTOM3.r : 0.0);
        float wobble = wobble_fbm(VERTEX * WOBBLE_FREQ[order] + place, WOBBLE_ROUGH[order]);
        VERTEX += NORMAL * wobble_amp * WOBBLE_ON[order] * CUSTOM1.b * wobble;
        if (ring_pixels < least_pixels && ring_pixels > 0.0) {
            VERTEX += NORMAL * CUSTOM1.b * (least_pixels / ring_pixels - 1.0);
        }
    }
#define SWAY_SLOT int(CUSTOM1.r + 0.5)
#define SWAY_SHARE CUSTOM1.g
#include "res://shaders/sway_swing.gdshaderinc"
}
void fragment() {
    // the bark tiles around the ring and along the axis; the wood of a cut face is one picture over its disk
    vec2 tiling = cut_face ? vec2(1.0) : tiles.xy;
    float base_tiling = cut_face ? 1.0 : tiles.z;
    vec2 uw = UV * tiling * white_uv;
    vec2 ub = UV * tiling * base_tiling * base_uv;
    vec3 white = texture(white_albedo, uw).rgb * white_color.rgb;
    vec3 base = texture(base_albedo, ub).rgb * base_color.rgb;
    ALBEDO = mix(white, base, base_share);
    NORMAL_MAP = mix(texture(white_normal, uw).rgb, texture(base_normal, ub).rgb, base_share);
    ROUGHNESS = mix(texture(white_rough, uw).r * white_roughness, texture(base_rough, ub).r * base_roughness,
                    base_share);
}

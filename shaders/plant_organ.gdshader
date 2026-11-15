/**
 * ASE GODOT ADAPTER - PLANT ORGAN
 *
 * @file        plant_organ.gdshader
 * @brief       The material of a leaf, a catkin and a fruit
 * @description A bound organ template on a branch (leaf, catkin, fruit stand; godot_plant_look.cpp): its glTF material is a
 *              BaseMaterial3D, and a BaseMaterial3D cannot swing. This shader draws the one surface of such a template with what
 *              the material carries - colour, the albedo, normal, roughness and backlight textures, the UV scale and the alpha
 *              scissor - and takes the swing of the branch by its two includes (sway_*.gdshaderinc). On the leaf's
 *              material (GODOT_LEAF_SEASON 1) the colour turns toward the season's and a wilting leaf (the instance
 *              uniform condition at wilted_code) toward the dry olive, the globals light_plant_leaf_season sets.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_018
 * @category   render/pipeline/stage
 * @usecase    usecase:100387 - view the plant
 * @actor      Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-13
 * @version     00.00.01.00001
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode cull_disabled;
#include "res://shaders/sway_uniforms.gdshaderinc"
uniform vec4 organ_color : source_color = vec4(1.0);
uniform sampler2D organ_albedo : source_color, hint_default_white, filter_linear_mipmap, repeat_enable;
uniform sampler2D organ_normal : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D organ_rough : hint_default_white, filter_linear_mipmap, repeat_enable;
uniform sampler2D organ_light : source_color, hint_default_black, filter_linear_mipmap, repeat_enable;
uniform float organ_roughness = 1.0;
uniform vec2 organ_uv = vec2(1.0);
uniform float organ_cut = 0.0;
uniform int wilted_code = 1;
uniform float leaf_season = 0.0;
instance uniform float condition = 0.0;
global uniform vec4 leaf_season_color;
global uniform vec4 leaf_dry_color;
void vertex() {
#include "res://shaders/sway_swing.gdshaderinc"
}
void fragment() {
    vec2 at = UV * organ_uv;
    vec4 albedo = texture(organ_albedo, at) * organ_color;
    // the leaf takes the colour of its season and, while its stand wilts, the dry olive (m.html leafCol), by the
    // alpha of each global; normal, roughness and backlight of the texture stay
    vec3 color = mix(albedo.rgb, leaf_season_color.rgb, leaf_season * leaf_season_color.a);
    if (int(round(condition)) == wilted_code) {
        color = mix(color, leaf_dry_color.rgb, leaf_season * leaf_dry_color.a);
    }
    ALBEDO = color;
    ALPHA = albedo.a;
    ALPHA_SCISSOR_THRESHOLD = organ_cut;
    NORMAL_MAP = texture(organ_normal, at).rgb;
    ROUGHNESS = texture(organ_rough, at).r * organ_roughness;
    BACKLIGHT = texture(organ_light, at).rgb;
}

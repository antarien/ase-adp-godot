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
 *              uniform wilted at 1: the condition of its patch equals the wilted code of flora.cond.v1, usecase:101340)
 *              toward the dry olive, the globals light_plant_leaf_season sets.
 *              A leaf lifts its card by the bend of its species before it swings (PLAN_ASE_VIVARIUM_TREEGEN T.6,
 *              Gitter mit Biegung; REF bend_leaves): r and g of INSTANCE_CUSTOM carry the bend across and along from
 *              the bend port (flora.bend.v1). The card stands in the frame of the unit leaf of ase-render-data -
 *              x across in [-LEAF_HALF_EDGE, LEAF_HALF_EDGE], y along from the foot 0 to the tip 1, its normal on z -
 *              and is lifted on z by across * ramp(|x| / LEAF_HALF_EDGE) + along * ramp(y - LEAF_HALF_EDGE): the
 *              Houdini grid is centred, so its z is y - LEAF_HALF_EDGE here, and its linear ramps 0 to 1 clamp below
 *              0. A leaf takes the colour offset its lineage drew (PLAN_ASE_VIVARIUM_TREEGEN T.6, Farbe, Variante,
 *              Herbst): b of INSTANCE_CUSTOM carries it from the tint port (flora.tint.v1), and the fragment lays
 *              the factor 1 + offset on the colour of the card; INSTANCE_CUSTOM is read in the vertex stage only, so
 *              a varying hands the offset on. A leaf shows the atlas cell its lineage drew: a of INSTANCE_CUSTOM
 *              carries it from the atlas port (flora.atlas.v1) packed as cell * 16 + (cells - 1), and the fragment
 *              samples the texture at ((u + cell) / cells, v). A catkin or a fruit stand carries no custom data;
 *              INSTANCE_CUSTOM reads 0, lifts nothing, keeps the colour by the factor 1 and u by cell 0 of cells 1
 *              exactly.
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
uniform float leaf_season = 0.0;
instance uniform float wilted = 0.0;
// the autumn colour of the plant's individual, 0 green at the start of autumn to 1 at its own leaf fall (autumn port
// flora.autumn.v1, usecase:101646 colour the leaves in autumn); 0 while its record is unknown
instance uniform float autumn = 0.0;
global uniform vec4 leaf_season_color;
global uniform vec4 leaf_dry_color;
// 1 while the season is autumn: then the season's colour takes the leaf by the plant's own autumn colour
global uniform float leaf_season_autumn;
// half the edge of the unit leaf of ase-render-data (RENDER_DATA_TMPL_LEAF_HALF_EDGE): the midrib to the blade edge
const float LEAF_HALF_EDGE = 0.5;
// the cell and the cells share a of INSTANCE_CUSTOM as cell * ATLAS_PACK + (cells - 1) (GODOT_ATLS_PACK)
const float ATLAS_PACK = 16.0;
// the colour offset of the card from the tint port, read where INSTANCE_CUSTOM stands and handed to the fragment
varying float leaf_tint;
// the atlas cell of the card and the cells of its texture from the atlas port, handed on the same way
varying float leaf_cell;
varying float leaf_cells;
void vertex() {
    leaf_tint = INSTANCE_CUSTOM.b;
    float packed = round(INSTANCE_CUSTOM.a);
    leaf_cell = floor(packed / ATLAS_PACK);
    leaf_cells = packed - leaf_cell * ATLAS_PACK + 1.0;
    // the bend of the card across the midrib and along it, in leaf lengths of the unit leaf (bend port)
    VERTEX.z += INSTANCE_CUSTOM.r * clamp(abs(VERTEX.x) / LEAF_HALF_EDGE, 0.0, 1.0)
              + INSTANCE_CUSTOM.g * clamp(VERTEX.y - LEAF_HALF_EDGE, 0.0, 1.0);
#include "res://shaders/sway_swing.gdshaderinc"
}
void fragment() {
    // the cell of the texture the card shows: u moved by the cell and scaled by the cells (atlas port; cell 0 of
    // cells 1 keeps u exactly)
    vec2 at = vec2((UV.x + leaf_cell) / leaf_cells, UV.y) * organ_uv;
    vec4 albedo = texture(organ_albedo, at) * organ_color;
    // the leaf takes the colour of its season and, while its stand wilts, the dry olive (m.html leafCol), by the
    // alpha of each global; normal, roughness and backlight of the texture stay
    // in autumn every birch turns at its own pace: the share is its own autumn colour, not the season's alone
    float share = mix(1.0, autumn, leaf_season_autumn);
    vec3 color = mix(albedo.rgb, leaf_season_color.rgb, leaf_season * leaf_season_color.a * share);
    if (wilted > 0.5) {
        color = mix(color, leaf_dry_color.rgb, leaf_season * leaf_dry_color.a);
    }
    // the colour offset its lineage drew, on the card in every season (tint port; 0 keeps the colour exactly)
    color *= 1.0 + leaf_tint;
    ALBEDO = color;
    ALPHA = albedo.a;
    ALPHA_SCISSOR_THRESHOLD = organ_cut;
    NORMAL_MAP = texture(organ_normal, at).rgb;
    ROUGHNESS = texture(organ_rough, at).r * organ_roughness;
    BACKLIGHT = texture(organ_light, at).rgb;
}

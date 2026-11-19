/**
 * ASE GODOT ADAPTER - PLANT STAND IN
 *
 * @file        plant_stand_in.gdshader
 * @brief       The stand-in material of a role whose file is missing
 * @description The stand-in material (godot_plant_look.cpp, operator order of 2026-10-23): a role whose file is missing is
 *              drawn in its GODOT_COLOR_STAND_IN_* colour through this shader, so the scene reads as meadow, bark and leaf
 *              before Phase 04 delivers a file. The ground darkens with the soil water of the focused patch
 *              (GODOT_SHADER_SOIL_REL, -1 while unknown), a leaf takes the wilted or the dead colour by the marks of
 *              its patch (GODOT_SHADER_WILTED, GODOT_SHADER_DEAD: 1 where the condition of the patch equals the code
 *              ase-plant files for it on flora.cond.v1, usecase:101340, D2454). On the leaf's material (GODOT_LEAF_SEASON 1) the colour turns toward
 *              the season's (leaf_season_color, its alpha the share) and a wilting leaf toward the dry olive
 *              (leaf_dry_color), the two globals light_plant_leaf_season sets (m.html leafCol). Both faces are drawn: the stand-in leaf is a card without thickness. It
 *              takes the swing of the branch by its two includes (sway_*.gdshaderinc), so a stand-in organ hangs on its twig.
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
uniform vec4 stand_in_color : source_color = vec4(1.0);
uniform vec4 wilted_color : source_color = vec4(1.0);
uniform vec4 dead_color : source_color = vec4(1.0);
uniform float wet_shade = 1.0;
uniform float stand_in_roughness = 1.0;
instance uniform float soil_rel = -1.0;
instance uniform float wilted = 0.0;
instance uniform float dead = 0.0;
uniform float leaf_season = 0.0;
global uniform vec4 leaf_season_color;
global uniform vec4 leaf_dry_color;
void vertex() {
#include "res://shaders/sway_swing.gdshaderinc"
}
void fragment() {
    vec3 color = stand_in_color.rgb;
    color = mix(color, leaf_season_color.rgb, leaf_season * leaf_season_color.a);
    if (soil_rel >= 0.0) {
        color *= mix(1.0, wet_shade, clamp(soil_rel, 0.0, 1.0));
    }
    if (dead > 0.5) {
        color = dead_color.rgb;
    } else if (wilted > 0.5) {
        // the leaf of a wilting stand turns from its season toward the dry olive (m.html leafCol: S.soil < 25);
        // before the HUD stands that olive carries no share, and the leaf takes the wilted colour
        color = leaf_season * leaf_dry_color.a > 0.0 ? mix(color, leaf_dry_color.rgb, leaf_dry_color.a)
                                                     : wilted_color.rgb;
    }
    ALBEDO = color;
    ROUGHNESS = stand_in_roughness;
}

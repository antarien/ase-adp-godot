/**
 * ASE GODOT ADAPTER - CAMP FLAME
 *
 * @file        camp_flame.gdshader
 * @brief       The tongue of a burning fuel body of the camp fire, drawn upright toward the eye
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP, Endabnahme im Bild B2 to B6 (usecase:5436 - kindle fire). ase-fire
 *              keeps one fire cell per fuel body and lays its stage marks and heat on ingle.fire.v1; the view stands
 *              one quad over the body of every cell (VivariumView::follow_camp, godot_vivarium_camp.cpp) and drives
 *              this shader per instance. Nothing of the fire is computed here: the stage and the heat come from the
 *              owner, the shader only draws them.
 *
 *              stage 0 heating   a glow at the foot of the body that brightens as the heat rises
 *              stage 1 flame     a full tongue, flickering upward
 *              stage 2 charcoal  the same tongue; the view stands it lower, the volatiles are burnt
 *              stage 3 ember     no tongue, a red glow at the foot
 *              stage 4 ash       nothing; the view lays the ash bed beside it
 *
 *              heat_share is the heat of the cell over the hottest cell of the camp, so the tinder that catches first
 *              burns brightest and every log that catches after it brings its own tongue: the fire grows as its cells
 *              do, and shrinks as the view lowers each tongue with the fuel its cell has left.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_068
 * @category   render/pipeline/stage
 * @usecase    usecase:5436 - kindle fire
 * @actor      Funken (FNKN)
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.01.00001
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_never, blend_add, fog_disabled, shadows_disabled;

instance uniform float heat_share = 0.0;  // heat of the cell over the hottest cell of the camp, 0 to 1
instance uniform float stage = 0.0;       // 0 heating, 1 flame, 2 charcoal, 3 ember, 4 ash: the marks of ingle.fire.v1
instance uniform float seed = 0.0;        // the cell's own phase, so no two tongues flicker alike

uniform vec4 core_color : source_color = vec4(1.0, 0.9, 0.55, 1.0);   // the hot heart of a tongue
uniform vec4 rim_color : source_color = vec4(1.0, 0.38, 0.06, 1.0);   // the cooler edge of a tongue
uniform vec4 ember_color : source_color = vec4(0.85, 0.16, 0.02, 1.0); // the glow of heating wood and ember
uniform float rise_speed = 1.6;   // how fast the flicker climbs the tongue, tongue heights per second
uniform float flicker = 0.35;     // how far the noise bends the edge and the tip of a tongue

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float sum = 0.0;
    float weight = 0.5;
    for (int octave = 0; octave < 4; octave++) {
        sum += weight * noise(p);
        p *= 2.0;
        weight *= 0.5;
    }
    return sum;
}

void vertex() {
    // UPRIGHT TOWARD THE EYE: the quad turns about the up axis to face the camera and keeps the scale the view gave
    // it, so a tongue stays a tongue from the fixed view and from the diablo view alike
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 side = normalize(cross(up, INV_VIEW_MATRIX[2].xyz));
    vec3 front = normalize(cross(side, up));
    mat4 upright = mat4(vec4(side * length(MODEL_MATRIX[0].xyz), 0.0), vec4(up * length(MODEL_MATRIX[1].xyz), 0.0),
                        vec4(front * length(MODEL_MATRIX[2].xyz), 0.0), MODEL_MATRIX[3]);
    MODELVIEW_MATRIX = VIEW_MATRIX * upright;
    MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);
}

void fragment() {
    float v = 1.0 - UV.y;            // 0 at the foot of the tongue, 1 at its tip
    float x = (UV.x - 0.5) * 2.0;    // -1 to 1 across it
    float n = fbm(vec2(UV.x * 3.0 + seed, v * 2.5 - TIME * rise_speed));
    float width = (1.0 - v) * (0.55 + 0.45 * n);
    float body = smoothstep(width, width * 0.4, abs(x)) * smoothstep(1.0, 0.25, v + (n - 0.5) * flicker);
    float flaming = step(0.5, stage) * step(stage, 2.5);
    float glowing = step(2.5, stage) * step(stage, 3.5);
    float heating = 1.0 - step(0.5, stage);
    float foot = smoothstep(0.35, 0.0, v) * smoothstep(1.0, 0.2, abs(x));
    float tongue = body * flaming;
    float glow = foot * max(glowing, heating) * (0.6 + 0.4 * n);
    vec3 colour = mix(rim_color.rgb, core_color.rgb, smoothstep(0.2, 0.9, body * (1.0 - v)));
    colour = mix(colour, ember_color.rgb, clamp(glow * 2.0, 0.0, 1.0) * (1.0 - tongue));
    float alpha = clamp(tongue + glow, 0.0, 1.0) * clamp(heat_share, 0.0, 1.0);
    ALBEDO = colour * alpha;
    ALPHA = alpha;
}

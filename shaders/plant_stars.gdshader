/**
 * ASE GODOT ADAPTER - PLANT STARS
 *
 * @file        plant_stars.gdshader
 * @brief       The stars of the night sky
 * @description The look of every star (m.html drawSky, row N02): unshaded, outside the fog, blended by its alpha, its colour
 *              and its alpha the instance's. The alpha takes a factor: on a clear night (`twinkle` 1) the mockup's
 *              twinkle_base + twinkle_swing · sin(t · (twinkle_rate + twinkle_rate_span · size) + twinkle_phase_step · place),
 *              the size in pixels and the place in the set from the instance's custom data (red and green), t the shader's
 *              TIME; otherwise twinkle_steady. The flicker runs on the card, no star is laid anew for it.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_071
 * @category   render/pipeline/stage
 * @usecase    usecase:5606 - observe the sky
 * @actor      Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.01.00000
 */
shader_type spatial;
render_mode unshaded, fog_disabled, blend_mix, cull_back;
uniform float twinkle = 0.0;
uniform float twinkle_base = 1.0;
uniform float twinkle_swing = 0.0;
uniform float twinkle_rate = 0.0;
uniform float twinkle_rate_span = 0.0;
uniform float twinkle_phase_step = 0.0;
uniform float twinkle_steady = 1.0;
varying float star_rate;
varying float star_phase;
void vertex() {
    star_rate = twinkle_rate + twinkle_rate_span * INSTANCE_CUSTOM.r;
    star_phase = twinkle_phase_step * INSTANCE_CUSTOM.g;
}
void fragment() {
    float flicker = twinkle_base + twinkle_swing * sin(TIME * star_rate + star_phase);
    ALBEDO = COLOR.rgb;
    ALPHA = COLOR.a * mix(twinkle_steady, flicker, twinkle);
}

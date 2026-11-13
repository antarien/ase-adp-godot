/**
 * ASE GODOT ADAPTER - PLANT SUN HALO
 *
 * @file        plant_sun_halo.gdshader
 * @brief       The halo around the sun
 * @description The look of the halo (m.html drawSky 417, createRadialGradient(x, y, inner, x, y, outer)): unshaded, outside
 *              the fog, blended by its alpha, never writing depth. Up to halo_inner of the square's half width it stands in
 *              halo_color at halo_strength of its alpha; from there to the edge of the inscribed circle the colour and the
 *              alpha run linearly to halo_edge, as the canvas runs between its two stops; beyond the circle it stays at
 *              halo_edge, the canvas' last stop.
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
render_mode unshaded, fog_disabled, blend_mix, cull_disabled, depth_draw_never;
uniform vec4 halo_color : source_color = vec4(1.0);
uniform vec4 halo_edge : source_color = vec4(0.0);
uniform float halo_strength = 0.0;
uniform float halo_inner = 0.0;
void fragment() {
    float out_of_middle = length(UV * 2.0 - 1.0);
    float run = clamp((out_of_middle - halo_inner) / (1.0 - halo_inner), 0.0, 1.0);
    ALBEDO = mix(halo_color.rgb, halo_edge.rgb, run);
    ALPHA = mix(halo_color.a * halo_strength, halo_edge.a, run);
}

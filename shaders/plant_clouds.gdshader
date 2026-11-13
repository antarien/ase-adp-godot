/**
 * ASE GODOT ADAPTER - PLANT CLOUDS
 *
 * @file        plant_clouds.gdshader
 * @brief       The veil of a cloud bank
 * @description The veil of a cloud bank: unshaded, outside the fog, blended by its alpha, writing no depth. `at` runs in
 *              half axes of the bank from its middle, out to `reach` at the quad's rim. A gradient noise of `grain` cells per
 *              half axis, three octaves at gain one half and lacunarity two, pushes it in and out by up to `lift`; the
 *              opacity falls from full to none over the outer `soft` share of the half axis behind the frayed rim. Every
 *              octave, the first as well, is turned by the rotation (0.8, 0.6) once more than the one before, so no grid
 *              line of any octave runs along the picture's vertical or horizontal. A value noise interpolated by a fade
 *              holds its value flat across each grid line; an unturned first octave put such a line through the middle of
 *              every bank (the strip is a whole number of cells), and the frayed rim followed it as a straight vertical
 *              stroke. A gradient noise is zero on its grid points and changes across every line, and its quintic fade
 *              keeps the second derivative continuous, so its folds stay soft. Below
 *              the middle the colour darkens toward the foot by up to `shade`, so the bank reads lit from above. Each bank
 *              reads its own strip of the noise plane, one quad width of cells beside the last, so no two banks fray alike.
 *              The colour and the opacity are the instance's, given in sRGB as the tokens carry them and turned linear here.
 *              The hash of a cell takes its argument below one before it multiplies: a corner of one cell is the corner of
 *              the cell beside it, and however the compiler folds the sum that reaches it, both must read the same value.
 *              A sine hash of large arguments turns the last bit of that sum into a different value, and the bank breaks
 *              into hard tiles one cell wide.
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
render_mode unshaded, fog_disabled, blend_mix, cull_disabled, depth_draw_never, shadows_disabled;
uniform float reach = 1.0;
uniform float soft = 1.0;
uniform float grain = 1.0;
uniform float lift = 0.0;
uniform float shade = 0.0;
varying vec4 tint;
varying float strip;
float cell(vec2 p) {
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}
vec2 slope(vec2 p) {
    float a = cell(p) * 6.28318530718;
    return vec2(cos(a), sin(a));
}
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float a = dot(slope(i), f);
    float b = dot(slope(i + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
    float c = dot(slope(i + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
    float d = dot(slope(i + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));
    return 0.5 + 0.70710678 * mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}
void vertex() {
    vec3 low = COLOR.rgb / 12.92;
    vec3 high = pow((COLOR.rgb + 0.055) / 1.055, vec3(2.4));
    tint = vec4(mix(high, low, vec3(lessThan(COLOR.rgb, vec3(0.04045)))), COLOR.a);
    strip = float(INSTANCE_ID) * 2.0 * reach * grain;
}
void fragment() {
    vec2 at = (UV * 2.0 - 1.0) * reach;
    mat2 turn = mat2(vec2(0.8, 0.6), vec2(-0.6, 0.8));
    vec2 p = turn * (at * grain) + vec2(strip, 0.0);
    vec2 q = turn * p * 2.0;
    vec2 r = turn * q * 2.0;
    float n = (noise(p) + 0.5 * noise(q) + 0.25 * noise(r)) / 1.75;
    float edge = length(at) - (n * 2.0 - 1.0) * lift;
    float body = 1.0 - smoothstep(1.0 - soft, 1.0, edge);
    float foot = clamp(at.y, 0.0, 1.0) * shade;
    ALBEDO = tint.rgb * (1.0 - foot);
    ALPHA = tint.a * body;
}

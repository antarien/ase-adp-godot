/**
 * ASE GODOT ADAPTER - PLANT MOON
 *
 * @file        plant_moon.gdshader
 * @brief       The discs of the moons
 * @description The look of a moon disc (m.html drawSky 416, row N02): unshaded, outside the fog, blended by its alpha, in
 *              `moon_color`. A shadow disc in the colour of the sky lies over it - shadow_radius moon radii wide, its middle
 *              shadow_shift moon radii right of and above the moon's middle on the screen - and covers it by shadow_cover.
 *              The shadow is drawn by letting the sky through where it lies, so it always has the sky's own colour:
 *              the alpha falls to 1 - shadow_cover inside it. Right and up are those of the view, so the crescent stands as
 *              the mockup draws it however the camera turns.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_066
 * @category   render/pipeline/stage
 * @usecase    usecase:101384 - place sun and moon spatially
 * @actor      cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.01.00000
 */
shader_type spatial;
render_mode unshaded, fog_disabled, blend_mix, cull_back;
uniform vec4 moon_color : source_color = vec4(1.0);
uniform float disc_radius = 1.0;
uniform vec2 shadow_shift = vec2(0.0);
uniform float shadow_radius = 0.0;
uniform float shadow_cover = 0.0;
varying vec3 disc_middle;
void vertex() {
    disc_middle = (MODELVIEW_MATRIX * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
}
void fragment() {
    vec2 across = (VERTEX.xy - disc_middle.xy) / disc_radius;
    float shaded = step(length(across - shadow_shift), shadow_radius);
    ALBEDO = moon_color.rgb;
    ALPHA = moon_color.a * (1.0 - shadow_cover * shaded);
}

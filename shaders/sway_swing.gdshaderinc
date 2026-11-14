/**
 * ASE GODOT ADAPTER - SWAY SWING
 *
 * @file        sway_swing.gdshaderinc
 * @brief       The swing of a branch, set into vertex()
 * @description The tip of the branch swings about its mean displacement by the gust amplitude with the natural frequency
 *              of the branch, and the template's y, 0 at the base of the organ and 1 at its top, takes the share of that
 *              displacement from the base to the top, so neighbouring internodes of a branch meet and a leaf, whose two
 *              shares are one, moves whole with its place. ase-phytomer computed the values; the frequency is real, so the
 *              swing runs on TIME and never on the game clock. Set into vertex() with
 *              #include "res://shaders/sway_swing.gdshaderinc", after sway_uniforms.gdshaderinc stands in the head.
 *              An instanced organ finds its slot by INSTANCE_ID and its share by the template's y. A mesh that carries
 *              the swing per vertex, the bark tube of usecase:101613, defines SWAY_SLOT (the slot of the internode
 *              the vertex belongs to) and SWAY_SHARE (the share its ring takes) before the include.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_018
 * @category   render/pipeline/stage
 * @usecase    usecase:100387 - view the plant
 * @actor      Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-13
 * @version     00.00.01.00000
 */
#ifndef SWAY_SLOT
#define SWAY_SLOT INSTANCE_ID
#endif
    ivec2 sway_at = ivec2((SWAY_SLOT % sway_row) * 2, SWAY_SLOT / sway_row);
    vec4 sway_wave = texelFetch(sway_data, sway_at, 0);
    vec4 sway_form = texelFetch(sway_data, sway_at + ivec2(1, 0), 0);
    vec2 sway_tip = sway_wave.gb + vec2(sway_wave.a, sway_form.r) * sin(TAU * sway_wave.r * TIME);
#ifdef SWAY_SHARE
    float sway_share = SWAY_SHARE;
#else
    float sway_share = mix(sway_form.g, sway_form.b, clamp(VERTEX.y, 0.0, 1.0));
#endif
    VERTEX += inverse(mat3(MODEL_MATRIX)) * vec3(sway_tip.x, 0.0, sway_tip.y) * sway_share;

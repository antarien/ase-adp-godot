/**
 * ASE GODOT ADAPTER - FIGURE EDGE GLASS
 *
 * @file        figure_edge_glass.gdshader
 * @brief       The red glass pane that flashes where a walking body bumps the edge of the first own chunk
 * @description 07.13, usecase:101612 keep figure in loaded area: ase-physics holds every walking body inside the chunk
 *              borders of the ground and files each bump (HUB_PHYS_BNDY_HIT); the boundary port carries it to the view
 *              (avatar.boundary.v1, AvatarHostBndyExptSystem). Where the body touched the edge the scene stands a quad
 *              turned along the outward normal (flash_figure_edges, godot_figure_scene.cpp) and drives its glow per
 *              instance: 1 at the bump, falling to 0 over the fade. The pane is glass, not a wall drawn solid: its colour
 *              (FIGURE_EDGE_GLASS, its alpha the glass's own share) glows strongest where it is seen edge on (rim_power,
 *              hud_figure_edge_glass_rim) and thins toward the borders of the quad, so the touched point reads as the
 *              centre of the flash. No message stands with it; at a glow of 0 the scene hides the quad.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_019
 * @category   render/pipeline/stage
 * @usecase    usecase:101612 - keep figure in loaded area
 * @actor      Game Areal Plate (AREAL)
 * @created     2026-11-18
 * @modified    2026-11-18
 * @version     00.00.01.00001
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_never, blend_add, fog_disabled, shadows_disabled;
uniform vec4 glass_color : source_color = vec4(1.0);
uniform float rim_power = 1.0;
instance uniform float glow = 0.0;
void fragment() {
    float facing = abs(dot(normalize(NORMAL), normalize(VIEW)));
    float rim = pow(1.0 - facing, max(rim_power, 0.0));
    vec2 centred = abs(UV * 2.0 - 1.0);
    float spread = (1.0 - centred.x * centred.x) * (1.0 - centred.y * centred.y);
    ALBEDO = glass_color.rgb;
    ALPHA = clamp(clamp(glow, 0.0, 1.0) * spread * (glass_color.a + rim), 0.0, 1.0);
}

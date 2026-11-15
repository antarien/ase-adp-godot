/**
 * ASE GODOT ADAPTER - PLANT SKY
 *
 * @file        plant_sky.gdshader
 * @brief       The sky behind the scene from horizon to zenith
 * @description The background of the care view (registers B0184, B0187; godot_plant_environment.hpp): `background` is the
 *              sky high up, `horizon` the haze low over the land, both records of the sky port. The colour runs between
 *              them over the upward share of the view direction and reaches `background` at `frame_top`, the upward
 *              share of the ray through the middle of the frame's top edge. An overcast sky turns both ends toward
 *              its greys by `overcast_mix`. The mix happens in linear light, after Godot decoded the source colours once.
 *
 * @module     ase-adp-godot
 * @layer      5 (Adapter)
 * @design     DSGN_066
 * @category   render/pipeline/stage
 * @usecase    usecase:100387 - view the plant
 * @actor      Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.01.00000
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */
shader_type sky;
uniform vec3 background : source_color = vec3(0.0);
uniform vec3 horizon : source_color = vec3(0.0);
uniform vec3 overcast_zenith : source_color = vec3(0.0);
uniform vec3 overcast_horizon : source_color = vec3(0.0);
uniform float overcast_mix = 0.0;
uniform float frame_top = 1.0;
void sky() {
	vec3 high = mix(background, overcast_zenith, overcast_mix);
	vec3 low = mix(horizon, overcast_horizon, overcast_mix);
	COLOR = mix(low, high, clamp(EYEDIR.y / frame_top, 0.0, 1.0));
}

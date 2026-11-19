#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE EDGE GLASS
 *
 * @file        godot_figure_glass.hpp
 * @brief       The red glass pane that flashes where a walking body bumps the edge of the first own chunk
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.13: walking happens on the first own chunk only (32 x 32 m),
 *              and its edge belongs to the ground. ase-physics holds every walking body inside the chunk borders
 *              of HUB_GIS_GRND and files each bump (PhysicsWalkBndySystem, HUB_PHYS_BNDY_HIT); the boundary port
 *              of ase-pl-avatar carries it to the view (avatar.boundary.v1, AvatarHostBndyExptSystem) with the
 *              outward normal, the touched point and the count of bumps of that body.
 *
 *              THE PANE FLASHES, NO MESSAGE STANDS WITH IT: where the body touched the edge the scene stands one
 *              quad per body, turned along the outward normal, drawn with figure_edge_glass.gdshader. Whenever the
 *              count of the port differs from the count the pane flashed for, the pane glows at full for the hold
 *              and fades to nothing over the fade, then it is hidden. The view computes nothing of the edge: where
 *              it lies and when a body touched it is ase-physics' own statement, the pane only shows it.
 *
 *              THE MEASURES ARE SHA'S: width, height, hold, fade and the power of the rim are the tokens
 *              hud_figure_edge_glass_* of theme.css, the colour is FIGURE_EDGE_GLASS of colors.ts, both read with
 *              the HUD (load_figure_hud) and bound once by bind_figure_glass.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_019
 * @category    render/pipeline/stage
 * @usecase     usecase:101612 - keep figure in loaded area
 * @actor       Game Areal Plate (AREAL)
 * @created     2026-11-18
 * @modified    2026-11-18
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/color.hpp>

#include <vector>

namespace ase::adp::godot {

/**
 * The glass pane of the chunk edge, bound once by bind_figure_glass. All measures stay 0 and the material null until
 * then; the HUD refuses a measure not above 0 (load_figure_hud).
 */
struct FigureSceneGlass {
    double width = 0.0;   // metres along the edge the pane covers (hud_figure_edge_glass_width)
    double height = 0.0;  // metres from the feet of the bumping body up (hud_figure_edge_glass_height)
    double hold = 0.0;    // seconds at full glow after the bump (hud_figure_edge_glass_hold)
    double fade = 0.0;    // seconds it then fades to nothing (hud_figure_edge_glass_fade)
    ::godot::Ref<::godot::ShaderMaterial> material;  // figure_edge_glass.gdshader with colour and rim; one material, the glow per instance
};

/**
 * Binds the glass pane: the measures the HUD read and the material of res://shaders/figure_edge_glass.gdshader with
 * the colour (FIGURE_EDGE_GLASS) and the power of its rim (hud_figure_edge_glass_rim). The material stays null when
 * the shader file does not load; load_plant_shader stands that on the error log.
 */
FigureSceneGlass bind_figure_glass(double width, double height, double hold, double fade, double rim,
                                   const ::godot::Color& color);

/**
 * One pane per body of the boundary port under one holder node of the scene. Whenever the bumps of a body differ from
 * the count its pane flashed for, or the body has no pane yet, the pane stands at the touched point, from the feet of
 * the figure the body is up, turned along the outward normal, and glows at full for the hold, then fades over the
 * fade (`elapsed_s` the frame's seconds); at the end it is hidden. A body the scene draws no figure for has no feet
 * the view knows, so no pane stands for it. Nothing while the glass is not bound.
 */
void flash_figure_edges(::godot::Node3D* scene, const std::vector<VivariumFigureBumpView>& bumps,
                        const std::vector<VivariumFigureView>& figures, const FigureSceneGlass& glass,
                        double elapsed_s);

}  // namespace ase::adp::godot

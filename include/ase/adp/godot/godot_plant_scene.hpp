#pragma once

/**
 * ASE GODOT ADAPTER - PLANT SCENE
 *
 * @file        godot_plant_scene.hpp
 * @brief       The 3D scene of the care view: camera, sun, sky, ground and the plant from its organ views
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2: the view becomes a 3D scene outdoors - a
 *              camera with a fixed near view onto the birch, the ground of the patch and the brook beside
 *              it, a directional light with shadows, sky and ambient light from the values of
 *              ase-pl-sky. These functions make the Godot nodes and hand them the arrays of
 *              godot_plant_geometry.hpp; they compute no organ, no template, no matrix, no height and no
 *              light. The plant takes the instance and organ views of GodotHostResourceManager and the
 *              organ templates of its template ports (usecase:101011 tessellate the organ templates), the ground
 *              the terrain samples, the light the sky port (VivariumSkyView), the materials and the
 *              template files the look bound once (godot_plant_look.hpp, 03.2): a missing material is
 *              drawn in the error material, a missing template file as the port template that stands in
 *              for it. Soil water and condition reach the materials as instance shader parameters
 *              (mark_plant_stands); how they look is the asset's.
 *
 *              A PLANT IS A NODE AT ITS STEM BASE (03.11 step 2): one Node3D per body under
 *              GODOT_PLANT_BODIES, placed at the origin of its patch, and under it one
 *              MultiMeshInstance3D per bundle whose buffer is the instance records of that body as the
 *              port carried them - the plant-local 3x4 of ase-render-data becomes a world place through
 *              the node, never through a number changed here.
 *
 *              THE BARK IS ONE TUBE PER AXIS (register B0295 point 4, usecase:101613): the rings
 *              ase-render-data built whenever a body ticked, laid out by tubes_by_body into the bark node of a
 *              body (GODOT_PLANT_BARK, godot_plant_scene_mesh.cpp) with two meshes of the same vertices: the fine
 *              tube GODOT_PLANT_TUBES on every corner of the knot ring and the coarse tube of the twigs
 *              GODOT_PLANT_TWIGS on evenly spaced corners (register B0317). The order budget
 *              (godot_order_budget.hpp) reaches the bark shader as its shown orders: the fine tube closes a ring
 *              of a higher order onto its axis, the coarse tube a ring up to them, and both a ring thinner than
 *              a pixel. Leaves, catkins and fruit stands are instances of their templates. The
 *              root lies in the ground and is not drawn (03.1 point 4). A FAR BODY IS AN IMPOSTOR: beyond the
 *              distance at which its widest ring is thinner than a pixel (impostor_of, godot_plant_impostor.hpp)
 *              one card stands for the plant, drawn once by a viewport of its own (godot_plant_scene_impostor.cpp).
 *
 *              THE SUN is the light every scene of this adapter takes from the host
 *              (light_asset_showcase, godot_asset_showcase_scene.hpp), here with shadows - one way from
 *              the sky port to a DirectionalLight3D, not a second.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-11
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/godot_plant_impostor.hpp>
#include <ase/adp/godot/godot_plant_environment.hpp>  // SkyOvercastShade, the shade of an overcast sky
#include <ase/adp/godot/godot_plant_look.hpp>
#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * Builds the scene under `parent` and returns its root (GODOT_PLANT_SCENE_ROOT): the current camera, the
 * environment, the empty ground, the empty node the plants stand under (GODOT_PLANT_BODIES), the moon light
 * and the two hidden discs of sun and moon. The sun is set by light_plant_scene once the sky port carried
 * its records.
 */
::godot::Node3D* build_plant_scene(::godot::Node* parent, const PlantSceneCamera& camera);

/** Puts the camera at the near view onto `focus` as patch_focus_aim places it (select a patch, godot_patch_focus.hpp). */
void aim_plant_camera(::godot::Node3D* scene, const PlantSceneCamera& camera, const PlantOrigin& focus);

/**
 * Replaces the ground mesh with build_ground over the samples and cells of the ports - places, normals,
 * texture coordinates and triangles as ase-terrain and ase-render-data laid them -, its vertex colours
 * laid by mark_ground over the levelled `discs` and the brook `beds`, drawn in `material`, the ground of
 * the focused patch's soil (PlantSceneLook::ground), whose grid lines stand `step_m` apart: the distance
 * of two height samples. P0690: where `surface` was built from the same samples and cells, patch_ground
 * writes only what moved and send_ground_patch hands Godot those spans; the mesh is built anew only
 * otherwise, and hold_ground_surface then reads where Godot keeps its vertices into `surface`.
 */
void fill_plant_ground(::godot::Node3D* scene, const std::vector<VivariumTerrainView>& samples,
                       const std::vector<VivariumGroundCellView>& cells, const std::vector<PlantGroundDisc>& discs,
                       const std::vector<PlantGroundBed>& beds, float step_m,
                       const ::godot::Ref<::godot::Material>& material, PlantGroundSurface& surface);

/**
 * After a full build of the ground (godot_plant_ground_mesh.cpp): the layout RenderingServer names for the
 * format of `mesh`'s surface and the vertex and attribute streams Godot made of it, read into `surface`;
 * `surface.patchable` only where ground_code_holds found Godot's code in them.
 */
void hold_ground_surface(const ::godot::Ref<::godot::ArrayMesh>& mesh, PlantGroundSurface& surface);

/** Hands Godot the spans the last patch_ground named, and the grown bounds where a vertex moved past them. */
void send_ground_patch(const ::godot::Ref<::godot::ArrayMesh>& mesh, const PlantGroundSurface& surface);

/**
 * The arrays of one surface as an ArrayMesh, normals included as they stand (godot_plant_scene_mesh.cpp); an
 * empty surface is no mesh.
 */
::godot::Ref<::godot::ArrayMesh> mesh_of(const PlantMeshArrays& arrays);

/**
 * Puts the 3x4 records of `rows` - twelve values each, as the instance port carried them - on the MultiMesh of
 * `node`, each drawing `shape`; `material` overrides its surfaces, an empty one leaves the template's own. With
 * `custom`, every instance carries its GODOT_INSTANCE_CUSTOM_VALUES after its twelve values. With `sway`, the
 * node gets a material of its own, made from `material`, that carries the sway of its instances as a data
 * texture (usecase:101335 animate the branch sway). No node, nothing.
 */
void fill_instances(::godot::MultiMeshInstance3D* node, const ::godot::Ref<::godot::Mesh>& shape,
                    const std::vector<float>& rows, const std::vector<float>* custom,
                    const ::godot::Ref<::godot::Material>& material, const std::vector<float>* sway);

/**
 * Puts the bark tube of one body on `node` (usecase:101613 show the plant bark tubes): one mesh with CUSTOM0 and
 * CUSTOM1 beside every vertex as tubes_by_body laid them, closed by `indices` - the fine tube (arrays.indices) or
 * the coarse tube of the twigs (twig_indices, register B0317) - and a material of its own, made from `material`,
 * with the sway data texture of its internodes, `shown_orders` (GODOT_BARK_SHOWN_ORDERS) and `coarse`
 * (GODOT_BARK_COARSE). A tube without vertices or indices draws nothing. No node, nothing.
 */
void fill_tube(::godot::MeshInstance3D* node, const PlantBodyTube& tube, const std::vector<int32_t>& indices,
               const ::godot::Ref<::godot::Material>& material, uint32_t shown_orders, bool coarse);

/**
 * Puts the impostor nodes under `plant` (register B0295 point 4, usecase:101613): the card GODOT_PLANT_IMPOSTOR,
 * hidden, and the viewport GODOT_PLANT_IMPOSTOR_VIEW of its own world with an orthographic camera and one drawing
 * node per drawing node of the plant, drawing nothing yet. No plant, nothing.
 */
void make_impostor(::godot::Node3D* plant);

/**
 * Stands the card in for `plant` from `frame.reach` on: the drawing nodes of the plant end their visibility range
 * there, the drawing nodes of the viewport take the resources the plant nodes were just filled with, the viewport
 * draws the texture once at the pixels of the frame, and the card begins at the reach in the size of the body. A
 * frame with reach 0 hides the card and lets the plant draw at every distance. No plant, nothing.
 */
void fill_impostor(::godot::Node3D* plant, const PlantImpostorFrame& frame);

/**
 * Replaces the plants: one node per body of `origins` at its stem base, under it the bark tube of the body
 * (fill_tube over the bark port, the shown orders `shown_orders`) and the instance records of leaves, catkins
 * and fruit stands put into MultiMesh buffers as the ports carried them, on their templates. While the template
 * ports carry no set yet the stand-ins are empty meshes and draw nothing. A plant whose body lost its origin
 * leaves the scene.
 */
void fill_plant_organs(::godot::Node3D* scene, const GodotHostResourceManager& resources,
                       const std::unordered_map<uint32_t, PlantOrigin>& origins, uint32_t shown_orders,
                       const PlantSceneLook& look);

/**
 * Tells the materials what the patch records carry (03.2): the ground the soil water of the focused
 * patch (`ground`, GODOT_SHADER_SOIL_REL), the leaves of each plant the condition of its patch (`stands`
 * by body, GODOT_SHADER_CONDITION), as instance shader parameters. An unknown value sets the parameter
 * back to the shader's own default. Wet soil and a wilted leaf look as the asset's shader draws them; a
 * material without the parameter draws as it is.
 */
void mark_plant_stands(::godot::Node3D* scene, const std::unordered_map<uint32_t, PlantStand>& stands,
                       const PlantStand& ground);

/**
 * Replaces the seeds (register N0173, usecase:100711 publish the seed places): one instance of the nutlet
 * template per seed of the seed port (GodotHostResourceManager::get_seeds) whose body has an origin, at the
 * stem base of that body plus the seed's place - the place ase-physics holds, flying or landed, taken as the
 * port carried it, never moved here. A bound template is drawn at scale one (it is built in metres); while
 * its file is missing the leaf template of the template ports (GODOT_TMPL_LEAF) stands in at
 * GODOT_NUTLET_STAND_IN_M in the error material. A seed whose body has no origin is not drawn.
 */
void fill_plant_seeds(::godot::Node3D* scene, const GodotHostResourceManager& resources,
                      const std::unordered_map<uint32_t, PlantOrigin>& origins, const PlantSceneLook& look);

/**
 * Sets the sun (with shadows), the moon light, the sky colour behind the scene, the ambient light and the
 * fog from the sky port, and stands the discs of sun and moon in the sky along their directions
 * (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11: sun, moon and the day cycle). Sky colour, ambient light, fog and
 * exposure are light_plant_environment's (godot_plant_environment.hpp): the fog veils the ground only, the
 * sky shows the background record through its sky shader, and the stars stand before it. The moon light and its disc stand
 * only while the port carries a moon record; the sun disc only while the sun is above the horizon plane.
 * `overcast` dims the sun by its light factor and reaches light_plant_environment for the sky and the ambient
 * light (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N01). `edge` is the ground's visible far edge (plant_ground_far_edge;
 * null while none stands); light_plant_sky stands the sky dome so its horizon lies there (B0262).
 */
void light_plant_scene(::godot::Node3D* scene, const VivariumSkyView& sky, const SkyOvercastShade& overcast,
                       const ::godot::Vector3* edge);

}  // namespace ase::adp::godot

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
 *              THE LINE BETWEEN FINE AND COARSE is the order budget's (godot_order_budget.hpp): every
 *              internode is a cylinder instance, those up to the shown order on the fine template
 *              (GODOT_TMPL_FINE), the finer ones on the coarse one (GODOT_TMPL_CORS); leaves, catkins and
 *              fruit stands are instances of their templates. The root lies in the ground and is not
 *              drawn (03.1 point 4).
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
 * @modified    2026-10-26
 * @version     00.00.06.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/godot_plant_look.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/material.hpp>
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
 * of two height samples.
 */
void fill_plant_ground(::godot::Node3D* scene, const std::vector<VivariumTerrainView>& samples,
                       const std::vector<VivariumGroundCellView>& cells, const std::vector<PlantGroundDisc>& discs,
                       const std::vector<PlantGroundBed>& beds, float step_m,
                       const ::godot::Ref<::godot::Material>& material);

/**
 * Replaces the plants: one node per body of `origins` at its stem base, under it the instance records of
 * the five drawn classes put into MultiMesh buffers as the ports carried them - the internodes up to
 * `shown_orders` on the fine cylinder of the template ports (GODOT_TMPL_FINE), those above on the coarse
 * one (GODOT_TMPL_CORS), leaves, catkins and fruit stands on their templates. While the template ports
 * carry no set yet the cylinders and the stand-ins are empty meshes and draw nothing. A plant whose body
 * lost its origin leaves the scene.
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
 * (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11: sun, moon and the day cycle). The moon light and its disc stand
 * only while the port carries a moon record; the sun disc only while the sun is above the horizon plane.
 */
void light_plant_scene(::godot::Node3D* scene, const VivariumSkyView& sky);

}  // namespace ase::adp::godot

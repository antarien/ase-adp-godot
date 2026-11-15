#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE SCENE
 *
 * @file        godot_figure_scene.hpp
 * @brief       The figures of the body port in the care view's 3D scene and the player camera that follows one
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.9: every record of the body port of ase-pl-avatar
 *              (avatar.body.v1) stands in the scene as one node at its place, turned to its course. The
 *              node carries the figure asset of the stage (role `figure`, villager_body) once the binding
 *              holds its file; while the file is missing it is a stand-in body of the figure's height in the
 *              error material, with a mark on its front so the course reads.
 *
 *              THE FILE LOADS ONCE, ON A WORKER THREAD, AND WARMS UNDER THE START SCREEN: the binding names
 *              the bound file, and the view hands its parse and pack to a task of Godot's WorkerThreadPool
 *              (godot_figure_load.hpp), so its ready() returns at once and the start screen draws while the
 *              file loads (villager_body.glb is 169.6 MB). The view takes the pack on the frame the task
 *              completed; every figure node is an instance of that pack, so its meshes and materials are the
 *              pack's own resources. While the file loads no figure node is built and the start screen holds.
 *              Once the pack is taken, one instance of it stands in the scene behind the screen: its
 *              pipelines compile before the screen lifts and never under a visible frame. Once the screen
 *              lifted that instance leaves the scene.
 *
 *              THE BODY IN MOTION (07.9): the four clips of the file (idle, walk, run, turn) loop and one
 *              AnimationTree per figure mixes them - idle, walk and run by the gait of the port, the turn
 *              clip while the body turns on the spot toward the course of the port. Of the four detail levels
 *              (meshes named _lod<k>_) the one drawn follows the figure's share of the frame height against
 *              the three bands the port carries (RND_CONST_FIGURE_LOD_NEAR/_MID/_FAR). Under the Skeleton3D of
 *              the rig a BoneAttachment3D (FigureSocketHandR) follows the socket bone socket_hand_r: what the
 *              right hand carries (07.10, ase-equipment) hangs below it and moves with every clip.
 *
 *              THE TOOL IN THE HAND (07.10): the gear port of ase-pl-avatar (avatar.gear.v1) carries one
 *              record per piece a figure bears; the piece at socket_hand_r hangs below the socket as an
 *              instance of its asset - spade (pa), hoe (enxada), knife (faca), the roles hand.<item key> of
 *              the asset list, loaded by tasks of the WorkerThreadPool like the figure file
 *              (godot_figure_hand.hpp). The stand-in body carries a hand node of the same name at its right
 *              side, so the tool reads there too.
 *
 *              THE PLAYER CAMERA (07.6): while the camera port carries the eye of a camera that follows a
 *              figure, the scene's camera stands at that eye and looks at its aim. The fixed view of Phase
 *              06 is untouched: once the port carries no record, the view aims its own camera again
 *              (scene_focus_). Which camera is drawn is ase-camera's; the view only shows it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101611 - animate character motion
 * @actor       SS/PC
 * @created     2026-11-13
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/godot_figure_hand.hpp>
#include <ase/adp/godot/godot_figure_load.hpp>
#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * How the drawn body follows the course its port reports, SHA's hud_figure_body_turn_* read with the HUD
 * (load_figure_hud); both stay 0 until the configuration is read, and the HUD refuses a value not above 0.
 */
struct FigureSceneMotion {
    double turn_rate = 0.0;  // share of the open course the drawn body closes per second (hud_figure_body_turn_rate)
    double turn_full = 0.0;  // open course in radians at which the turn clip plays alone on the spot (hud_figure_body_turn_full)
    double look_lift = 0.0;  // metres above the feet the camera's distance is measured to, for the detail level (hud_figure_body_look_lift)
};

/** What the figures are drawn with, bound once when the configuration is read. */
struct FigureSceneLook {
    std::string                        name;      // the asset name of the role `figure` (villager_body); empty when the stage names none
    std::string                        file;      // the bound file of that name below the asset root; empty while it is missing
    ::godot::Ref<::godot::PackedScene> model;     // the bound file, loaded once and packed; every figure node is an instance of it; null while it loads, is missing or does not load
    ::godot::Ref<::godot::Material>    stand_in;  // the error material of the stand-in body, drawn while the file is missing or does not load
    FigureSceneLoad                    load;      // the task that parses and packs the file (godot_figure_load.hpp); while it runs no figure node is built
    FigureSceneMotion                  motion;    // how the body turns toward the course of the port, SHA's measures
    std::unordered_map<uint32_t, FigureHandTool> tools;  // the tools of the hand (roles hand.<item key>), keyed by name_id of the item key
    FigureHandTool                     rack;      // the body of the tool place (role rack), loaded like a tool; lay_tools stands it
};

/**
 * Binds the figure of the stage: the file of the role `figure`, the error material of its name and the motion the
 * HUD read before (load_figure_hud runs ahead of the binding). The file itself loads in a task of the
 * WorkerThreadPool (start_figure_load on the look's `load`).
 */
FigureSceneLook bind_figure_look(const VivariumAssetBinding& binding,
                                 const std::unordered_map<std::string, std::string>& roles,
                                 const FigureSceneMotion& motion);

/**
 * Stands one instance of the packed figure file in the scene while the start screen stands, so the pipelines of
 * its materials compile under the screen; nothing when the file is missing or does not load.
 */
void warm_figure(::godot::Node3D* scene, const FigureSceneLook& look);

/** Takes the instance warm_figure stood out of the scene once the start screen lifted. */
void end_figure_warmth(::godot::Node3D* scene);

/**
 * One node per figure of the body port under the scene, at its place and turned to its course; a node whose
 * figure the port no longer carries is freed. A new node is an instance of the packed file, or the stand-in
 * body while the binding holds none. The piece of the gear port that a figure carries at socket_hand_r hangs
 * below its socket as the stand-in of its kind (pa, enxada, faca); it is rebuilt when the kind changes and
 * leaves when the port carries no piece in that hand. The clip mix reads the gait against the codes of the code
 * port (avatar.code.v1, `codes`): stand, walk and run are ase-pl-avatar's values, never the adapter's; while no
 * code record stood the clips are neither set up nor driven.
 */
void fill_figures(::godot::Node3D* scene, const std::vector<VivariumFigureView>& figures,
                  const std::vector<VivariumFigureGearView>& gear, const FigureSceneLook& look,
                  const VivariumFigureCodeView& codes);

/** The scene's camera at the eye of the player camera, looking at its aim; false while the port carries none. */
bool aim_figure_camera(::godot::Node3D* scene, const VivariumFigureCameraView& camera);

/**
 * The height in metres of the figure `object_id` as its node carries it: level 0 of the figure file measured
 * when the node was built, or the stand-in's height. False while the scene stands no node for that figure.
 */
bool figure_height(::godot::Node3D* scene, uint32_t object_id, double& height);

/**
 * THE RIG OF A FIGURE, ONCE (07.6): eye height and height in metres above the soles of the figure `object_id`,
 * measured when its node was built - the eyes from the rest pose of the bones L_Eye and R_Eye of its Skeleton3D,
 * the height from level 0. True exactly once per node, on the first call after the node carries a measured rig;
 * the node is marked taken, so the view reports it once to the rig port of ase-pl-avatar. False for the stand-in
 * body (it has no skeleton), for a file whose skeleton lacks an eye bone and while the scene stands no such node.
 */
bool take_figure_rig(::godot::Node3D* scene, uint32_t object_id, double& eye_height, double& height);

/**
 * THE PLACE OF A FIGURE, BY STEPS: the place of the node of figure `object_id` in the scene (x east, y up, z north,
 * metres). True on the first call after the node stood and then whenever the body moved at least `step_m` from the
 * place last returned; the node keeps that place, so the view's place line is written per step and never per frame.
 * False while the scene stands no such node.
 */
bool take_figure_place(::godot::Node3D* scene, uint32_t object_id, double step_m, double& x, double& y, double& z);

/**
 * THE FACE AND THE EYE AS THEY ARE DRAWN (07.3): the edges in texels of the albedo bound on the face material
 * (Std_Skin_Head of villager_body) and on the iris material (__Iris) of the node of figure `object_id` - the pack of
 * the stage or the image of the file - and in `level` the coarser of the detail levels the meshes carrying them
 * show. The view notes all three when the zoom comes to rest, so a run holds face and eye against the edges of their
 * source images and against level 0 - both are sharp only with the texture of the source on the finest level.
 * False for the stand-in body, when a shown level lacks either material or its albedo, and while no such node stands.
 */
bool figure_face_look(::godot::Node3D* scene, uint32_t object_id, int32_t& level, int64_t& face_edge,
                      int64_t& eye_edge);

}  // namespace ase::adp::godot

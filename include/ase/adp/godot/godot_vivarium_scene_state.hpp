#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW: THE SCENE STATE
 *
 * @file        godot_vivarium_scene_state.hpp
 * @brief       VivariumSceneState - the asset set the view binds and the 3D scene of the care view with its ground modes
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1, 03.2, 03.6 and 03.8, PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7:
 *              the play stage and its bound asset names, the asset showcase, the root of the 3D scene and its
 *              camera, zoom and look, the order budget of the bark, the generations each part of the scene was
 *              last laid from, the ground as last handed to Godot, the pick radii and the modes a tap in the scene
 *              runs in (cutting, bed, flatten) with the beds and discs the player laid in this host.
 *
 *              ONE RESPONSIBILITY, ITS OWN HEADER: VivariumView (godot_vivarium_node.hpp) holds this record as its
 *              member scene_state_ (composition, the validator forbids inheritance), so its member functions in
 *              godot_vivarium_scene.cpp, ..._assets.cpp, ..._prune.cpp, ..._bed.cpp and ..._flatten.cpp read these
 *              members as scene_state_.<name>. A new member of the scene's state
 *              is written here, never into the view's header (operator order 2026-11-25: the view's header stood
 *              at the limit of 500 lines).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    R0221 (the split of the view's header)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/godot_order_budget.hpp>
#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_vivarium_water_marks.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/** The bound asset set and the 3D scene of the care view with the modes a tap in it runs in (03.1 to 03.8, 04.7). */
struct VivariumSceneState {
    uint32_t                 play_stage_ = 0;  // play stage whose asset names the view binds (play_stage)
    ::godot::String          asset_root_;      // res:// directory of the manifest's files
    std::unordered_map<std::string, std::string> asset_roles_;  // role the view draws with → asset name of that stage
    std::vector<std::string> asset_names_;     // the stage's asset names in list order, materials first
    ::godot::String          asset_manifest_;  // the manifest of the asset set: read, broken or absent
    VivariumAssetBinding     assets_;          // bound files and missing names, bound at configuration
    std::vector<std::string> asset_materials_;  // the stage's material names; every other name of asset_names_ is an organ template
    ::godot::Node3D*         showcase_ = nullptr;  // root of the asset showcase (modes.assets), nullptr without its launch argument
    ::godot::Node3D*         scene_ = nullptr;     // root of the 3D scene of the care view, built once the host booted (03.1)
    PlantSceneCamera         scene_camera_;        // the near view (view.camera_*)
    float                    zoom_ = GODOT_ZOOM_START;  // the near view's zoom, moved by the mouse wheel or a pinch
    PlantSceneLook           scene_look_;         // materials and organ templates of the scene, bound once at the start (03.2)
    OrderBudget              order_budget_;        // the axis orders the bark shader shows, by frame time (view, 03.6 V8, B0295)
    std::unordered_map<uint32_t, PlantOrigin> scene_origins_;  // stem base per patch object, last placed
    uint64_t                 scene_organs_ = 0;    // organ generation the plant was last filled from
    uint64_t                 scene_bark_ = 0;      // bark generation the tubes were last laid from: only a body that ticked moves it
    uint64_t                 scene_terrain_ = 0;   // terrain generation the ground was last built from
    PlantGroundSurface       ground_surface_;      // the ground as last handed to Godot, the input of its patch (P0690)
    VivariumWaterMarks       water_marks_;         // the brook, pit and current generations the water was last laid from
    int64_t                  scene_hour_ = -1;     // whole game hour of the day of the last `scene hour` line, -1 before the clock
    uint64_t                 scene_stars_ = 0;     // star generation of the sky port at the last `scene sky` line, 0 before the first star set
    uint64_t                 scene_templates_ = 0; // template generation the plant was last filled from (the organ templates tessellated)
    uint32_t                 scene_focus_ = GODOT_NO_PLACE;  // the place the camera last looked at
    float                    pick_px_ = 0.0f;      // logical pixels around a tap that hit an internode's middle (view.pick_px)
    float                    least_ring_px_ = 0.0f;  // screen pixels a bark ring narrower than this is held at (view.least_ring_px)
    bool                     pruning_ = false;     // the cutting mode is on: a tap in the scene cuts, it selects no place
    bool                     bedding_ = false;     // the bed mode is on: a tap in the scene marks a bed's end, it selects no place
    bool                     bed_started_ = false; // the first end of the bed is marked
    float                    bed_x_ = 0.0f;        // the first end, world metres east
    float                    bed_z_ = 0.0f;        // the first end, world metres north
    std::vector<VivariumBrookSpec> player_beds_;   // the beds the player laid in this host, marked on the ground like the configured ones
    bool                     flattening_ = false;  // the flatten mode is on: a tap in the scene levels a disc, it selects no place
    float                    flatten_m_ = 0.0f;    // radius of the disc one tap levels, metres (view.flatten_m)
    std::vector<PlantGroundDisc> player_flats_;    // the discs the player levelled in this host; their height is the ground's at the centre
};

}  // namespace ase::adp::godot

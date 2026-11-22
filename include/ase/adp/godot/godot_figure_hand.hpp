#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE HAND
 *
 * @file        godot_figure_hand.hpp
 * @brief       The tool a figure carries in its right hand, drawn below the hand socket of its node
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.10: the gear port of ase-pl-avatar (avatar.gear.v1)
 *              carries one record per piece a figure bears - its bearer, the socket it hangs at and the
 *              kind of the piece (name_id of the item key of the hand of Phase 06). The piece at the right
 *              hand hangs below the hand socket of the figure's node as an instance of its asset: the asset
 *              list names it under the role hand.<item key> of the play stage (vivarium_assets.json), the
 *              binding gives its file and a task of the WorkerThreadPool loads it as the figure's file is
 *              loaded. While the file is missing, loading or does not load, the piece is a block in the error
 *              material of its name (the asset is then reported once at boot as `asset missing <name>`, the
 *              gap N0448 names); a kind the list names no role for is that block in the error material of
 *              the figure. The piece is rebuilt when its kind changes or its file arrives and leaves when the
 *              port carries no piece in that hand.
 *              THE TOOL PLACE IN 3D (PLAN_ASE_VIVARIUM_PHASE_06_HUD 8.3): the same tool files stand in the scene
 *              when a tool is not carried. The rack body of ase-pl-toolrack is an instance of the file of the
 *              role rack (prop_tool_rack_fork, its origin the ground point between the stakes) at the place the
 *              carry bridge gives (toolrack.rack.v1); a tool at the place leans against its crossbar with its
 *              lowest point on the ground at its slot, a tool let go lies flat where it was let go. How far a
 *              tool reaches below its grip and how thick it is are read from the bounds of its loaded file, so
 *              the adapter guesses no length. A thing the asset list names no tool for keeps its 2D drawing.
 *              A tap reaches the place through its body: rack_struck casts the ray of the camera through the
 *              finger onto the bounds of the rack body, the way VivariumView::pick_ground meets the ground (D01).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101625 - carry a tool in hand
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/godot_figure_load.hpp>
#include <ase/adp/godot/godot_host_figure_view.hpp>
#include <ase/adp/godot/godot_hud_world_drop.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/** One tool of the hand as the asset list names it: its asset, the bound file, its load and its error material. */
struct FigureHandTool {
    std::string                        name;      // the asset name of the role hand.<item key> (tool_spade_ironshod)
    std::string                        file;      // the bound file of that name below the asset root; empty while it is missing
    ::godot::Ref<::godot::PackedScene> model;     // the bound file, loaded and packed; null while it loads, is missing or does not load
    ::godot::Ref<::godot::Material>    stand_in;  // the error material of its name, drawn as a block while model is null
    FigureSceneLoad                    load;      // the task that parses and packs the file; stays at its address in the map
};

/**
 * The tools of the hand the stage names: every role hand.<item key> of `roles` keyed by name_id of the item key,
 * with the file `binding` holds for its asset and the error material of that asset. The loads are not started here.
 */
[[nodiscard]] std::unordered_map<uint32_t, FigureHandTool> bind_hand_tools(
    const VivariumAssetBinding& binding, const std::unordered_map<std::string, std::string>& roles);

/**
 * The kind of the piece the gear port hangs at `slot` (the socket bone's name, socket_hand_r) of `figure`;
 * 0 when the port carries no piece there.
 */
uint32_t hand_kind(uint32_t figure, const std::vector<VivariumFigureGearView>& gear, const char* slot);

/**
 * Draws the piece of `kind` below `socket`: an instance of its tool's file once `tools` holds it loaded, else a
 * block in the tool's error material; a kind `tools` has no role for is that block in `stand_in`. Keeps the drawn
 * piece while its kind and its source hold, rebuilds it when either changes, takes it away when `kind` is 0.
 */
void carry_tool(::godot::Node3D* socket, uint32_t kind, const std::unordered_map<uint32_t, FigureHandTool>& tools,
                const ::godot::Ref<::godot::Material>& stand_in);

/**
 * The body of the tool place as the asset list names it under the role rack: its asset, the bound file and the
 * error material of that asset, bound like a tool of the hand. The load is not started here.
 */
[[nodiscard]] FigureHandTool bind_rack_body(const VivariumAssetBinding& binding,
                                            const std::unordered_map<std::string, std::string>& roles);

/**
 * Stands the tool place and the tools on the ground below `scene`: the rack body at `rack`, its crossbar along
 * `along` (the right of the camera the place was laid by); every thing of `things` whose item `tools` names leans
 * at its slot (at_rack) or lies at its place, an instance of its file once loaded, else a block in its error
 * material. A thing that left the ground leaves the scene; a thing whose item has no tool is not drawn here.
 * Every thing it stands is marked bodied, so the ground of the HUD draws its shadow and hit box and not its
 * picture; true when the rack body stands.
 */
bool lay_tools(::godot::Node3D* scene, const ::godot::Vector3& rack, const ::godot::Vector3& along,
               const FigureHandTool& rack_body, std::vector<GroundThing>& things,
               const std::unordered_map<uint32_t, FigureHandTool>& tools);

/**
 * THE TAP ON THE TOOL PLACE IN THE WORLD (D01, PLAN_ASE_VIVARIUM_PHASE_06_HUD 8.6): true when the ray of `camera`
 * through the viewport point `at` meets the bounds of the rack body lay_tools stood below `scene`, the meshes of
 * its file or its block, in the space of the scene; false while no rack body stands or no camera draws.
 */
bool rack_struck(::godot::Node3D* scene, const ::godot::Camera3D* camera, const ::godot::Vector2& at);

}  // namespace ase::adp::godot

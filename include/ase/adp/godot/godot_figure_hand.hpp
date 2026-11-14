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
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101625 - carry a tool in hand
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.02.00001
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/godot_figure_load.hpp>
#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

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

}  // namespace ase::adp::godot

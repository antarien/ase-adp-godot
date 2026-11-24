#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM CAMP
 *
 * @file        godot_vivarium_camp.hpp
 * @brief       What the view holds of the camp fire between frames, and the reader of its start configuration
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.0, T.1, T.3, T.7: the camp fire is laid by the host, never by
 *              the view. The view sends the place once (ingle.camp.v1), the hull corners of every make once
 *              (ingle.hull.v1) and every part once on a later frame (ingle.part.v1); ase-physics drops, tips and
 *              rests the parts, and the view draws each body ase-physics reports (ingle.body.v1) with the mesh of its
 *              make. When ase-bdi sets a keeper to laying wood (ingle.refuel.v1), the view lays one new log as a part
 *              of its own on the body he tends, once per keeper and body while his action stands.
 *
 *              VivariumView::follow_camp (godot_vivarium_camp.cpp) works this state every frame;
 *              read_camp_config (godot_vivarium_camp_config.cpp) fills the start configuration once.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    ecs/module
 * @usecase     usecase:5436 - kindle fire
 * @actor       Funken (FNKN)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_camp_load.hpp>
#include <ase/adp/godot/godot_host_fire_view.hpp>
#include <ase/adp/godot/godot_host_start_config.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ase::adp::godot {

/** The camp fire as the view holds it between frames; one per view, cleared when the host starts anew. */
struct VivariumCampState {
    std::unordered_map<std::string, CampRoleLoad> loads;  // role → the load of its export; a node keeps its address while its task runs
    bool                     loads_started = false;  // every role of camp.parts was handed to its task
    bool                     camp_laid = false;      // the camp place went out on ingle.camp.v1
    bool                     hulls_filed = false;    // every hull corner of every make went out on ingle.hull.v1
    uint64_t                 hulls_taken = 0;        // corners the port took so far, in the order of the loads; a full queue resumes there on the next frame
    uint64_t                 hulls_refused = 0;      // corners the port refused for another reason than a full queue, over all frames
    bool                     parts_filed = false;    // every part of camp.parts went out on ingle.part.v1, a frame after the hulls
    std::unordered_map<uint32_t, ::godot::Ref<::godot::Mesh>> meshes;  // make → the mesh of its node, filled as each load is taken
    ::godot::Node3D*         root = nullptr;         // the node the bodies are drawn under, a child of the scene
    std::unordered_map<uint32_t, ::godot::MeshInstance3D*> drawn;  // body → its node, as ingle.body.v1 last showed it
    std::unordered_map<uint32_t, bool> framed;       // body → whether its place lay in the camera frustum when last noted
    std::unordered_map<uint32_t, ::godot::Node3D*> flames;  // fire cell → its flame: tongue, glow, smoke, sparks and ash bed
    std::unordered_map<uint32_t, uint32_t> flame_noted;     // fire cell → stage << 1 | framed, as its line last noted it
    ::godot::Ref<::godot::ShaderMaterial> flame_material;   // camp_flame.gdshader, loaded at the first fire cell
    bool                     flame_loaded = false;   // the load of camp_flame.gdshader was tried, once per view
    std::unordered_set<uint64_t> refuels_laid;       // keeper << 32 | body: the log laid for that action, while it stands
    uint32_t                 next_log = 0;           // the log variant the next refuel lays, in the order of the export
};

/**
 * Reads the five ports of ase-pl-ingle, the camp place (entry camp) and every part of the fire (camp.parts) from
 * the start file into `start`; load_patches ran before, the place is held against start.patches. Returns nullptr
 * when everything stands, else the key that is missing or malformed, for fail_config.
 */
const char* read_camp_config(const ::godot::Dictionary& root, VivariumStartConfig& start);

/**
 * A new host starts: nothing of the camp is laid in it yet, so the place, the hulls and the parts go out again, and
 * the bodies of the old host leave the scene. The loads stay: the exports of the stage are the same.
 */
void restart_camp(VivariumCampState& camp);

/** The view leaves the tree: waits for every load task that still runs, so the records may leave with it. */
void end_camp(VivariumCampState& camp);

/**
 * The flame of every fire cell of `fires` (ingle.fire.v1) over the drawn body it burns, shaped from its stage, heat
 * and fuel (godot_vivarium_camp_flame.cpp); a cell gone from the records takes its flame along. One line per cell
 * whose stage or frame in `eye` changed goes into `lines` for the view to note.
 */
void follow_camp_flames(VivariumCampState& camp, const std::vector<VivariumSurvivalFireView>& fires,
                        const ::godot::Camera3D* eye, std::vector<::godot::String>& lines);

}  // namespace ase::adp::godot

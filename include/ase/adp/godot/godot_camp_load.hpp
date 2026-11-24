#pragma once

/**
 * ASE GODOT ADAPTER - CAMP LOAD
 *
 * @file        godot_camp_load.hpp
 * @brief       The camp exports of the stage, parsed by a task of Godot's WorkerThreadPool into one make per variant
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.1, T.3, T.9: every part of the camp fire comes from a Blender
 *              recipe, exported as one glTF file per role (camp.stone, camp.log, camp.stick, camp.tinder). One node
 *              per variant, named by its make (camp_stone.<variant>), its origin the centre of mass; the node extras
 *              carry make, mass_kg, roughness_m (how deep the deepest valley of the surface lies beneath the hull,
 *              required: a part without it is not known) and hull - the corners of the convex hull in the axes of the file, which glTF
 *              writes as Godot's: x east, y up, z north. The task parses the file, reads the extras from the JSON
 *              of the document and keeps each variant's mesh; nothing of it enters the scene tree.
 *
 *              The host builds the collision form from the corners (ase-physics, never the plugin): the view files
 *              them on ingle.hull.v1 before the first part of that make, and draws every body ase-physics reports
 *              with the mesh of its make (godot_vivarium_camp.cpp).
 *
 *              THE TASK TOUCHES NOTHING OF THE SCENE TREE, as the figure load (godot_figure_load.hpp): the record
 *              stays at its address until the task completed; the view waits for a running task when it leaves.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    render/pipeline/stage
 * @usecase     usecase:5436 - kindle fire
 * @actor       Funken (FNKN)
 * @created     2026-11-21
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One recipe variant of a camp role as its export carries it: make, mass, hull corners and mesh. */
struct CampMake {
    std::string                 name;          // the make string of the node extras, camp_stone.<variant>
    uint32_t                    make = 0;      // its name id, the value the host ports carry
    uint32_t                    variant = 0;   // the suffix of the make, the index camp.parts names
    double                      mass_kg = 0.0; // the mass of the variant, kg (extras mass_kg)
    double                      roughness_m = 0.0; // depth of its surface beneath its convex hull, m (extras roughness_m)
    std::vector<double>         hull;          // the hull corners, three values each: east, up, north of the centre of mass, m
    ::godot::Ref<::godot::Mesh> mesh;          // the mesh of the node, its surfaces with the materials of the file
};

/**
 * The export of one camp role while a task of the WorkerThreadPool parses it. The task reads root and file and
 * writes the makes; the main thread reads them only after the task completed.
 */
struct CampRoleLoad {
    std::string           role;         // the role of the view, camp.stone
    ::godot::String       asset_root;   // the asset root the file stands below
    std::string           file;         // the bound file below it; empty when the stage binds none
    std::vector<CampMake> makes;        // written by the task: every node that carries make, mass_kg, roughness_m and hull
    uint32_t              refused = 0u; // written by the task: the nodes with a make string that lack another extra
    std::string           refusal;      // written by the task: the first of them and the extra it lacks, or why the file failed
    int64_t               task = -1;    // the WorkerThreadPool task while it runs or its makes were not taken; -1 when none runs
    bool                  taken = false; // the makes were taken once
};

/**
 * Hands the parse of `load.file` below `asset_root` to a task of the WorkerThreadPool; nothing when the file is
 * empty or a task runs. `load` stays at its address until the task completed (take_camp_load or end_camp_load).
 */
void start_camp_load(CampRoleLoad& load, const ::godot::String& asset_root);

/** On the frame the task completed: marks the makes taken and returns true; false while it runs or when none runs. */
bool take_camp_load(CampRoleLoad& load);

/** Waits for a task that still runs and drops its makes, so the record may leave; called when the view leaves the tree. */
void end_camp_load(CampRoleLoad& load);

}  // namespace ase::adp::godot

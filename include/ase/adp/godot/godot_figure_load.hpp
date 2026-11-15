#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE LOAD
 *
 * @file        godot_figure_load.hpp
 * @brief       The figure file of the stage, parsed and packed by a task of Godot's WorkerThreadPool
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.9: the figure asset of the stage (role `figure`,
 *              villager_body) is one glTF file of 169.6 MB. Parsed on the main thread inside the view's
 *              ready() it held the first frame for seconds before the start screen could draw. The parse and
 *              the pack run in a task of the WorkerThreadPool instead: ready() hands the task over and returns,
 *              the start screen draws and holds while the task runs, and the view takes the pack on the frame
 *              the task completed. From there the figure scene works as before: every figure node is an
 *              instance of the pack, and one instance warms its pipelines behind the start screen
 *              (godot_figure_scene.hpp).
 *
 *              THE TASK TOUCHES NOTHING OF THE SCENE TREE: it reads the file's path and name and writes the
 *              pack into the load record; the main thread reads the pack only after the pool reports the task
 *              completed. The generated node tree never enters the scene tree and is freed by the task once
 *              packed. The record must stay at its address until the task completed; the view holds it as a
 *              member and waits for a running task when it leaves the tree (end_figure_load).
 *
 *              THE START SCREEN COUNTS WHAT THE TASK HAS DONE (B0357): the task reads the file in blocks and
 *              stores the bytes read after each one, then counts the parse, the scene and the pack as one
 *              step each. figure_load_share turns these counters into the share of the progress bar; the
 *              counters are plain fields read and written with atomic builtins, because the record is copied
 *              with FigureSceneLook on the main thread before a task runs.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101611 - animate character motion
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/**
 * The figure file while a task of the WorkerThreadPool parses and packs it. The task reads root and file and
 * writes the pack; the main thread reads the pack only after the task completed.
 */
struct FigureSceneLoad {
    ::godot::String                    asset_root;      // the asset root the file stands below
    std::string                        file;            // the bound file below it
    ::godot::Ref<::godot::PackedScene> model;           // written by the task: the parsed file packed, null when it does not load
    int64_t                            task = -1;       // the WorkerThreadPool task while it runs or its pack was not taken; -1 when none runs
    int64_t                            bytes_total = 0; // written by the task once the file is open: its length; read by the main thread while the task runs
    int64_t                            bytes_read = 0;  // written by the task after every block it read; read by the main thread while the task runs
    int64_t                            phases_done = 0; // written by the task after parse, scene and pack, one each; read by the main thread while the task runs
    int64_t                            read_usec = 0;   // written by the task: the time the blocks took; read after the task completed
    int64_t                            parse_usec = 0;  // written by the task: the time the parse took, texture packs included
    int64_t                            scene_usec = 0;  // written by the task: the time generate_scene took
    int64_t                            pack_usec = 0;   // written by the task: the time the pack took
};

/**
 * Hands the parse and pack of `file` below `asset_root` to a task of the WorkerThreadPool; nothing when `file` is
 * empty or a task runs. `load` stays at its address until the task completed (take_figure_load or
 * end_figure_load): the task writes into it.
 */
void start_figure_load(FigureSceneLoad& load, const ::godot::String& asset_root, const std::string& file);

/** True while the task of start_figure_load runs or its pack was not taken yet. */
bool figure_load_running(const FigureSceneLoad& load);

/**
 * On the frame the task completed: hands its pack to `model` (null when the file does not load) and returns
 * true; false while it runs or when none runs, and `model` stays as it is.
 */
bool take_figure_load(FigureSceneLoad& load, ::godot::Ref<::godot::PackedScene>& model);

/** Waits for a task that still runs and drops its pack, so the record may leave; called when the view leaves the tree. */
void end_figure_load(FigureSceneLoad& load);

/**
 * The share of the load the task has done, 0..1: every block of the file it has read counts one, and the
 * parse, the scene and the pack count one each. 1 when no task runs; 0 while the file is not open yet.
 */
double figure_load_share(const FigureSceneLoad& load);

/** The time each step of the last completed task took, as a line for the log; read after take_figure_load. */
::godot::String figure_load_times(const FigureSceneLoad& load);

}  // namespace ase::adp::godot

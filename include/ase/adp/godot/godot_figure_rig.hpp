#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE RIG
 *
 * @file        godot_figure_rig.hpp
 * @brief       The rig of the figure file: its clips mixed by one AnimationTree, and the height of its eyes
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.6, 07.9 and 07.14: an instance of the figure file
 *              (villager_body) carries a Skeleton3D and an AnimationPlayer with the clips idle, walk, run and
 *              turn, from version 2 on also bend, the bend at the brook played once per draw. The figure
 *              scene (godot_figure_scene.hpp) builds one node per figure and hands its instance here twice:
 *              once when the node is built, so an AnimationTree beside the player mixes the clips and the
 *              eyes of the rig are measured, and every frame, so the mix follows the gait of the body port and
 *              the open course of the turn on the spot.
 *
 *              THE POSES AT THE CAMP FIRE (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.11, usecase:2157 - go to Sleep):
 *              from version 3 on the file also carries sit_ground, sit_stone, sit_down, sit_down_stone,
 *              sleep_back, sleep_side and lie_down (villager_body.json build.clips, scripts/figure.py:clip_pose).
 *              The root of the mix is a state machine whose state stand holds the gait mix; a resting figure
 *              sits down onto the ground or a ring stone and back up by the same clip played backward, a
 *              sleeping one lies down from the seat and wakes back into it. The pose is chosen from what
 *              ase-metabolism writes (MTB_IS_RESTING, MTB_IS_SLEEPING), never by the adapter: a figure that
 *              sleeps always lies, none sleeps standing.
 *
 *              THE EYES COME FROM THE REST POSE OF THE SKELETON. The bones of the Character Creator rig are
 *              turned against each other, so the sum of the file's bone offsets puts the feet a metre above the
 *              soles; only the global rest of L_Eye and R_Eye in the skeleton's space, carried through the
 *              nodes up to the model, gives the height the face landing of ase-camera aims at.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101611 - animate character motion
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-22
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_pose.hpp>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

#include <cstdint>

namespace ase::adp::godot {

/**
 * The motion of the figure file: its four clips loop, and an AnimationTree beside its AnimationPlayer mixes
 * them - idle, walk and run at the gait points the code port names (`gait_stand`, `gait_walk`, `gait_run`, a
 * blend space from the stand point to the run point), the turn clip over them by the open course on the spot,
 * and, where the file carries the bend clip, a OneShot that plays it once over all of them. All of it is the
 * state stand of a state machine; where the file carries the seven clips of the camp fire, the states of the
 * seat and the sleep stand beside it (T.11). false when the file carries no AnimationPlayer or not all four
 * looping clips.
 */
bool add_figure_motion(::godot::Node* model, double gait_stand, double gait_walk, double gait_run);

/**
 * Drives the mix of the figure node `body` in this frame: `gait` as the body port carries it, bounded to the
 * range from `gait_stand` to `gait_run` the code port names, `turning` the share of the turn clip. `bend` true
 * fires the bend clip once over the mix (07.14, the figure draws water at the brook); a bend still playing is
 * not fired again, and a mix built from a file without the bend clip ignores it. `pose` (FIGURE_POSE_*) is the
 * state the machine travels to, through sitting down, lying down, waking and standing up; a mix without the
 * camp clips stays in stand. Nothing when the node carries no mix.
 */
void drive_figure_motion(::godot::Node3D* body, double gait, double gait_stand, double gait_run, double turning,
                         bool bend, uint8_t pose);

/**
 * The pose (FIGURE_POSE_*) the mix of the figure node `body` last travelled to, as drive_figure_motion noted it on the
 * AnimationTree: the pose the figure is drawn in, never the one the rest record asks for. false when the node carries
 * no mix with the camp clips (B0783: the pose line of the view and the camp pictures of the acceptance read it).
 */
bool figure_motion_pose(::godot::Node3D* body, uint8_t& pose);

/**
 * The height of the eyes of the figure file `model` in the frame of its parent, metres: the mean of the rest
 * origins of L_Eye and R_Eye. false when the file carries no Skeleton3D or its skeleton lacks an eye bone.
 */
bool figure_eye_level(::godot::Node* model, double& eye);

/**
 * The radius of the capsule around the whole body of the figure file `model`, metres: half the horizontal distance
 * between the rest origins of L_Upperarm and R_Upperarm, the shoulder joints, in the frame of its parent. ase-physics
 * collides the walking body as that one capsule, never as one shape per bone. false when the file carries no
 * Skeleton3D or its skeleton lacks a shoulder bone.
 */
bool figure_body_radius(::godot::Node* model, double& radius);

}  // namespace ase::adp::godot

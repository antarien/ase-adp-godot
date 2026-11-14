#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE RIG
 *
 * @file        godot_figure_rig.hpp
 * @brief       The rig of the figure file: its four clips mixed by one AnimationTree, and the height of its eyes
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.6 and 07.9: an instance of the figure file (villager_body)
 *              carries a Skeleton3D and an AnimationPlayer with the clips idle, walk, run and turn. The figure
 *              scene (godot_figure_scene.hpp) builds one node per figure and hands its instance here twice:
 *              once when the node is built, so an AnimationTree beside the player mixes the clips and the
 *              eyes of the rig are measured, and every frame, so the mix follows the gait of the body port and
 *              the open course of the turn on the spot.
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
 * @modified    2026-11-14
 * @version     00.00.01.00001
 */

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace ase::adp::godot {

/**
 * The motion of the figure file: its four clips loop, and an AnimationTree beside its AnimationPlayer mixes
 * them - idle, walk and run at the gait points the code port names (`gait_stand`, `gait_walk`, `gait_run`, a
 * blend space from the stand point to the run point), the turn clip over them by the open course on the spot.
 * false when the file carries no AnimationPlayer or not all four clips.
 */
bool add_figure_motion(::godot::Node* model, double gait_stand, double gait_walk, double gait_run);

/**
 * Drives the mix of the figure node `body` in this frame: `gait` as the body port carries it, bounded to the
 * range from `gait_stand` to `gait_run` the code port names, `turning` the share of the turn clip. Nothing when
 * the node carries no mix.
 */
void drive_figure_motion(::godot::Node3D* body, double gait, double gait_stand, double gait_run, double turning);

/**
 * The height of the eyes of the figure file `model` in the frame of its parent, metres: the mean of the rest
 * origins of L_Eye and R_Eye. false when the file carries no Skeleton3D or its skeleton lacks an eye bone.
 */
bool figure_eye_level(::godot::Node* model, double& eye);

}  // namespace ase::adp::godot

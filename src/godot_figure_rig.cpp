/**
 * ASE GODOT ADAPTER - FIGURE RIG IMPLEMENTATION
 *
 * @file        godot_figure_rig.cpp
 * @brief       Mixes the clips of the figure file by one AnimationTree and measures the eyes of its rig
 * @description See godot_figure_rig.hpp. The AnimationTree stands beside the AnimationPlayer of the instance, so
 *              both resolve the tracks from the same root; its blend space mixes idle, walk and run by the gait,
 *              a Blend2 lays the turn clip over them, and a OneShot plays the bend clip once over the whole
 *              mix when the figure draws water at the brook (07.14): the body bends through the joints of its
 *              rig, hips, knees and spine as ase-assgen keyed them. All of it is the state stand of a state
 *              machine at the root; a file with the clips of the camp fire (T.11) adds the seat on the ground and
 *              on a ring stone, reached by sit_down and sit_down_stone and left by the same clips played backward,
 *              and the sleep on the back, reached from the ground seat by lie_down and left by it backward, with
 *              the sleep on the side beside it, rolled into from the back and back onto it before waking. The
 *              eyes are measured once from the rest pose of the skeleton, never from the posed bones of a clip.
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

#include <ase/adp/godot/godot_figure_rig.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_node_animation.hpp>
#include <godot_cpp/classes/animation_node_blend2.hpp>
#include <godot_cpp/classes/animation_node_blend_space1_d.hpp>
#include <godot_cpp/classes/animation_node_blend_tree.hpp>
#include <godot_cpp/classes/animation_node_one_shot.hpp>
#include <godot_cpp/classes/animation_node_state_machine.hpp>
#include <godot_cpp/classes/animation_node_state_machine_playback.hpp>
#include <godot_cpp/classes/animation_node_state_machine_transition.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/**
 * THE CLIPS, THE MIX AND THE EYE BONES OF THE FIGURE FILE. Only this unit builds the mix and reads the skeleton,
 * so the names stand here and not in types.hpp, which nearly every unit of the adapter includes.
 */
constexpr const char* kFigureMotion = "FigureMotion";       // name of the AnimationTree that mixes the clips of one figure
constexpr const char* kFigureClipIdle = "idle";             // the clips of villager_body (ase-assgen recipe), mixed in this order
constexpr const char* kFigureClipWalk = "walk";
constexpr const char* kFigureClipRun = "run";
constexpr const char* kFigureClipTurn = "turn";
constexpr const char* kFigureClipBend = "bend";             // the bend at the brook, played once, never looped (villager_body version 2)
constexpr const char* kFigureParamMove = "parameters/stand/move/blend_position";  // gait of the clip mix, at the points of the code port
constexpr const char* kFigureParamTurn = "parameters/stand/mix/blend_amount";     // share of the turn clip in the mix
constexpr const char* kFigureParamBend = "parameters/stand/bend/request";         // fires the bend once over the mix
constexpr const char* kFigureParamBendActive = "parameters/stand/bend/active";    // true while the bend plays
constexpr const char* kFigureParamPlayback = "parameters/playback";               // the playback of the pose machine
constexpr const char* kFigureStateStart = "Start";          // the entry node every root state machine of Godot carries
constexpr const char* kFigureStateStand = "stand";          // the gait mix with turn and bend
constexpr const char* kFigureClipSitGround = "sit_ground";  // the camp fire clips of villager_body (T.11): the held seats
constexpr const char* kFigureClipSitStone = "sit_stone";
constexpr const char* kFigureClipSitDown = "sit_down";      // standing onto the ground seat, backward back up
constexpr const char* kFigureClipSitDownStone = "sit_down_stone";  // standing onto the stone seat, backward back up
constexpr const char* kFigureClipSleepBack = "sleep_back";  // the held sleep, breathing
constexpr const char* kFigureClipSleepSide = "sleep_side";  // the held sleep on the side, rolled into from the back
constexpr const char* kFigureClipLieDown = "lie_down";      // the ground seat into the sleep, backward awake again
constexpr const char* kFigureStateStandUp = "stand_up";     // sit_down played backward
constexpr const char* kFigureStateStandUpStone = "stand_up_stone";  // sit_down_stone played backward
constexpr const char* kFigureStateWake = "wake";            // lie_down played backward
constexpr const char* kFigureMetaPoses = "ase_poses";       // on the AnimationTree: the machine carries the camp states
constexpr const char* kFigureMetaPose = "ase_pose";         // on the AnimationTree: the pose last travelled to
constexpr float kFigurePoseFade = 0.25f;                    // seconds one pose state fades into the next
constexpr float kFigureBendFadeIn = 0.15f;                  // seconds the bend takes to lay over the mix
constexpr float kFigureBendFadeOut = 0.2f;                  // seconds the mix takes to return after the bend
constexpr const char* kFigureBoneEyeLeft = "L_Eye";         // the eye bones of the rig of villager_body (Character Creator skeleton)
constexpr const char* kFigureBoneEyeRight = "R_Eye";
constexpr float kFigureEyeMean = 0.5f;                      // the eyes stand at the mean of both eye bones
constexpr const char* kFigureBoneShoulderLeft = "L_Upperarm";   // the shoulder joints of the rig of villager_body (Character Creator skeleton)
constexpr const char* kFigureBoneShoulderRight = "R_Upperarm";
constexpr double kFigureRadiusShare = 0.5;                  // the capsule's radius is half the span between both shoulder joints

/** The first Skeleton3D below the figure file `model`; nullptr when the file carries none. */
::godot::Skeleton3D* rig_skeleton(::godot::Node* model) {
    const ::godot::TypedArray<::godot::Node> skeletons = model->find_children("*", "Skeleton3D", true, false);
    if (skeletons.is_empty()) {
        return nullptr;
    }
    return ::godot::Object::cast_to<::godot::Skeleton3D>(skeletons[0]);
}

/** Every Node3D from `skeleton` up to `model`, the model's own transform included: the frame of the model's parent. */
::godot::Transform3D rig_frame(::godot::Skeleton3D* skeleton, ::godot::Node* model) {
    ::godot::Transform3D frame;
    for (::godot::Node* node = skeleton; node != nullptr; node = node->get_parent()) {
        if (auto* spatial = ::godot::Object::cast_to<::godot::Node3D>(node)) {
            frame = spatial->get_transform() * frame;
        }
        if (node == model) {
            break;
        }
    }
    return frame;
}

/** One clip node of the mix. */
::godot::Ref<::godot::AnimationNodeAnimation> clip_node(const char* clip) {
    ::godot::Ref<::godot::AnimationNodeAnimation> node;
    node.instantiate();
    node->set_animation(::godot::StringName(clip));
    return node;
}

/** One clip node played backward: standing up and waking run the clip of sitting and lying down in reverse. */
::godot::Ref<::godot::AnimationNodeAnimation> reversed_clip_node(const char* clip) {
    ::godot::Ref<::godot::AnimationNodeAnimation> node = clip_node(clip);
    node->set_play_mode(::godot::AnimationNodeAnimation::PLAY_MODE_BACKWARD);
    return node;
}

/**
 * One transition of the pose machine. `at_end` holds the state until its clip played out (sitting down, lying
 * down, standing up, waking), and `automatic` advances it without a travel asking for it; a travel takes every
 * transition on its path, since each is enabled.
 */
void pose_transition(::godot::AnimationNodeStateMachine* machine, const char* from, const char* to, bool at_end,
                     bool automatic) {
    ::godot::Ref<::godot::AnimationNodeStateMachineTransition> transition;
    transition.instantiate();
    transition->set_xfade_time(kFigurePoseFade);
    transition->set_switch_mode(at_end ? ::godot::AnimationNodeStateMachineTransition::SWITCH_MODE_AT_END
                                       : ::godot::AnimationNodeStateMachineTransition::SWITCH_MODE_IMMEDIATE);
    transition->set_advance_mode(automatic ? ::godot::AnimationNodeStateMachineTransition::ADVANCE_MODE_AUTO
                                           : ::godot::AnimationNodeStateMachineTransition::ADVANCE_MODE_ENABLED);
    machine->add_transition(::godot::StringName(from), ::godot::StringName(to), transition);
}

/** true when the AnimationPlayer `player` carries every clip of the camp fire the pose machine plays. */
bool carries_camp_clips(::godot::AnimationPlayer* player) {
    const char* clips[] = {kFigureClipSitGround, kFigureClipSitStone, kFigureClipSitDown,
                           kFigureClipSitDownStone, kFigureClipSleepBack, kFigureClipSleepSide,
                           kFigureClipLieDown};
    for (const char* clip : clips) {
        if (!player->has_animation(::godot::StringName(clip))) {
            return false;
        }
    }
    return true;
}

/**
 * The seat on the ground, the seat on a ring stone and the sleep beside stand (T.11). The held poses loop - the
 * figure breathes while it sits and sleeps - and the passages between them play once. Sitting and lying down end
 * in the held pose by themselves; standing up ends in stand, waking in the ground seat. The sleep is reached only
 * from the ground seat, so a figure that falls asleep sits down first and never lies down from standing. The sleep
 * on the side is reached only from the sleep on the back, the mix rolling the body over in the fade, and left the
 * same way, so a figure always wakes from its back.
 */
void add_camp_poses(::godot::AnimationPlayer* player, ::godot::AnimationNodeStateMachine* machine) {
    for (const char* held : {kFigureClipSitGround, kFigureClipSitStone, kFigureClipSleepBack, kFigureClipSleepSide}) {
        player->get_animation(::godot::StringName(held))->set_loop_mode(::godot::Animation::LOOP_LINEAR);
    }
    for (const char* passage : {kFigureClipSitDown, kFigureClipSitDownStone, kFigureClipLieDown}) {
        player->get_animation(::godot::StringName(passage))->set_loop_mode(::godot::Animation::LOOP_NONE);
    }
    machine->add_node(kFigureClipSitDown, clip_node(kFigureClipSitDown));
    machine->add_node(kFigureClipSitGround, clip_node(kFigureClipSitGround));
    machine->add_node(kFigureStateStandUp, reversed_clip_node(kFigureClipSitDown));
    machine->add_node(kFigureClipSitDownStone, clip_node(kFigureClipSitDownStone));
    machine->add_node(kFigureClipSitStone, clip_node(kFigureClipSitStone));
    machine->add_node(kFigureStateStandUpStone, reversed_clip_node(kFigureClipSitDownStone));
    machine->add_node(kFigureClipLieDown, clip_node(kFigureClipLieDown));
    machine->add_node(kFigureClipSleepBack, clip_node(kFigureClipSleepBack));
    machine->add_node(kFigureClipSleepSide, clip_node(kFigureClipSleepSide));
    machine->add_node(kFigureStateWake, reversed_clip_node(kFigureClipLieDown));

    pose_transition(machine, kFigureStateStand, kFigureClipSitDown, false, false);
    pose_transition(machine, kFigureClipSitDown, kFigureClipSitGround, true, true);
    pose_transition(machine, kFigureClipSitGround, kFigureStateStandUp, false, false);
    pose_transition(machine, kFigureStateStandUp, kFigureStateStand, true, true);
    pose_transition(machine, kFigureStateStand, kFigureClipSitDownStone, false, false);
    pose_transition(machine, kFigureClipSitDownStone, kFigureClipSitStone, true, true);
    pose_transition(machine, kFigureClipSitStone, kFigureStateStandUpStone, false, false);
    pose_transition(machine, kFigureStateStandUpStone, kFigureStateStand, true, true);
    pose_transition(machine, kFigureClipSitGround, kFigureClipLieDown, false, false);
    pose_transition(machine, kFigureClipLieDown, kFigureClipSleepBack, true, true);
    pose_transition(machine, kFigureClipSleepBack, kFigureStateWake, false, false);
    pose_transition(machine, kFigureStateWake, kFigureClipSitGround, true, true);
    pose_transition(machine, kFigureClipSleepBack, kFigureClipSleepSide, false, false);
    pose_transition(machine, kFigureClipSleepSide, kFigureClipSleepBack, false, false);
}

/** The state of the pose machine that holds `pose` (FIGURE_POSE_*). */
const char* pose_state(uint8_t pose) {
    if (pose == FIGURE_POSE_SIT_GROUND) {
        return kFigureClipSitGround;
    }
    if (pose == FIGURE_POSE_SIT_STONE) {
        return kFigureClipSitStone;
    }
    if (pose == FIGURE_POSE_SLEEP) {
        return kFigureClipSleepBack;
    }
    if (pose == FIGURE_POSE_SLEEP_SIDE) {
        return kFigureClipSleepSide;
    }
    return kFigureStateStand;
}

}  // namespace

bool add_figure_motion(::godot::Node* model, double gait_stand, double gait_walk, double gait_run) {
    const ::godot::TypedArray<::godot::Node> players = model->find_children("*", "AnimationPlayer", true, false);
    if (players.is_empty()) {
        return false;
    }
    auto* player = ::godot::Object::cast_to<::godot::AnimationPlayer>(players[0]);
    const char* clips[] = {kFigureClipIdle, kFigureClipWalk, kFigureClipRun, kFigureClipTurn};
    for (const char* clip : clips) {
        if (player == nullptr || !player->has_animation(::godot::StringName(clip))) {
            return false;
        }
        player->get_animation(::godot::StringName(clip))->set_loop_mode(::godot::Animation::LOOP_LINEAR);
    }

    ::godot::Ref<::godot::AnimationNodeBlendSpace1D> move;
    move.instantiate();
    move->set_min_space(static_cast<float>(gait_stand));
    move->set_max_space(static_cast<float>(gait_run));
    move->add_blend_point(clip_node(kFigureClipIdle), static_cast<float>(gait_stand));
    move->add_blend_point(clip_node(kFigureClipWalk), static_cast<float>(gait_walk));
    move->add_blend_point(clip_node(kFigureClipRun), static_cast<float>(gait_run));
    ::godot::Ref<::godot::AnimationNodeBlend2> mix;
    mix.instantiate();
    ::godot::Ref<::godot::AnimationNodeBlendTree> root;
    root.instantiate();
    root->add_node("move", move);
    root->add_node("turn", clip_node(kFigureClipTurn));
    root->add_node("mix", mix);
    root->connect_node("mix", 0, "move");
    root->connect_node("mix", 1, "turn");
    // the bend lies over the whole mix once per draw; a file of version 1 carries no bend clip, and its figure
    // keeps the four looping clips without the node, so the approved stage asset still moves
    if (player->has_animation(::godot::StringName(kFigureClipBend))) {
        player->get_animation(::godot::StringName(kFigureClipBend))->set_loop_mode(::godot::Animation::LOOP_NONE);
        ::godot::Ref<::godot::AnimationNodeOneShot> bend;
        bend.instantiate();
        bend->set_fadein_time(kFigureBendFadeIn);
        bend->set_fadeout_time(kFigureBendFadeOut);
        root->add_node("bend", bend);
        root->add_node("bend_clip", clip_node(kFigureClipBend));
        root->connect_node("bend", 0, "mix");
        root->connect_node("bend", 1, "bend_clip");
        root->connect_node("output", 0, "bend");
    } else {
        root->connect_node("output", 0, "mix");
    }

    // the gait mix is the state stand of the pose machine; the machine enters it at once, and the camp states
    // stand beside it where the file carries their clips (T.11)
    ::godot::Ref<::godot::AnimationNodeStateMachine> machine;
    machine.instantiate();
    machine->add_node(kFigureStateStand, root);
    pose_transition(machine.ptr(), kFigureStateStart, kFigureStateStand, false, true);
    const bool poses = carries_camp_clips(player);
    if (poses) {
        add_camp_poses(player, machine.ptr());
    }

    // beside the player, so both resolve the tracks from the same root (their default root node "..")
    ::godot::AnimationTree* motion = memnew(::godot::AnimationTree);
    motion->set_name(kFigureMotion);
    motion->set_tree_root(machine);
    motion->set_animation_player(::godot::NodePath(::godot::String("../") + ::godot::String(player->get_name())));
    motion->set_meta(kFigureMetaPoses, poses);
    motion->set_meta(kFigureMetaPose, static_cast<int64_t>(FIGURE_POSE_STAND));
    player->get_parent()->add_child(motion);
    motion->set_active(true);
    return true;
}

void drive_figure_motion(::godot::Node3D* body, double gait, double gait_stand, double gait_run, double turning,
                         bool bend, uint8_t pose) {
    const ::godot::TypedArray<::godot::Node> trees = body->find_children(kFigureMotion, "AnimationTree", true, false);
    if (trees.is_empty()) {
        return;
    }
    auto* motion = ::godot::Object::cast_to<::godot::AnimationTree>(trees[0]);
    if (motion == nullptr) {
        return;
    }
    motion->set(kFigureParamMove, ase::math::clamp(gait, gait_stand, gait_run));
    motion->set(kFigureParamTurn, turning);
    // a draw while the bend still plays adds no second bend: the figure is already down at the water
    if (bend && motion->get(kFigureParamBendActive).get_type() == ::godot::Variant::BOOL &&
        !static_cast<bool>(motion->get(kFigureParamBendActive))) {
        motion->set(kFigureParamBend, static_cast<int64_t>(::godot::AnimationNodeOneShot::ONE_SHOT_REQUEST_FIRE));
    }

    // the pose (T.11): travelled to once when it changes; the machine plays the passages on the way itself, so a
    // figure seated on the ground that falls asleep lies down from its seat, and one that falls asleep on a stone
    // stands up, sits down on the ground and lies down from there
    if (!motion->has_meta(kFigureMetaPoses) || !static_cast<bool>(motion->get_meta(kFigureMetaPoses))) {
        return;
    }
    if (static_cast<int64_t>(motion->get_meta(kFigureMetaPose)) == static_cast<int64_t>(pose)) {
        return;
    }
    auto* playback = ::godot::Object::cast_to<::godot::AnimationNodeStateMachinePlayback>(
        static_cast<::godot::Object*>(motion->get(kFigureParamPlayback)));
    if (playback == nullptr) {
        return;
    }
    playback->travel(::godot::StringName(pose_state(pose)));
    motion->set_meta(kFigureMetaPose, static_cast<int64_t>(pose));
}

bool figure_eye_level(::godot::Node* model, double& eye) {
    ::godot::Skeleton3D* skeleton = rig_skeleton(model);
    if (skeleton == nullptr) {
        return false;
    }
    const int32_t left = skeleton->find_bone(kFigureBoneEyeLeft);
    const int32_t right = skeleton->find_bone(kFigureBoneEyeRight);
    if (left < 0 || right < 0) {
        return false;
    }
    const ::godot::Transform3D frame = rig_frame(skeleton, model);
    const ::godot::Vector3 eyes =
        (skeleton->get_bone_global_rest(left).origin + skeleton->get_bone_global_rest(right).origin) * kFigureEyeMean;
    eye = static_cast<double>(frame.xform(eyes).y);
    return true;
}

bool figure_body_radius(::godot::Node* model, double& radius) {
    ::godot::Skeleton3D* skeleton = rig_skeleton(model);
    if (skeleton == nullptr) {
        return false;
    }
    const int32_t left = skeleton->find_bone(kFigureBoneShoulderLeft);
    const int32_t right = skeleton->find_bone(kFigureBoneShoulderRight);
    if (left < 0 || right < 0) {
        return false;
    }
    // both joints in the frame of the model's parent; only the horizontal span counts, the capsule stands upright
    const ::godot::Transform3D frame = rig_frame(skeleton, model);
    const ::godot::Vector3 span = frame.xform(skeleton->get_bone_global_rest(left).origin) -
                                  frame.xform(skeleton->get_bone_global_rest(right).origin);
    radius = kFigureRadiusShare * static_cast<double>(ase::math::sqrt(span.x * span.x + span.z * span.z));
    return true;
}

}  // namespace ase::adp::godot

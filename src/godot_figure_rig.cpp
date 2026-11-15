/**
 * ASE GODOT ADAPTER - FIGURE RIG IMPLEMENTATION
 *
 * @file        godot_figure_rig.cpp
 * @brief       Mixes the four clips of the figure file by one AnimationTree and measures the eyes of its rig
 * @description See godot_figure_rig.hpp. The AnimationTree stands beside the AnimationPlayer of the instance, so
 *              both resolve the tracks from the same root; its blend space mixes idle, walk and run by the gait,
 *              a Blend2 lays the turn clip over them. The eyes are measured once from the rest pose of the
 *              skeleton, never from the posed bones of a clip.
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

#include <ase/adp/godot/godot_figure_rig.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_node_animation.hpp>
#include <godot_cpp/classes/animation_node_blend2.hpp>
#include <godot_cpp/classes/animation_node_blend_space1_d.hpp>
#include <godot_cpp/classes/animation_node_blend_tree.hpp>
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
constexpr const char* kFigureParamMove = "parameters/move/blend_position";  // gait of the clip mix, at the points of the code port
constexpr const char* kFigureParamTurn = "parameters/mix/blend_amount";     // share of the turn clip in the mix
constexpr const char* kFigureBoneEyeLeft = "L_Eye";         // the eye bones of the rig of villager_body (Character Creator skeleton)
constexpr const char* kFigureBoneEyeRight = "R_Eye";
constexpr float kFigureEyeMean = 0.5f;                      // the eyes stand at the mean of both eye bones

/** One clip node of the mix. */
::godot::Ref<::godot::AnimationNodeAnimation> clip_node(const char* clip) {
    ::godot::Ref<::godot::AnimationNodeAnimation> node;
    node.instantiate();
    node->set_animation(::godot::StringName(clip));
    return node;
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
    root->connect_node("output", 0, "mix");

    // beside the player, so both resolve the tracks from the same root (their default root node "..")
    ::godot::AnimationTree* motion = memnew(::godot::AnimationTree);
    motion->set_name(kFigureMotion);
    motion->set_tree_root(root);
    motion->set_animation_player(::godot::NodePath(::godot::String("../") + ::godot::String(player->get_name())));
    player->get_parent()->add_child(motion);
    motion->set_active(true);
    return true;
}

void drive_figure_motion(::godot::Node3D* body, double gait, double gait_stand, double gait_run, double turning) {
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
}

bool figure_eye_level(::godot::Node* model, double& eye) {
    const ::godot::TypedArray<::godot::Node> skeletons = model->find_children("*", "Skeleton3D", true, false);
    if (skeletons.is_empty()) {
        return false;
    }
    auto* skeleton = ::godot::Object::cast_to<::godot::Skeleton3D>(skeletons[0]);
    if (skeleton == nullptr) {
        return false;
    }
    const int32_t left = skeleton->find_bone(kFigureBoneEyeLeft);
    const int32_t right = skeleton->find_bone(kFigureBoneEyeRight);
    if (left < 0 || right < 0) {
        return false;
    }
    // every Node3D from the skeleton up to the model, the model's own transform included: the frame of its parent
    ::godot::Transform3D frame;
    for (::godot::Node* node = skeleton; node != nullptr; node = node->get_parent()) {
        if (auto* spatial = ::godot::Object::cast_to<::godot::Node3D>(node)) {
            frame = spatial->get_transform() * frame;
        }
        if (node == model) {
            break;
        }
    }
    const ::godot::Vector3 eyes =
        (skeleton->get_bone_global_rest(left).origin + skeleton->get_bone_global_rest(right).origin) * kFigureEyeMean;
    eye = static_cast<double>(frame.xform(eyes).y);
    return true;
}

}  // namespace ase::adp::godot

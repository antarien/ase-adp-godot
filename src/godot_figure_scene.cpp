/**
 * ASE GODOT ADAPTER - FIGURE SCENE IMPLEMENTATION
 *
 * @file        godot_figure_scene.cpp
 * @brief       Places, turns, animates and details the figures of the body port; aims the camera at the player camera's eye
 * @description See godot_figure_scene.hpp. The figures stand under one node of the scene, one child per figure
 *              named by its object id; place, course, gait and detail bands of every child come each frame
 *              from the body port. The bound figure file is parsed and packed once by a task of the
 *              WorkerThreadPool (godot_figure_load.hpp); no figure node is built before the view took the
 *              pack. Every new figure is an instance of the pack: its clips
 *              idle, walk, run and turn loop and are mixed by one AnimationTree, the bend clip plays once over
 *              them when the vessel in its hand gains water at the brook, and the height of its eyes is
 *              measured from the rest pose of its skeleton (godot_figure_rig.hpp), and its meshes are marked
 *              with the detail level their name carries (villager_body_lod<k>_<part>); a BoneAttachment3D
 *              under its Skeleton3D follows the socket bone socket_hand_r, and the tool the gear port hangs
 *              in that hand is drawn below it (godot_figure_hand.hpp). One instance of the
 *              pack stands in the scene while the start screen stands, so its pipelines compile under it.
 *              While the stage carries no file the child is a capsule of the figure's height in the error
 *              material of its name with a block on its front and a hand socket at its right side.
 *
 *              The view reads height, rig, place and face of a built node back through the figure queries
 *              (godot_figure_query.cpp); both units reach the node by the names of godot_figure_node.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101611 - animate character motion
 * @actor       SS/PC
 * @created     2026-11-13
 * @modified    2026-11-22
 * @version     00.00.26.00026 [refine]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_scene.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/godot_figure_hand.hpp>
#include <ase/adp/godot/godot_figure_node.hpp>
#include <ase/adp/godot/godot_figure_rig.hpp>
#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/bone_attachment3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/capsule_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

/**
 * THE STAND-IN BODY, THE WARM INSTANCE, THE SOCKET AND THE MOTION OF THE BODY. Only this unit builds them, so the
 * names and measures stand here and not in types.hpp, which nearly every unit of the adapter includes; the names
 * of the figure's node, which the figure queries read too, stand in godot_figure_node.hpp.
 */
constexpr const char* kFigureRole = "figure";              // the role of the stage the figure asset is named under (vivarium_assets.json)
constexpr const char* kFigureWarm = "FigureWarm";           // the instance of the figure file that stands behind the start screen while its pipelines compile
constexpr const char* kFigureSocketBone = "socket_hand_r";  // the socket bone of the rig of villager_body (ase-assgen recipe), the right hand
constexpr const char* kFigureSocket = "FigureSocketHandR";  // name of the BoneAttachment3D that follows the socket bone; what the hand carries hangs below it
constexpr const char* kFigureLodMark = "_lod";              // a mesh name carries its level as _lod<k>_ (villager_body_lod0_Tunic)
constexpr int32_t kFigureLodLevels = 4;                    // detail levels of the figure file, 0 the finest
constexpr float kFigureStandInRadius = 0.3f;                // stand-in body: radius of the capsule, metres
constexpr float kFigureStandInHeight = 1.8f;                // stand-in body: height of the capsule, metres, the height of a grown villager
constexpr float kFigureFrontEdge = 0.18f;                   // stand-in body: edge of the block on its front, metres
constexpr float kFigureFrontLift = 1.5f;                    // stand-in body: height of the block's centre above the feet, metres
constexpr double kFigureDistMin = 1.0e-3;                   // camera closer than this (metres) draws level 0
constexpr double kFigureAimMinSquared = 1.0e-6;             // eye and aim closer than this (squared metres) give no direction to look along
constexpr float kFigureHandSide = 0.36f;                    // stand-in body: its hand socket beside the capsule on its right (+x), metres
constexpr float kFigureHandLift = 0.95f;                    // stand-in body: height of its hand socket above the feet, metres
constexpr float kFigureModelTurn = ase::math::PI;           // turn of the figure file around y: glTF faces +z, course 0 of ase-player looks along -z
constexpr double kFigureWaterRise = 1.0e-3;                 // litres the vessel must gain between two snapshots to count as a draw, above float noise of the port

/**
 * The litres of the vessel the figure `bearer` carries as the gear port shows them; false while it carries
 * none. ase-equipment fills the vessel in one act of a draw (EquipmentReqWatrSystem), so a rise of these
 * litres is the owner's own statement that the figure drew water at the brook - the view bends it then and
 * computes nothing of the draw.
 */
bool carried_water(uint32_t bearer, const std::vector<VivariumFigureGearView>& gear, double& litres) {
    for (const VivariumFigureGearView& piece : gear) {
        if (piece.bearer == bearer && piece.vessel) {
            litres = piece.water_l;
            return true;
        }
    }
    return false;
}

/**
 * The pose of the figure `figure` whose node is `body` (T.11): its record of the rest port and the radius of its
 * body handed to figure_pose (godot_figure_pose.hpp). The radius is the capsule ase-physics collides the figure
 * as, from the shoulder joints of its rig, or the stand-in's while the node carries no rig. A figure the port
 * carries no record for passes NaN, and stands.
 */
uint8_t pose_of(const VivariumFigureView& figure, const ::godot::Node3D* body,
                const std::vector<VivariumSurvivalRestView>& rests, const std::vector<FigureSceneStone>& stones) {
    FigureSceneRest rest{std::nan(""), std::nan(""), std::nan(""), std::nan("")};
    for (const VivariumSurvivalRestView& candidate : rests) {
        if (candidate.object_id == figure.object_id) {
            rest.is_resting = candidate.is_resting;
            rest.sleeping = candidate.sleeping;
            rest.rest_quality = candidate.rest_quality;
            rest.rest_target = candidate.rest_target;
            break;
        }
    }
    const double radius = body->has_meta(kFigureMetaRadius) ? static_cast<double>(body->get_meta(kFigureMetaRadius))
                                                            : static_cast<double>(kFigureStandInRadius);
    return figure_pose(rest, figure.x, figure.z, radius, stones);
}

/** The detail level a node name carries as _lod<k>_, -1 when it carries none. */
int32_t level_of_name(const ::godot::String& name) {
    const int64_t at = name.find(kFigureLodMark);
    const int64_t digit_at = at + 4;
    if (at < 0 || digit_at + 1 >= name.length()) {
        return -1;
    }
    const int32_t level = static_cast<int32_t>(name[digit_at]) - static_cast<int32_t>('0');
    if (level < 0 || level >= kFigureLodLevels || name[digit_at + 1] != '_') {
        return -1;
    }
    return level;
}

/**
 * Marks every mesh below `node` with the detail level its own name or the nearest named ancestor carries and
 * widens [bottom, top] by the meshes of level 0, in the frame of the figure's node (`frame`).
 */
void mark_levels(::godot::Node* node, int32_t inherited, const ::godot::Transform3D& frame, double& bottom,
                 double& top) {
    const int32_t own = level_of_name(::godot::String(node->get_name()));
    const int32_t level = own >= 0 ? own : inherited;
    ::godot::Transform3D here = frame;
    if (auto* spatial = ::godot::Object::cast_to<::godot::Node3D>(node)) {
        here = frame * spatial->get_transform();
    }
    if (auto* mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(node); mesh != nullptr && level >= 0) {
        mesh->set_meta(kFigureMetaLevel, level);
        if (level == 0) {
            const ::godot::AABB box = here.xform(mesh->get_aabb());
            bottom = ase::math::min(bottom, static_cast<double>(box.position.y));
            top = ase::math::max(top, static_cast<double>(box.position.y + box.size.y));
        }
    }
    for (int32_t i = 0; i < node->get_child_count(); ++i) {
        mark_levels(node->get_child(i), level, here, bottom, top);
    }
}

/** Shows the meshes of `level` below `node` and hides the other levels; a mesh without a level stays as it is. */
void show_level(::godot::Node* node, int32_t level) {
    if (auto* mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(node); mesh != nullptr &&
                                                                              mesh->has_meta(kFigureMetaLevel)) {
        mesh->set_visible(static_cast<int32_t>(mesh->get_meta(kFigureMetaLevel)) == level);
    }
    for (int32_t i = 0; i < node->get_child_count(); ++i) {
        show_level(node->get_child(i), level);
    }
}

/**
 * The socket of the figure file: a BoneAttachment3D under the Skeleton3D of the rig that follows the bone
 * socket_hand_r, so a thing carried in the right hand hangs below it and moves with every clip. false when the
 * file carries no Skeleton3D or its skeleton no socket bone.
 */
bool add_socket(::godot::Node* model) {
    const ::godot::TypedArray<::godot::Node> skeletons = model->find_children("*", "Skeleton3D", true, false);
    if (skeletons.is_empty()) {
        return false;
    }
    auto* skeleton = ::godot::Object::cast_to<::godot::Skeleton3D>(skeletons[0]);
    if (skeleton == nullptr || skeleton->find_bone(kFigureSocketBone) < 0) {
        return false;
    }
    ::godot::BoneAttachment3D* socket = memnew(::godot::BoneAttachment3D);
    socket->set_name(kFigureSocket);
    skeleton->add_child(socket);
    socket->set_bone_name(kFigureSocketBone);
    return true;
}

/**
 * The stand-in body under `body`: a capsule standing on the feet and a block on its front (-z, the course
 * 0 of ase-player looks along), both in the error material of the figure's name.
 */
void add_stand_in(::godot::Node3D* body, const ::godot::Ref<::godot::Material>& material) {
    ::godot::Ref<::godot::CapsuleMesh> capsule;
    capsule.instantiate();
    capsule->set_radius(kFigureStandInRadius);
    capsule->set_height(kFigureStandInHeight);
    ::godot::MeshInstance3D* trunk = memnew(::godot::MeshInstance3D);
    trunk->set_mesh(capsule);
    trunk->set_material_override(material);
    trunk->set_position(::godot::Vector3(0.0f, kFigureStandInHeight * 0.5f, 0.0f));
    body->add_child(trunk);

    ::godot::Ref<::godot::BoxMesh> block;
    block.instantiate();
    block->set_size(::godot::Vector3(kFigureFrontEdge, kFigureFrontEdge, kFigureFrontEdge));
    ::godot::MeshInstance3D* front = memnew(::godot::MeshInstance3D);
    front->set_mesh(block);
    front->set_material_override(material);
    front->set_position(::godot::Vector3(0.0f, kFigureFrontLift, -kFigureStandInRadius));
    body->add_child(front);

    // the hand socket of the stand-in: what the right hand carries hangs below it as below the rig's socket
    ::godot::Node3D* hand = memnew(::godot::Node3D);
    hand->set_name(kFigureSocket);
    hand->set_position(::godot::Vector3(kFigureHandSide, kFigureHandLift, 0.0f));
    body->add_child(hand);
}

/**
 * A new node of one figure: an instance of the packed file, turned so its face looks along the course, with its
 * levels marked, its height measured from level 0 and its clips mixed; else the stand-in body.
 *
 * THE FILE FACES +z, THE COURSE LOOKS ALONG -z. A glTF file stands with its front on +z; course 0 of ase-player
 * looks along -z, as the stand-in's front block does, and the player camera of ase-camera stands behind that
 * course. Unturned, the figure walks backwards and shows the player camera its face; the half turn of the file
 * under the figure's node puts its front on the course, so the camera looks over its shoulder.
 */
::godot::Node3D* new_figure(const FigureSceneLook& look, uint32_t object_id, const VivariumFigureCodeView& codes) {
    ::godot::Node3D* body = memnew(::godot::Node3D);
    body->set_name(figure_node_name(object_id));
    body->set_meta(kFigureMetaHeight, kFigureStandInHeight);
    ::godot::Node* model = look.model.is_null() ? nullptr : look.model->instantiate();
    if (model == nullptr) {
        add_stand_in(body, look.stand_in);
        return body;
    }
    body->add_child(model);
    if (auto* spatial = ::godot::Object::cast_to<::godot::Node3D>(model)) {
        spatial->set_rotation(::godot::Vector3(0.0f, kFigureModelTurn, 0.0f));
    }
    double bottom = HUGE_VAL;
    double top = -HUGE_VAL;
    mark_levels(model, -1, ::godot::Transform3D(), bottom, top);
    if (top > bottom) {
        body->set_meta(kFigureMetaHeight, top - bottom);
        // the eyes stand above the same soles the height is measured from (07.6, the face landing of ase-camera)
        double eye = 0.0;
        if (figure_eye_level(model, eye)) {
            body->set_meta(kFigureMetaEye, eye - bottom);
        }
        // the capsule ase-physics collides the walking body as: one around the whole body, as wide as the shoulders
        double radius = 0.0;
        if (figure_body_radius(model, radius)) {
            body->set_meta(kFigureMetaRadius, radius);
        }
    }
    // the clip mix spans the gait values of the code port: stand, walk and run as ase-pl-avatar writes them
    if (codes.known) {
        (void)add_figure_motion(model, codes.gait_stand, codes.gait_walk, codes.gait_run);
    }
    (void)add_socket(model);
    return body;
}

/** The open course from `from` to `to`, wrapped into (-pi, pi]. */
double course_gap(double from, double to) {
    const auto gap = static_cast<float>(to - from);
    return static_cast<double>(ase::math::atan2(ase::math::sin(gap), ase::math::cos(gap)));
}

/**
 * The detail level of a figure of `height` metres seen by `eye` from `distance` metres: its share of the frame
 * height against the three bands of the port (the mockup's pickLod). Level 0 without an eye.
 */
int32_t level_for(const ::godot::Camera3D* eye, double height, double distance, const VivariumFigureView& figure) {
    if (eye == nullptr || distance < kFigureDistMin) {
        return 0;
    }
    const float half_fov = static_cast<float>(eye->get_fov()) * ase::math::PI / 360.0f;
    const double share = height / (2.0 * distance * static_cast<double>(ase::math::tan(half_fov)));
    if (share > figure.lod_near) {
        return 0;
    }
    if (share > figure.lod_mid) {
        return 1;
    }
    return share > figure.lod_far ? 2 : 3;
}

}  // namespace

FigureSceneLook bind_figure_look(const VivariumAssetBinding& binding,
                                 const std::unordered_map<std::string, std::string>& roles,
                                 const FigureSceneMotion& motion) {
    FigureSceneLook look;
    look.motion = motion;  // SHA's turn of the body, read with the HUD before the binding
    const auto role = roles.find(kFigureRole);
    if (role != roles.end()) {
        look.name = role->second;
        const auto file = binding.files.find(look.name);
        if (file != binding.files.end()) {
            look.file = file->second;
        }
    }
    // the file itself loads in a task of the WorkerThreadPool (godot_figure_load.hpp), not here
    look.stand_in = error_material(binding, look.name);
    look.tools = bind_hand_tools(binding, roles);  // the tools of the hand; their files load beside the figure file
    look.rack = bind_rack_body(binding, roles);    // the tool place's body; its file loads beside the tools
    return look;
}

void warm_figure(::godot::Node3D* scene, const FigureSceneLook& look) {
    // Behind the opaque start screen the scene renders, and the start screen lifts only after frames without a
    // new pipeline compilation (VivariumView::follow_initialisation). The instance stands with all four detail
    // levels shown, so every surface of the file meets its pipelines here and not under the first visible frame.
    if (scene == nullptr || look.model.is_null() || figure_child_of<::godot::Node3D>(scene, kFigureWarm) != nullptr) {
        return;
    }
    ::godot::Node3D* warm = memnew(::godot::Node3D);
    warm->set_name(kFigureWarm);
    ::godot::Node* model = look.model->instantiate();
    if (model == nullptr) {
        memdelete(warm);
        return;
    }
    warm->add_child(model);
    scene->add_child(warm);
}

void end_figure_warmth(::godot::Node3D* scene) {
    if (scene == nullptr) {
        return;
    }
    ::godot::Node3D* warm = figure_child_of<::godot::Node3D>(scene, kFigureWarm);
    if (warm == nullptr) {
        return;
    }
    scene->remove_child(warm);
    warm->queue_free();
}

void fill_figures(::godot::Node3D* scene, const std::vector<VivariumFigureView>& figures,
                  const std::vector<VivariumFigureGearView>& gear, const FigureSceneLook& look,
                  const VivariumFigureCodeView& codes, const std::vector<VivariumSurvivalRestView>& rests,
                  const std::vector<FigureSceneStone>& stones) {
    // while the file loads no node is built: a node built now would stay the stand-in body once the pack came
    if (scene == nullptr || figure_load_running(look.load)) {
        return;
    }
    ::godot::Node3D* holder = figure_child_of<::godot::Node3D>(scene, kFigureHolder);
    if (holder == nullptr) {
        holder = memnew(::godot::Node3D);
        holder->set_name(kFigureHolder);
        scene->add_child(holder);
    }
    const ::godot::Camera3D* eye = figure_child_of<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    const double dt = scene->is_inside_tree() ? scene->get_process_delta_time() : 0.0;
    // every figure of this frame at its place; the plugin's axis order is already Godot's here
    // (VivariumFigureView: y up, z north), the course is ase-player's yaw, Godot's rotation around y
    ::godot::Dictionary carried;
    for (const VivariumFigureView& figure : figures) {
        const ::godot::String name = figure_node_name(figure.object_id);
        carried[name] = true;
        ::godot::Node3D* body = figure_child_of<::godot::Node3D>(holder, name);
        const bool fresh = body == nullptr;
        if (fresh) {
            body = new_figure(look, figure.object_id, codes);
            holder->add_child(body);
        }
        const ::godot::Vector3 place(static_cast<float>(figure.x), static_cast<float>(figure.y),
                                     static_cast<float>(figure.z));
        body->set_position(place);

        // the body turns toward the course of the port by SHA's rate; a new figure stands at it at once
        const double held = fresh ? figure.course : static_cast<double>(body->get_rotation().y);
        const double gap = course_gap(held, figure.course);
        const double turned = held + gap * ase::math::min(1.0, dt * look.motion.turn_rate);
        body->set_rotation(::godot::Vector3(0.0f, static_cast<float>(turned), 0.0f));

        // the clips: idle, walk and run by the gait, the turn clip while it turns on the spot (the mockup's weights);
        // the share in motion runs from the code port's stand to its walk, a span ase-pl-avatar sets, never 0 to 1
        if (codes.known) {
            const double walk_span = codes.gait_walk - codes.gait_stand;
            const double moving =
                walk_span > 0.0 ? ase::math::clamp((figure.gait - codes.gait_stand) / walk_span, 0.0, 1.0) : 0.0;
            const double turning =
                ase::math::min(1.0, ase::math::abs(gap) / look.motion.turn_full) * (1.0 - moving);
            // the bend at the brook (07.14): the vessel in its hand gained water since the last snapshot, under
            // SS/PC by the player's tap and under AI/PC by the fill of the BDI alike; the first sight of a
            // vessel only records its litres
            double litres = 0.0;
            bool bend = false;
            if (carried_water(figure.object_id, gear, litres)) {
                bend = body->has_meta(kFigureMetaWater) &&
                       litres > static_cast<double>(body->get_meta(kFigureMetaWater)) + kFigureWaterRise;
                body->set_meta(kFigureMetaWater, litres);
            } else if (body->has_meta(kFigureMetaWater)) {
                body->remove_meta(kFigureMetaWater);
                if (body->has_meta(kFigureMetaWaterNoted)) {
                    body->remove_meta(kFigureMetaWaterNoted);  // the water line notes the next vessel from its first litres
                }
            }
            drive_figure_motion(body, figure.gait, codes.gait_stand, codes.gait_run, turning, bend,
                                pose_of(figure, body, rests, stones));
        }

        // the detail level by the figure's share of the frame height
        const double height = static_cast<double>(body->get_meta(kFigureMetaHeight));
        const double distance =
            eye == nullptr ? 0.0
                           : static_cast<double>(eye->get_global_position().distance_to(
                                 place + ::godot::Vector3(0.0f, static_cast<float>(look.motion.look_lift), 0.0f)));
        const int32_t level = level_for(eye, height, distance, figure);
        body->set_meta(kFigureMetaLevelDrawn, level);  // the level line of the view reads it per change (07-G)
        show_level(body, level);

        // what the right hand carries by the gear port, below the socket of the rig or of the stand-in
        const ::godot::TypedArray<::godot::Node> sockets = body->find_children(kFigureSocket, "Node3D", true, false);
        if (!sockets.is_empty()) {
            carry_tool(::godot::Object::cast_to<::godot::Node3D>(sockets[0]),
                       hand_kind(figure.object_id, gear, kFigureSocketBone), look.tools, look.stand_in);
        }
    }
    // a figure the port no longer carries leaves the scene
    for (int32_t i = holder->get_child_count() - 1; i >= 0; --i) {
        ::godot::Node* child = holder->get_child(i);
        if (!carried.has(::godot::String(child->get_name()))) {
            holder->remove_child(child);
            child->queue_free();
        }
    }
}

bool aim_figure_camera(::godot::Node3D* scene, const VivariumFigureCameraView& camera) {
    if (scene == nullptr || !camera.known) {
        return false;
    }
    ::godot::Camera3D* eye = figure_child_of<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    if (eye == nullptr) {
        return false;
    }
    const double dx = camera.aim_x - camera.eye_x;
    const double dy = camera.aim_y - camera.eye_y;
    const double dz = camera.aim_z - camera.eye_z;
    if (dx * dx + dy * dy + dz * dz < kFigureAimMinSquared) {
        return false;  // ase-camera files an eye on its aim only before its first follow pass
    }
    eye->look_at_from_position(::godot::Vector3(static_cast<float>(camera.eye_x), static_cast<float>(camera.eye_y),
                                                static_cast<float>(camera.eye_z)),
                               ::godot::Vector3(static_cast<float>(camera.aim_x), static_cast<float>(camera.aim_y),
                                                static_cast<float>(camera.aim_z)));
    return true;
}

}  // namespace ase::adp::godot

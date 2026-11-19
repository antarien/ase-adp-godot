/**
 * ASE GODOT ADAPTER - FIGURE QUERIES
 *
 * @file        godot_figure_query.cpp
 * @brief       Reads height, rig, place and face back from the node of a figure the scene built
 * @description See godot_figure_scene.hpp. The figure scene builds one node per figure of the body port and marks
 *              on it what it measured: the height of level 0, the eyes of the rig, and on every mesh its detail
 *              level. The view asks those back each frame for its ports and its log lines - the rig once per
 *              node for the rig port of ase-pl-avatar, the place per step for the place line, the height for the
 *              face landing, and the face when the zoom comes to rest. These queries only read the node (and
 *              mark what was handed over); building, placing and detailing it stays the scene's.
 *
 *              THE FACE AND THE EYE ARE READ AS THEY ARE DRAWN (07.3). Both are sharp only with the texture of
 *              their source on the finest level: the shown mesh that carries the face material Std_Skin_Head,
 *              and the one that carries the iris material __Iris, each name their detail level, and the albedo
 *              active on each names its edge in texels - the pack the stage wrote (stage_native.py keeps the
 *              edge of the source image) or the image of the file. The view notes the coarser of both levels and
 *              both edges with the zoom line, and vivarium_figure_acceptance.py holds them against level 0 and
 *              the edges of Std_Skin_Head_Diffuse and __Iris_Diffuse in the staged file.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101611 - animate character motion
 * @actor       SS/PC
 * @created     2026-11-15
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_scene.hpp>
#include <ase/adp/godot/godot_figure_node.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr const char* kFigureFaceMaterial = "Std_Skin_Head";  // the glTF material of the face of villager_body (Character Creator), named on import
constexpr const char* kFigureEyeMaterial = "__Iris";          // the glTF material of the iris of villager_body: what makes the eye read sharp or soft

/** The node of figure `object_id` below the scene's holder; nullptr while the scene stands none. */
::godot::Node3D* figure_body(::godot::Node3D* scene, uint32_t object_id) {
    if (scene == nullptr) {
        return nullptr;
    }
    ::godot::Node3D* holder = figure_child_of<::godot::Node3D>(scene, kFigureHolder);
    if (holder == nullptr) {
        return nullptr;
    }
    return figure_child_of<::godot::Node3D>(holder, figure_node_name(object_id));
}

/**
 * The shown mesh below `node` that carries the material named `material_name`: its detail level and the edge of the
 * albedo active on it (an override the loader set counts as drawn). False when no shown mesh carries it, or that
 * material no albedo.
 */
bool albedo_of(::godot::Node* node, const ::godot::String& material_name, int32_t& level, int64_t& edge) {
    if (auto* mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(node);
        mesh != nullptr && mesh->is_visible() && mesh->has_meta(kFigureMetaLevel) && mesh->get_mesh().is_valid()) {
        for (int32_t surface = 0; surface < mesh->get_mesh()->get_surface_count(); ++surface) {
            const ::godot::Ref<::godot::Material> material = mesh->get_active_material(surface);
            auto* pbr = ::godot::Object::cast_to<::godot::BaseMaterial3D>(material.ptr());
            if (pbr == nullptr || pbr->get_name() != material_name) {
                continue;
            }
            const ::godot::Ref<::godot::Texture2D> albedo = pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO);
            if (albedo.is_null()) {
                return false;  // the material without an albedo: no edge to hold against its source
            }
            level = static_cast<int32_t>(mesh->get_meta(kFigureMetaLevel));
            edge = static_cast<int64_t>(albedo->get_width());
            return true;
        }
    }
    for (int32_t i = 0; i < node->get_child_count(); ++i) {
        if (albedo_of(node->get_child(i), material_name, level, edge)) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool figure_height(::godot::Node3D* scene, uint32_t object_id, double& height) {
    ::godot::Node3D* body = figure_body(scene, object_id);
    if (body == nullptr || !body->has_meta(kFigureMetaHeight)) {
        return false;
    }
    height = static_cast<double>(body->get_meta(kFigureMetaHeight));
    return true;
}

bool take_figure_rig(::godot::Node3D* scene, uint32_t object_id, double& eye_height, double& height, double& radius) {
    ::godot::Node3D* body = figure_body(scene, object_id);
    if (body == nullptr || !body->has_meta(kFigureMetaEye) || !body->has_meta(kFigureMetaHeight) ||
        !body->has_meta(kFigureMetaRadius) || body->has_meta(kFigureMetaRigTaken)) {
        return false;
    }
    eye_height = static_cast<double>(body->get_meta(kFigureMetaEye));
    height = static_cast<double>(body->get_meta(kFigureMetaHeight));
    radius = static_cast<double>(body->get_meta(kFigureMetaRadius));
    body->set_meta(kFigureMetaRigTaken, true);  // reported once per node: a node built anew is measured anew
    return true;
}

bool take_figure_place(::godot::Node3D* scene, uint32_t object_id, double step_m, double& x, double& y, double& z) {
    ::godot::Node3D* body = figure_body(scene, object_id);
    if (body == nullptr) {
        return false;
    }
    // the node's place is the place of the body port in Godot's axes (x east, y up, z north), as fill_figures set it
    const ::godot::Vector3 place = body->get_position();
    if (body->has_meta(kFigureMetaPlaceNoted)) {
        const ::godot::Vector3 noted = body->get_meta(kFigureMetaPlaceNoted);
        if (static_cast<double>(place.distance_to(noted)) < step_m) {
            return false;
        }
    }
    body->set_meta(kFigureMetaPlaceNoted, place);  // a node built anew carries no noted place: its first stand is noted
    x = static_cast<double>(place.x);
    y = static_cast<double>(place.y);
    z = static_cast<double>(place.z);
    return true;
}

bool take_figure_gait(::godot::Node3D* scene, uint32_t object_id, double gait) {
    ::godot::Node3D* body = figure_body(scene, object_id);
    if (body == nullptr) {
        return false;
    }
    // the body port writes the gait as one of the codes of the code port, so a change is any other value than the
    // one noted; a node built anew carries no noted value and its first gait is noted
    if (body->has_meta(kFigureMetaGaitNoted) && static_cast<double>(body->get_meta(kFigureMetaGaitNoted)) == gait) {
        return false;
    }
    body->set_meta(kFigureMetaGaitNoted, gait);
    return true;
}

bool take_figure_level(::godot::Node3D* scene, uint32_t object_id, int32_t& level) {
    ::godot::Node3D* body = figure_body(scene, object_id);
    if (body == nullptr || !body->has_meta(kFigureMetaLevelDrawn)) {
        return false;
    }
    // the scene notes the level it drew this frame; a change is any other level than the one noted, and a node built
    // anew carries no noted level, so its first drawn level is noted
    const auto drawn = static_cast<int32_t>(body->get_meta(kFigureMetaLevelDrawn));
    if (body->has_meta(kFigureMetaLevelNoted) && static_cast<int32_t>(body->get_meta(kFigureMetaLevelNoted)) == drawn) {
        return false;
    }
    body->set_meta(kFigureMetaLevelNoted, drawn);
    level = drawn;
    return true;
}

bool figure_face_look(::godot::Node3D* scene, uint32_t object_id, int32_t& level, int64_t& face_edge,
                      int64_t& eye_edge) {
    ::godot::Node3D* body = figure_body(scene, object_id);
    if (body == nullptr) {
        return false;
    }
    int32_t face_level = 0;
    int32_t eye_level = 0;
    if (!albedo_of(body, ::godot::String(kFigureFaceMaterial), face_level, face_edge) ||
        !albedo_of(body, ::godot::String(kFigureEyeMaterial), eye_level, eye_edge)) {
        return false;
    }
    level = face_level > eye_level ? face_level : eye_level;  // level 0 stands only when face and eye both draw the finest
    return true;
}

}  // namespace ase::adp::godot

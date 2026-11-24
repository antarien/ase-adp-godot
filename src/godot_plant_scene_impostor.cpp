/**
 * ASE GODOT ADAPTER - IMPOSTOR NODES OF THE PLANTS
 *
 * @file        godot_plant_scene_impostor.cpp
 * @brief       The card that stands for a far plant and the viewport that draws its texture once
 * @description Register B0295 point 4: far bodies become an impostor. Every plant node carries a card
 *              (GODOT_PLANT_IMPOSTOR, a Sprite3D turning about y to the camera) and a viewport of its own world
 *              (GODOT_PLANT_IMPOSTOR_VIEW) with an orthographic camera and seven drawing nodes of its own, one per
 *              drawing node of the plant - the fine and the coarse bark tube under a bark node of its own
 *              (register B0317) and the five instance nodes, the fine and the coarse leaf card among them
 *              (PLAN_ASE_VIVARIUM_TREEGEN T.6: a leaf of a body in the far band is drawn as the coarse 1 x 1 card,
 *              GODOT_PLANT_LEAVES_CORS). Each of them reads the very mesh, instance buffer and material resource
 *              the plant node was just filled with - no value is taken over, the resource is the same object - so
 *              the viewport draws the body exactly as the scene does, the closing of thin rings included. It draws
 *              once each time the plant is filled, which is when a body ticked or the shown orders moved. WHEN the
 *              card takes over is the graded level of ase-render-data (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10,
 *              usecase:101657 grade plants by picture height, flora.lod.v1): show_plant_share sets the drawing
 *              nodes and the card by the share level_share gives, every frame, so the body and the card cross over
 *              in the dither band of ase-render-data and are never both whole and never both gone. The reach of
 *              impostor_of (godot_plant_impostor.hpp) only sizes the texture.
 *
 *              The viewport draws without light (DEBUG_DRAW_UNSHADED, the albedo of the materials) and with a clear
 *              background; the card is shaded, so the sun and the sky of the scene light it like the plant beside it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100969 - instance the plant organs
 * @actor       Scene Graph Culler (SCENGRACUL)
 * @created     2026-11-13
 * @modified    2026-11-24
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_plant_impostor.hpp>
#include <ase/adp/godot/lod/godot_plant_level_scene.hpp>
#include <ase/adp/godot/lod/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/sprite3d.hpp>
#include <godot_cpp/classes/sprite_base3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

// the seven drawing nodes of a plant; the bark draws through the two tubes below its node (register B0317), so
// their paths run through it
void drawn_parts(::godot::NodePath (&parts)[GODOT_PLANT_DRAWN_PARTS]) {
    const ::godot::String bark = ::godot::String(GODOT_PLANT_BARK) + ::godot::String("/");
    const char* const instanced[] = {
        GODOT_PLANT_LEAVES, GODOT_PLANT_LEAVES_CORS, GODOT_PLANT_MALE, GODOT_PLANT_FEMALE, GODOT_PLANT_FRUIT};
    uint32_t at = 0u;
    for (const char* tube : {GODOT_PLANT_TUBES, GODOT_PLANT_TWIGS}) {
        parts[at] = ::godot::NodePath(bark + ::godot::String(tube));
        ++at;
    }
    for (const char* part : instanced) {
        parts[at] = ::godot::NodePath(part);
        ++at;
    }
}

}  // namespace

void make_impostor(::godot::Node3D* plant) {
    if (plant == nullptr) {
        return;
    }
    ::godot::Sprite3D* card = memnew(::godot::Sprite3D);
    card->set_name(GODOT_PLANT_IMPOSTOR);
    card->set_billboard_mode(::godot::BaseMaterial3D::BILLBOARD_FIXED_Y);
    card->set_draw_flag(::godot::SpriteBase3D::FLAG_SHADED, true);
    card->set_alpha_cut_mode(::godot::SpriteBase3D::ALPHA_CUT_DISCARD);
    card->set_visible(false);
    plant->add_child(card);

    ::godot::SubViewport* view = memnew(::godot::SubViewport);
    view->set_name(GODOT_PLANT_IMPOSTOR_VIEW);
    view->set_use_own_world_3d(true);
    view->set_transparent_background(true);
    view->set_debug_draw(::godot::Viewport::DEBUG_DRAW_UNSHADED);
    view->set_update_mode(::godot::SubViewport::UPDATE_DISABLED);
    plant->add_child(view);

    ::godot::Camera3D* eye = memnew(::godot::Camera3D);
    eye->set_name(GODOT_PLANT_CAMERA);
    view->add_child(eye);
    eye->set_current(true);
    ::godot::Node3D* bark = memnew(::godot::Node3D);
    bark->set_name(GODOT_PLANT_BARK);
    view->add_child(bark);
    for (const char* tube : {GODOT_PLANT_TUBES, GODOT_PLANT_TWIGS}) {
        ::godot::MeshInstance3D* drawer = memnew(::godot::MeshInstance3D);
        drawer->set_name(tube);
        bark->add_child(drawer);
    }
    for (const char* part :
         {GODOT_PLANT_LEAVES, GODOT_PLANT_LEAVES_CORS, GODOT_PLANT_MALE, GODOT_PLANT_FEMALE, GODOT_PLANT_FRUIT}) {
        ::godot::MultiMeshInstance3D* drawer = memnew(::godot::MultiMeshInstance3D);
        drawer->set_name(part);
        view->add_child(drawer);
    }
}

void fill_impostor(::godot::Node3D* plant, const PlantImpostorFrame& frame) {
    if (plant == nullptr) {
        return;
    }
    // the card gets a texture when the body has a ring to measure (a reach) and a size; WHEN it stands in is the
    // level of ase-render-data (show_plant_share), never the reach - the reach only sizes the texture
    const bool stood_in = frame.reach > 0.0f && frame.height > 0.0f && frame.texture_height > 0u;
    ::godot::NodePath parts[GODOT_PLANT_DRAWN_PARTS];
    drawn_parts(parts);
    ::godot::Sprite3D* card =
        ::godot::Object::cast_to<::godot::Sprite3D>(plant->get_node_or_null(::godot::NodePath(GODOT_PLANT_IMPOSTOR)));
    ::godot::SubViewport* view = ::godot::Object::cast_to<::godot::SubViewport>(
        plant->get_node_or_null(::godot::NodePath(GODOT_PLANT_IMPOSTOR_VIEW)));
    if (card == nullptr || view == nullptr) {
        return;
    }
    if (!stood_in) {
        card->set_texture(::godot::Ref<::godot::Texture2D>());
        card->set_visible(false);
        view->set_update_mode(::godot::SubViewport::UPDATE_DISABLED);
        return;
    }

    // the drawing nodes of the viewport take the resources the plant nodes were just filled with
    for (const ::godot::NodePath& part : parts) {
        ::godot::Node* plant_part = plant->get_node_or_null(part);
        ::godot::Node* view_part = view->get_node_or_null(part);
        ::godot::MeshInstance3D* plant_mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(plant_part);
        ::godot::MeshInstance3D* view_mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(view_part);
        if (plant_mesh != nullptr && view_mesh != nullptr) {
            view_mesh->set_mesh(plant_mesh->get_mesh());
            view_mesh->set_material_override(plant_mesh->get_material_override());
        }
        ::godot::MultiMeshInstance3D* plant_multi = ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(plant_part);
        ::godot::MultiMeshInstance3D* view_multi = ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(view_part);
        if (plant_multi != nullptr && view_multi != nullptr) {
            view_multi->set_multimesh(plant_multi->get_multimesh());
            view_multi->set_material_override(plant_multi->get_material_override());
        }
    }

    // the camera looks along -z at the box of the body and stands one body depth in front of it, never closer
    // than twice its own near plane, so no part of the body lies outside its depth range
    ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(view->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (eye != nullptr) {
        const float near = eye->get_near();
        const float gap = ase::math::max(frame.depth, 2.0f * near);
        eye->set_orthogonal(frame.height, near, gap + frame.depth + gap);
        eye->set_position(
            ::godot::Vector3(frame.centre_x, frame.centre_y, frame.centre_z + 0.5f * frame.depth + gap));
    }
    view->set_size(::godot::Vector2i(static_cast<int32_t>(frame.texture_width),
                                     static_cast<int32_t>(frame.texture_height)));
    view->set_update_mode(::godot::SubViewport::UPDATE_ONCE);

    // the card shows the texture at the size the body has: its height over the pixels of that height
    card->set_texture(view->get_texture());
    card->set_pixel_size(frame.height / static_cast<float>(frame.texture_height));
    card->set_position(::godot::Vector3(frame.centre_x, frame.centre_y, frame.centre_z));
}

void show_plant_share(::godot::Node3D* plant, const PlantLevelShare& share) {
    if (plant == nullptr) {
        return;
    }
    // the body fades out by the share it handed over: transparency 1 - body, hidden where nothing of it is left
    ::godot::NodePath parts[GODOT_PLANT_DRAWN_PARTS];
    drawn_parts(parts);
    for (const ::godot::NodePath& part : parts) {
        ::godot::GeometryInstance3D* drawn =
            ::godot::Object::cast_to<::godot::GeometryInstance3D>(plant->get_node_or_null(part));
        if (drawn != nullptr) {
            drawn->set_visible(share.body > 0.0f);
            drawn->set_transparency(1.0f - share.body);
        }
    }
    // the card shows by the share handed to it, and only with a texture fill_impostor prepared
    ::godot::Sprite3D* card =
        ::godot::Object::cast_to<::godot::Sprite3D>(plant->get_node_or_null(::godot::NodePath(GODOT_PLANT_IMPOSTOR)));
    if (card == nullptr) {
        return;
    }
    const bool textured = card->get_texture().is_valid();
    card->set_visible(textured && share.card > 0.0f);
    card->set_transparency(1.0f - share.card);
}

}  // namespace ase::adp::godot

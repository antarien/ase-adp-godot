/**
 * ASE GODOT ADAPTER - IMPOSTOR NODES OF THE PLANTS
 *
 * @file        godot_plant_scene_impostor.cpp
 * @brief       The card, the billboard and the canopy dome that stand for a far plant, and the share each of them shows
 * @description Register B0295 point 4: far bodies become an impostor. Every plant node carries a card
 *              (GODOT_PLANT_IMPOSTOR, a Sprite3D turning about y to the camera), a billboard (GODOT_PLANT_BILLBOARD,
 *              LOD 4) and a canopy dome (GODOT_PLANT_CANOPY, LOD 5). None of them draws a picture of its own: the
 *              body is drawn once into its tile on a shared view page and shown from there
 *              (godot_plant_scene_atlas.cpp, PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10 Detailstufen, "Atlas-Seiten
 *              2048 x 2048"), so 10 000 birches need a handful of render targets, not 10 000.
 *
 *              fill_impostor runs whenever a body is filled, which is when a body ticked or the shown orders moved. It
 *              keeps the box of the body on the plant node (GODOT_KEY_VIEW_FRAME) and lets its view tile wait to be
 *              drawn (wait_for_view_tile); draw_view_tiles draws the waiting tiles within the budget of a frame and
 *              sets the card, the billboard and the dome on their regions. A body without a ring to measure or
 *              without a size gives its slot back (drop_impostor), and its card, billboard and dome lose their
 *              texture and hide.
 *
 *              WHEN each form takes over is the graded level of ase-render-data (usecase:101657 grade plants by
 *              picture height, flora.lod.v1): show_plant_share sets the drawing nodes, the card and the billboard by
 *              the share level_share gives, every frame, so two forms cross over in the dither band of ase-render-data
 *              and are never both whole and never both gone; show_plant_canopy sets the dome by the share handed on to
 *              the canopy, thinned by the cover of its raster cell (flora.cnpy.v1). The reach of impostor_of
 *              (godot_plant_impostor.hpp) only decides that the body can stand in.
 *
 *              The card and the billboard are shaded, so the sun and the sky of the scene light them like the plant
 *              beside them; the view pages they show draw the albedo of the materials.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100969 - instance the plant organs
 * @actor       Scene Graph Culler (SCENGRACUL)
 * @created     2026-11-13
 * @modified    2026-11-24
 * @version     00.00.25.00025 [core]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_plant_impostor.hpp>
#include <ase/adp/godot/lod/godot_host_lod_view.hpp>
#include <ase/adp/godot/lod/godot_plant_atlas.hpp>
#include <ase/adp/godot/lod/godot_plant_level_scene.hpp>
#include <ase/adp/godot/lod/types.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/sprite3d.hpp>
#include <godot_cpp/classes/sprite_base3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

namespace {

template <typename T>
T* child_of(::godot::Node* node, const char* name) {
    return ::godot::Object::cast_to<T>(node->get_node_or_null(::godot::NodePath(name)));
}

// a sprite of the plant that turns about y to the camera, shaded, cut at its alpha and hidden until it has a region
::godot::Sprite3D* make_sprite(::godot::Node3D* plant, const char* name) {
    ::godot::Sprite3D* sprite = memnew(::godot::Sprite3D);
    sprite->set_name(name);
    sprite->set_billboard_mode(::godot::BaseMaterial3D::BILLBOARD_FIXED_Y);
    sprite->set_draw_flag(::godot::SpriteBase3D::FLAG_SHADED, true);
    sprite->set_alpha_cut_mode(::godot::SpriteBase3D::ALPHA_CUT_DISCARD);
    sprite->set_visible(false);
    plant->add_child(sprite);
    return sprite;
}

// a sprite loses its picture and hides
void empty_sprite(::godot::Sprite3D* sprite) {
    if (sprite == nullptr) {
        return;
    }
    sprite->set_texture(::godot::Ref<::godot::Texture2D>());
    sprite->set_region_enabled(false);
    sprite->set_visible(false);
}

}  // namespace

void make_impostor(::godot::Node3D* plant) {
    if (plant == nullptr) {
        return;
    }
    // the view card of LOD 3 and the billboard of LOD 4: each shows the region of the plant's slot on a shared page,
    // set by draw_view_tiles
    make_sprite(plant, GODOT_PLANT_IMPOSTOR);
    make_sprite(plant, GODOT_PLANT_BILLBOARD);

    // the canopy dome of LOD 5: a flattened sphere wearing the view tile, so the canopy of the whole carries the
    // colours of the individuals of its cell; its size and its skin come with draw_view_tiles, its height and cover
    // with show_plant_canopy
    ::godot::MeshInstance3D* dome = memnew(::godot::MeshInstance3D);
    dome->set_name(GODOT_PLANT_CANOPY);
    ::godot::Ref<::godot::SphereMesh> shape;
    shape.instantiate();
    dome->set_mesh(shape);
    ::godot::Ref<::godot::StandardMaterial3D> skin;
    skin.instantiate();
    skin->set_transparency(::godot::BaseMaterial3D::TRANSPARENCY_ALPHA);
    dome->set_material_override(skin);
    dome->set_visible(false);
    plant->add_child(dome);
}

void fill_impostor(::godot::Node3D* plant, const PlantImpostorFrame& frame) {
    if (plant == nullptr) {
        return;
    }
    // the body can stand in when it has a ring to measure (a reach) and a size; WHEN it stands in is the level of
    // ase-render-data (show_plant_share), never the reach
    const bool stood_in = frame.reach > 0.0f && frame.height > 0.0f && frame.texture_height > 0u;
    ::godot::Node* atlas = atlas_of(plant);
    if (!stood_in) {
        empty_sprite(child_of<::godot::Sprite3D>(plant, GODOT_PLANT_IMPOSTOR));
        empty_sprite(child_of<::godot::Sprite3D>(plant, GODOT_PLANT_BILLBOARD));
        ::godot::MeshInstance3D* dome = child_of<::godot::MeshInstance3D>(plant, GODOT_PLANT_CANOPY);
        ::godot::Ref<::godot::StandardMaterial3D> skin;
        if (dome != nullptr) {
            skin = dome->get_material_override();
        }
        if (skin.is_valid()) {
            skin->set_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO, ::godot::Ref<::godot::Texture2D>());
            dome->set_visible(false);
        }
        drop_impostor(plant, atlas);
        return;
    }

    // the box of the body is kept on the plant, so its tile can be drawn again when the observer walks past a step;
    // the tile waits behind the plants already waiting
    ::godot::PackedFloat32Array box;
    box.push_back(frame.width);
    box.push_back(frame.height);
    box.push_back(frame.depth);
    box.push_back(frame.centre_x);
    box.push_back(frame.centre_y);
    box.push_back(frame.centre_z);
    plant->set_meta(::godot::StringName(GODOT_KEY_VIEW_FRAME), box);
    wait_for_view_tile(atlas, plant);
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
    // the card shows by the share handed to it, and only once its tile was drawn
    ::godot::Sprite3D* card = child_of<::godot::Sprite3D>(plant, GODOT_PLANT_IMPOSTOR);
    if (card != nullptr) {
        card->set_visible(card->get_texture().is_valid() && share.card > 0.0f);
        card->set_transparency(1.0f - share.card);
    }
    // the billboard shows by the share handed to it from the card, with its region on the billboard page; the share
    // handed on to the canopy leaves the individual undrawn
    ::godot::Sprite3D* billboard = child_of<::godot::Sprite3D>(plant, GODOT_PLANT_BILLBOARD);
    if (billboard != nullptr) {
        billboard->set_visible(billboard->get_texture().is_valid() && share.tile > 0.0f);
        billboard->set_transparency(1.0f - share.tile);
    }
}

void show_plant_canopy(::godot::Node3D* plant, float canopy, const VivariumCnpyView* cnpy) {
    if (plant == nullptr) {
        return;
    }
    ::godot::MeshInstance3D* dome = child_of<::godot::MeshInstance3D>(plant, GODOT_PLANT_CANOPY);
    if (dome == nullptr) {
        return;
    }
    const ::godot::Ref<::godot::SphereMesh> shape = dome->get_mesh();
    const ::godot::Ref<::godot::StandardMaterial3D> skin = dome->get_material_override();
    // the dome shows by the share the billboard handed on to the canopy, thinned by the cover of its raster cell;
    // a cell without a canopy record or a dome without the view tile as skin draws nothing
    const float shown = canopy * canopy_cover(cnpy);
    const bool drawn = cnpy != nullptr && shape.is_valid() && skin.is_valid() &&
                       skin->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO).is_valid() && shown > 0.0f;
    dome->set_visible(drawn);
    if (!drawn) {
        return;
    }
    dome->set_transparency(1.0f - shown);
    // its top stands at the crown height of the tallest stand of the cell (GIS_CANOPY_HEIGHT)
    ::godot::Vector3 at = dome->get_position();
    at.y = static_cast<float>(cnpy->height) - 0.5f * shape->get_height();
    dome->set_position(at);
}

}  // namespace ase::adp::godot

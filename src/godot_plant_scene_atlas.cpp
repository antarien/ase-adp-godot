/**
 * ASE GODOT ADAPTER - ATLAS PAGES OF THE FAR PLANTS
 *
 * @file        godot_plant_scene_atlas.cpp
 * @brief       The shared view pages of LOD 3 and billboard pages of LOD 4, the slot of each plant and the tiles drawn per frame
 * @description THE VIEW CARD (LOD 3) ON SHARED VIEW PAGES (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN Detailstufen,
 *              "Blickkarte"): instead of a viewport per plant, one viewport of its own world per page of
 *              GODOT_PLANT_BILLBOARD_PAGE_SIZE pixels a side (GODOT_PLANT_VIEW_PAGE) draws many individuals in tiles of
 *              GODOT_PLANT_VIEW_WIDTH x GODOT_PLANT_VIEW_HEIGHT pixels, UPDATE_ONCE per page. A plant takes a slot
 *              (take_slot, view_slot) and its tile node in the page world (GODOT_PLANT_VIEW_TILE) carries seven drawing
 *              nodes, one per drawing node of the plant. Each reads the very mesh, instance buffer and material the
 *              plant node was filled with, so the tile draws the body as the scene does. The tile node fits the box of
 *              the body into its tile and turns it to the step of its bearing to the observer (view_step). A tile waits
 *              to be drawn when its body was filled anew (wait_for_view_tile) or that step changed; at most
 *              GODOT_PLANT_VIEW_TILE_BUDGET tiles are drawn per frame (draw_view_tiles), a drawing budget of the adapter,
 *              never a cap of the simulation. The card shows its region of the page.
 *
 *              THE BILLBOARD (LOD 4): the same slot holds a tile of GODOT_PLANT_BILLBOARD_WIDTH x
 *              GODOT_PLANT_BILLBOARD_HEIGHT pixels on a 2D billboard page (GODOT_PLANT_BILLBOARD_PAGE) that shows the
 *              view tile at half its size, so 10 000 birches need five view pages and two billboard pages, not 10 000
 *              viewports. Godot draws its viewports in the order they were made; a billboard page draws in the frame
 *              after its tile changed (GODOT_KEY_BILLBOARD_FILLED, draw_billboard_pages), when the view page it reads
 *              has drawn. A plant gives its slot back when its card loses the body or it leaves the scene
 *              (drop_impostor). Past the tile level the canopy dome stands in, skinned with the view tile.
 *
 *              The view pages draw without light (DEBUG_DRAW_UNSHADED) on a clear background; the card is shaded, so
 *              the sun and the sky of the scene light it like the plant beside it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100969 - instance the plant organs
 * @actor       Scene Graph Culler (SCENGRACUL)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_impostor.hpp>
#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_plant_scene_parts.hpp>
#include <ase/adp/godot/lod/godot_host_lod_view.hpp>
#include <ase/adp/godot/lod/godot_plant_atlas.hpp>
#include <ase/adp/godot/lod/godot_plant_level_scene.hpp>
#include <ase/adp/godot/lod/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/atlas_texture.hpp>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/sprite3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

template <typename T>
T* child_of(::godot::Node* node, const char* name) {
    return ::godot::Object::cast_to<T>(node->get_node_or_null(::godot::NodePath(name)));
}

// the frame of the engine now: a billboard page whose tile changed in it draws in the next one
int64_t frame_now() {
    return static_cast<int64_t>(::godot::Engine::get_singleton()->get_process_frames());
}

::godot::String numbered(const char* prefix, uint32_t number) {
    return ::godot::String(prefix) + ::godot::String::num_uint64(number);
}

// billboard page `page`, made when the atlas has none yet: a 2D viewport with a clear background that draws only
// when a tile of it changed
::godot::SubViewport* billboard_page_of(::godot::Node* atlas, uint32_t page) {
    const ::godot::String name = numbered(GODOT_PLANT_BILLBOARD_PAGE, page);
    ::godot::SubViewport* sheet = ::godot::Object::cast_to<::godot::SubViewport>(atlas->get_node_or_null(name));
    if (sheet != nullptr) {
        return sheet;
    }
    sheet = memnew(::godot::SubViewport);
    sheet->set_name(name);
    sheet->set_disable_3d(true);
    sheet->set_transparent_background(true);
    sheet->set_size(::godot::Vector2i(GODOT_PLANT_BILLBOARD_PAGE_SIZE, GODOT_PLANT_BILLBOARD_PAGE_SIZE));
    sheet->set_update_mode(::godot::SubViewport::UPDATE_DISABLED);
    atlas->add_child(sheet);
    return sheet;
}

// view page `page`, made when the atlas has none yet: a viewport of its own world, unlit, with an orthographic camera
// over the whole page; a tile is one unit wide and GODOT_PLANT_VIEW_HEIGHT / GODOT_PLANT_VIEW_WIDTH high, row 0 at the
// top, so the page spans GODOT_PLANT_VIEW_COLUMNS units both ways
::godot::SubViewport* view_page_of(::godot::Node* atlas, uint32_t page) {
    const ::godot::String name = numbered(GODOT_PLANT_VIEW_PAGE, page);
    ::godot::SubViewport* sheet = ::godot::Object::cast_to<::godot::SubViewport>(atlas->get_node_or_null(name));
    if (sheet != nullptr) {
        return sheet;
    }
    sheet = memnew(::godot::SubViewport);
    sheet->set_name(name);
    sheet->set_use_own_world_3d(true);
    sheet->set_transparent_background(true);
    sheet->set_debug_draw(::godot::Viewport::DEBUG_DRAW_UNSHADED);
    sheet->set_size(::godot::Vector2i(GODOT_PLANT_BILLBOARD_PAGE_SIZE, GODOT_PLANT_BILLBOARD_PAGE_SIZE));
    sheet->set_update_mode(::godot::SubViewport::UPDATE_DISABLED);
    atlas->add_child(sheet);
    ::godot::Camera3D* eye = memnew(::godot::Camera3D);
    eye->set_name(GODOT_PLANT_CAMERA);
    sheet->add_child(eye);
    const float span = static_cast<float>(GODOT_PLANT_VIEW_COLUMNS);
    eye->set_orthogonal(span, GODOT_PLANT_VIEW_NEAR, GODOT_PLANT_VIEW_FAR);
    eye->set_position(::godot::Vector3(0.5f * span, -0.5f * span, GODOT_PLANT_VIEW_EYE_Z));
    eye->set_current(true);
    return sheet;
}

// the billboard tile of slot `index`, made when its page has none yet: a TextureRect at the place of the slot that
// keeps the aspect of the view tile it shows
::godot::TextureRect* billboard_tile_of(::godot::SubViewport* sheet, uint32_t index, const AtlasSlot& place) {
    const ::godot::String name = numbered(GODOT_PLANT_BILLBOARD_TILE, index);
    ::godot::TextureRect* tile = ::godot::Object::cast_to<::godot::TextureRect>(sheet->get_node_or_null(name));
    if (tile != nullptr) {
        return tile;
    }
    tile = memnew(::godot::TextureRect);
    tile->set_name(name);
    tile->set_expand_mode(::godot::TextureRect::EXPAND_IGNORE_SIZE);
    tile->set_stretch_mode(::godot::TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
    sheet->add_child(tile);
    tile->set_position(::godot::Vector2(static_cast<float>(place.column * GODOT_PLANT_BILLBOARD_WIDTH),
                                        static_cast<float>(place.row * GODOT_PLANT_BILLBOARD_HEIGHT)));
    tile->set_size(::godot::Vector2(static_cast<float>(GODOT_PLANT_BILLBOARD_WIDTH),
                                    static_cast<float>(GODOT_PLANT_BILLBOARD_HEIGHT)));
    return tile;
}

// the view tile node of slot `index` in the world of its page, made with its seven drawing nodes when it has none yet
::godot::Node3D* view_tile_of(::godot::SubViewport* sheet, uint32_t index) {
    const ::godot::String name = numbered(GODOT_PLANT_VIEW_TILE, index);
    ::godot::Node3D* tile = ::godot::Object::cast_to<::godot::Node3D>(sheet->get_node_or_null(name));
    if (tile != nullptr) {
        return tile;
    }
    tile = memnew(::godot::Node3D);
    tile->set_name(name);
    sheet->add_child(tile);
    ::godot::Node3D* bark = memnew(::godot::Node3D);
    bark->set_name(GODOT_PLANT_BARK);
    tile->add_child(bark);
    for (const char* tube : {GODOT_PLANT_TUBES, GODOT_PLANT_TWIGS}) {
        ::godot::MeshInstance3D* drawer = memnew(::godot::MeshInstance3D);
        drawer->set_name(tube);
        bark->add_child(drawer);
    }
    for (const char* part :
         {GODOT_PLANT_LEAVES, GODOT_PLANT_LEAVES_CORS, GODOT_PLANT_MALE, GODOT_PLANT_FEMALE, GODOT_PLANT_FRUIT}) {
        ::godot::MultiMeshInstance3D* drawer = memnew(::godot::MultiMeshInstance3D);
        drawer->set_name(part);
        tile->add_child(drawer);
    }
    return tile;
}

// the drawing nodes of `to` take the resources of the drawing nodes of `from`; a null `from` empties them
void hand_parts(::godot::Node* from, ::godot::Node* to) {
    ::godot::NodePath parts[GODOT_PLANT_DRAWN_PARTS];
    drawn_parts(parts);
    for (const ::godot::NodePath& part : parts) {
        ::godot::Node* source = from == nullptr ? nullptr : from->get_node_or_null(part);
        ::godot::Node* target = to->get_node_or_null(part);
        ::godot::MeshInstance3D* to_mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(target);
        if (to_mesh != nullptr) {
            ::godot::MeshInstance3D* mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(source);
            to_mesh->set_mesh(mesh == nullptr ? ::godot::Ref<::godot::Mesh>() : mesh->get_mesh());
            to_mesh->set_material_override(mesh == nullptr ? ::godot::Ref<::godot::Material>() : mesh->get_material_override());
        }
        ::godot::MultiMeshInstance3D* to_multi = ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(target);
        if (to_multi != nullptr) {
            ::godot::MultiMeshInstance3D* multi = ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(source);
            to_multi->set_multimesh(multi == nullptr ? ::godot::Ref<::godot::MultiMesh>() : multi->get_multimesh());
            to_multi->set_material_override(multi == nullptr ? ::godot::Ref<::godot::Material>() : multi->get_material_override());
        }
    }
}

// the slot of the plant: the one it holds, else the last one given back, else the next one never taken
uint32_t take_slot(::godot::Node* atlas, ::godot::Node* plant) {
    const ::godot::StringName held(GODOT_KEY_BILLBOARD_SLOT);
    if (plant->has_meta(held)) {
        return static_cast<uint32_t>(static_cast<int64_t>(plant->get_meta(held)));
    }
    const ::godot::StringName free_key(GODOT_KEY_BILLBOARD_FREE);
    ::godot::PackedInt32Array given_back = atlas->get_meta(free_key, ::godot::PackedInt32Array());
    uint32_t index = 0u;
    if (given_back.size() > 0) {
        const int64_t last = given_back.size() - 1;
        index = static_cast<uint32_t>(given_back[last]);
        given_back.remove_at(last);
        atlas->set_meta(free_key, given_back);
    } else {
        const ::godot::StringName next_key(GODOT_KEY_BILLBOARD_NEXT);
        index = static_cast<uint32_t>(static_cast<int64_t>(atlas->get_meta(next_key, static_cast<int64_t>(0))));
        atlas->set_meta(next_key, static_cast<int64_t>(index) + 1);
    }
    plant->set_meta(held, static_cast<int64_t>(index));
    return index;
}

// the step of the bearing from the plant to the observer
int32_t step_of(::godot::Node3D* plant, const ::godot::Camera3D* eye) {
    const ::godot::Vector3 from = eye->get_global_position() - plant->get_global_position();
    return view_step(static_cast<float>(from.x), static_cast<float>(from.z));
}

// draw the view tile of the plant from the box kept by fill_impostor, turned to `step`, and set the card, the
// billboard and the canopy dome on its regions
void draw_view_tile(::godot::Node3D* plant, ::godot::Node* atlas, int32_t step) {
    const ::godot::PackedFloat32Array box =
        plant->get_meta(::godot::StringName(GODOT_KEY_VIEW_FRAME), ::godot::PackedFloat32Array());
    if (box.size() < 6) {
        return;
    }
    const float width = box[0];
    const float height = box[1];
    const float depth = box[2];
    const ::godot::Vector3 centre(box[3], box[4], box[5]);
    const uint32_t index = take_slot(atlas, plant);
    const AtlasSlot place = view_slot(index);
    ::godot::SubViewport* sheet = view_page_of(atlas, place.page);
    ::godot::Node3D* tile = view_tile_of(sheet, index);
    hand_parts(plant, tile);

    // the box of the body fits its tile, one unit wide and tall / wide high; the tile turns it by the middle of its
    // step so the side facing the observer faces the camera of the page along -z
    const float tall = static_cast<float>(GODOT_PLANT_VIEW_HEIGHT) / static_cast<float>(GODOT_PLANT_VIEW_WIDTH);
    const float across = ase::math::max(width, depth);
    const float scale = across > 0.0f ? ase::math::min(1.0f / across, tall / height) : tall / height;
    const float turn = -(static_cast<float>(step) + 0.5f) * GODOT_PLANT_VIEW_ANGLE_STEP;
    const ::godot::Basis basis =
        ::godot::Basis(::godot::Vector3(0.0f, 1.0f, 0.0f), turn).scaled(::godot::Vector3(scale, scale, scale));
    const ::godot::Vector3 cell(static_cast<float>(place.column) + 0.5f, -(static_cast<float>(place.row) + 0.5f) * tall, 0.0f);
    tile->set_transform(::godot::Transform3D(basis, cell - basis.xform(centre)));
    sheet->set_update_mode(::godot::SubViewport::UPDATE_ONCE);
    plant->set_meta(::godot::StringName(GODOT_KEY_VIEW_STEP), static_cast<int64_t>(step));

    // the card shows the region of its tile at the size the body has: a unit of the page is GODOT_PLANT_VIEW_WIDTH pixels
    const ::godot::Rect2 region(static_cast<float>(place.column * GODOT_PLANT_VIEW_WIDTH),
                                static_cast<float>(place.row * GODOT_PLANT_VIEW_HEIGHT),
                                static_cast<float>(GODOT_PLANT_VIEW_WIDTH), static_cast<float>(GODOT_PLANT_VIEW_HEIGHT));
    const float metres_per_pixel = 1.0f / (static_cast<float>(GODOT_PLANT_VIEW_WIDTH) * scale);
    ::godot::Sprite3D* card = child_of<::godot::Sprite3D>(plant, GODOT_PLANT_IMPOSTOR);
    if (card != nullptr) {
        card->set_texture(sheet->get_texture());
        card->set_region_enabled(true);
        card->set_region_rect(region);
        card->set_pixel_size(metres_per_pixel);
        card->set_position(centre);
    }

    // the billboard tile shows the view tile at the ratio of the two tile sizes, and the billboard its region
    const AtlasSlot spot = billboard_slot(index);
    ::godot::SubViewport* board_page = billboard_page_of(atlas, spot.page);
    ::godot::Ref<::godot::AtlasTexture> cut;
    cut.instantiate();
    cut->set_atlas(sheet->get_texture());
    cut->set_region(region);
    billboard_tile_of(board_page, index, spot)->set_texture(cut);
    board_page->set_meta(::godot::StringName(GODOT_KEY_BILLBOARD_FILLED), frame_now());
    const float fit = static_cast<float>(GODOT_PLANT_BILLBOARD_WIDTH) / static_cast<float>(GODOT_PLANT_VIEW_WIDTH);
    ::godot::Sprite3D* billboard = child_of<::godot::Sprite3D>(plant, GODOT_PLANT_BILLBOARD);
    if (billboard != nullptr) {
        billboard->set_texture(board_page->get_texture());
        billboard->set_region_enabled(true);
        billboard->set_region_rect(::godot::Rect2(static_cast<float>(spot.column * GODOT_PLANT_BILLBOARD_WIDTH),
                                                  static_cast<float>(spot.row * GODOT_PLANT_BILLBOARD_HEIGHT),
                                                  static_cast<float>(GODOT_PLANT_BILLBOARD_WIDTH),
                                                  static_cast<float>(GODOT_PLANT_BILLBOARD_HEIGHT)));
        billboard->set_pixel_size(metres_per_pixel / fit);
        billboard->set_position(centre);
    }

    // the canopy dome takes the size of the crown and the view tile as its skin, by the uv of the tile on the page
    ::godot::MeshInstance3D* dome = child_of<::godot::MeshInstance3D>(plant, GODOT_PLANT_CANOPY);
    ::godot::Ref<::godot::SphereMesh> shape;
    ::godot::Ref<::godot::StandardMaterial3D> skin;
    if (dome != nullptr) {
        shape = dome->get_mesh();
        skin = dome->get_material_override();
    }
    if (shape.is_valid() && skin.is_valid()) {
        const float radius = 0.5f * across;
        shape->set_radius(radius);
        shape->set_height(2.0f * radius * GODOT_PLANT_CANOPY_DOME);
        const float page = static_cast<float>(GODOT_PLANT_BILLBOARD_PAGE_SIZE);
        skin->set_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO, sheet->get_texture());
        skin->set_uv1_scale(::godot::Vector3(region.size.x / page, region.size.y / page, 1.0f));
        skin->set_uv1_offset(::godot::Vector3(region.position.x / page, region.position.y / page, 0.0f));
        dome->set_position(centre);
    }
}

}  // namespace

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

::godot::Node* atlas_of(::godot::Node* plant) {
    ::godot::Node* bodies = plant == nullptr ? nullptr : plant->get_parent();
    ::godot::Node3D* scene = bodies == nullptr ? nullptr : ::godot::Object::cast_to<::godot::Node3D>(bodies->get_parent());
    return scene == nullptr ? nullptr : part_of<::godot::Node>(scene, GODOT_PLANT_BILLBOARD_ATLAS);
}

void wait_for_view_tile(::godot::Node* atlas, ::godot::Node* plant) {
    const ::godot::StringName queued(GODOT_KEY_VIEW_QUEUED);
    if (atlas == nullptr || plant == nullptr || plant->has_meta(queued)) {
        return;
    }
    plant->set_meta(queued, true);
    const ::godot::StringName queue_key(GODOT_KEY_VIEW_QUEUE);
    ::godot::PackedInt64Array queue = atlas->get_meta(queue_key, ::godot::PackedInt64Array());
    queue.push_back(static_cast<int64_t>(plant->get_instance_id()));
    atlas->set_meta(queue_key, queue);
}

void drop_impostor(::godot::Node* plant, ::godot::Node* atlas) {
    if (plant == nullptr || atlas == nullptr) {
        return;
    }
    plant->remove_meta(::godot::StringName(GODOT_KEY_VIEW_FRAME));
    plant->remove_meta(::godot::StringName(GODOT_KEY_VIEW_STEP));
    const ::godot::StringName held(GODOT_KEY_BILLBOARD_SLOT);
    if (!plant->has_meta(held)) {
        return;
    }
    const uint32_t index = static_cast<uint32_t>(static_cast<int64_t>(plant->get_meta(held)));
    plant->remove_meta(held);
    // both tiles are emptied and kept for the next plant; their pages draw the empty places
    ::godot::SubViewport* sheet = view_page_of(atlas, view_slot(index).page);
    hand_parts(nullptr, view_tile_of(sheet, index));
    sheet->set_update_mode(::godot::SubViewport::UPDATE_ONCE);
    const AtlasSlot spot = billboard_slot(index);
    ::godot::SubViewport* board_page = billboard_page_of(atlas, spot.page);
    billboard_tile_of(board_page, index, spot)->set_texture(::godot::Ref<::godot::Texture2D>());
    board_page->set_meta(::godot::StringName(GODOT_KEY_BILLBOARD_FILLED), frame_now());
    const ::godot::StringName free_key(GODOT_KEY_BILLBOARD_FREE);
    ::godot::PackedInt32Array given_back = atlas->get_meta(free_key, ::godot::PackedInt32Array());
    given_back.push_back(static_cast<int32_t>(index));
    atlas->set_meta(free_key, given_back);
}

void draw_billboard_pages(::godot::Node* atlas) {
    if (atlas == nullptr) {
        return;
    }
    // a billboard page draws once in the frame after a tile of it changed, when its view page has drawn
    const ::godot::StringName filled_key(GODOT_KEY_BILLBOARD_FILLED);
    const int64_t now = frame_now();
    const int32_t count = atlas->get_child_count();
    for (int32_t i = 0; i < count; ++i) {
        ::godot::SubViewport* sheet = ::godot::Object::cast_to<::godot::SubViewport>(atlas->get_child(i));
        if (sheet == nullptr) {
            continue;
        }
        const int64_t filled = static_cast<int64_t>(sheet->get_meta(filled_key, static_cast<int64_t>(-1)));
        if (filled >= 0 && filled < now) {
            sheet->set_update_mode(::godot::SubViewport::UPDATE_ONCE);
            sheet->set_meta(filled_key, static_cast<int64_t>(-1));
        }
    }
}

void draw_view_tiles(::godot::Node3D* scene) {
    ::godot::Node3D* bodies = scene == nullptr ? nullptr : part_of<::godot::Node3D>(scene, GODOT_PLANT_BODIES);
    ::godot::Node* atlas = scene == nullptr ? nullptr : part_of<::godot::Node>(scene, GODOT_PLANT_BILLBOARD_ATLAS);
    const ::godot::Camera3D* eye = scene == nullptr ? nullptr : part_of<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    if (bodies == nullptr || atlas == nullptr || eye == nullptr) {
        return;
    }
    // a drawn tile whose bearing to the observer left its step waits to be drawn again
    const ::godot::StringName step_key(GODOT_KEY_VIEW_STEP);
    const int32_t count = bodies->get_child_count();
    for (int32_t i = 0; i < count; ++i) {
        ::godot::Node3D* plant = ::godot::Object::cast_to<::godot::Node3D>(bodies->get_child(i));
        if (plant != nullptr && plant->has_meta(step_key) &&
            static_cast<int64_t>(plant->get_meta(step_key)) != static_cast<int64_t>(step_of(plant, eye))) {
            wait_for_view_tile(atlas, plant);
        }
    }
    // the oldest waiting tiles, at most the budget of one frame; a plant that left the scene is passed
    const ::godot::StringName queue_key(GODOT_KEY_VIEW_QUEUE);
    const ::godot::PackedInt64Array queue = atlas->get_meta(queue_key, ::godot::PackedInt64Array());
    const int64_t taken = ase::math::min(queue.size(), static_cast<int64_t>(GODOT_PLANT_VIEW_TILE_BUDGET));
    for (int64_t i = 0; i < taken; ++i) {
        ::godot::Node3D* plant =
            ::godot::Object::cast_to<::godot::Node3D>(::godot::ObjectDB::get_instance(static_cast<uint64_t>(queue[i])));
        if (plant == nullptr) {
            continue;
        }
        plant->remove_meta(::godot::StringName(GODOT_KEY_VIEW_QUEUED));
        draw_view_tile(plant, atlas, step_of(plant, eye));
    }
    atlas->set_meta(queue_key, queue.slice(taken));
}

}  // namespace ase::adp::godot

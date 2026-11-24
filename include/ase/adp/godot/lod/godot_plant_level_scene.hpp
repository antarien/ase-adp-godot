#pragma once

/**
 * ASE GODOT ADAPTER - PLANT LEVELS IN THE SCENE
 *
 * @file        godot_plant_level_scene.hpp
 * @brief       Show every plant body and its view card by the graded level of ase-render-data
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen (usecase:101657 grade plants by picture height):
 *              the level of each graded plant body reaches the adapter through flora.lod.v1 (get_lods of
 *              GodotHostResourceManager). show_plant_levels runs every frame over the plant bodies of the scene and
 *              hands each its share of body and card (level_share, godot_host_lod_view.hpp); show_plant_share sets
 *              the drawing nodes and the card of one plant by that share. The card texture is prepared by
 *              fill_impostor whenever a body is filled; whether it shows is decided here, never by a distance rule
 *              of the adapter.
 *
 *              A HEADER OF ITS OWN: godot_plant_scene.hpp reaches about a hundred units of the avt build; these two
 *              names are used by the plant scene, its impostor file and the vivarium scene only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101657 - grade plants by picture height
 * @actor       Picture Height Grader (PICTHGTGRA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/lod/godot_host_lod_view.hpp>

#include <cstdint>
#include <unordered_map>

namespace godot {
class Node;
class Node3D;
}  // namespace godot

namespace ase::adp::godot {

/**
 * Hand every plant body below the plant root of `scene` its share of body and card by its level in `lods` (keyed by
 * the patch's object id), and its canopy dome the share handed to the canopy by the cover of its raster cell in
 * `cnpys` (flora.cnpy.v1, same key). A body without a level record is not graded and draws whole; a patch without a
 * canopy record draws no dome.
 */
void show_plant_levels(::godot::Node3D* scene, const std::unordered_map<uint32_t, VivariumLodView>& lods,
                       const std::unordered_map<uint32_t, VivariumCnpyView>& cnpys);

/** Set the drawing nodes and the view card of one plant node by `share`. */
void show_plant_share(::godot::Node3D* plant, const PlantLevelShare& share);

/**
 * Set the canopy dome of one plant node (LOD 5): shown by `canopy` (share.canopy of level_share) times the cover of
 * `cnpy` (canopy_cover), its top at the crown height of the raster cell. No record (nullptr) hides it.
 */
void show_plant_canopy(::godot::Node3D* plant, float canopy, const VivariumCnpyView* cnpy);

/**
 * Give the atlas slot of `plant` back to `atlas` (GODOT_PLANT_BILLBOARD_ATLAS, LOD 3 and 4): its view tile and its
 * billboard tile are emptied, their pages draw the empty place and the next plant takes the slot before a new one. A
 * plant without a slot gives nothing back. Called when the card of the plant loses its body and before the plant
 * leaves the scene.
 */
void drop_impostor(::godot::Node* plant, ::godot::Node* atlas);

/**
 * Draw every billboard page of `atlas` once whose tile changed in an earlier frame: a page never draws in the frame
 * its view tile was drawn, so it always reads the picture that view page drew. Runs every frame with show_plant_levels.
 */
void draw_billboard_pages(::godot::Node* atlas);

/**
 * Draw the view card tiles (LOD 3) of the plants below the plant root of `scene` that wait for it: a plant waits when
 * its body was filled anew (fill_impostor) or the step of its bearing to the camera of the scene changed (view_step).
 * At most GODOT_PLANT_VIEW_TILE_BUDGET tiles are drawn per frame, oldest first; the rest wait for the next frame.
 * Runs every frame with show_plant_levels, before draw_billboard_pages.
 */
void draw_view_tiles(::godot::Node3D* scene);

}  // namespace ase::adp::godot

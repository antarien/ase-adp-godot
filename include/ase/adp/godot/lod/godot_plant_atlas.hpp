#pragma once

/**
 * ASE GODOT ADAPTER - ATLAS PAGES OF THE FAR PLANTS
 *
 * @file        godot_plant_atlas.hpp
 * @brief       The drawing nodes of a plant, the atlas node of its scene and the wait of its view tile
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen: the view cards of LOD 3 and the billboards of
 *              LOD 4 stand on shared atlas pages (GODOT_PLANT_BILLBOARD_ATLAS) instead of one viewport per plant.
 *              godot_plant_scene_atlas.cpp keeps the pages, the slots and the queue of tiles that wait to be drawn;
 *              godot_plant_scene_impostor.cpp makes the card, the billboard and the dome of each plant and hands the
 *              body to the queue whenever it was filled anew. These three names are what the two files share.
 *
 *              A HEADER OF ITS OWN: godot_plant_scene.hpp reaches about a hundred units of the avt build and
 *              godot_plant_level_scene.hpp is read by the vivarium scene; these names are used by the two atlas
 *              files only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
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

#include <ase/adp/godot/lod/types.hpp>

#include <godot_cpp/variant/node_path.hpp>

namespace godot {
class Node;
}  // namespace godot

namespace ase::adp::godot {

/**
 * The paths of the seven drawing nodes of a plant node and of its view tile: the fine and the coarse bark tube below
 * the bark node (register B0317) and the five instance nodes, the coarse leaf card among them (TREEGEN T.6).
 */
void drawn_parts(::godot::NodePath (&parts)[GODOT_PLANT_DRAWN_PARTS]);

/** The atlas node of the scene `plant` stands in (plant, the bodies node, the scene root), nullptr outside a scene. */
::godot::Node* atlas_of(::godot::Node* plant);

/**
 * Let the view tile of `plant` wait to be drawn on its page of `atlas`, once and behind the plants already waiting;
 * draw_view_tiles draws GODOT_PLANT_VIEW_TILE_BUDGET of them per frame.
 */
void wait_for_view_tile(::godot::Node* atlas, ::godot::Node* plant);

}  // namespace ase::adp::godot

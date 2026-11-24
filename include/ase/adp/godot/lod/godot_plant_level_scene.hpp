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
class Node3D;
}  // namespace godot

namespace ase::adp::godot {

/**
 * Hand every plant body below the plant root of `scene` its share of body and card by its level in `lods` (keyed by
 * the patch's object id). A body without a record is not graded and draws whole.
 */
void show_plant_levels(::godot::Node3D* scene, const std::unordered_map<uint32_t, VivariumLodView>& lods);

/** Set the drawing nodes and the view card of one plant node by `share`. */
void show_plant_share(::godot::Node3D* plant, const PlantLevelShare& share);

}  // namespace ase::adp::godot

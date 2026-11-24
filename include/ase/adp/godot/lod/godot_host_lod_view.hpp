#pragma once

/**
 * ASE GODOT ADAPTER - HOST LEVEL VIEW
 *
 * @file        godot_host_lod_view.hpp
 * @brief       The graded level of one plant body as the last snapshot of the level port showed it, and the share of
 *              body and card it gives
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen (usecase:101657 grade plants by picture height):
 *              ase-render-data grades every plant body by the height of its picture into one of six levels and
 *              hands a share of the body over to the next coarser level in the lowest tenth of each band
 *              (RND_LOD_LEVEL, RND_LOD_FADE, the codes RND_CONST_LOD_LEVEL_*). ase-pl-flora files them on
 *              flora.lod.v1; GodotHostResourceManager keeps one VivariumLodView per graded patch (get_lods), and the
 *              plant scene hands the body over to its view card by level_share - never by a distance rule of the
 *              adapter.
 *
 *              A HEADER OF ITS OWN: the patch view godot_host_patch_view.hpp is included by more than a hundred own
 *              units of the avt build and takes no new line (header ratchet, operator decision 2026-11-21). The
 *              level is read by the level reader, the plant scene and their tests only.
 *
 *              NO GODOT TYPE: plain numbers in, plain numbers out, so the host tests check the share without a
 *              Godot process. The Godot side is show_plant_levels (godot_plant_level_scene.hpp).
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

namespace ase::adp::godot {

/** The level record of one patch - a copy, never a reference into the App. Every value as the record carried it. */
struct VivariumLodView {
    double level = 0.0;  // the graded level of the plant body, one of the RND_CONST_LOD_LEVEL_* codes (RND_LOD_LEVEL)
    double fade = 0.0;   // the share of the body handed over to the next coarser level (RND_LOD_FADE)
    double view = 0.0;   // RND_CONST_LOD_LEVEL_VIEW: from this level the view card stands in for the body
    double tile = 0.0;   // RND_CONST_LOD_LEVEL_TILE: the level of the billboard
    double cnpy = 0.0;   // RND_CONST_LOD_LEVEL_CNPY: the level of the canopy
};

/** How much of a plant the drawn body and how much its view card show: 1 fully, 0 not at all. */
struct PlantLevelShare {
    float body = 1.0f;  // share of the drawn body: its bark tubes and its instance nodes
    float card = 0.0f;  // share of the view card (GODOT_PLANT_IMPOSTOR)
};

/**
 * The share of body and card for the level `lod` of a plant. No record (nullptr): the body is not graded and is drawn
 * whole. A level at or above the view level: the card stands in whole. The level just below the view level hands its
 * fade over to the card, so body and card cross over in the dither band of ase-render-data. Every finer level draws
 * the body whole.
 */
PlantLevelShare level_share(const VivariumLodView* lod);

}  // namespace ase::adp::godot

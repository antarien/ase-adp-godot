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

#include <cstdint>

namespace ase::adp::godot {

/** The level record of one patch - a copy, never a reference into the App. Every value as the record carried it. */
struct VivariumLodView {
    double level = 0.0;  // the graded level of the plant body, one of the RND_CONST_LOD_LEVEL_* codes (RND_LOD_LEVEL)
    double fade = 0.0;   // the share of the body handed over to the next coarser level (RND_LOD_FADE)
    double view = 0.0;   // RND_CONST_LOD_LEVEL_VIEW: from this level the view card stands in for the body
    double tile = 0.0;   // RND_CONST_LOD_LEVEL_TILE: the level of the billboard
    double cnpy = 0.0;   // RND_CONST_LOD_LEVEL_CNPY: the level of the canopy
};

/** How much of a plant each of its four forms shows: 1 fully, 0 not at all. The four always add up to one. */
struct PlantLevelShare {
    float body = 1.0f;    // share of the drawn body (LOD 0 to 2): its bark tubes and its instance nodes
    float card = 0.0f;    // share of the view card (LOD 3, GODOT_PLANT_IMPOSTOR)
    float tile = 0.0f;    // share of the billboard tile (LOD 4, GODOT_PLANT_BILLBOARD), drawn from the view card
    float canopy = 0.0f;  // share of the canopy of the whole (LOD 5): no form of the individual is drawn
};

/**
 * The canopy record of one patch (flora.cnpy.v1, LOD 5 "Kronendach des Ganzen") - a copy, never a reference into the
 * App. The rasters of the whole under the patch as ase-gis sampled them, with the coefficient of ase-plant.
 */
struct VivariumCnpyView {
    double lai = 0.0;     // GIS_CANOPY_LAI of the raster cell under the patch, m2 of leaf over m2 of ground
    double height = 0.0;  // GIS_CANOPY_HEIGHT of that cell, crown height of its tallest stand in metres
    double ext = 0.0;     // PLT_CONST_CNPY_LIGHT_EXT, the extinction coefficient of the cover
};

/**
 * The share of the ground the canopy of `cnpy` covers, 1 - exp(-ext * lai) (Beer-Lambert, the cover ase-plant reads
 * its light through). No record (nullptr): no canopy reported, nothing is covered.
 */
float canopy_cover(const VivariumCnpyView* cnpy);

/**
 * The share of the four forms for the level `lod` of a plant. No record (nullptr): the body is not graded and is
 * drawn whole. Below the view level the body is drawn; at the view level the card stands in, at the tile level the
 * billboard, from the canopy level on the individual is part of the canopy. A level whose next coarser level draws
 * another form hands its fade over to that form, so two forms cross over in the dither band of ase-render-data and
 * the shares always add up to one.
 */
PlantLevelShare level_share(const VivariumLodView* lod);

/**
 * The place of one tile on the shared atlas pages (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN Detailstufen: "Atlas-Seiten
 * 2048 x 2048"): the view card tiles of LOD 3 (32 x 64) and the billboard tiles of LOD 4 (16 x 32). A plant holds one
 * slot number for both; the slots fill a page row by row before the next page begins.
 */
struct AtlasSlot {
    uint32_t page = 0;    // the atlas page the tile stands on, 0 for the first
    uint32_t column = 0;  // the column of the tile on its page, 0 at the left edge
    uint32_t row = 0;     // the row of the tile on its page, 0 at the top edge
};

/** The billboard page, column and row of the slot numbered `index` (GODOT_PLANT_BILLBOARD_COLUMNS x ROWS a page). */
AtlasSlot billboard_slot(uint32_t index);

/** The view card page, column and row of the slot numbered `index` (GODOT_PLANT_VIEW_COLUMNS x ROWS a page). */
AtlasSlot view_slot(uint32_t index);

/**
 * The step of the bearing from a plant to the observer standing `dx`, `dz` metres from it on the ground: the angle
 * from +z towards +x counted in GODOT_PLANT_VIEW_ANGLE_STEP, rounded down. Its view tile is drawn again when the step
 * changes (PLAN Detailstufen: "der Blickwinkel sich um mehr als einen Schritt gedreht hat").
 */
int32_t view_step(float dx, float dz);

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the record keys of the level port the adapter reads
 * @description The graded level of a plant body (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen;
 *              usecase:101657 grade plants by picture height): ase-render-data grades every plant body by the height
 *              of its picture (RND_LOD_LEVEL, RND_LOD_FADE under the body, the codes RND_CONST_LOD_LEVEL_* on GLOBAL),
 *              ase-pl-flora files one record per graded patch on its port flora.lod.v1, its object id the patch and
 *              its five values level, fade, view, tile and cnpy (FLORA_HOST_PORT_LOD and the keys of ase-pl-flora
 *              types/lod/types.hpp, repeated here because the adapter reads the port and never links the plugin).
 *              GodotHostResourceManager merges them into VivariumPatchView; the plant scene hands the body over to
 *              its card by them (level_share, godot_plant_impostor.hpp), never by a distance rule of its own.
 *              LOD 5, the canopy of the whole (usecase:100387 view the plant): ase-pl-flora files one record per patch
 *              whose raster cell reports a canopy on flora.cnpy.v1, its three values cnpy_lai, cnpy_height and
 *              cnpy_ext (FLORA_HOST_PORT_CNPY, ase-pl-flora types/cnpy/types.hpp); the plant scene shows a canopy
 *              dome by the share handed to the canopy, thinned by the cover of the cell (canopy_cover).
 *              LOD 4, the billboard: every plant takes one slot on a shared atlas page of the scene
 *              (GODOT_PLANT_BILLBOARD_ATLAS, billboard_slot); a page draws the view cards of all its plants into
 *              tiles of 16 x 32 pixels in one pass, so 10 000 birches need two render targets, not 10 000.
 *              LOD 3, the view card: the same slot places the body of the plant in a tile of 32 x 64 pixels on a
 *              view page, one viewport of its own world per page (GODOT_PLANT_VIEW_PAGE, view_slot), turned to the
 *              step of its bearing to the observer (view_step); a fixed number of tiles is drawn per frame
 *              (GODOT_PLANT_VIEW_TILE_BUDGET), and the billboard tile is drawn from the view tile.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter port/types.hpp and scene/types.hpp are included by more
 *              than a hundred own units of the avt build and take no new line (header ratchet, operator decision
 *              2026-11-21). These names are read by the level reader of GodotHostResourceManager and its test only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @usecase     usecase:101657 - grade plants by picture height
 * @actor       Picture Height Grader (PICTHGTGRA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: lod = level of detail, the graded level of a plant body; view = the level of the view card,
 * tile = the level of the billboard, cnpy = the level of the canopy, each as the code ase-render-data grades with;
 * atlas = the shared pages the view and billboard tiles of all plants stand on, slot = the place of one tile on them,
 * step = the bearing to the observer counted in GODOT_PLANT_VIEW_ANGLE_STEP.
 */
constexpr const char* GODOT_KEY_LOD_LEVEL = "lod_level";  // level record: the graded level of the plant body of the patch, one of the RND_CONST_LOD_LEVEL_* codes
constexpr const char* GODOT_KEY_LOD_FADE = "lod_fade";    // level record: the share handed over to the next coarser level, RND_CONST_LOD_FADE_NONE..RND_CONST_LOD_FADE_FULL
constexpr const char* GODOT_KEY_LOD_VIEW = "lod_view";    // level record: RND_CONST_LOD_LEVEL_VIEW, the level from which the view card stands in for the body
constexpr const char* GODOT_KEY_LOD_TILE = "lod_tile";    // level record: RND_CONST_LOD_LEVEL_TILE, the level of the billboard
constexpr const char* GODOT_KEY_LOD_CNPY = "lod_cnpy";    // level record: RND_CONST_LOD_LEVEL_CNPY, the level of the canopy
constexpr double GODOT_LOD_CODE_STEP = 1.0;               // distance of two neighbouring codes of RND_CONST_LOD_LEVEL_*: FULL 0 to CNPY 5 follow each other by one (hub_constants.json), so the level whose fade leads into the view level lies one step below it
constexpr uint32_t GODOT_PLANT_DRAWN_PARTS = 7u;          // drawing nodes of a plant whose share the level sets: the fine and the coarse bark tube and the five instance nodes (leaves, coarse leaves, male, female, fruit)
constexpr const char* GODOT_PLANT_BILLBOARD = "Billboard";            // node name: the billboard of the individual (LOD 4), a Sprite3D turning about y to the camera like the view card
constexpr int32_t GODOT_PLANT_BILLBOARD_WIDTH = 16;       // pixels: width of the billboard tile of LOD 4 (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN, Detailstufen: a tile of 16 x 32 pixels drawn from the view card)
constexpr int32_t GODOT_PLANT_BILLBOARD_HEIGHT = 32;      // pixels: height of the billboard tile of LOD 4
constexpr const char* GODOT_PLANT_BILLBOARD_ATLAS = "BillboardAtlas"; // node name: the node under the plant scene that holds the shared billboard pages of all plants
constexpr const char* GODOT_PLANT_BILLBOARD_PAGE = "BillboardPage";   // node name prefix: one 2D viewport per atlas page, its page number appended
constexpr const char* GODOT_PLANT_BILLBOARD_TILE = "BillboardTile";   // node name prefix: the TextureRect of one slot on its page, the slot number appended; it shows the view card texture of its plant
constexpr const char* GODOT_KEY_BILLBOARD_NEXT = "billboard_next";    // meta of the atlas node: the lowest slot number no plant has taken yet
constexpr const char* GODOT_KEY_BILLBOARD_FREE = "billboard_free";    // meta of the atlas node: the slot numbers plants gave back, taken again before a new one
constexpr const char* GODOT_KEY_BILLBOARD_SLOT = "billboard_slot";    // meta of a plant node: the slot its billboard tile stands on
constexpr const char* GODOT_KEY_BILLBOARD_FILLED = "billboard_filled"; // meta of a page: the process frame a tile of it was last filled or given back in, -1 once the page drew it
constexpr int32_t GODOT_PLANT_BILLBOARD_PAGE_SIZE = 2048; // pixels: side of one billboard atlas page (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN, Detailstufen: atlas pages of 2048 x 2048)
constexpr uint32_t GODOT_PLANT_BILLBOARD_COLUMNS = static_cast<uint32_t>(GODOT_PLANT_BILLBOARD_PAGE_SIZE / GODOT_PLANT_BILLBOARD_WIDTH);  // tiles side by side on one page: 128
constexpr uint32_t GODOT_PLANT_BILLBOARD_ROWS = static_cast<uint32_t>(GODOT_PLANT_BILLBOARD_PAGE_SIZE / GODOT_PLANT_BILLBOARD_HEIGHT);    // rows of tiles on one page: 64, so one page holds 8192 billboards
constexpr const char* GODOT_PLANT_VIEW_PAGE = "ViewPage";      // node name prefix: one viewport of its own world per view card page (LOD 3) under the atlas node, its page number appended
constexpr const char* GODOT_PLANT_VIEW_TILE = "ViewTile";      // node name prefix: the node in the world of a view page that carries the seven drawing nodes of one slot, the slot number appended
constexpr const char* GODOT_KEY_VIEW_FRAME = "view_frame";     // meta of a plant node: the box of its body as width, height, depth and the centre x, y, z, kept so its tile can be drawn again
constexpr const char* GODOT_KEY_VIEW_STEP = "view_step";       // meta of a plant node: the step of the bearing to the observer its view tile was last drawn from
constexpr const char* GODOT_KEY_VIEW_QUEUE = "view_queue";     // meta of the atlas node: the instance ids of the plants whose view tile waits to be drawn, oldest first
constexpr const char* GODOT_KEY_VIEW_QUEUED = "view_queued";   // meta of a plant node: present while the plant stands in the view queue, so it stands there once
constexpr int32_t GODOT_PLANT_VIEW_WIDTH = 32;                 // pixels: width of the view card tile of LOD 3 (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN, Detailstufen: tiles of 32 x 64 pixels)
constexpr int32_t GODOT_PLANT_VIEW_HEIGHT = 64;                // pixels: height of the view card tile of LOD 3
constexpr uint32_t GODOT_PLANT_VIEW_COLUMNS = static_cast<uint32_t>(GODOT_PLANT_BILLBOARD_PAGE_SIZE / GODOT_PLANT_VIEW_WIDTH);   // view tiles side by side on one page: 64
constexpr uint32_t GODOT_PLANT_VIEW_ROWS = static_cast<uint32_t>(GODOT_PLANT_BILLBOARD_PAGE_SIZE / GODOT_PLANT_VIEW_HEIGHT);     // rows of view tiles on one page: 32, so one page holds 2048 view cards
constexpr uint32_t GODOT_PLANT_VIEW_TILE_BUDGET = GODOT_PLANT_VIEW_COLUMNS;  // view tiles drawn per frame, a drawing budget of the adapter and no cap of the simulation: one page row, so the 2800 tiles of the memory proof (PLAN Speicherbeweis) are all drawn again in 44 frames, far inside one bearing step of a walking observer
constexpr float GODOT_PLANT_VIEW_ANGLE_STEP = 0.17453292f;     // radians: the step of the bearing from a plant to the observer past which its view tile is drawn again, 10 degrees (PLAN Detailstufen: at 400 m the bearing turns 10 degrees when the observer walks 70 m)
constexpr float GODOT_PLANT_VIEW_EYE_Z = 1.0f;                 // tile units: the camera of a view page stands this far in front of the plane of its tiles; a body fits its tile at most one unit deep, so it spans -0.5 to 0.5
constexpr float GODOT_PLANT_VIEW_NEAR = 0.25f;                 // tile units: near plane of the camera of a view page, so the front of a body at 0.5 lies inside its depth range
constexpr float GODOT_PLANT_VIEW_FAR = 2.0f;                   // tile units: far plane of the camera of a view page, so the back of a body at -0.5 lies inside its depth range
constexpr const char* GODOT_KEY_CNPY_LAI = "cnpy_lai";        // canopy record (flora.cnpy.v1): GIS_CANOPY_LAI of the raster cell under the patch, m2 of leaf over m2 of ground
constexpr const char* GODOT_KEY_CNPY_HEIGHT = "cnpy_height";  // canopy record: GIS_CANOPY_HEIGHT of that cell, crown height of its tallest stand in metres
constexpr const char* GODOT_KEY_CNPY_EXT = "cnpy_ext";        // canopy record: PLT_CONST_CNPY_LIGHT_EXT, the extinction coefficient of the cover 1 - exp(-ext * lai)
constexpr const char* GODOT_PLANT_CANOPY = "PlantCanopy";     // node name: the canopy dome of the patch (LOD 5), textured with the view card and covering by the cover of its raster cell
constexpr float GODOT_PLANT_CANOPY_DOME = 0.5f;               // height of the canopy dome as a share of its radius: the crown layer of a stand is flatter than it is wide

}  // namespace ase::adp::godot

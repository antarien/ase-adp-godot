#pragma once

/**
 * ASE GODOT ADAPTER - HOST FIGURE VIEW
 *
 * @file        godot_host_figure_view.hpp
 * @brief       The figures of the embedded host as the last snapshot of the body, camera, gear, boundary and code ports showed them
 * @description The copy GodotHostResourceManager keeps of every record ase-pl-avatar lays for a figure: its body,
 *              the player camera following it, the pieces it carries, its bumps against the chunk edge and the
 *              meaning of the coded values (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE). One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that draw,
 *              lead or show a figure read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * One figure as the last snapshot of the body port showed it - a copy (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE
 * 07.8, ase-pl-avatar). ase-player moves the figure; the plugin lays its place with the height as its own
 * axis, and the view takes the three axes in Godot's order (east, up, north) and computes none of them.
 */
struct VivariumFigureView {
    uint32_t object_id = 0;   // the figure, as the plugin names it on its port
    double   x = 0.0;         // world metres east
    double   y = 0.0;         // world metres up, the height ase-player holds
    double   z = 0.0;         // world metres north
    double   course = 0.0;    // course of the figure in radians (HUB_PLR_MOTN_YAW)
    double   vel_x = 0.0;     // horizontal velocity east, m/s
    double   vel_z = 0.0;     // horizontal velocity north, m/s
    double   gait = 0.0;      // gait value of the plugin (stand, walk, run) the clips blend by
    double   soul = 0.0;      // soul value of the plugin: AI/PC or SS/PC, the button soul.mode shows it
    double   camera = 0.0;    // camera value of the plugin: fixed view or the camera following this figure
    double   lod_near = 0.0;  // frame height share from which level 0 of the body is drawn (RND_CONST_FIGURE_LOD_NEAR)
    double   lod_mid = 0.0;   // frame height share from which level 1 is drawn (RND_CONST_FIGURE_LOD_MID)
    double   lod_far = 0.0;   // frame height share from which level 2 is drawn, below it level 3 (RND_CONST_FIGURE_LOD_FAR)
};

/**
 * The player camera as the last snapshot of the camera port showed it (07.5): eye and aim of the camera
 * ase-camera computes obliquely above the figure it follows, in Godot's order. known is false while no
 * camera follows a figure - the view then draws its fixed start view of Phase 06.
 */
struct VivariumFigureCameraView {
    bool     known = false;   // true while the port carried the record of the active player camera
    uint32_t figure = 0;      // the figure the camera follows
    double   eye_x = 0.0;     // eye, world metres east
    double   eye_y = 0.0;     // eye, world metres up
    double   eye_z = 0.0;     // eye, world metres north
    double   aim_x = 0.0;     // look-at, world metres east
    double   aim_y = 0.0;     // look-at, world metres up
    double   aim_z = 0.0;     // look-at, world metres north
    bool     bound = false;   // true while ase-camera holds the camera on the rim of the loaded area (07-G, cam.bound)
    bool     edge = false;    // true while ase-physics holds the followed figure at a chunk border (HUB_PHYS_STAND_BNDY, RAND DES AREALS)
};

/**
 * One piece a figure carries as the last snapshot of the gear port showed it (07.10, usecase:101625 carry a
 * tool in hand): a row of ase-equipment, its bearer, its slot and its kind as catalog ids
 * (KernelEmbeddedHost::name_id). The view hangs the tool of the kind under the socket the slot names. A vessel
 * (07.14, usecase:100367 water a patch) closes its record with the litres ase-equipment holds in it
 * (HUB_EQP_ITEM_WATER_L); the HUD's can draws that level, so a pour of the AI empties the can the player sees.
 */
struct VivariumFigureGearView {
    uint32_t object_id = 0;  // the piece, as the plugin names it on its port
    uint32_t bearer = 0;     // the figure that carries it, the object id of its body record
    uint32_t slot = 0;       // the slot it sits in: name_id of the socket (socket_hand_r)
    uint32_t kind = 0;       // what it is: name_id of the item key of Phase 06 (pa, enxada, faca)
    bool     vessel = false; // true while the record carried water_l: the piece is a vessel of ase-equipment
    double   water_l = 0.0;  // litres the vessel holds, HUB_EQP_ITEM_WATER_L (AVATAR_HOST_KEY_WATER)
    double   mm_per_l = 0.0; // millimetres one litre lays on the roots in its bearer's hand, HUB_EQP_ITEM_POUR_MM_PER_L
                             // (AVATAR_HOST_KEY_POUR, N0517); 0 while the vessel's record carries no column yet
};

/**
 * The last bump of one walking body against the edge of the first own chunk as the last snapshot of the boundary
 * port showed it (07.13, usecase:101612 keep figure in loaded area). ase-physics holds the body inside the chunk
 * borders and files the bump (HUB_PHYS_BNDY_HIT); ase-pl-avatar lays it out every Production run
 * (AvatarHostBndyExptSystem). The view flashes the red glass pane whenever bumps rises over the count it saw last;
 * the edge belongs to the terrain, so a body the record names that the view draws no figure for still bumps.
 */
struct VivariumFigureBumpView {
    uint32_t object_id = 0;  // the walking body, the object id of its body record when it is a figure
    double   normal_x = 0.0; // east share of the outward normal of the edge: -1 west edge, 1 east edge, 0 neither (AVTR_HOST_KEY_NORMAL_X)
    double   normal_z = 0.0; // north share of the outward normal of the edge: -1 south edge, 1 north edge, 0 neither (AVTR_HOST_KEY_NORMAL_Z)
    double   touch_x = 0.0;  // the touched point on the edge, world metres east, Godot's x (AVTR_HOST_KEY_TOUCH_X)
    double   touch_z = 0.0;  // the touched point on the edge, world metres north, Godot's z (AVTR_HOST_KEY_TOUCH_Z)
    uint32_t bumps = 0;      // bumps of this body against the edge since it walks (AVTR_HOST_KEY_BUMPS)
};

/**
 * The meaning of the coded values of the body record as the last snapshot of the code port showed it (07.4 and
 * 07.6, usecase:101611 animate character motion): the gait points the clips blend at, the soul codes and the
 * camera codes, all as ase-pl-avatar defines them (AvatarHostCodeExptSystem). read_figures reads the port anew
 * with every snapshot, and the view compares gait, soul and camera of a figure against these values, never
 * against constants of the adapter. known is false while no code record stood - the view then draws no gait and
 * leads no figure.
 */
struct VivariumFigureCodeView {
    bool   known = false;      // true while the port carried the one code record with all seven values
    double gait_stand = 0.0;   // gait value of a standing figure, the lower end of the clip mix
    double gait_walk = 0.0;    // gait value of a walking figure
    double gait_run = 0.0;     // gait value of a running figure, the upper end of the clip mix
    double soul_ai = 0.0;      // soul value while the AI leads the figure (AI/PC)
    double soul_player = 0.0;  // soul value while its player leads the figure (SS/PC)
    double cam_fixed = 0.0;    // camera value while the fixed view of Phase 06 is drawn
    double cam_player = 0.0;   // camera value while the player camera follows the figure
};

}  // namespace ase::adp::godot

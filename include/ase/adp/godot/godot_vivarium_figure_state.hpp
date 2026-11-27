#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW: THE FIGURE STATE
 *
 * @file        godot_vivarium_figure_state.hpp
 * @brief       VivariumFigureState - what the view holds of the figures it shows and of the controls that lead one
 * @description PLAN_ASE_VIVARIUM_PHASE_07 07.4 to 07.13: the figure file or its stand-in, the red glass pane of the
 *              chunk edge, the edges the player camera and the figure stood at last frame, the figures bound to
 *              their area plate and given their start outfit, the keys held last frame, the hand's last request
 *              and the phone's stick, orbit drag and cam.mode.
 *
 *              ONE RESPONSIBILITY, ITS OWN HEADER: VivariumView (godot_vivarium_node.hpp) holds this record as its
 *              member figure_state_ (composition, the validator forbids inheritance), so its member functions in
 *              godot_vivarium_figure.cpp, ..._figure_controls.cpp, ..._figure_input.cpp and the HUD files read
 *              these members as figure_state_.<name>. A new member of the
 *              figure's state is written here, never into the view's header (operator order 2026-11-25: the
 *              view's header stood at the limit of 500 lines).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    R0221 (the split of the view's header)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_glass.hpp>
#include <ase/adp/godot/godot_figure_scene.hpp>
#include <ase/adp/godot/godot_vivarium_figure_controls.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** The figures of the care view and the controls that lead the seat's figure (07.4 to 07.13). */
struct VivariumFigureState {
    FigureSceneLook          figure_look_;        // the figure file of the stage or its stand-in material, bound once at the start (07.9)
    FigureSceneGlass         figure_glass_;       // the red glass pane of the chunk edge, SHA's measures and colour, bound with the HUD (07.13, usecase:101612)
    bool                     figure_view_shown_ = false;   // the scene's camera stood at the player camera's eye last frame (07.6)
    bool                     figure_edge_shown_ = false;   // ase-physics held the followed figure at a chunk border last frame (07-G, RAND DES AREALS)
    bool                     figure_bound_shown_ = false;  // ase-camera held the player camera on the rim of the area last frame (07-G, cam.bound, usecase:101612)
    std::vector<uint32_t>    figure_area_bound_;           // the figures bound to the area plate they first stood on (07.7), each once
    std::vector<uint32_t>    figure_stocked_;              // the figures of this seat given their start outfit (T.8 stock, N0590 pack), each once
    bool                     figure_switch_held_ = false;  // the key of cam.mode was held last frame: one switch per press
    bool                     figure_steered_ = false;      // the stick was off its rest last frame: a released stick is sent once at rest
    bool                     figure_zoomed_ = false;       // a zoom key was held last frame: the eye the rail settled at is noted once on release (07.6)
    bool                     figure_hand_sent_ = false;    // a hold or drop for the figure's hand went out since its figure was led (07.10)
    uint32_t                 figure_hand_figure_ = 0;      // the figure the last hold or drop named
    uint32_t                 figure_hand_item_ = 0;        // the item the last hold named, 0 after a drop: one request per change of the hand
    VivariumFigureControls   figure_controls_;             // the phone's stick, orbit drag and cam.mode (07.9)
};

}  // namespace ase::adp::godot

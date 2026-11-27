#pragma once

/**
 * ASE GODOT ADAPTER - HUD COMPOSE SHARE
 *
 * @file        godot_hud_compose_share.hpp
 * @brief       HudDrawShare and HudDrawStep: the steps of VivariumHud::draw and the CPU time each took (P0692)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5 and register entry P0692. The view's hud_draw_split line
 *              named the drawing of the HUD as one number of about 28 ms per frame; to see where the time goes,
 *              VivariumHud::draw writes one HudDrawShare per step into its shares, and the view prints them after its
 *              own share (godot_vivarium_hud.cpp). The steps before the layers - the shade under the birch, the world
 *              group, the tool place in the world, the holes, the veil and the screen shade - and the parts of the
 *              layer inventory are named here by HudDrawStep; every layer of HudShown::layers is named by itself.
 *
 *              THE PARTS OF THE INVENTORY (P0695) are written before the layer's own share, which still names their
 *              sum; the parts of the hand come from HandSlot::cost() and are written before the hand's own share, the
 *              lines of the hand's text and the shaping in all of them are the parts of inventory_hand_text.
 *
 *              The record measures, it never decides: nothing in the drawing reads a share back.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    P0692, P0695
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/** One step of VivariumHud::draw and the main thread's CPU time it took. */
struct HudDrawShare {
    std::string step;
    uint64_t    cpu_usec = 0;
};

/** The names of the steps of VivariumHud::draw that are no layer of HudShown::layers. */
struct HudDrawStep {
    // The steps before the layers.
    static constexpr const char* TREE_SHADE = "tree_shade";  // ShadeTree: the shade under the birch
    static constexpr const char* WORLD = "world";            // GroundThings: the things on the ground
    static constexpr const char* RACK_WORLD = "rack_world";  // ToolRack: the tool place in the world
    static constexpr const char* HOLES = "holes";            // the tap hole and the sow hole
    static constexpr const char* VEIL = "veil";              // WeatherVeil
    static constexpr const char* SHADE = "shade";            // ScreenShade
    // The parts of the layer inventory (P0695).
    static constexpr const char* INV_HAND = "inventory_hand";      // HandSlot
    // The parts of the hand, from HandSlot::cost().
    static constexpr const char* INV_HAND_LOOK = "inventory_hand_look";  // look() and sprite()
    static constexpr const char* INV_HAND_ART = "inventory_hand_art";    // hexagon and drawing
    static constexpr const char* INV_HAND_STOW = "inventory_hand_stow";  // count and stow button
    static constexpr const char* INV_HAND_MODE = "inventory_hand_mode";  // HandMode::draw
    static constexpr const char* INV_HAND_TEXT = "inventory_hand_text";  // name, hint, case, reading
    // The lines of the hand's text and the shaping in all of them: the parts of inventory_hand_text.
    static constexpr const char* INV_HAND_TITLE = "inventory_hand_title";      // the item's name
    static constexpr const char* INV_HAND_HINT = "inventory_hand_hint";        // the instruction
    static constexpr const char* INV_HAND_CASE = "inventory_hand_case";        // the case words
    static constexpr const char* INV_HAND_READING = "inventory_hand_reading";  // label and value
    static constexpr const char* INV_HAND_VALUE = "inventory_hand_value";      // of the reading: value
    static constexpr const char* INV_HAND_SHAPE = "inventory_hand_shape";      // shaping, every line
    static constexpr const char* INV_HAND_WARM = "inventory_hand_warm";        // readying, every line
    static constexpr const char* INV_BELT = "inventory_belt";      // ToolBelt
    static constexpr const char* INV_BAG = "inventory_bag";        // BagIcon
    static constexpr const char* INV_BARS = "inventory_bars";      // ActivityBars (usecase:101305, MIG_ASE_SKILL_RSM B5)
    static constexpr const char* INV_TOAST = "inventory_toast";    // Toast
    static constexpr const char* INV_WINDOW = "inventory_window";  // BagWindow: the pack and the pouch
    static constexpr const char* INV_SELECT = "inventory_select";  // SelectLabel
};

}  // namespace ase::adp::godot

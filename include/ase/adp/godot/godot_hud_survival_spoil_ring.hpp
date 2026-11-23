#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL SPOIL RING
 *
 * @file        godot_hud_survival_spoil_ring.hpp
 * @brief       The spoil ring ESTRAGO around the food of the bag, drawn as the survival mockup's food(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U07 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 234 to 235, element S7). While the bag carries food the mockup draws, in one group fading in
 *              with the view (class fade):
 *
 *                arc      radius radius about (centre_x, centre_y), from the top clockwise over the share spoil of
 *                         a turn, stroked at line in DOMAIN_GROUND: the ring closes as the food spoils
 *                word     survival.food.spoil at (word_x, word_y), size word_size, anchored middle, in DOMAIN_GROUND
 *
 *              Every name above stands for hud_survival_food_<name>. The spoil of food is the owner's: organic
 *              decomposition belongs to ase-entropy (PLAN_ASE_ENTROPY, EntropyDcyRotTag), which carries the decay of
 *              litter, deadwood and the seed bank today and none of food. The ring draws what the frame says; it never
 *              asks a port (the bridge writes the frame, gap N0530 under the carrier N0448).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528 (survival rows of the inventory), N0530 (survival facets without a value), carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>

namespace ase::adp::godot {

/** The food of the bag for the spoil ring, written by the bridge from the owner's port or the SHA defaults. */
struct SurvivalSpoilFrame {
    static constexpr const char* GAP_SPOIL = "N0530";  // the bridge gap: no survival.food.v1 offered, or no spoil on it
    bool shown = false;  // st.food != null: the bag carries food, the ring shows
    double spoil = 0.0;  // st.food: the share the food has spoiled, 0 fresh to 1 spoiled through
};

/** The spoil ring around the food of the bag. */
class SurvivalSpoilRing {
public:
    /** Binds every hud_survival_food_* token, the ring drawer, the ground ink and the word survival.food.spoil. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The ring of `food` for this frame; false with the reason. */
    bool set(const SurvivalSpoilFrame& food, std::string& failed);

    /** Draws the ring under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

private:
    const UiTheme* theme_ = nullptr;
    UiMotion fade_;              // hud_anim_fade: the group's fade-in
    UiRing ring_;                // the arc drawer of the HUD's rings
    UiTextBlock word_;           // survival.food.spoil
    ::godot::Color ground_;      // DOMAIN_GROUND: arc and word
    double centre_x_ = 0.0;      // hud_survival_food_centre_x: cx
    double centre_y_ = 0.0;      // hud_survival_food_centre_y: cy
    double radius_ = 0.0;        // hud_survival_food_radius: r
    double line_ = 0.0;          // hud_survival_food_line: stroke-width
    double word_x_ = 0.0;        // hud_survival_food_word_x
    double word_y_ = 0.0;        // hud_survival_food_word_y
    double word_size_ = 0.0;     // hud_survival_food_word_size
    bool shown_ = false;         // st.food != null of this frame
    double spoil_ = 0.0;         // st.food of this frame
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL AIR MARK
 *
 * @file        godot_hud_survival_air_mark.hpp
 * @brief       The air mark AR TÓXICO in the sky band under the wind, drawn as the survival mockup's air(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U06 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 233, element S6). While the air is worse than normal the mockup draws, in one group fading in
 *              with the view (class fade):
 *
 *                glyph    the smoke glyph hud_survival_fire_glyph_smoke at (glyph_x, glyph_y), size glyph_size,
 *                         stroked at glyph_line in DOMAIN_ALERT
 *                word     survival.air.toxic at (word_x, word_y), size word_size, in DOMAIN_ALERT
 *
 *              Every name above stands for hud_survival_air_<name>. The rating of the air is the owner's:
 *              AtmoSimGasBthSystem of ase-atmosphere publishes ATM_GAS_BTH_STATE, the protocol constant of normal
 *              air stands in hub_constants.json. The mark draws what the frame says; it never asks a port (the bridge
 *              writes the frame, gap N0530 under the carrier N0448).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528 (survival rows of the inventory), N0530 (survival facets without a value), D2500 (the glyph also
 *              stands while soot of GIS_LOCAL_SOOT hangs in the seat's cell, the facet Rauch), carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The rating of the air for the air mark, written by the bridge from the owner's port or the SHA defaults. */
struct SurvivalAirFrame {
    static constexpr const char* GAP_AIR = "N0530";     // the bridge gap: no survival.body.v1 record or no breath on it
    static constexpr const char* GAP_SOOT = "D2500";    // the bridge gap: no port carries GIS_LOCAL_SOOT
    bool toxic = false;  // st.air: the owner rates the air worse than normal, the mark shows
    double soot = 0.0;   // GIS_LOCAL_SOOT of the seat's figure (survival.smoke.v1): kg of soot in the air of its cell; the plume stands above 0
};

/** The air mark in the sky band. */
class SurvivalAirMark {
public:
    /** Binds every hud_survival_air_* token, the smoke glyph, the alert ink and the word survival.air.toxic. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The mark of `air` for this frame; false with the reason. */
    bool set(const SurvivalAirFrame& air, std::string& failed);

    /** Draws the mark under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

private:
    const UiTheme* theme_ = nullptr;
    UiMotion fade_;                     // hud_anim_fade: the group's fade-in
    UiTextBlock word_;                  // survival.air.toxic
    std::vector<UiSvgOutline> glyph_;   // hud_survival_fire_glyph_smoke: GL.smoke
    ::godot::Color alert_;              // DOMAIN_ALERT: glyph and word
    double glyph_x_ = 0.0;              // hud_survival_air_glyph_x
    double glyph_y_ = 0.0;              // hud_survival_air_glyph_y
    double glyph_size_ = 0.0;           // hud_survival_air_glyph_size: k
    double glyph_line_ = 0.0;           // hud_survival_air_glyph_line
    double word_x_ = 0.0;               // hud_survival_air_word_x
    double word_y_ = 0.0;               // hud_survival_air_word_y
    double word_size_ = 0.0;            // hud_survival_air_word_size
    bool toxic_ = false;                // st.air of this frame
    bool smoky_ = false;                // the plan's facet Rauch (T.8): soot hangs in the air of the seat's cell this frame
};

}  // namespace ase::adp::godot

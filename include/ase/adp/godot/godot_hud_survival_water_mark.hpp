#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL WATER MARK
 *
 * @file        godot_hud_survival_water_mark.hpp
 * @brief       The water mark at the bag: the drop, or ÁGUA TURVA while the figure is poisoned, as the mockup's water(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U08 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 236, element S8). The mockup draws one of two forms:
 *
 *                calm     the drop hud_survival_glyph_thirst at (x, y), size drop_size, stroked at
 *                         hud_survival_glyph_line in OVERLAY_INK_FAINT (--hud3), outside any fade
 *                poison   in one group fading in with the view (class fade): the same drop at size poison_size in
 *                         DOMAIN_ALERT with the cross hud_rail_state_poison_mark stroked at
 *                         hud_rail_state_poison_mark_line (GL.poison), and survival.water.murky at
 *                         (word_x, word_y), size word_size, in DOMAIN_ALERT
 *
 *              Every name above without a prefix stands for hud_survival_water_<name>. The form follows the sign
 *              poison of the figure (st.sign.includes('poison')): the mark reads SurvivalBodyFrame::poisoned, the
 *              same state the rail SINAIS draws (TAG_MTB_POISONED of ase-metabolism), so the poison state has one
 *              place in the frame and the mark never asks a port.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528 (survival rows of the inventory), N0530 (survival facets without a value), N0592 (the toadstool
 *              carries no gustatory signature yet, so no meal sets TAG_MTB_POISONED and the poison form has no case),
 *              carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_body_rail.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The water mark at the bag. */
class SurvivalWaterMark {
public:
    /** Binds every hud_survival_water_* token, the drop and the cross, both inks and the word survival.water.murky. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The form of the mark for the figure's `body` of this frame; false with the reason. */
    bool set(const SurvivalBodyFrame& body, std::string& failed);

    /** Draws the mark under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

private:
    const UiTheme* theme_ = nullptr;
    UiMotion fade_;                     // hud_anim_fade: the poison group's fade-in
    UiTextBlock word_;                  // survival.water.murky
    std::vector<UiSvgOutline> drop_;    // hud_survival_glyph_thirst: GL.thirst
    std::vector<UiSvgOutline> cross_;   // hud_rail_state_poison_mark: the cross of GL.poison
    ::godot::Color faint_;              // OVERLAY_INK_FAINT (--hud3): the calm drop
    ::godot::Color alert_;              // DOMAIN_ALERT: the poisoned drop, its cross and the word
    double x_ = 0.0;                    // hud_survival_water_x
    double y_ = 0.0;                    // hud_survival_water_y
    double drop_size_ = 0.0;            // hud_survival_water_drop_size: k of the calm drop
    double poison_size_ = 0.0;          // hud_survival_water_poison_size: k of GL.poison
    double drop_line_ = 0.0;            // hud_survival_glyph_line: the drop's stroke
    double cross_line_ = 0.0;           // hud_rail_state_poison_mark_line: the cross's stroke
    double word_x_ = 0.0;               // hud_survival_water_word_x
    double word_y_ = 0.0;               // hud_survival_water_word_y
    double word_size_ = 0.0;            // hud_survival_water_word_size
    bool poisoned_ = false;             // st.sign.includes('poison') of this frame
};

}  // namespace ase::adp::godot

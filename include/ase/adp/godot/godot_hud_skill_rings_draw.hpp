#pragma once

/**
 * ASE GODOT ADAPTER - HUD SKILL RINGS
 *
 * @file        godot_hud_skill_rings_draw.hpp
 * @brief       The two skill rings of the game HUD: DCD above, ACD below, the remaining time of the learning entries
 * @description MIG_ASE_SKILL_RSM step B5, usecase:101305 show skill progress. The prototype draws two wheels on the
 *              right of its screen (PercentageWheel at 1340,180 for DCD and at 1340,300 for ACD,
 *              PLAN_ASE_VIVARIUM_RSM_REF "zwei Kreisanzeigen rechts"): the general remaining time of what the figure
 *              learns, as important as the bars at the foot (operator order 2026-11-25). They stand in the game HUD,
 *              never in the debug drawer, drawn on the layer skill that every mode of hud.modes names after its band:
 *
 *                track    the full ring of radius r about (x, <ring>_y), OVERLAY_INK_FAINT at line
 *                arc      from the top clockwise over the share of a turn, DOMAIN_TIME at line: the ring closes as
 *                         the time of its row fills
 *                per cent texts.hud.skill_ring.percent at (x, <ring>_y + pct_dy), data face pct_size, middle,
 *                         OVERLAY_INK
 *                word     texts.hud.skill_ring.<ring> at (x + word_dx, <ring>_y + word_dy), label face word_size,
 *                         anchored at its end, OVERLAY_INK
 *
 *              Every name above stands for hud_skill_ring_<name>; the ring DCD is GODOT_RSM_RING_DCD, ACD is
 *              GODOT_RSM_RING_ACD.
 *
 *              THE PLACE IS MEASURED, never taken from a neighbour (theme.css, THE SKILL RINGS): on the frame of the
 *              top band the corner x 304 to 356, y 42 to 104 is free of the arc's rings, its hit and its readout, the
 *              battery, the figure's mark and the body rail CORPO; the two rings fill it at x 340.
 *
 *              THE SHARES ARE THE OWNER'S: DCD is the largest dcd_shr, ACD the largest acd / acd_max over the learning
 *              entries of the seat's figure that avatar.skill.v1 laid this frame (debug_rsm_shares); the rings count
 *              nothing. A frame whose port names no entry keeps the shares drawn last, as the timer table keeps its
 *              last set; before the first learning order both rings stand at 0 %.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @category    ecs/module
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_text_place.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The shares of the two rings for this frame, written by the view from the learning entries of the seat's figure. */
struct SkillRingsFrame {
    bool known = false;  // the port named at least one learning entry of the seat's figure this frame
    double dcd = 0.0;    // DebugRsmShares::dcd: the largest share of the grace left, 0 to 1
    double acd = 0.0;    // DebugRsmShares::acd: the largest share of the fluctuation, 0 to 1
};

/** The two skill rings of the game HUD. */
class SkillRings {
public:
    /** Binds every hud_skill_ring_* token, the ring drawer, the tones and the words; false with the missing name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws both rings under `frame`, `elapsed` seconds after the view appeared; false with the reason. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkillRingsFrame& now, double elapsed, bool still,
              std::string& failed);

    /** The share ring `which` (GODOT_RSM_RING_*) shows now: the frame's while its port names an entry, else the last. */
    double shown(uint32_t which) const;

private:
    /** Ring `which` at its centre with its share, its per cent and its word; false with the reason. */
    bool ring(UiPen& canvas, const ::godot::Transform2D& frame, uint32_t which, const UiTextPlace& text,
              std::string& failed);

    const UiTheme* theme_ = nullptr;
    UiRing ring_;                       // the arc drawer of the HUD's rings
    double x_ = 0.0;                    // hud_skill_ring_x: the centre of both rings
    double dcd_y_ = 0.0;                // hud_skill_ring_dcd_y: the centre of the upper ring
    double acd_y_ = 0.0;                // hud_skill_ring_acd_y: the centre of the lower ring
    double r_ = 0.0;                    // hud_skill_ring_r
    double line_ = 0.0;                 // hud_skill_ring_line
    double word_dx_ = 0.0;              // hud_skill_ring_word_dx: the end of the word left of the centre
    double word_dy_ = 0.0;              // hud_skill_ring_word_dy
    double word_size_ = 0.0;            // hud_skill_ring_word_size
    double pct_dy_ = 0.0;               // hud_skill_ring_pct_dy
    double pct_size_ = 0.0;             // hud_skill_ring_pct_size
    ::godot::Color track_;              // OVERLAY_INK_FAINT: the full ring under the arc
    ::godot::Color time_;               // DOMAIN_TIME: the arc of the share
    ::godot::Color ink_;                // OVERLAY_INK: word and per cent
    std::vector<HudRun> percent_;       // texts.hud.skill_ring.percent: "{value} %"
    std::vector<UiTextBlock> words_;    // the word of each ring, by GODOT_RSM_RING_*
    std::vector<UiTextBlock> values_;   // the per cent of each ring, by GODOT_RSM_RING_*
    std::vector<double> held_;          // the share of each ring drawn last, by GODOT_RSM_RING_*
};

}  // namespace ase::adp::godot

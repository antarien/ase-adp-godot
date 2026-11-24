#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL REST CARD
 *
 * @file        godot_hud_survival_rest_card.hpp
 * @brief       The rest card SENTADO or DORMINDO on a leader line from the figure, drawn as the survival mockup's rest(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U05 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 224 to 230, element S5). While the figure rests the mockup draws, in one group fading in with
 *              the view (class fade), about the column bx = hud_survival_rest_column_x and the baseline
 *              ey = hud_survival_rest_elbow_y:
 *
 *                leader   a polyline from (anchor_x, anchor_y) over (elbow_x, ey) to (bx, ey) in DOMAIN_SKY
 *                title    survival.rest.sleep while the figure sleeps, survival.rest.sit otherwise, at
 *                         (bx, ey + title_dy), size title_size, in DOMAIN_SKY
 *                pose     the glyph sleep or sit at (bx + pose_dx, ey + pose_dy), size pose_size, stroked at
 *                         pose_line in DOMAIN_SKY; the sleep glyph carries the two letters survival.rest.snore at
 *                         snore_near and snore_far in units of the size
 *                fatigue  an arrow at bx + arrow_dx from ey + fatigue_tail_dy up to ey + fatigue_tip_dy, its head
 *                         head_dx beside and head_dy under the tip, at arrow_line in DOMAIN_SKY; the word
 *                         survival.rest.fatigue at (bx + word_dx, ey + fatigue_word_dy) in OVERLAY_INK_SOFT
 *                warmth   the same arrow from warmth_tail_dy to warmth_tip_dy in DOMAIN_CUT, the word
 *                         survival.rest.warmth at (bx + word_dx, ey + warmth_word_dy) in OVERLAY_INK_SOFT
 *
 *              Every name above stands for hud_survival_rest_<name>. The rest follows MTB_IS_RESTING of
 *              ase-metabolism (MetabolismRstSystem from BDI_CURRENT_ACTION), laid as is_resting on survival.rest.v1;
 *              whether the figure sits or sleeps is MTB_IS_SLEEPING, laid as sleeping on the same record (N0555).
 *              The card draws what the frame says; it never
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
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The rest of the figure for the rest card, written by the bridge from the owner's port or the SHA defaults. */
struct SurvivalRestFrame {
    static constexpr const char* GAP_REST = "N0530";  // the bridge gap: is_resting and sleeping (N0555) only on a record of survival.rest.v1
    bool shown = false;     // st.pose: the figure rests (MTB_IS_RESTING), the card shows
    bool sleeping = false;  // st.pose === 'sleep': DORMINDO with the glyph sleep, SENTADO with sit otherwise
};

/** The rest card at the figure on its leader line. */
class SurvivalRestCard {
public:
    /** Binds every hud_survival_rest_* token, the glyphs, the inks and the survival.rest.* words. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The card of `rest` for this frame: shown and pose; false with the reason. */
    bool set(const SurvivalRestFrame& rest, std::string& failed);

    /** Draws the card under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

private:
    /**
     * Strokes `outlines` scaled by `size` about `centre` in `color` at `width`, laid in the units of `frame`: the
     * mockup's GL[pose](x, y, k, c).
     */
    void stroke_glyph(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<UiSvgOutline>& outlines,
                      const ::godot::Vector2& centre, double size, const ::godot::Color& color, double width) const;

    /**
     * Draws one arrow of recovery at x = `x`, from `tail` up to `tip`, in `color`, laid in the units of `frame`: the
     * mockup's two L() of it.
     */
    void draw_arrow(UiPen& canvas, const ::godot::Transform2D& frame, double x, double tail, double tip,
                    const ::godot::Color& color) const;

    /** Draws the letter survival.rest.snore at `place` in `size` and `color`. */
    void draw_snore(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& place, double size,
                    const ::godot::Color& color, double elapsed, bool still);

    const UiTheme* theme_ = nullptr;
    UiLeaderLine line_;                     // the leader; the line width hud_line
    UiMotion fade_;                         // hud_anim_fade: the group's fade-in
    UiTextBlock sit_;                       // survival.rest.sit
    UiTextBlock sleep_;                     // survival.rest.sleep
    UiTextBlock snore_;                     // survival.rest.snore: the letter z of the sleep glyph
    UiTextBlock fatigue_;                   // survival.rest.fatigue
    UiTextBlock warmth_;                    // survival.rest.warmth
    std::vector<UiSvgOutline> sit_glyph_;   // hud_survival_rest_glyph_sit
    std::vector<UiSvgOutline> sleep_glyph_; // hud_survival_rest_glyph_sleep
    ::godot::Color soft_;                   // OVERLAY_INK_SOFT: --hud2, the arrows' words
    ::godot::Color sky_;                    // DOMAIN_SKY: leader, title, pose, the arrow of fatigue
    ::godot::Color cut_;                    // DOMAIN_CUT: the arrow of warmth
    double anchor_x_ = 0.0;                 // hud_survival_rest_anchor_x
    double anchor_y_ = 0.0;                 // hud_survival_rest_anchor_y
    double elbow_x_ = 0.0;                  // hud_survival_rest_elbow_x
    double elbow_y_ = 0.0;                  // hud_survival_rest_elbow_y: the baseline ey
    double column_x_ = 0.0;                 // hud_survival_rest_column_x: bx
    double title_dy_ = 0.0;                 // hud_survival_rest_title_dy
    double title_size_ = 0.0;               // hud_survival_rest_title_size
    double pose_dx_ = 0.0;                  // hud_survival_rest_pose_dx
    double pose_dy_ = 0.0;                  // hud_survival_rest_pose_dy
    double pose_size_ = 0.0;                // hud_survival_rest_pose_size: k
    double pose_line_ = 0.0;                // hud_survival_rest_pose_line
    double arrow_dx_ = 0.0;                 // hud_survival_rest_arrow_dx
    double arrow_line_ = 0.0;               // hud_survival_rest_arrow_line
    double head_dx_ = 0.0;                  // hud_survival_rest_head_dx
    double head_dy_ = 0.0;                  // hud_survival_rest_head_dy
    double fatigue_tip_dy_ = 0.0;           // hud_survival_rest_fatigue_tip_dy
    double fatigue_tail_dy_ = 0.0;          // hud_survival_rest_fatigue_tail_dy
    double fatigue_word_dy_ = 0.0;          // hud_survival_rest_fatigue_word_dy
    double warmth_tip_dy_ = 0.0;            // hud_survival_rest_warmth_tip_dy
    double warmth_tail_dy_ = 0.0;           // hud_survival_rest_warmth_tail_dy
    double warmth_word_dy_ = 0.0;           // hud_survival_rest_warmth_word_dy
    double word_dx_ = 0.0;                  // hud_survival_rest_word_dx
    double word_size_ = 0.0;                // hud_survival_rest_word_size
    double snore_near_dx_ = 0.0;            // hud_survival_rest_snore_near_dx: in units of k
    double snore_near_dy_ = 0.0;            // hud_survival_rest_snore_near_dy
    double snore_near_size_ = 0.0;          // hud_survival_rest_snore_near_size
    double snore_far_dx_ = 0.0;             // hud_survival_rest_snore_far_dx
    double snore_far_dy_ = 0.0;             // hud_survival_rest_snore_far_dy
    double snore_far_size_ = 0.0;           // hud_survival_rest_snore_far_size
    bool sleeping_ = false;                 // st.pose === 'sleep' of this frame
    bool shown_ = false;                    // st.pose of this frame
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - HUD FIGURE FOCUS
 *
 * @file        godot_hud_figure_focus.hpp
 * @brief       The focus of the seat's figure: its reach, the comb soul.mode over the head, the spine of its soul
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.9 and 07.11 (VIVARIUM_PHASE_07_FIGURE_V1, fig.focus), drawn in
 *              the grammar of Phase 06 zone Z2 without its ring: information about a thing hangs at the thing. A tap
 *              on the figure or on the arc fig.turn opens this focus as a tap on the birch opens FocusCluster; the
 *              top right stays with the sky band (Z1) and the debug view (Z5).
 *
 *              THE REACH. Its centre is the figure's place lifted by hud_figure_focus_anchor_share of its height,
 *              its radius the screen distance from that centre to the head times hud_figure_focus_ring_share, never
 *              less than hud_focus_radius. Nothing draws it: a tap inside it opens and closes the focus
 *              (hit_figure), and the arc fig.turn (FigureTurn) lies on the ground before the feet where Phase 06
 *              drew a ring.
 *
 *              THE COMB. soul.mode is a hexagon of hud_figure_focus_comb_radius on the figure's axis,
 *              hud_figure_focus_comb_gap above the head, held hud_figure_focus_comb_pad off the frame's edges, under
 *              the head bar and left of cam.mode's square at the switch's height. OVERLAY_DISC fills it, DOMAIN_SKY
 *              strokes it at hud_figure_focus_comb_line and draws its glyph (figure.soul_mode.to_ai_glyph while the
 *              player leads the figure, figure.soul_mode.to_player_glyph while the AI leads it) and its word on the
 *              side toward the frame's middle, fading in after hud_fan_delay. While the head stands below the comb a
 *              leader in FIGURE_RAIL runs down to it. A tap on the comb is the request (hit_comb).
 *
 *              THE SPINE. A leader line in FIGURE_RAIL from hud_figure_focus_body_share of the reach right of the
 *              axis out by hud_figure_focus_soul_dx, kept hud_figure_focus_spine_room off the right edge, its row at
 *              the anchor held between one comb under the head bar and hud_figure_focus_spine_low; right of its end
 *              figure.focus.soul in FIGURE_SOUL_LABEL and under it figure.soul.sspc in DOMAIN_FLOW or
 *              figure.soul.aipc in DOMAIN_SKY.
 *
 *              cam.mode does not hang at the figure: after the switch to the fixed view the figure may stand outside
 *              the picture, so it is the switch under the debug switch (FigureSwitch).
 *
 *              NOTHING ABOUT THE FIGURE IS COMPUTED HERE: the place and the height are the body port's and the
 *              scene's; the comb's request goes through the ports of ase-pl-avatar in
 *              godot_vivarium_figure_input.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>

namespace ase::adp::godot {

/** Where the figure stands on the HUD this frame and who leads it. */
struct FigureFocusFrame {
    ::godot::Vector2 foot;      // the figure's place, in the mockup's units
    ::godot::Vector2 head;      // the top of its body, in the mockup's units
    bool led_self = false;      // the player leads the figure (SS/PC); false: the AI leads it (AI/PC)
    double elapsed = 0.0;       // seconds since the focus opened
    bool still = false;         // every motion held
};

/** The focus of the seat's figure. */
class FigureFocus {
public:
    /** Binds measures, colours and words; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Lays the reach of `now` without drawing it, so a tap on the closed focus finds the figure. */
    void lay(const FigureFocusFrame& now);

    /** Draws the open focus of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const FigureFocusFrame& now, std::string& failed);

    /** Forgets the comb drawn last: the focus closed or the figure left. */
    void close();

    /** True when `point` (in the mockup's units) lies inside the reach laid last. */
    bool hit_figure(const ::godot::Vector2& point) const;

    /** True when `point` (in the mockup's units) lies on the comb soul.mode drawn last over the head. */
    bool hit_comb(const ::godot::Vector2& point) const;

private:
    double anchor_share_ = 0.0;     // hud_figure_focus_anchor_share
    double ring_share_ = 0.0;       // hud_figure_focus_ring_share: the reach as a share of anchor to head
    double soul_dx_ = 0.0;          // hud_figure_focus_soul_dx
    double soul_label_dy_ = 0.0;    // hud_figure_focus_soul_label_dy
    double soul_label_size_ = 0.0;  // hud_figure_focus_soul_label_size
    double soul_value_dy_ = 0.0;    // hud_figure_focus_soul_value_dy
    double soul_value_size_ = 0.0;  // hud_figure_focus_soul_value_size
    double radius_min_ = 0.0;       // hud_focus_radius: the least reach
    double leader_delay_ = 0.0;     // hud_focus_leader_delay: the comb's leader and the spine
    double label_delay_ = 0.0;      // hud_focus_title_delay: SEELE
    double value_delay_ = 0.0;      // hud_focus_subtitle_delay: the soul's word
    double chip_delay_ = 0.0;       // hud_fan_delay: the comb's word

    ::godot::Color flow_;           // DOMAIN_FLOW: the player's soul

    ::godot::Vector2 centre_;       // the reach laid last, mockup units
    double radius_ = 0.0;           // its radius; 0 before the first lay

    UiLeaderLine leader_;
    UiTextBlock soul_label_;        // class t: figure.focus.soul
    UiTextBlock soul_player_;       // class t: figure.soul.sspc
    UiTextBlock soul_ai_;           // class t: figure.soul.aipc

    // THE COMB (soul.mode over the head) AND THE SPINE (the soul right of the figure), 07.11
    double comb_radius_ = 0.0;      // hud_figure_focus_comb_radius
    double comb_gap_ = 0.0;         // hud_figure_focus_comb_gap: the comb's lower edge above the head
    double comb_pad_ = 0.0;         // hud_figure_focus_comb_pad: off the frame, the head bar and cam.mode
    double comb_line_ = 0.0;        // hud_figure_focus_comb_line
    double comb_glyph_size_ = 0.0;  // hud_figure_focus_comb_glyph_size
    double comb_word_dx_ = 0.0;     // hud_figure_focus_comb_word_dx: off the comb's edge toward the middle
    double comb_word_dy_ = 0.0;     // hud_figure_focus_comb_word_dy
    double comb_word_size_ = 0.0;   // hud_figure_focus_comb_word_size
    double comb_hit_ = 0.0;         // hud_figure_focus_comb_hit: a tap this far outside the edge still takes it
    double body_share_ = 0.0;       // hud_figure_focus_body_share: the spine starts this share of the reach off the axis
    double leader_line_ = 0.0;      // hud_figure_focus_leader_line: the comb's leader and the spine
    double soul_text_dx_ = 0.0;     // hud_figure_focus_soul_text_dx: SEELE and the soul right of the spine's end
    double spine_room_ = 0.0;       // hud_figure_focus_spine_room: the soul's word needs this much right of the end
    double spine_low_ = 0.0;        // hud_figure_focus_spine_low: the spine's lowest row, above the dock's label
    double head_bar_ = 0.0;         // hud_figure_head_h: the comb stays under the head bar
    double frame_w_ = 0.0;          // hud_frame_width
    double frame_h_ = 0.0;          // hud_frame_height
    double switch_x_ = 0.0;         // hud_figure_switch_x: the comb keeps left of cam.mode's square
    double switch_y_ = 0.0;         // hud_figure_switch_y
    double switch_hit_ = 0.0;       // hud_figure_switch_hit

    ::godot::Color disc_;           // OVERLAY_DISC: the comb's fill
    ::godot::Color sky_;            // DOMAIN_SKY: the comb, its glyph, its word and the AI's soul
    ::godot::Color rail_;           // FIGURE_RAIL: the comb's leader and the spine
    ::godot::Color soul_ink_;       // FIGURE_SOUL_LABEL: the word SEELE

    UiIcon comb_to_ai_;             // figure.soul_mode.to_ai_glyph: while the player leads the figure
    UiIcon comb_to_self_;           // figure.soul_mode.to_player_glyph: while the AI leads it
    UiTextBlock comb_word_ai_;      // class t: figure.soul_mode.to_ai
    UiTextBlock comb_word_self_;    // class t: figure.soul_mode.to_player

    ::godot::Vector2 comb_;         // the comb drawn last, mockup units
    bool comb_shown_ = false;       // false while the focus is closed
};

}  // namespace ase::adp::godot

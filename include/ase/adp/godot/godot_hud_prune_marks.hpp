#pragma once

/**
 * ASE GODOT ADAPTER - HUD PRUNE MARKS
 *
 * @file        godot_hud_prune_marks.hpp
 * @brief       The glowing marks at the internodes that may be cut, while the billhook is in hand
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, building block PruneMarks (the mockup's
 *              prunMarks()), drawn in the mode prune (Ast schneiden, usecase:100365).
 *
 *              WHICH INTERNODES. Which shoots may be cut is the plant module's to say, never the view's:
 *              flora.scene.v1 marks each internode that may be cut. The mark names the organ by its object
 *              id; the mark stands at the middle of its tube - the instance's origin plus half its y axis
 *              (VivariumInstanceView rows, the unit tube of the organ template), from the stem base the
 *              scene placed the plant at. A marked internode without an instance this frame, or behind the
 *              camera, gets no mark and takes no tap.
 *
 *              THE MARK. Mark i fades in with hud_anim_fade after i * hud_prune_stagger. A pulse of
 *              hud_prune_pulse_radius in DOMAIN_CUT at hud_line, a disc of hud_prune_radius in OVERLAY_DISC
 *              ringed in DOMAIN_CUT at hud_prune_line, the glyph texts.hud.prune_glyph at
 *              hud_prune_glyph_size in DOMAIN_CUT. The tap target is hud_prune_hit_radius around it; a tap
 *              names the internode, and the view sends the cut to the plant module's port.
 *
 *              THE PICTURE IS WHOLE. The marks stand on the internodes the frame names and never ask whether a
 *              port delivered: while flora.scene.v1 marks none (N0462) VivariumHandBridge::cut writes the
 *              mockup's TREE.cand into the frame and VivariumView::open_gap marks the gap on the edge of the
 *              bridge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100365 - prune a branch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0462 (flora.scene.v1: the internodes that may be cut), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_organ_view.hpp>
#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the prune marks show this frame. */
struct PruneMarksFrame {
    const UiAnchor* anchor = nullptr;                             // the projection of this frame
    PlantOrigin origin;                                           // the stem base of the plant in focus
    const std::vector<VivariumInstanceView>* segments = nullptr;  // the internode instances of that plant
    std::vector<uint32_t> cuttable;                               // the internodes to mark, lowest first
    double elapsed = 0.0;                                         // seconds since the mode prune opened
    bool still = false;                                           // every motion held
};

/** Where one cut mark was drawn last: its internode and the square of hud_prune_hit_radius round it. */
struct PruneMarkPlace {
    uint32_t internode = 0;
    ::godot::Rect2 rect;  // in frame units
};

/** The marks of the internodes that may be cut. */
class PruneMarks {
public:
    /** Binds measures, colours, glyph, word, fade and ring; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the marks of `now` under `frame`, or the gap; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const PruneMarksFrame& now,
              std::string& failed);

    /** The internode whose mark `point` (in mockup units) hits; false where it hits none. */
    bool hit(const ::godot::Vector2& point, uint32_t& internode) const;

    /** The marks drawn last, the same hit() asks, for the place lines of the view. */
    void places(std::vector<PruneMarkPlace>& out) const;

private:
    /** One mark drawn last, for hit(). */
    struct Drawn {
        uint32_t internode = 0;
        ::godot::Vector2 centre;
    };

    /** The middle of the tube of `internode` on the HUD; false without an instance or behind the camera. */
    bool middle(const PruneMarksFrame& now, uint32_t internode, ::godot::Vector2& out) const;

    double pulse_radius_ = 0.0;  // hud_prune_pulse_radius
    double radius_ = 0.0;        // hud_prune_radius
    double line_w_ = 0.0;        // hud_prune_line
    double glyph_size_ = 0.0;    // hud_prune_glyph_size
    double hit_radius_ = 0.0;    // hud_prune_hit_radius
    double stagger_ = 0.0;       // hud_prune_stagger
    double pulse_line_ = 0.0;    // hud_line
    ::godot::Color cut_;         // DOMAIN_CUT
    ::godot::Color disc_;        // OVERLAY_DISC
    UiIcon glyph_;               // texts.hud.prune_glyph
    UiMotion fade_;              // hud_anim_fade
    UiRing ring_;

    std::vector<Drawn> drawn_;  // the marks of the last draw
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - HUD FOCUS RING
 *
 * @file        godot_hud_focus_ring.hpp
 * @brief       The ring of the focus: disc, ticks, the arcs of soil water and growth, their numbers, the stage glyph
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, the ring of block FocusCluster (the first part of the
 *              mockup's focus()). FocusCluster (godot_hud_focus_cluster.hpp) owns it and lays the head, the rows
 *              and the forecast around the centre this ring reports.
 *
 *              THE PLACE. Centred on the trunk's x, hud_focus_ring_offset_min or hud_focus_ring_offset_share of
 *              the plant's height above the stem base, whichever is larger; radius hud_focus_radius. The height
 *              the share is taken of stops at hud_focus_ring_height_max, the mockup's treeH at MADURA: the
 *              mockup's tree never grows past it, the drawn one can (a jumped stand, a tall rod), and above it
 *              the ring would climb into the column, PRÓXIMO ESTÁGIO under the rows on the right.
 *
 *              THE RING. A disc in OVERLAY_DISC at the alpha hud_focus_ring_fill_alpha under a circle in
 *              OVERLAY_INK_FAINT at hud_line; hud_focus_tick_count ticks every hud_focus_tick_step degrees from
 *              hud_focus_tick_inner outside the ring to hud_focus_tick_outer, every hud_focus_tick_major_every-th
 *              to hud_focus_tick_outer_major, at hud_line_hair.
 *
 *              THE ARCS. From perception levels[1] on two arcs at hud_focus_arc_width: the soil water from the
 *              bottom clockwise over half a turn times the share of field capacity in DOMAIN_WATER (delay
 *              hud_focus_arc_delay), the growth toward the next stage from the top over half a turn in
 *              DOMAIN_LIFE (delay hud_focus_arc_second_delay). Below that level one plain ring at
 *              hud_focus_plain_ring_line and hud_focus_plain_ring_alpha, in DOMAIN_ALERT while the soil is below
 *              hud_focus_soil_dry_below, else DOMAIN_LIFE.
 *
 *              THE NUMBERS. From levels[2] on both shares as whole percent in class d, size
 *              hud_focus_number_size, hud_focus_number_gap outside the ring: the water to the left at
 *              hud_focus_number_dy in DOMAIN_WATER, the growth to the right at hud_focus_number_second_dy in
 *              DOMAIN_LIFE.
 *
 *              THE STAGE GLYPH. texts.hud.stage_glyph.<stage> in DOMAIN_LIFE, or texts.hud.stage_glyph_dead in
 *              DOMAIN_ALERT for a dead stand, centred at hud_focus_stage_glyph_size.
 *
 *              THE GROWTH always stands in the frame: while flora.patch.v1 carries no growth toward the next
 *              stage (N0454), the bridge writes the mockup's share from hud_bridge_patch_growth_share of SHA into it
 *              (godot_vivarium_hud_bridge.cpp, bridge_patch); the ring never asks whether a port delivered.
 *
 *              A TAP inside the ring goes back to the mode rest (data-act rest).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the ring shows this frame. */
struct FocusRingFrame {
    ::godot::Vector2 trunk;         // the stem base in the mockup's units (UiAnchor::trunk)
    double height = 0.0;            // the plant's height in the mockup's units
    uint32_t level = 0;             // the perception level (hud.perception.levels)
    uint8_t stage = 0;              // GODOT_STAGE_* of flora.patch.v1
    bool dead = false;              // ase-plant marks the stand GODOT_COND_DEAD
    double soil_rel = 0.0;          // soil water over field capacity, 0..1
    double growth_share = 0.0;      // the growth toward the next stage, 0..1 (flora.patch.v1 or the bridge)
    double elapsed = 0.0;           // seconds since the focus opened
    bool still = false;             // every motion held
};

/** The ring of the focus. */
class FocusRing {
public:
    /** Binds measures, colours, glyphs and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Places and draws the ring of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const FocusRingFrame& now,
              std::string& failed);

    /** The centre of the ring drawn last, in the mockup's units. */
    const ::godot::Vector2& centre() const { return centre_; }

    /** The ring's radius (hud_focus_radius). */
    double radius() const { return radius_; }

    /** True when `point` (in the mockup's units) lies inside the ring drawn last. */
    bool hit(const ::godot::Vector2& point) const;

    /** The square round the ring drawn last (mockup units), whose centre hit() answers; false while none is drawn. */
    bool place(::godot::Rect2& out) const;

private:
    /** The disc, the circle and the ticks. */
    void draw_ticks(UiPen& canvas, const ::godot::Transform2D& frame) const;

    /** The two shares as whole percent. */
    bool draw_numbers(UiPen& canvas, const ::godot::Transform2D& frame, const FocusRingFrame& now,
                      std::string& failed);

    double radius_ = 0.0;            // hud_focus_radius
    double offset_min_ = 0.0;        // hud_focus_ring_offset_min
    double offset_share_ = 0.0;      // hud_focus_ring_offset_share
    double height_max_ = 0.0;        // hud_focus_ring_height_max
    double fill_alpha_ = 0.0;        // hud_focus_ring_fill_alpha
    double tick_count_ = 0.0;        // hud_focus_tick_count
    double tick_step_ = 0.0;         // hud_focus_tick_step
    double tick_inner_ = 0.0;        // hud_focus_tick_inner
    double tick_outer_ = 0.0;        // hud_focus_tick_outer
    double tick_outer_major_ = 0.0;  // hud_focus_tick_outer_major
    double tick_major_every_ = 0.0;  // hud_focus_tick_major_every
    double arc_width_ = 0.0;         // hud_focus_arc_width
    double arc_delay_ = 0.0;         // hud_focus_arc_delay
    double arc_second_delay_ = 0.0;  // hud_focus_arc_second_delay
    double plain_line_ = 0.0;        // hud_focus_plain_ring_line
    double plain_alpha_ = 0.0;       // hud_focus_plain_ring_alpha
    double number_gap_ = 0.0;        // hud_focus_number_gap
    double number_dy_ = 0.0;         // hud_focus_number_dy
    double number_second_dy_ = 0.0;  // hud_focus_number_second_dy
    double number_size_ = 0.0;       // hud_focus_number_size
    double glyph_size_ = 0.0;        // hud_focus_stage_glyph_size
    double soil_dry_ = 0.0;          // hud_focus_soil_dry_below
    double line_ = 0.0;              // hud_line
    double hair_ = 0.0;              // hud_line_hair

    uint32_t level_bars_ = 0;        // hud.perception.levels[1]: the arcs
    uint32_t level_numbers_ = 0;     // hud.perception.levels[2]: the numbers

    ::godot::Color disc_;            // OVERLAY_DISC at hud_focus_ring_fill_alpha
    ::godot::Color faint_;           // OVERLAY_INK_FAINT
    ::godot::Color water_;           // DOMAIN_WATER
    ::godot::Color life_;            // DOMAIN_LIFE
    ::godot::Color alert_;           // DOMAIN_ALERT

    std::vector<UiIcon> stage_glyphs_;  // texts.hud.stage_glyph.<n>, one per GODOT_STAGE_*
    UiIcon dead_glyph_;                 // texts.hud.stage_glyph_dead

    UiRing ring_;
    UiTextBlock water_number_;   // class d
    UiTextBlock growth_number_;  // class d

    ::godot::Vector2 centre_;    // the ring drawn last
    bool drawn_ = false;         // a ring was drawn, so hit() has a centre
};

}  // namespace ase::adp::godot

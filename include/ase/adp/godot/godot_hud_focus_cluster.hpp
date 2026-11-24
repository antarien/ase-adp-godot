#pragma once

/**
 * ASE GODOT ADAPTER - HUD FOCUS CLUSTER
 *
 * @file        godot_hud_focus_cluster.hpp
 * @brief       The focus around the chosen plant: its ring, the head, the spine rows and the stage forecast
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block FocusCluster (the mockup's focus(), drawn in the
 *              mode focus). The ring is FocusRing (godot_hud_focus_ring.hpp), owned here; the tool fan beside it
 *              is ActionFan (godot_hud_action_fan.hpp), not this block.
 *
 *              THE HEAD. A leader line in OVERLAY_INK_SOFT (delay hud_focus_leader_delay) from the ring's upper
 *              right (hud_focus_leader_knee of the radius along both axes) at forty-five degrees up to the column
 *              hud_focus_column_dx right of the ring, then up to hud_focus_leader_top. texts.hud.species in class
 *              tb at hud_focus_title_dx right of the column, hud_focus_title_y, size hud_focus_title_size; under
 *              it texts.hud.selection_of with the area in OVERLAY_INK_SOFT at hud_focus_subtitle_y, size
 *              hud_focus_subtitle_size, each fading in after its delay.
 *
 *              THE ROWS, drawn by UiSpine down the column. Stage: texts.hud.stages.<n> or texts.hud.words.none
 *              for a dead stand, with the stage pips lit up to the stage. Soil water: below perception levels[1]
 *              the word dry, moist or wet by hud_focus_soil_dry_below and hud_focus_soil_moist_below (shares of
 *              field capacity), from levels[1] the share as texts.hud.focus_share with its bar, from levels[2]
 *              texts.hud.focus_soil_full with the millimetres at hud_focus_decimals; DOMAIN_ALERT while dry,
 *              else DOMAIN_WATER. Cover: few or good by hud_focus_cover_few_below, from levels[1] the share with
 *              its bar, DOMAIN_GROUND. Loose soil above hud_focus_loose_shown_above: words.loose, from levels[1]
 *              texts.hud.focus_loose_share with its bar, DOMAIN_GROUND. Sap while a tap runs: words.flowing,
 *              from levels[1] texts.hud.focus_sap_fill with the vessel's fill over its capacity, DOMAIN_FLOW.
 *              Condition: words.wilted or words.dead in DOMAIN_ALERT while ase-plant marks the stand so
 *              (the marks wilted and dead, set against the codes of flora.cond.v1). In the debug view each row carries texts.hud.focus_source.<row>.
 *
 *              THE FORECAST. From levels[1] on, while the stand lives and has a stage after this one: a leader
 *              line from the ring's lower right to hud_focus_next_from_dx right of the ring and
 *              hud_focus_next_dy below its centre, then on to hud_focus_next_to_dx (delay
 *              hud_focus_next_delay); texts.hud.next_stage above it and under it, from levels[2],
 *              texts.hud.next_in_days with the days, below that level texts.hud.next_soon.
 *
 *              THE FRAME IS WHOLE. ase-plant reports stage, condition, soil water and cover on flora.patch.v1.
 *              The growth toward the next stage and the days to it (N0454), the loose soil (N0456) and the birch
 *              sap (N0457) stand in the frame all the same: while no port carries one, the bridge writes the
 *              mockup's value from hud_bridge_patch_* of SHA into it and logs the gap once on its edge
 *              (godot_vivarium_hud_bridge.cpp, bridge_patch). The cluster never asks whether a port delivered.
 *
 *              A TAP on the ring goes back to the mode rest (data-act rest).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_focus_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_spine.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the focus shows this frame. */
struct FocusClusterFrame {
    ::godot::Vector2 trunk;      // the stem base in the mockup's units (UiAnchor::trunk)
    double height = 0.0;         // the plant's height in the mockup's units
    uint32_t area = 0;           // the area number texts.hud.selection_of names
    uint32_t level = 0;          // the perception level (hud.perception.levels)
    bool debug = false;          // the debug view: each row names its source

    uint8_t stage = 0;           // GODOT_STAGE_* of flora.patch.v1
    bool cond_known = true;      // the patch condition equals a code of flora.cond.v1; a frame without a patch shows a stand without a mark
    bool wilted = false;         // ase-plant marks the stand wilted (VivariumPatchView::wilted)
    bool dead = false;           // ase-plant marks the stand dead (VivariumPatchView::dead)
    double soil_mm = 0.0;        // soil water, mm
    double soil_rel = 0.0;       // soil water over field capacity, 0..1
    double coverage = 0.0;       // covered share, 0..1

    double growth_share = 0.0;     // N0454: the share of the way to the next stage, 0..1
    bool next_due = false;         // the stand has a stage after this one
    double next_days = 0.0;        // game days until it is reached

    double loose_share = 0.0;      // N0456: the loosened share of the soil, 0..1

    bool sap_running = false;      // N0457: a tap runs into a vessel
    double sap_l = 0.0;            // litres in the vessel
    double sap_cap_l = 0.0;        // litres the vessel holds

    double elapsed = 0.0;        // seconds since the focus opened
    bool still = false;          // every motion held
};

/** The focus cluster. */
class FocusCluster {
public:
    /** Binds the ring, measures, colours, words and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the focus of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const FocusClusterFrame& now,
              std::string& failed);

    /** True when `point` (in the mockup's units) lies on the ring drawn last: the tap goes back to rest. */
    bool hit(const ::godot::Vector2& point) const { return ring_.hit(point); }

    /** The ring drawn last: its centre is where ActionFan lays its chips. */
    const FocusRing& ring() const { return ring_; }

private:
    /** The leader line, the species and the area line. */
    bool draw_head(UiPen& canvas, const ::godot::Transform2D& frame, const FocusClusterFrame& now,
                   std::string& failed);

    /** The rows of stage, soil water and cover, handed to the spine; false with the reason in `failed`. */
    bool rows(const FocusClusterFrame& now, std::vector<UiSpineRow>& out, std::string& failed) const;

    /** The rows of loose soil, sap and condition, each where it stands; false with the reason in `failed`. */
    bool marks(const FocusClusterFrame& now, std::vector<UiSpineRow>& out, std::string& failed) const;

    /** The forecast line with its leader: the next stage and the days to it. */
    bool draw_next(UiPen& canvas, const ::godot::Transform2D& frame, const FocusClusterFrame& now,
                   std::string& failed);

    /** The word `runs` with `values` filled in, as plain text; false with the reason in `failed`. */
    static bool filled(const std::vector<HudRun>& runs, const std::vector<HudPortValue>& values, std::string& out,
                       std::string& failed);

    /** The share `share` as whole percent. */
    static std::string percent(double share);

    const UiTheme* theme_ = nullptr;  // the theme the rows' fixed words are read from

    double column_dx_ = 0.0;         // hud_focus_column_dx
    double leader_knee_ = 0.0;       // hud_focus_leader_knee
    double leader_top_ = 0.0;        // hud_focus_leader_top
    double leader_delay_ = 0.0;      // hud_focus_leader_delay
    double title_dx_ = 0.0;          // hud_focus_title_dx
    double title_y_ = 0.0;           // hud_focus_title_y
    double title_size_ = 0.0;        // hud_focus_title_size
    double title_delay_ = 0.0;       // hud_focus_title_delay
    double subtitle_y_ = 0.0;        // hud_focus_subtitle_y
    double subtitle_size_ = 0.0;     // hud_focus_subtitle_size
    double subtitle_delay_ = 0.0;    // hud_focus_subtitle_delay
    double next_dy_ = 0.0;           // hud_focus_next_dy
    double next_from_dx_ = 0.0;      // hud_focus_next_from_dx
    double next_to_dx_ = 0.0;        // hud_focus_next_to_dx
    double next_delay_ = 0.0;        // hud_focus_next_delay
    double next_label_dx_ = 0.0;     // hud_focus_next_label_dx
    double next_label_dy_ = 0.0;     // hud_focus_next_label_dy
    double next_label_size_ = 0.0;   // hud_focus_next_label_size
    double next_label_delay_ = 0.0;  // hud_focus_next_label_delay
    double next_value_dy_ = 0.0;     // hud_focus_next_value_dy
    double next_value_size_ = 0.0;   // hud_focus_next_value_size
    double next_value_delay_ = 0.0;  // hud_focus_next_value_delay
    double soil_dry_ = 0.0;          // hud_focus_soil_dry_below
    double soil_moist_ = 0.0;        // hud_focus_soil_moist_below
    double cover_few_ = 0.0;         // hud_focus_cover_few_below
    double loose_shown_ = 0.0;       // hud_focus_loose_shown_above
    double decimals_ = 0.0;          // hud_focus_decimals

    uint32_t level_bars_ = 0;        // hud.perception.levels[1]: bars and shares
    uint32_t level_numbers_ = 0;     // hud.perception.levels[2]: numbers

    ::godot::Color ink_;             // OVERLAY_INK
    ::godot::Color soft_;            // OVERLAY_INK_SOFT
    ::godot::Color water_;           // DOMAIN_WATER
    ::godot::Color life_;            // DOMAIN_LIFE
    ::godot::Color ground_;          // DOMAIN_GROUND
    ::godot::Color flow_;            // DOMAIN_FLOW
    ::godot::Color alert_;           // DOMAIN_ALERT

    std::vector<std::vector<HudRun>> stages_;   // texts.hud.stages.<n>
    std::vector<HudRun> share_words_;           // texts.hud.focus_share
    std::vector<HudRun> soil_full_words_;       // texts.hud.focus_soil_full
    std::vector<HudRun> loose_share_words_;     // texts.hud.focus_loose_share
    std::vector<HudRun> sap_fill_words_;        // texts.hud.focus_sap_fill
    std::vector<HudRun> next_days_words_;       // texts.hud.next_in_days
    std::vector<HudRun> next_soon_words_;       // texts.hud.next_soon
    std::vector<HudRun> selection_words_;       // texts.hud.selection_of

    FocusRing ring_;
    UiLeaderLine leader_;
    UiSpine spine_;
    UiTextBlock title_;          // class tb: texts.hud.species
    UiTextBlock subtitle_;       // class t: texts.hud.selection_of
    UiTextBlock next_label_;     // class t: texts.hud.next_stage
    UiTextBlock next_value_;     // class t
};

}  // namespace ase::adp::godot

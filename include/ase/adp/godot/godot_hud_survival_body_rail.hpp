#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL BODY RAIL
 *
 * @file        godot_hud_survival_body_rail.hpp
 * @brief       The body rail CORPO at the right edge of the care view, drawn as the survival mockup's corpo(st) does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U01 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 193 to 210, element S1). The counterpart of the rail of signs SINAIS (UiEdgeRail) on the
 *              opposite edge. The mockup draws:
 *
 *                line     a vertical line at hud_survival_rail_x from hud_survival_rail_top to
 *                         hud_survival_rail_bottom in OVERLAY_INK_FAINT (--hud3), at the line width hud_line
 *                label    the word survival.body.label (CORPO) start-anchored at (hud_survival_rail_x,
 *                         hud_survival_rail_label_y), size hud_survival_rail_label_size, turned by
 *                         hud_survival_rail_label_turn degrees about that point, in OVERLAY_INK_SOFT (--hud2)
 *                rows     one row per body quantity that counts for the figure, in the order heat, hunger, thirst,
 *                         energy, fatigue, health, row i at y = hud_survival_rail_row_y + i * hud_survival_rail_row_step,
 *                         fading in hud_survival_rail_row_delay * i seconds after the view (class fade):
 *                           glyph   hud_survival_glyph_<quantity> (path data of one unit) scaled by
 *                                   hud_survival_rail_glyph_size about (x, y), stroked in the row's tone at
 *                                   hud_survival_glyph_line; the heat's stem at hud_survival_glyph_stem_line
 *                           track   from hud_survival_rail_track_left to hud_survival_rail_track_right at
 *                                   y + hud_survival_rail_track_dy in OVERLAY_INK_FAINT at hud_line
 *                           fill    from track_right - (track_right - track_left) * fill to track_right at the
 *                                   same y in the row's tone at hud_survival_rail_fill_line
 *                           name    the row's word end-anchored at (hud_survival_rail_name_x,
 *                                   y + hud_survival_rail_text_dy), size hud_survival_rail_name_size, in its tone
 *                           number  the row's value in the data font at (hud_survival_rail_number_x,
 *                                   y + hud_survival_rail_text_dy), size hud_survival_rail_number_size, in
 *                                   OVERLAY_INK_SOFT
 *                pulse    hud_survival_glyph_pulse at (x, hud_survival_rail_pulse_y), size
 *                         hud_survival_rail_pulse_size, in OVERLAY_INK (--hud) while at least one row shows,
 *                         otherwise OVERLAY_INK_SOFT: at rest the rail is its line, its word and its pulse
 *
 *              THE ROWS (s.html VIT). The tone and the fill of a row come from the figure's quantity as its owner
 *              writes it; the rules are tokens of SHA:
 *                heat     MTB_EFFECTIVE_TEMP in degrees, fill MTB_TEMP_STRESS; while the stress exceeds
 *                         hud_survival_rail_stress_onset the row is DOMAIN_SKY under hud_survival_rail_comfort_low
 *                         and DOMAIN_CUT above it, otherwise DOMAIN_LIFE; the number survival.body.degrees with
 *                         hud_survival_rail_temp_decimals places
 *                hunger   MTB_HUNGER of hud_survival_rail_level_full, DOMAIN_GROUND, survival.body.percent
 *                thirst   MTB_THIRST of level_full, DOMAIN_WATER, survival.body.percent
 *                energy   MTB_ENERGY of level_full, DOMAIN_TIME, survival.body.of_full
 *                fatigue  MTB_FATIGUE as a share 0 to 1, DOMAIN_SKY, survival.body.percent of the share
 *                health   MTB_HEALTH as a share 0 to 1, DOMAIN_ALERT while the share times level_full lies under
 *                         hud_survival_rail_health_alert, otherwise
 *                         DOMAIN_LIFE, survival.body.percent
 *              The rail draws what its frame says counts; it never asks a port and never decides whether the
 *              owner delivered (the bridge writes the frame, gap N0530 under the carrier N0448).
 *
 *              THE TAP. A row's target runs from hud_survival_rail_track_left, hud_survival_rail_hit_w wide and
 *              hud_survival_rail_hit_h high about the row's y. The mockup binds no gesture to it
 *              (s.html 242 takes clicks only on the buttons of the bench) and draws no body card, so no hit test
 *              asks it (PLAN U01, corrected 2026-11-20).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528 (survival rows of the inventory), N0530 (facets without a hub value), carrier N0448
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

/** One body quantity of the figure as the frame carries it. */
struct SurvivalBodyValue {
    bool counts = false;  // the quantity counts for the figure: its row shows (s.html 71, VIT r[2] != null)
    double value = 0.0;   // the quantity as its owner writes it
};

/** The body of the figure for the rail, written by the bridge from the owner's port or the SHA defaults. */
struct SurvivalBodyFrame {
    static constexpr const char* GAP_BODY = "N0530";  // the bridge gap: no survival.body.v1 record of the seat figure
    SurvivalBodyValue temp;    // MTB_EFFECTIVE_TEMP: the felt temperature in degrees
    double temp_stress = 0.0;  // MTB_TEMP_STRESS: 0 to 1, the heat row's fill and its cold or hot tone
    SurvivalBodyValue hunger;  // MTB_HUNGER: 0 to hud_survival_rail_level_full
    SurvivalBodyValue thirst;  // MTB_THIRST: 0 to level_full
    SurvivalBodyValue energy;  // MTB_ENERGY: 0 to level_full
    SurvivalBodyValue fatigue; // MTB_FATIGUE: a share 0 to 1
    SurvivalBodyValue health;  // MTB_HEALTH: a share 0 to 1, shown on the scale of level_full
    bool poisoned = false;     // TAG_MTB_POISONED, 1 by the rule of hub_thresholds.json: the state on the rail SINAIS (U02)
    bool bleeding = false;     // TAG_MTB_BLEEDING
    bool emaciated = false;    // TAG_MTB_EMACIATED
    bool well_rested = false;  // TAG_MTB_WELL_RESTED
};

/** The body rail CORPO at the right edge. */
class SurvivalBodyRail {
public:
    /** Binds every hud_survival_rail_* and hud_survival_glyph_* token, the inks, the words and the primitives. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The rows of `body` for this frame: tone, fill and number of each quantity that counts; false with the reason. */
    bool set(const SurvivalBodyFrame& body, std::string& failed);

    /** Draws the rail under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

    /** True with the shown row's place in `index` when `point` (mockup units) lies on a row's target. */
    bool hit_row(const ::godot::Vector2& point, int32_t& index) const;

    /** The target of the shown row at place `index` (mockup units), the band hit_row() asks; false past the last. */
    bool row_place(size_t index, ::godot::Rect2& place) const;

private:
    /** One quantity's row: its word, its glyph and what set() wrote for this frame. */
    struct Row {
        UiTextBlock name;                    // survival.body.<quantity>, in the row's tone
        UiTextBlock number;                  // the value in the data font
        std::vector<UiSvgOutline> glyph;     // hud_survival_glyph_<quantity>
        std::vector<UiSvgOutline> stem;      // hud_survival_glyph_temp_stem: only the heat carries one
        std::vector<HudRun> format;          // survival.body.degrees, .percent or .of_full
        ::godot::Color tone;                 // the domain colour set() chose
        double joined = 0.0;                 // hud_survival_glyph_<quantity>_joined: 1 strokes the glyph with round knees
        double fill = 0.0;                   // 0 to 1, the share of the track
        bool shown = false;                  // the quantity counts this frame
    };

    /** Binds `row` to the word `word`, the glyph token `glyph` and the number format `format`. */
    bool bind_row(Row& row, const char* word, const char* glyph, const char* format, std::string& failed);

    /** Writes `text` into the row's number through its format; false with the reason. */
    bool set_number(Row& row, const std::string& text, std::string& failed);

    /**
     * Strokes `outlines` scaled by `size` about `centre` in `color` at `width`, laid in the units of `frame`; with
     * `joined` above 0 every outline, open or closed, gets round knees (stroke-linejoin round).
     */
    void stroke_glyph(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<UiSvgOutline>& outlines,
                      const ::godot::Vector2& centre, double size, const ::godot::Color& color, double width,
                      double joined) const;

    const UiTheme* theme_ = nullptr;
    UiLeaderLine line_;              // the rail's line, the track and the line width hud_line
    UiMotion fade_;                  // hud_anim_fade: the rows' fade-in
    UiTextBlock label_;              // survival.body.label, class t
    Row heat_;                       // VIT temp: MTB_EFFECTIVE_TEMP and MTB_TEMP_STRESS
    Row hunger_;                     // VIT hunger: MTB_HUNGER
    Row thirst_;                     // VIT thirst: MTB_THIRST
    Row energy_;                     // VIT energy: MTB_ENERGY
    Row fatigue_;                    // VIT fatigue: MTB_FATIGUE
    Row health_;                     // VIT health: MTB_HEALTH
    std::vector<UiSvgOutline> pulse_;// hud_survival_glyph_pulse
    ::godot::Color ink_;             // OVERLAY_INK: --hud
    ::godot::Color faint_;           // OVERLAY_INK_FAINT: --hud3
    ::godot::Color soft_;            // OVERLAY_INK_SOFT: --hud2
    ::godot::Color sky_;             // DOMAIN_SKY
    ::godot::Color cut_;             // DOMAIN_CUT
    ::godot::Color life_;            // DOMAIN_LIFE
    ::godot::Color ground_;          // DOMAIN_GROUND
    ::godot::Color water_;           // DOMAIN_WATER
    ::godot::Color time_;            // DOMAIN_TIME
    ::godot::Color alert_;           // DOMAIN_ALERT
    double x_ = 0.0;                 // hud_survival_rail_x
    double top_ = 0.0;               // hud_survival_rail_top
    double bottom_ = 0.0;            // hud_survival_rail_bottom
    double label_y_ = 0.0;           // hud_survival_rail_label_y
    double label_size_ = 0.0;        // hud_survival_rail_label_size
    double label_turn_ = 0.0;        // hud_survival_rail_label_turn
    double row_y_ = 0.0;             // hud_survival_rail_row_y
    double row_step_ = 0.0;          // hud_survival_rail_row_step
    double row_delay_ = 0.0;         // hud_survival_rail_row_delay
    double glyph_size_ = 0.0;        // hud_survival_rail_glyph_size
    double track_left_ = 0.0;        // hud_survival_rail_track_left
    double track_right_ = 0.0;       // hud_survival_rail_track_right
    double track_dy_ = 0.0;          // hud_survival_rail_track_dy
    double fill_line_ = 0.0;         // hud_survival_rail_fill_line
    double name_x_ = 0.0;            // hud_survival_rail_name_x
    double name_size_ = 0.0;         // hud_survival_rail_name_size
    double text_dy_ = 0.0;           // hud_survival_rail_text_dy
    double number_x_ = 0.0;          // hud_survival_rail_number_x
    double number_size_ = 0.0;       // hud_survival_rail_number_size
    double pulse_y_ = 0.0;           // hud_survival_rail_pulse_y
    double pulse_size_ = 0.0;        // hud_survival_rail_pulse_size
    double level_full_ = 0.0;        // hud_survival_rail_level_full
    double stress_onset_ = 0.0;      // hud_survival_rail_stress_onset
    double health_alert_ = 0.0;      // hud_survival_rail_health_alert
    double comfort_low_ = 0.0;       // hud_survival_rail_comfort_low
    double temp_decimals_ = 0.0;     // hud_survival_rail_temp_decimals
    double hit_w_ = 0.0;             // hud_survival_rail_hit_w
    double hit_h_ = 0.0;             // hud_survival_rail_hit_h
    double glyph_line_ = 0.0;        // hud_survival_glyph_line
    double stem_line_ = 0.0;         // hud_survival_glyph_stem_line
    size_t shown_ = 0;               // rows shown this frame
};

}  // namespace ase::adp::godot

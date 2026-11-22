#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL FIRE CARD
 *
 * @file        godot_hud_survival_fire_card.hpp
 * @brief       The fire card FOGUEIRA on a leader line from the camp fire, drawn as the survival mockup's fireCard(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U04 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 212 to 222, element S4). While a camp fire stands by the figure the mockup draws, in one group
 *              fading in with the view (class fade), about the column bx = hud_survival_fire_column_x and the
 *              baseline ey = hud_survival_fire_elbow_y:
 *
 *                leader   a polyline from (anchor_x, anchor_y) over (elbow_x, ey) to (bx, ey) in DOMAIN_CUT
 *                title    survival.fire.title with {stage} = survival.fire.stage.<n> at (bx, ey + title_dy), size
 *                         title_size, in DOMAIN_CUT
 *                stages   the five glyphs ignite, flame, charcoal, ember, ash at (bx + stage_dx + i * stage_step,
 *                         ey + stage_dy), size stage_size: the burning stage in DOMAIN_CUT with a ring of
 *                         ring_radius at ring_line, the others in OVERLAY_INK_FAINT (--hud3)
 *                log      a rounded frame log_w by log_h, its top at ly + log_top, ly = ey + log_dy, corner
 *                         log_round, stroked at log_line in OVERLAY_INK_SOFT (--hud2); inside it, fill_inset in
 *                         from its left and top, a fill fill_w * fuel by
 *                         fill_h, corner fill_round, in DOMAIN_GROUND
 *                fuel     survival.fire.fuel with {value} the fuel in percent at (bx + fuel_dx, ly + fuel_dy), data
 *                         font, size fuel_size, in OVERLAY_INK_SOFT
 *                row      the air glyph and survival.fire.air in DOMAIN_FLOW, the smoke glyph and survival.fire.smoke
 *                         in DOMAIN_ALERT while the air is toxic and OVERLAY_INK_SOFT otherwise, the ash glyph and
 *                         survival.fire.ash in OVERLAY_INK_SOFT; glyphs at ly + row_glyph_dy (ash at ly + ash_dy),
 *                         words at ly + row_word_dy, size row_word_size
 *
 *              Every name above stands for hud_survival_fire_<name>. The stage follows the marks of HUB_FIRE_CELL
 *              (FLAM, EMBR, ASH) of ase-fire, the fuel HUB_FIRE_CELL_FUEL over FIRE_FUEL_FULL, the smoke the
 *              owner's grade of the air ATM_GAS_BTH_STATE (breath_state) above the grade it gives normal air
 *              ATM_CONST_BREATHABILITY_NORMAL (breath_normal), both laid by ingle.fire.v1 per record. The card draws
 *              what the frame says; it never asks a port (the bridge writes the frame, gap B0657 while no fire cell
 *              is laid and gap B0662 while a record misses one of the two grades, both under the carrier N0448).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528 (survival rows of the inventory), B0657 (camp state port), B0662 (smoke grade), carrier N0448
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
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The camp fire by the figure for the fire card, written by the bridge from the owner's port or the SHA defaults. */
struct SurvivalFireFrame {
    static constexpr const char* GAP_FIRE = "B0657";   // the bridge gap: no record of ingle.fire.v1 stands
    static constexpr const char* GAP_MARKS = "B0662";  // the bridge gap while a record of ingle.fire.v1 misses breath_state, breath_normal or oxygen (IngleHostFireExptSystem lays each the owner published)
    static constexpr int32_t STAGE_IGNITE = 0;         // STG[0] ignite: the cell stands with none of its marks yet
    static constexpr int32_t STAGE_FLAME = 1;          // STG[1] flame: mark HUB_FIRE_CELL_FLAM
    static constexpr int32_t STAGE_CHARCOAL = 2;       // STG[2] charcoal: mark HUB_FIRE_CELL_CHAR
    static constexpr int32_t STAGE_EMBER = 3;          // STG[3] ember: mark HUB_FIRE_CELL_EMBR
    static constexpr int32_t STAGE_LAST = 4;           // STG[4] ash, the last of the five stages: mark HUB_FIRE_CELL_ASH
    bool shown = false;    // st.fire: a camp fire stands by the figure, the card shows
    int32_t stage = 0;     // st.stage: 0 ignite, 1 flame, 2 charcoal, 3 ember, 4 ash (marks FLAM, CHAR, EMBR, ASH)
    double fuel = 0.0;     // st.fuel: HUB_FIRE_CELL_FUEL over HUB_FIRE_CELL_FUEL_LAID (ingle.fire.v1), a share 0 to 1
    bool toxic = false;    // st.air: the owner grades the air worse than normal (ATM_GAS_BTH_STATE), the smoke turns DOMAIN_ALERT
    double oxygen = 0.0;   // the air the fire draws: ATM_OXYGEN_FACTOR (ingle.fire.v1 oxygen), 1 normal to 0 none; the air stream fades with it
    double ash_kg = 0.0;   // what the fire leaves: HUB_ENT_ASH_REQ_ASH (ingle.left.v1 ash_kg), kg of ash since the cell was laid; the ash heap inks with it
    bool burning = false;  // the cell burns at a rate: ingle.left.v1 carries burn_left; the burn time shows only then
    double burn_left = 0.0;  // how long the fire lasts: HUB_FIRE_CELL_BURN_LEFT (ingle.left.v1 burn_left), seconds at the burn rate of this tick
    static constexpr const char* GAP_LEFT = "E0257";   // the bridge gap while the host offers no ingle.left.v1 (IngleHostLeftExptSystem lays ash_kg and burn_left)
};

/** The fire card FOGUEIRA on its leader line from the camp fire. */
class SurvivalFireCard {
public:
    /** Binds every hud_survival_fire_* token, the glyph line, the inks and the survival.fire.* words. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The card of `fire` for this frame: title, stage and fuel; false with the reason. */
    bool set(const SurvivalFireFrame& fire, std::string& failed);

    /** Draws the card under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

private:
    /** One stage of the fire: its glyph's outlines and dots, the line they are stroked at, and its word. */
    struct Stage {
        std::vector<UiSvgOutline> glyph;      // hud_survival_fire_glyph_<stage>
        std::vector<UiSvgOutline> mark;       // the charcoal's two strokes: hud_survival_fire_glyph_charcoal_mark
        std::vector<UiSvgOutline> dots;       // the ember's and the ash's dots, one point each
        double line = 0.0;                    // the glyph's stroke
        double mark_line = 0.0;               // hud_survival_fire_glyph_charcoal_mark_line
        double dot_radius = 0.0;              // the dots' radius in pixels, whatever the glyph's size
        std::string word;                     // survival.fire.stage.<n>
    };

    static constexpr size_t STAGES = 5;     // STG: ignite, flame, charcoal, ember, ash
    static constexpr size_t STAGE_ASH = 4;  // STG[4]: GL.ash also stands in the row under the log

    /** Binds `stage` to its glyph token `glyph`, its line token `line` (nullptr: the glyph line) and its word. */
    bool bind_stage(const UiTheme& theme, Stage& stage, const char* glyph, const char* line, const char* word,
                    std::string& failed);

    /**
     * Strokes `outlines` scaled by `size` about `centre` in `color` at `width`, laid in the units of `frame`: the
     * mockup's GL[name](x, y, k, c).
     */
    void stroke_glyph(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<UiSvgOutline>& outlines,
                      const ::godot::Vector2& centre, double size, const ::godot::Color& color, double width) const;

    /** Draws the dots of `outlines` about `centre`, their places scaled by `size`, filled and stroked in `color`. */
    void draw_dots(UiPen& canvas, const std::vector<UiSvgOutline>& outlines, const ::godot::Vector2& centre,
                   double size, double radius, const ::godot::Color& color) const;

    /** Draws `stage`'s glyph, mark and dots at `centre`, size `size`, in `color`, laid in the units of `frame`. */
    void draw_stage(UiPen& canvas, const ::godot::Transform2D& frame, const Stage& stage, const ::godot::Vector2& centre,
                    double size, const ::godot::Color& color) const;

    const UiTheme* theme_ = nullptr;
    UiLeaderLine line_;                 // the leader; the line width hud_line
    UiMotion fade_;                     // hud_anim_fade: the group's fade-in
    UiTextBlock title_;                 // survival.fire.title, class t
    UiTextBlock fuel_;                  // survival.fire.fuel, class d
    UiTextBlock air_;                   // survival.fire.air
    UiTextBlock smoke_;                 // survival.fire.smoke
    UiTextBlock ash_word_;              // survival.fire.ash
    UiTextBlock ash_kg_;                // survival.fire.ash_kg with {value}, class d
    UiTextBlock burn_;                  // survival.fire.burn with {value}, class d
    std::vector<HudRun> ash_kg_runs_;   // survival.fire.ash_kg with {value}
    std::vector<HudRun> burn_runs_;     // survival.fire.burn with {value}
    std::vector<HudRun> title_runs_;    // survival.fire.title with {stage}
    std::vector<HudRun> fuel_runs_;     // survival.fire.fuel with {value}
    std::vector<Stage> stages_;         // ignite, flame, charcoal, ember, ash in the order of st.stage
    std::vector<UiSvgOutline> air_glyph_;   // hud_survival_fire_glyph_air
    std::vector<UiSvgOutline> smoke_glyph_; // hud_survival_fire_glyph_smoke
    ::godot::Color faint_;              // OVERLAY_INK_FAINT: --hud3
    ::godot::Color soft_;               // OVERLAY_INK_SOFT: --hud2
    ::godot::Color cut_;                // DOMAIN_CUT: leader, title, burning stage, ring
    ::godot::Color ground_;             // DOMAIN_GROUND: the fuel's fill
    ::godot::Color flow_;               // DOMAIN_FLOW: the air
    ::godot::Color alert_;              // DOMAIN_ALERT: the smoke of toxic air
    double anchor_x_ = 0.0;             // hud_survival_fire_anchor_x
    double anchor_y_ = 0.0;             // hud_survival_fire_anchor_y
    double elbow_x_ = 0.0;              // hud_survival_fire_elbow_x
    double elbow_y_ = 0.0;              // hud_survival_fire_elbow_y: the baseline ey
    double column_x_ = 0.0;             // hud_survival_fire_column_x: bx
    double title_dy_ = 0.0;             // hud_survival_fire_title_dy
    double title_size_ = 0.0;           // hud_survival_fire_title_size
    double stage_dx_ = 0.0;             // hud_survival_fire_stage_dx
    double stage_step_ = 0.0;           // hud_survival_fire_stage_step
    double stage_dy_ = 0.0;             // hud_survival_fire_stage_dy
    double stage_size_ = 0.0;           // hud_survival_fire_stage_size
    double ring_radius_ = 0.0;          // hud_survival_fire_ring_radius
    double ring_line_ = 0.0;            // hud_survival_fire_ring_line
    double log_dy_ = 0.0;               // hud_survival_fire_log_dy
    double log_top_ = 0.0;              // hud_survival_fire_log_top: the frame's top relative to ly
    double log_w_ = 0.0;                // hud_survival_fire_log_w
    double log_h_ = 0.0;                // hud_survival_fire_log_h
    double log_round_ = 0.0;            // hud_survival_fire_log_round
    double log_line_ = 0.0;             // hud_survival_fire_log_line
    double fill_inset_ = 0.0;           // hud_survival_fire_fill_inset
    double fill_w_ = 0.0;               // hud_survival_fire_fill_w
    double fill_h_ = 0.0;               // hud_survival_fire_fill_h
    double fill_round_ = 0.0;           // hud_survival_fire_fill_round
    double fuel_dx_ = 0.0;              // hud_survival_fire_fuel_dx
    double fuel_dy_ = 0.0;              // hud_survival_fire_fuel_dy
    double fuel_size_ = 0.0;            // hud_survival_fire_fuel_size
    double row_glyph_dy_ = 0.0;         // hud_survival_fire_row_glyph_dy
    double row_word_dy_ = 0.0;          // hud_survival_fire_row_word_dy
    double row_word_size_ = 0.0;        // hud_survival_fire_row_word_size
    double air_dx_ = 0.0;               // hud_survival_fire_air_dx
    double air_size_ = 0.0;             // hud_survival_fire_air_size
    double air_word_dx_ = 0.0;          // hud_survival_fire_air_word_dx
    double smoke_dx_ = 0.0;             // hud_survival_fire_smoke_dx
    double smoke_size_ = 0.0;           // hud_survival_fire_smoke_size
    double smoke_word_dx_ = 0.0;        // hud_survival_fire_smoke_word_dx
    double ash_dx_ = 0.0;               // hud_survival_fire_ash_dx
    double ash_dy_ = 0.0;               // hud_survival_fire_ash_dy
    double ash_size_ = 0.0;             // hud_survival_fire_ash_size
    double ash_word_dx_ = 0.0;          // hud_survival_fire_ash_word_dx
    double ash_kg_dx_ = 0.0;            // hud_survival_fire_ash_kg_dx
    double ash_kg_decimals_ = 0.0;      // hud_survival_fire_ash_kg_decimals
    double ash_empty_ = 0.0;            // hud_survival_fire_ash_empty: the heap's ink while no ash is left
    double burn_dx_ = 0.0;              // hud_survival_fire_burn_dx
    double burn_dy_ = 0.0;              // hud_survival_fire_burn_dy
    double burn_unit_s_ = 0.0;          // hud_survival_fire_burn_unit_s: seconds per unit of survival.fire.burn
    double burn_decimals_ = 0.0;        // hud_survival_fire_burn_decimals
    double glyph_line_ = 0.0;           // hud_survival_glyph_line: the stroke of every glyph without its own
    double air_line_ = 0.0;             // hud_survival_fire_glyph_air_line
    double dot_line_ = 0.0;             // hud_survival_fire_glyph_dot_line
    double ember_dot_ = 0.0;            // hud_survival_fire_glyph_ember_dot_radius
    double ash_dot_ = 0.0;              // hud_survival_fire_glyph_ash_dot_radius
    double fuel_share_ = 0.0;           // st.fuel of this frame
    int32_t stage_ = 0;                 // st.stage of this frame
    bool toxic_ = false;                // st.air of this frame
    double oxygen_ = 0.0;               // ATM_OXYGEN_FACTOR of this frame, the strength of the air stream
    double ash_kg_value_ = 0.0;         // HUB_ENT_ASH_REQ_ASH of this frame, kg
    bool burning_ = false;              // the cell burns at a rate in this frame: the burn time shows
    bool shown_ = false;                // st.fire of this frame
};

}  // namespace ase::adp::godot

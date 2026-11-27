#pragma once

/**
 * ASE GODOT ADAPTER - HUD DEBUG LAYER
 *
 * @file        godot_hud_debug_layer.hpp
 * @brief       The debug layer of the care view: three status lines, the handle and the drawer with its four tabs
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, building block DebugLayer (the mockup's debugLayer()),
 *              section "Debug-Ebene": the buttons of the section debug of vivarium_start.json drawn as the mockup
 *              draws them. Every chip is an entry VivariumView::load_debug read (debug.buttons, debug.jumps,
 *              debug.date, debug.reliefs); this block places and draws them and says what a tap hit, the asks
 *              stay with VivariumView (ask_pace, ask_jump, step_date, ask_date, ask_shape).
 *
 *              THE STATUS LINES. At hud_debug_x from hud_debug_y, hud_debug_line_step apart, class d of
 *              hud_debug_line_size in DEBUG_INK: texts.hud.debug_build ({adapter} {client} {units}),
 *              texts.hud.debug_time ({day} the day of the year, {hour} {sim} {steps}, from the clock record the
 *              bridge has filled, VivariumView::bridge_clock) and texts.hud.debug_patch ({cover} {mm} {rel},
 *              empty while no patch is chosen) and texts.hud.debug_salt ({sum} {hyd} {soil} {chunks}, the salt of
 *              every chunk of hydrogeology.salt.v1 summed, H.9 I-4; empty while the port laid no record).
 *
 *              THE HANDLE. Closed, a rounded rectangle hud_debug_handle_x, _y, _w, _h of hud_debug_handle_radius
 *              in DEBUG_GROUND with a DEBUG_LINE outline of hud_debug_chip_line, and texts.hud.debug_open middle at
 *              hud_debug_handle_text_x, _y; a tap on it opens the drawer.
 *
 *              THE DRAWER. Open, the drawer from hud_debug_drawer_y, hud_debug_drawer_h high and on a tall
 *              surface down to the end of the hand band (reach.hand), across the surface (reach.surface) in
 *              DEBUG_DRAWER under a DEBUG_LINE line of hud_debug_drawer_line; the title
 *              texts.hud.debug_title at hud_debug_pad, drawer_y + hud_debug_title_dy of hud_debug_title_size,
 *              the fold texts.hud.debug_close end-anchored at hud_debug_fold_x of hud_debug_fold_size, its tap
 *              target hud_debug_fold_hit_x, drawer_y, _w, _h. The four tabs texts.hud.debug_tabs.<tab> as chips
 *              from hud_debug_pad, hud_debug_tab_step apart, hud_debug_tab_w by hud_debug_tab_h at
 *              drawer_y + hud_debug_tab_dy. The body starts at drawer_y + hud_debug_body_dy.
 *
 *              TEMPO. debug.buttons in a row, hud_debug_pace_step apart, hud_debug_pace_w by hud_debug_chip_h,
 *              chosen where its factor is DebugLayerFrame::factor, the one the clock runs at as the hourglass
 *              shows it (mockup 889: S.factor === f); the note texts.hud.debug_notes.tempo
 *              hud_debug_row_step below at hud_debug_note_dy.
 *              SALTO. debug.jumps in three rows by what they name: the stage jumps hud_debug_stage_step apart,
 *              chosen where the chosen patch stands at that stage; hud_debug_stage_row below the hour jumps,
 *              hud_debug_hour_per_row to a row, hud_debug_hour_step by hud_debug_hour_row apart; hud_debug_hour_block
 *              below every other jump, hud_debug_jump_step apart; the note hud_debug_row_step below at
 *              hud_debug_note_jump_dy.
 *              DATA. debug.date.fields hud_debug_date_step apart: the label at hud_debug_date_label_dy, the chip
 *              texts.hud.debug_date_less (the smallest step back of that part), the field hud_debug_date_field_dx
 *              in DEBUG_FIELD with the value middle at hud_debug_date_value_dx, _dy, and the chip
 *              texts.hud.debug_date_more (the smallest step ahead) at hud_debug_date_plus_dx, all at
 *              hud_debug_date_chip_dy. hud_debug_date_block below every step of the first field's part,
 *              hud_debug_vec_step apart, hud_debug_vec_w wide; hud_debug_row_step below go and now,
 *              hud_debug_go_w by hud_debug_go_h, hud_debug_go_step apart; hud_debug_go_block below the keys of
 *              the typed entry, debug.date.keys and then debug.date.clear, hud_debug_key_step apart,
 *              hud_debug_key_w wide; hud_debug_key_block below the note. A tap on a field cell picks its part
 *              for the keys (register N0475), and the picked cell wears DEBUG_GROUND with its value in
 *              DEBUG_INK_ON.
 *              RELEVO. debug.reliefs hud_debug_relief_step apart, chosen where its amplitude is
 *              DebugLayerFrame::height_scale_m, the one that stands (mockup 898: S.relief === r);
 *              the note hud_debug_row_step below at hud_debug_note_jump_dy.
 *
 *              THE CHIP. A rounded rectangle of hud_debug_chip_radius, DEBUG_GROUND when chosen and DEBUG_CHIP_OFF
 *              otherwise, a DEBUG_LINE outline of hud_debug_chip_line, the label middle at its centre plus
 *              hud_debug_chip_label_dy, class d of hud_debug_chip_label_size, DEBUG_INK_ON on a chosen chip and
 *              DEBUG_INK otherwise.
 *
 *              THE TONES. Every DEBUG_* colour is a tone of the dark mode of colors.ts (surface, text and border
 *              scale), the one every panel wears; the mockup's amber #FFB300 is replaced by operator order of
 *              2026-11-11 (PLAN_ASE_VIVARIUM_PHASE_06_HUD, rows B17, B18, X01 to X09). Every outline (handle,
 *              chip, field, drawer line) is DEBUG_LINE.
 *
 *              No backend gap: every value it shows is the adapter's own (build line, host clock, patch record
 *              and the asks of the section debug).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_046
 * @category    ecs/module
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
 * @created     2026-11-10
 * @modified    2026-11-25
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_debug_rsm.hpp>
#include <ase/adp/godot/godot_hud_debug_rsm_draw.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_vivarium_debug.hpp>
#include <ase/adp/godot/hud/debug/types.hpp>
#include <ase/adp/godot/rsm/types.hpp>
#include <ase/adp/godot/patch/types.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the debug layer shows this frame. */
struct DebugLayerFrame {
    bool open = false;                          // the drawer is open; false: the handle
    uint32_t tab = GODOT_HUD_DEBUG_TAB_TEMPO;   // GODOT_HUD_DEBUG_TAB_* the open drawer shows
    uint32_t view = GODOT_HUD_DEBUG_VIEW_SYSTEM;// GODOT_HUD_DEBUG_VIEW_* the open drawer shows: the tabs, the RSM table or the four lines
    std::vector<DebugRsmEntry> rsm;             // the learning entries of avatar.skill.v1 the RSM table shows, last set the host delivered
    uint8_t stage = GODOT_STAGE_NONE;           // the stage of the chosen patch; NONE: no patch chosen
    double factor = 0.0;                        // the factor the clock runs at, the one the hourglass fills
    double height_scale_m = 0.0;                // the relief amplitude that stands, metres
    std::string adapter;                       // the adapter's version (build.identity adapter_version)
    std::string client;                         // the client commit of the identity
    uint64_t units = 0;                         // units of the bundle
    double day = 0.0;                           // the day of the year of the bridged clock record, from 1
    double hour = 0.0;                          // the hour of the game day
    double simulation_s = 0.0;                  // simulation seconds the host has run
    uint64_t steps = 0;                         // ticks the host has run
    bool patch = false;                         // a patch is chosen and its record read
    double cover = 0.0;                         // its coverage, share 0..1
    double soil_mm = 0.0;                       // its soil water, millimetres
    double soil_rel = 0.0;                      // its soil water, share 0..1 of the field capacity
    uint64_t salt_chunks = 0;                   // chunks the salt port carries (hydrogeology.salt.v1, H.9 I-4); 0: no salt line
    double salt_hyd_kg = 0.0;                   // salt in the water of those chunks, kg, summed over the port's records
    double salt_soil_kg = 0.0;                  // salt in their soil, kg, summed the same way
    double salt_kg = 0.0;                       // the sum ase-gis added per chunk, kg, summed the same way
    double date[GODOT_DATE_PARTS] = {};         // the date the entry shows, by GODOT_DATE_PART_*
    uint8_t typing = GODOT_DATE_PART_NONE;      // GODOT_DATE_PART_* the keys type into; NONE: no field picked
    double elapsed = 0.0;                       // seconds since the layer appeared
    bool still = false;                         // every motion held
    HudCover reach;                             // the surface the open drawer reaches, set by the HUD
};

/** What a tap on the debug layer hit; GODOT_HUD_DEBUG_NONE in every index it did not hit. */
struct DebugLayerHit {
    bool handle = false;                        // the closed handle: open the drawer
    bool fold = false;                          // the fold: close the drawer
    uint32_t tab = GODOT_HUD_DEBUG_NONE;        // the tab chosen
    uint32_t view = GODOT_HUD_DEBUG_NONE;       // the tab layer of the drawer chosen (GODOT_HUD_DEBUG_VIEW_*)
    uint32_t button = GODOT_HUD_DEBUG_NONE;     // index in debug.buttons: ask that pace
    uint32_t jump = GODOT_HUD_DEBUG_NONE;       // index in debug.jumps: ask that jump
    uint32_t step = GODOT_HUD_DEBUG_NONE;       // index in debug.date.steps: move that part
    bool go = false;                            // debug.date.go: jump to the entry
    bool now = false;                           // debug.date.now: the entry back to the clock
    uint8_t field = GODOT_DATE_PART_NONE;       // GODOT_DATE_PART_* of the field cell hit: pick it for the keys
    uint32_t key = GODOT_HUD_DEBUG_NONE;        // index in debug.date.keys: type that digit
    bool clear = false;                         // debug.date.clear: empty the picked part
    uint32_t relief = GODOT_HUD_DEBUG_NONE;     // index in debug.reliefs: ask that relief
};

/** One chip of the open drawer, placed. */
struct DebugLayerChip {
    ::godot::Rect2 rect;                        // in mockup units
    std::string label;
    bool on = false;                            // chosen
    DebugLayerHit act;                          // what a tap on it asks
};

/** One field of the date entry, placed: the cell between its two chips with its label and value. */
struct DebugLayerField {
    ::godot::Rect2 cell;                        // in mockup units
    std::string label;                          // debug.date.fields label
    ::godot::Vector2 label_at;                  // start of the label
    ::godot::Vector2 value_at;                  // middle of the value
    uint8_t part = GODOT_DATE_PART_NONE;        // GODOT_DATE_PART_* the field shows
};

/** The body of the open drawer, placed: every chip, the date fields of DATA, and the line of the note. */
struct DebugLayerBody {
    std::vector<DebugLayerChip> chips;
    std::vector<DebugLayerField> fields;
    double note_y = 0.0;
};

/** The debug layer. */
class DebugLayer {
public:
    /** Binds measures, colours and words; false with the name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the status lines and the handle or the drawer of `now` over `debug`; false with the reason. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const VivariumDebug& debug,
              const DebugLayerFrame& now, std::string& failed);

    /** What `point` (in mockup units) hits, in `out`; false where it hits nothing of the layer. */
    bool hit(const ::godot::Vector2& point, const VivariumDebug& debug, const DebugLayerFrame& now,
             DebugLayerHit& out) const;

    /** Every place a tap on the layer answers (in mockup units), in `out`: closed the handle, open the fold
     *  and the chips of lay() with tabs; hit() asks the same places. False with the reason in `failed`. */
    bool places(const VivariumDebug& debug, const DebugLayerFrame& now, std::vector<DebugLayerChip>& out,
                std::string& failed) const;

    /** The body of the open drawer (in mockup units), in `out`; false while the drawer is closed. A tap on it that
     *  misses every chip stops there (the mockup's drawer rect carries no act), and draw() paints the same rect. */
    bool drawer_place(const DebugLayerFrame& now, ::godot::Rect2& out) const;

private:
    /** The open drawer's tabs and body for `now`, in `out` (godot_hud_debug_layer_lay.cpp); false with the reason. */
    bool lay(const VivariumDebug& debug, const DebugLayerFrame& now, DebugLayerBody& out, std::string& failed) const;
    bool lay_tempo(const VivariumDebug& debug, const DebugLayerFrame& now, double y, DebugLayerBody& out) const;
    bool lay_salto(const VivariumDebug& debug, const DebugLayerFrame& now, double y, DebugLayerBody& out) const;
    bool lay_data(const VivariumDebug& debug, double y, DebugLayerBody& out, std::string& failed) const;
    bool lay_relevo(const VivariumDebug& debug, const DebugLayerFrame& now, double y, DebugLayerBody& out) const;

    bool draw_lines(UiPen& canvas, const ::godot::Transform2D& frame, const DebugLayerFrame& now,
                    const UiTextPlace& text, std::string& failed);
    /** The three view chips in the title row of the open drawer, placed; the closed console shows none. */
    void view_chips(const DebugLayerFrame& now, std::vector<DebugLayerChip>& out) const;
    /** The view chips and the body of the chosen layer under them: the RSM table or the four status lines; the
     *  SYSTEM layer's tabs and chips draw() paints from lay(). */
    bool draw_view(UiPen& canvas, const ::godot::Transform2D& frame, const DebugLayerFrame& now,
                   const UiTextPlace& text, std::string& failed);
    bool chip(UiPen& canvas, const ::godot::Transform2D& frame, const DebugLayerChip& chip,
              const DebugLayerFrame& now, UiTextBlock& label, std::string& failed);
    /** Grows `blocks` to `count`, each new block bound as data text from theme_; false with the reason. */
    bool grown(std::vector<UiTextBlock>& blocks, size_t count, std::string& failed);

    double x_ = 0.0;                  // hud_debug_x
    double y_ = 0.0;                  // hud_debug_y
    double line_step_ = 0.0;          // hud_debug_line_step
    double line_size_ = 0.0;          // hud_debug_line_size
    ::godot::Rect2 handle_;           // hud_debug_handle_x, _y, _w, _h
    double handle_radius_ = 0.0;      // hud_debug_handle_radius
    double handle_text_x_ = 0.0;      // hud_debug_handle_text_x
    double handle_text_y_ = 0.0;      // hud_debug_handle_text_y
    double drawer_y_ = 0.0;           // hud_debug_drawer_y
    double drawer_h_ = 0.0;           // hud_debug_drawer_h
    double drawer_line_ = 0.0;        // hud_debug_drawer_line
    double pad_ = 0.0;                // hud_debug_pad
    double title_dy_ = 0.0;           // hud_debug_title_dy
    double title_size_ = 0.0;         // hud_debug_title_size
    double fold_x_ = 0.0;             // hud_debug_fold_x
    double fold_size_ = 0.0;          // hud_debug_fold_size
    ::godot::Rect2 fold_hit_;         // hud_debug_fold_hit_x, drawer_y, hud_debug_fold_hit_w, _h
    double tab_dy_ = 0.0;             // hud_debug_tab_dy
    double tab_step_ = 0.0;           // hud_debug_tab_step
    double tab_w_ = 0.0;              // hud_debug_tab_w
    double tab_h_ = 0.0;              // hud_debug_tab_h
    double body_dy_ = 0.0;            // hud_debug_body_dy
    double chip_h_ = 0.0;             // hud_debug_chip_h
    double row_step_ = 0.0;           // hud_debug_row_step
    double note_dy_ = 0.0;            // hud_debug_note_dy
    double note_jump_dy_ = 0.0;       // hud_debug_note_jump_dy
    double note_size_ = 0.0;          // hud_debug_note_size
    double pace_step_ = 0.0;          // hud_debug_pace_step
    double pace_w_ = 0.0;             // hud_debug_pace_w
    double stage_step_ = 0.0;         // hud_debug_stage_step
    double stage_w_ = 0.0;            // hud_debug_stage_w
    double stage_row_ = 0.0;          // hud_debug_stage_row
    double hour_step_ = 0.0;          // hud_debug_hour_step
    double hour_w_ = 0.0;             // hud_debug_hour_w
    double hour_row_ = 0.0;           // hud_debug_hour_row
    double hour_per_row_ = 0.0;       // hud_debug_hour_per_row
    double hour_block_ = 0.0;         // hud_debug_hour_block
    double jump_step_ = 0.0;          // hud_debug_jump_step
    double jump_w_ = 0.0;             // hud_debug_jump_w
    double registro_dy_ = 0.0;        // hud_debug_registro_dy, from the row of the other jumps to row REGISTRO
    double registro_row_ = 0.0;       // hud_debug_registro_row, one row of REGISTRO to the next
    double registro_per_row_ = 0.0;   // hud_debug_registro_per_row, event jumps in one row before it wraps
    double date_step_ = 0.0;          // hud_debug_date_step
    double date_label_dy_ = 0.0;      // hud_debug_date_label_dy
    double date_label_size_ = 0.0;    // hud_debug_date_label_size
    double date_chip_dy_ = 0.0;       // hud_debug_date_chip_dy
    double date_chip_w_ = 0.0;        // hud_debug_date_chip_w
    double date_field_dx_ = 0.0;      // hud_debug_date_field_dx
    double date_field_w_ = 0.0;       // hud_debug_date_field_w
    double date_value_dx_ = 0.0;      // hud_debug_date_value_dx
    double date_value_dy_ = 0.0;      // hud_debug_date_value_dy
    double date_value_size_ = 0.0;    // hud_debug_date_value_size
    double date_plus_dx_ = 0.0;       // hud_debug_date_plus_dx
    double date_block_ = 0.0;         // hud_debug_date_block
    double vec_step_ = 0.0;           // hud_debug_vec_step
    double vec_w_ = 0.0;              // hud_debug_vec_w
    double go_w_ = 0.0;               // hud_debug_go_w
    double go_h_ = 0.0;               // hud_debug_go_h
    double go_step_ = 0.0;            // hud_debug_go_step
    double go_block_ = 0.0;           // hud_debug_go_block
    double key_step_ = 0.0;           // hud_debug_key_step
    double key_w_ = 0.0;              // hud_debug_key_w
    double key_block_ = 0.0;          // hud_debug_key_block
    double relief_step_ = 0.0;        // hud_debug_relief_step
    double relief_w_ = 0.0;           // hud_debug_relief_w
    double chip_radius_ = 0.0;        // hud_debug_chip_radius
    double chip_line_ = 0.0;          // hud_debug_chip_line
    double chip_label_dy_ = 0.0;      // hud_debug_chip_label_dy
    double chip_label_size_ = 0.0;    // hud_debug_chip_label_size
    double view_x_ = 0.0;             // hud_debug_view_x
    double view_y_ = 0.0;             // the top of the view chips: hud_debug_drawer_y + hud_debug_view_dy
    double view_step_ = 0.0;          // hud_debug_view_step

    ::godot::Color ink_;              // DEBUG_INK
    ::godot::Color ink_on_;           // DEBUG_INK_ON
    ::godot::Color edge_;             // DEBUG_LINE
    ::godot::Color ground_;           // DEBUG_GROUND
    ::godot::Color chip_off_;         // DEBUG_CHIP_OFF
    ::godot::Color drawer_;           // DEBUG_DRAWER
    ::godot::Color field_;            // DEBUG_FIELD

    std::vector<HudRun> build_words_;   // texts.hud.debug_build
    std::vector<HudRun> time_words_;    // texts.hud.debug_time
    std::vector<HudRun> patch_words_;   // texts.hud.debug_patch
    std::vector<HudRun> salt_words_;    // texts.hud.debug_salt
    std::string less_;                  // texts.hud.debug_date_less
    std::string more_;                  // texts.hud.debug_date_more
    std::vector<std::string> tabs_;     // texts.hud.debug_tabs.<tab>, by GODOT_HUD_DEBUG_TAB_*
    std::vector<std::string> notes_;    // texts.hud.debug_notes.<tab>, by GODOT_HUD_DEBUG_TAB_*
    std::vector<std::string> views_;    // texts.hud.debug_views.<view>, by GODOT_HUD_DEBUG_VIEW_*

    UiTextBlock open_;              // texts.hud.debug_open, class d
    UiTextBlock close_;               // texts.hud.debug_close, class d
    UiTextBlock title_;               // texts.hud.debug_title, class d
    UiTextBlock note_;                // the note of the tab, class d

    // One block per line of text, so each keeps its shaped glyphs while its text and size stand (P0692): one block
    // set to another text or size for every line of a frame shaped again on every draw.
    const UiTheme* theme_ = nullptr;          // the theme bind() read, to bind a block a longer list adds
    UiTextBlock build_line_;                  // texts.hud.debug_build, class d
    UiTextBlock time_line_;                   // texts.hud.debug_time, class d
    UiTextBlock patch_line_;                  // texts.hud.debug_patch, class d
    UiTextBlock salt_line_;                   // texts.hud.debug_salt, class d
    DebugRsmTable rsm_table_;                 // the RSM table and its rings (godot_hud_debug_rsm_draw.hpp)
    std::vector<UiTextBlock> view_labels_;    // the label of each view chip
    std::vector<UiTextBlock> chip_labels_;    // the label of each chip of the open tab, by its place in the body
    std::vector<UiTextBlock> date_labels_;    // the label of each date field, by its place in the body
    std::vector<UiTextBlock> date_values_;    // the value of each date field, by its place in the body
};

}  // namespace ase::adp::godot

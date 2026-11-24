#pragma once

/**
 * ASE GODOT ADAPTER - HUD TOOL RACK
 *
 * @file        godot_hud_tool_rack.hpp
 * @brief       The tool place: the forked stakes in the world and, in the mode rack, the list of its long tools
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, block ToolRack (the mockup's drawWorld for the tool
 *              place and rackLayer).
 *
 *              IN THE WORLD. Two stakes MATERIAL_WOOD_DARK stand hud_world_rack_stake_half metres beside the
 *              place flora.carry.v1 reports, hud_world_rack_stake_h metres high at hud_world_rack_stake_line; the
 *              crossbar WORLD_RACK_BAR runs hud_world_rack_bar_half metres to each side, hud_world_rack_bar_rise
 *              metres up, at hud_world_rack_bar_line; under them lies the shadow WORLD_RACK_SHADOW, an ellipse
 *              hud_world_rack_shadow_rx by hud_world_rack_shadow_ry metres moved by hud_world_rack_shadow_dx and
 *              _dy. A metre is measured at the place's depth as WorldDrop measures it, and draw_world draws on
 *              the world layer WorldDrop darkens with the hour.
 *
 *              THE LIST (mode rack). The scrim OVERLAY_SCRIM_RACK over the band hud_rack_dim_y to
 *              hud_rack_dim_y + hud_rack_dim_h, stretched to the surface the HUD sets in `cover` (hud_cover_dim:
 *              from the band's end on the top band to the hand's top on the bottom band), takes a tap as rest. A post DOMAIN_GROUND of
 *              hud_rack_post_radius stands hud_rack_post_lift metres above the place; the leader runs from it over
 *              the knee (hud_rack_spine_x, post + hud_rack_elbow_dy) up to hud_rack_spine_top. The title
 *              texts.hud.rack_title (class tb) and the subtitle texts.hud.rack_sub with {place} stand at
 *              hud_rack_title_x; with no tool texts.hud.rack_empty. Every tool at the place gets a row, ordered
 *              along the place, hud_rack_row_step apart from hud_rack_row_y: a thing chip of
 *              hud_rack_cell_radius over UiHexChip with its picture over UiItemSprite, the name
 *              items.<id>.name (class tb), its verbs verbs.<v> joined by texts.hud.rack_verbs_join in
 *              DOMAIN_GROUND, from hud.perception.levels[1] on texts.hud.item_size and from levels[2] on
 *              texts.hud.item_asset after it (class d); each row fades in hud_rack_row_stagger after the one
 *              above it and is hit over hud_rack_hit_* as item with its key. With more than one row a spine
 *              OVERLAY_INK_FAINT joins the chips. The box of a chip's picture is hud_rack_cell_box (the
 *              mockup's POS), which DragLayer lifts a thing from.
 *
 *              THE DROP. A drag stands on the place within hud_drop_rack_radius of its foot lifted by
 *              hud_drop_rack_lift; DragLayer asks over() after the hand, the belt and the hip bag, and lays the
 *              thing in the next free slot with the place word where.rack.
 *
 *              FULL FROM THE FRAME. The tools at the place, their facts and the place itself are state of
 *              ase-inventory on flora.carry.v1. ToolRack draws whatever the frame carries and never asks
 *              whether the port delivered: until it does, the bridge VivariumCarryBridge
 *              (godot_vivarium_hud_carry_frame.cpp) writes the mockup's place and tools from SHA into the
 *              frame and VivariumView marks the gap N0451 in its own source (open_gap, godot_vivarium_hud.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

/** One tool at the tool place as flora.carry.v1 reports it. */
struct RackTool {
    std::string key;                 // o.key: the thing it is
    std::string id;                  // o.id: the item, which names its words (items.<id>.name)
    std::string art;                 // the item_art.json name of its drawing
    double along = 0.0;              // o.gu: its place along the rack, which orders the rows
    std::vector<std::string> verbs;  // D.verbs: what it does, each the key of verbs.<v>
    double length_cm = 0.0;          // D.len
    double kg = 0.0;                 // D.kg
    std::string asset;               // D.asset: the asset it is drawn from
};

/** The tool place in the world this frame. */
struct ToolRackWorld {
    const UiAnchor* anchor = nullptr;  // the projection of this frame
    ::godot::Vector3 place;            // the tool place on the ground
    bool bodied = false;               // the rack body stands in the 3D scene (lay_tools): nothing is drawn here, the sun casts its shadow
    bool struck = false;               // the camera ray of the last press met the rack body (rack_struck): D01 while bodied
};

/** The list of the mode rack this frame. */
struct ToolRackFrame {
    const UiAnchor* anchor = nullptr;            // the projection of this frame
    uint32_t level = 0;                          // the depth of perception
    const std::vector<RackTool>* tools = nullptr;// the tools at the place, nullptr while none
    ::godot::Vector3 place;                      // the tool place on the ground
    std::string area;                            // {place}: the number of the area the place belongs to
    double elapsed = 0.0;                        // the seconds since the mode rack opened
    bool still = false;                          // reduced motion: every animation at its end
    HudCover cover;                              // the surface the scrim reaches, set by the HUD
};

/** Where one tool's picture was drawn in the list (the mockup's POS entry). */
struct RackToolPlace {
    std::string key;       // the thing
    std::string id;        // the item it is (o.id), which the place line of the view names the row by
    ::godot::Rect2 box;    // the picture's box in frame units
    ::godot::Rect2 hit;    // the row's hit box
    double k = 0.0;        // the scale of its drawing
};

/** The tool place. */
class ToolRack {
public:
    /** Binds measures, colours, words and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The stakes, the bar and their shadow at the place; false with the reason in `failed`. */
    bool draw_world(UiPen& canvas, const ::godot::Transform2D& frame, const ToolRackWorld& now,
                    std::string& failed);

    /** The list of the mode rack and its hit boxes; false with the reason in `failed`. */
    bool draw_list(UiPen& canvas, const ::godot::Transform2D& frame, const ToolRackFrame& now,
                   std::string& failed);

    /** The act at `point` of the list as last drawn: item with the key in `key`, rest on the scrim, else empty. */
    std::string hit(const ::godot::Vector2& point, std::string& key) const;

    /** The tool rows as last drawn, the topmost first. */
    const std::vector<RackToolPlace>& places() const { return places_; }

    /** A drag at `point` stands on the tool place: within hud_drop_rack_radius of the place lifted by
     *  hud_drop_rack_lift (the mockup's dropTarget); false while the place is seen nowhere in the picture
     *  (no UiAnchor, or the place stands behind the camera). While the rack body stands in the scene (bodied,
     *  PLAN_ASE_VIVARIUM_PHASE_06_HUD 8.6) the place takes the drag where the camera ray through the finger meets
     *  the body (struck), or where `ground`, the ground under the finger (`ground_hit`), lies within the same radius
     *  measured in scene metres around the place: hud_drop_rack_radius over the frame length of one scene metre
     *  at the place's depth. */
    bool over(const ToolRackWorld& now, const ::godot::Vector2& point, bool ground_hit,
              const ::godot::Vector3& ground) const;

private:
    /** The sprite of the drawing `art`, read once; nullptr with the reason in `failed`. */
    const UiItemSprite* sprite(const std::string& art, std::string& failed);

    /** One chip of its own per row, bound on first use; nullptr with the reason in `failed`. */
    UiHexChip* chip(std::size_t row, std::string& failed);

    /** Draws `word` filled with `values` into `block` at `place`; false with the reason in `failed`. */
    static bool say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
                    const std::vector<HudRun>& word, const std::vector<HudPortValue>& values,
                    const UiTextPlace& place, std::string& failed);

    /** The verbs of `tool` joined by texts.hud.rack_verbs_join; false with the missing word in `failed`. */
    bool verbs_of(const RackTool& tool, std::vector<HudRun>& out, std::string& failed) const;

    /** A mass as the mockup's kgf prints it; a length as a plain number. */
    std::string number(double value, bool fine) const;

    /**
     * The frame point of `place` and the frame length of one metre of the mockup's world there: the scene's
     * metre at that place times hud_world_metre_scale. False when it lies behind the camera.
     */
    bool metre(const UiAnchor& anchor, const ::godot::Vector3& place, ::godot::Vector2& at, double& length) const;

    const UiTheme* theme_ = nullptr;
    double metre_scale_ = 0.0;                                                      // hud_world_metre_scale
    // drawWorld
    double shadow_dx_ = 0.0, shadow_dy_ = 0.0, shadow_rx_ = 0.0, shadow_ry_ = 0.0;  // hud_world_rack_shadow_*
    double stake_half_ = 0.0, stake_h_ = 0.0, stake_line_ = 0.0;                    // hud_world_rack_stake_*
    double bar_half_ = 0.0, bar_rise_ = 0.0, bar_line_ = 0.0;                       // hud_world_rack_bar_*
    // rackLayer
    double dim_y_ = 0.0, dim_h_ = 0.0;                                              // hud_rack_dim_*
    double post_radius_ = 0.0, post_lift_ = 0.0;                                    // hud_rack_post_*
    double spine_x_ = 0.0, elbow_dy_ = 0.0, spine_top_ = 0.0, leader_delay_ = 0.0;  // the leader
    double title_x_ = 0.0, title_y_ = 0.0, title_size_ = 0.0, title_delay_ = 0.0;   // hud_rack_title_*
    double sub_y_ = 0.0, sub_size_ = 0.0, sub_delay_ = 0.0;                         // hud_rack_subtitle_*
    double empty_y_ = 0.0, empty_size_ = 0.0;                                       // hud_rack_empty_*
    double row_y_ = 0.0, row_step_ = 0.0, row_stagger_ = 0.0;                       // hud_rack_row_*
    double cell_radius_ = 0.0, cell_box_ = 0.0, cell_delay_ = 0.0;                  // hud_rack_cell_*
    double text_x_ = 0.0;                                                           // hud_rack_text_x
    double name_dy_ = 0.0, name_size_ = 0.0, name_delay_ = 0.0;                     // hud_rack_name_*
    double verbs_dy_ = 0.0, verbs_size_ = 0.0, verbs_delay_ = 0.0;                  // hud_rack_verbs_*
    double facts_dy_ = 0.0, facts_size_ = 0.0, facts_delay_ = 0.0;                  // hud_rack_facts_*
    double hit_x_ = 0.0, hit_dy_ = 0.0, hit_w_ = 0.0, hit_h_ = 0.0;                 // hud_rack_hit_*
    double decimals_ = 0.0, fine_below_ = 0.0, fine_decimals_ = 0.0;                // hud_load_*
    double drop_radius_ = 0.0, drop_lift_ = 0.0;                                    // hud_drop_rack_*
    uint32_t level_facts_ = 0;                                                      // hud.perception.levels[1]
    uint32_t level_asset_ = 0;                                                      // hud.perception.levels[2]
    ::godot::Color wood_dark_, bar_, shadow_, scrim_, ground_, soft_, faint_;
    std::vector<HudRun> title_, sub_, empty_, join_, size_, asset_;
    UiTextBlock title_text_;   // class tb: the title and the names
    UiTextBlock label_text_;   // class t: the subtitle, the empty line and the verbs
    UiTextBlock data_text_;    // class d: the facts
    UiLeaderLine line_;        // the leader and the spine
    std::vector<UiHexChip> chips_;                              // one per row
    std::vector<std::pair<std::string, UiItemSprite>> sprites_;  // one per drawing, read on first use
    std::vector<RackToolPlace> places_;                          // the rows as last drawn
    ::godot::Rect2 scrim_box_;                                   // the scrim as last drawn
    bool listed_ = false;                                        // the list was drawn this frame
};

}  // namespace ase::adp::godot

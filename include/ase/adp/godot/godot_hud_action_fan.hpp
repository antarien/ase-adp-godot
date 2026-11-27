#pragma once

/**
 * ASE GODOT ADAPTER - HUD ACTION FAN
 *
 * @file        godot_hud_action_fan.hpp
 * @brief       The fan of tools left of the focus ring: one hexagon per tool whose condition holds now
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block ActionFan (the tool part of the mockup's
 *              focus(), drawn in the mode focus beside FocusCluster).
 *
 *              WHICH TOOLS. ContextResolver decides them (HudShown::fan): a tool of hud.focus.fan.tools shows
 *              while every mark of hud.tools.<name>.requires holds and its hud.tools.<name>.has is in
 *              possession, in the order of the places. This block draws what it is handed and decides nothing.
 *
 *              THE PLACES. Place n (from 1) sits hud_fan_<n>_dx right of and hud_fan_<n>_dy below the centre
 *              of the focus ring (FocusRing::centre), for as many places as hud.focus.fan.offsets has; each
 *              chip has the radius hud_fan_radius and fades in after hud_fan_delay plus hud_fan_stagger per
 *              place.
 *
 *              THE CHIP. Every chip is drawn in its tool's domain, the token hud_fan_<name>_tone (DOMAIN_<code>).
 *              Every chip is a thing chip (UiHexChip::set_thing) with the tool's activity symbol (MIG_ASE_SKILL_RSM
 *              B7, usecase:101440): the symbol activity_art.json puts on the place tool.<name>
 *              (UiActivityArt::place), drawn by UiItemSprite. It shows the action on its object, not only the
 *              tool: the drop into the soil for water, the cut on the branch for prune, the tool place for the
 *              ground tools. A tool without a symbol on its place refuses the binding, no stand-in is drawn. At
 *              perception levels[0] each chip carries its name texts.hud.tools.<name> as label, above it none.
 *
 *              A TAP on a chip names its tool: a tool with an item takes that item into the hand (data-act
 *              take:<name>), a tool without one opens the tool place (data-act rack).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-25
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_context_resolver.hpp>
#include <ase/adp/godot/godot_hud_ui_activity_art.hpp>
#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the fan shows this frame. */
struct ActionFanFrame {
    ::godot::Vector2 centre;          // the centre of the focus ring (FocusRing::centre), in the mockup's units
    std::vector<HudFanTool> tools;    // HudShown::fan: the tools whose condition holds, with their places
    uint32_t level = 0;               // the perception level (hud.perception.levels)
    double elapsed = 0.0;             // seconds since the focus opened
    bool still = false;               // every motion held
};

/** What a tap on the fan names. */
struct ActionFanHit {
    uint32_t tool = UINT32_MAX;  // index into HudContextSpec::tools
    bool take = false;           // the tool serves through its item: take it into the hand
    bool rack = false;           // the tool has no item: open the tool place
};

/** One chip of the fan as a tap reaches it: the square round its hexagon (in the mockup's units), its centre the
 *  chip's, and what hit() names there. */
struct ActionFanPlace {
    ::godot::Rect2 rect;
    ActionFanHit act;
};

/** The fan of tools. */
class ActionFan {
public:
    /** The activity symbols the chips draw; bound before bind() (VivariumHud::bind_activities). */
    void bind_activities(const UiActivityArt& activities) { activities_ = &activities; }

    /** Binds the places, the delays, every tool's chip, symbol, tone and name; false with the reason in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the fan of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const ActionFanFrame& now,
              std::string& failed);

    /** True with the tool in `out` when `point` (in the mockup's units) lies on a chip drawn last. */
    bool hit(const ::godot::Vector2& point, ActionFanHit& out) const;

    /** Every chip drawn last as a tap reaches it, in `out`, in the order hit() asks them. */
    void places(std::vector<ActionFanPlace>& out) const;

private:
    /** One tool of hud.tools as the fan draws it. */
    struct FanTool {
        bool thing = false;          // the tool serves through an item
        ::godot::Color tone;         // hud_fan_<name>_tone, the tool's domain
        std::string label;           // texts.hud.tools.<name>
        UiItemSprite drawing;        // the activity symbol on the place tool.<name> (UiActivityArt::place)
    };

    /** The chip at place `place` drawn last, for hit(). */
    struct FanDrawn {
        uint32_t tool = UINT32_MAX;
        UiHexChipPlace place;
    };

    /** Reads the tool `spec` into `out`; false with the reason in `failed`. */
    bool bind_tool(const UiTheme& theme, const HudToolSpec& spec, FanTool& out, std::string& failed) const;

    double radius_ = 0.0;       // hud_fan_radius
    double delay_ = 0.0;        // hud_fan_delay
    double stagger_ = 0.0;      // hud_fan_stagger
    uint32_t level_words_ = 0;  // hud.perception.levels[0]: the chips carry their names
    const UiActivityArt* activities_ = nullptr;  // bind_activities

    std::vector<::godot::Vector2> offsets_;  // hud_fan_<n>_dx, hud_fan_<n>_dy, one per place
    std::vector<FanTool> tools_;             // one per HudContextSpec::tools, in its order
    std::vector<UiHexChip> chips_;           // one per place
    std::vector<FanDrawn> drawn_;            // the chips drawn last
};

}  // namespace ase::adp::godot

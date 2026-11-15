#pragma once

/**
 * ASE GODOT ADAPTER - HUD THEME
 *
 * @file        godot_hud_theme.hpp
 * @brief       Every colour, stroke, radius, gap and duration the HUD draws with, and its chips, read from `hud`
 * @description The HUD units (godot_hud_*) draw nothing with a number of their own: the section `hud` of
 *              vivarium_start.json holds every value, and this unit is its one reader. It reads through the
 *              readers of godot_config_read.hpp and takes the icons through read_icon of
 *              godot_view_support.hpp, from the icon table and font the section `debug` names - no second
 *              path to either.
 *
 *              A CHIP CARRIES THE LABEL OF ITS BUTTON. Every chip names a key of the section `buttons` and
 *              takes that button's label: the text stands once, and the chip triggers the action the button
 *              triggered (PLAN_ASE_VIVARIUM_PHASE_06_HUD, Vom Fingertipp zur Handlung). A chip whose key
 *              `buttons` does not name is a broken configuration.
 *
 *              THE RULES STAND APART FROM THE CHIPS. `rules` runs parallel to `chips`, index for index, in
 *              the form HudContext reads (godot_hud_context.hpp): the context is resolved every frame and
 *              takes the vector as it stands, without copying a field out of a chip.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_context.hpp>

#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace ase::adp::godot {

/** One chip of the spine (hud.chips), what it draws. */
struct HudChip {
    ::godot::String  id;           // the key of the section buttons whose action the chip triggers
    ::godot::String  label;        // that button's label (buttons.<id>.label)
    ::godot::String  icon;         // the glyph of its ICON_* in SHA's icon table
    ::godot::Vector2 icon_offset;  // the offset Icon.tsx gives that glyph, CSS pixels at icon_web_px
    ::godot::Color   tone;         // the colour of its domain (hud.tones.<tone>)
};

/** The section hud as the HUD units draw with it. */
struct HudTheme {
    ::godot::Color ink;                         // HUD white, full (hud.ink)
    ::godot::Color ink_mid;                     // HUD white, middle opacity (hud.ink_mid)
    ::godot::Color ink_low;                     // HUD white, low opacity (hud.ink_low)
    ::godot::Color shade;                       // the dark face under a chip (hud.shade)
    ::godot::Ref<::godot::FontFile> icon_font;  // the Nerd Font of the icons (debug.icon_font)
    double icon_web_px = 0.0;                   // the font size the icon offsets are meant for (debug.icon_table)
    double line_px = 0.0;                       // stroke of an outline (hud.line_px)
    double line_on_px = 0.0;                    // stroke of the outline of a running chip (hud.line_on_px)
    double ring_radius_px = 0.0;                // radius of the focus ring around the selected patch (hud.ring_radius_px)
    double chip_radius_px = 0.0;                // radius of a hex chip, centre to corner (hud.chip_radius_px)
    double chip_gap_px = 0.0;                   // gap between two chips on the spine (hud.chip_gap_px)
    double spine_x_px = 0.0;                    // x of the spine on the logical surface (hud.spine_x_px)
    double spine_top_px = 0.0;                  // y of the top edge of the first chip (hud.spine_top_px)
    double leader_px = 0.0;                     // stroke of a leader line (hud.leader_px)
    double rail_x_px = 0.0;                     // x of the edge rail on the logical surface (hud.rail_x_px)
    double rail_dot_px = 0.0;                   // radius of a dot of the edge rail (hud.rail_dot_px)
    double label_px = 0.0;                      // font size of a chip label (hud.label_px)
    double toast_s = 0.0;                       // real seconds a toast stands (hud.toast_s)
    std::vector<HudChip> chips;                 // the chips of the spine, top to bottom (hud.chips)
    std::vector<HudChipRule> rules;             // their rules, index for index (hud.chips[].needs_place, debug_only)
};

/**
 * Reads the section hud of `root` into `theme`. `icons` and `icon_font` are the icon table and font of the
 * section debug (read_icon_table, icon_font_of), `icon_web_px` the size of that table. False with the
 * failing key in `failed_key` when a value is missing, of the wrong type, not positive where a size is
 * meant, a tone or icon the tables do not name, or a chip key the section buttons does not name.
 */
bool read_hud_theme(const ::godot::Dictionary& root, const ::godot::Dictionary& icons,
                    const ::godot::Ref<::godot::FontFile>& icon_font, double icon_web_px, HudTheme& theme,
                    const char*& failed_key);

}  // namespace ase::adp::godot

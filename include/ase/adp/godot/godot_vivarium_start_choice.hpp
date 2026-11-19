#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM START CHOICE
 *
 * @file        godot_vivarium_start_choice.hpp
 * @brief       VivariumStartChoice - the two switches and the start chip of the start screen
 * @description The start screen holds the world until the keeper chose how it starts (operator order of
 *              2026-11-19). The choice stands where the loading line stands later, in the HUD's own form: hex
 *              chips of UiHexChip, the form of the action fan, with line weight, fill, glyph and label from the
 *              theme of SHA (res://config/design_tokens.json, translations_hud.json), nothing drawn by hand.
 *
 *              THE FORM. The first row carries online and offline (start.online, start.offline with their glyphs
 *              start_glyph.*), the chosen one lit in hud_start_chip_tone, the other at rest. Offline only, a
 *              second row carries a new world and the last kept state (start.initial, start.resume). The start
 *              chip (start.begin, hud_start_begin_tone) stands lit under the last row. Radius, spread and row
 *              gap are hud_start_chip_radius, hud_start_chip_spread and hud_start_chip_row_gap.
 *
 *              THE TOUCH. tap() switches a row and answers GODOT_START_MODE_NONE, or answers the chosen mode
 *              when the start chip is tapped. Before the first draw nothing is placed and every tap misses.
 *              At rest the switches stand offline and on the last kept state, the start the vivarium made
 *              before the choice existed; chosen() answers that mode for a launch that takes no taps (self
 *              test, measurement, asset showcase).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-19
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/** The choice of the start screen: online or offline, offline a new world or the last kept state. */
class VivariumStartChoice {
public:
    /** Binds measures, tones, glyphs and words from the theme; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /**
     * Draws the rows under `frame`, the first row centred on `anchor` (mockup units); `elapsed` is the time the
     * screen stands, `still` holds every fade at its end.
     */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& anchor, double elapsed,
              bool still);

    /** A tap at `point` (mockup units): switches a row and answers NONE, or answers the mode the start chip starts. */
    uint8_t tap(const ::godot::Vector2& point);

    /** The mode the switches stand on now. */
    uint8_t chosen() const;

private:
    UiHexChip online_;                  // start.online, start_glyph.online
    UiHexChip offline_;                 // start.offline, start_glyph.offline
    UiHexChip initial_;                 // start.initial, start_glyph.initial
    UiHexChip resume_;                  // start.resume, start_glyph.resume
    UiHexChip begin_;                   // start.begin, start_glyph.begin
    UiHexChipPlace online_at_;          // the places of the last draw, which tap() matches against
    UiHexChipPlace offline_at_;
    UiHexChipPlace initial_at_;
    UiHexChipPlace resume_at_;
    UiHexChipPlace begin_at_;
    ::godot::Color tone_;               // DOMAIN_<hud_start_chip_tone>
    ::godot::Color begin_tone_;         // DOMAIN_<hud_start_begin_tone>
    double radius_ = 0.0;               // hud_start_chip_radius
    double spread_ = 0.0;               // hud_start_chip_spread
    double row_gap_ = 0.0;              // hud_start_chip_row_gap
    bool offline_chosen_ = true;        // the first row stands on offline
    bool resume_chosen_ = true;         // the second row stands on the last kept state
    bool placed_ = false;               // a draw placed the chips
};

}  // namespace ase::adp::godot

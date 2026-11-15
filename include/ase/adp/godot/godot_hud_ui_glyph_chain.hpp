#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI GLYPH CHAIN
 *
 * @file        godot_hud_ui_glyph_chain.hpp
 * @brief       The chain of the day's phases in the sky band, with the wandering sun or moon over it
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiGlyphChain. The mockup's band() draws,
 *              with x(h) = hud_band_chain_x + h * hud_band_chain_step and the chain's height
 *              y = hud_band_chain_y:
 *
 *                line     L from x(first start) to x(last end) in OVERLAY_INK_FAINT (--hud3), the line width
 *                         hud_line, drawn in with class draw
 *                light    the hours of daylight from sunrise to sunset as a line of hud_band_light_line in
 *                         DOMAIN_CUT at hud_band_light_alpha
 *                phases   phase p at x of the middle of its hours: the current one a disc of
 *                         hud_band_phase_on_radius in DOMAIN_TIME, every other one a disc of
 *                         hud_band_phase_off_radius in OVERLAY_DOT_DARK stroked in OVERLAY_INK_SOFT at hud_line;
 *                         under it its word (class t, hud_band_phase_label_size, middle) at
 *                         hud_band_phase_label_y, in DOMAIN_TIME when current and OVERLAY_INK_SOFT otherwise
 *                body     ICON_SUN in DOMAIN_CUT by day, ICON_MOON in DOMAIN_SKY by night, centred at
 *                         (x(hour), hud_band_sun_y), size hud_band_sun_size
 *
 *              THE HOURS ARE NOT THIS UNIT'S. The phases, their hours, the current phase, the clock's hour,
 *              sunrise, sunset and night are ase-time's day and ase-ephemeris' sun, delivered through the
 *              clock record; the sky band that owns the chain passes them, with the phase words from the
 *              texts of the language. The chain computes nothing of the day; it places what it is given.
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

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One phase of the day, in the order of the day. */
struct UiGlyphChainPhase {
    std::string label;     // the phase's word from the texts (phases.aso .. phases.nox)
    double start = 0.0;    // the hour the phase begins
    double end = 0.0;      // the hour the next phase begins
};

/** The state of the day the chain shows this frame. */
struct UiGlyphChainDay {
    double hour = 0.0;     // the clock's hour: where the body stands
    double sunrise = 0.0;  // the hour the sun rises: the start of the light line
    double sunset = 0.0;   // the hour the sun sets: the end of the light line
    bool night = false;    // the sun is under the horizon: the moon stands instead of the sun
    double elapsed = 0.0;  // seconds since the band appeared
    bool still = false;    // every motion held
};

/** The phase chain of the sky band. */
class UiGlyphChain {
public:
    /** Binds the band's chain tokens, the inks, both bodies and the primitives; false with the missing one in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The phases of the day and the place of the current one in them; false with the reason in `failed`. */
    bool set(const std::vector<UiGlyphChainPhase>& phases, int32_t current, std::string& failed);

    /** Draws the chain under `frame` for `day`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiGlyphChainDay& day);

private:
    /** x(h): the chain's x of the hour `hour`. */
    double at(double hour) const;

    const UiTheme* theme_ = nullptr;
    UiLeaderLine line_;                 // the chain's line and the off dots' line width
    UiRing ring_;                       // the phase dots
    UiIcon sun_;                        // ICON_SUN
    UiIcon moon_;                       // ICON_MOON
    ::godot::Color faint_;              // OVERLAY_INK_FAINT: --hud3
    ::godot::Color soft_;               // OVERLAY_INK_SOFT: --hud2
    ::godot::Color time_;               // DOMAIN_TIME: --time
    ::godot::Color cut_;                // DOMAIN_CUT: --cut
    ::godot::Color sky_;                // DOMAIN_SKY: --sky
    ::godot::Color dark_;               // OVERLAY_DOT_DARK: the fill of an unchosen phase
    double x_ = 0.0;                    // hud_band_chain_x
    double step_ = 0.0;                 // hud_band_chain_step
    double y_ = 0.0;                    // hud_band_chain_y
    double light_line_ = 0.0;           // hud_band_light_line
    double light_alpha_ = 0.0;          // hud_band_light_alpha
    double on_radius_ = 0.0;            // hud_band_phase_on_radius
    double off_radius_ = 0.0;           // hud_band_phase_off_radius
    double label_y_ = 0.0;              // hud_band_phase_label_y
    double label_size_ = 0.0;           // hud_band_phase_label_size
    double sun_y_ = 0.0;                // hud_band_sun_y
    double sun_size_ = 0.0;             // hud_band_sun_size
    std::vector<UiGlyphChainPhase> phases_;
    std::vector<UiTextBlock> labels_;   // one word per phase, class t
    int32_t current_ = 0;
};

}  // namespace ase::adp::godot

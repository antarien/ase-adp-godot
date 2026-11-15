#pragma once

/**
 * ASE GODOT ADAPTER - HUD BROOK REFLECTION
 *
 * @file        godot_hud_brook_reflection.hpp
 * @brief       The sky the brook reflects and the strokes of its current, derived for the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N07 (m.html drawWater 429 to 433, design_tokens.json
 *              section VIVARIUM BROOK REFLECTION). The mockup fills the brook with a gradient: at its far reach the
 *              horizon's colour turned toward BROOK_TONE_FAR by brook_tone_far_share, at its near reach the colour of
 *              the sky above turned toward BROOK_TONE_NEAR by brook_tone_near_share, both as skyCols draws them, the
 *              overcast greys mixed in. Over it brook_streak_count strokes in BROOK_STREAK run along the middle line,
 *              brook_streak_gap of the course apart, each brook_streak_length of it long and brook_streak_width
 *              mockup units wide, moving brook_streak_speed of the course per second. The mockup's fill is opaque;
 *              the operator's order keeps the brook water murky green, so the reflection is light added on top at
 *              brook_reflect_share and never takes anything of the water's own colour. The block derives the colours
 *              and measures as a shade; the scene hands it to the water (light_plant_brook_reflection,
 *              godot_plant_brook_reflection.hpp). Nothing of the water is changed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>    // VivariumStreamView, the course the strokes follow
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_plant_brook_reflection.hpp>  // BrookReflectionShade, the shade the scene hands the water

#include <godot_cpp/variant/color.hpp>

#include <string>

namespace ase::adp::godot {

/** What the brook's reflection follows this frame. */
struct BrookReflectionFrame {
    ::godot::Color horizon;                     // the horizon's colour as the sky shader draws it, the overcast grey mixed in
    ::godot::Color above;                       // the colour of the sky above as the sky shader draws it, the overcast grey mixed in
    double frame_pixels = 0.0;                  // REQUIRED: pixels of the screen per mockup unit (the HUD frame's scale)
    const VivariumStreamView* stream = nullptr; // REQUIRED: the brook whose course the strokes follow
};

/** The sky the brook reflects and the strokes of its current. */
class BrookReflection {
public:
    /** Binds the seven measures and the three colours; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The shade of `now`; false with the reason in `failed` for a frame without stream, scale or course. */
    bool shade(const BrookReflectionFrame& now, BrookReflectionShade& out, std::string& failed) const;

private:
    double reflect_share_ = 0.0;    // brook_reflect_share: share of the reflected sky added as light over the murky green
    double tone_far_share_ = 0.0;   // brook_tone_far_share
    double tone_near_share_ = 0.0;  // brook_tone_near_share
    double streak_count_ = 0.0;     // brook_streak_count
    double streak_gap_ = 0.0;       // brook_streak_gap
    double streak_length_ = 0.0;    // brook_streak_length
    double streak_width_ = 0.0;     // brook_streak_width, mockup units
    double streak_speed_ = 0.0;     // brook_streak_speed
    ::godot::Color tone_far_;       // BROOK_TONE_FAR
    ::godot::Color tone_near_;      // BROOK_TONE_NEAR
    ::godot::Color streak_;         // BROOK_STREAK
};

}  // namespace ase::adp::godot

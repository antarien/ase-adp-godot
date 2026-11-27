#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI ARC FADE
 *
 * @file        godot_hud_ui_arc_fade.hpp
 * @brief       UiArcFade: the opacity of a point of the sky arc from its x place, the ends fade out
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, mockup arcFade and arcMask. The mockup masks the three
 *              rings with a gradient over the frame's width; Godot knows no mask on the canvas, so each point of a ring
 *              (a tick, a sign, a word, a piece of track) takes its opacity from its x place: the line through the
 *              stops hud_arc_fade_0 .. hud_arc_fade_5 (a share of hud_frame_width) and their opacities
 *              hud_arc_fade_<n>_alpha. The ends fade out over the clock and the battery and, on the Pixel 8, over the
 *              camera cut-out. The line itself is arc_fade_opacity (godot_hud_arc_pure.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_pure.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/variant/color.hpp>

#include <string>

namespace ase::adp::godot {

/** The fade of the arc's ends. */
class UiArcFade {
public:
    /** Binds the six stops and their opacities and the frame's width; false with the missing token in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The opacity of a point at `x`, in the mockup's units of the frame. */
    double opacity(double x) const;

    /** `color` with its alpha multiplied by `opacity`. */
    static ::godot::Color faded(const ::godot::Color& color, double opacity);

private:
    ArcFadeStops stops_;       // hud_arc_fade_<n> and hud_arc_fade_<n>_alpha
    double frame_width_ = 0.0;  // hud_frame_width
};

}  // namespace ase::adp::godot

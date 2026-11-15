#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI MOTION
 *
 * @file        godot_hud_ui_motion.hpp
 * @brief       The one curve of every animation of the HUD: a duration and a CSS easing of SHA's design tokens
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, building block Basis. The mockup animates three
 *              ways: .fade (opacity 0 to 1), .draw (a stroke drawn from its start, stroke-dashoffset 1 to 0)
 *              and .pulse (a ring that grows and fades, endlessly). Each is a duration and an easing that
 *              theme.css carries as --ase-hud-anim-* and design_tokens.json as hud_anim_<name>_ms and
 *              hud_anim_<name>_ease; UiMotion is the one place that turns them into a progress, so a glyph,
 *              a text, an arc and a pulse ease the same way and no block computes its own curve.
 *
 *              THE EASING AS CSS DEFINES IT. A token names linear, ease, ease-in, ease-out, ease-in-out or
 *              cubic-bezier(x1, y1, x2, y2); a keyword is its bezier of CSS Easing Functions Level 1 (the
 *              control points in types.hpp), and a bezier is solved for the parameter whose x is the time
 *              share, its y the progress - what the browser does for the mockup. A token naming any other
 *              easing refuses bind with its name; there is no stand-in curve.
 *
 *              ONCE AND ENDLESS. progress answers a one-time animation that starts `delay` seconds after the
 *              block appeared (animation-delay of the mockup) and holds at 1; cycle answers an infinite one
 *              (the pulse) as the eased share of the current turn. still is the mockup's .still and its
 *              prefers-reduced-motion: every one-time animation stands at its end.
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

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <string>

namespace ase::adp::godot {

/** One animation of the HUD: its duration and its easing curve. */
class UiMotion {
public:
    /**
     * Binds the tokens <name>_ms and <name>_ease of the theme (name: hud_anim_fade, hud_anim_draw,
     * hud_anim_pulse). False with `failed` naming the token that is missing, not positive or not an easing.
     */
    bool bind(const UiTheme& theme, const std::string& name, std::string& failed);

    /** The eased progress 0..1 of a one-time animation `elapsed` seconds after appearing, starting after `delay`. */
    double progress(double elapsed, double delay, bool still) const;

    /** The eased share 0..1 of the current turn of an endless animation `elapsed` seconds after appearing. */
    double cycle(double elapsed) const;

    /** The duration in seconds. */
    double seconds() const { return seconds_; }

private:
    /** The bezier's y at the time share `time` (0..1). */
    double ease(double time) const;

    double seconds_ = 0.0;  // <name>_ms in seconds
    double x1_ = 0.0;       // the curve's first control point
    double y1_ = 0.0;
    double x2_ = 0.0;       // the curve's second control point
    double y2_ = 0.0;
};

}  // namespace ase::adp::godot

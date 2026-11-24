#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI TEXT PLACE
 *
 * @file        godot_hud_ui_text_place.hpp
 * @brief       Where and how one line of HUD text is drawn this frame: the mockup's x, y and o of T
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiTextBlock. A block lays the place of its
 *              line every frame - the baseline point, the font size, the anchor, the fill, the opacity, the fade
 *              and the one rotation the mockup uses - and hands it to UiTextBlock::draw. Cut from
 *              godot_hud_ui_text_block.hpp by its circle of readers (header ratchet, WRFL_ASE_TIER_WIPE rule 1):
 *              the place is what a block writes, the line's class is what draws it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

/** Where and how one line is drawn this frame: the mockup's x, y and o. */
struct UiTextPlace {
    ::godot::Vector2 at;           // x, y: the baseline point the anchor stands on, in the mockup's units
    double size = 0.0;             // o.z: the font size in the mockup's units
    double anchor = 0.0;           // o.a: the share of the width left of `at` (GODOT_TEXT_ANCHOR_*)
    ::godot::Color color;          // o.col: the fill; the caller passes ink() where the mockup gives none
    double opacity = 0.0;          // o.op: the opacity of the whole text
    bool fade = false;             // o.fade: fades in with hud_anim_fade
    double delay = 0.0;            // o.d: the seconds before the fade starts
    double elapsed = 0.0;          // the seconds since the text appeared
    bool still = false;            // .still and reduced motion: every animation at its end
    bool bare = false;             // a <text> written past T, without the class sh: no outline (m.html 756)
    double rotate_deg = 0.0;       // o.tr rotate(a px py): the angle, clockwise in degrees
    ::godot::Vector2 pivot;        // o.tr rotate(a px py): the point it turns about
};

}  // namespace ase::adp::godot

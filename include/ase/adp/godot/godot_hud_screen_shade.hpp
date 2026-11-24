#pragma once

/**
 * ASE GODOT ADAPTER - HUD SCREEN SHADE
 *
 * @file        godot_hud_screen_shade.hpp
 * @brief       The shade over the scene and under every overlay: vignette, head of the screen, foot of the screen
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block ScreenShade (the last three layers of the
 *              mockup's drawScene, drawn after the weather and before any overlay).
 *
 *              THE VIGNETTE. A radial ramp around (hud_shade_vignette_x, hud_shade_vignette_y): OVERLAY_CLEAR up to
 *              hud_shade_vignette_inner, rising to OVERLAY_VIGNETTE at hud_shade_vignette_outer and staying there
 *              beyond. It is one square ramp texture of twice the outer radius, of which the part over the
 *              surface (HudCover::surface, the frame hud_frame_width by hud_frame_height on a surface of the
 *              mockup's aspect) is drawn; past the ramp the edge texel, OVERLAY_VIGNETTE, carries on.
 *
 *              THE HEAD. On the top band (HudCover::head): from its top edge OVERLAY_SCRIM_TOP fading to
 *              OVERLAY_CLEAR at hud_shade_head_h; above the band, up to the surface's top, OVERLAY_SCRIM_TOP.
 *
 *              THE FOOT. On the bottom band (HudCover::hand): from hud_shade_foot_y OVERLAY_CLEAR deepening to
 *              OVERLAY_SHADOW at hud_frame_height; below it, down to the surface's foot, OVERLAY_SHADOW.
 *
 *              No backend: the shade depends on nothing a port delivers.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/classes/gradient_texture2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>

namespace ase::adp::godot {

/** The shade over the scene. */
class ScreenShade {
public:
    /** Binds the measures and colours and builds the vignette ramp; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws vignette, head and foot under `frame` over `cover`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const HudCover& cover,
              std::string& failed) const;

private:
    double height_ = 0.0;         // hud_frame_height: the foot band ends there
    double head_h_ = 0.0;         // hud_shade_head_h
    double foot_y_ = 0.0;         // hud_shade_foot_y
    ::godot::Vector2 ramp_at_;    // the frame's origin in texels of the vignette ramp
    double texel_ = 0.0;          // texels of the ramp per frame unit
    ::godot::Color clear_, scrim_top_, shadow_;
    ::godot::Ref<::godot::GradientTexture2D> vignette_;
    bool bound_ = false;
};

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD SCREEN SHADE
 *
 * @file        godot_hud_screen_shade.cpp
 * @brief       The shade over the scene and under every overlay: vignette, head of the screen, foot of the screen
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_screen_shade.hpp.
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

#include <ase/adp/godot/godot_hud_screen_shade.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool ScreenShade::bind(const UiTheme& theme, std::string& failed) {
    bound_ = false;
    double centre_x = 0.0;
    double centre_y = 0.0;
    double inner = 0.0;
    double outer = 0.0;
    for (const std::pair<const char*, double*>& slot :
         std::initializer_list<std::pair<const char*, double*>>{
             {"hud_frame_height", &height_}, {"hud_shade_head_h", &head_h_},
             {"hud_shade_foot_y", &foot_y_}, {"hud_shade_vignette_x", &centre_x},
             {"hud_shade_vignette_y", &centre_y}, {"hud_shade_vignette_inner", &inner},
             {"hud_shade_vignette_outer", &outer}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    ::godot::Color vignette;
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_CLEAR", &clear_},
                                                                       {"OVERLAY_SCRIM_TOP", &scrim_top_},
                                                                       {"OVERLAY_SHADOW", &shadow_},
                                                                       {"OVERLAY_VIGNETTE", &vignette}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(outer > inner) || !(inner >= 0.0)) {
        failed = "hud_shade_vignette_outer must lie beyond hud_shade_vignette_inner";
        return false;
    }

    // One texel per frame unit: the ramp texture is the square of twice the outer radius around the centre.
    const double side = static_cast<double>(::ase::math::ceil(static_cast<float>(2.0 * outer)));
    const double texel = side / (2.0 * outer);

    ::godot::PackedFloat32Array offsets;
    offsets.push_back(static_cast<float>(inner / outer));
    offsets.push_back(static_cast<float>(outer / outer));
    ::godot::PackedColorArray colors;
    colors.push_back(clear_);
    colors.push_back(vignette);
    ::godot::Ref<::godot::Gradient> ramp;
    ramp.instantiate();
    ramp->set_offsets(offsets);
    ramp->set_colors(colors);

    vignette_.instantiate();
    vignette_->set_gradient(ramp);
    vignette_->set_fill(::godot::GradientTexture2D::FILL_RADIAL);
    vignette_->set_fill_from(::godot::Vector2(0.5f, 0.5f));
    vignette_->set_fill_to(::godot::Vector2(1.0f, 0.5f));
    vignette_->set_width(static_cast<int32_t>(side));
    vignette_->set_height(static_cast<int32_t>(side));

    ramp_at_ = ::godot::Vector2(static_cast<float>((outer - centre_x) * texel), static_cast<float>((outer - centre_y) * texel));
    texel_ = texel;
    bound_ = true;
    return true;
}

bool ScreenShade::draw(UiPen& canvas, const ::godot::Transform2D& frame, const HudCover& cover,
                       std::string& failed) const {
    if (!bound_) {
        failed = "ScreenShade drawn before bind";
        return false;
    }
    const ::godot::Rect2& surface = cover.surface;
    if (!surface.has_area()) {
        failed = "ScreenShade drawn without the surface (HudCover::surface)";
        return false;
    }
    const float left = surface.position.x;
    const float right = surface.position.x + surface.size.x;
    const float top = surface.position.y;
    const float bottom = surface.position.y + surface.size.y;
    // The head on the top band, the foot on the bottom band (the band zone and the hand zone of hud.zones).
    const float head_top = static_cast<float>(cover.head);
    const float head = static_cast<float>(head_h_ + cover.head);
    const float foot = static_cast<float>(foot_y_ + cover.hand);
    const float foot_end = static_cast<float>(height_ + cover.hand);
    const float texel = static_cast<float>(texel_);

    canvas.draw_set_transform_matrix(frame);
    canvas.draw_texture_rect_region(vignette_, surface,
                                    ::godot::Rect2(ramp_at_ + surface.position * texel, surface.size * texel));

    if (head_top > top) {
        canvas.draw_rect(::godot::Rect2(left, top, right - left, head_top - top), scrim_top_, true);
    }
    ::godot::PackedVector2Array head_points;
    head_points.push_back(::godot::Vector2(left, head_top));
    head_points.push_back(::godot::Vector2(right, head_top));
    head_points.push_back(::godot::Vector2(right, head));
    head_points.push_back(::godot::Vector2(left, head));
    ::godot::PackedColorArray head_colors;
    head_colors.push_back(scrim_top_);
    head_colors.push_back(scrim_top_);
    head_colors.push_back(clear_);
    head_colors.push_back(clear_);
    canvas.draw_polygon(head_points, head_colors);

    ::godot::PackedVector2Array foot_points;
    foot_points.push_back(::godot::Vector2(left, foot));
    foot_points.push_back(::godot::Vector2(right, foot));
    foot_points.push_back(::godot::Vector2(right, foot_end));
    foot_points.push_back(::godot::Vector2(left, foot_end));
    ::godot::PackedColorArray foot_colors;
    foot_colors.push_back(clear_);
    foot_colors.push_back(clear_);
    foot_colors.push_back(shadow_);
    foot_colors.push_back(shadow_);
    canvas.draw_polygon(foot_points, foot_colors);
    if (bottom > foot_end) {
        canvas.draw_rect(::godot::Rect2(left, foot_end, right - left, bottom - foot_end), shadow_, true);
    }

    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

}  // namespace ase::adp::godot

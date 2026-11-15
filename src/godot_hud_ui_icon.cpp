/**
 * ASE GODOT ADAPTER - HUD UI ICON
 *
 * @file        godot_hud_ui_icon.cpp
 * @brief       Binds an ICON_* of ui_icons.json to the Nerd Font and draws it centred as the mockup's G does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_icon.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <cstdint>

namespace ase::adp::godot {

int64_t hud_raster(const ::godot::Transform2D& device, double size) {
    const double pixel = static_cast<double>(ase::math::abs(device.get_scale().x));
    // The nearest whole screen pixel size: the glyph is drawn 1:1 at it, never scaled onto `size`.
    const int64_t raster = static_cast<int64_t>(ase::math::round(static_cast<float>(size * pixel)));
    return ase::math::max(raster, GODOT_TEXT_RASTER_MIN);
}

bool UiIcon::bind(const UiTheme& theme, const std::string& name, std::string& failed) {
    font_ = ::godot::Ref<::godot::FontFile>();
    glyph_ = ::godot::String();
    name_ = name;
    const ::godot::Ref<::godot::FontFile>& face = theme.faces().data;
    if (!read_icon(theme.icons(), ::godot::String::utf8(name.c_str()), face, glyph_, offset_css_)) {
        glyph_ = ::godot::String();
        failed = "icon." + name;
        return false;
    }
    font_ = face;
    web_px_ = theme.icon_web_px();
    // The face's own proportions: read at a size so large that the text server's whole-pixel rounding vanishes.
    const double design = static_cast<double>(GODOT_TEXT_DESIGN_PX);
    ascent_share_ = static_cast<double>(font_->get_ascent(GODOT_TEXT_DESIGN_PX)) / design;
    descent_share_ = static_cast<double>(font_->get_descent(GODOT_TEXT_DESIGN_PX)) / design;
    return true;
}

void UiIcon::draw(UiPen& canvas, const ::godot::Vector2& centre, double size, const ::godot::Color& color,
                  double opacity) const {
    if (!bound()) {
        return;
    }
    const int32_t font_size = static_cast<int32_t>(size);
    // text-anchor middle: the advance centred; dominant-baseline central: the middle of the face's designed ascent
    // and descent on the centre, so the baseline lies half that span below it, never from the rounded metrics.
    const float width = font_->get_string_size(glyph_, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
    const float baseline =
        centre.y + static_cast<float>((ascent_share_ - descent_share_) * static_cast<double>(font_size)) * GODOT_HALF;
    const float share = static_cast<float>(size / web_px_);
    ::godot::Color drawn = color;
    drawn.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    // The pen start on a whole pixel of the space drawn in, so the glyph's raster meets the screen pixels. The font
    // draws on the pen's canvas item under the transform the pen holds, at its size with oversampling 1: the pen's
    // own draw_string would multiply the size by the transform's scale and raster it a second time.
    const ::godot::RID item = canvas.get_canvas_item();
    font_->draw_string(item,
                       ::godot::Vector2(ase::math::round(centre.x - width * GODOT_HALF + offset_css_.x * share),
                                        ase::math::round(baseline + offset_css_.y * share)),
                       glyph_, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, canvas.tinted(drawn),
                       ::godot::TextServer::JUSTIFICATION_KASHIDA | ::godot::TextServer::JUSTIFICATION_WORD_BOUND,
                       ::godot::TextServer::DIRECTION_AUTO, ::godot::TextServer::ORIENTATION_HORIZONTAL,
                       GODOT_TEXT_OVERSAMPLING);
}

void UiIcon::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& centre,
                  double size, const ::godot::Color& color, double opacity) const {
    if (!bound() || size <= 0.0) {
        return;
    }
    // Rastered at the whole screen pixel size and drawn 1:1 with its pen start on a whole screen pixel: a scaled
    // raster glyph is resampled and blurs. The canvas takes screen⁻¹ first, so the stretch onto the window
    // (canvas_items) cancels and a raster pixel is a screen pixel. The transform moves by the whole pixels of
    // the centre and the draw takes the fraction, so the pen start is rounded once from the true centre: a
    // centre rounded first and a half advance rounded after it miss it by up to a pixel.
    const ::godot::Transform2D screen = canvas.screen();
    const ::godot::Transform2D device = screen * frame;
    const int64_t raster = hud_raster(device, size);
    const ::godot::Vector2 at = device.xform(centre);
    const ::godot::Vector2 whole(ase::math::round(at.x), ase::math::round(at.y));
    canvas.draw_set_transform_matrix(screen.affine_inverse() * ::godot::Transform2D(0.0f, whole));
    draw(canvas, at - whole, static_cast<double>(raster), color, opacity);
    canvas.draw_set_transform_matrix(frame);
}

double UiIcon::advance(double size) const {
    if (!bound()) {
        return 0.0;
    }
    return font_->get_string_size(glyph_, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, static_cast<int32_t>(size)).x;
}

}  // namespace ase::adp::godot

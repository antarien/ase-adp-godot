/**
 * ASE GODOT ADAPTER - HUD UI HEX CHIP
 *
 * @file        godot_hud_ui_hex_chip.cpp
 * @brief       Draws the glyph chip HX and the body of the thing chip HXI as the mockup does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_hex_chip.hpp.
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

#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** `color` with its alpha multiplied by `opacity`. */
::godot::Color faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

/** Reads the token `name` into `out`; false with `name` in `failed`. */
bool read_token(const UiTheme& theme, const char* name, double& out, std::string& failed) {
    if (!theme.measure(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

}  // namespace

::godot::PackedVector2Array hud_hex_points(const ::godot::Vector2& centre, double radius) {
    ::godot::PackedVector2Array points;
    const float step = ase::math::TWO_PI / static_cast<float>(GODOT_HEX_CORNERS);
    for (int32_t corner = 0; corner < GODOT_HEX_CORNERS; ++corner) {
        const float angle = step * static_cast<float>(corner);
        points.push_back(centre + ::godot::Vector2(ase::math::cos(angle), ase::math::sin(angle)) *
                                      static_cast<float>(radius));
    }
    return points;
}

bool UiHexChip::bind(const UiTheme& theme, std::string& failed) {
    if (!fade_.bind(theme, "hud_anim_fade", failed) || !label_.bind_label(theme, failed)) {
        return false;
    }
    if (!theme.color("OVERLAY_CHIP_OFF", off_)) {
        failed = "OVERLAY_CHIP_OFF";
        return false;
    }
    if (!theme.color("OVERLAY_ITEM_CHIP_OFF", thing_off_)) {
        failed = "OVERLAY_ITEM_CHIP_OFF";
        return false;
    }
    if (!read_token(theme, "hud_line_chip", line_, failed) ||
        !read_token(theme, "hud_line_chip_on", line_on_, failed) ||
        !read_token(theme, "hud_chip_on_fill_alpha", on_alpha_, failed) ||
        !read_token(theme, "hud_chip_glyph_scale", glyph_scale_, failed) ||
        !read_token(theme, "hud_chip_glyph_label_scale", glyph_label_scale_, failed) ||
        !read_token(theme, "hud_chip_glyph_label_lift", glyph_label_lift_, failed) ||
        !read_token(theme, "hud_chip_label_drop", label_drop_, failed) ||
        !read_token(theme, "hud_chip_label_size", label_size_, failed) ||
        !read_token(theme, "hud_chip_thing_on_fill_alpha", thing_on_alpha_, failed) ||
        !read_token(theme, "hud_chip_thing_scale", thing_scale_, failed) ||
        !read_token(theme, "hud_chip_thing_label_scale", thing_label_scale_, failed) ||
        !read_token(theme, "hud_chip_thing_label_lift", thing_label_lift_, failed) ||
        !read_token(theme, "hud_chip_thing_label_drop", thing_label_drop_, failed) ||
        !read_token(theme, "hud_chip_thing_label_size", thing_label_size_, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool UiHexChip::set_glyph(const std::string& icon, const std::string& label, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.hex_chip.unbound";
        return false;
    }
    if (icon != icon_.name() && !icon_.bind(*theme_, icon, failed)) {
        return false;
    }
    thing_ = false;
    labelled_ = !label.empty();
    return label_.set(label, failed);
}

bool UiHexChip::set_thing(const std::string& label, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.hex_chip.unbound";
        return false;
    }
    thing_ = true;
    labelled_ = !label.empty();
    return label_.set(label, failed);
}

double UiHexChip::shown(const UiHexChipPlace& place) const {
    return place.fade ? fade_.progress(place.elapsed, place.delay, place.still) : 1.0;
}

void UiHexChip::draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiHexChipPlace& place) {
    const double opacity = shown(place);
    if (theme_ == nullptr || opacity <= 0.0) {
        return;
    }
    canvas.draw_set_transform_matrix(frame);
    const ::godot::PackedVector2Array corners = hud_hex_points(place.centre, place.radius);
    // fill: the tone at the form's fill-opacity when on, the dark ground of the form when off.
    const ::godot::Color fill =
        place.on ? faded(place.tone, thing_ ? thing_on_alpha_ : on_alpha_) : (thing_ ? thing_off_ : off_);
    canvas.draw_colored_polygon(corners, faded(fill, opacity));
    ::godot::PackedVector2Array outline = corners;
    outline.push_back(corners[0]);
    canvas.draw_polyline(outline, faded(place.tone, opacity), static_cast<float>(place.on ? line_on_ : line_),
                         true);
    if (!thing_) {
        // G(x, y - lift, k, r * scale, col): the glyph centred, smaller and lifted over a label.
        const double size = place.radius * (labelled_ ? glyph_label_scale_ : glyph_scale_);
        const float lift = static_cast<float>(labelled_ ? glyph_label_lift_ : 0.0);
        icon_.draw(canvas, frame, place.centre - ::godot::Vector2(0.0f, lift), size, place.tone, opacity);
    }
    if (labelled_) {
        UiTextPlace label;
        label.at = place.centre +
                   ::godot::Vector2(0.0f, static_cast<float>(place.radius * (thing_ ? thing_label_drop_ : label_drop_)));
        label.size = thing_ ? thing_label_size_ : label_size_;
        label.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        label.color = place.tone;
        label.opacity = opacity;
        label.still = place.still;
        label.elapsed = place.elapsed;
        label_.draw(canvas, frame, label);
    }
}

UiHexChipPicture UiHexChip::picture(const UiHexChipPlace& place) const {
    UiHexChipPicture out;
    out.centre = place.centre - ::godot::Vector2(0.0f, static_cast<float>(labelled_ ? thing_label_lift_ : 0.0));
    out.box = place.radius * (labelled_ ? thing_label_scale_ : thing_scale_);
    out.opacity = shown(place);
    return out;
}

bool hud_hex_contains(const ::godot::Vector2& centre, double radius, const ::godot::Vector2& point) {
    // Inside a convex polygon with clockwise corners on the canvas: on the inner side of every edge.
    const ::godot::PackedVector2Array corners = hud_hex_points(centre, radius);
    for (int32_t corner = 0; corner < GODOT_HEX_CORNERS; ++corner) {
        const ::godot::Vector2 from = corners[corner];
        const ::godot::Vector2 to = corners[(corner + 1) % GODOT_HEX_CORNERS];
        if ((to - from).cross(point - from) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool UiHexChip::hit(const UiHexChipPlace& place, const ::godot::Vector2& point) const {
    return hud_hex_contains(place.centre, place.radius, point);
}

}  // namespace ase::adp::godot

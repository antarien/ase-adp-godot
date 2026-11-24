/**
 * ASE GODOT ADAPTER - HUD UI ITEM SPRITE
 *
 * @file        godot_hud_ui_item_sprite.cpp
 * @brief       Reads a thing's SVG paths from item_art.json and draws them fitted into a box as artFit does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_item_sprite.hpp. The path data is
 *              flattened by read_svg_path of godot_hud_ui_svg_path.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_stroke.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** Reads the token `name` into `out`; false with `name` in `failed`. */
bool read_token(const UiTheme& theme, const char* name, double& out, std::string& failed) {
    if (!theme.measure(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

/** The colour named under `key` of `from`, resolved through the theme; false when absent or unknown. */
bool read_named_color(const UiTheme& theme, const ::godot::Dictionary& from, const char* key, ::godot::Color& out) {
    ::godot::String name;
    if (!read_text(from, key, name)) {
        return false;
    }
    return theme.color(std::string(name.utf8().get_data()), out);
}

/** `color` with its alpha multiplied by `opacity`. */
::godot::Color faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

/** Reads the level of a vessel under `level` into `out`; false when a field of its kind is missing. */
bool read_level(const UiTheme& theme, const ::godot::Dictionary& level, UiItemSpriteLevel& out) {
    ::godot::String kind;
    if (!read_text(level, "kind", kind)) {
        return false;
    }
    out.line = kind == ::godot::String("line");
    out.body = kind == ::godot::String("body");
    const bool common = read_number(level, "bottom", out.bottom) && read_number(level, "span", out.span) &&
                        out.span > 0.0;
    if (out.line) {
        return common && read_number(level, "left", out.left) && read_number(level, "right", out.right) &&
               read_number(level, "sag", out.sag) && read_number(level, "w", out.width) &&
               read_named_color(theme, level, "stroke", out.color);
    }
    if (out.body) {
        return common && read_number(level, "top", out.top) && read_number(level, "top_left", out.top_left) &&
               read_number(level, "top_right", out.top_right) && read_number(level, "foot_left", out.foot_left) &&
               read_number(level, "foot_right", out.foot_right) && read_named_color(theme, level, "fill", out.color);
    }
    return false;
}

}  // namespace

bool UiItemSprite::bind(const UiTheme& theme, std::string& failed) {
    if (!read_token(theme, "hud_item_stroke_scale", stroke_scale_, failed) ||
        !read_token(theme, "hud_item_long_ratio", long_ratio_, failed) ||
        !read_token(theme, "hud_item_long_turn", long_turn_, failed) ||
        !read_token(theme, "hud_item_long_fill", long_fill_, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool UiItemSprite::set(const std::string& name, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.item_sprite.unbound";
        return false;
    }
    ::godot::Dictionary entry;
    if (!theme_->item_art(name, entry)) {
        failed = "item_art.items." + name;
        return false;
    }
    if (!read_number(entry, "vw", vw_) || !read_number(entry, "vh", vh_) || !(vw_ > 0.0) || !(vh_ > 0.0)) {
        failed = "item_art.items." + name + ".vw/vh";
        return false;
    }
    const ::godot::Variant art = entry.get("art", ::godot::Variant());
    if (art.get_type() != ::godot::Variant::ARRAY) {
        failed = "item_art.items." + name + ".art";
        return false;
    }
    const ::godot::Array list = art;
    paths_.clear();
    shadows_.clear();  // a kept silhouette is the mask of the drawing read before
    for (int64_t index = 0; index < list.size(); ++index) {
        const std::string where = "item_art.items." + name + ".art." + std::to_string(index);
        const ::godot::Variant item = list[index];
        if (item.get_type() != ::godot::Variant::DICTIONARY) {
            failed = where;
            return false;
        }
        const ::godot::Dictionary source = item;
        UiItemSpritePath path;
        ::godot::String data;
        if (!read_text(source, "d", data)) {
            failed = where + ".d";
            return false;
        }
        std::string reason;
        if (!read_svg_path(std::string(data.utf8().get_data()), path.outlines, reason)) {
            failed = where + ".d: " + reason;
            return false;
        }
        path.stroked = source.has("stroke");
        path.filled = source.has("fill");
        if ((path.stroked && !read_named_color(*theme_, source, "stroke", path.stroke)) ||
            (path.filled && !read_named_color(*theme_, source, "fill", path.fill)) ||
            (!path.stroked && !path.filled)) {
            failed = where + ".stroke/fill";
            return false;
        }
        if (!read_number(source, "w", path.width) || !(path.width > 0.0)) {
            failed = where + ".w";
            return false;
        }
        paths_.push_back(path);
    }

    has_level_ = entry.has("level");
    level_ = UiItemSpriteLevel();
    level_outline_ = UiSvgOutline();  // a new level is laid again at its first share
    level_share_ = 0.0;
    if (has_level_) {
        ::godot::Dictionary level;
        if (!read_section(entry, "level", level) || !read_level(*theme_, level, level_)) {
            failed = "item_art.items." + name + ".level";
            return false;
        }
    }
    return true;
}

void UiItemSprite::paint(UiPen& canvas, const ::godot::Transform2D& frame, const UiSvgOutline& outline,
                         const ::godot::Transform2D& local, const ::godot::Color* fill, const ::godot::Color* stroke,
                         double width, const UiItemSpritePlace& place) const {
    // The outline through one read pointer, every point through one write pointer (P0697): a push_back or an index
    // per point crosses the GDExtension border each time, the way UiPen::moved measured it (P0690).
    const int64_t count = outline.points.size();
    if (count == 0) {
        return;
    }
    const ::godot::Vector2* from = outline.points.ptr();
    ::godot::PackedVector2Array points;
    points.resize(count);
    ::godot::Vector2* to = points.ptrw();
    for (int64_t index = 0; index < count; ++index) {
        to[index] = local.xform(from[index]);
    }
    // A closed outline whose last point came back onto its first carries it once.
    int64_t kept = count;
    if (outline.closed && count > 1 &&
        to[count - 1].distance_to(to[0]) < static_cast<float>(GODOT_ITEM_POINT_EPSILON)) {
        kept = count - 1;
        points.resize(kept);
    }
    if (fill != nullptr && kept >= GODOT_ITEM_POLYGON_MIN) {
        ::godot::PackedColorArray colors;
        colors.push_back(faded(*fill, place.opacity));
        canvas.draw_polygon(points, colors);
    }
    if (stroke == nullptr) {
        return;
    }
    const ::godot::Color ink = faded(*stroke, place.opacity);
    const float drawn = static_cast<float>(width * stroke_scale_);
    if (!(drawn > 0.0f)) {
        return;
    }
    // stroke-linecap and stroke-linejoin round as ONE mesh (P0692): a polyline with a disc on every corner drew each
    // knee twice, and a translucent ink stood two layers deep there. A closed run goes back onto its first point.
    ui_stroke(canvas, frame, points, outline.closed && kept > 1, ink, drawn);
}

void UiItemSprite::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                        const UiItemSpritePlace& place) const {
    if (theme_ == nullptr || paths_.empty()) {
        return;
    }
    // artFit: a long thing turns in its box and fills it the more.
    const bool tall = vh_ / vw_ > long_ratio_;
    const bool wide = vw_ / vh_ > long_ratio_;
    const double turn = tall ? long_turn_ : (wide ? -long_turn_ : 0.0);
    const double k = place.box * (tall || wide ? long_fill_ : 1.0) / ase::math::max(vw_, vh_);
    // translate(x - vw k / 2, y - vh k / 2) rotate(turn about vw k / 2, vh k / 2) scale(k): the middle of the
    // drawn box stands on the centre, the turn goes about it.
    const float scale = static_cast<float>(k);
    const ::godot::Vector2 middle(static_cast<float>(vw_) * scale * GODOT_HALF,
                                  static_cast<float>(vh_) * scale * GODOT_HALF);
    const ::godot::Transform2D local =
        ::godot::Transform2D(static_cast<float>(turn) * ase::math::DEG_TO_RAD, place.centre) *
        ::godot::Transform2D(0.0f, -middle) *
        ::godot::Transform2D(0.0f, ::godot::Size2(scale, scale), 0.0f, ::godot::Vector2());
    draw_local(canvas, frame, local, place);
}

void UiItemSprite::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                               const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place) const {
    if (theme_ == nullptr || paths_.empty()) {
        return;
    }
    // artSvg without a turn: translate(x y) scale(k).
    const float scale = static_cast<float>(k);
    const ::godot::Transform2D local(0.0f, ::godot::Size2(scale, scale), 0.0f, corner);
    draw_local(canvas, frame, local, place);
}

void UiItemSprite::draw_local(UiPen& canvas, const ::godot::Transform2D& frame,
                              const ::godot::Transform2D& local, const UiItemSpritePlace& place) const {
    canvas.draw_set_transform_matrix(frame);
    if (place.silhouette) {
        // drop-shadow: the blurred mask as one quad, its pixels the frame's pixels.
        const ::godot::Size2 axes = frame.get_scale();
        const double scale = ase::math::max(static_cast<double>(::godot::Math::abs(axes.x)),
                                            static_cast<double>(::godot::Math::abs(axes.y)));
        const UiItemSpriteShadow& cast =
            shadows_.of(paths_, stroke_scale_, local, scale, place.spread, faded(place.shade, place.opacity));
        if (cast.texture.is_valid()) {
            canvas.draw_texture_rect_region(
                cast.texture, ::godot::Rect2(local.get_origin() + cast.corner, cast.size),
                ::godot::Rect2(0.0f, 0.0f, static_cast<float>(cast.width), static_cast<float>(cast.height)));
        }
        return;
    }
    for (const UiItemSpritePath& path : paths_) {
        for (const UiSvgOutline& outline : path.outlines) {
            paint(canvas, frame, outline, local, path.filled ? &path.fill : nullptr,
                  path.stroked ? &path.stroke : nullptr, path.width, place);
        }
    }
    const double share = ase::math::clamp(place.level, 0.0, 1.0);
    if (!has_level_ || !(share > 0.0)) {
        return;
    }
    // The outline in sprite units depends on the share alone, so it is laid again only when the level moves; the
    // frame, the place and the shade are applied by paint on every draw (P0692).
    if (share != level_share_) {
        const double y = level_.bottom - share * level_.span;
        UiSvgOutline level;
        if (level_.line) {
            // M left y Q middle (y + sag) right y, stroked.
            const ::godot::Vector2 from(static_cast<float>(level_.left), static_cast<float>(y));
            const ::godot::Vector2 control(static_cast<float>((level_.left + level_.right) * GODOT_HALF),
                                           static_cast<float>(y + level_.sag));
            const ::godot::Vector2 to(static_cast<float>(level_.right), static_cast<float>(y));
            svg_append(level, from);
            svg_append_quadratic(level, from, control, to);
        } else {
            // The walls run from the top corners to the foot corners; the sap fills between them below y.
            const double along = (y - level_.top) / level_.span;
            const double wall_left = level_.top_left + (level_.foot_left - level_.top_left) * along;
            const double wall_right = level_.top_right + (level_.foot_right - level_.top_right) * along;
            const float foot = static_cast<float>(level_.bottom);
            level.closed = true;
            svg_append(level, ::godot::Vector2(static_cast<float>(wall_left), static_cast<float>(y)));
            svg_append(level, ::godot::Vector2(static_cast<float>(wall_right), static_cast<float>(y)));
            svg_append(level, ::godot::Vector2(static_cast<float>(level_.foot_right), foot));
            svg_append(level, ::godot::Vector2(static_cast<float>(level_.foot_left), foot));
        }
        level_outline_ = level;
        level_share_ = share;
    }
    if (level_.line) {
        paint(canvas, frame, level_outline_, local, nullptr, &level_.color, level_.width, place);
        return;
    }
    paint(canvas, frame, level_outline_, local, &level_.color, nullptr, 0.0, place);
}

}  // namespace ase::adp::godot

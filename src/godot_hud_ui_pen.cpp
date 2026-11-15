/**
 * ASE GODOT ADAPTER - HUD UI PEN
 *
 * @file        godot_hud_ui_pen.cpp
 * @brief       The one drawer of the HUD: every shape laid in mockup units, rendered anew at the screen's pixels
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_pen.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-11
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_pen.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ase::adp::godot {

namespace {

// THE BROWSER FILLS EVERY PATH, GODOT ONLY ONE IT CAN CUT INTO TRIANGLES. A ring laid on the ground folds where the
// ground drops away behind it (the dig brush over a fresh hole), a far crumb shrinks to a sliver: Godot refuses both
// with "Invalid polygon data, triangulation failed" and draws nothing (run 20261113-023554-p0690-split, nine times
// after a spade cut), where the mockup's canvas fills the folded path. Such a polygon is merged into its simple pieces
// and each piece is drawn, a point of a piece taking the colour of the outline point nearest to it; a polygon with no
// area has no piece and covers nothing, as in the browser.
void lay_polygon(::godot::CanvasItem& canvas, const ::godot::PackedVector2Array& points,
                 const ::godot::PackedColorArray& colors) {
    ::godot::Geometry2D* geometry = ::godot::Geometry2D::get_singleton();
    if (points.size() < 3 || colors.is_empty()) {
        return;
    }
    if (!geometry->triangulate_polygon(points).is_empty()) {
        canvas.draw_polygon(points, colors);
        return;
    }
    const ::godot::TypedArray<::godot::PackedVector2Array> pieces =
        geometry->merge_polygons(points, ::godot::PackedVector2Array());
    for (int64_t index = 0; index < pieces.size(); ++index) {
        const ::godot::PackedVector2Array piece = pieces[index];
        if (piece.size() < 3 || geometry->triangulate_polygon(piece).is_empty()) {
            continue;
        }
        if (colors.size() != points.size()) {
            canvas.draw_polygon(piece, colors);
            continue;
        }
        ::godot::PackedColorArray shades;
        for (int64_t at = 0; at < piece.size(); ++at) {
            int64_t nearest = 0;
            for (int64_t other = 1; other < points.size(); ++other) {
                if (points[other].distance_squared_to(piece[at]) < points[nearest].distance_squared_to(piece[at])) {
                    nearest = other;
                }
            }
            shades.push_back(colors[nearest]);
        }
        canvas.draw_polygon(piece, shades);
    }
}

}  // namespace

UiPen::UiPen(::godot::CanvasItem& canvas) : canvas_(canvas) {
    device_ = canvas_.get_screen_transform();
    unscreen_ = device_.affine_inverse();
    scale_ = ase::math::sqrt(ase::math::abs(device_.columns[0].cross(device_.columns[1])));
    canvas_.draw_set_transform_matrix(unscreen_);
}

void UiPen::draw_set_transform_matrix(const ::godot::Transform2D& matrix) {
    matrix_ = matrix;
    device_ = canvas_.get_screen_transform() * matrix;
    const ::godot::Vector2 x = device_.columns[0];
    const ::godot::Vector2 y = device_.columns[1];
    const float x_length = x.length();
    const float y_length = y.length();
    uniform_ = ase::math::abs(x_length - y_length) <= GODOT_PEN_UNIFORM_EPS * ase::math::max(x_length, 1.0f) &&
               ase::math::abs(x.dot(y)) <= GODOT_PEN_UNIFORM_EPS * ase::math::max(x_length * y_length, 1.0f);
    upright_ = ase::math::abs(x.y) <= GODOT_PEN_UNIFORM_EPS && ase::math::abs(y.x) <= GODOT_PEN_UNIFORM_EPS;
    scale_ = uniform_ ? x_length : ase::math::sqrt(ase::math::abs(x.cross(y)));
    if (handed_) {
        canvas_.draw_set_transform_matrix(matrix_);
    }
}

void UiPen::plain() {
    if (handed_) {
        canvas_.draw_set_transform_matrix(unscreen_);
        handed_ = false;
    }
}

::godot::RID UiPen::get_canvas_item() {
    canvas_.draw_set_transform_matrix(matrix_);
    handed_ = true;
    return canvas_.get_canvas_item();
}

::godot::Transform2D UiPen::screen() const {
    return canvas_.get_screen_transform();
}

::godot::PackedVector2Array UiPen::moved(const ::godot::PackedVector2Array& points) const {
    // Every set() and every index of a packed array is a call over the GDExtension border; one pointer each way
    // reads and writes the whole run in place (P0690, as godot_hud_ui_item_shadow.cpp writes its bytes).
    ::godot::PackedVector2Array out;
    const int64_t count = points.size();
    out.resize(count);
    const ::godot::Vector2* from = points.ptr();
    ::godot::Vector2* to = out.ptrw();
    for (int64_t index = 0; index < count; ++index) {
        to[index] = device_.xform(from[index]);
    }
    return out;
}

float UiPen::pixels(float width) const {
    return width < 0.0f ? width : width * scale_;
}

float UiPen::core(float width) const {
    if (width < 0.0f) {
        return width;
    }
    // A core of at least 1 carries the whole feather (core + F covers `width`); below it Godot scales the feather
    // by the core, so core * (1 + F) covers it. Both meet at a core of 1.
    if (width >= 1.0f + GODOT_PEN_FEATHER_PX) {
        return width - GODOT_PEN_FEATHER_PX;
    }
    return width / (1.0f + GODOT_PEN_FEATHER_PX);
}

float UiPen::feather(float core) const {
    if (core < 0.0f) {
        return 0.0f;
    }
    return core >= 1.0f ? GODOT_PEN_FEATHER_PX : GODOT_PEN_FEATHER_PX * core;
}

::godot::PackedVector2Array UiPen::trimmed(const ::godot::PackedVector2Array& points, float core) const {
    const int64_t count = points.size();
    if (count < 2 || points[0].is_equal_approx(points[count - 1])) {
        return points;  // a closed run has no ends
    }
    // Godot lays the end feather past the last point; half of it is the coverage the browser's butt end lacks.
    const float cut = feather(core) * 0.5f;
    if (cut <= 0.0f) {
        return points;
    }
    ::godot::PackedVector2Array out = points;
    const float first = (points[1] - points[0]).length();
    const float last = (points[count - 1] - points[count - 2]).length();
    const float reach = count == 2 ? cut * 2.0f : cut;
    if (first > reach) {
        out.set(0, points[0] + (points[1] - points[0]) / first * cut);
    }
    if (last > reach) {
        out.set(count - 1, points[count - 1] - (points[count - 1] - points[count - 2]) / last * cut);
    }
    return out;
}

::godot::PackedVector2Array UiPen::round(const ::godot::Vector2& centre, float radius, float start, float end,
                                         int32_t least, bool closed) const {
    // The chord in screen pixels decides the division: the arc's length on the screen over GODOT_PEN_SEGMENT_PX.
    const double length = static_cast<double>(ase::math::abs(end - start)) * static_cast<double>(radius * scale_);
    const int32_t chords = static_cast<int32_t>(ase::math::ceil(static_cast<float>(length / GODOT_PEN_SEGMENT_PX)));
    const int32_t count = ase::math::max(ase::math::max(chords, least - 1), 1);
    ::godot::PackedVector2Array out;
    for (int32_t step = 0; step <= count; ++step) {
        if (closed && step == count) {
            break;
        }
        const float angle = start + (end - start) * static_cast<float>(step) / static_cast<float>(count);
        out.push_back(
            device_.xform(centre + ::godot::Vector2(ase::math::cos(angle), ase::math::sin(angle)) * radius));
    }
    return out;
}

void UiPen::fill_rimmed(const ::godot::PackedVector2Array& drawn, const ::godot::Color& shade) {
    ::godot::PackedColorArray colors;
    colors.push_back(shade);
    // The rim's core runs along the outline drawn in by half the core plus half the feather: the outer half of the
    // core and the feather's ramp then cover the band the nominal edge cuts in two, as the browser covers its edge.
    const float inset = (GODOT_PEN_RIM_PX + GODOT_PEN_FEATHER_PX) * 0.5f;
    const ::godot::TypedArray<::godot::PackedVector2Array> pieces =
        ::godot::Geometry2D::get_singleton()->offset_polygon(drawn, -inset, ::godot::Geometry2D::JOIN_MITER);
    if (pieces.is_empty()) {
        // Thinner than its rim: the shape draws as given, its area the nominal one.
        lay_polygon(canvas_, drawn, colors);
        return;
    }
    for (int64_t index = 0; index < pieces.size(); ++index) {
        const ::godot::PackedVector2Array piece = pieces[index];
        if (piece.size() < 3) {
            continue;
        }
        lay_polygon(canvas_, piece, colors);
        ::godot::PackedVector2Array rim = piece;
        rim.push_back(piece[0]);
        canvas_.draw_polyline(rim, shade, GODOT_PEN_RIM_PX, true);
    }
}

void UiPen::draw_line(const ::godot::Vector2& from, const ::godot::Vector2& to, const ::godot::Color& color,
                      float width, bool antialiased) {
    (void)antialiased;
    plain();
    const float drawn = core(pixels(width));
    ::godot::PackedVector2Array ends;
    ends.push_back(device_.xform(from));
    ends.push_back(device_.xform(to));
    ends = trimmed(ends, drawn);
    canvas_.draw_line(ends[0], ends[1], tinted(color), drawn, true);
}

void UiPen::draw_dashed_line(const ::godot::Vector2& from, const ::godot::Vector2& to, const ::godot::Color& color,
                             float width, float dash, bool aligned, bool antialiased) {
    (void)antialiased;
    plain();
    // Godot lays the dashes itself: each takes the core, its ends keep their feather.
    canvas_.draw_dashed_line(device_.xform(from), device_.xform(to), tinted(color), core(pixels(width)),
                             dash * scale_, aligned, true);
}

void UiPen::draw_polyline(const ::godot::PackedVector2Array& points, const ::godot::Color& color, float width,
                          bool antialiased) {
    (void)antialiased;
    plain();
    const float drawn = core(pixels(width));
    canvas_.draw_polyline(trimmed(moved(points), drawn), tinted(color), drawn, true);
}

void UiPen::draw_polygon(const ::godot::PackedVector2Array& points, const ::godot::PackedColorArray& colors) {
    plain();
    const ::godot::PackedVector2Array drawn = moved(points);
    ::godot::PackedColorArray shades;
    const int64_t count = colors.size();
    shades.resize(count);
    const ::godot::Color* from = colors.ptr();
    ::godot::Color* to = shades.ptrw();
    for (int64_t index = 0; index < count; ++index) {
        to[index] = tinted(from[index]);
    }
    // The browser smooths a filled path's edge; a polygon has none, so an opaque one of one colour gets a rim.
    if (shades.size() == 1 && shades[0].a >= 1.0f && drawn.size() > 2) {
        fill_rimmed(drawn, shades[0]);
        return;
    }
    lay_polygon(canvas_, drawn, shades);
}

void UiPen::draw_colored_polygon(const ::godot::PackedVector2Array& points, const ::godot::Color& color) {
    ::godot::PackedColorArray colors;
    colors.push_back(color);
    draw_polygon(points, colors);
}

void UiPen::draw_circle(const ::godot::Vector2& centre, float radius, const ::godot::Color& color, bool filled,
                        float width, bool antialiased) {
    (void)antialiased;
    plain();
    const ::godot::Color shade = tinted(color);
    if (uniform_) {
        const ::godot::Vector2 middle = device_.xform(centre);
        const float nominal = radius * scale_;
        if (!filled) {
            canvas_.draw_circle(middle, nominal, shade, false, core(pixels(width)), true);
            return;
        }
        // The feather's middle on the nominal radius: half the feather in from it. Below a diameter of 1 Godot
        // scales the feather by the radius; between both ranges the smallest whole-feather disc stands, its alpha
        // cut to the nominal disc's share of its covered area.
        const float half = GODOT_PEN_FEATHER_PX * 0.5f;
        float drawn = nominal - half;
        ::godot::Color dot = shade;
        if (nominal < 0.5f * (1.0f + half)) {
            drawn = nominal / (1.0f + half);
        } else if (drawn < 0.5f) {
            drawn = 0.5f;
            const float share = nominal / (drawn + half);
            dot.a = shade.a * share * share;
        }
        canvas_.draw_circle(middle, drawn, dot, true, -1.0f, true);
        return;
    }
    // An oval: the circle's points one by one through the transform.
    ::godot::PackedVector2Array points = round(centre, radius, 0.0f, ase::math::TWO_PI, 0, true);
    if (filled) {
        if (shade.a < 1.0f) {
            ::godot::PackedColorArray colors;
            colors.push_back(shade);
            lay_polygon(canvas_, points, colors);
            return;
        }
        fill_rimmed(points, shade);
        return;
    }
    points.push_back(points[0]);
    canvas_.draw_polyline(points, shade, core(pixels(width)), true);
}

void UiPen::draw_arc(const ::godot::Vector2& centre, float radius, float start_angle, float end_angle,
                     int32_t point_count, const ::godot::Color& color, float width, bool antialiased) {
    (void)antialiased;
    plain();
    const float drawn = core(pixels(width));
    canvas_.draw_polyline(trimmed(round(centre, radius, start_angle, end_angle, point_count, false), drawn),
                          tinted(color), drawn, true);
}

void UiPen::draw_rect(const ::godot::Rect2& rect, const ::godot::Color& color, bool filled, float width,
                      bool antialiased) {
    (void)antialiased;
    plain();
    if (uniform_ && upright_) {
        const ::godot::Vector2 corner = device_.xform(rect.position);
        const ::godot::Vector2 far = device_.xform(rect.position + rect.size);
        const ::godot::Rect2 drawn(corner, far - corner);
        canvas_.draw_rect(drawn.abs(), tinted(color), filled, filled ? -1.0f : core(pixels(width)), !filled);
        return;
    }
    ::godot::PackedVector2Array corners;
    corners.push_back(rect.position);
    corners.push_back(rect.position + ::godot::Vector2(rect.size.x, 0.0f));
    corners.push_back(rect.position + rect.size);
    corners.push_back(rect.position + ::godot::Vector2(0.0f, rect.size.y));
    if (filled) {
        draw_colored_polygon(corners, color);
        return;
    }
    corners.push_back(rect.position);
    draw_polyline(corners, color, width, true);
}

void UiPen::draw_texture_rect_region(const ::godot::Ref<::godot::Texture2D>& texture, const ::godot::Rect2& rect,
                                     const ::godot::Rect2& source) {
    plain();
    const ::godot::Vector2 corner = device_.xform(rect.position);
    const ::godot::Vector2 far = device_.xform(rect.position + rect.size);
    canvas_.draw_texture_rect_region(texture, ::godot::Rect2(corner, far - corner).abs(), source,
                                     tinted(::godot::Color(1.0f, 1.0f, 1.0f, 1.0f)));
}

void UiPen::draw_string(const ::godot::Ref<::godot::Font>& font, const ::godot::Vector2& at,
                        const ::godot::String& text, ::godot::HorizontalAlignment alignment, float width,
                        int32_t font_size, const ::godot::Color& color) {
    const int32_t size = static_cast<int32_t>(ase::math::round(static_cast<float>(font_size) * scale_));
    if (uniform_ && !upright_ && scale_ > 0.0f) {
        // A turned line: the canvas takes only the turn into screen pixels, its axes cut to length 1, and the font
        // rasters at the screen's pixels; the point is carried into those pixels so the turned frame lands where
        // device_ puts it.
        ::godot::Transform2D turn = device_;
        turn.columns[0] = device_.columns[0] / scale_;
        turn.columns[1] = device_.columns[1] / scale_;
        canvas_.draw_set_transform_matrix(unscreen_ * turn);
        handed_ = true;
        canvas_.draw_string(font, at * scale_, text, alignment, pixels(width), ase::math::max(size, 1),
                            tinted(color));
        return;
    }
    if (!uniform_) {
        // A stretched line has no single pixel size: the canvas takes the transform and the font draws under it.
        canvas_.draw_set_transform_matrix(matrix_);
        handed_ = true;
        canvas_.draw_string(font, at, text, alignment, width, font_size, tinted(color));
        return;
    }
    plain();
    canvas_.draw_string(font, device_.xform(at), text, alignment, pixels(width), ase::math::max(size, 1),
                        tinted(color));
}

void UiPen::set_tint(const ::godot::Color& tint) {
    tint_ = tint;
}

::godot::Color UiPen::tinted(const ::godot::Color& color) const {
    return ::godot::Color(color.r * tint_.r, color.g * tint_.g, color.b * tint_.b, color.a);
}

}  // namespace ase::adp::godot

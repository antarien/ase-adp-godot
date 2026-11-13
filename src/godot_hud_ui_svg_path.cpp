/**
 * ASE GODOT ADAPTER - HUD UI SVG PATH
 *
 * @file        godot_hud_ui_svg_path.cpp
 * @brief       SVG path data into flattened outlines
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_svg_path.hpp. Cut out of
 *              godot_hud_ui_item_sprite.cpp, whose drawing kept its own responsibility.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace ase::adp::godot {

namespace {

/** The reader of one SVG path: a cursor over the text. */
struct PathReader {
    const char* text = nullptr;
    size_t at = 0;
    size_t length = 0;
};

/** Steps over the separators; true when a character is left. */
bool skip_separators(PathReader& reader) {
    while (reader.at < reader.length && std::strchr(GODOT_ITEM_PATH_SEPARATORS, reader.text[reader.at]) != nullptr) {
        ++reader.at;
    }
    return reader.at < reader.length;
}

/** The next character is one of the path commands. */
bool at_command(const PathReader& reader) {
    return reader.at < reader.length && std::strchr(GODOT_ITEM_PATH_COMMANDS, reader.text[reader.at]) != nullptr;
}

/** Reads the next number; false when none stands there. */
bool read_value(PathReader& reader, float& out) {
    if (!skip_separators(reader) || at_command(reader)) {
        return false;
    }
    char* end = nullptr;
    out = std::strtof(reader.text + reader.at, &end);
    const size_t used = static_cast<size_t>(end - (reader.text + reader.at));
    if (used == 0) {
        return false;
    }
    reader.at += used;
    return true;
}

/** The signed angle from `u` to `v`, as SVG 1.1 implementation notes F.6.5.4 measure it. */
float vector_angle(const ::godot::Vector2& u, const ::godot::Vector2& v) {
    return ase::math::atan2(u.x * v.y - u.y * v.x, u.x * v.x + u.y * v.y);
}

/**
 * The elliptical arc from `from` to `to` (SVG A), flattened into `outline`: endpoint to centre form after
 * SVG 1.1 implementation notes F.6.5 and F.6.6, radii scaled up when they cannot reach.
 */
void append_arc(UiSvgOutline& outline, const ::godot::Vector2& from, float rx, float ry, float turn_deg, bool large,
                bool sweep, const ::godot::Vector2& to) {
    rx = ase::math::abs(rx);
    ry = ase::math::abs(ry);
    const float epsilon = static_cast<float>(GODOT_ITEM_POINT_EPSILON);
    if (from.distance_to(to) < epsilon) {
        return;
    }
    if (rx < epsilon || ry < epsilon) {
        svg_append(outline, to);
        return;
    }
    const float phi = turn_deg * ase::math::DEG_TO_RAD;
    const float cos_phi = ase::math::cos(phi);
    const float sin_phi = ase::math::sin(phi);
    const ::godot::Vector2 half = (from - to) * GODOT_HALF;
    const float x1 = cos_phi * half.x + sin_phi * half.y;
    const float y1 = -sin_phi * half.x + cos_phi * half.y;
    const float reach = (x1 * x1) / (rx * rx) + (y1 * y1) / (ry * ry);
    if (reach > 1.0f) {
        const float grow = ase::math::sqrt(reach);
        rx *= grow;
        ry *= grow;
    }
    const float numerator = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1;
    const float denominator = rx * rx * y1 * y1 + ry * ry * x1 * x1;
    const float share = denominator > 0.0f && numerator > 0.0f ? ase::math::sqrt(numerator / denominator) : 0.0f;
    const float sign = large == sweep ? -1.0f : 1.0f;
    const float cx1 = sign * share * rx * y1 / ry;
    const float cy1 = -sign * share * ry * x1 / rx;
    const ::godot::Vector2 mid = (from + to) * GODOT_HALF;
    const float cx = cos_phi * cx1 - sin_phi * cy1 + mid.x;
    const float cy = sin_phi * cx1 + cos_phi * cy1 + mid.y;
    const ::godot::Vector2 start_dir((x1 - cx1) / rx, (y1 - cy1) / ry);
    const ::godot::Vector2 end_dir((-x1 - cx1) / rx, (-y1 - cy1) / ry);
    const float start = vector_angle(::godot::Vector2(1.0f, 0.0f), start_dir);
    float sweep_angle = vector_angle(start_dir, end_dir);
    if (!sweep && sweep_angle > 0.0f) {
        sweep_angle -= ase::math::TWO_PI;
    } else if (sweep && sweep_angle < 0.0f) {
        sweep_angle += ase::math::TWO_PI;
    }
    for (int32_t step = 1; step <= GODOT_ITEM_CURVE_POINTS; ++step) {
        const float angle = start + sweep_angle * static_cast<float>(step) / static_cast<float>(GODOT_ITEM_CURVE_POINTS);
        const float ex = rx * ase::math::cos(angle);
        const float ey = ry * ase::math::sin(angle);
        svg_append(outline, ::godot::Vector2(cx + cos_phi * ex - sin_phi * ey, cy + sin_phi * ex + cos_phi * ey));
    }
}

/** Ends the outline being drawn: kept when it holds a point, then a fresh one begins. */
void finish(std::vector<UiSvgOutline>& outlines, UiSvgOutline& outline) {
    if (outline.points.size() > 0) {
        outlines.push_back(outline);
    }
    outline = UiSvgOutline();
}

/** Reads `count` numbers into `values`; false when one is missing. */
bool read_values(PathReader& reader, float* values, int32_t count) {
    for (int32_t index = 0; index < count; ++index) {
        if (!read_value(reader, values[index])) {
            return false;
        }
    }
    return true;
}

}  // namespace

void svg_append(UiSvgOutline& outline, const ::godot::Vector2& point) {
    const int64_t count = outline.points.size();
    if (count > 0 && outline.points[count - 1].distance_to(point) < static_cast<float>(GODOT_ITEM_POINT_EPSILON)) {
        return;
    }
    outline.points.push_back(point);
}

void svg_append_quadratic(UiSvgOutline& outline, const ::godot::Vector2& from, const ::godot::Vector2& control,
                          const ::godot::Vector2& to) {
    for (int32_t step = 1; step <= GODOT_ITEM_CURVE_POINTS; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(GODOT_ITEM_CURVE_POINTS);
        const float u = 1.0f - t;
        svg_append(outline, from * (u * u) + control * (2.0f * u * t) + to * (t * t));
    }
}

void svg_append_cubic(UiSvgOutline& outline, const ::godot::Vector2& from, const ::godot::Vector2& first,
                      const ::godot::Vector2& second, const ::godot::Vector2& to) {
    for (int32_t step = 1; step <= GODOT_ITEM_CURVE_POINTS; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(GODOT_ITEM_CURVE_POINTS);
        const float u = 1.0f - t;
        svg_append(outline, from * (u * u * u) + first * (3.0f * u * u * t) + second * (3.0f * u * t * t) +
                                to * (t * t * t));
    }
}

void svg_append_rounded_rect(UiSvgOutline& outline, const ::godot::Rect2& rect, float radius) {
    // SVG rect x y width height rx: rx is cut to half of each side, the corners are quarter arcs.
    const float r = ase::math::min(ase::math::max(radius, 0.0f),
                                   ase::math::min(rect.size.x, rect.size.y) * GODOT_HALF);
    const float left = rect.position.x;
    const float top = rect.position.y;
    const float right = left + rect.size.x;
    const float bottom = top + rect.size.y;
    svg_append(outline, ::godot::Vector2(left + r, top));
    svg_append(outline, ::godot::Vector2(right - r, top));
    append_arc(outline, ::godot::Vector2(right - r, top), r, r, 0.0f, false, true, ::godot::Vector2(right, top + r));
    svg_append(outline, ::godot::Vector2(right, bottom - r));
    append_arc(outline, ::godot::Vector2(right, bottom - r), r, r, 0.0f, false, true,
               ::godot::Vector2(right - r, bottom));
    svg_append(outline, ::godot::Vector2(left + r, bottom));
    append_arc(outline, ::godot::Vector2(left + r, bottom), r, r, 0.0f, false, true,
               ::godot::Vector2(left, bottom - r));
    svg_append(outline, ::godot::Vector2(left, top + r));
    append_arc(outline, ::godot::Vector2(left, top + r), r, r, 0.0f, false, true, ::godot::Vector2(left + r, top));
    outline.closed = true;
}

bool read_svg_path(const std::string& data, std::vector<UiSvgOutline>& outlines, std::string& failed) {
    PathReader reader;
    reader.text = data.c_str();
    reader.length = data.size();
    outlines.clear();
    UiSvgOutline outline;
    ::godot::Vector2 cursor;
    ::godot::Vector2 start;
    char command = 0;
    while (skip_separators(reader)) {
        if (at_command(reader)) {
            command = reader.text[reader.at];
            ++reader.at;
        } else if (command == 0) {
            failed = "the path data begins without a command";
            return false;
        }
        const bool relative = command >= 'a' && command <= 'z';
        const ::godot::Vector2 base = relative ? cursor : ::godot::Vector2();
        const char upper = static_cast<char>(relative ? command - ('a' - 'A') : command);
        if (upper == 'Z') {
            outline.closed = true;
            finish(outlines, outline);
            cursor = start;
            command = 0;
            continue;
        }
        float values[7] = {};
        if (upper == 'M') {
            if (!read_values(reader, values, 2)) {
                failed = "M without its point";
                return false;
            }
            finish(outlines, outline);
            cursor = base + ::godot::Vector2(values[0], values[1]);
            start = cursor;
            svg_append(outline, cursor);
            command = relative ? 'l' : 'L';
            continue;
        }
        if (outline.points.size() == 0) {
            // A command after Z draws on from the start of the closed outline.
            svg_append(outline, cursor);
        }
        if (upper == 'L') {
            if (!read_values(reader, values, 2)) {
                failed = "L without its point";
                return false;
            }
            cursor = base + ::godot::Vector2(values[0], values[1]);
            svg_append(outline, cursor);
        } else if (upper == 'H') {
            if (!read_values(reader, values, 1)) {
                failed = "H without its x";
                return false;
            }
            cursor.x = relative ? cursor.x + values[0] : values[0];
            svg_append(outline, cursor);
        } else if (upper == 'V') {
            if (!read_values(reader, values, 1)) {
                failed = "V without its y";
                return false;
            }
            cursor.y = relative ? cursor.y + values[0] : values[0];
            svg_append(outline, cursor);
        } else if (upper == 'Q') {
            if (!read_values(reader, values, 4)) {
                failed = "Q without its control and end point";
                return false;
            }
            const ::godot::Vector2 end = base + ::godot::Vector2(values[2], values[3]);
            svg_append_quadratic(outline, cursor, base + ::godot::Vector2(values[0], values[1]), end);
            cursor = end;
        } else if (upper == 'C') {
            if (!read_values(reader, values, 6)) {
                failed = "C without its two controls and end point";
                return false;
            }
            const ::godot::Vector2 end = base + ::godot::Vector2(values[4], values[5]);
            svg_append_cubic(outline, cursor, base + ::godot::Vector2(values[0], values[1]),
                             base + ::godot::Vector2(values[2], values[3]), end);
            cursor = end;
        } else if (upper == 'A') {
            if (!read_values(reader, values, 7)) {
                failed = "A without its radii, turn, flags and end point";
                return false;
            }
            const ::godot::Vector2 end = base + ::godot::Vector2(values[5], values[6]);
            append_arc(outline, cursor, values[0], values[1], values[2], values[3] != 0.0f, values[4] != 0.0f, end);
            cursor = end;
        } else {
            failed = std::string("the path command ") + command + " is not one of " + GODOT_ITEM_PATH_COMMANDS;
            return false;
        }
    }
    finish(outlines, outline);
    return true;
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD UI STROKE
 *
 * @file        godot_hud_ui_stroke.cpp
 * @brief       A polyline stroked as the browser strokes a path with stroke-linejoin round: one mesh in screen pixels
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_stroke.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    B0678
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_stroke.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_cover.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

// The left normal of a direction: the direction turned by a quarter, so a turn from da to db with a positive cross
// product turns toward it.
::godot::Vector2 normal_of(const ::godot::Vector2& direction) {
    return ::godot::Vector2(-direction.y, direction.x);
}

int32_t lay(::godot::PackedVector2Array& mesh, ::godot::PackedColorArray& colors, const ::godot::Vector2& point,
            const ::godot::Color& color) {
    mesh.push_back(point);
    colors.push_back(color);
    return static_cast<int32_t>(mesh.size() - 1);
}

void triangle(::godot::PackedInt32Array& indices, int32_t a, int32_t b, int32_t c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
}

// The feather along an edge from core point a to core point b, out to the feather points fb and fa.
void band(::godot::PackedInt32Array& indices, int32_t a, int32_t b, int32_t fb, int32_t fa) {
    triangle(indices, a, b, fb);
    triangle(indices, a, fb, fa);
}

// The round piece around `centre` from the core point `core_from` (feather `feather_from`) turning by `sweep` radians
// onto `core_to` (feather `feather_to`): a fan from `hub`, divided by its length at the feather's end as the pen
// divides an arc, with the feather band along its rim.
void fan(::godot::PackedVector2Array& mesh, ::godot::PackedColorArray& colors, ::godot::PackedInt32Array& indices,
         int32_t hub, const ::godot::Vector2& centre, int32_t core_from, int32_t feather_from, int32_t core_to,
         int32_t feather_to, float from, float sweep, float half, float reach, const ::godot::Color& shade,
         const ::godot::Color& clear) {
    const float length = ase::math::abs(sweep) * reach;
    const int32_t steps = ase::math::max(
        1, static_cast<int32_t>(ase::math::ceil(length / static_cast<float>(GODOT_PEN_SEGMENT_PX))));
    int32_t core = core_from;
    int32_t feather = feather_from;
    for (int32_t step = 1; step <= steps; ++step) {
        int32_t next_core = core_to;
        int32_t next_feather = feather_to;
        if (step < steps) {
            const float angle = from + sweep * static_cast<float>(step) / static_cast<float>(steps);
            const ::godot::Vector2 toward(ase::math::cos(angle), ase::math::sin(angle));
            next_core = lay(mesh, colors, centre + toward * half, shade);
            next_feather = lay(mesh, colors, centre + toward * reach, clear);
        }
        triangle(indices, hub, core, next_core);
        band(indices, core, next_core, next_feather, feather);
        core = next_core;
        feather = next_feather;
    }
}

// The angle of a direction, for the start of a fan.
float angle_of(const ::godot::Vector2& direction) {
    return ase::math::atan2(direction.y, direction.x);
}

// The knee at `knee` between the segment along `incoming` (length `incoming_length`) and the one along `outgoing`
// (length `outgoing_length`): the outer side round around the knee, the inner sides meeting in one point when that
// point lies within GODOT_STROKE_INNER_SHARE of both segments. Hands back the end corners of the incoming segment and
// the start corners of the outgoing one, each core and feather on the left and on the right.
void knee(::godot::PackedVector2Array& mesh, ::godot::PackedColorArray& colors, ::godot::PackedInt32Array& indices,
          const ::godot::Vector2& point, const ::godot::Vector2& incoming, float incoming_length,
          const ::godot::Vector2& outgoing, float outgoing_length, float half, float reach,
          const ::godot::Color& shade, const ::godot::Color& clear, int32_t& end_left, int32_t& end_left_feather,
          int32_t& end_right, int32_t& end_right_feather, int32_t& start_left, int32_t& start_left_feather,
          int32_t& start_right, int32_t& start_right_feather) {
    const ::godot::Vector2 normal_in = normal_of(incoming);
    const ::godot::Vector2 normal_out = normal_of(outgoing);
    const float cross = incoming.cross(outgoing);
    const float dot = incoming.dot(outgoing);
    // The inner side is the one the stroke turns toward: the left normal for a positive cross product.
    const float side = cross < 0.0f ? -1.0f : 1.0f;
    const float sweep = ase::math::atan2(cross, dot);
    const float opening = 1.0f + dot;
    const float tangent = opening > GODOT_STROKE_JOIN_EPS ? ase::math::abs(cross) / opening : 0.0f;
    const bool meet = opening > GODOT_STROKE_JOIN_EPS &&
                      reach * tangent <= GODOT_STROKE_INNER_SHARE * ase::math::min(incoming_length, outgoing_length);

    const int32_t outer_in = lay(mesh, colors, point - normal_in * (side * half), shade);
    const int32_t outer_in_feather = lay(mesh, colors, point - normal_in * (side * reach), clear);
    const int32_t outer_out = lay(mesh, colors, point - normal_out * (side * half), shade);
    const int32_t outer_out_feather = lay(mesh, colors, point - normal_out * (side * reach), clear);

    int32_t inner_in = 0;
    int32_t inner_in_feather = 0;
    int32_t inner_out = 0;
    int32_t inner_out_feather = 0;
    int32_t hub = 0;
    if (meet) {
        // Both inner offset lines cross a tangent of the half turn back along the incoming segment.
        inner_in = lay(mesh, colors, point + normal_in * (side * half) - incoming * (half * tangent), shade);
        inner_in_feather = lay(mesh, colors, point + normal_in * (side * reach) - incoming * (reach * tangent), clear);
        inner_out = inner_in;
        inner_out_feather = inner_in_feather;
        hub = inner_in;
    } else {
        inner_in = lay(mesh, colors, point + normal_in * (side * half), shade);
        inner_in_feather = lay(mesh, colors, point + normal_in * (side * reach), clear);
        inner_out = lay(mesh, colors, point + normal_out * (side * half), shade);
        inner_out_feather = lay(mesh, colors, point + normal_out * (side * reach), clear);
        hub = lay(mesh, colors, point, shade);
    }
    fan(mesh, colors, indices, hub, point, outer_in, outer_in_feather, outer_out, outer_out_feather,
        angle_of(normal_in * -side), sweep, half, reach, shade, clear);

    if (side > 0.0f) {
        end_left = inner_in;
        end_left_feather = inner_in_feather;
        end_right = outer_in;
        end_right_feather = outer_in_feather;
        start_left = inner_out;
        start_left_feather = inner_out_feather;
        start_right = outer_out;
        start_right_feather = outer_out_feather;
        return;
    }
    end_left = outer_in;
    end_left_feather = outer_in_feather;
    end_right = inner_in;
    end_right_feather = inner_in_feather;
    start_left = outer_out;
    start_left_feather = outer_out_feather;
    start_right = inner_out;
    start_right_feather = inner_out_feather;
}

// The body of one segment between its start and end corners, with the feather along both sides.
void segment(::godot::PackedInt32Array& indices, int32_t start_left, int32_t start_left_feather, int32_t start_right,
             int32_t start_right_feather, int32_t end_left, int32_t end_left_feather, int32_t end_right,
             int32_t end_right_feather) {
    triangle(indices, start_left, end_left, end_right);
    triangle(indices, start_left, end_right, start_right);
    band(indices, start_left, end_left, end_left_feather, start_left_feather);
    band(indices, start_right, end_right, end_right_feather, start_right_feather);
}

}  // namespace

void ui_stroke(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::PackedVector2Array& points,
               bool closed, const ::godot::Color& color, double width) {
    if (points.is_empty()) {
        return;
    }
    if (width < 0.0) {
        canvas.draw_set_transform_matrix(frame);
        ::godot::PackedVector2Array run = points;
        if (closed) {
            run.push_back(points[0]);
        }
        canvas.draw_polyline(run, color, static_cast<float>(width));
        return;
    }
    const ::godot::Transform2D screen = canvas.screen();
    const ::godot::Transform2D device = screen * frame;
    const float scale = ase::math::sqrt(ase::math::abs(device.columns[0].cross(device.columns[1])));
    const float core = pen_core(static_cast<float>(width) * scale);
    if (!(core > 0.0f)) {
        return;
    }
    const float half = core * GODOT_HALF;
    const float reach = half + pen_feather(core);

    // The run in screen pixels, a point on the one before it left out; a closed run that ends on its first point
    // ends before it.
    ::godot::PackedVector2Array run;
    for (int64_t index = 0; index < points.size(); ++index) {
        const ::godot::Vector2 placed = device.xform(points[index]);
        if (run.is_empty() || run[run.size() - 1].distance_to(placed) > GODOT_STROKE_POINT_EPS_PX) {
            run.push_back(placed);
        }
    }
    while (closed && run.size() > 1 && run[run.size() - 1].distance_to(run[0]) <= GODOT_STROKE_POINT_EPS_PX) {
        run.remove_at(run.size() - 1);
    }
    const int64_t count = run.size();
    const bool ring = closed && count >= 3;

    const ::godot::Color shade = canvas.tinted(color);
    ::godot::Color clear = shade;
    clear.a = 0.0f;
    ::godot::PackedVector2Array mesh;
    ::godot::PackedColorArray colors;
    ::godot::PackedInt32Array indices;

    if (count == 1) {
        // A stroke of no length: the disc of its width, as stroke-linecap round draws it.
        const ::godot::Vector2 centre = run[0];
        const int32_t hub = lay(mesh, colors, centre, shade);
        const int32_t rim = lay(mesh, colors, centre + ::godot::Vector2(half, 0.0f), shade);
        const int32_t rim_feather = lay(mesh, colors, centre + ::godot::Vector2(reach, 0.0f), clear);
        fan(mesh, colors, indices, hub, centre, rim, rim_feather, rim, rim_feather, 0.0f, ase::math::TWO_PI, half,
            reach, shade, clear);
    } else {
        const int64_t segments = ring ? count : count - 1;
        ::godot::PackedVector2Array directions;
        ::godot::PackedFloat32Array lengths;
        for (int64_t index = 0; index < segments; ++index) {
            const ::godot::Vector2 span = run[(index + 1) % count] - run[index];
            lengths.push_back(span.length());
            directions.push_back(span / span.length());
        }

        int32_t start_left = 0;
        int32_t start_left_feather = 0;
        int32_t start_right = 0;
        int32_t start_right_feather = 0;
        int32_t last_left = 0;
        int32_t last_left_feather = 0;
        int32_t last_right = 0;
        int32_t last_right_feather = 0;
        if (ring) {
            // The knee on the first point joins the last segment onto the first.
            knee(mesh, colors, indices, run[0], directions[segments - 1], lengths[segments - 1], directions[0],
                 lengths[0], half, reach, shade, clear, last_left, last_left_feather, last_right, last_right_feather,
                 start_left, start_left_feather, start_right, start_right_feather);
        } else {
            // stroke-linecap round at the start: a half disc from the left side back around to the right.
            const ::godot::Vector2 left = normal_of(directions[0]);
            const int32_t hub = lay(mesh, colors, run[0], shade);
            start_left = lay(mesh, colors, run[0] + left * half, shade);
            start_left_feather = lay(mesh, colors, run[0] + left * reach, clear);
            start_right = lay(mesh, colors, run[0] - left * half, shade);
            start_right_feather = lay(mesh, colors, run[0] - left * reach, clear);
            fan(mesh, colors, indices, hub, run[0], start_left, start_left_feather, start_right, start_right_feather,
                angle_of(left), ase::math::PI, half, reach, shade, clear);
        }

        for (int64_t index = 0; index < segments; ++index) {
            int32_t end_left = 0;
            int32_t end_left_feather = 0;
            int32_t end_right = 0;
            int32_t end_right_feather = 0;
            int32_t next_left = 0;
            int32_t next_left_feather = 0;
            int32_t next_right = 0;
            int32_t next_right_feather = 0;
            if (index + 1 < segments) {
                knee(mesh, colors, indices, run[index + 1], directions[index], lengths[index], directions[index + 1],
                     lengths[index + 1], half, reach, shade, clear, end_left, end_left_feather, end_right,
                     end_right_feather, next_left, next_left_feather, next_right, next_right_feather);
            } else if (ring) {
                end_left = last_left;
                end_left_feather = last_left_feather;
                end_right = last_right;
                end_right_feather = last_right_feather;
            } else {
                // stroke-linecap round at the end: a half disc from the right side forward around to the left.
                const ::godot::Vector2 end = run[count - 1];
                const ::godot::Vector2 left = normal_of(directions[index]);
                const int32_t hub = lay(mesh, colors, end, shade);
                end_left = lay(mesh, colors, end + left * half, shade);
                end_left_feather = lay(mesh, colors, end + left * reach, clear);
                end_right = lay(mesh, colors, end - left * half, shade);
                end_right_feather = lay(mesh, colors, end - left * reach, clear);
                fan(mesh, colors, indices, hub, end, end_right, end_right_feather, end_left, end_left_feather,
                    angle_of(-left), ase::math::PI, half, reach, shade, clear);
            }
            segment(indices, start_left, start_left_feather, start_right, start_right_feather, end_left,
                    end_left_feather, end_right, end_right_feather);
            start_left = next_left;
            start_left_feather = next_left_feather;
            start_right = next_right;
            start_right_feather = next_right_feather;
        }
    }

    // The mesh lies in screen pixels: the canvas under the inverse of screen(), then the caller's frame again.
    canvas.draw_set_transform_matrix(screen.affine_inverse());
    const ::godot::RID item = canvas.get_canvas_item();
    ::godot::RenderingServer::get_singleton()->canvas_item_add_triangle_array(item, indices, mesh, colors);
    canvas.draw_set_transform_matrix(frame);
}

}  // namespace ase::adp::godot

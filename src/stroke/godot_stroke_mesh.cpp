/**
 * ASE GODOT ADAPTER - THE MESH OF A ROUND-JOINED STROKE WITHOUT GODOT
 *
 * @file        godot_stroke_mesh.cpp
 * @brief       stroke_run, stroke_bound, stroke_lay - the pieces of ui_stroke's mesh in screen pixels, as plain numbers
 * @description See godot_stroke_mesh.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    render/pipeline/stage
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    T0357
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/stroke/godot_stroke_mesh.hpp>
#include <ase/adp/godot/stroke/types.hpp>
#include <ase/adp/godot/pen/types.hpp>

#include <ase/math/math.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

UiStrokePoint plus(const UiStrokePoint& a, const UiStrokePoint& b) {
    return UiStrokePoint{a.x + b.x, a.y + b.y};
}

UiStrokePoint minus(const UiStrokePoint& a, const UiStrokePoint& b) {
    return UiStrokePoint{a.x - b.x, a.y - b.y};
}

UiStrokePoint times(const UiStrokePoint& a, float factor) {
    return UiStrokePoint{a.x * factor, a.y * factor};
}

float cross(const UiStrokePoint& a, const UiStrokePoint& b) {
    return a.x * b.y - a.y * b.x;
}

float dot(const UiStrokePoint& a, const UiStrokePoint& b) {
    return a.x * b.x + a.y * b.y;
}

float length_of(const UiStrokePoint& a) {
    return ase::math::sqrt(dot(a, a));
}

// The left normal of a direction: the direction turned by a quarter, so a turn from da to db with a positive cross
// product turns toward it.
UiStrokePoint normal_of(const UiStrokePoint& direction) {
    return UiStrokePoint{-direction.y, direction.x};
}

// The angle of a direction, for the start of a fan.
float angle_of(const UiStrokePoint& direction) {
    return ase::math::atan2(direction.y, direction.x);
}

// One corner of the mesh with its share of the ink's alpha, written through the mesh's pointers.
int32_t lay(UiStrokeMesh& mesh, const UiStrokePoint& point, float opacity) {
    mesh.corners[mesh.laid] = point;
    mesh.opacity[mesh.laid] = opacity;
    return mesh.laid++;
}

void triangle(UiStrokeMesh& mesh, int32_t a, int32_t b, int32_t c) {
    mesh.indices[mesh.filled] = a;
    mesh.indices[mesh.filled + 1] = b;
    mesh.indices[mesh.filled + 2] = c;
    mesh.filled += 3;
}

// The feather along an edge from core point a to core point b, out to the feather points fb and fa.
void band(UiStrokeMesh& mesh, int32_t a, int32_t b, int32_t fb, int32_t fa) {
    triangle(mesh, a, b, fb);
    triangle(mesh, a, fb, fa);
}

// The steps a fan turning by `sweep` radians divides into, by its length at the feather's end `reach`, as the pen
// divides an arc.
int32_t fan_steps(float sweep, float reach) {
    const float length = ase::math::abs(sweep) * reach;
    return ase::math::max(1,
                          static_cast<int32_t>(ase::math::ceil(length / static_cast<float>(GODOT_PEN_SEGMENT_PX))));
}

// The round piece around `centre` from the core point `core_from` (feather `feather_from`) turning by `sweep` radians
// onto `core_to` (feather `feather_to`): a fan from `hub`, divided by fan_steps, with the feather band along its rim.
void fan(UiStrokeMesh& mesh, int32_t hub, const UiStrokePoint& centre, int32_t core_from, int32_t feather_from,
         int32_t core_to, int32_t feather_to, float from, float sweep, float half, float reach) {
    const int32_t steps = fan_steps(sweep, reach);
    int32_t core = core_from;
    int32_t feather = feather_from;
    for (int32_t step = 1; step <= steps; ++step) {
        int32_t next_core = core_to;
        int32_t next_feather = feather_to;
        if (step < steps) {
            const float angle = from + sweep * static_cast<float>(step) / static_cast<float>(steps);
            const UiStrokePoint toward{ase::math::cos(angle), ase::math::sin(angle)};
            next_core = lay(mesh, plus(centre, times(toward, half)), GODOT_STROKE_CORE_OPACITY);
            next_feather = lay(mesh, plus(centre, times(toward, reach)), GODOT_STROKE_FEATHER_OPACITY);
        }
        triangle(mesh, hub, core, next_core);
        band(mesh, core, next_core, next_feather, feather);
        core = next_core;
        feather = next_feather;
    }
}

// The knee at `point` between the segment along `incoming` (length `incoming_length`) and the one along `outgoing`
// (length `outgoing_length`): the outer side round around the knee, the inner sides meeting in one point when that
// point lies within GODOT_STROKE_INNER_SHARE of both segments. Hands back the end corners of the incoming segment and
// the start corners of the outgoing one, each core and feather on the left and on the right.
void knee(UiStrokeMesh& mesh, const UiStrokePoint& point, const UiStrokePoint& incoming, float incoming_length,
          const UiStrokePoint& outgoing, float outgoing_length, float half, float reach, int32_t& end_left,
          int32_t& end_left_feather, int32_t& end_right, int32_t& end_right_feather, int32_t& start_left,
          int32_t& start_left_feather, int32_t& start_right, int32_t& start_right_feather) {
    const UiStrokePoint normal_in = normal_of(incoming);
    const UiStrokePoint normal_out = normal_of(outgoing);
    const float turn = cross(incoming, outgoing);
    const float along = dot(incoming, outgoing);
    // The inner side is the one the stroke turns toward: the left normal for a positive cross product.
    const float side = turn < 0.0f ? -1.0f : 1.0f;
    const float sweep = ase::math::atan2(turn, along);
    const float opening = 1.0f + along;
    const float tangent = opening > GODOT_STROKE_JOIN_EPS ? ase::math::abs(turn) / opening : 0.0f;
    const bool meet = opening > GODOT_STROKE_JOIN_EPS &&
                      reach * tangent <= GODOT_STROKE_INNER_SHARE * ase::math::min(incoming_length, outgoing_length);

    const int32_t outer_in = lay(mesh, minus(point, times(normal_in, side * half)), GODOT_STROKE_CORE_OPACITY);
    const int32_t outer_in_feather =
        lay(mesh, minus(point, times(normal_in, side * reach)), GODOT_STROKE_FEATHER_OPACITY);
    const int32_t outer_out = lay(mesh, minus(point, times(normal_out, side * half)), GODOT_STROKE_CORE_OPACITY);
    const int32_t outer_out_feather =
        lay(mesh, minus(point, times(normal_out, side * reach)), GODOT_STROKE_FEATHER_OPACITY);

    int32_t inner_in = 0;
    int32_t inner_in_feather = 0;
    int32_t inner_out = 0;
    int32_t inner_out_feather = 0;
    int32_t hub = 0;
    if (meet) {
        // Both inner offset lines cross a tangent of the half turn back along the incoming segment.
        inner_in = lay(mesh, minus(plus(point, times(normal_in, side * half)), times(incoming, half * tangent)),
                       GODOT_STROKE_CORE_OPACITY);
        inner_in_feather =
            lay(mesh, minus(plus(point, times(normal_in, side * reach)), times(incoming, reach * tangent)),
                GODOT_STROKE_FEATHER_OPACITY);
        inner_out = inner_in;
        inner_out_feather = inner_in_feather;
        hub = inner_in;
    } else {
        inner_in = lay(mesh, plus(point, times(normal_in, side * half)), GODOT_STROKE_CORE_OPACITY);
        inner_in_feather = lay(mesh, plus(point, times(normal_in, side * reach)), GODOT_STROKE_FEATHER_OPACITY);
        inner_out = lay(mesh, plus(point, times(normal_out, side * half)), GODOT_STROKE_CORE_OPACITY);
        inner_out_feather = lay(mesh, plus(point, times(normal_out, side * reach)), GODOT_STROKE_FEATHER_OPACITY);
        hub = lay(mesh, point, GODOT_STROKE_CORE_OPACITY);
    }
    fan(mesh, hub, point, outer_in, outer_in_feather, outer_out, outer_out_feather, angle_of(times(normal_in, -side)),
        sweep, half, reach);

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
void segment(UiStrokeMesh& mesh, int32_t start_left, int32_t start_left_feather, int32_t start_right,
             int32_t start_right_feather, int32_t end_left, int32_t end_left_feather, int32_t end_right,
             int32_t end_right_feather) {
    triangle(mesh, start_left, end_left, end_right);
    triangle(mesh, start_left, end_right, start_right);
    band(mesh, start_left, end_left, end_left_feather, start_left_feather);
    band(mesh, start_right, end_right, end_right_feather, start_right_feather);
}

// The round end at `end` from the side `from_left` (core and feather) around to the other side: a half disc.
void cap(UiStrokeMesh& mesh, const UiStrokePoint& end, const UiStrokePoint& left, float half, float reach,
         bool at_start, int32_t& end_left, int32_t& end_left_feather, int32_t& end_right, int32_t& end_right_feather) {
    const int32_t hub = lay(mesh, end, GODOT_STROKE_CORE_OPACITY);
    end_left = lay(mesh, plus(end, times(left, half)), GODOT_STROKE_CORE_OPACITY);
    end_left_feather = lay(mesh, plus(end, times(left, reach)), GODOT_STROKE_FEATHER_OPACITY);
    end_right = lay(mesh, minus(end, times(left, half)), GODOT_STROKE_CORE_OPACITY);
    end_right_feather = lay(mesh, minus(end, times(left, reach)), GODOT_STROKE_FEATHER_OPACITY);
    if (at_start) {
        // stroke-linecap round at the start: from the left side back around to the right.
        fan(mesh, hub, end, end_left, end_left_feather, end_right, end_right_feather, angle_of(left), ase::math::PI,
            half, reach);
        return;
    }
    // stroke-linecap round at the end: from the right side forward around to the left.
    fan(mesh, hub, end, end_right, end_right_feather, end_left, end_left_feather, angle_of(times(left, -1.0f)),
        ase::math::PI, half, reach);
}

}  // namespace

int64_t stroke_run(UiStrokePoint* run, int64_t given, bool closed) {
    int64_t count = 0;
    for (int64_t index = 0; index < given; ++index) {
        if (count == 0 || length_of(minus(run[index], run[count - 1])) > GODOT_STROKE_POINT_EPS_PX) {
            run[count] = run[index];
            ++count;
        }
    }
    while (closed && count > 1 && length_of(minus(run[count - 1], run[0])) <= GODOT_STROKE_POINT_EPS_PX) {
        --count;
    }
    return count;
}

UiStrokeSize stroke_bound(int64_t count, bool closed, float reach) {
    UiStrokeSize bound;
    if (count < 1) {
        return bound;
    }
    const bool ring = closed && count >= 3;
    const int64_t arc = fan_steps(ase::math::PI, reach) + GODOT_STROKE_SPARE_STEPS;
    if (count == 1) {
        const int64_t disc = fan_steps(ase::math::TWO_PI, reach) + GODOT_STROKE_SPARE_STEPS;
        bound.corners = GODOT_STROKE_DISC_CORNERS + GODOT_STROKE_STEP_CORNERS * (disc - 1);
        bound.indices = GODOT_STROKE_STEP_INDICES * disc;
        return bound;
    }
    const int64_t segments = ring ? count : count - 1;
    const int64_t knees = ring ? count : count - 2;
    const int64_t caps = ring ? 0 : 2;
    const int64_t fan_corners = GODOT_STROKE_STEP_CORNERS * (arc - 1);
    bound.corners =
        knees * (GODOT_STROKE_KNEE_CORNERS + fan_corners) + caps * (GODOT_STROKE_CAP_CORNERS + fan_corners);
    bound.indices = (knees + caps) * GODOT_STROKE_STEP_INDICES * arc + segments * GODOT_STROKE_SEGMENT_INDICES;
    return bound;
}

void stroke_lay(const UiStrokePoint* run, int64_t count, bool closed, float half, float reach, UiStrokeMesh& mesh) {
    if (count < 1) {
        return;
    }
    if (count == 1) {
        // A stroke of no length: the disc of its width, as stroke-linecap round draws it.
        const UiStrokePoint centre = run[0];
        const int32_t hub = lay(mesh, centre, GODOT_STROKE_CORE_OPACITY);
        const int32_t rim = lay(mesh, plus(centre, UiStrokePoint{half, 0.0f}), GODOT_STROKE_CORE_OPACITY);
        const int32_t rim_feather = lay(mesh, plus(centre, UiStrokePoint{reach, 0.0f}), GODOT_STROKE_FEATHER_OPACITY);
        fan(mesh, hub, centre, rim, rim_feather, rim, rim_feather, 0.0f, ase::math::TWO_PI, half, reach);
        return;
    }
    const bool ring = closed && count >= 3;
    const int64_t segments = ring ? count : count - 1;

    int32_t start_left = 0;
    int32_t start_left_feather = 0;
    int32_t start_right = 0;
    int32_t start_right_feather = 0;
    int32_t last_left = 0;
    int32_t last_left_feather = 0;
    int32_t last_right = 0;
    int32_t last_right_feather = 0;
    // The direction and length of a segment are taken where it is used: one segment is read by the knee at each of
    // its ends, so no array of them is kept.
    const UiStrokePoint first_span = minus(run[1 % count], run[0]);
    const float first_length = length_of(first_span);
    const UiStrokePoint first_direction = times(first_span, 1.0f / first_length);
    if (ring) {
        // The knee on the first point joins the last segment onto the first.
        const UiStrokePoint last_span = minus(run[0], run[count - 1]);
        const float last_length = length_of(last_span);
        knee(mesh, run[0], times(last_span, 1.0f / last_length), last_length, first_direction, first_length, half,
             reach, last_left, last_left_feather, last_right, last_right_feather, start_left, start_left_feather,
             start_right, start_right_feather);
    } else {
        cap(mesh, run[0], normal_of(first_direction), half, reach, true, start_left, start_left_feather, start_right,
            start_right_feather);
    }

    UiStrokePoint direction = first_direction;
    float length = first_length;
    for (int64_t index = 0; index < segments; ++index) {
        int32_t end_left = 0;
        int32_t end_left_feather = 0;
        int32_t end_right = 0;
        int32_t end_right_feather = 0;
        int32_t next_left = 0;
        int32_t next_left_feather = 0;
        int32_t next_right = 0;
        int32_t next_right_feather = 0;
        UiStrokePoint next_direction = direction;
        float next_length = length;
        if (index + 1 < segments) {
            const UiStrokePoint next_span = minus(run[(index + 2) % count], run[index + 1]);
            next_length = length_of(next_span);
            next_direction = times(next_span, 1.0f / next_length);
            knee(mesh, run[index + 1], direction, length, next_direction, next_length, half, reach, end_left,
                 end_left_feather, end_right, end_right_feather, next_left, next_left_feather, next_right,
                 next_right_feather);
        } else if (ring) {
            end_left = last_left;
            end_left_feather = last_left_feather;
            end_right = last_right;
            end_right_feather = last_right_feather;
        } else {
            cap(mesh, run[count - 1], normal_of(direction), half, reach, false, end_left, end_left_feather, end_right,
                end_right_feather);
        }
        segment(mesh, start_left, start_left_feather, start_right, start_right_feather, end_left, end_left_feather,
                end_right, end_right_feather);
        start_left = next_left;
        start_left_feather = next_left_feather;
        start_right = next_right;
        start_right_feather = next_right_feather;
        direction = next_direction;
        length = next_length;
    }
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - THE MESH OF A ROUND-JOINED STROKE AGAINST ITS BOUND WITHOUT GODOT
 *
 * @file        test_godot_stroke_mesh.cpp
 * @design      DSGN_067
 * @brief       Pins that stroke_lay never lays more corners or indices than stroke_bound sizes the arrays to
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, ui_stroke (godot_hud_ui_stroke.hpp, P0692, T0357):
 *              ui_stroke sizes its arrays once by stroke_bound and lets stroke_lay write through their pointers, so a
 *              bound too small writes past the end. For an open run, a closed ring, a run of one point, a hairpin, a
 *              thick zigzag and a closed run whose end lies on its first point, every case holds: the corners and
 *              indices laid stay within the bound, the indices come in threes and each names a laid corner, and every
 *              corner carries the core or the feather share of the ink's alpha. stroke_run leaves out repeated points
 *              and the closing point of a closed run.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
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

#include <doctest/doctest.h>

#include <ase/adp/godot/stroke/godot_stroke_mesh.hpp>
#include <ase/adp/godot/stroke/types.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using ase::adp::godot::GODOT_STROKE_CORE_OPACITY;
using ase::adp::godot::GODOT_STROKE_FEATHER_OPACITY;
using ase::adp::godot::stroke_bound;
using ase::adp::godot::stroke_lay;
using ase::adp::godot::stroke_run;
using ase::adp::godot::UiStrokeMesh;
using ase::adp::godot::UiStrokePoint;
using ase::adp::godot::UiStrokeSize;

namespace {

constexpr float TEST_HALF_THIN = 0.75f;   // half the core of a thin HUD line, screen pixels
constexpr float TEST_REACH_THIN = 1.75f;  // its feather's end
constexpr float TEST_HALF_THICK = 18.0f;  // half the core of a thick stroke, screen pixels: many fan steps
constexpr float TEST_REACH_THICK = 19.0f; // its feather's end

// The mesh of `points` stroked at `half` and `reach`, laid into arrays sized by the bound, and the checks every
// mesh holds. Hands back what was laid.
UiStrokeSize stroke_checked(std::vector<UiStrokePoint> points, bool closed, float half, float reach) {
    const int64_t count = stroke_run(points.data(), static_cast<int64_t>(points.size()), closed);
    const UiStrokeSize bound = stroke_bound(count, closed, reach);
    std::vector<UiStrokePoint> corners(static_cast<size_t>(bound.corners));
    std::vector<float> opacity(static_cast<size_t>(bound.corners));
    std::vector<int32_t> indices(static_cast<size_t>(bound.indices));
    UiStrokeMesh mesh;
    mesh.corners = corners.data();
    mesh.opacity = opacity.data();
    mesh.indices = indices.data();
    stroke_lay(points.data(), count, closed, half, reach, mesh);

    CHECK(mesh.laid > 0);
    CHECK(mesh.filled > 0);
    CHECK(static_cast<int64_t>(mesh.laid) <= bound.corners);
    CHECK(mesh.filled <= bound.indices);
    CHECK(mesh.filled % 3 == 0);
    for (int64_t index = 0; index < mesh.filled; ++index) {
        const int32_t corner = indices[static_cast<size_t>(index)];
        CHECK(corner >= 0);
        CHECK(corner < mesh.laid);
    }
    for (int32_t index = 0; index < mesh.laid; ++index) {
        const float share = opacity[static_cast<size_t>(index)];
        CHECK((share == GODOT_STROKE_CORE_OPACITY || share == GODOT_STROKE_FEATHER_OPACITY));
    }
    UiStrokeSize laid;
    laid.corners = mesh.laid;
    laid.indices = mesh.filled;
    return laid;
}

}  // namespace

TEST_CASE("stroke_run leaves out repeated points and the closing point of a closed run") {
    std::vector<UiStrokePoint> open{{0.0f, 0.0f}, {0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}};
    CHECK(stroke_run(open.data(), static_cast<int64_t>(open.size()), false) == 3);
    CHECK(open[2].y == 10.0f);

    std::vector<UiStrokePoint> closed{{0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}, {0.0f, 0.0f}};
    CHECK(stroke_run(closed.data(), static_cast<int64_t>(closed.size()), true) == 3);

    std::vector<UiStrokePoint> point{{4.0f, 4.0f}, {4.0f, 4.0f}};
    CHECK(stroke_run(point.data(), static_cast<int64_t>(point.size()), true) == 1);
}

TEST_CASE("a run of one point lays the disc of its width within the bound") {
    stroke_checked({{5.0f, 5.0f}}, false, TEST_HALF_THIN, TEST_REACH_THIN);
    stroke_checked({{5.0f, 5.0f}}, true, TEST_HALF_THICK, TEST_REACH_THICK);
}

TEST_CASE("an open run lays its round ends and knees within the bound") {
    stroke_checked({{0.0f, 0.0f}, {40.0f, 0.0f}}, false, TEST_HALF_THIN, TEST_REACH_THIN);
    stroke_checked({{0.0f, 0.0f}, {40.0f, 0.0f}, {40.0f, 40.0f}}, false, TEST_HALF_THIN, TEST_REACH_THIN);
    stroke_checked({{0.0f, 0.0f}, {40.0f, 0.0f}, {40.0f, 40.0f}}, false, TEST_HALF_THICK, TEST_REACH_THICK);
}

TEST_CASE("a closed ring lays one knee per corner within the bound") {
    stroke_checked({{0.0f, 0.0f}, {40.0f, 0.0f}, {40.0f, 40.0f}, {0.0f, 40.0f}}, true, TEST_HALF_THIN,
                   TEST_REACH_THIN);
    stroke_checked({{0.0f, 0.0f}, {40.0f, 0.0f}, {40.0f, 40.0f}, {0.0f, 40.0f}, {0.0f, 0.0f}}, true,
                   TEST_HALF_THICK, TEST_REACH_THICK);
    // A closed run of two points is stroked open, with its two round ends.
    stroke_checked({{0.0f, 0.0f}, {40.0f, 0.0f}}, true, TEST_HALF_THIN, TEST_REACH_THIN);
}

TEST_CASE("a hairpin turns its fan close to pi and stays within the bound") {
    const UiStrokeSize straight = stroke_checked({{0.0f, 0.0f}, {100.0f, 0.0f}, {200.0f, 0.0f}}, false,
                                                 TEST_HALF_THICK, TEST_REACH_THICK);
    const UiStrokeSize hairpin = stroke_checked({{0.0f, 0.0f}, {100.0f, 0.0f}, {0.0f, 0.5f}}, false,
                                                TEST_HALF_THICK, TEST_REACH_THICK);
    // Positive control: the knee of the hairpin lays a fan of many steps where the straight knee lays one.
    CHECK(hairpin.corners > straight.corners);
    CHECK(hairpin.indices > straight.indices);
}

TEST_CASE("a thick zigzag of many short segments stays within the bound") {
    std::vector<UiStrokePoint> zigzag;
    for (int32_t step = 0; step < 50; ++step) {
        zigzag.push_back(UiStrokePoint{static_cast<float>(step) * 3.0f, (step % 2 == 0) ? 0.0f : 30.0f});
    }
    stroke_checked(zigzag, false, TEST_HALF_THICK, TEST_REACH_THICK);
    stroke_checked(zigzag, true, TEST_HALF_THICK, TEST_REACH_THICK);
}

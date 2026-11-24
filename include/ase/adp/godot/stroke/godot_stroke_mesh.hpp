#pragma once

/**
 * ASE GODOT ADAPTER - THE MESH OF A ROUND-JOINED STROKE WITHOUT GODOT
 *
 * @file        godot_stroke_mesh.hpp
 * @brief       stroke_run, stroke_bound, stroke_lay - the pieces of ui_stroke's mesh in screen pixels, as plain numbers
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, the mesh of ui_stroke (godot_hud_ui_stroke.hpp):
 *                stroke_run    leaves out a point that lies on the one before it, and on a closed run the last
 *                              points that lie on its first
 *                stroke_bound  the most corners and indices stroke_lay can lay for a run of `count` points at the
 *                              feather's end `reach`: every knee and round end turns by at most pi, a run of one
 *                              point once around (stroke/types.hpp)
 *                stroke_lay    the core, the round knees and ends and the feather band as corners with the share of
 *                              the ink's alpha each carries, and the triangles between them; it writes through the
 *                              pointers of a UiStrokeMesh sized by stroke_bound and counts what it laid
 *              ui_stroke places the run in screen pixels, sizes the arrays by the bound, lets stroke_lay fill them
 *              and hands the mesh to the canvas (P0692: one sizing per stroke, no push_back per corner).
 *
 *              WHY A UNIT OF ITS OWN WITHOUT A GODOT TYPE: the mesh is the stroke's whole computation and the only
 *              part a wrong count breaks; in the host library a host test holds every laid corner and index
 *              against the bound the arrays are sized to (T0357), without a Godot process.
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

#include <cstdint>

namespace ase::adp::godot {

/** A point of a stroke in screen pixels. */
struct UiStrokePoint {
    float x = 0.0f;  // screen pixels to the right
    float y = 0.0f;  // screen pixels down
};

/** How many corners and indices a stroke's mesh holds. */
struct UiStrokeSize {
    int64_t corners = 0;  // corners, each with its point and its share of the ink's alpha
    int64_t indices = 0;  // indices into the corners, three per triangle
};

/** The arrays stroke_lay writes into, sized by stroke_bound, and how far it wrote. */
struct UiStrokeMesh {
    UiStrokePoint* corners = nullptr;  // the corner points, screen pixels
    float* opacity = nullptr;          // per corner the share of the ink's alpha: core 1, feather's end 0
    int32_t* indices = nullptr;        // three per triangle
    int32_t laid = 0;                  // corners written
    int64_t filled = 0;                // indices written
};

/**
 * Leaves out of the `given` points of `run` every point within GODOT_STROKE_POINT_EPS_PX of the one kept before it
 * and, when the run is `closed`, the last points within it of the first; moves the kept points to the front and
 * hands back how many there are (at least one when `given` is at least one).
 */
int64_t stroke_run(UiStrokePoint* run, int64_t given, bool closed);

/**
 * The most corners and indices stroke_lay lays for a run of `count` points, `closed` or open, with the feather's
 * end `reach` pixels from the middle of the line.
 */
UiStrokeSize stroke_bound(int64_t count, bool closed, float reach);

/**
 * Lays the mesh of the `count` points of `run` (as stroke_run left them), `closed` or open, its core `half` pixels
 * to each side of the line and its feather to `reach`, into `mesh` from its counts on; a closed run of fewer than
 * three points is stroked open.
 */
void stroke_lay(const UiStrokePoint* run, int64_t count, bool closed, float half, float reach, UiStrokeMesh& mesh);

}  // namespace ase::adp::godot

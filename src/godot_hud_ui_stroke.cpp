/**
 * ASE GODOT ADAPTER - HUD UI STROKE
 *
 * @file        godot_hud_ui_stroke.cpp
 * @brief       A polyline stroked as the browser strokes a path with stroke-linejoin round: one mesh in screen pixels
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_stroke.hpp. The pieces of the mesh are
 *              laid by stroke_lay (stroke/godot_stroke_mesh.cpp) without a Godot type; this unit places the run in
 *              screen pixels, sizes the arrays once by stroke_bound and hands the mesh to the canvas (P0692, T0357).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    B0678
 * @created     2026-11-21
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_stroke.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_cover.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/adp/godot/stroke/godot_stroke_mesh.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

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

    // The run in screen pixels through one read pointer; stroke_run leaves out a point on the one before it and, on
    // a closed run, the end that lies on its first point.
    const int64_t given = points.size();
    const ::godot::Vector2* source = points.ptr();
    std::vector<UiStrokePoint> run(static_cast<size_t>(given));
    for (int64_t index = 0; index < given; ++index) {
        const ::godot::Vector2 placed = device.xform(source[index]);
        run[static_cast<size_t>(index)] = UiStrokePoint{placed.x, placed.y};
    }
    const int64_t count = stroke_run(run.data(), given, closed);

    // The arrays sized once for the most the run can lay, written through their pointers and cut to what was laid.
    const UiStrokeSize bound = stroke_bound(count, closed, reach);
    std::vector<UiStrokePoint> corners(static_cast<size_t>(bound.corners));
    std::vector<float> opacity(static_cast<size_t>(bound.corners));
    ::godot::PackedInt32Array index_array;
    index_array.resize(bound.indices);
    UiStrokeMesh mesh;
    mesh.corners = corners.data();
    mesh.opacity = opacity.data();
    mesh.indices = index_array.ptrw();
    stroke_lay(run.data(), count, closed, half, reach, mesh);
    index_array.resize(mesh.filled);

    // Every corner in the ink, its alpha the ink's times the share stroke_lay gave it: full at the core, clear at the
    // feather's end.
    const ::godot::Color shade = canvas.tinted(color);
    ::godot::PackedVector2Array mesh_array;
    ::godot::PackedColorArray color_array;
    mesh_array.resize(mesh.laid);
    color_array.resize(mesh.laid);
    ::godot::Vector2* placing = mesh_array.ptrw();
    ::godot::Color* tinting = color_array.ptrw();
    for (int32_t index = 0; index < mesh.laid; ++index) {
        const UiStrokePoint& corner = corners[static_cast<size_t>(index)];
        placing[index] = ::godot::Vector2(corner.x, corner.y);
        tinting[index] = shade;
        tinting[index].a = shade.a * opacity[static_cast<size_t>(index)];
    }

    // The mesh lies in screen pixels: the canvas under the inverse of screen(), then the caller's frame again.
    canvas.draw_set_transform_matrix(screen.affine_inverse());
    const ::godot::RID item = canvas.get_canvas_item();
    ::godot::RenderingServer::get_singleton()->canvas_item_add_triangle_array(item, index_array, mesh_array,
                                                                              color_array);
    canvas.draw_set_transform_matrix(frame);
}

}  // namespace ase::adp::godot

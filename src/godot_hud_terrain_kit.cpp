/**
 * ASE GODOT ADAPTER - HUD TERRAIN KIT
 *
 * @file        godot_hud_terrain_kit.cpp
 * @brief       The ground mode over the ground: the lines of the mesh, the arrows of the brook, the root zone, the reticle
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_terrain_kit.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0463 (flora.root.v1: the radius of the root zone, the bridge fills it), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_terrain_kit.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** `color` at `alpha`. */
::godot::Color faded(const ::godot::Color& color, double alpha) {
    ::godot::Color out = color;
    out.a = static_cast<float>(alpha);
    return out;
}

}  // namespace

bool TerrainKit::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_terrain_grid_alpha", &grid_alpha_},
             {"hud_terrain_grid_alpha_depth", &grid_alpha_depth_},
             {"hud_terrain_grid_w", &grid_w_},
             {"hud_terrain_column_every", &column_every_},
             {"hud_terrain_column_alpha", &column_alpha_},
             {"hud_terrain_column_w", &column_w_},
             {"hud_terrain_root_alpha", &root_alpha_},
             {"hud_terrain_root_dash", &root_dash_},
             {"hud_terrain_root_gap", &root_gap_},
             {"hud_terrain_root_steps", &root_steps_},
             {"hud_terrain_line_w", &line_w_},
             {"hud_terrain_flow_count", &flow_count_},
             {"hud_terrain_flow_tail", &flow_tail_},
             {"hud_terrain_flow_tip", &flow_tip_},
             {"hud_terrain_flow_back", &flow_back_},
             {"hud_terrain_flow_half", &flow_half_},
             {"hud_ground_line_alpha", &line_alpha_},
             {"hud_ground_line_alpha_depth", &line_alpha_depth_},
             {"hud_ground_line_w", &line_ground_w_},
             {"hud_ground_columns", &ground_columns_},
             {"hud_ground_rows", &ground_rows_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (column_every_ < 1.0 || flow_count_ < 1.0) {
        failed = "TerrainKit: hud_terrain_column_every and hud_terrain_flow_count must be at least 1";
        return false;
    }
    if (ground_columns_ < 2.0 || ground_rows_ < 2.0) {
        failed = "TerrainKit: hud_ground_columns and hud_ground_rows must be at least 2";
        return false;
    }
    columns_ = static_cast<int64_t>(ground_columns_);
    rows_ = static_cast<int64_t>(ground_rows_);
    if (!theme.tone("ground", ground_)) {
        failed = "DOMAIN_GROUND";
        return false;
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"DOMAIN_FLOW", &flow_},
             {"DOMAIN_ALERT", &alert_},
             {"OVERLAY_SHADOW", &shadow_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return crumbs_.bind(theme, failed) && reticle_.bind(theme, failed);
}

const GroundReticle& TerrainKit::reticle() const {
    return reticle_;
}

void TerrainKit::draw_line(UiPen& canvas, int64_t column, int64_t row, int64_t step_column,
                           int64_t step_row, const ::godot::Color& color, double width) const {
    // Each run of seen points is drawn as one polyline. Its packed array is sized once and written through one
    // pointer: a push_back per point crossed the GDExtension border for every point of the lattice (P0696, as
    // UiPen::moved writes its run, P0690).
    const int64_t step = row * columns_ + column;
    const int64_t stride = step_row * columns_ + step_column;
    int64_t from = step;     // the first point of the open run
    int64_t length = 0;      // the points of the open run
    const auto flush = [&]() {
        if (length > 1) {
            ::godot::PackedVector2Array run;
            run.resize(length);
            ::godot::Vector2* to = run.ptrw();
            for (int64_t index = 0; index < length; ++index) {
                to[index] = points_[static_cast<size_t>(from + index * stride)];
            }
            canvas.draw_polyline(run, color, static_cast<float>(width), true);
        }
        length = 0;
    };
    for (; column < columns_ && row < rows_; column += step_column, row += step_row) {
        const int64_t at = row * columns_ + column;
        if (seen_[static_cast<size_t>(at)] != 0) {
            from = length == 0 ? at : from;
            ++length;
            continue;
        }
        flush();
    }
    flush();
}

bool TerrainKit::lay_lattice(const UiAnchor& anchor, const UiGround& ground) {
    // The lattice has the mockup's columns by rows (m.html 360, NX by NZ), spread over the square from edge to
    // edge, each point at the ground height under it - not one point per sample of the port. Under two samples
    // per side the port laid no cell, and no point has a height.
    side_ = ground.side();
    if (side_ < 2) {
        return false;
    }
    const auto count = static_cast<size_t>(columns_ * rows_);
    points_.assign(count, ::godot::Vector2());
    seen_.assign(count, 0);
    bool any = false;
    double bottom = 0.0;
    top_ = 0.0;
    const auto last_column = static_cast<double>(columns_ - 1);
    const auto last_row = static_cast<double>(rows_ - 1);
    for (int64_t row = 0; row < rows_; ++row) {
        for (int64_t column = 0; column < columns_; ++column) {
            const auto at = static_cast<size_t>(row * columns_ + column);
            ::godot::Vector3 place;
            if (!ground.across(static_cast<double>(column) / last_column, static_cast<double>(row) / last_row,
                               place) ||
                !anchor.place(place, points_[at])) {
                continue;
            }
            seen_[at] = 1;
            const auto y = static_cast<double>(points_[at].y);
            top_ = any ? ase::math::min(top_, y) : y;
            bottom = any ? ase::math::max(bottom, y) : y;
            any = true;
        }
    }
    span_ = bottom - top_;
    return any;
}

void TerrainKit::draw_lattice_rows(UiPen& canvas, const ::godot::Color& tone, double alpha,
                                   double alpha_depth, double width) const {
    // Each row fades with its depth: its mean height on the surface between the highest and the lowest.
    for (int64_t row = 0; row < rows_; ++row) {
        double sum = 0.0;
        double seen = 0.0;
        for (int64_t column = 0; column < columns_; ++column) {
            const auto at = static_cast<size_t>(row * columns_ + column);
            if (seen_[at] != 0) {
                sum += static_cast<double>(points_[at].y);
                seen += 1.0;
            }
        }
        if (seen <= 0.0) {
            continue;
        }
        const double depth = span_ > 0.0 ? (sum / seen - top_) / span_ : 0.0;
        draw_line(canvas, 0, row, 1, 0, faded(tone, alpha + alpha_depth * depth), width);
    }
}

bool TerrainKit::draw_rows(UiPen& canvas, const ::godot::Transform2D& frame, const TerrainKitFrame& now,
                           std::string& failed) {
    if (now.anchor == nullptr || now.ground == nullptr) {
        failed = "TerrainKit: the frame carries no anchor or no ground for the rows";
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    if (lay_lattice(*now.anchor, *now.ground)) {
        draw_lattice_rows(canvas, shadow_, line_alpha_, line_alpha_depth_, line_ground_w_);
    }
    // The crumbs of loosened soil lie over the rows in every mode (m.html drawGround 441 to 444, row N08).
    crumbs_.draw(canvas, now.crumbs);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

bool TerrainKit::draw_mesh(UiPen& canvas, const ::godot::Transform2D& frame, const TerrainKitFrame& now,
                           std::string& failed) {
    if (now.anchor == nullptr || now.ground == nullptr) {
        failed = "TerrainKit: the frame carries no anchor or no ground for the mesh";
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    if (lay_lattice(*now.anchor, *now.ground)) {
        draw_lattice_rows(canvas, ground_, grid_alpha_, grid_alpha_depth_, grid_w_);
        const auto every = static_cast<int64_t>(column_every_);
        for (int64_t column = 0; column < columns_; column += every) {
            draw_line(canvas, column, 0, 0, 1, faded(ground_, column_alpha_), column_w_);
        }
    }
    crumbs_.draw(canvas, now.crumbs);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

bool TerrainKit::draw_flow(UiPen& canvas, const UiAnchor& anchor, const UiGround& ground,
                           const VivariumStreamView& stream, double elapsed, std::string& failed) const {
    const double dx = stream.end_x - stream.x;
    const double dz = stream.end_z - stream.z;
    const double length = ::godot::Math::sqrt(dx * dx + dz * dz);
    if (length <= 0.0) {
        failed = "TerrainKit: the stream record " + std::to_string(stream.object_id) + " has a bed without length";
        return false;
    }
    const double travel = elapsed * stream.velocity_m_s / length;
    const auto count = static_cast<int64_t>(flow_count_);
    for (int64_t arrow = 0; arrow < count; ++arrow) {
        double share = static_cast<double>(arrow) / static_cast<double>(count) + travel;
        share -= ::godot::Math::floor(share);
        // The heading comes from the point flow_tail behind; at the start of the bed from the point as far ahead.
        const bool behind = share >= flow_tail_;
        const double other = behind ? share - flow_tail_ : share + flow_tail_;
        ::godot::Vector2 at;
        ::godot::Vector2 near;
        if (!ground.surface(anchor, stream.x + dx * share, stream.z + dz * share, stream.depth_m, at) ||
            !ground.surface(anchor, stream.x + dx * other, stream.z + dz * other, stream.depth_m, near)) {
            continue;
        }
        ::godot::Vector2 heading = behind ? at - near : near - at;
        if (heading.length_squared() <= 0.0f) {
            continue;
        }
        heading = heading.normalized();
        const ::godot::Vector2 side(-heading.y, heading.x);
        ::godot::PackedVector2Array tip;
        tip.push_back(at + heading * static_cast<float>(flow_tip_));
        tip.push_back(at - heading * static_cast<float>(flow_back_) - side * static_cast<float>(flow_half_));
        tip.push_back(at - heading * static_cast<float>(flow_back_) + side * static_cast<float>(flow_half_));
        canvas.draw_colored_polygon(tip, flow_);
    }
    return true;
}

bool TerrainKit::draw_roots(UiPen& canvas, const TerrainKitFrame& now, std::string& failed) {
    // The ring always stands: root_m is flora.root.v1's radius, the hand bridge's while the port is silent (N0463).
    if (!(now.root_m > 0.0)) {
        failed = "TerrainKit: the frame carries no root zone radius";
        return false;
    }
    ::godot::PackedVector2Array ring;
    if (now.ground->ring(*now.anchor, now.origin.x, now.origin.z, now.root_m, static_cast<int64_t>(root_steps_),
                         ring)) {
        svg_dash_points(canvas, ring, faded(alert_, root_alpha_), line_w_, root_dash_, root_gap_);
    }
    return true;
}

bool TerrainKit::draw(UiPen& canvas, const ::godot::Transform2D& frame, const TerrainKitFrame& now,
                      std::string& failed) {
    if (now.anchor == nullptr || now.ground == nullptr || now.streams == nullptr) {
        failed = "TerrainKit: the frame carries no anchor, no ground or no streams";
        return false;
    }
    // The lines of the mesh lie under the overlay on their own layer, ground_terrain (draw_mesh).
    canvas.draw_set_transform_matrix(frame);
    bool drawn = draw_roots(canvas, now, failed);
    for (const VivariumStreamView& stream : *now.streams) {
        if (!drawn) {
            break;
        }
        drawn = draw_flow(canvas, *now.anchor, *now.ground, stream, now.elapsed, failed);
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return drawn && reticle_.draw(canvas, frame, now.reticle, failed);
}

}  // namespace ase::adp::godot

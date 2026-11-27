/**
 * ASE GODOT ADAPTER - HUD ROOT CUT
 *
 * @file        godot_hud_root_cut.cpp
 * @brief       The ground opened under the trunk in the water mode: soil, held water, the level after a pour, roots
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_root_cut.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0463 (flora.root.v1), N0454 (flora.patch.v1 level after a pour), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_root_cut.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/sprite/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_color_array.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool RootCut::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_root_cut_rx", &rx_},
             {"hud_root_cut_ry", &ry_},
             {"hud_root_cut_dy", &dy_},
             {"hud_root_cut_floor_pad", &floor_pad_},
             {"hud_root_cut_arc_points", &arc_points_},
             {"hud_root_cut_pour_dash", &pour_dash_},
             {"hud_root_cut_pour_line", &pour_line_},
             {"hud_root_cut_rim_line", &rim_line_},
             {"hud_root_cut_root_width", &root_width_},
             {"hud_root_cut_root_width_step", &root_width_step_},
             {"hud_root_cut_root_width_min", &root_width_min_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"ROOT_CUT_SOIL_TOP", &soil_top_},
                                                                       {"ROOT_CUT_SOIL_DEEP", &soil_deep_},
                                                                       {"DOMAIN_WATER_HELD", &held_},
                                                                       {"DOMAIN_WATER_DEEP", &deep_},
                                                                       {"DOMAIN_WATER", &water_},
                                                                       {"DOMAIN_WATER_RIM", &rim_color_},
                                                                       {"ROOT_CUT_ROOT", &root_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(rx_ > 0.0) || !(ry_ > 0.0) || !(arc_points_ >= 2.0)) {
        failed = "hud_root_cut_rx and hud_root_cut_ry must be positive and hud_root_cut_arc_points at least 2";
        return false;
    }
    if (dy_ > floor_pad_) {
        failed = "hud_root_cut_floor_pad must cover hud_root_cut_dy, or the floor of the cut falls below the soil";
        return false;
    }
    return true;
}

::godot::Vector2 RootCut::rim(const ::godot::Vector2& centre, double share) const {
    const float turn = static_cast<float>(share) * ::ase::math::PI;
    return centre + ::godot::Vector2(static_cast<float>(rx_) * ::ase::math::cos(turn),
                                     static_cast<float>(ry_) * ::ase::math::sin(turn));
}

::godot::Color RootCut::ramp(const ::godot::Color& upper, const ::godot::Color& lower, double top, double span,
                             double y) {
    const double share = span > 0.0 ? ::ase::math::clamp((y - top) / span, 0.0, 1.0) : 0.0;
    return upper.lerp(lower, static_cast<float>(share));
}

void RootCut::draw_water(UiPen& canvas, const ::godot::Vector2& centre, double trunk_y,
                         double level) const {
    // The level line meets the rim where sin(turn) is the depth share s; the water is the rim below it.
    const double depth = ::ase::math::clamp((level - static_cast<double>(centre.y)) / ry_, 0.0, 1.0);
    const double half = rx_ * static_cast<double>(::ase::math::sqrt(static_cast<float>(1.0 - depth * depth)));
    const double floor_y = trunk_y + ry_;
    ::godot::PackedVector2Array points;
    ::godot::PackedColorArray colors;
    points.push_back(::godot::Vector2(centre.x + static_cast<float>(half), static_cast<float>(level)));
    colors.push_back(held_);
    const uint32_t count = static_cast<uint32_t>(arc_points_);
    for (uint32_t i = 0; i < count; ++i) {
        const ::godot::Vector2 point = rim(centre, static_cast<double>(i) / static_cast<double>(count - 1));
        if (static_cast<double>(point.y) > level) {
            points.push_back(point);
            colors.push_back(ramp(held_, deep_, level, floor_y - level, static_cast<double>(point.y)));
        }
    }
    points.push_back(::godot::Vector2(centre.x - static_cast<float>(half), static_cast<float>(level)));
    colors.push_back(held_);
    if (points.size() >= GODOT_ITEM_POLYGON_MIN) {
        canvas.draw_polygon(points, colors);
    }
}

bool RootCut::cut(const ::godot::Vector2& centre, ::godot::Vector2& a, ::godot::Vector2& b) const {
    // Inside the ellipse: A t^2 + B t + C <= 0 over the segment a + t (b - a); below the centre line: y >= centre.y.
    const double ax = static_cast<double>(a.x - centre.x) / rx_;
    const double ay = static_cast<double>(a.y - centre.y) / ry_;
    const double dx = static_cast<double>(b.x - a.x) / rx_;
    const double dy = static_cast<double>(b.y - a.y) / ry_;
    const double qa = dx * dx + dy * dy;
    const double qb = 2.0 * (ax * dx + ay * dy);
    const double qc = ax * ax + ay * ay - 1.0;
    double low = 0.0;
    double high = 1.0;
    if (qa > 0.0) {
        const double disc = qb * qb - 4.0 * qa * qc;
        if (disc < 0.0) {
            return false;
        }
        const double root = static_cast<double>(::ase::math::sqrt(static_cast<float>(disc)));
        low = ::ase::math::max(low, (-qb - root) / (2.0 * qa));
        high = ::ase::math::min(high, (-qb + root) / (2.0 * qa));
    } else if (qc > 0.0) {
        return false;
    }
    if (dy != 0.0) {
        const double cross = -ay / dy;
        if (dy > 0.0) {
            low = ::ase::math::max(low, cross);
        } else {
            high = ::ase::math::min(high, cross);
        }
    } else if (ay < 0.0) {
        return false;
    }
    if (!(high > low)) {
        return false;
    }
    const ::godot::Vector2 from = a;
    const ::godot::Vector2 span = b - a;
    a = from + span * static_cast<float>(low);
    b = from + span * static_cast<float>(high);
    return true;
}

bool RootCut::draw(UiPen& canvas, const ::godot::Transform2D& frame, const RootCutFrame& now,
                   std::string& failed) {
    if (now.anchor == nullptr) {
        failed = "RootCut drawn without the projection of the frame";
        return false;
    }
    ::godot::Vector2 trunk;
    if (!now.anchor->trunk(now.origin, trunk)) {
        return true;  // the trunk lies behind the camera: there is no ground to open this frame
    }

    const double trunk_y = static_cast<double>(trunk.y);
    const ::godot::Vector2 centre = trunk + ::godot::Vector2(0.0f, static_cast<float>(dy_));
    const uint32_t count = static_cast<uint32_t>(arc_points_);
    ::godot::PackedVector2Array outline;
    ::godot::PackedColorArray soil;
    for (uint32_t i = 0; i < count; ++i) {
        const ::godot::Vector2 point = rim(centre, static_cast<double>(i) / static_cast<double>(count - 1));
        outline.push_back(point);
        soil.push_back(ramp(soil_top_, soil_deep_, trunk_y, ry_, static_cast<double>(point.y)));
    }

    canvas.draw_set_transform_matrix(frame);
    canvas.draw_polygon(outline, soil);

    const double soil_rel = ::ase::math::clamp(now.soil_rel, 0.0, 1.0);
    draw_water(canvas, centre, trunk_y, static_cast<double>(centre.y) + ry_ * (1.0 - soil_rel));
    if (now.pour_rel > 0.0) {
        const double reach = ::ase::math::min(1.0, soil_rel + now.pour_rel);
        const double depth = 1.0 - reach;
        const float level = centre.y + static_cast<float>(ry_ * depth);
        const float half = static_cast<float>(rx_) * ::ase::math::sqrt(static_cast<float>(1.0 - depth * depth));
        canvas.draw_dashed_line(::godot::Vector2(centre.x - half, level), ::godot::Vector2(centre.x + half, level),
                                water_, static_cast<float>(pour_line_), static_cast<float>(pour_dash_));
    }

    if (now.roots != nullptr) {
        for (const RootSegment& segment : *now.roots) {
            ::godot::Vector2 a;
            ::godot::Vector2 b;
            if (!now.anchor->place(segment.from, a) || !now.anchor->place(segment.to, b) || !cut(centre, a, b)) {
                continue;  // the segment lies behind the camera or outside the cut
            }
            const double width = ::ase::math::max(
                root_width_min_, root_width_ - static_cast<double>(segment.order) * root_width_step_);
            canvas.draw_line(a, b, root_, static_cast<float>(width), true);
        }
    }

    canvas.draw_polyline(outline, rim_color_, static_cast<float>(rim_line_), true);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

}  // namespace ase::adp::godot

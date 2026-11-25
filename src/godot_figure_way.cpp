/**
 * ASE GODOT ADAPTER - FIGURE WAY IMPLEMENTATION
 *
 * @file        godot_figure_way.cpp
 * @brief       Reads the way of a figure from its node, advances it by the place of the port and writes it back
 * @description See godot_figure_way.hpp. The eight numbers of FigureStepWay stand in one PackedFloat64Array
 *              under one meta name, in the order of the struct; a node without it, or with an array of another
 *              length, starts its way at the place of the port.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101588 - move player figure over ground
 * @actor       SS/PC
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_way.hpp>
#include <ase/adp/godot/godot_figure_step.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** THE WAY ON THE NODE. Only this unit reads and writes it, so the name stands here and not in types.hpp. */
constexpr const char* kFigureMetaWay = "step_way";  // meta of a body: the eight numbers of its FigureStepWay
constexpr int64_t kFigureWayNumbers = 8;            // from x y z, to x y z, since, span: the fields of FigureStepWay in order

}  // namespace

::godot::Vector3 figure_way_place(::godot::Node3D* body, const ::godot::Vector3& place, double dt, bool fresh) {
    if (body == nullptr) {
        return place;
    }
    FigureStepWay way;
    ::godot::PackedFloat64Array kept;
    if (body->has_meta(kFigureMetaWay)) {
        kept = body->get_meta(kFigureMetaWay);
    }
    const bool known = !fresh && kept.size() == kFigureWayNumbers;
    if (known) {
        way = FigureStepWay{kept[0], kept[1], kept[2], kept[3], kept[4], kept[5], kept[6], kept[7]};
    }
    figure_step_take(way, static_cast<double>(place.x), static_cast<double>(place.y), static_cast<double>(place.z),
                     dt, !known);
    kept.resize(kFigureWayNumbers);
    kept.set(0, way.from_x);
    kept.set(1, way.from_y);
    kept.set(2, way.from_z);
    kept.set(3, way.to_x);
    kept.set(4, way.to_y);
    kept.set(5, way.to_z);
    kept.set(6, way.since);
    kept.set(7, way.span);
    body->set_meta(kFigureMetaWay, kept);

    const double along = figure_step_along(way);
    const ::godot::Vector3 from(static_cast<float>(way.from_x), static_cast<float>(way.from_y),
                                static_cast<float>(way.from_z));
    return from.lerp(place, static_cast<float>(along));
}

}  // namespace ase::adp::godot

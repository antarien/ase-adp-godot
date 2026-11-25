/**
 * ASE GODOT ADAPTER - FIGURE STEP IMPLEMENTATION
 *
 * @file        godot_figure_step.cpp
 * @brief       Advances the way of a figure by the place of its port and closes its turn free of the frame rate
 * @description See godot_figure_step.hpp. The span limit stands in the unit, which alone uses it, as the figure
 *              scene keeps its own measures (godot_figure_scene.cpp), not in types.hpp.
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

#include <ase/adp/godot/godot_figure_step.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

namespace {

constexpr double kFigureStepSpanMax = 0.05;  // a move later than this (seconds, a frame under 20 Hz) is drawn at once: every frame then carries a step of its own
constexpr double kFigureStepWhole = 1.0;     // the whole way: the figure stands at the place of the port

}  // namespace

void figure_step_take(FigureStepWay& way, double x, double y, double z, double dt, bool fresh) {
    if (fresh) {
        way = FigureStepWay{x, y, z, x, y, z, 0.0, 0.0};
        return;
    }
    way.since += dt;
    if (x == way.to_x && y == way.to_y && z == way.to_z) {
        return;
    }
    // the port moved it: the new way runs from the place before to the place now over the span since that move
    way.from_x = way.to_x;
    way.from_y = way.to_y;
    way.from_z = way.to_z;
    way.to_x = x;
    way.to_y = y;
    way.to_z = z;
    way.span = way.since;
    way.since = 0.0;
}

double figure_step_along(const FigureStepWay& way) {
    if (way.span <= 0.0 || way.span > kFigureStepSpanMax) {
        return kFigureStepWhole;
    }
    return ase::math::clamp(way.since / way.span, 0.0, kFigureStepWhole);
}

double figure_turn_closed(double dt, double rate) {
    if (dt <= 0.0 || rate <= 0.0) {
        return 0.0;
    }
    return kFigureStepWhole - static_cast<double>(ase::math::exp(static_cast<float>(-dt * rate)));
}

}  // namespace ase::adp::godot

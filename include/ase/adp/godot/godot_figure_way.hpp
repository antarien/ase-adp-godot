#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE WAY
 *
 * @file        godot_figure_way.hpp
 * @brief       Keeps the way of a figure as meta on its node and gives the place a frame draws the body at
 * @description The way a frame draws a figure on between two fixed steps of its port is computed without a
 *              Godot type (godot_figure_step.hpp). The figure scene builds and places the nodes and holds no
 *              state of its own across frames, so the way stands on the body node, as its other measures do
 *              (godot_figure_node.hpp): one meta value carries the eight numbers of FigureStepWay. A body that
 *              leaves the scene takes its way with it, and a new body starts without one.
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

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

/**
 * The place a frame of `dt` seconds draws `body` at when the port gives it `place`; `fresh` for a body built in
 * this frame, which stands at the place at once. Advances the way kept on the node of `body`.
 */
::godot::Vector3 figure_way_place(::godot::Node3D* body, const ::godot::Vector3& place, double dt, bool fresh);

}  // namespace ase::adp::godot

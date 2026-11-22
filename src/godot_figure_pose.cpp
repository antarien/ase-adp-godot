/**
 * ASE GODOT ADAPTER - FIGURE POSE IMPLEMENTATION
 *
 * @file        godot_figure_pose.cpp
 * @brief       Chooses the held pose of a figure from its rest record and the ring stones under it
 * @description See godot_figure_pose.hpp. The sleep is read first: a figure ase-metabolism holds asleep lies,
 *              even where its record says nothing of the rest, so none sleeps standing; its rest quality against
 *              the target of good rest chooses the side or the back. A key the record does not carry is NaN and
 *              reads as not said.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:2157 - go to Sleep
 * @actor       Sleeping Phases (SLEEPPHASES)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_pose.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/** true when the record carries the key and it says yes: MTB_IS_RESTING and MTB_IS_SLEEPING are 1 or 0. */
bool said(double value) {
    return !std::isnan(value) && value != 0.0;
}

}  // namespace

uint8_t figure_pose(const FigureSceneRest& rest, double east, double north, double radius,
                    const std::vector<FigureSceneStone>& stones) {
    if (said(rest.sleeping)) {
        // below good rest the figure lies on its side; a record without the quality or its target gives the back
        if (!std::isnan(rest.rest_quality) && !std::isnan(rest.rest_target) && rest.rest_quality < rest.rest_target) {
            return FIGURE_POSE_SLEEP_SIDE;
        }
        return FIGURE_POSE_SLEEP;
    }
    if (!said(rest.is_resting)) {
        return FIGURE_POSE_STAND;
    }
    for (const FigureSceneStone& stone : stones) {
        const double dx = stone.x - east;
        const double dz = stone.z - north;
        if (dx * dx + dz * dz <= radius * radius) {
            return FIGURE_POSE_SIT_STONE;
        }
    }
    return FIGURE_POSE_SIT_GROUND;
}

}  // namespace ase::adp::godot

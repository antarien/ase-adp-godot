/**
 * ASE GODOT ADAPTER - THE HIGHEST SAMPLE PER TILE OF THE GROUND AND THE SIGHT LINE OVER IT
 *
 * @file        godot_ground_tops.cpp
 * @brief       The highest ground sample of every tile, and the first place of a sight line the ground stands over
 * @description usecase:101650 show the river water, P0733, see godot_ground_tops.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101650 - show the river water
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/ground/godot_ground_tops.hpp>
#include <ase/adp/godot/ground/types.hpp>

#include <ase/math/math.hpp>

#include <limits>

namespace ase::adp::godot {

namespace {

/** The value `place` of the line's `places` steps from `from` to `to`. */
double along(double from, double to, int64_t place, int64_t places) {
    return from + (to - from) * (static_cast<double>(place) / static_cast<double>(places));
}

/** The steps of `per_place` an axis at `at` takes to the far edge of its tile from `low` to `high`. */
double places_to_edge(double at, double per_place, double low, double high) {
    if (per_place > 0.0) {
        return (high - at) / per_place;
    }
    return per_place < 0.0 ? (low - at) / per_place : std::numeric_limits<double>::infinity();
}

}  // namespace

void GroundTops::lay(int64_t side) {
    side_ = side;
    tiles_ = side_ > 1 ? (side_ - 2) / GODOT_GROUND_TILE_CELLS + 1 : 0;
    tops_.assign(static_cast<size_t>(tiles_ * tiles_), -std::numeric_limits<double>::infinity());
}

void GroundTops::add(int64_t column, int64_t row, double height) {
    if (tiles_ == 0) {
        return;
    }
    // A sample is a corner of the cells on both its sides, which lie in two tiles where it stands on a tile edge.
    const int64_t last = side_ - 2;
    const int64_t west = ase::math::max(column - 1, int64_t{0}) / GODOT_GROUND_TILE_CELLS;
    const int64_t east = ase::math::min(column, last) / GODOT_GROUND_TILE_CELLS;
    const int64_t south = ase::math::max(row - 1, int64_t{0}) / GODOT_GROUND_TILE_CELLS;
    const int64_t north = ase::math::min(row, last) / GODOT_GROUND_TILE_CELLS;
    for (int64_t tile_row = south; tile_row <= north; ++tile_row) {
        for (int64_t tile_column = west; tile_column <= east; ++tile_column) {
            double& top = tops_[static_cast<size_t>(tile_row * tiles_ + tile_column)];
            top = height > top ? height : top;
        }
    }
}

int64_t GroundTops::stretch(const GroundSightLine& line, int64_t place, bool& passed) const {
    const double column = along(line.from_column, line.to_column, place, line.places);
    const double row = along(line.from_row, line.to_row, place, line.places);
    const auto edge = static_cast<double>(side_ - 1);
    const bool inside = column >= 0.0 && row >= 0.0 && column <= edge && row <= edge;
    if (tiles_ == 0 || !inside) {
        passed = true;  // no height off the square
        return place;
    }
    // On the far edge the last cell carries the place, as in UiGround::height.
    const int64_t tile_column = ase::math::min(static_cast<int64_t>(column), side_ - 2) / GODOT_GROUND_TILE_CELLS;
    const int64_t tile_row = ase::math::min(static_cast<int64_t>(row), side_ - 2) / GODOT_GROUND_TILE_CELLS;
    const auto low_column = static_cast<double>(tile_column * GODOT_GROUND_TILE_CELLS);
    const auto low_row = static_cast<double>(tile_row * GODOT_GROUND_TILE_CELLS);
    const auto cells = static_cast<double>(GODOT_GROUND_TILE_CELLS);
    const auto places = static_cast<double>(line.places);
    const double ahead =
        ase::math::min(places_to_edge(column, (line.to_column - line.from_column) / places, low_column,
                                      ase::math::min(low_column + cells, edge)),
                       places_to_edge(row, (line.to_row - line.from_row) / places, low_row,
                                      ase::math::min(low_row + cells, edge)));
    const int64_t rest = line.places - 1 - place;
    const int64_t last = place + static_cast<int64_t>(ase::math::min(ahead, static_cast<double>(rest)));
    const double lowest = ase::math::min(along(line.from_y, line.to_y, place, line.places),
                                         along(line.from_y, line.to_y, last, line.places));
    passed = tops_[static_cast<size_t>(tile_row * tiles_ + tile_column)] < lowest;
    return last;
}

int64_t GroundTops::first_over(const GroundSightLine& line, GroundHeightAt height_at, const void* context,
                               double& over) const {
    for (int64_t place = 1; place < line.places;) {
        bool passed = false;
        const int64_t last = stretch(line, place, passed);
        for (; !passed && place <= last; ++place) {
            const double column = along(line.from_column, line.to_column, place, line.places);
            const double row = along(line.from_row, line.to_row, place, line.places);
            double ground = 0.0;
            if (height_at(context, column, row, ground) && ground > along(line.from_y, line.to_y, place, line.places)) {
                over = ground;
                return place;
            }
        }
        place = last + 1;
    }
    return GODOT_GROUND_NO_PLACE;
}

}  // namespace ase::adp::godot

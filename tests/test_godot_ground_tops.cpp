/**
 * ASE ADAPTER TESTS - THE SIGHT LINE OVER THE GROUND AGAINST THE FULL MARCH, WITHOUT GODOT
 *
 * @file        test_godot_ground_tops.cpp
 * @design      DSGN_066
 * @brief       Holds the sight line by tiles against the full march over a carved bed, and counts the heights it asks
 * @description usecase:101650 show the river water, P0733. The ground is a square of 257 by 257 samples at 0.125 m,
 *              the square of the start configuration, with a bed carved 0.6 m deep across a rolling ground; a height
 *              is read on the triangles of the drawn cell as UiGround::height reads it. The full march asks every
 *              place; the march by tiles must name the same first place and the same ground for every line, and ask
 *              fewer heights over the bed. The points stand on the water of the bed, hidden by its lip, and 0.6 m over
 *              the ground, seen. Further cases: a tile without samples, a line off the square, a line that stands
 *              still, a hill on a tile edge, and a square too small for a cell.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
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

#include <doctest/doctest.h>

#include <ase/adp/godot/ground/godot_ground_tops.hpp>
#include <ase/adp/godot/ground/types.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr int64_t kSide = 257;        // samples per side of the square (half edge 16 m, step 0.125 m)
constexpr double kStep = 0.125;       // metres between two samples
constexpr double kBedRow = 128.0;     // the row the bed is carved along
constexpr double kBedHalf = 8.0;      // half the width of the bed, lattice steps: 1 m
constexpr double kBedDepth = 0.6;     // metres the bed is carved deep
constexpr double kEye = 1.9;          // metres the eye stands over the ground
constexpr double kWater = -0.43;      // metres the water of the bed stands at
constexpr double kSign = 0.6;         // metres a point over the ground stands at

struct Square {
    std::vector<double> samples;
    std::vector<uint8_t> held;        // whether the port carried the sample
    int64_t side = 0;
    GroundTops tops;
};

struct Sight {
    const Square* square = nullptr;
    GroundSightLine line;
    mutable int64_t asked = 0;        // the heights the march asked
};

double bed_height(int64_t column, int64_t row) {
    const double roll = 0.2 * std::sin(static_cast<double>(column) * 0.05) * std::cos(static_cast<double>(row) * 0.07);
    const double off = std::abs(static_cast<double>(row) - kBedRow) / kBedHalf;
    return off < 1.0 ? roll - kBedDepth * (1.0 - off * off) : roll;
}

Square square_of(int64_t side, double (*height)(int64_t, int64_t), bool (*held)(int64_t, int64_t)) {
    Square made;
    made.side = side;
    made.tops.lay(side);
    for (int64_t row = 0; row < side; ++row) {
        for (int64_t column = 0; column < side; ++column) {
            made.samples.push_back(height(column, row));
            made.held.push_back(held(column, row) ? 1u : 0u);
            if (held(column, row)) {
                made.tops.add(column, row, height(column, row));
            }
        }
    }
    return made;
}

bool all_held(int64_t, int64_t) {
    return true;
}

bool height_at(const void* context, double fx, double fz, double& out) {
    const auto* sight = static_cast<const Sight*>(context);
    sight->asked += 1;
    const Square& ground = *sight->square;
    const auto last = static_cast<double>(ground.side - 1);
    if (fx < 0.0 || fz < 0.0 || fx > last || fz > last) {
        return false;
    }
    const int64_t column = std::min(static_cast<int64_t>(fx), ground.side - 2);
    const int64_t row = std::min(static_cast<int64_t>(fz), ground.side - 2);
    const auto sw = static_cast<size_t>(row * ground.side + column);
    const size_t se = sw + 1;
    const size_t nw = sw + static_cast<size_t>(ground.side);
    const size_t ne = nw + 1;
    if (ground.held[sw] == 0u || ground.held[se] == 0u || ground.held[nw] == 0u || ground.held[ne] == 0u) {
        return false;
    }
    const double ax = fx - static_cast<double>(column);
    const double az = fz - static_cast<double>(row);
    const std::vector<double>& h = ground.samples;
    out = ax >= az ? h[sw] + (h[se] - h[sw]) * ax + (h[ne] - h[se]) * az
                   : h[sw] + (h[ne] - h[nw]) * ax + (h[nw] - h[sw]) * az;
    return true;
}

int64_t full_march(const Sight& sight, double& over) {
    const GroundSightLine& line = sight.line;
    for (int64_t place = 1; place < line.places; ++place) {
        const double share = static_cast<double>(place) / static_cast<double>(line.places);
        const double column = line.from_column + (line.to_column - line.from_column) * share;
        const double row = line.from_row + (line.to_row - line.from_row) * share;
        double ground = 0.0;
        if (height_at(&sight, column, row, ground) && ground > line.from_y + (line.to_y - line.from_y) * share) {
            over = ground;
            return place;
        }
    }
    return GODOT_GROUND_NO_PLACE;
}

Sight sight_of(const Square& square, double from_column, double from_row, double from_y, double to_column,
               double to_row, double to_y) {
    Sight sight;
    sight.square = &square;
    const double run = std::hypot(to_column - from_column, to_row - from_row);
    sight.line = GroundSightLine{from_column, from_row, from_y, to_column, to_row, to_y,
                                 static_cast<int64_t>(std::ceil(run / 0.5))};
    return sight;
}

/** Both marches over one line; the first place and its ground must be the same. Adds the heights each asked. */
void hold(const Sight& sight, int64_t& asked_full, int64_t& asked_tiles, int64_t& hidden) {
    double over_full = -1.0;
    double over_tiles = -1.0;
    sight.asked = 0;
    const int64_t full = full_march(sight, over_full);
    asked_full += sight.asked;
    sight.asked = 0;
    const int64_t tiles = sight.square->tops.first_over(sight.line, height_at, &sight, over_tiles);
    asked_tiles += sight.asked;
    hidden += full != GODOT_GROUND_NO_PLACE ? 1 : 0;
    CHECK(tiles == full);
    CHECK(over_tiles == over_full);
}

}  // namespace

TEST_CASE("usecase:101650: the march by tiles names the place and the ground of the full march over a carved bed") {
    const Square square = square_of(kSide, bed_height, all_held);
    int64_t asked_full = 0;
    int64_t asked_tiles = 0;
    int64_t hidden = 0;
    int64_t lines = 0;
    // The eye 9.5 m off the bed on both its sides, at thirteen places along it.
    for (int64_t stand = -6; stand <= 6; ++stand) {
        const double eye_column = 128.0 + static_cast<double>(stand) * 9.0;
        for (int64_t side = -1; side <= 1; side += 2) {
            const double eye_row = kBedRow + static_cast<double>(side) * 9.5 / kStep;
            for (int64_t column = 20; column <= 236; column += 12) {
                for (int64_t across = -7; across <= 7; ++across) {
                    const double to_row = kBedRow + static_cast<double>(across);
                    hold(sight_of(square, eye_column, eye_row, kEye, static_cast<double>(column), to_row, kWater),
                         asked_full, asked_tiles, hidden);
                    hold(sight_of(square, eye_column, eye_row, kEye, static_cast<double>(column), to_row, kSign),
                         asked_full, asked_tiles, hidden);
                    lines += 2;
                }
            }
        }
    }
    MESSAGE("lines ", lines, " hidden ", hidden, " heights full ", asked_full, " by tiles ", asked_tiles);
    // Both answers occur, so the comparison holds a hidden line and a seen one.
    CHECK(hidden > 0);
    CHECK(hidden < lines);
    CHECK(asked_tiles * 3 < asked_full);
}

TEST_CASE("usecase:101650: a line across the whole square and off it has the answer of the full march") {
    const Square square = square_of(kSide, bed_height, all_held);
    int64_t asked_full = 0;
    int64_t asked_tiles = 0;
    int64_t hidden = 0;
    for (int64_t turn = 0; turn < 64; ++turn) {
        const double angle = static_cast<double>(turn) * 0.0981747704246810387;  // a full turn over 64
        const double far_column = 128.0 + 260.0 * std::cos(angle);
        const double far_row = 60.0 + 260.0 * std::sin(angle);
        for (int64_t drop = -4; drop <= 4; ++drop) {
            const double far_y = static_cast<double>(drop) * 0.25;
            hold(sight_of(square, 128.0, 60.0, 0.5, far_column, far_row, far_y), asked_full, asked_tiles, hidden);
            hold(sight_of(square, far_column, far_row, far_y, 128.0, 60.0, 0.5), asked_full, asked_tiles, hidden);
        }
    }
    CHECK(hidden > 0);
}

TEST_CASE("usecase:101650: a tile without samples hides nothing, and its neighbours keep their answer") {
    const Square square = square_of(kSide, bed_height, [](int64_t column, int64_t row) {
        return column < 96 || column > 120 || row < 80 || row > 130;
    });
    int64_t asked_full = 0;
    int64_t asked_tiles = 0;
    int64_t hidden = 0;
    for (int64_t column = 60; column <= 160; ++column) {
        hold(sight_of(square, 108.0, 40.0, 0.4, static_cast<double>(column), 160.0, -0.3), asked_full, asked_tiles,
             hidden);
    }
    CHECK(hidden > 0);
}

TEST_CASE("usecase:101650: a hill on the edge of two tiles hides from both sides") {
    const Square square = square_of(
        33, [](int64_t column, int64_t row) { return column == 16 && row == 16 ? 2.0 : 0.0; }, all_held);
    double over = 0.0;
    const Sight east = sight_of(square, 1.0, 16.0, 1.0, 31.0, 16.0, 1.0);
    const Sight west = sight_of(square, 31.0, 16.0, 1.0, 1.0, 16.0, 1.0);
    CHECK(square.tops.first_over(east.line, height_at, &east, over) != GODOT_GROUND_NO_PLACE);
    CHECK(over > 1.0);
    CHECK(square.tops.first_over(west.line, height_at, &west, over) != GODOT_GROUND_NO_PLACE);
    const Sight beside = sight_of(square, 1.0, 20.0, 1.0, 31.0, 20.0, 1.0);
    CHECK(square.tops.first_over(beside.line, height_at, &beside, over) == GODOT_GROUND_NO_PLACE);
}

TEST_CASE("usecase:101650: a ground laid anew forgets the hill dug away and holds the one raised") {
    const auto hill = [](int64_t column, int64_t row) { return column == 16 && row == 16 ? 2.0 : 0.0; };
    // Column 28 stands inside one tile; a hill on a tile edge would be a corner of the tile beside it as well.
    const auto moved = [](int64_t column, int64_t row) { return column == 28 && row == 16 ? 2.0 : 0.0; };
    Square square = square_of(33, hill, all_held);
    const Square raised = square_of(33, moved, all_held);
    double before = 0.0;
    const Sight standing = sight_of(square, 16.0, 1.0, 1.0, 16.0, 31.0, 1.0);
    CHECK(square.tops.first_over(standing.line, height_at, &standing, before) != GODOT_GROUND_NO_PLACE);
    // UiGround::follow lays the tops anew when terrain_generation moved: the same tops, over the samples of now.
    square.samples = raised.samples;
    square.tops.lay(square.side);
    for (int64_t row = 0; row < square.side; ++row) {
        for (int64_t column = 0; column < square.side; ++column) {
            square.tops.add(column, row, moved(column, row));
        }
    }
    double over = 0.0;
    const Sight across_old = sight_of(square, 16.0, 1.0, 1.0, 16.0, 31.0, 1.0);
    CHECK(square.tops.first_over(across_old.line, height_at, &across_old, over) == GODOT_GROUND_NO_PLACE);
    CHECK(across_old.asked == 0);  // the tile of the hill dug away is passed, its old top is gone
    const Sight across_new = sight_of(square, 28.0, 1.0, 1.0, 28.0, 31.0, 1.0);
    CHECK(square.tops.first_over(across_new.line, height_at, &across_new, over) != GODOT_GROUND_NO_PLACE);
    CHECK(over > 1.0);
}

TEST_CASE("usecase:101650: a line that stands still and a square without a cell hide nothing") {
    const Square square = square_of(33, [](int64_t, int64_t) { return 5.0; }, all_held);
    double over = 7.0;
    const Sight still = sight_of(square, 16.0, 16.0, 1.0, 16.0, 16.0, 0.0);
    CHECK(square.tops.first_over(still.line, height_at, &still, over) == GODOT_GROUND_NO_PLACE);
    const Square point = square_of(1, [](int64_t, int64_t) { return 5.0; }, all_held);
    const Sight across = sight_of(point, 0.0, 0.0, 1.0, 10.0, 10.0, 0.0);
    CHECK(point.tops.first_over(across.line, height_at, &across, over) == GODOT_GROUND_NO_PLACE);
    CHECK(over == 7.0);
}

}  // namespace ase::adp::godot

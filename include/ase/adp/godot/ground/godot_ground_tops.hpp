#pragma once

/**
 * ASE GODOT ADAPTER - THE HIGHEST SAMPLE PER TILE OF THE GROUND AND THE SIGHT LINE OVER IT
 *
 * @file        godot_ground_tops.hpp
 * @brief       The highest ground sample of every tile, and the first place of a sight line the ground stands over
 * @description usecase:101650 show the river water, P0733: the brook's line on the HUD stands where the eye sees the
 *              water over its banks. The march of UiGround::sees asked the ground's height at every half lattice step
 *              between the eye and a point, sixteen heights a metre of sight, for every water point of a brook,
 *              although most of that line runs well over the ground. GroundTops holds the highest sample of every
 *              tile of GODOT_GROUND_TILE_CELLS cells an edge. first_over walks the same places from the eye, but where
 *              the highest sample of the tile a place lies in stays under the line over its whole stretch through that
 *              tile, it goes on behind the tile without asking a height.
 *
 *              THE ANSWER OF THE FULL MARCH. A place inside a cell stands on one of the two triangles over its four
 *              corner samples, so it stands at most as high as the highest of them; a sample on a tile edge counts
 *              for the tiles on both sides. The line is straight, so over a stretch it is lowest at one of its two
 *              ends. A tile under both ends hides nothing on that stretch, and only such a tile is passed over. The
 *              places asked are the ones of the full march, in its order, so the first place over the line and its
 *              ground are the ones it names. A tile without a sample holds no height and is passed over, as the full
 *              march finds none there either; a place off the square has no height and is stepped over alone.
 *
 *              NO GODOT TYPE: lattice steps in, a place out, so the host tests hold it against the full march without
 *              a Godot process. The Godot side is UiGround (godot_hud_ui_ground.hpp), which lays the tiles from the
 *              samples of the terrain port and gives the height by its own triangles.
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

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** A sight line from the eye to a point, walked in equal places: column and row in lattice steps of the square. */
struct GroundSightLine {
    double from_column = 0.0;  // the eye, lattice steps east of the south-west corner
    double from_row = 0.0;     // the eye, lattice steps north of it
    double from_y = 0.0;       // the eye's height, metres
    double to_column = 0.0;    // the point
    double to_row = 0.0;
    double to_y = 0.0;
    int64_t places = 0;        // the steps between eye and point; the places 1 to places - 1 lie between them
};

/** The ground's height at `column`, `row` (lattice steps), as `context` knows it; false without a height there. */
using GroundHeightAt = bool (*)(const void* context, double column, double row, double& out);

/** The highest ground sample of every tile of a square lattice. */
class GroundTops {
public:
    /** An empty square of `side` samples an edge: no tile holds a sample. */
    void lay(int64_t side);

    /** The sample `height` at lattice point `column`, `row`, counted for every tile whose edge or face it lies on. */
    void add(int64_t column, int64_t row, double height);

    /**
     * The first place of `line` whose ground stands over it, by `height_at` with `context`; GODOT_GROUND_NO_PLACE
     * where every place is seen over. `over` takes the ground's height at that place and is kept otherwise.
     */
    int64_t first_over(const GroundSightLine& line, GroundHeightAt height_at, const void* context, double& over) const;

private:
    /** The last place of `line` in the tile `place` lies in, and whether the line stays over that tile until there. */
    int64_t stretch(const GroundSightLine& line, int64_t place, bool& passed) const;

    std::vector<double> tops_;  // per tile (row * tiles + column) its highest sample
    int64_t side_ = 0;          // samples per side of the square
    int64_t tiles_ = 0;         // tiles per side
};

}  // namespace ase::adp::godot

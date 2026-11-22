/**
 * ASE ADAPTER TESTS - THE SWITCH OF THE FIGURE STANDS CLEAR OF THE BAND
 *
 * @file        test_godot_hud_figure_layout.cpp
 * @design      DSGN_021
 * @brief       Pins that the tap square of cam.mode covers no row of the Phase 06 band: not the sky field, not
 *              the season word, not the debug switch above it
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.1, register B0352. cam.mode (fig.cam) stands right
 *              under the debug button of the band (VivariumFigureSwitch, godot_hud_figure_switch.cpp) and
 *              takes a tap inside the square of half side hud-figure-switch-hit around its centre. The band
 *              of VIVARIUM_PHASE_06_HUD_V2 holds, at the right of the head, the sky field a tap opens the
 *              calendar sheet with (hud-band-hit-x/y/w/h), the season word set right-aligned so it ends at
 *              hud-band-season-x, and the debug switch whose square of full side hud-band-debug-hit
 *              stands around hud-band-debug-x/y. Every measure is read from SHA's theme.css in the tree,
 *              the one source the stage turns into design_tokens.json, so a moved token fails here and
 *              never only in the window. Pure computation, no Godot type.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>

#include <cstdlib>
#include <string>

namespace {

// SHA's theme in the tree, from the adapter's folder: the file the stage reads the tokens of
std::string theme_text() {
    const std::string adapter = ase::fileio::parent_of(ase::fileio::parent_of(__FILE__));
    return ase::fileio::read_text(
        ase::fileio::path_join(adapter, "../../clients/sha-client-web/sha-web-styles/src/theme.css"));
}

// the number of the custom property "--ase-<name>: <number>;" in the theme; found is false without it
double token(const std::string& theme, const std::string& name, bool& found) {
    const std::string key = "--ase-" + name + ":";
    const std::size_t at = theme.find(key);
    found = at != std::string::npos;
    if (!found) {
        return 0.0;
    }
    return std::strtod(theme.c_str() + at + key.size(), nullptr);
}

}  // namespace

TEST_CASE("07.1 B0352: the tap square of cam.mode covers no row of the band") {
    const std::string theme = theme_text();
    REQUIRE_MESSAGE(!theme.empty(), "clients/sha-client-web/sha-web-styles/src/theme.css");

    bool found = false;
    const double switch_x = token(theme, "hud-figure-switch-x", found);
    REQUIRE(found);
    const double switch_y = token(theme, "hud-figure-switch-y", found);
    REQUIRE(found);
    const double switch_hit = token(theme, "hud-figure-switch-hit", found);  // half the side of its tap square
    REQUIRE(found);
    const double season_x = token(theme, "hud-band-season-x", found);  // the season word ends here, right-aligned
    REQUIRE(found);
    const double sky_x = token(theme, "hud-band-hit-x", found);
    REQUIRE(found);
    const double sky_y = token(theme, "hud-band-hit-y", found);
    REQUIRE(found);
    const double sky_w = token(theme, "hud-band-hit-w", found);
    REQUIRE(found);
    const double sky_h = token(theme, "hud-band-hit-h", found);
    REQUIRE(found);
    const double debug_y = token(theme, "hud-band-debug-y", found);
    REQUIRE(found);
    const double debug_hit = token(theme, "hud-band-debug-hit", found);  // the full side of its tap square
    REQUIRE(found);

    const double left = switch_x - switch_hit;
    const double top = switch_y - switch_hit;

    // positive control: the measures are read, a square of no side would pass every check below
    CHECK(switch_hit > 0.0);

    // the season word ends at season_x; the square begins right of it
    CHECK(left > season_x);
    // the sky field reaches to sky_x + sky_w; the square begins right of it or stands below it
    CHECK((left >= sky_x + sky_w || top >= sky_y + sky_h));
    // the debug switch's square ends half its side under its centre; the square of cam.mode begins under it
    CHECK(top >= debug_y + debug_hit * 0.5);
}

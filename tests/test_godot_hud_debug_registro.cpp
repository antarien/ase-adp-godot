/**
 * ASE ADAPTER TESTS - THE ROW REGISTRO OF THE SALTO TAB WITHOUT GODOT
 *
 * @file        test_godot_hud_debug_registro.cpp
 * @design      DSGN_046
 * @brief       Pins the place of the row REGISTRO in the debug drawer: every event jump of the start config inside it
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT "Debug: Zeitraffer und Sprünge", TASK row D01: DebugLayer::lay_salto
 *              lays every jump that names a kind of ase-calendar's register (key event in debug.jumps) in a row of its
 *              own under the other jumps, hud_debug_registro_per_row chips to a row, hud_debug_registro_row from one
 *              row to the next, the note under the last row. The case reads SHA's theme.css and the start config of
 *              the app from the tree and holds the rows the layer lays from them against the drawer at the foot and
 *              the 360 of the frame, so a token or a jump more that pushes a chip out of the drawer fails here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>

#include <cstdint>
#include <cstdlib>
#include <string>

namespace ase::adp::godot {

namespace {

// a file of the tree, from the adapter's folder
std::string tree_text(const std::string& relative) {
    const std::string adapter = ase::fileio::parent_of(ase::fileio::parent_of(__FILE__));
    return ase::fileio::read_text(ase::fileio::path_join(adapter, relative));
}

// the number of "--ase-<name>: <number>;" in the theme; a token the theme lacks fails the case that asks for it
double token(const std::string& theme, const std::string& name) {
    const std::string key = "--ase-" + name + ":";
    const std::size_t at = theme.find(key);
    CAPTURE(name);
    REQUIRE(at != std::string::npos);
    return std::strtod(theme.c_str() + at + key.size(), nullptr);
}

// the jumps of debug.jumps that name a kind of the register: one line each with a label and the key event
uint32_t event_jumps(const std::string& config) {
    uint32_t count = 0u;
    std::size_t from = 0u;
    while (from < config.size()) {
        std::size_t end = config.find('\n', from);
        if (end == std::string::npos) {
            end = config.size();
        }
        const std::string line = config.substr(from, end - from);
        if (line.find("\"label\"") != std::string::npos && line.find("\"event\": \"") != std::string::npos) {
            ++count;
        }
        from = end + 1u;
    }
    return count;
}

}  // namespace

TEST_CASE("debug registro: every event jump of the start config lies in the drawer, five to a row, the note above the foot") {
    const std::string theme = tree_text("../../clients/sha-client-web/sha-web-styles/src/theme.css");
    const std::string config = tree_text("../../clients/avt-client-android/godot/config/vivarium_start.json");
    REQUIRE_FALSE(theme.empty());
    REQUIRE_FALSE(config.empty());
    const double frame_w = 360.0;  // the mockup frame, 360 x 640 units
    const double drawer_end = token(theme, "hud-debug-drawer-y") + token(theme, "hud-debug-drawer-h");
    const double chip_h = token(theme, "hud-debug-chip-h");
    const double per_row = token(theme, "hud-debug-registro-per-row");
    const double row = token(theme, "hud-debug-registro-row");
    REQUIRE(per_row >= 1.0);

    // the rows of SALTO from the top of its body as DebugLayer::lay_salto lays them
    const double body_y = token(theme, "hud-debug-drawer-y") + token(theme, "hud-debug-body-dy");
    const double other_y = body_y + token(theme, "hud-debug-stage-row") + token(theme, "hud-debug-hour-block");
    const double registro_y = other_y + token(theme, "hud-debug-registro-dy");
    const uint32_t events = event_jumps(config);
    const uint32_t per = static_cast<uint32_t>(per_row);
    const uint32_t rows = (events + per - 1u) / per;
    CAPTURE(events);
    CHECK(events == 15u);  // Ereignis ▸ and the fourteen kinds of the register but tumbrae and lunbrex
    CHECK(rows == 3u);

    // no chip of REGISTRO over the other jumps, no row over the next, the last row and the note inside the drawer
    CHECK(registro_y >= other_y + chip_h);
    CHECK(row >= chip_h);
    CHECK(registro_y + static_cast<double>(rows - 1u) * row + chip_h <= drawer_end);
    CHECK(registro_y + static_cast<double>(rows) * row + token(theme, "hud-debug-note-jump-dy") <= drawer_end);

    // a full row inside the frame
    CHECK(token(theme, "hud-debug-pad") + (per_row - 1.0) * token(theme, "hud-debug-jump-step") +
              token(theme, "hud-debug-jump-w") <=
          frame_w);
}

}  // namespace ase::adp::godot

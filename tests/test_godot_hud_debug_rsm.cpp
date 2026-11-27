/**
 * ASE ADAPTER TESTS - THE RSM TABLE OF THE DEBUG LAYER WITHOUT GODOT
 *
 * @file        test_godot_hud_debug_rsm.cpp
 * @design      DSGN_074
 * @brief       Pins the clock of the prototype, the column order, the ring shares, the order of the slices and the view
 * @description MIG_ASE_SKILL_RSM steps B5 and B6: the clock is HH:MM:SS as CountdownTimerFormater.format writes it, the
 *              columns stand newest left, the shares carry the largest share of the grace and the running slice (the
 *              server's dcd_shr and qnt_shr) and of the fluctuation and the TUC (acd of acd_max, tuc of tuc_max), a
 *              slice of an entry stands behind the running one by (slc_head - slot + slc_len) % slc_len, a tap on a
 *              view chip chooses one of the three tab layers while a tap that names none leaves it, every layer of
 *              the console lies inside the drawer at the foot, and the two skill rings DCD and ACD of the game HUD
 *              lie in the corner of the top band measured free, at 0 % before the first learning order, read from
 *              SHA's theme.css.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.03.00003 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_hud_debug_rsm.hpp>
#include <ase/adp/godot/rsm/types.hpp>
#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr double kNan = std::numeric_limits<double>::quiet_NaN();

// SHA's theme in the tree, from the adapter's folder: the file the stage copies the tokens of
std::string theme_text() {
    const std::string adapter = ase::fileio::parent_of(ase::fileio::parent_of(__FILE__));
    return ase::fileio::read_text(
        ase::fileio::path_join(adapter, "../../clients/sha-client-web/sha-web-styles/src/theme.css"));
}

// the number of "--ase-<name>: <number>;" in the theme; a token the theme lacks fails the case that asks for it
double token(const std::string& theme, const std::string& name) {
    const std::string key = "--ase-" + name + ":";
    const std::size_t at = theme.find(key);
    CAPTURE(name);
    REQUIRE(at != std::string::npos);
    return std::strtod(theme.c_str() + at + key.size(), nullptr);
}

DebugRsmEntry entry_of(uint32_t id) {
    DebugRsmEntry entry;
    entry.id = id;
    return entry;
}

}  // namespace

TEST_CASE("debug rsm: the clock is HH:MM:SS with two digits a part") {
    CHECK(debug_rsm_clock(0.0) == "00:00:00");
    CHECK(debug_rsm_clock(66.0) == "00:01:06");
    CHECK(debug_rsm_clock(3661.0) == "01:01:01");
    CHECK(debug_rsm_clock(59.9) == "00:00:59");   // whole seconds, cut as the prototype's int is
    CHECK(debug_rsm_clock(360000.0) == "100:00:00");  // the hours grow past two digits where they must
    CHECK(debug_rsm_clock(-5.0) == "00:00:00");
}

TEST_CASE("debug rsm: a value that is no number prints a dash") {
    CHECK(debug_rsm_clock(kNan) == "-");
    CHECK(debug_rsm_clock(std::numeric_limits<double>::infinity()) == "-");
    CHECK(debug_rsm_whole(kNan) == "-");
    CHECK(debug_rsm_whole(36.9) == "36");
    CHECK(debug_rsm_whole(0.0) == "0");
}

TEST_CASE("debug rsm: QNT counts down as qnt_max minus qnt and never goes under 0") {
    CHECK(debug_rsm_left(20.0, 60.0) == doctest::Approx(40.0));
    CHECK(debug_rsm_left(0.0, 60.0) == doctest::Approx(60.0));
    CHECK(debug_rsm_left(75.0, 60.0) == doctest::Approx(0.0));
    CHECK(std::isnan(debug_rsm_left(kNan, 60.0)));
    CHECK(std::isnan(debug_rsm_left(20.0, kNan)));
}

TEST_CASE("debug rsm: the lengths follow from the port values, no key carries them") {
    CHECK(rsm_slice_length(20.0, 40.0) == doctest::Approx(60.0));
    CHECK(std::isnan(rsm_slice_length(kNan, 40.0)));
    CHECK(rsm_grace_length(3.0, 0.25) == doctest::Approx(12.0));
    CHECK(std::isnan(rsm_grace_length(3.0, 0.0)));
    CHECK(std::isnan(rsm_grace_length(3.0, kNan)));
    CHECK(rsm_slice_age(20.0, 2.0, 60.0) == doctest::Approx(140.0));
    CHECK(rsm_slice_age(20.0, 0.0, 60.0) == doctest::Approx(20.0));
    CHECK(std::isnan(rsm_slice_age(20.0, kNan, 60.0)));
    CHECK(debug_rsm_left(20.0, rsm_slice_length(20.0, 40.0)) == doctest::Approx(40.0));
}

TEST_CASE("debug rsm: the columns stand newest left, the newest being the highest entry id") {
    const std::vector<DebugRsmEntry> in = {entry_of(30), entry_of(70), entry_of(50)};
    std::vector<DebugRsmEntry> out;
    debug_rsm_columns(in, 4u, out);

    REQUIRE(out.size() == 3u);
    CHECK(out[0].id == 70u);
    CHECK(out[1].id == 50u);
    CHECK(out[2].id == 30u);
    CHECK(in[0].id == 30u);  // the set the host delivered is left as it was
}

TEST_CASE("debug rsm: no more columns than the table has room for, the oldest ones left out") {
    const std::vector<DebugRsmEntry> in = {entry_of(1), entry_of(2), entry_of(3), entry_of(4), entry_of(5)};
    std::vector<DebugRsmEntry> out;
    debug_rsm_columns(in, 4u, out);

    REQUIRE(out.size() == 4u);
    CHECK(out[0].id == 5u);
    CHECK(out[3].id == 2u);
}

TEST_CASE("debug rsm: before the first learning order the table shows nothing held, so it draws its column of zeros") {
    std::vector<DebugRsmEntry> held;
    std::vector<DebugRsmEntry> out = {entry_of(9)};
    debug_rsm_shown({}, 4u, held, out);

    CHECK(out.empty());   // the drawer then draws one DebugRsmEntry{} under the idle word
    CHECK(held.empty());
    const DebugRsmEntry zero{};
    CHECK(zero.ses == 0.0);
    CHECK(zero.qnt_left == 0.0);
    CHECK(zero.lck == 0.0);
}

TEST_CASE("debug rsm: the last active entry stays with its last values when the port names none") {
    std::vector<DebugRsmEntry> held;
    std::vector<DebugRsmEntry> out;
    DebugRsmEntry last = entry_of(7);
    last.ses = 42.0;
    last.phase = 1.0;
    debug_rsm_shown({last}, 4u, held, out);
    REQUIRE(out.size() == 1u);

    debug_rsm_shown({}, 4u, held, out);  // the entry ended: the port names no entry any more

    REQUIRE(out.size() == 1u);
    CHECK(out[0].id == 7u);
    CHECK(out[0].ses == doctest::Approx(42.0));
    CHECK(out[0].phase == doctest::Approx(1.0));
}

TEST_CASE("debug rsm: a new set replaces the held one, newest left and no more than the table has room for") {
    std::vector<DebugRsmEntry> held = {entry_of(3)};
    std::vector<DebugRsmEntry> out;
    debug_rsm_shown({entry_of(4), entry_of(6), entry_of(5)}, 2u, held, out);

    REQUIRE(out.size() == 2u);
    CHECK(out[0].id == 6u);
    CHECK(out[1].id == 5u);
    REQUIRE(held.size() == 2u);
    CHECK(held[0].id == 6u);
}

TEST_CASE("debug rsm: the rings carry the largest share over all entries") {
    DebugRsmEntry first = entry_of(1);
    first.dcd_shr = 0.6;       // the server's share of the grace left
    first.acd = 117.0;
    first.acd_max = 120.0;
    first.qnt_shr = 0.25;      // the server's share of the running slice
    first.tuc = 30.0;
    first.tuc_max = 120.0;
    DebugRsmEntry second = entry_of(2);
    second.dcd_shr = 0.25;
    second.acd = 30.0;
    second.acd_max = 120.0;
    second.qnt_shr = 0.75;
    second.tuc = 90.0;
    second.tuc_max = 120.0;

    const DebugRsmShares shares = debug_rsm_shares({first, second});
    CHECK(shares.dcd == doctest::Approx(0.6));
    CHECK(shares.acd == doctest::Approx(0.975));
    CHECK(shares.qnt == doctest::Approx(0.75));
    CHECK(shares.tuc == doctest::Approx(0.75));
}

TEST_CASE("debug rsm: a share stays within 0 and 1, an entry with no length takes no part") {
    DebugRsmEntry over = entry_of(1);
    over.dcd_shr = 1.5;
    DebugRsmEntry bare = entry_of(2);
    bare.dcd_shr = kNan;
    bare.acd = kNan;
    bare.acd_max = 120.0;
    bare.tuc = 50.0;
    bare.tuc_max = 0.0;
    bare.qnt_shr = kNan;

    const DebugRsmShares shares = debug_rsm_shares({over, bare});
    CHECK(shares.dcd == doctest::Approx(1.0));
    CHECK(shares.acd == doctest::Approx(0.0));
    CHECK(shares.qnt == doctest::Approx(0.0));
    CHECK(shares.tuc == doctest::Approx(0.0));
    const DebugRsmShares none = debug_rsm_shares({});
    CHECK(none.dcd == 0.0);
    CHECK(none.acd == 0.0);
    CHECK(none.qnt == 0.0);
    CHECK(none.tuc == 0.0);
}

TEST_CASE("debug rsm: a slice stands behind the running one by its ring place") {
    CHECK(rsm_slice_back(3.0, 3.0, 20.0) == doctest::Approx(0.0));   // the running slice is the newest
    CHECK(rsm_slice_back(3.0, 2.0, 20.0) == doctest::Approx(1.0));
    CHECK(rsm_slice_back(3.0, 0.0, 20.0) == doctest::Approx(3.0));
    CHECK(rsm_slice_back(3.0, 4.0, 20.0) == doctest::Approx(19.0));  // the ring wraps: the oldest place
    CHECK(rsm_slice_back(0.0, 19.0, 20.0) == doctest::Approx(1.0));
    CHECK(std::isnan(rsm_slice_back(3.0, 3.0, 0.0)));                // no ring
    CHECK(std::isnan(rsm_slice_back(kNan, 3.0, 20.0)));
}

TEST_CASE("debug rsm: the age of a slice is qnt plus back times qnt_max") {
    CHECK(rsm_slice_age(12.0, 0.0, 60.0) == doctest::Approx(12.0));
    CHECK(rsm_slice_age(12.0, 2.0, 60.0) == doctest::Approx(132.0));
    CHECK(std::isnan(rsm_slice_age(kNan, 2.0, 60.0)));
    CHECK(std::isnan(rsm_slice_age(12.0, kNan, 60.0)));
}

TEST_CASE("debug rsm: a tap on a view chip chooses the view, a tap that names none leaves it") {
    CHECK(debug_view_choose(GODOT_HUD_DEBUG_VIEW_SYSTEM, GODOT_HUD_DEBUG_VIEW_RSM) == GODOT_HUD_DEBUG_VIEW_RSM);
    CHECK(debug_view_choose(GODOT_HUD_DEBUG_VIEW_RSM, GODOT_HUD_DEBUG_VIEW_SYSTEM) == GODOT_HUD_DEBUG_VIEW_SYSTEM);
    CHECK(debug_view_choose(GODOT_HUD_DEBUG_VIEW_RSM, UINT32_MAX) == GODOT_HUD_DEBUG_VIEW_RSM);
    CHECK(debug_view_choose(GODOT_HUD_DEBUG_VIEW_SYSTEM, GODOT_HUD_DEBUG_VIEWS) == GODOT_HUD_DEBUG_VIEW_SYSTEM);
}

TEST_CASE("debug rsm: a column head is the word of the entry's action code, BDI_CONST_ACTION_*, never a practice 1 to 10") {
    // skill_ctl_sys.cpp takes the acted BDI code as the entry's skill_id: the pour 164, the cut 169, the dig 170 and
    // the heap 171 of the led figure (MIG_ASE_SKILL_RSM B2) must each find a word place, or the RSM view fails the HUD.
    uint32_t slot = UINT32_MAX;
    CHECK(debug_rsm_action_slot(164.0, slot));
    CHECK(slot == 164u - GODOT_RSM_ACTION_FIRST);
    CHECK(debug_rsm_action_slot(169.0, slot));
    CHECK(slot == 169u - GODOT_RSM_ACTION_FIRST);
    CHECK(debug_rsm_action_slot(170.0, slot));
    CHECK(debug_rsm_action_slot(171.0, slot));
    CHECK(slot == GODOT_RSM_ACTION_LAST - GODOT_RSM_ACTION_FIRST);
    CHECK(debug_rsm_action_slot(150.0, slot));
    CHECK(slot == 0u);
    for (double practice = 1.0; practice <= 10.0; practice += 1.0) {
        CHECK_FALSE(debug_rsm_action_slot(practice, slot));
    }
    CHECK_FALSE(debug_rsm_action_slot(172.0, slot));
    CHECK_FALSE(debug_rsm_action_slot(std::numeric_limits<double>::quiet_NaN(), slot));
}

TEST_CASE("debug rsm: the console opens on SYSTEM and has the three tab layers SYSTEM, RSM, STATUS") {
    CHECK(GODOT_HUD_DEBUG_VIEW_SYSTEM == 0u);
    CHECK(GODOT_HUD_DEBUG_VIEW_RSM == 1u);
    CHECK(GODOT_HUD_DEBUG_VIEW_STATUS == 2u);
    CHECK(GODOT_HUD_DEBUG_VIEWS == 3u);
    // one layer at a time: a tap on a chip shows its layer, a tap that names no layer keeps the one that shows
    CHECK(debug_view_choose(GODOT_HUD_DEBUG_VIEW_SYSTEM, GODOT_HUD_DEBUG_VIEW_STATUS) == GODOT_HUD_DEBUG_VIEW_STATUS);
    CHECK(debug_view_choose(GODOT_HUD_DEBUG_VIEW_STATUS, GODOT_HUD_DEBUG_VIEW_RSM) == GODOT_HUD_DEBUG_VIEW_RSM);
    CHECK(debug_view_choose(GODOT_HUD_DEBUG_VIEW_RSM, GODOT_HUD_DEBUG_VIEWS) == GODOT_HUD_DEBUG_VIEW_RSM);
}

TEST_CASE("debug rsm: nothing of the console stands over the scene, every layer lies inside the drawer at the foot") {
    const std::string theme = theme_text();
    REQUIRE_FALSE(theme.empty());
    const double drawer_y = token(theme, "hud-debug-drawer-y");
    const double drawer_end = drawer_y + token(theme, "hud-debug-drawer-h");
    const double tab_w = token(theme, "hud-debug-tab-w");
    const double tab_h = token(theme, "hud-debug-tab-h");
    const double tab_row = drawer_y + token(theme, "hud-debug-tab-dy");
    const double view_x = token(theme, "hud-debug-view-x");
    const double view_top = drawer_y + token(theme, "hud-debug-view-dy");
    const double view_end = view_top + tab_h;
    const double fold_hit_x = token(theme, "hud-debug-fold-hit-x");
    const double frame_w = 360.0;  // the mockup frame, 360 x 640 units

    // the handle at the foot, the view chips in the title row: under the drawer line, above the tab row of SYSTEM,
    // left of the fold
    CHECK(token(theme, "hud-debug-handle-y") >= drawer_y);
    CHECK(view_top >= drawer_y);
    CHECK(view_end <= tab_row);
    CHECK(view_x + static_cast<double>(GODOT_HUD_DEBUG_VIEWS - 1u) * token(theme, "hud-debug-view-step") + tab_w <=
          fold_hit_x);

    // the body of RSM and STATUS: under the view chips, inside the drawer. The grid stands as DebugRsmTable::grid draws
    // it: the column `place` in band place / band-cols, each band GODOT_RSM_GRID_ROWS touching cells, the second band
    // a rule gap under the first (row_y); the foot is the last cell of the last band plus the bottom margin
    const double body_y = token(theme, "hud-debug-y");
    const double row_step = token(theme, "hud-debug-rsm-row-step");
    const double top_dy = token(theme, "hud-debug-rsm-top-dy");
    const double band_cols = token(theme, "hud-debug-rsm-band-cols");
    REQUIRE(band_cols >= 1.0);
    const double bands = std::ceil(token(theme, "hud-debug-rsm-cols") / band_cols);
    const double grid_rows = bands * static_cast<double>(GODOT_RSM_GRID_ROWS);
    const double table_top = body_y + top_dy;
    const double table_foot = body_y - row_step + grid_rows * row_step +
                              (bands > 1.0 ? token(theme, "hud-debug-rsm-rule-gap") : 0.0) + top_dy +
                              token(theme, "hud-debug-rsm-cell-h") + token(theme, "hud-debug-rsm-bottom-dy");
    CAPTURE(table_foot);
    CHECK(table_top >= view_end);
    CHECK(table_foot <= drawer_end);
    CHECK(body_y + 3.0 * token(theme, "hud-debug-line-step") <= drawer_end);  // the four status lines

    // a band of the grid ends inside the frame; the places the rings QNT and TUC keep lie inside the drawer
    const double table_right = token(theme, "hud-debug-x") +
                               band_cols * (token(theme, "hud-debug-rsm-col-w") + token(theme, "hud-debug-rsm-col-gap"));
    CAPTURE(table_right);
    CHECK(table_right <= frame_w);
    const double ring_x = token(theme, "hud-debug-rsm-ring-x");
    const double ring_r = token(theme, "hud-debug-rsm-ring-r");
    const double ring_line = token(theme, "hud-debug-rsm-ring-line");
    CHECK(ring_x + ring_r + ring_line <= frame_w);
    for (const char* ring : {"qnt", "tuc"}) {
        const double centre = token(theme, std::string("hud-debug-rsm-ring-") + ring + "-y");
        CAPTURE(ring);
        CHECK(centre + token(theme, "hud-debug-rsm-ring-word-dy") >= view_end);
        CHECK(centre + ring_r + ring_line <= drawer_end);
    }
}

TEST_CASE("skill rings: DCD above ACD in the free corner of the top band, at 0 % before the first learning order") {
    const std::string theme = theme_text();
    REQUIRE_FALSE(theme.empty());
    // The corner measured free on the top band's frame (godot_hud_skill_rings_draw.hpp, THE PLACE IS MEASURED):
    // x 304 to 356, y 42 to 104, right of the arc's rings, hit and readout, left of the battery and of CORPO.
    const double corner_left = 304.0;
    const double corner_right = 356.0;
    const double corner_top = 42.0;
    const double corner_foot = 104.0;
    const double x = token(theme, "hud-skill-ring-x");
    const double r = token(theme, "hud-skill-ring-r");
    const double line = token(theme, "hud-skill-ring-line");
    const double outer = r + line / 2.0;
    const double dcd_y = token(theme, "hud-skill-ring-dcd-y");
    const double acd_y = token(theme, "hud-skill-ring-acd-y");

    CHECK(dcd_y < acd_y);                          // DCD above, ACD below, as PercentageWheel 1340,180 over 1340,300
    CHECK(dcd_y + outer <= acd_y - outer);         // the two rings do not touch
    CHECK(x - outer >= corner_left);
    CHECK(x + outer <= corner_right);
    CHECK(dcd_y - outer >= corner_top);
    CHECK(acd_y + outer <= corner_foot);
    // the word ends left of its ring and starts inside the corner (three letters of the label face at word_size)
    const double word_end = x + token(theme, "hud-skill-ring-word-dx");
    CHECK(word_end <= x - outer);
    CHECK(word_end - 3.0 * token(theme, "hud-skill-ring-word-size") >= corner_left);

    // the shares the rings show are those of debug_rsm_shares: before the first learning order 0 and 0
    const DebugRsmShares none = debug_rsm_shares({});
    CHECK(none.dcd == 0.0);
    CHECK(none.acd == 0.0);
    DebugRsmEntry learning = entry_of(7);
    learning.dcd_shr = 0.4;
    learning.acd = 60.0;
    learning.acd_max = 120.0;
    const DebugRsmShares shares = debug_rsm_shares({learning});
    CHECK(shares.dcd == doctest::Approx(0.4));
    CHECK(shares.acd == doctest::Approx(0.5));
}

TEST_CASE("activity band: twenty bars fill the band inside the edge rails and under the dividers, the switch in the gap") {
    const std::string theme = theme_text();
    REQUIRE_FALSE(theme.empty());
    // The bottom band of the mockup (360 x 640): the edge rails end at x 24 and begin at x 336, the dividers run at
    // y 552 with their gap over the hand from x 145 to x 214, the hand's hit begins at y 558.
    const double rail_left = 24.0;
    const double rail_right = 336.0;
    const double divider_y = 552.0;
    const double gap_left = 145.0;
    const double gap_right = 214.0;
    const double hand_hit_top = 558.0;
    const double frame_h = 640.0;
    const double x = token(theme, "hud-band-bars-x");
    const double w = token(theme, "hud-band-bars-w");
    const double gap = token(theme, "hud-band-bars-gap");
    const double base = token(theme, "hud-band-bars-base");
    const double h = token(theme, "hud-band-bars-h");
    const double pad = token(theme, "hud-band-bars-hit-pad");
    const double right = x + GODOT_RSM_BARS_MAX * w + (GODOT_RSM_BARS_MAX - 1u) * gap;
    CHECK(x >= rail_left);
    CHECK(right <= rail_right);
    CHECK(base - h - pad >= divider_y);   // the tallest bar and its hit stay under the dividers
    CHECK(base <= frame_h);
    CHECK(token(theme, "hud-band-bars-text-y") > divider_y);
    CHECK(token(theme, "hud-band-bars-text-y") <= base - h);
    const double switch_x = token(theme, "hud-band-switch-x");
    const double switch_y = token(theme, "hud-band-switch-y");
    CHECK(switch_x >= gap_left);
    CHECK(switch_x + token(theme, "hud-band-switch-w") <= gap_right);
    CHECK(switch_y + token(theme, "hud-band-switch-h") <= hand_hit_top);
    CHECK(GODOT_RSM_BAND_HOLD_SECONDS == doctest::Approx(10.0));  // the operator's ten seconds
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - THE SKY ARC OF THE HUD, WITHOUT GODOT
 *
 * @file        test_godot_hud_arc.cpp
 * @design      DSGN_067
 * @brief       Holds the turn of a ring, the fade of its ends, the names of the calendar's codes and the bridge's frame
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, host test ase-adp-godot-arc-test. Cases: the angle of
 *              a disc is (share minus now) times the half span and nothing else; a ring turns by equal steps while now
 *              runs by equal steps (no jump at 100 times); an anchor that moves leaves every sign where it stood; the
 *              fade of the ends over the six stops of SHA; a kind and a ring named by the codes the calendar files and
 *              never by a number of the adapter (the codes here are shuffled on purpose); the bridge's frame while no
 *              port carries a record (the mockup's values and one gap per port), with the three ports whole, with the
 *              arc ahead of the set held (the pair that agreed stands, never a mixture of two states), with no next
 *              event and with an occurrence whose kind no code names.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_hud_arc_pure.hpp>

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

constexpr double kHalf = 27.0;     // hud_arc_day_half_deg of SHA
constexpr double kEpsilon = 1.0e-9;

/** The codes of the calendar, numbered in an order of their own so that no test compares against a known number. */
VivariumEventCodeView calendar_codes() {
    VivariumEventCodeView codes;
    codes.known = true;
    codes.none = 99.0;
    codes.kind_count = 16.0;
    codes.hunt = 7.0;
    codes.season = 3.0;
    codes.vectum = 11.0;
    codes.stage = 0.0;
    codes.period = 5.0;
    codes.dawn = 12.0;
    codes.dusk = 8.0;
    codes.moonrise = 1.0;
    codes.fullmoon = 13.0;
    codes.eclipse = 4.0;
    codes.conj = 9.0;
    codes.conj_great = 2.0;
    codes.antares = 10.0;
    codes.arcturus = 6.0;
    codes.tumbrae = 15.0;
    codes.lunbrex = 14.0;
    codes.ring_day = 20.0;
    codes.ring_lun = 30.0;
    codes.ring_year = 10.0;
    return codes;
}

ArcMockup mockup() {
    ArcMockup out;
    out.day_now = 0.1;
    out.lun_now = 0.2;
    out.year_now = 0.3;
    out.next_in_h = 5.6;
    out.next_in_d = 0.0;  // hud_bridge_arc_next_in_d: 5.6 hours make no whole TUMBRAE
    out.next_kind = GODOT_ARC_KIND_DUSK;
    return out;
}

VivariumClockView clock_of_the_date() {
    VivariumClockView clock;
    clock.known = true;
    clock.period = 4.0;
    clock.year = 3.0;
    clock.epoch = 2.0;
    clock.cycle = 1.0;
    clock.month = 4.0;
    clock.month_day = 12.0;
    clock.hour = 20.0;
    return clock;
}

VivariumArcView arc_of(double seq, double now, double next_kind, double next_in_h) {
    VivariumArcView arc;
    arc.known = true;
    arc.seq = seq;
    arc.day_now = now;
    arc.lun_now = now / 2.0;
    arc.year_now = now / 4.0;
    arc.next_kind = next_kind;
    arc.next_body = 3.0;
    arc.next_in_h = next_in_h;
    return arc;
}

VivariumAlmanacView set_of(double seq, const VivariumEventCodeView& codes) {
    VivariumAlmanacView set;
    set.known = true;
    set.seq = seq;
    set.occurrences.push_back({codes.dusk, codes.ring_day, 0.0, 0.25, 0.0});
    set.occurrences.push_back({codes.fullmoon, codes.ring_lun, 2.0, -0.5, 0.0});
    set.occurrences.push_back({codes.season, codes.ring_year, 1.0, 0.75, 0.5});
    return set;
}

}  // namespace

TEST_CASE("a disc stands at the share of the occurrence minus the share of now, times the half span") {
    CHECK(std::abs(arc_disc_angle(0.5, 0.25, kHalf) - 6.75) < kEpsilon);
    CHECK(std::abs(arc_disc_angle(0.25, 0.25, kHalf)) < kEpsilon);
    CHECK(arc_disc_angle(-1.0, 0.0, kHalf) < 0.0);
    CHECK(std::abs(arc_disc_angle(1.0, 0.0, kHalf) - kHalf) < kEpsilon);
}

TEST_CASE("a ring turns by equal steps while now runs by equal steps, no jump at the fastest pace") {
    const double step = 1.0 / 400.0;  // the share a frame carries at a hundred times, as the owner computes it
    double last = arc_disc_angle(0.5, 0.0, kHalf);
    double first_step = 0.0;
    for (int frame = 1; frame <= 200; ++frame) {
        const double angle = arc_disc_angle(0.5, step * static_cast<double>(frame), kHalf);
        const double moved = last - angle;
        first_step = frame == 1 ? moved : first_step;
        CHECK(moved > 0.0);                          // no frame without a turn, never back
        CHECK(std::abs(moved - first_step) < 1.0e-9);  // no step larger than its neighbour
        CHECK(moved < 1.0);                          // far under the one degree the old 1 Hz jump would be
        last = angle;
    }
}

TEST_CASE("an anchor that moves leaves every sign where it stood") {
    const double anchor_shift = 0.8;  // the owner re-anchors on now: every share and the share of now drop by it
    for (const double share : {-1.0, -0.3, 0.0, 0.4, 0.9}) {
        const double before = arc_disc_angle(share, 0.8, kHalf);
        const double after = arc_disc_angle(share - anchor_shift, 0.8 - anchor_shift, kHalf);
        CHECK(std::abs(before - after) < kEpsilon);
    }
}

TEST_CASE("the ends fade out over the stops of SHA and the middle stands whole") {
    ArcFadeStops stops;
    const double at[GODOT_ARC_FADE_STOPS] = {0.0, 0.1, 0.32, 0.68, 0.86, 1.0};
    const double alpha[GODOT_ARC_FADE_STOPS] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0};
    for (uint32_t stop = 0u; stop < GODOT_ARC_FADE_STOPS; ++stop) {
        stops.at[stop] = at[stop];
        stops.alpha[stop] = alpha[stop];
    }
    CHECK(arc_fade_opacity(stops, 0.0) == 0.0);
    CHECK(arc_fade_opacity(stops, 0.05) == 0.0);  // over the clock: nothing
    CHECK(std::abs(arc_fade_opacity(stops, 0.21) - 0.5) < 1.0e-9);
    CHECK(arc_fade_opacity(stops, 0.5) == 1.0);
    CHECK(std::abs(arc_fade_opacity(stops, 0.77) - 0.5) < 1.0e-9);
    CHECK(arc_fade_opacity(stops, 0.95) == 0.0);  // over the battery: nothing
    CHECK(arc_fade_opacity(stops, -1.0) == 0.0);
    CHECK(arc_fade_opacity(stops, 2.0) == 0.0);
}

TEST_CASE("an event jump finds the code of its word in the codes of the calendar, row REGISTRO of EXT.8") {
    const VivariumEventCodeView codes = calendar_codes();
    double code = -1.0;
    CHECK(arc_kind_code(codes, GODOT_ARC_KIND_FULLMOON, code));
    CHECK(code == codes.fullmoon);
    CHECK(arc_kind_code(codes, GODOT_ARC_KIND_HUNT, code));
    CHECK(code == codes.hunt);
    CHECK(arc_kind_code(codes, GODOT_ARC_KIND_ARCTURUS, code));
    CHECK(code == codes.arcturus);
    CHECK(arc_kind_code(codes, GODOT_ARC_KIND_STAGE, code));
    CHECK(code == codes.stage);  // a code of 0 is a code like any other
    // every kind the table names comes back through its own name
    for (const double kind : {codes.period, codes.dawn, codes.dusk, codes.moonrise, codes.hunt, codes.fullmoon,
                              codes.conj, codes.eclipse, codes.stage, codes.season, codes.vectum, codes.arcturus,
                              codes.antares, codes.conj_great, codes.tumbrae, codes.lunbrex}) {
        const char* name = arc_kind_name(codes, kind);
        REQUIRE(name != nullptr);
        double back = -1.0;
        CHECK(arc_kind_code(codes, name, back));
        CHECK(back == kind);
    }
    // the word "next" is no kind: the view takes next_kind of the arc for it
    code = -1.0;
    CHECK_FALSE(arc_kind_code(codes, GODOT_ARC_EVENT_NEXT, code));
    CHECK(code == -1.0);
    CHECK_FALSE(arc_kind_code(codes, "comet", code));
    CHECK_FALSE(arc_kind_code(codes, nullptr, code));
    VivariumEventCodeView unread;
    CHECK_FALSE(arc_kind_code(unread, GODOT_ARC_KIND_DAWN, code));  // no code before both records stood
}

TEST_CASE("a kind and a ring are named by the codes of the calendar, never by a number of the adapter") {
    const VivariumEventCodeView codes = calendar_codes();
    CHECK(std::string(arc_kind_name(codes, codes.dusk)) == GODOT_ARC_KIND_DUSK);
    CHECK(std::string(arc_kind_name(codes, codes.dawn)) == GODOT_ARC_KIND_DAWN);
    CHECK(std::string(arc_kind_name(codes, codes.moonrise)) == GODOT_ARC_KIND_MOONRISE);
    CHECK(std::string(arc_kind_name(codes, codes.hunt)) == GODOT_ARC_KIND_HUNT);
    CHECK(std::string(arc_kind_name(codes, codes.fullmoon)) == GODOT_ARC_KIND_FULLMOON);
    CHECK(std::string(arc_kind_name(codes, codes.conj)) == GODOT_ARC_KIND_CONJ);
    CHECK(std::string(arc_kind_name(codes, codes.eclipse)) == GODOT_ARC_KIND_ECLIPSE);
    CHECK(std::string(arc_kind_name(codes, codes.stage)) == GODOT_ARC_KIND_STAGE);
    CHECK(std::string(arc_kind_name(codes, codes.season)) == GODOT_ARC_KIND_SEASON);
    CHECK(std::string(arc_kind_name(codes, codes.vectum)) == GODOT_ARC_KIND_VECTUM);
    CHECK(std::string(arc_kind_name(codes, codes.arcturus)) == GODOT_ARC_KIND_ARCTURUS);
    CHECK(std::string(arc_kind_name(codes, codes.antares)) == GODOT_ARC_KIND_ANTARES);
    CHECK(std::string(arc_kind_name(codes, codes.conj_great)) == GODOT_ARC_KIND_CONJ_GREAT);
    CHECK(std::string(arc_kind_name(codes, codes.period)) == GODOT_ARC_KIND_PERIOD);
    CHECK(std::string(arc_kind_name(codes, codes.tumbrae)) == GODOT_ARC_KIND_TUMBRAE);  // "TUM n", A05
    CHECK(std::string(arc_kind_name(codes, codes.lunbrex)) == GODOT_ARC_KIND_LUNBREX);  // "LUN n", A08
    CHECK(arc_kind_name(codes, codes.none) == nullptr);  // no event is no kind
    CHECK(arc_kind_name(codes, 55.0) == nullptr);
    uint32_t ring = 7u;
    CHECK(arc_ring_index(codes, codes.ring_year, ring));
    CHECK(ring == GODOT_ARC_SLOT_YEAR);
    CHECK(arc_ring_index(codes, codes.ring_day, ring));
    CHECK(ring == GODOT_ARC_SLOT_DAY);
    CHECK(arc_ring_index(codes, codes.ring_lun, ring));
    CHECK(ring == GODOT_ARC_SLOT_LUN);
    CHECK_FALSE(arc_ring_index(codes, 55.0, ring));
    VivariumEventCodeView unknown = codes;
    unknown.known = false;
    CHECK(arc_kind_name(unknown, codes.dusk) == nullptr);
    CHECK_FALSE(arc_ring_index(unknown, codes.ring_day, ring));
}

TEST_CASE("while no port carries a record the arc stands whole from the mockup and each port is a gap") {
    SkyArcFrame out;
    VivariumArcView paired;
    const ArcGaps gaps = arc_bridge(VivariumArcView{}, VivariumAlmanacView{}, VivariumEventCodeView{},
                                    clock_of_the_date(), mockup(), paired, out);
    CHECK(gaps.arc);
    CHECK(gaps.almanac);
    CHECK(gaps.codes);
    CHECK_FALSE(gaps.unnamed);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].now == 0.1);
    CHECK(out.rings[GODOT_ARC_SLOT_LUN].now == 0.2);
    CHECK(out.rings[GODOT_ARC_SLOT_YEAR].now == 0.3);
    CHECK(out.next_kind == GODOT_ARC_KIND_DUSK);
    CHECK(out.next_in_h == 5.6);
    CHECK(out.next_in_d == 0.0);
    CHECK(out.seq == 0u);
    for (const ArcRingFrame& ring : out.rings) {
        CHECK(ring.signs.empty());
    }
    CHECK(out.period == 4.0);  // the date is the clock's, whatever the arc ports say
    CHECK(out.month_day == 12.0);
}

TEST_CASE("with the three ports whole the occurrences stand on their rings and the next event is named") {
    const VivariumEventCodeView codes = calendar_codes();
    SkyArcFrame out;
    VivariumArcView paired;
    const ArcGaps gaps = arc_bridge(arc_of(5.0, 0.4, codes.dusk, 3.5), set_of(5.0, codes), codes, clock_of_the_date(),
                                    mockup(), paired, out);
    CHECK_FALSE(gaps.arc);
    CHECK_FALSE(gaps.almanac);
    CHECK_FALSE(gaps.codes);
    CHECK_FALSE(gaps.unnamed);
    CHECK(out.seq == 6u);  // the set's number plus one: the number 0 is the arc without a set
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].now == 0.4);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].signs.size() == 1u);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].signs[0].kind == GODOT_ARC_KIND_DUSK);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].signs[0].share == 0.25);
    CHECK(out.rings[GODOT_ARC_SLOT_LUN].signs.size() == 1u);
    CHECK(out.rings[GODOT_ARC_SLOT_LUN].signs[0].kind == GODOT_ARC_KIND_FULLMOON);
    CHECK(out.rings[GODOT_ARC_SLOT_LUN].signs[0].body == 2.0);
    CHECK(out.rings[GODOT_ARC_SLOT_YEAR].signs.size() == 1u);
    CHECK(out.rings[GODOT_ARC_SLOT_YEAR].signs[0].span == 0.5);
    CHECK(out.next_kind == GODOT_ARC_KIND_DUSK);
    CHECK(out.next_in_h == 3.5);
    CHECK(out.next_in_d == 0.0);
    CHECK(paired.known);
}

TEST_CASE("the whole TUMBRAE of the deadline pass through as the owner counted them, the adapter divides nothing") {
    const VivariumEventCodeView codes = calendar_codes();
    SkyArcFrame out;
    VivariumArcView paired;
    VivariumArcView arc = arc_of(5.0, 0.4, codes.season, 65.0);
    arc.next_in_d = 2.0;  // CAL_ARC_NEXT_IN_D: floor(65 / 30), computed by CalendarArcNextCalcSystem
    arc_bridge(arc, set_of(5.0, codes), codes, clock_of_the_date(), mockup(), paired, out);
    CHECK(out.next_kind == GODOT_ARC_KIND_SEASON);
    CHECK(out.next_in_h == 65.0);
    CHECK(out.next_in_d == 2.0);
    arc.next_in_d = 7.0;  // a count no division of 65 hours gives: the frame carries the owner's word, not its own
    arc_bridge(arc, set_of(5.0, codes), codes, clock_of_the_date(), mockup(), paired, out);
    CHECK(out.next_in_d == 7.0);
}

TEST_CASE("the arc ahead of the set held shows the pair that agreed, never a mixture of two states") {
    const VivariumEventCodeView codes = calendar_codes();
    VivariumArcView paired;
    SkyArcFrame out;
    arc_bridge(arc_of(5.0, 0.4, codes.dusk, 3.5), set_of(5.0, codes), codes, clock_of_the_date(), mockup(), paired, out);
    // The owner re-anchored: the arc carries the number 6 and shares against the new anchor, the set held is still 5.
    const ArcGaps gaps = arc_bridge(arc_of(6.0, 0.05, codes.dusk, 3.4), set_of(5.0, codes), codes, clock_of_the_date(),
                                    mockup(), paired, out);
    CHECK_FALSE(gaps.arc);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].now == 0.4);  // the share against the anchor the signs stand on
    CHECK(out.seq == 6u);                              // the set of the number 5, as before
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].signs.size() == 1u);
    // The set of the number 6 arrives: both are of the new state.
    arc_bridge(arc_of(6.0, 0.05, codes.dusk, 3.4), set_of(6.0, codes), codes, clock_of_the_date(), mockup(), paired, out);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].now == 0.05);
    CHECK(out.seq == 7u);
}

TEST_CASE("a first arc without any set shows its shares of now on rings that hold no sign yet") {
    const VivariumEventCodeView codes = calendar_codes();
    SkyArcFrame out;
    VivariumArcView paired;
    const ArcGaps gaps = arc_bridge(arc_of(1.0, 0.4, codes.dusk, 3.5), VivariumAlmanacView{}, codes, clock_of_the_date(),
                                    mockup(), paired, out);
    CHECK_FALSE(gaps.arc);
    CHECK(gaps.almanac);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].now == 0.4);
    CHECK(out.seq == 0u);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].signs.empty());
}

TEST_CASE("no next event stands as an empty reading, and a kind no code names is a gap") {
    const VivariumEventCodeView codes = calendar_codes();
    SkyArcFrame out;
    VivariumArcView paired;
    arc_bridge(arc_of(5.0, 0.4, codes.none, -1.0), set_of(5.0, codes), codes, clock_of_the_date(), mockup(), paired, out);
    CHECK(out.next_kind.empty());
    arc_bridge(arc_of(5.0, 0.4, codes.dusk, -1.0), set_of(5.0, codes), codes, clock_of_the_date(), mockup(), paired, out);
    CHECK(out.next_kind.empty());  // a deadline below 0 is none
    VivariumAlmanacView odd = set_of(5.0, codes);
    odd.occurrences.push_back({55.0, codes.ring_day, 0.0, 0.1, 0.0});
    const ArcGaps gaps = arc_bridge(arc_of(5.0, 0.4, codes.dusk, 3.5), odd, codes, clock_of_the_date(), mockup(), paired,
                                    out);
    CHECK(gaps.unnamed);
    CHECK(out.rings[GODOT_ARC_SLOT_DAY].signs.size() == 1u);  // the unnamed occurrence is left out, the others stand
}

}  // namespace ase::adp::godot

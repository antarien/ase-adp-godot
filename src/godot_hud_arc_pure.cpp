/**
 * ASE GODOT ADAPTER - HUD ARC, THE PART WITHOUT A GODOT TYPE
 *
 * @file        godot_hud_arc_pure.cpp
 * @brief       The turn of a ring, the fade of its ends and the names of the codes, see godot_hud_arc_pure.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_pure.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>

#include <cstddef>
#include <cstring>

namespace ase::adp::godot {

namespace {

/** One kind of the register: its name and where its code stands in the codes of the calendar. */
struct KindSlot {
    const char* name;
    double VivariumEventCodeView::*code;
};

constexpr KindSlot kKinds[] = {
    {GODOT_ARC_KIND_PERIOD, &VivariumEventCodeView::period},
    {GODOT_ARC_KIND_DAWN, &VivariumEventCodeView::dawn},
    {GODOT_ARC_KIND_DUSK, &VivariumEventCodeView::dusk},
    {GODOT_ARC_KIND_MOONRISE, &VivariumEventCodeView::moonrise},
    {GODOT_ARC_KIND_HUNT, &VivariumEventCodeView::hunt},
    {GODOT_ARC_KIND_FULLMOON, &VivariumEventCodeView::fullmoon},
    {GODOT_ARC_KIND_CONJ, &VivariumEventCodeView::conj},
    {GODOT_ARC_KIND_ECLIPSE, &VivariumEventCodeView::eclipse},
    {GODOT_ARC_KIND_STAGE, &VivariumEventCodeView::stage},
    {GODOT_ARC_KIND_SEASON, &VivariumEventCodeView::season},
    {GODOT_ARC_KIND_VECTUM, &VivariumEventCodeView::vectum},
    {GODOT_ARC_KIND_ARCTURUS, &VivariumEventCodeView::arcturus},
    {GODOT_ARC_KIND_ANTARES, &VivariumEventCodeView::antares},
    {GODOT_ARC_KIND_CONJ_GREAT, &VivariumEventCodeView::conj_great},
    {GODOT_ARC_KIND_TUMBRAE, &VivariumEventCodeView::tumbrae},
    {GODOT_ARC_KIND_LUNBREX, &VivariumEventCodeView::lunbrex}};

constexpr double VivariumEventCodeView::*kRings[GODOT_ARC_RINGS] = {
    &VivariumEventCodeView::ring_day, &VivariumEventCodeView::ring_lun, &VivariumEventCodeView::ring_year};

}  // namespace

double arc_disc_angle(double share, double now, double half_deg) {
    return (share - now) * half_deg;
}

double arc_ring_angle(double degrees) {
    return GODOT_RING_TURN_DEG * static_cast<double>(GODOT_HALF) - degrees;
}

double arc_fade_opacity(const ArcFadeStops& stops, double place) {
    if (place <= stops.at[0]) {
        return stops.alpha[0];
    }
    for (uint32_t stop = 1u; stop < GODOT_ARC_FADE_STOPS; ++stop) {
        if (place <= stops.at[stop]) {
            const double run = stops.at[stop] - stops.at[stop - 1u];
            const double along = run > 0.0 ? (place - stops.at[stop - 1u]) / run : 0.0;
            return stops.alpha[stop - 1u] + (stops.alpha[stop] - stops.alpha[stop - 1u]) * along;
        }
    }
    return stops.alpha[GODOT_ARC_FADE_STOPS - 1u];
}

const char* arc_kind_name(const VivariumEventCodeView& codes, double code) {
    if (!codes.known) {
        return nullptr;
    }
    for (const KindSlot& slot : kKinds) {
        if (codes.*(slot.code) == code) {
            return slot.name;
        }
    }
    return nullptr;
}

bool arc_kind_code(const VivariumEventCodeView& codes, const char* name, double& code) {
    if (!codes.known || name == nullptr) {
        return false;
    }
    for (const KindSlot& slot : kKinds) {
        if (std::strcmp(slot.name, name) == 0) {
            code = codes.*(slot.code);
            return true;
        }
    }
    return false;
}

namespace {

/** The SURA date of the clock record into the frame. */
void fill_date(const VivariumClockView& clock, SkyArcFrame& out) {
    out.period = clock.period;
    out.year = clock.year;
    out.epoch = clock.epoch;
    out.cycle = clock.cycle;
    out.month = clock.month;
    out.month_day = clock.month_day;
    out.hour = clock.hour;
}

/** The occurrences of the set onto their rings; false when one names a kind or ring no code names. */
bool fill_signs(const VivariumAlmanacView& almanac, const VivariumEventCodeView& codes, SkyArcFrame& out) {
    bool named = true;
    for (const VivariumOccurrenceView& occurrence : almanac.occurrences) {
        const char* kind = arc_kind_name(codes, occurrence.kind);
        uint32_t ring = 0u;
        if (kind == nullptr || !arc_ring_index(codes, occurrence.ring, ring)) {
            named = false;
            continue;
        }
        ArcSignFrame sign;
        sign.kind = kind;
        sign.body = occurrence.body;
        sign.share = occurrence.share;
        sign.span = occurrence.span;
        out.rings[ring].signs.push_back(sign);
    }
    return named;
}

/** The next event of `arc` by name; false when its kind is none of the codes. Empty while none is named. */
bool fill_next(const VivariumArcView& arc, const VivariumEventCodeView& codes, SkyArcFrame& out) {
    out.next_body = arc.next_body;
    out.next_in_h = arc.next_in_h;
    out.next_in_d = arc.next_in_d;
    out.next_kind.clear();
    if (!codes.known || arc.next_kind == codes.none || arc.next_in_h < 0.0) {
        return true;
    }
    const char* kind = arc_kind_name(codes, arc.next_kind);
    if (kind == nullptr) {
        return false;
    }
    out.next_kind = kind;
    return true;
}

}  // namespace

ArcGaps arc_bridge(const VivariumArcView& arc, const VivariumAlmanacView& almanac, const VivariumEventCodeView& codes,
                   const VivariumClockView& clock, const ArcMockup& mockup, VivariumArcView& paired, SkyArcFrame& out) {
    ArcGaps gaps;
    gaps.arc = !arc.known;
    gaps.almanac = !almanac.known;
    gaps.codes = !codes.known;
    out = SkyArcFrame{};
    fill_date(clock, out);
    if (!arc.known) {
        out.rings[GODOT_ARC_SLOT_DAY].now = mockup.day_now;
        out.rings[GODOT_ARC_SLOT_LUN].now = mockup.lun_now;
        out.rings[GODOT_ARC_SLOT_YEAR].now = mockup.year_now;
        out.next_kind = mockup.next_kind;
        out.next_in_h = mockup.next_in_h;
        out.next_in_d = mockup.next_in_d;
        return gaps;
    }
    const bool set = almanac.known && codes.known;
    const bool agree = set && almanac.seq == arc.seq;
    if (agree) {
        paired = arc;
    }
    // While the arc's number is not the held set's, the pair that agreed stands: anchor and occurrences of one state.
    const VivariumArcView& shown = !set || agree || !paired.known ? arc : paired;
    out.rings[GODOT_ARC_SLOT_DAY].now = shown.day_now;
    out.rings[GODOT_ARC_SLOT_LUN].now = shown.lun_now;
    out.rings[GODOT_ARC_SLOT_YEAR].now = shown.year_now;
    bool named = fill_next(shown, codes, out);
    if (set && (agree || paired.known)) {
        out.seq = static_cast<uint64_t>(almanac.seq) + 1u;
        named = fill_signs(almanac, codes, out) && named;
    }
    gaps.unnamed = !named;
    return gaps;
}

bool arc_ring_index(const VivariumEventCodeView& codes, double code, uint32_t& index) {
    if (!codes.known) {
        return false;
    }
    for (uint32_t ring = 0u; ring < GODOT_ARC_RINGS; ++ring) {
        if (codes.*(kRings[ring]) == code) {
            index = ring;
            return true;
        }
    }
    return false;
}

}  // namespace ase::adp::godot

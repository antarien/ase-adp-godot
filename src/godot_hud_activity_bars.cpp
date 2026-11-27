/**
 * ASE GODOT ADAPTER - HUD ACTIVITY BARS
 *
 * @file        godot_hud_activity_bars.cpp
 * @brief       The order, the places, the lit action and the hit of the activity bars of the game HUD
 * @description MIG_ASE_SKILL_RSM step B5, see godot_hud_activity_bars.hpp. No Godot type.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @category    ecs/module
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.02.00002 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_activity_bars.hpp>
#include <ase/adp/godot/rsm/types.hpp>

#include <ase/math/math.hpp>

#include <algorithm>
#include <bit>
#include <cmath>

namespace ase::adp::godot {

namespace {

/** The action code `action` as a whole code; false where it is no number, under 0 or past the last code. */
bool action_code(double action, uint32_t& out) {
    if (!std::isfinite(action) || action < 0.0 || action >= static_cast<double>(GODOT_RSM_ACTION_NONE)) {
        return false;
    }
    out = static_cast<uint32_t>(action);
    return true;
}

}  // namespace

void activity_bars_order(const std::vector<ActivityBar>& in, uint32_t limit, std::vector<ActivityBar>& out) {
    out.clear();
    uint32_t code = 0;
    for (const ActivityBar& bar : in) {
        if (action_code(bar.action, code) && std::isfinite(bar.qnt) && std::isfinite(bar.age)) {
            out.push_back(bar);
        }
    }
    // The youngest left: the least age first; of two of the same age the later slice, the higher id.
    std::stable_sort(out.begin(), out.end(), [](const ActivityBar& left, const ActivityBar& right) {
        return left.age < right.age || (left.age == right.age && left.id > right.id);
    });
    if (out.size() > limit) {
        out.resize(limit);
    }
}

void activity_bars_place(const std::vector<ActivityBar>& ordered, const ActivityBarsLayout& layout,
                         const std::vector<uint32_t>& lit, std::vector<ActivityBarPlace>& out) {
    out.clear();
    const double step = layout.w + layout.gap;
    for (size_t index = 0; index < ordered.size(); ++index) {
        const ActivityBar& bar = ordered[index];
        ActivityBarPlace place;
        if (!action_code(bar.action, place.action)) {
            continue;  // activity_bars_order lets no such bar through; a caller that skips it gets no place for it
        }
        place.id = bar.id;
        place.qnt = bar.qnt;
        place.w = layout.w;
        place.h = static_cast<double>(
            ase::math::clamp(static_cast<float>(bar.qnt * layout.unit), 0.0f, static_cast<float>(layout.h)));
        place.x = layout.x + static_cast<double>(index) * step;
        place.y = layout.base - place.h;
        place.lit = std::find(lit.begin(), lit.end(), place.action) != lit.end();
        out.push_back(place);
    }
}

bool activity_bars_strip(const ActivityBarsLayout& layout, size_t count, double& x, double& y, double& w, double& h) {
    if (count == 0u) {
        return false;
    }
    const double row = static_cast<double>(count) * (layout.w + layout.gap) - layout.gap;
    x = layout.x - layout.pad;
    y = layout.base - layout.h - layout.pad;
    w = row + layout.pad + layout.pad;
    h = layout.h + layout.pad + layout.pad;
    return true;
}

bool activity_bars_hit(const std::vector<ActivityBarPlace>& places, const ActivityBarsLayout& layout, double px,
                       double py, size_t& out) {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    if (!activity_bars_strip(layout, places.size(), x, y, w, h) || px < x || px > x + w || py < y || py > y + h) {
        return false;
    }
    // The column under the touch; left of the first bar the first, right of the last bar the last.
    const double step = layout.w + layout.gap;
    const size_t last = places.size() - 1u;
    if (!(step > 0.0) || px <= layout.x) {
        out = 0u;
        return true;
    }
    const double column = (px - layout.x) / step;  // not under 0 here: the cast cuts it down to its column
    out = column >= static_cast<double>(last) ? last : static_cast<size_t>(column);
    return true;
}

double activity_bars_lit_qnt(const std::vector<ActivityBarPlace>& places) {
    double sum = 0.0;
    for (const ActivityBarPlace& place : places) {
        sum += place.lit ? place.qnt : 0.0;
    }
    return sum;
}

uint64_t activity_bars_sign(const std::vector<ActivityBar>& ordered) {
    // FNV-1a over the id and the quantity of each bar: a new slice changes the ids, a grown quantity its bits. The
    // age is left out, it grows in every frame of a running slice and would turn the band to the bars without a deed.
    constexpr uint64_t basis = 1469598103934665603ull;
    constexpr uint64_t prime = 1099511628211ull;
    uint64_t sign = basis;
    for (const ActivityBar& bar : ordered) {
        const uint64_t qnt_bits = std::bit_cast<uint64_t>(bar.qnt);
        for (const uint64_t word : {static_cast<uint64_t>(bar.id), qnt_bits}) {
            for (uint32_t shift = 0u; shift < 64u; shift += 8u) {
                sign ^= (word >> shift) & 0xffull;
                sign *= prime;
            }
        }
    }
    return sign;
}

void activity_band_step(const ActivityBandInput& in, double hold_seconds, ActivityBandState& state) {
    // THE FIRST SET IS NO CHANGE: the bars the port held when the HUD started are the past, not a deed of now.
    const bool changed = state.seen && in.sign != state.sign;
    const bool quiet = in.wall - state.changed >= hold_seconds;  // the bars stood still for the hold before this frame
    state.seen = true;
    state.sign = in.sign;
    if (in.count == 0u) {
        state.shows = GODOT_RSM_BAND_SLOTS;  // no bar: nothing to show but the slots, the switch has nothing to turn to
        state.pinned = false;
        return;
    }
    if (in.toggled) {
        state.shows = state.shows == GODOT_RSM_BAND_BARS ? GODOT_RSM_BAND_SLOTS : GODOT_RSM_BAND_BARS;
        state.pinned = true;
        state.changed = in.wall;
        return;
    }
    if (in.needs_slots) {
        state.shows = GODOT_RSM_BAND_SLOTS;  // the inventory in use wins over the bars, pinned or not
        state.pinned = false;
        return;
    }
    if (changed) {
        // A change turns the band to the bars; pinned slots yield only to a change after a quiet hold, since a
        // skilling that never stopped would else take back the band the player just chose. Pinned bars stay pinned.
        if (!state.pinned) {
            state.shows = GODOT_RSM_BAND_BARS;
        } else if (state.shows == GODOT_RSM_BAND_SLOTS && quiet) {
            state.shows = GODOT_RSM_BAND_BARS;
            state.pinned = false;
        }
        state.changed = in.wall;
        return;
    }
    if (!state.pinned && state.shows == GODOT_RSM_BAND_BARS && quiet) {
        state.shows = GODOT_RSM_BAND_SLOTS;  // the bars stood still for the hold: the slots return
    }
}

}  // namespace ase::adp::godot

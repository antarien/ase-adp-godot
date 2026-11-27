/**
 * ASE GODOT ADAPTER - HUD DEBUG RSM TABLE
 *
 * @file        godot_hud_debug_rsm.cpp
 * @brief       The clock, the column order and the held set of the RSM table of the debug layer
 * @description MIG_ASE_SKILL_RSM step B4, see godot_hud_debug_rsm.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @category    ecs/module
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_debug_rsm.hpp>
#include <ase/adp/godot/rsm/types.hpp>

#include <ase/math/math.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ase::adp::godot {

namespace {

/** The two digits of the prototype's "00" format; more where the number needs them. */
std::string two_digits(uint64_t value) {
    return value < 10u ? "0" + std::to_string(value) : std::to_string(value);
}

/** The share `value` of `limit`, 0..1; false where either is no number or the limit is no length. */
bool share_of(double value, double limit, double& out) {
    if (!std::isfinite(value) || !std::isfinite(limit) || !(limit > 0.0)) {
        return false;
    }
    out = static_cast<double>(ase::math::clamp(static_cast<float>(value / limit), 0.0f, 1.0f));
    return true;
}

}  // namespace

std::string debug_rsm_clock(double seconds) {
    if (!std::isfinite(seconds) || seconds >= static_cast<double>(std::numeric_limits<int64_t>::max())) {
        return "-";
    }
    const uint64_t total = seconds > 0.0 ? static_cast<uint64_t>(seconds) : 0u;
    const uint64_t minutes = total / GODOT_RSM_SECONDS_PER_MINUTE;
    const uint64_t hours = minutes / GODOT_RSM_MINUTES_PER_HOUR;
    return two_digits(hours) + ":" + two_digits(minutes % GODOT_RSM_MINUTES_PER_HOUR) + ":" +
           two_digits(total % GODOT_RSM_SECONDS_PER_MINUTE);
}

std::string debug_rsm_whole(double value) {
    const double reach = static_cast<double>(std::numeric_limits<int64_t>::max());
    if (!std::isfinite(value) || value >= reach || value <= -reach) {
        return "-";
    }
    return std::to_string(static_cast<int64_t>(value));
}

double debug_rsm_left(double qnt, double qnt_max) {
    if (!std::isfinite(qnt) || !std::isfinite(qnt_max)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return qnt_max > qnt ? qnt_max - qnt : 0.0;
}

double rsm_slice_length(double qnt, double qnt_left) {
    if (!std::isfinite(qnt) || !std::isfinite(qnt_left)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return qnt + qnt_left;
}

double rsm_grace_length(double dcd, double dcd_shr) {
    if (!std::isfinite(dcd) || !std::isfinite(dcd_shr) || !(dcd_shr > 0.0)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return dcd / dcd_shr;
}

double rsm_slice_back(double head, double slot, double places) {
    if (!std::isfinite(head) || !std::isfinite(slot) || !(places >= 1.0) || !std::isfinite(places)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const int64_t ring = static_cast<int64_t>(places);
    return static_cast<double>((static_cast<int64_t>(head) - static_cast<int64_t>(slot) + ring) % ring);
}

double rsm_slice_age(double qnt, double back, double qnt_max) {
    if (!std::isfinite(qnt) || !std::isfinite(back) || !std::isfinite(qnt_max)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return qnt + back * qnt_max;
}

void debug_rsm_columns(const std::vector<DebugRsmEntry>& in, uint32_t limit, std::vector<DebugRsmEntry>& out) {
    out = in;
    std::stable_sort(out.begin(), out.end(),
                     [](const DebugRsmEntry& left, const DebugRsmEntry& right) { return left.id > right.id; });
    if (out.size() > limit) {
        out.resize(limit);
    }
}

DebugRsmShares debug_rsm_shares(const std::vector<DebugRsmEntry>& in) {
    DebugRsmShares out;
    for (const DebugRsmEntry& entry : in) {
        double share = 0.0;
        if (share_of(entry.dcd_shr, 1.0, share) && share > out.dcd) {
            out.dcd = share;  // the server's share of the grace left
        }
        if (share_of(entry.acd, entry.acd_max, share) && share > out.acd) {
            out.acd = share;
        }
        if (share_of(entry.qnt_shr, 1.0, share) && share > out.qnt) {
            out.qnt = share;  // the server's share of the running slice
        }
        if (share_of(entry.tuc, entry.tuc_max, share) && share > out.tuc) {
            out.tuc = share;
        }
    }
    return out;
}

void debug_rsm_shown(const std::vector<DebugRsmEntry>& in, uint32_t limit, std::vector<DebugRsmEntry>& held,
                     std::vector<DebugRsmEntry>& out) {
    if (!in.empty()) {
        debug_rsm_columns(in, limit, held);  // the set of this frame, kept for a frame whose port names no entry
    }
    out = held;
}

uint32_t debug_view_choose(uint32_t current, uint32_t hit) {
    return hit < GODOT_HUD_DEBUG_VIEWS ? hit : current;
}

bool debug_rsm_action_slot(double action, uint32_t& out) {
    if (!std::isfinite(action) || action < static_cast<double>(GODOT_RSM_ACTION_FIRST) ||
        action > static_cast<double>(GODOT_RSM_ACTION_LAST)) {
        return false;
    }
    out = static_cast<uint32_t>(action) - GODOT_RSM_ACTION_FIRST;
    return true;
}

}  // namespace ase::adp::godot

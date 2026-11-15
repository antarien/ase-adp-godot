/**
 * ASE GODOT ADAPTER - HUD UI REFRESH
 *
 * @file        godot_hud_ui_refresh.cpp
 * @brief       Binds the HUD's refresh period and the longest step of its clock and answers when to sample again
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, inventory row T10, see godot_hud_ui_refresh.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_refresh.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

bool UiRefresh::bind(const UiTheme& theme, std::string& failed) {
    double ms = 0.0;
    if (!theme.measure("hud_refresh_ms", ms) || !(ms > 0.0)) {
        failed = "hud_refresh_ms";
        return false;
    }
    if (!theme.measure("hud_frame_dt_max", step_max_) || !(step_max_ > 0.0)) {
        failed = "hud_frame_dt_max";
        return false;
    }
    period_ = ms / GODOT_MS_PER_SECOND;
    since_ = 0.0;
    due_ = true;
    return true;
}

double UiRefresh::step(double elapsed) const {
    // Math.min(0.05, (now - last) / 1000) of the mockup's tick; a clock that ran backwards moves nothing
    return ase::math::min(ase::math::max(elapsed, 0.0), step_max_);
}

void UiRefresh::advance(double step, bool held) {
    // acc += dt; if (acc > 0.25 || S.holding || S.sysHold || S.mode === 'prune') { acc = 0; render(false); }
    since_ += step;
    if (since_ > period_ || held) {
        since_ = 0.0;
        due_ = true;
    }
}

bool UiRefresh::take() {
    const bool due = due_;
    due_ = false;
    return due;
}

}  // namespace ase::adp::godot

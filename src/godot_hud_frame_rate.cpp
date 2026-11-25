/**
 * ASE GODOT ADAPTER - HUD FRAME RATE
 *
 * @file        godot_hud_frame_rate.cpp
 * @brief       Counts the frames of a window over the real time they took and judges them against the floor
 * @description Operator order 2026-11-24, see godot_hud_frame_rate.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_frame_rate.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <ase/math/math.hpp>

#include <cmath>

namespace ase::adp::godot {

bool HudFrameRate::configure(double window_s, double frame_budget_ms) {
    if (!std::isfinite(window_s) || !(window_s > 0.0) || !std::isfinite(frame_budget_ms) || !(frame_budget_ms > 0.0)) {
        return false;
    }
    const int32_t floor_rate =
        static_cast<int32_t>(ase::math::round(static_cast<float>(GODOT_MS_PER_SECOND / frame_budget_ms)));
    if (floor_rate <= 0) {
        return false;
    }
    window_s_ = window_s;
    floor_ = floor_rate;
    restart();
    return true;
}

bool HudFrameRate::frame(double elapsed_s) {
    if (!(window_s_ > 0.0) || !std::isfinite(elapsed_s) || !(elapsed_s > 0.0)) {
        return false;
    }
    if (fresh_) {
        // the frame the count starts on: its distance reaches back before the count
        fresh_ = false;
        return false;
    }
    span_s_ += elapsed_s;
    ++frames_;
    if (span_s_ < window_s_) {
        return false;
    }
    rate_ = static_cast<double>(frames_) / span_s_;
    const int32_t shown = static_cast<int32_t>(ase::math::round(static_cast<float>(rate_)));
    const bool under = shown < floor_;
    changed_ = !known_ || shown != shown_;
    crossed_ = !known_ || under != under_;
    shown_ = shown;
    under_ = under;
    known_ = true;
    span_s_ = 0.0;
    frames_ = 0;
    return true;
}

void HudFrameRate::restart() {
    span_s_ = 0.0;
    frames_ = 0;
    fresh_ = true;
}

}  // namespace ase::adp::godot

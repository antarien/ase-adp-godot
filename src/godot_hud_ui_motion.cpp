/**
 * ASE GODOT ADAPTER - HUD UI MOTION
 *
 * @file        godot_hud_ui_motion.cpp
 * @brief       Binds a duration and a CSS easing of SHA's design tokens and answers the eased progress
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_motion.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/hud/motion/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ase::adp::godot {

namespace {

/** The x or y of a CSS cubic bezier from (0,0) over the controls `a`, `b` to (1,1) at parameter `t`. */
double bezier_at(double a, double b, double t) {
    const double rest = 1.0 - t;
    return 3.0 * rest * rest * t * a + 3.0 * rest * t * t * b + t * t * t;
}

/** The four control numbers of a keyword or a cubic-bezier() text; false for any other easing. */
bool read_easing(const ::godot::String& text, double& x1, double& y1, double& x2, double& y2) {
    const ::godot::String bare = text.strip_edges();
    if (bare == GODOT_EASE_LINEAR) {
        x1 = GODOT_EASE_LINEAR_X1; y1 = GODOT_EASE_LINEAR_Y1; x2 = GODOT_EASE_LINEAR_X2; y2 = GODOT_EASE_LINEAR_Y2;
        return true;
    }
    if (bare == GODOT_EASE) {
        x1 = GODOT_EASE_DEFAULT_X1; y1 = GODOT_EASE_DEFAULT_Y1; x2 = GODOT_EASE_DEFAULT_X2; y2 = GODOT_EASE_DEFAULT_Y2;
        return true;
    }
    if (bare == GODOT_EASE_IN) {
        x1 = GODOT_EASE_IN_X1; y1 = GODOT_EASE_IN_Y1; x2 = GODOT_EASE_IN_X2; y2 = GODOT_EASE_IN_Y2;
        return true;
    }
    if (bare == GODOT_EASE_OUT) {
        x1 = GODOT_EASE_OUT_X1; y1 = GODOT_EASE_OUT_Y1; x2 = GODOT_EASE_OUT_X2; y2 = GODOT_EASE_OUT_Y2;
        return true;
    }
    if (bare == GODOT_EASE_IN_OUT) {
        x1 = GODOT_EASE_IN_OUT_X1; y1 = GODOT_EASE_IN_OUT_Y1; x2 = GODOT_EASE_IN_OUT_X2; y2 = GODOT_EASE_IN_OUT_Y2;
        return true;
    }
    const ::godot::String head = GODOT_EASE_BEZIER;
    const ::godot::String tail = GODOT_EASE_BEZIER_END;
    if (!bare.begins_with(head) || !bare.ends_with(tail)) {
        return false;
    }
    const ::godot::String inner = bare.substr(head.length(), bare.length() - head.length() - tail.length());
    const ::godot::PackedStringArray parts = inner.split(GODOT_EASE_BEZIER_SEPARATOR);
    if (parts.size() != GODOT_EASE_BEZIER_POINTS) {
        return false;
    }
    double numbers[GODOT_EASE_BEZIER_POINTS] = {};
    for (int64_t index = 0; index < GODOT_EASE_BEZIER_POINTS; ++index) {
        const ::godot::String part = parts[index].strip_edges();
        if (!part.is_valid_float()) {
            return false;
        }
        numbers[index] = part.to_float();
    }
    // CSS: the x of both controls lies in 0..1, so that time runs forward along the curve.
    if (numbers[0] < 0.0 || numbers[0] > 1.0 || numbers[2] < 0.0 || numbers[2] > 1.0) {
        return false;
    }
    x1 = numbers[0]; y1 = numbers[1]; x2 = numbers[2]; y2 = numbers[3];
    return true;
}

}  // namespace

bool UiMotion::bind(const UiTheme& theme, const std::string& name, std::string& failed) {
    const std::string duration = name + "_ms";
    const std::string easing = name + "_ease";
    double ms = 0.0;
    if (!theme.measure(duration, ms) || ms <= 0.0) {
        failed = duration;
        return false;
    }
    ::godot::String text;
    if (!theme.token_text(easing, text) || !read_easing(text, x1_, y1_, x2_, y2_)) {
        failed = easing;
        return false;
    }
    seconds_ = ms / GODOT_MS_PER_SECOND;
    return true;
}

double UiMotion::ease(double time) const {
    // The parameter t whose x is the time share; x rises with t because both control x lie in 0..1.
    double low = 0.0;
    double high = 1.0;
    double t = time;
    for (int32_t step = 0; step < GODOT_EASE_SOLVE_STEPS; ++step) {
        const double x = bezier_at(x1_, x2_, t);
        if (ase::math::abs(x - time) < GODOT_EASE_SOLVE_EPSILON) {
            break;
        }
        if (x < time) {
            low = t;
        } else {
            high = t;
        }
        t = (low + high) * GODOT_HALF;
    }
    return bezier_at(y1_, y2_, t);
}

double UiMotion::progress(double elapsed, double delay, bool still) const {
    if (still) {
        return 1.0;
    }
    if (seconds_ <= 0.0 || elapsed <= delay) {
        return 0.0;
    }
    const double share = (elapsed - delay) / seconds_;
    if (share >= 1.0) {
        return 1.0;
    }
    return ease(share);
}

double UiMotion::cycle(double elapsed) const {
    if (seconds_ <= 0.0 || elapsed <= 0.0) {
        return 0.0;
    }
    const double turns = elapsed / seconds_;
    // turns is positive, so dropping its fraction by truncation is its floor, in double precision.
    return ease(turns - static_cast<double>(static_cast<int64_t>(turns)));
}

}  // namespace ase::adp::godot

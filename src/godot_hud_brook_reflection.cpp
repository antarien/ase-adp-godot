/**
 * ASE GODOT ADAPTER - HUD BROOK REFLECTION
 *
 * @file        godot_hud_brook_reflection.cpp
 * @brief       The sky the brook reflects and the strokes of its current, derived for the 3D scene
 * @description See godot_hud_brook_reflection.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_brook_reflection.hpp>

#include <godot_cpp/core/math.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool BrookReflection::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"brook_reflect_share", &reflect_share_},
             {"brook_tone_far_share", &tone_far_share_},
             {"brook_tone_near_share", &tone_near_share_},
             {"brook_streak_count", &streak_count_},
             {"brook_streak_gap", &streak_gap_},
             {"brook_streak_length", &streak_length_},
             {"brook_streak_width", &streak_width_},
             {"brook_streak_speed", &streak_speed_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    // the reflect share adds the sky as light over the murky green own colour, a tone share turns the reflected sky toward the
    // water's own tone; each from none to all of it
    for (const std::pair<const char*, double>& share : std::initializer_list<std::pair<const char*, double>>{
             {"brook_reflect_share", reflect_share_},
             {"brook_tone_far_share", tone_far_share_},
             {"brook_tone_near_share", tone_near_share_}}) {
        if (!(share.second >= 0.0 && share.second <= 1.0)) {
            failed = std::string(share.first) + " must lie in 0..1";
            return false;
        }
    }
    // the strokes are a whole number, their gap and length shares of the course, their width and speed not negative
    if (!(streak_count_ >= 0.0 && ::godot::Math::floor(streak_count_) == streak_count_)) {
        failed = "brook_streak_count must be a whole number, not negative";
        return false;
    }
    if (!(streak_gap_ > 0.0 && streak_gap_ <= 1.0 && streak_length_ >= 0.0 && streak_length_ <= streak_gap_)) {
        failed = "brook_streak_gap must lie in (0, 1] and brook_streak_length in 0..brook_streak_gap";
        return false;
    }
    if (!(streak_width_ >= 0.0 && streak_speed_ >= 0.0)) {
        failed = "brook_streak_width and brook_streak_speed must not be negative";
        return false;
    }
    for (const std::pair<const char*, ::godot::Color*>& colour :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"BROOK_TONE_FAR", &tone_far_}, {"BROOK_TONE_NEAR", &tone_near_}, {"BROOK_STREAK", &streak_}}) {
        if (!theme.color(colour.first, *colour.second)) {
            failed = colour.first;
            return false;
        }
    }
    return true;
}

bool BrookReflection::shade(const BrookReflectionFrame& now, BrookReflectionShade& out, std::string& failed) const {
    out = BrookReflectionShade{};
    if (now.stream == nullptr) {
        failed = "BrookReflection: the frame carries no stream";
        return false;
    }
    if (!(now.frame_pixels > 0.0)) {
        failed = "BrookReflection: the frame carries no pixels per mockup unit";
        return false;
    }
    const double dx = now.stream->end_x - now.stream->x;
    const double dz = now.stream->end_z - now.stream->z;
    if (!(dx * dx + dz * dz > 0.0)) {
        failed = "BrookReflection: the stream record " + std::to_string(now.stream->object_id) +
                 " has a bed without length";
        return false;
    }
    // m.html drawWater: the far stop mix(sky horizon, tone far, 0.45), the near stop mix(sky above, tone near, 0.5)
    const ::godot::Color far = now.horizon.lerp(tone_far_, static_cast<float>(tone_far_share_));
    const ::godot::Color near = now.above.lerp(tone_near_, static_cast<float>(tone_near_share_));
    out.reflect_share = static_cast<float>(reflect_share_);
    out.far = ::godot::Color(far.r, far.g, far.b, 1.0f);
    out.near = ::godot::Color(near.r, near.g, near.b, 1.0f);
    out.streak = streak_;
    out.streak_count = static_cast<float>(streak_count_);
    out.streak_gap = static_cast<float>(streak_gap_);
    out.streak_length = static_cast<float>(streak_length_);
    out.streak_speed = static_cast<float>(streak_speed_);
    out.streak_px = static_cast<float>(streak_width_ * now.frame_pixels);
    out.start_x = static_cast<float>(now.stream->x);
    out.start_z = static_cast<float>(now.stream->z);
    out.end_x = static_cast<float>(now.stream->end_x);
    out.end_z = static_cast<float>(now.stream->end_z);
    // the strokes stand on the axis of the winding bed, which the shader finds as the middle of the wet strip; the
    // half width bounds how far it looks for the rims
    out.half_m = static_cast<float>(now.stream->half_m);
    out.shown = true;
    return true;
}

}  // namespace ase::adp::godot

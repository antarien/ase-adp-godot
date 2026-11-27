/**
 * ASE GODOT ADAPTER - HUD UI ARC FADE
 *
 * @file        godot_hud_ui_arc_fade.cpp
 * @brief       UiArcFade: the opacity of a point of the sky arc from its x place, see godot_hud_ui_arc_fade.hpp
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

#include <ase/adp/godot/godot_hud_ui_arc_fade.hpp>

#include <cstdint>

namespace ase::adp::godot {

bool UiArcFade::bind(const UiTheme& theme, std::string& failed) {
    if (!theme.measure("hud_frame_width", frame_width_) || !(frame_width_ > 0.0)) {
        failed = "hud_frame_width";
        return false;
    }
    for (uint32_t stop = 0u; stop < GODOT_ARC_FADE_STOPS; ++stop) {
        const std::string place = "hud_arc_fade_" + std::to_string(stop);
        const std::string alpha = place + "_alpha";
        if (!theme.measure(place, stops_.at[stop])) {
            failed = place;
            return false;
        }
        if (!theme.measure(alpha, stops_.alpha[stop])) {
            failed = alpha;
            return false;
        }
    }
    return true;
}

double UiArcFade::opacity(double x) const {
    return arc_fade_opacity(stops_, x / frame_width_);
}

::godot::Color UiArcFade::faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

}  // namespace ase::adp::godot

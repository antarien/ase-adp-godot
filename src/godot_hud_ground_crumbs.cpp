/**
 * ASE GODOT ADAPTER - HUD GROUND CRUMBS
 *
 * @file        godot_hud_ground_crumbs.cpp
 * @brief       The dark crumbs of broken-up soil where a loosen verb went into the ground
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD row N08, see godot_hud_ground_crumbs.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0456 (flora.patch.v1 loose: the bridge lays the grid), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ground_crumbs.hpp>
#include <ase/adp/godot/godot_hud_ui_rng.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool GroundCrumbs::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_crumb_floor", &floor_},
             {"hud_crumb_count", &count_},
             {"hud_crumb_seed_row", &seed_row_},
             {"hud_crumb_rx_share", &rx_share_},
             {"hud_crumb_rx", &rx_},
             {"hud_crumb_ry_share", &ry_share_},
             {"hud_crumb_ry", &ry_},
             {"hud_crumb_steps", &steps_},
             {"hud_crumb_lit_r", &lit_r_},
             {"hud_crumb_lit_g", &lit_g_},
             {"hud_crumb_lit_b", &lit_b_},
             {"hud_crumb_dark_r", &dark_r_},
             {"hud_crumb_dark_g", &dark_g_},
             {"hud_crumb_dark_b", &dark_b_},
             {"hud_crumb_alpha", &alpha_},
             {"hud_crumb_alpha_loose", &alpha_loose_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(steps_ >= 3.0)) {
        failed = "hud_crumb_steps: at least 3";
        return false;
    }
    return true;
}

void GroundCrumbs::draw(UiPen& canvas, const GroundCrumbsFrame& now) const {
    if (now.anchor == nullptr || now.ground == nullptr || now.loose == nullptr || now.columns <= 0 ||
        now.rows <= 0 || static_cast<int64_t>(now.loose->size()) != now.columns * now.rows) {
        return;
    }
    // (46 L + 12) | 0 per channel: the mockup floors each channel to a whole 0..255.
    const double channel_max = 255.0;
    const auto channel = [&](double lit, double dark) {
        return static_cast<float>(::godot::Math::floor(lit * now.light + dark) / channel_max);
    };
    const float red = channel(lit_r_, dark_r_);
    const float green = channel(lit_g_, dark_g_);
    const float blue = channel(lit_b_, dark_b_);
    const auto steps = static_cast<int64_t>(steps_);
    for (int64_t row = 0; row < now.rows; ++row) {
        for (int64_t column = 0; column < now.columns; ++column) {
            const double share = (*now.loose)[static_cast<size_t>(row * now.columns + column)];
            if (!(share > floor_)) {
                continue;
            }
            const double x = now.west_x + static_cast<double>(column) * now.step_x_m;
            const double z = now.south_z + static_cast<double>(row) * now.step_z_m;
            // The drawn step east at this point gives the crumbs their size, as the mockup's kz(z) does.
            ::godot::Vector2 at;
            ::godot::Vector2 east;
            if (!now.ground->surface(*now.anchor, x, z, 0.0, at) ||
                !now.ground->surface(*now.anchor, x + now.step_x_m, z, 0.0, east)) {
                continue;
            }
            const double step = static_cast<double>((east - at).length());
            const double rx = rx_share_ * step + rx_;
            const double ry = ry_share_ * step + ry_;
            const ::godot::Color tint(red, green, blue, static_cast<float>(alpha_ + alpha_loose_ * share));
            auto state = static_cast<uint32_t>(static_cast<double>(row) * seed_row_ + static_cast<double>(column));
            const auto crumbs = static_cast<int64_t>(1.0 + ::godot::Math::round(share * count_));
            for (int64_t crumb = 0; crumb < crumbs; ++crumb) {
                const double moved_x = x + (ui_rng_draw(state) - 0.5) * now.step_x_m;
                const double moved_z = z + (ui_rng_draw(state) - 0.5) * now.step_z_m;
                ::godot::Vector2 centre;
                if (!now.ground->surface(*now.anchor, moved_x, moved_z, 0.0, centre)) {
                    continue;
                }
                ::godot::PackedVector2Array outline;
                for (int64_t point = 0; point < steps; ++point) {
                    const double turn = Math_TAU * static_cast<double>(point) / static_cast<double>(steps);
                    outline.push_back(centre + ::godot::Vector2(static_cast<float>(rx * ::godot::Math::cos(turn)),
                                                                static_cast<float>(ry * ::godot::Math::sin(turn))));
                }
                canvas.draw_colored_polygon(outline, tint);
            }
        }
    }
}

}  // namespace ase::adp::godot

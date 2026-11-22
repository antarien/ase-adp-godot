/**
 * ASE GODOT ADAPTER - VIVARIUM HUD LOOSE BRIDGE
 *
 * @file        godot_vivarium_hud_loose.cpp
 * @brief       VivariumLooseBridge - the mockup's LO on the ground square, see the header
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD bridge section, see godot_vivarium_hud_loose.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0456 (flora.patch.v1 loose), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_hud_loose.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/core/math.hpp>

#include <cstring>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool VivariumLooseBridge::load(const UiTheme& theme, const VivariumGroundSpec& square, double start_share,
                               std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_bridge_loose_cell_x_m", &cell_x_},
             {"hud_bridge_loose_cell_z_m", &cell_z_},
             {"hud_bridge_loose_gain", &gain_},
             {"hud_bridge_loose_root_cost", &root_cost_},
             {"hud_bridge_loose_settle_rain", &settle_rain_},
             {"hud_bridge_loose_settle_drizzle", &settle_drizzle_},
             {"hud_bridge_loose_patch_m", &patch_m_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(cell_x_ > 0.0) || !(cell_z_ > 0.0) || !(patch_m_ > 0.0) || !(square.half_m > 0.0)) {
        failed = "hud_bridge_loose_cell_x_m, _cell_z_m, _patch_m and the ground square's half edge: above 0";
        return false;
    }
    if (!(start_share >= 0.0 && start_share <= 1.0)) {
        failed = "hud_bridge_patch_loose_share: a share 0..1";
        return false;
    }
    start_ = start_share;
    west_x_ = square.x - square.half_m;
    south_z_ = square.z - square.half_m;
    columns_ = static_cast<int64_t>(::godot::Math::floor(2.0 * square.half_m / cell_x_)) + 1;
    rows_ = static_cast<int64_t>(::godot::Math::floor(2.0 * square.half_m / cell_z_)) + 1;
    loose_.assign(static_cast<size_t>(columns_ * rows_), start_);
    hour_ = -1.0;
    return true;
}

void VivariumLooseBridge::loosen(double x, double z, double brush_m) {
    if (!(brush_m > 0.0)) {
        return;
    }
    for (int64_t row = 0; row < rows_; ++row) {
        const double dz = south_z_ + static_cast<double>(row) * cell_z_ - z;
        for (int64_t column = 0; column < columns_; ++column) {
            const double dx = west_x_ + static_cast<double>(column) * cell_x_ - x;
            const double reach = ::godot::Math::sqrt(dx * dx + dz * dz) / brush_m;
            if (reach > 1.0) {
                continue;
            }
            double& point = loose_[static_cast<size_t>(row * columns_ + column)];
            point = ase::math::min(1.0, point + gain_ * (1.0 - reach * reach));
        }
    }
}

void VivariumLooseBridge::settle(double game_hour, const char* weather) {
    const double before = hour_;
    hour_ = game_hour;
    if (before < 0.0 || !(game_hour > before) || weather == nullptr) {
        return;
    }
    double rate = 0.0;
    if (std::strcmp(weather, GODOT_HUD_WEATHER_RAIN) == 0) {
        rate = settle_rain_;
    } else if (std::strcmp(weather, GODOT_HUD_WEATHER_DRIZZLE) == 0) {
        rate = settle_drizzle_;
    }
    if (!(rate > 0.0)) {
        return;
    }
    const double kept = ase::math::max(0.0, 1.0 - (game_hour - before) * rate);
    for (double& point : loose_) {
        point *= kept;
    }
}

double VivariumLooseBridge::share(double x, double z) const {
    double sum = 0.0;
    int64_t count = 0;
    for (int64_t row = 0; row < rows_; ++row) {
        const double dz = south_z_ + static_cast<double>(row) * cell_z_ - z;
        for (int64_t column = 0; column < columns_; ++column) {
            const double dx = west_x_ + static_cast<double>(column) * cell_x_ - x;
            if (dx * dx + dz * dz < patch_m_ * patch_m_) {
                sum += loose_[static_cast<size_t>(row * columns_ + column)];
                ++count;
            }
        }
    }
    return count > 0 ? sum / static_cast<double>(count) : start_;
}

double VivariumLooseBridge::root_cost() const {
    return root_cost_;
}

void VivariumLooseBridge::frame(GroundCrumbsFrame& out) const {
    out.loose = &loose_;
    out.columns = columns_;
    out.rows = rows_;
    out.west_x = west_x_;
    out.south_z = south_z_;
    out.step_x_m = cell_x_;
    out.step_z_m = cell_z_;
}

}  // namespace ase::adp::godot

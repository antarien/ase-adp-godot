/**
 * ASE GODOT ADAPTER - HUD CLOUD BANK
 *
 * @file        godot_hud_cloud_bank.cpp
 * @brief       The cloud banks of an overcast sky, laid in frame units for the sky of the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N04, see godot_hud_cloud_bank.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0449 (flora.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_cloud_bank.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool CloudBank::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"sky_cloud_banks", &banks_},
             {"sky_cloud_step_x", &step_x_},
             {"sky_cloud_speed", &speed_},
             {"sky_cloud_period", &period_},
             {"sky_cloud_offset_x", &offset_x_},
             {"sky_cloud_y", &y_},
             {"sky_cloud_rows", &rows_},
             {"sky_cloud_row_step", &row_step_},
             {"sky_cloud_rx", &rx_},
             {"sky_cloud_ry", &ry_},
             {"sky_cloud_reach", &veil_.reach},
             {"sky_cloud_soft", &veil_.soft},
             {"sky_cloud_grain", &veil_.grain},
             {"sky_cloud_lift", &veil_.lift},
             {"sky_cloud_shade", &veil_.shade}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"WEATHER_CLOUD", &cloud_},
                                                                       {"WEATHER_CLOUD_SNOW", &snow_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(banks_ >= 0.0) || !(period_ > 0.0) || !(rows_ >= 1.0)) {
        failed = "sky_cloud_banks must not be negative, sky_cloud_period must be positive, sky_cloud_rows at least 1";
        return false;
    }
    if (!(veil_.reach >= 1.0) || !(veil_.soft > 0.0) || !(veil_.grain > 0.0)) {
        failed = "sky_cloud_reach must be at least 1, sky_cloud_soft and sky_cloud_grain positive";
        return false;
    }
    return true;
}

void CloudBank::lay(const CloudBankFrame& now, std::vector<CloudBankEllipse>& out) const {
    out.clear();
    if (now.kind == nullptr) {
        return;
    }
    // The mockup's overcast(): rain, drizzle, snow, fog and a clouded sky.
    bool overcast = false;
    for (const char* kind : {GODOT_HUD_WEATHER_RAIN, GODOT_HUD_WEATHER_DRIZZLE, GODOT_HUD_WEATHER_SNOW,
                             GODOT_HUD_WEATHER_FOG, GODOT_HUD_WEATHER_CLOUDY}) {
        overcast = overcast || std::strcmp(now.kind, kind) == 0;
    }
    if (!overcast) {
        return;
    }
    const ::godot::Color colour = std::strcmp(now.kind, GODOT_HUD_WEATHER_SNOW) == 0 ? snow_ : cloud_;
    const uint32_t banks = static_cast<uint32_t>(banks_);
    const uint32_t rows = static_cast<uint32_t>(rows_);
    for (uint32_t index = 0u; index < banks; ++index) {
        CloudBankEllipse bank;
        // the run goes left as well under a wind from the right: the remainder folds back into [0, period)
        const float run = static_cast<float>(static_cast<double>(index) * step_x_ + now.time * speed_);
        float place = ase::math::fmod(run, static_cast<float>(period_));
        if (place < 0.0f) {
            place += static_cast<float>(period_);
        }
        bank.x = static_cast<double>(place) + offset_x_;
        bank.y = y_ + static_cast<double>(index % rows) * row_step_;
        bank.rx = rx_;
        bank.ry = ry_;
        bank.color = colour;
        out.push_back(bank);
    }
}

}  // namespace ase::adp::godot

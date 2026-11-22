#pragma once

/**
 * ASE GODOT ADAPTER - HUD CLOUD BANK
 *
 * @file        godot_hud_cloud_bank.hpp
 * @brief       The cloud banks of an overcast sky, laid in frame units for the sky of the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N04, block CloudBank (the cloud banks of the
 *              mockup's drawSky, drawn after the sun and before the hills and the tree).
 *
 *              THE KINDS. Rain, drizzle, snow, fog and a clouded sky (the mockup's overcast()) lay the banks; a
 *              clear or partly clouded sky lays none.
 *
 *              THE BANKS. sky_cloud_banks ellipses. Bank i stands at x = ((i times sky_cloud_step_x plus the
 *              drifted time times sky_cloud_speed) folded into [0, sky_cloud_period)) plus sky_cloud_offset_x and at
 *              y = sky_cloud_y plus (i modulo sky_cloud_rows) times sky_cloud_row_step, with the half axes
 *              sky_cloud_rx and sky_cloud_ry, in WEATHER_CLOUD_SNOW under snow and in WEATHER_CLOUD otherwise;
 *              the opacity rides in the colour.
 *
 *              THE PLACE IN THE PICTURE. The mockup draws the banks behind the hills and the tree; on the HUD's
 *              canvas they would lie over the crown. So the block only lays them, in the units of the middle
 *              band's frame, and light_plant_clouds (godot_plant_clouds.hpp) stands each in the sky of the 3D
 *              scene on the camera's ray through its point of the picture.
 *
 *              BRIDGE GAP N0449. The kind of weather is state of the weather simulation on sky.weather.v1; the
 *              block reads it from its frame, which the bridge fills from hud_bridge_weather_* of SHA while the port
 *              files no record (godot_vivarium_hud_bridge.cpp, bridge_weather). The block never asks whether a
 *              port delivered.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0449 (sky.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/variant/color.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the sky is this frame. */
struct CloudBankFrame {
    const char* kind = nullptr;   // the weather key of the port or of the bridge (GODOT_HUD_WEATHER_*), N0449
    double time = 0.0;            // seconds the banks have drifted, each weighed by the wind's pace across the picture (cloud_drift_share), negative for a run to the left
};

/** One cloud bank, an ellipse in the units of the middle band's frame. */
struct CloudBankEllipse {
    double x = 0.0;               // centre, frame units
    double y = 0.0;               // centre, frame units
    double rx = 0.0;              // half axis across, frame units
    double ry = 0.0;              // half axis up, frame units
    ::godot::Color color;         // WEATHER_CLOUD or WEATHER_CLOUD_SNOW, its opacity in the alpha
};

/** How a bank's veil is drawn in the sky of the scene, in half axes of the bank. */
struct CloudBankVeil {
    double reach = 0.0;           // sky_cloud_reach, quad half extent
    double soft = 0.0;            // sky_cloud_soft, feathered share of the half axis
    double grain = 0.0;           // sky_cloud_grain, noise cells across the bank
    double lift = 0.0;            // sky_cloud_lift, noise reach into the rim
    double shade = 0.0;           // sky_cloud_shade, darkening at the foot
};

/** The cloud banks of an overcast sky. */
class CloudBank {
public:
    /** Binds the measures and the colours; false with the missing name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Lays the banks of `now` into `out`, none under an open sky. */
    void lay(const CloudBankFrame& now, std::vector<CloudBankEllipse>& out) const;

    /** The veil every bank is drawn with (sky_cloud_reach, _soft, _grain, _lift, _shade). */
    const CloudBankVeil& veil() const { return veil_; }

private:
    CloudBankVeil veil_;
    double banks_ = 0.0;          // sky_cloud_banks
    double step_x_ = 0.0;         // sky_cloud_step_x
    double speed_ = 0.0;          // sky_cloud_speed
    double period_ = 0.0;         // sky_cloud_period
    double offset_x_ = 0.0;       // sky_cloud_offset_x
    double y_ = 0.0;              // sky_cloud_y
    double rows_ = 0.0;           // sky_cloud_rows
    double row_step_ = 0.0;       // sky_cloud_row_step
    double rx_ = 0.0;             // sky_cloud_rx
    double ry_ = 0.0;             // sky_cloud_ry

    ::godot::Color cloud_;        // WEATHER_CLOUD
    ::godot::Color snow_;         // WEATHER_CLOUD_SNOW
};

}  // namespace ase::adp::godot

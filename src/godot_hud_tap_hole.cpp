/**
 * ASE GODOT ADAPTER - HUD TAP HOLE
 *
 * @file        godot_hud_tap_hole.cpp
 * @brief       The tap hole's drawing: spout, vessel and drop from the frame, every measure from SHA
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_tap_hole.hpp. The mockup's lines
 *              (m.html 473 to 475): by = TY(), sxp = TX + 2.5, syp = by - max(10, treeH() * 0.07); the spout from
 *              (sxp, syp) to (sxp + 5, syp + 1); the cocho at (sxp + 5 - vw * kk / 2, by + 1 - vh * kk) scaled
 *              by kk = 0.42; the drop at (sxp + 5.2, syp + 1.5 + ph * (by - syp - 6)), ph = (t * 1.4) mod 1.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101609 - tap birch sap
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0457 (flora.sap.v1), N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_tap_hole.hpp>

#include <ase/math/math.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool TapHole::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_world_tap_share", &share_},
             {"hud_world_tap_min", &lowest_},
             {"hud_world_tap_spout_dx", &spout_dx_},
             {"hud_world_tap_spout_len", &spout_len_},
             {"hud_world_tap_spout_rise", &spout_rise_},
             {"hud_world_tap_spout_line", &spout_line_},
             {"hud_world_tap_vessel_k", &vessel_k_},
             {"hud_world_tap_vessel_dy", &vessel_dy_},
             {"hud_world_tap_drop_dx", &drop_dx_},
             {"hud_world_tap_drop_dy", &drop_dy_},
             {"hud_world_tap_drop_radius", &drop_radius_},
             {"hud_world_tap_drop_rate", &drop_rate_},
             {"hud_world_tap_drop_gap", &drop_gap_},
             {"hud_world_tap_dx", &pulse_dx_},
             {"hud_world_tap_pulse_radius", &pulse_radius_},
             {"hud_line", &pulse_line_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"MATERIAL_PITH", &pith_},
                                                                       {"SAP_DROP", &drop_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.tone("flow", flow_)) {
        failed = "DOMAIN_FLOW";
        return false;
    }
    return ring_.bind(theme, failed) && vessel_.bind(theme, failed);
}

void TapHole::draw_pulse(UiPen& canvas, const ::godot::Transform2D& frame, const TapHoleFrame& now, double elapsed,
                         bool still) const {
    if (!now.placed) {
        return;  // the birch stands off the screen: there is no hole to show
    }
    const double foot = static_cast<double>(now.trunk.y);
    const ::godot::Vector2 centre(static_cast<float>(static_cast<double>(now.trunk.x) + pulse_dx_),
                                  static_cast<float>(foot - ase::math::max(lowest_, now.tree_height * share_)));
    canvas.draw_set_transform_matrix(frame);
    ring_.pulse(canvas, centre, pulse_radius_, flow_, pulse_line_, elapsed, still);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
}

void TapHole::draw(UiPen& canvas, const ::godot::Transform2D& frame, const TapHoleFrame& now) const {
    if (!now.running || !now.placed) {
        return;  // no tap runs, or the birch stands off the screen: the mockup draws nothing
    }
    const double foot = static_cast<double>(now.trunk.y);
    const double spout_x = static_cast<double>(now.trunk.x) + spout_dx_;
    const double spout_y = foot - ase::math::max(lowest_, now.tree_height * share_);
    const double mouth_x = spout_x + spout_len_;

    // The elder spout: from the hole out of the trunk, a little down to its mouth.
    canvas.draw_set_transform_matrix(frame);
    canvas.draw_line(::godot::Vector2(static_cast<float>(spout_x), static_cast<float>(spout_y)),
                     ::godot::Vector2(static_cast<float>(mouth_x), static_cast<float>(spout_y + spout_rise_)), pith_,
                     static_cast<float>(spout_line_), true);

    // The bark vessel at the foot under the mouth, its sap body at what it holds.
    if (now.vessel) {
        const UiItemSprite& drawing = vessel_.item().sprite();
        const ::godot::Vector2 corner(static_cast<float>(mouth_x - drawing.view_width() * vessel_k_ / 2.0),
                                      static_cast<float>(foot + vessel_dy_ - drawing.view_height() * vessel_k_));
        vessel_.draw_scaled(canvas, frame, corner, vessel_k_, UiItemSpritePlace(), now.carry);
        canvas.draw_set_transform_matrix(frame);
    }

    // The drop: from the mouth down to the gap above the foot, over and over.
    const double phase = static_cast<double>(ase::math::fmod(static_cast<float>(now.clock * drop_rate_), 1.0f));
    const double fall = foot - spout_y - drop_gap_;
    canvas.draw_circle(::godot::Vector2(static_cast<float>(mouth_x + drop_dx_),
                                        static_cast<float>(spout_y + drop_dy_ + phase * fall)),
                       static_cast<float>(drop_radius_), drop_);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
}

}  // namespace ase::adp::godot

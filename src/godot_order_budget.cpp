/**
 * ASE GODOT ADAPTER - VISIBLE ORDERS BY FRAME TIME IMPLEMENTATION
 *
 * @file        godot_order_budget.cpp
 * @brief       The line between tube and instance, moved one order at a time by the frame time
 * @description See godot_order_budget.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-10
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_order_budget.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

bool order_budget_valid(const OrderBudget& budget) {
    return std::isfinite(budget.frame_budget_ms) && budget.frame_budget_ms > 0.0 && budget.headroom > 0.0 &&
           budget.headroom < 1.0 && budget.calm_frames > 0u && budget.over_frames > 0u &&
           budget.shown <= budget.ceiling;
}

bool order_budget_sample(OrderBudget& budget, double frame_ms) {
    if (!std::isfinite(frame_ms) || frame_ms < 0.0) {
        return false;
    }
    if (frame_ms > budget.frame_budget_ms) {
        budget.calm_run = 0u;
        budget.over_run += 1u;
        if (budget.over_run >= budget.over_frames && budget.shown > 0u) {
            budget.over_run = 0u;
            budget.shown -= 1u;
            return true;
        }
        return false;
    }
    if (frame_ms < budget.frame_budget_ms * budget.headroom) {
        budget.over_run = 0u;
        budget.calm_run += 1u;
        if (budget.calm_run >= budget.calm_frames && budget.shown < budget.ceiling) {
            budget.calm_run = 0u;
            budget.shown += 1u;
            return true;
        }
        return false;
    }
    budget.calm_run = 0u;
    budget.over_run = 0u;
    return false;
}

}  // namespace ase::adp::godot

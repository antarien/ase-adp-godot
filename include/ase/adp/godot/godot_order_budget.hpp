#pragma once

/**
 * ASE GODOT ADAPTER - VISIBLE ORDERS BY FRAME TIME
 *
 * @file        godot_order_budget.hpp
 * @brief       How many branching orders the view draws on the fine cylinder, following the measured
 *              frame time
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 4 and 03.6 (V8): the engine simulates
 *              every organ up to its measured limit; how finely the VIEW draws an order follows from the
 *              frame time. Every internode is an instance of a cylinder template with the 3x4
 *              ase-render-data computed (03.11 step 2); the orders up to the shown one take the fine
 *              template (GODOT_TMPL_FINE), the finer orders the coarse one (GODOT_TMPL_CORS), both laid
 *              by ase-render-data on the template ports (usecase:101011 tessellate the organ templates). The
 *              view never changes what the engine computes - this budget only moves the line between
 *              the two templates.
 *
 *              THE RULE: a frame longer than the budget counts as over, a frame below the budget's
 *              headroom share counts as calm, anything between resets both runs. `over_frames` over
 *              frames in a row take one order back; `calm_frames` calm frames in a row give one more,
 *              up to the ceiling. One spike moves nothing, a lasting load moves one order at a time.
 *              The numbers come from the start configuration ("view" in vivarium_start.json): the
 *              frame budget is the operator's frame-time target, never a constant here.
 *
 *              NO GODOT TYPE, so the host tests drive it with any series of frame times.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-18
 * @version     00.00.01.00003
 */

#include <cstdint>

namespace ase::adp::godot {

/** The budget's settings (start configuration) and its running state. */
struct OrderBudget {
    double   frame_budget_ms = 0.0;  // frame time the view may take, milliseconds
    double   headroom = 0.0;         // share of the budget below which a frame is calm, 0..1
    uint32_t ceiling = 0;            // most orders ever drawn on the fine cylinder
    uint32_t calm_frames = 0;        // calm frames in a row that give one order more
    uint32_t over_frames = 0;        // over frames in a row that take one order back
    uint32_t shown = 0;              // the highest order drawn on the fine cylinder now
    uint32_t calm_run = 0;           // calm frames in a row so far
    uint32_t over_run = 0;           // over frames in a row so far
};

/**
 * true when the settings can drive a budget: a positive frame budget, a headroom inside (0, 1), at
 * least one frame for each run, and a start order not above the ceiling.
 */
[[nodiscard]] bool order_budget_valid(const OrderBudget& budget);

/**
 * One frame's measured time in milliseconds. Answers true when the shown order changed - the view
 * sorts its internodes onto the two templates again then. A frame time that is no finite, non-negative
 * number moves nothing.
 */
bool order_budget_sample(OrderBudget& budget, double frame_ms);

}  // namespace ase::adp::godot

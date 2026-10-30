/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW WATERING
 *
 * @file        godot_vivarium_water.cpp
 * @brief       VivariumView - Regar: when a pour makes sense, the pour to the patch port, and its arrival
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1, sequence V3: the Plant Keeper waters the selected
 *              patch. The pour goes to the host as an irrigation of the patch; the view shows the soil
 *              water once the patch port carries it - never an optimistic number - and logs once whether
 *              the pour arrived (follow_water, every frame after the advance). The tap that leads here
 *              is godot_vivarium_input.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-26
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/math/math.hpp>

#include <cstdint>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

bool VivariumView::can_water() const {
    // No selection, a dead stand, a pour still on its way, a broken host: the button is drawn
    // disabled and a press only logs that it was.
    if (selected_place_ == GODOT_NO_PLACE || water_pending_place_ != GODOT_NO_PLACE || !resources_.running() ||
        !failure_text_.is_empty()) {
        return false;
    }
    const VivariumPatchView* patch = resources_.get_patch_at(selected_place_);
    return patch != nullptr && patch->condition != GODOT_COND_DEAD;
}

void VivariumView::water() {
    if (!can_water()) {
        note("water " + measure(selected_place_) + " result=disabled");
        return;
    }
    const VivariumPatchView* patch = resources_.get_patch_at(selected_place_);
    const double soil_before = patch->soil_mm;
    const HostStatus status = resources_.irrigate(patch->object_id);
    note("water " + measure(selected_place_) + " amount_mm=" +
         ::godot::String::num(start_.irrigate_amount_mm, GODOT_LOG_DECIMALS) + " result=" +
         GodotHostResourceManager::status_name(status));
    if (status == ase::kernel::HostStatusOk) {
        uint64_t steps = 0u;
        (void)resources_.schedule_runs(GODOT_LOG_STEP_SCHEDULE, &steps);
        water_pending_place_ = selected_place_;
        water_pending_from_ = soil_before;
        water_pending_steps_ = steps;
    }
    node_->queue_redraw();
}

void VivariumView::follow_water() {
    // Input → plugin (Reception) → ase-hydro (Reception) → balance and publish (Dissemination) →
    // snapshot: the pour has arrived once the patch's soil water rose by GODOT_WATER_ARRIVED_SHARE of
    // what the pour can add - its amount, or the room up to the field capacity if that is smaller -
    // and only then is the result logged (L3). ANY RISE IS NOT THE POUR (run 20261025-phase03-v39):
    // one step after the press the soil stood at 20.004 mm against 20.003 from drift, the line said
    // applied, and the 25 mm arrived one step later at 45.003. It may also be refused on its way - by
    // ase-hydro above the field capacity - and then no rise ever comes: after
    // GODOT_WATER_WAIT_STEPS Dissemination runs that is a line of its own.
    if (water_pending_place_ == GODOT_NO_PLACE) {
        return;
    }
    const VivariumPatchView* patch = resources_.get_patch_at(water_pending_place_);
    if (patch == nullptr) {
        water_pending_place_ = GODOT_NO_PLACE;
        return;
    }
    const double expected_mm = ase::math::min(start_.irrigate_amount_mm, patch->capacity_mm - water_pending_from_);
    if (expected_mm > 0.0 && patch->soil_mm >= water_pending_from_ + GODOT_WATER_ARRIVED_SHARE * expected_mm) {
        note("water applied " + measure(water_pending_place_) + " soil_mm_before=" +
             ::godot::String::num(water_pending_from_, GODOT_LOG_DECIMALS));
        water_pending_place_ = GODOT_NO_PLACE;
        return;
    }
    uint64_t steps = 0u;
    if (resources_.schedule_runs(GODOT_LOG_STEP_SCHEDULE, &steps) == ase::kernel::HostStatusOk &&
        steps >= water_pending_steps_ + GODOT_WATER_WAIT_STEPS) {
        note("water not applied " + measure(water_pending_place_) + " soil_mm_before=" +
             ::godot::String::num(water_pending_from_, GODOT_LOG_DECIMALS));
        water_pending_place_ = GODOT_NO_PLACE;
    }
}

}  // namespace ase::adp::godot

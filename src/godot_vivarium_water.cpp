/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW WATERING
 *
 * @file        godot_vivarium_water.cpp
 * @brief       VivariumView - Regar: when a pour makes sense, and the pour to the patch port
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1, sequence V3: the Plant Keeper waters the selected
 *              patch. The pour goes to the host as an irrigation of the patch; the view shows the soil
 *              water once the patch port carries it - never an optimistic number. The tap that leads here
 *              is godot_vivarium_input.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>

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

}  // namespace ase::adp::godot

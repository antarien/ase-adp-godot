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
 *              THE AMOUNT IS THE CALLER'S. The button Regar pours irrigate_amount_mm; the HUD's can pours once,
 *              at its release, the litres it held times hud_bridge_hand_mm_per_l (m.html 1014-1015, the
 *              mockup's S.soil += mm and it.fill -= round(mm / 25)). The arrival is measured against that
 *              amount, and its toast says REGOU {mm} MM, or REGOU {mm} MM · REGADOR VAZIO where the can
 *              emptied with this pour; the can's fill is the carry bridge's while flora.carry.v1 carries
 *              none (N0451).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1: the can's fill for toast.watered_empty), carrier N0448
 * @created     2026-10-18
 * @modified    2026-11-12
 * @version     00.00.01.00005
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/math/math.hpp>

#include <cstdint>
#include <string>
#include <vector>

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

bool VivariumView::water(double amount_mm, bool emptied) {
    if (!can_water() || !(amount_mm > 0.0)) {
        note("water " + measure(selected_place_) + " amount_mm=" + ::godot::String::num(amount_mm, GODOT_LOG_DECIMALS) +
             " result=disabled");
        return false;
    }
    const VivariumPatchView* patch = resources_.get_patch_at(selected_place_);
    const double soil_before = patch->soil_mm;
    const HostStatus status = resources_.irrigate(patch->object_id, amount_mm);
    note("water " + measure(selected_place_) + " amount_mm=" + ::godot::String::num(amount_mm, GODOT_LOG_DECIMALS) +
         " result=" + GodotHostResourceManager::status_name(status));
    const bool taken = status == ase::kernel::HostStatusOk;
    if (taken) {
        uint64_t steps = 0u;
        (void)resources_.schedule_runs(GODOT_LOG_STEP_SCHEDULE, &steps);
        water_pending_place_ = selected_place_;
        water_pending_from_ = soil_before;
        water_pending_steps_ = steps;
        water_pending_mm_ = amount_mm;
        water_pending_empty_ = emptied;
    }
    node_->queue_redraw();
    return taken;
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
    const double expected_mm = ase::math::min(water_pending_mm_, patch->capacity_mm - water_pending_from_);
    if (expected_mm > 0.0 && patch->soil_mm >= water_pending_from_ + GODOT_WATER_ARRIVED_SHARE * expected_mm) {
        note("water applied " + measure(water_pending_place_) + " soil_mm_before=" +
             ::godot::String::num(water_pending_from_, GODOT_LOG_DECIMALS));
        water_pending_place_ = GODOT_NO_PLACE;
        // The HUD's toast says what arrived, as the port shows it: the rise of the soil water, in whole
        // millimetres (Math.round(mm)), once per pour. Where the can emptied with it (the carry bridge's fill,
        // N0451) the toast is toast.watered_empty, as the mockup's `REGOU ${mm} MM · REGADOR VAZIO`. The values
        // stand named before the test: a list built inside the short-circuit is a temporary the compiler sees
        // dangle (-Wdangling-pointer).
        std::string failed;
        const std::vector<HudPortValue> rise{HudPortValue{"mm", hud_decimals(patch->soil_mm - water_pending_from_, 0)}};
        const char* key = water_pending_empty_ ? VivariumHud::TOAST_WATERED_EMPTY : VivariumHud::TOAST_WATERED;
        water_pending_empty_ = false;
        if (hud_live() && !hud_.toast(key, rise, VivariumHud::COLOUR_WATER, failed)) {
            fail_hud(failed);
        }
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

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW LEVELLING
 *
 * @file        godot_vivarium_level.cpp
 * @brief       VivariumView - Nivelar: when levelling makes sense, and the request to the level port
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 point 2, register D1598: the player levels the ground
 *              of the selected patch with a tap of the finger on the field Nivelar. The request goes to the
 *              host as a level of the patch; ase-terrain levels its disc, and the view shows the new ground
 *              once the terrain port carries it - never an optimistic shape. The tap that leads here is
 *              godot_vivarium_input.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_015
 * @category    ecs/module
 * @usecase     usecase:5939 - adapt area terrain
 * @actor       Game Areal Plate (AREAL)
 * @created     2026-10-24
 * @modified    2026-10-24
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

bool VivariumView::can_level() const {
    // No selection, no patch on it yet, a broken host: the field is drawn disabled and a tap only logs
    // that it was. A dead stand can be levelled: the ground is not the plant's.
    if (selected_place_ == GODOT_NO_PLACE || !resources_.running() || !failure_text_.is_empty()) {
        return false;
    }
    return resources_.get_patch_at(selected_place_) != nullptr;
}

void VivariumView::level() {
    if (!can_level()) {
        note("level " + measure(selected_place_) + " result=disabled");
        return;
    }
    const VivariumPatchView* patch = resources_.get_patch_at(selected_place_);
    const HostStatus status = resources_.level(patch->object_id);
    note("level " + measure(selected_place_) + " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

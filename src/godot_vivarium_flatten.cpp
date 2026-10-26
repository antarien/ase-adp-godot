/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FLATTENING
 *
 * @file        godot_vivarium_flatten.cpp
 * @brief       VivariumView - Aplanar: the flatten mode and the disc the player levels where he taps
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8, operator order of 2026-10-25 ("wo kann ich da das
 *              Gebiet flatten"): the player levels the ground at a place of his choosing with the mouse or
 *              the finger. In the flatten mode a tap on the ground sends a disc of radius view.flatten_m
 *              around the terrain sample under the tap to the flatten port; ase-pl-flora lays its body,
 *              ase-terrain levels it to the height of its centre, and the view shows the new ground once
 *              the terrain port carries it - never an optimistic shape. The ground shader paints the disc as
 *              bare soil like a levelled patch (fill_ground). Nivelar stays the level of the selected
 *              patch (godot_vivarium_level.cpp); the tap that leads here is godot_vivarium_input.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_015
 * @category    ecs/module
 * @usecase     usecase:5939 - adapt area terrain
 * @actor       Game Areal Plate (AREAL)
 * @created     2026-10-25
 * @modified    2026-10-25
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

bool VivariumView::can_flatten() const {
    // A running host and a ground to tap on.
    return resources_.running() && failure_text_.is_empty() && scene_ != nullptr &&
           !resources_.get_terrain().empty();
}

void VivariumView::toggle_flattening() {
    if (!flattening_ && !can_flatten()) {
        note("flatten mode result=disabled");
        return;
    }
    flattening_ = !flattening_;
    // one mode at a time: a tap in the scene either cuts, lays a bed or levels a disc
    pruning_ = false;
    bedding_ = false;
    bed_started_ = false;
    note(::godot::String("flatten mode ") + (flattening_ ? "on" : "off"));
    node_->queue_redraw();
}

void VivariumView::flatten(const ::godot::Vector2& at) {
    float x = 0.0f;
    float z = 0.0f;
    float nearest_px = -1.0f;
    if (!pick_ground(at, x, z, nearest_px)) {
        note("flatten at " + ::godot::String::num(at.x, 0) + "," + ::godot::String::num(at.y, 0) +
             " result=none nearest_px=" + ::godot::String::num(nearest_px, 1));
        return;
    }
    const HostStatus status = resources_.flatten(x, z, flatten_m_);
    if (status == ase::kernel::HostStatusOk) {
        // the height is the ground's at the centre once ase-terrain levelled it (fill_ground)
        player_flats_.push_back(PlantGroundDisc{x, z, flatten_m_, 0.0f});
    }
    note("flatten x=" + ::godot::String::num(x, 2) + " z=" + ::godot::String::num(z, 2) +
         " radius=" + ::godot::String::num(flatten_m_, 2) + " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW BED
 *
 * @file        godot_vivarium_bed.cpp
 * @brief       VivariumView - Cauce: the bed mode, the ground under the finger and the bed the player lays
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8, operator order of 2026-10-25 ("wie er das Flussbett
 *              entsprechend formen kann"): the player shapes the ground with the mouse or the finger. In
 *              the bed mode the first tap on the ground marks where a bed starts, the second where it
 *              ends, and the bed goes to the host as one more brook, the lay request the configured
 *              brooks take at the start: the width, depth, base flow and catchment of the first configured
 *              brook, the two ends the player's. ase-terrain carves it, ase-hydro lets its water run, and
 *              the view shows the new ground once the terrain port carries it - never an optimistic shape.
 *              The ground shader marks the bed like a configured one (fill_ground). The tap that leads
 *              here is godot_vivarium_input.cpp.
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

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

bool VivariumView::can_bed() const {
    // A running host, a scene to tap into and a configured brook whose width and water the bed takes.
    return resources_.running() && failure_text_.is_empty() && scene_ != nullptr && !start_.brooks.empty() &&
           !resources_.get_terrain().empty();
}

void VivariumView::toggle_bedding() {
    if (!bedding_ && !can_bed()) {
        note("bed mode result=disabled");
        return;
    }
    bedding_ = !bedding_;
    bed_started_ = false;
    // one mode at a time: a tap in the scene either cuts, lays a bed or levels a disc
    pruning_ = false;
    flattening_ = false;
    note(::godot::String("bed mode ") + (bedding_ ? "on" : "off"));
    node_->queue_redraw();
}

bool VivariumView::pick_ground(const ::godot::Vector2& at, float& x, float& z, float& nearest_px) const {
    // THE GROUND UNDER THE FINGER. Every terrain sample goes through the current camera onto the surface;
    // the nearest within pick_px_ is hit. The view computes no geometry of its own: the place of a sample
    // is the one ase-terrain laid on the terrain port, the same numbers the ground is drawn with.
    nearest_px = -1.0f;
    if (scene_ == nullptr) {
        return false;
    }
    ::godot::Camera3D* camera = node_->get_viewport()->get_camera_3d();
    if (camera == nullptr) {
        return false;
    }
    bool hit = false;
    float hit_px = pick_px_;
    for (const VivariumTerrainView& sample : resources_.get_terrain()) {
        const ::godot::Vector3 place(static_cast<float>(sample.x), static_cast<float>(sample.height),
                                     static_cast<float>(sample.z));
        if (camera->is_position_behind(place)) {
            continue;
        }
        const float px = camera->unproject_position(place).distance_to(at);
        if (nearest_px < 0.0f || px < nearest_px) {
            nearest_px = px;
        }
        if (px <= hit_px) {
            hit_px = px;
            hit = true;
            x = static_cast<float>(sample.x);
            z = static_cast<float>(sample.z);
        }
    }
    return hit;
}

void VivariumView::bed(const ::godot::Vector2& at) {
    float x = 0.0f;
    float z = 0.0f;
    float nearest_px = -1.0f;
    if (!pick_ground(at, x, z, nearest_px)) {
        note("bed at " + ::godot::String::num(at.x, 0) + "," + ::godot::String::num(at.y, 0) +
             " result=none nearest_px=" + ::godot::String::num(nearest_px, 1));
        return;
    }
    if (!bed_started_) {
        bed_started_ = true;
        bed_x_ = x;
        bed_z_ = z;
        note("bed start x=" + ::godot::String::num(x, 2) + " z=" + ::godot::String::num(z, 2));
        node_->queue_redraw();
        return;
    }
    bed_started_ = false;
    // the bed of the player: the two ends tapped, width, depth and water of the first configured brook
    VivariumBrookSpec brook = start_.brooks.front();
    brook.x = bed_x_;
    brook.z = bed_z_;
    brook.end_x = x;
    brook.end_z = z;
    const HostStatus status = resources_.lay_brook(brook);
    if (status == ase::kernel::HostStatusOk) {
        player_beds_.push_back(brook);
    }
    note("bed lay from x=" + ::godot::String::num(brook.x, 2) + " z=" + ::godot::String::num(brook.z, 2) +
         " to x=" + ::godot::String::num(brook.end_x, 2) + " z=" + ::godot::String::num(brook.end_z, 2) +
         " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

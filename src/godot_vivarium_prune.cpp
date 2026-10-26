/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW PRUNING
 *
 * @file        godot_vivarium_prune.cpp
 * @brief       VivariumView - Podar: the cutting mode, the internode under the finger and the cut
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.10 step 9, sequence V11: the Plant Keeper cuts a
 *              branch. The cut goes to the segment port of the plugin; which branch falls is
 *              ase-phytomer's, and the view shows the shoot without it once the organ port lays it out
 *              so. The tap that leads here is godot_vivarium_input.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100365 - prune a branch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-25
 * @version     00.00.01.00003
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <unordered_map>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

bool VivariumView::can_prune() const {
    // A running host and an internode on the port: without one there is nothing to cut.
    return resources_.running() && failure_text_.is_empty() && scene_ != nullptr &&
           !resources_.get_organs(GODOT_ORGAN_SEGMENT).empty();
}

void VivariumView::toggle_pruning() {
    if (!pruning_ && !can_prune()) {
        note("prune mode result=disabled");
        return;
    }
    pruning_ = !pruning_;
    // one mode at a time: a tap in the scene either cuts, lays a bed or levels a disc
    bedding_ = false;
    bed_started_ = false;
    flattening_ = false;
    note(::godot::String("prune mode ") + (pruning_ ? "on" : "off"));
    node_->queue_redraw();
}

uint32_t VivariumView::pick_segment(const ::godot::Vector2& at, uint32_t& candidates, float& nearest_px) const {
    // THE INTERNODE UNDER THE FINGER. The middle of every internode of the focused plant goes through
    // the current camera onto the surface; the nearest within pick_px_ is hit. The view computes no
    // geometry of its own: the middle is the instance transform ase-render-data computed for the
    // organ, applied to the middle of the unit tube, beside the stem base the patch record carries -
    // the same numbers the scene draws the internode with. Every internode in front of the camera is
    // counted and the closest distance kept, so a miss says whether there was nothing to hit or the
    // tap lay too far from it.
    candidates = 0u;
    nearest_px = -1.0f;
    if (scene_ == nullptr) {
        return GODOT_NO_OBJECT;
    }
    ::godot::Camera3D* camera = node_->get_viewport()->get_camera_3d();
    if (camera == nullptr) {
        return GODOT_NO_OBJECT;
    }
    const uint32_t focus = selected_place_ != GODOT_NO_PLACE ? selected_place_ : start_.patches.front().place;
    const VivariumPatchView* focused = resources_.get_patch_at(focus);
    if (focused == nullptr) {
        return GODOT_NO_OBJECT;
    }
    const auto origin = scene_origins_.find(focused->object_id);
    if (origin == scene_origins_.end()) {
        return GODOT_NO_OBJECT;
    }
    std::unordered_map<uint32_t, uint32_t> body_of;
    for (const VivariumOrganView& organ : resources_.get_organs(GODOT_ORGAN_SEGMENT)) {
        body_of[organ.object_id] = organ.body;
    }
    uint32_t hit = GODOT_NO_OBJECT;
    float hit_px = pick_px_;
    for (const VivariumInstanceView& instance : resources_.get_instances(GODOT_ORGAN_SEGMENT)) {
        const auto body = body_of.find(instance.object_id);
        if (body == body_of.end() || body->second != focused->object_id) {
            continue;
        }
        // rows: x, y, z axis and origin per row (GODOT_INSTANCE_VALUES); the tube runs along y
        const ::godot::Vector3 middle(origin->second.x + instance.rows[3] + GODOT_TUBE_MID * instance.rows[1],
                                      origin->second.y + instance.rows[7] + GODOT_TUBE_MID * instance.rows[5],
                                      origin->second.z + instance.rows[11] + GODOT_TUBE_MID * instance.rows[9]);
        if (camera->is_position_behind(middle)) {
            continue;
        }
        const float px = camera->unproject_position(middle).distance_to(at);
        candidates += 1u;
        if (nearest_px < 0.0f || px < nearest_px) {
            nearest_px = px;
        }
        if (px <= hit_px) {
            hit_px = px;
            hit = instance.object_id;
        }
    }
    return hit;
}

void VivariumView::prune(const ::godot::Vector2& at) {
    // The cut goes to the plugin; which branch falls is ase-phytomer's, and the view shows the shoot
    // without it once the organ port lays it out so - no optimistic removal here.
    uint32_t candidates = 0u;
    float nearest_px = -1.0f;
    const uint32_t organ = pick_segment(at, candidates, nearest_px);
    if (organ == GODOT_NO_OBJECT) {
        note("prune at " + ::godot::String::num(at.x, 0) + "," + ::godot::String::num(at.y, 0) +
             " result=none candidates=" + ::godot::String::num_uint64(candidates) +
             " nearest_px=" + ::godot::String::num(nearest_px, 1));
        return;
    }
    const HostStatus status = resources_.prune(organ);
    note("prune organ=" + ::godot::String::num_uint64(organ) + " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

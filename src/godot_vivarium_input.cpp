/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW INPUT
 *
 * @file        godot_vivarium_input.cpp
 * @brief       VivariumView - one press per physical tap, the order of the fields it hits, the selection
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.4 and PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1: the tap of
 *              the Plant Keeper. A tap is matched against the rectangles as the last drawn frame placed
 *              them (placed, godot_vivarium_draw.cpp), so it lands where the button was seen. A selection
 *              is a frame, no change to the simulation. Regar leads to godot_vivarium_water.cpp, Nivelar
 *              to godot_vivarium_level.cpp, the cutting mode to godot_vivarium_prune.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100386 - select a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-25
 * @version     00.00.02.00003
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/input_event_magnify_gesture.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/viewport.hpp>

#include <cstddef>
#include <cstdint>

namespace ase::adp::godot {

void VivariumView::unhandled_input(const ::godot::Ref<::godot::InputEvent>& event) {
    if (in_editor() || !node_->is_visible_in_tree()) {
        return;
    }
    // ONE PATH PER PHYSICAL TAP: touch arrives as ScreenTouch, a mouse as MouseButton - the
    // project switches Godot's touch-to-mouse emulation off, so an Android tap waters once.
    // make_input_local maps through the inverse canvas transform onto the visible surface; tap()
    // matches that point against the layout as the last frame placed it.
    const ::godot::Ref<::godot::InputEvent> local = node_->make_input_local(event);
    const ::godot::Ref<::godot::InputEventScreenTouch> touch = local;
    const ::godot::Ref<::godot::InputEventMouseButton> mouse = local;
    const ::godot::Ref<::godot::InputEventMagnifyGesture> pinch = local;
    // THE CAMERA ZOOMS: the mouse wheel on the desktop, two fingers on the device - towards the birch and
    // back to the whole ground (patch_focus_zoom)
    if (mouse.is_valid() && mouse->is_pressed() &&
        (mouse->get_button_index() == ::godot::MOUSE_BUTTON_WHEEL_UP ||
         mouse->get_button_index() == ::godot::MOUSE_BUTTON_WHEEL_DOWN)) {
        node_->get_viewport()->set_input_as_handled();
        zoom(mouse->get_button_index() == ::godot::MOUSE_BUTTON_WHEEL_UP ? 1 : -1);
        return;
    }
    if (pinch.is_valid()) {
        node_->get_viewport()->set_input_as_handled();
        zoom(pinch->get_factor() > 1.0f ? 1 : -1);
        return;
    }
    ::godot::Vector2 at;
    if (touch.is_valid()) {
        if (!touch->is_pressed()) {
            return;
        }
        at = touch->get_position();
    } else if (mouse.is_valid() && mouse->get_button_index() == ::godot::MOUSE_BUTTON_LEFT) {
        if (!mouse->is_pressed()) {
            return;
        }
        at = mouse->get_position();
    } else {
        return;
    }
    node_->get_viewport()->set_input_as_handled();
    tap(at);
}

void VivariumView::tap(const ::godot::Vector2& at) {
    // Without a placement - no configuration, a surface without room - nothing was drawn to tap.
    if (!(placement_.scale > 0.0f)) {
        return;
    }
    if (placed(layout_.reset_rect).has_point(at)) {
        restart();
        return;
    }
    if (!configured_ || !failure_text_.is_empty()) {
        return;  // on the error panel only Reiniciar answers
    }
    if (placed(layout_.water_rect).has_point(at)) {
        water();
        return;
    }
    if (placed(layout_.level_rect).has_point(at)) {
        level();
        return;
    }
    if (placed(layout_.prune_rect).has_point(at)) {
        toggle_pruning();
        return;
    }
    if (placed(layout_.bed_rect).has_point(at)) {
        toggle_bedding();
        return;
    }
    if (placed(layout_.flatten_rect).has_point(at)) {
        toggle_flattening();
        return;
    }
    if (pruning_) {
        prune(at);  // in the cutting mode a tap in the scene cuts and selects no place
        return;
    }
    if (bedding_) {
        bed(at);  // in the bed mode a tap in the scene marks an end of the bed and selects no place
        return;
    }
    if (flattening_) {
        flatten(at);  // in the flatten mode a tap in the scene levels a disc there and selects no place
        return;
    }
    for (std::size_t place = 0; place < layout_.place_rects.size(); ++place) {
        if (placed(layout_.place_rects[place]).has_point(at)) {
            select(static_cast<uint32_t>(place));
            return;
        }
    }
}

void VivariumView::zoom(int32_t steps) {
    // The camera is aimed anew in the next frame (follow_scene), at the new zoom.
    zoom_ = patch_focus_zoom(zoom_, steps);
    scene_focus_ = GODOT_NO_PLACE;
    note("zoom " + ::godot::String::num(zoom_, 3));
}

void VivariumView::select(uint32_t place) {
    // A selection frame, no change to the simulation.
    selected_place_ = place;
    note("select " + measure(place) + " result=" + (resources_.get_patch_at(place) != nullptr ? "ok" : "empty"));
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

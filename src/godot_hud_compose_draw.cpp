/**
 * ASE GODOT ADAPTER - HUD COMPOSE DRAW
 *
 * @file        godot_hud_compose_draw.cpp
 * @brief       VivariumHud: draws the weather, the world's things and the layers of the mode, the toast inside the inventory
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, see godot_hud_compose.hpp. In every mode, as the
 *              mockup's drawScene, the things on the ground and the tool place in the world come first; the WeatherVeil and the
 *              ScreenShade lie over the scene; then each layer of HudShown::layers in the order hud.modes names
 *              them. The Toast belongs to the inventory layer as the mockup's hand() closes with it (m.html 749):
 *              after the hand, the belt and the bag icon, under the windows, the reading, the debug layer and the
 *              ghost, and gone wherever the inventory is (arrive, sky, sys, the open debug drawer); it moves up while
 *              the terrain or a thing in the hand occupies the foot (the mockup's busy). The sky band draws on the
 *              top band's frame, the hand, the belt, the bag icon and the toast on the bottom band's. This unit also
 *              answers where a thing was last drawn, in the middle band's units, which is where a drag takes it up.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_compose.hpp>
#include <ase/adp/godot/godot_vivarium_watch.hpp>
#include <ase/adp/godot/hud/types.hpp>

namespace ase::adp::godot {

bool VivariumHud::draw(UiPen& canvas, const ::godot::Transform2D& frame, VivariumHudFrame& now,
                       std::string& failed) {
    // THE SHARES OF THE DRAWING (P0692): the view's hud_draw_split line named this call as one number, about 28 ms
    // of the main thread's CPU per frame (mean 28470 us, max 97116 us in goal-3's run of 2026-11-13). Each step below
    // writes its CPU time into shares_, so the line names the step that carries the cost.
    shares_.clear();
    uint64_t cpu_step = thread_cpu_usec();
    const auto share = [this, &cpu_step](const std::string& step) { share_step(step.c_str(), cpu_step); };
    // The mockup's order: the things in the world, the weather over them, the shade over the scene, the layers; the
    // toast is part of the inventory layer (draw_inventory). The container records are taken first and in every mode: the inventory layer may stand hidden,
    // the pack and the pouch still hold this frame's record (BagWindow::take).
    window_.take(now.window);
    // drawTree lays the shade under the birch, or the seed mound, before drawWorld and outside its brightness
    // (m.html 488 to 490).
    shade_tree_.draw(canvas, frame, now.shade);
    share(HudDrawStep::TREE_SHADE);
    // drawScene calls drawWorld in every mode (m.html 507): the world group stands in the picture whatever the mode
    // lets the finger reach, and only the hit side (VivariumHud::hit) asks for HITS_WORLD.
    if (!world_.draw(canvas, frame, now.world, failed)) {
        return false;
    }
    share(HudDrawStep::WORLD);
    if (!rack_.draw_world(canvas, frame, now.rack_world, failed)) {
        return false;
    }
    share(HudDrawStep::RACK_WORLD);
    // drawWorld closes with the tap hole and the sow hole (m.html 472 to 477), under the same brightness.
    tap_.draw(canvas, frame, now.tap);
    sow_.draw(canvas, frame, now.sow);
    share(HudDrawStep::HOLES);
    // The world group ends here, and with it its brightness: the weather, the shade and the layers draw as given.
    canvas.set_tint(::godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
    if (!veil_.draw(canvas, frame, now.veil, failed)) {
        return false;
    }
    share(HudDrawStep::VEIL);
    if (!shade_.draw(canvas, frame, now.cover, failed)) {
        return false;
    }
    share(HudDrawStep::SHADE);
    for (const std::string& layer : shown_.layers) {
        if (!draw_layer(canvas, frame, layer, now, failed)) {
            return false;
        }
        share(layer);
    }
    return true;
}

::godot::Transform2D VivariumHud::head_frame(const ::godot::Transform2D& frame) const {
    return frame.translated_local(::godot::Vector2(0.0f, static_cast<float>(cover_.head)));
}

::godot::Transform2D VivariumHud::hand_frame(const ::godot::Transform2D& frame) const {
    return frame.translated_local(::godot::Vector2(0.0f, static_cast<float>(cover_.hand)));
}

bool VivariumHud::draw_layer(UiPen& canvas, const ::godot::Transform2D& frame, const std::string& layer,
                             VivariumHudFrame& now, std::string& failed) {
    if (layer == LAYER_TAP_PULSE) {
        // render lays the pulse at the hole first in sap and plug (m.html 909), over the world's brightness.
        tap_.draw_pulse(canvas, frame, now.tap, wall_ - replay_.since(), now.still);
        return true;
    }
    if (layer == LAYER_BAND) {
        return band_.draw(canvas, head_frame(frame), now.band, failed);
    }
    if (layer == LAYER_RAIL) {
        // THE MARK STANDS ON THE WALL CLOCK (replay, godot_hud_compose.cpp): the elapsed time of a fade is taken on
        // that same clock. On the capped HUD clock it ran negative, the rail's line drew a share of 0 and stood
        // nowhere while its dots and its word showed (run 20261113-230745 debug_stage_3.png)
        rail_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        // The states of the body stand under the signs on the same rail and fade in on the same clock (s.html 187).
        if (!states_.set(now.body, failed)) {
            return false;
        }
        states_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        return true;
    }
    if (layer == LAYER_BODY) {
        // The body rail on the same wall clock as the rail of signs: its rows fade in after the view appeared.
        // The scale CONFORTO hangs over the heat row on its leader and fades in with it (s.html corpo); the card of
        // the camp fire follows it, as render() lays fireCard(st) right after corpo(st) (s.html 239), and the card of
        // the resting figure after the fire, as render() lays rest(st) right after fireCard(st), and the mark of
        // the air after the rest, as render() lays air(st) right after rest(st), and the spoil ring of the bag's food
        // after the air, as render() lays food(st) right after air(st), and the water mark after the food, as
        // render() lays water(st) last; it reads the same sign poison of the body the rail SINAIS draws.
        if (!body_.set(now.body, failed) || !warmth_.set(now.body, failed) || !fire_.set(now.fire, failed) ||
            !rest_.set(now.rest, failed) || !air_.set(now.air, failed) || !food_.set(now.food, failed) ||
            !water_.set(now.body, failed)) {
            return false;
        }
        body_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        warmth_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        fire_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        rest_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        air_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        food_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        water_.draw(canvas, frame, wall_ - replay_.since(), now.still);
        return true;
    }
    if (layer == LAYER_MARKERS) {
        return signs_.draw(canvas, frame, now.signs, failed);
    }
    if (layer == LAYER_SIGN_OPEN) {
        // SignMarker draws the open reading together with the markers; without them it draws it here.
        return shows(LAYER_MARKERS) || signs_.draw(canvas, frame, now.signs, failed);
    }
    if (layer == LAYER_FOCUS) {
        if (!focus_.draw(canvas, frame, now.focus, failed)) {
            return false;
        }
        now.fan.centre = focus_.ring().centre();
        return fan_.draw(canvas, frame, now.fan, failed);
    }
    if (layer == LAYER_PRUNE) {
        return prune_.draw(canvas, frame, now.prune, failed);
    }
    if (layer == LAYER_ROOT_CUT) {
        return root_cut_.draw(canvas, frame, now.root_cut, failed);
    }
    if (layer == LAYER_RETICLE) {
        return terrain_.draw(canvas, frame, now.terrain, failed);
    }
    if (layer == TerrainKit::LAYER_GROUND) {
        return terrain_.draw_rows(canvas, frame, now.terrain, failed);
    }
    if (layer == TerrainKit::LAYER_GROUND_TERRAIN) {
        return terrain_.draw_mesh(canvas, frame, now.terrain, failed);
    }
    if (layer == LAYER_ARRIVE) {
        return trail_.draw(canvas, frame, now.trail, failed);
    }
    if (layer == LAYER_SKY) {
        return sheet_.draw(canvas, frame, now.sheet, failed);
    }
    if (layer == LAYER_SYSTEM) {
        SystemRingFrame ring;
        ring.held = press_.system ? now.clock - press_.clock : 0.0;
        ring.elapsed = wall_ - replay_.since();
        ring.still = now.still;
        ring.cover = now.cover;
        return system_.draw(canvas, frame, ring, failed);
    }
    if (layer == LAYER_RACK) {
        return rack_.draw_list(canvas, frame, now.rack, failed);
    }
    if (layer == LAYER_INVENTORY) {
        return draw_inventory(canvas, frame, now, failed);
    }
    if (layer == LAYER_DEBUG) {
        if (now.debug_data == nullptr) {
            failed = "VivariumHudFrame.debug_data";
            return false;
        }
        return debug_.draw(canvas, frame, *now.debug_data, now.debug, failed);
    }
    if (layer == LAYER_DRAG) {
        return drag_.draw(canvas, frame, failed);
    }
    failed = "hud.modes: the layer " + layer + " names no block";
    return false;
}

bool VivariumHud::draw_inventory(UiPen& canvas, const ::godot::Transform2D& frame,
                                 VivariumHudFrame& now, std::string& failed) {
    // The hand, the belt and the bag icon stand on the bottom band; the windows open over the middle one.
    // The reading of a single tap stands over both windows (hand, bagWin, pouchWin, selLabel). hand() closes with
    // the toast (m.html 749), so it lies over the bag icon and under the windows.
    const ::godot::Transform2D hand = hand_frame(frame);
    ToastFrame toast;
    toast.now = wall_;  // the toast's until is a performance.now() in the mockup (m.html 931): the wall clock
    toast.busy = mode_ == GODOT_HUD_MODE_TERRAIN || hand_thing(now) != nullptr;
    // THE PARTS OF THE LAYER (P0695): the layer cost 77.6 ms in the first frame after an act and about 5 ms in each
    // frame after it (run 20261113-p0692-split); each part writes its own share, the layer's share names their sum.
    uint64_t cpu_step = thread_cpu_usec();
    if (!hand_.draw(canvas, hand, now.hand, failed)) {
        return false;
    }
    // The parts of the hand (P0692): the first frame after an act cost 68.9 ms in the hand alone while look() and
    // sprite() were already filled (run 20261113-p0695-dig); each part stands before the hand's own share.
    const HandSlotCost& hand_cost = hand_.cost();
    for (const std::pair<const char*, uint64_t>& part : std::initializer_list<std::pair<const char*, uint64_t>>{
             {HudDrawStep::INV_HAND_LOOK, hand_cost.look_usec},
             {HudDrawStep::INV_HAND_ART, hand_cost.art_usec},
             {HudDrawStep::INV_HAND_STOW, hand_cost.stow_usec},
             {HudDrawStep::INV_HAND_MODE, hand_cost.mode_usec},
             {HudDrawStep::INV_HAND_TEXT, hand_cost.text_usec},
             {HudDrawStep::INV_HAND_TITLE, hand_cost.title_usec},
             {HudDrawStep::INV_HAND_HINT, hand_cost.hint_usec},
             {HudDrawStep::INV_HAND_CASE, hand_cost.case_usec},
             {HudDrawStep::INV_HAND_READING, hand_cost.reading_usec},
             {HudDrawStep::INV_HAND_VALUE, hand_cost.value_usec},
             {HudDrawStep::INV_HAND_SHAPE, hand_cost.shape_usec},
             {HudDrawStep::INV_HAND_WARM, hand_cost.warm_usec}}) {
        shares_.push_back(HudDrawShare{part.first, part.second});
    }
    share_step(HudDrawStep::INV_HAND, cpu_step);
    if (!belt_.draw(canvas, hand, now.belt, failed)) {
        return false;
    }
    share_step(HudDrawStep::INV_BELT, cpu_step);
    if (!bag_icon_.draw(canvas, hand, now.bag_icon, failed)) {
        return false;
    }
    share_step(HudDrawStep::INV_BAG, cpu_step);
    if (!toast_.draw(canvas, hand, toast, failed)) {
        return false;
    }
    share_step(HudDrawStep::INV_TOAST, cpu_step);
    if (!window_.draw(canvas, frame, now.window, failed)) {
        return false;
    }
    share_step(HudDrawStep::INV_WINDOW, cpu_step);
    if (!draw_select(canvas, frame, now, failed)) {
        return false;
    }
    share_step(HudDrawStep::INV_SELECT, cpu_step);
    return true;
}

void VivariumHud::share_step(const char* step, uint64_t& cpu_step) {
    const uint64_t cpu_now = thread_cpu_usec();
    shares_.push_back(HudDrawShare{step, cpu_now - cpu_step});
    cpu_step = cpu_now;
}

bool VivariumHud::draw_select(UiPen& canvas, const ::godot::Transform2D& frame, const VivariumHudFrame& now,
                              std::string& failed) {
    // selLabel: nothing while no thing is chosen or the chosen one was drawn nowhere this frame (POS[o.key]).
    if (selected_.empty()) {
        return true;
    }
    const VivariumHudThing* chosen = thing(selected_, now);
    ::godot::Rect2 box;
    double k = 0.0;
    if (chosen == nullptr || !box_of(selected_, box, k)) {
        return true;
    }
    SelectLabelFrame label;
    label.id = chosen->id;
    label.length_cm = chosen->length_cm;
    label.kg = chosen->kg;
    label.weight_kg = chosen->weight_kg;
    label.litres = chosen->litres;
    label.count = chosen->count;
    label.stack = chosen->stack;
    label.opens = chosen->id == pouch_.item().id();
    label.held = same(chosen->from, GODOT_HUD_PLACE_HAND);
    label.box = box;
    label.level = shown_.level;
    label.elapsed = wall_ - replay_.since();  // S.sel is no part of lastKey: the leader plays with the frame
    label.still = now.still;
    return select_.draw(canvas, frame, label, failed);
}

const VivariumHudThing* VivariumHud::thing(const std::string& key, const VivariumHudFrame& now) const {
    for (const VivariumHudThing& candidate : now.things) {
        if (candidate.key == key) {
            return &candidate;
        }
    }
    return nullptr;
}

const VivariumHudThing* VivariumHud::hand_thing(const VivariumHudFrame& now) const {
    for (const VivariumHudThing& candidate : now.things) {
        if (same(candidate.from, GODOT_HUD_PLACE_HAND)) {
            return &candidate;
        }
    }
    return nullptr;
}

bool VivariumHud::box_of(const std::string& key, ::godot::Rect2& box, double& k) const {
    for (const std::vector<BagThingPlace>* list : {&window_.pouch_places(), &window_.pack_places()}) {
        for (const BagThingPlace& place : *list) {
            if (place.key == key) {
                box = place.box;
                k = place.k;
                return true;
            }
        }
    }
    for (const BagThingPlace& place : belt_.places()) {
        if (place.key == key) {
            box = place.box;
            box.position.y += static_cast<float>(cover_.hand);  // the belt is drawn on the bottom band
            k = place.k;
            return true;
        }
    }
    if (hand_.place().key == key) {
        box = hand_.place().box;
        box.position.y += static_cast<float>(cover_.hand);  // the hand is drawn on the bottom band
        k = hand_.place().k;
        return true;
    }
    for (const GroundThingPlace& place : world_.places()) {
        if (place.key == key) {
            box = place.box;
            k = place.k;
            return true;
        }
    }
    for (const RackToolPlace& place : rack_.places()) {
        if (place.key == key) {
            box = place.box;
            k = place.k;
            return true;
        }
    }
    return false;
}

}  // namespace ase::adp::godot

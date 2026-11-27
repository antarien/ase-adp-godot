/**
 * ASE GODOT ADAPTER - HUD COMPOSE HIT
 *
 * @file        godot_hud_compose_hit.cpp
 * @brief       VivariumHud: what a touch hits, from the topmost layer of the mode down to the ground of the world
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, see godot_hud_compose.hpp. The blocks are asked in the
 *              reverse order of drawing: the debug layer, the inventory (the windows, the bag, the belt, the hand),
 *              the tool place's list, the system ring, the calendar sheet, the trail, the prune marks, the fan and
 *              the focus ring, the signs, the rail and the sky band. What none of them takes falls to the world:
 *              the things on the ground, the tool place in the world, the trunk, the stand of the tree and the
 *              ground, each only where the mode lays it (hud.modes.<mode>.hits). The sky band is asked on the top
 *              band (cover_.head), the bag icon, the belt and the hand on the bottom band (cover_.hand). Where a tap
 *              lands on each part, for the view's place lines, answers godot_hud_compose_place.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_compose.hpp>
#include <ase/adp/godot/hud/drag/types.hpp>
#include <ase/adp/godot/hud/hand/types.hpp>
#include <ase/adp/godot/hud/touch/types.hpp>
#include <ase/adp/godot/rsm/types.hpp>

#include <godot_cpp/core/math.hpp>

#include <string>

namespace ase::adp::godot {

namespace {

/** The act constant a block answers as text, nullptr when it names none of them. */
const char* act_of(const std::string& text) {
    const char* const acts[] = {GODOT_HUD_ACT_POUR,        GODOT_HUD_ACT_STOW,        GODOT_HUD_ACT_VERB,
                                GODOT_HUD_ACT_ITEM,        GODOT_HUD_ACT_NOOP,        GODOT_HUD_ACT_REST,
                                GODOT_HUD_ACT_PACK_GRIP,   GODOT_HUD_ACT_POUCH_GRIP,  GODOT_HUD_ACT_WINDOW_BODY,
                                GODOT_HUD_ACT_PACK_CLOSE,  GODOT_HUD_ACT_POUCH_CLOSE, GODOT_HUD_ACT_SKY,
                                GODOT_HUD_ACT_DEBUG};
    for (const char* act : acts) {
        if (text == act) {
            return act;
        }
    }
    return nullptr;
}

}  // namespace

VivariumHudHit VivariumHud::hit(const ::godot::Vector2& point, const VivariumHudFrame& now) const {
    VivariumHudHit out;
    const auto keyed = [&](const char* act, const std::string& key) {
        out.act = act;
        out.key = key;
        const VivariumHudThing* found = key.empty() ? nullptr : thing(key, now);
        out.from = found != nullptr ? found->from : nullptr;
        return out;
    };

    if (shows(LAYER_DEBUG) && now.debug_data != nullptr) {
        DebugLayerHit part;
        if (debug_.hit(point, *now.debug_data, now.debug, part)) {
            out.act = ACT_DEBUG_PART;
            out.debug = part;
            return out;
        }
        // The open drawer lies over the scene: a tap on its body that misses every chip stops on it.
        ::godot::Rect2 drawer;
        if (debug_.drawer_place(now.debug, drawer) && drawer.has_point(point)) {
            return out;
        }
    }
    if (shows(LAYER_INVENTORY)) {
        const BagWindowHit window = window_.hit(point);
        if (window.part != nullptr) {
            return keyed(window.part, window.key);
        }
        // The bag icon, the belt and the hand stand on the bottom band, cover_.hand below the middle one.
        const ::godot::Vector2 at_hand = point - ::godot::Vector2(0.0f, static_cast<float>(cover_.hand));
        // The band switch in the gap of the dividers over the hand turns the band to the slots or the bars
        // (operator order 2026-11-25); while the bars hold the band, the hidden slots take no tap.
        if (bars_.switch_hit(at_hand)) {
            out.act = GODOT_RSM_ACT_BAND;
            return out;
        }
        const bool slots = bars_.band() != GODOT_RSM_BAND_BARS;
        const char* icon = slots ? bag_icon_.hit(at_hand) : nullptr;
        if (icon != nullptr) {
            return keyed(icon, std::string());
        }
        // The activity bars right of the hand or on the whole band (usecase:101305, B5): a touch names the action of
        // its bar, which stays lit while the finger holds it; the act asks the world nothing.
        uint32_t action = GODOT_RSM_ACTION_NONE;
        if (bars_.hit(at_hand, action)) {
            out.act = GODOT_RSM_ACT_BARS;
            out.index = action;
            return out;
        }
        if (slots) {
            const BeltHolsterHit belt = belt_.hit(at_hand);
            if (belt.part != nullptr) {
                return keyed(belt.part, belt.key);
            }
            std::string key;
            const std::string hand = hand_.hit(at_hand, key);
            if (!hand.empty()) {
                return keyed(act_of(hand), key);
            }
        }
    }
    if (shows(LAYER_RACK)) {
        std::string key;
        const std::string rack = rack_.hit(point, key);
        if (!rack.empty()) {
            return keyed(act_of(rack), key);
        }
    }
    if (shows(LAYER_SYSTEM)) {
        SystemRingHit ring;
        if (system_.hit(point, ring)) {
            out.act = ring.hold ? ACT_SYSTEM_HOLD : GODOT_HUD_ACT_REST;
            return out;
        }
    }
    if (shows(LAYER_SKY)) {
        CalendarSheetHit sheet;
        if (sheet_.hit(point, sheet)) {
            out.act = ACT_SHEET;
            out.sheet = sheet;
            return out;
        }
    }
    if (shows(LAYER_ARRIVE)) {
        uint32_t sign = UINT32_MAX;
        if (trail_.hit(point, sign)) {
            out.act = sign == UINT32_MAX ? GODOT_HUD_ACT_REST : ACT_SIGN;
            out.index = sign;
            return out;
        }
    }
    if (shows(LAYER_PRUNE)) {
        uint32_t internode = UINT32_MAX;
        if (prune_.hit(point, internode)) {
            out.act = ACT_CUT;
            out.index = internode;
            return out;
        }
    }
    if (shows(LAYER_FOCUS)) {
        ActionFanHit chip;
        if (fan_.hit(point, chip)) {
            out.act = chip.rack ? ACT_RACK : ACT_TAKE;
            out.index = chip.tool;
            return out;
        }
        if (focus_.hit(point)) {
            out.act = ACT_FOCUS;
            return out;
        }
    }
    if (shows(LAYER_MARKERS) || shows(LAYER_SIGN_OPEN)) {
        SignMarkerHit marker;
        if (signs_.hit(point, marker)) {
            const bool offer = marker.offer != UINT32_MAX;
            out.act = offer ? ACT_TAKE : ACT_SIGN;
            out.index = offer ? marker.offer : marker.sign;
            return out;
        }
    }
    if (shows(LAYER_RAIL)) {
        // HISTORY first: the artifact's rail draws its group after the dots (m.html 661), so where the hit square
        // of a low dot reaches into it, the glyph lies on top and takes the tap.
        if (rail_.hit_system(point)) {
            out.act = ACT_SYSTEM_PRESS;
            return out;
        }
        int32_t index = -1;
        if (rail_.hit_sign(point, index) && index >= 0 && static_cast<size_t>(index) < shown_.signs.size()) {
            out.act = ACT_SIGN;
            out.index = shown_.signs[static_cast<size_t>(index)].sign;
            return out;
        }
    }
    if (shows(LAYER_ARC)) {
        // The sky arc stands on the top band as the band does; a tap on it opens the calendar sheet like the band's.
        const char* arc = arc_.hit(point - ::godot::Vector2(0.0f, static_cast<float>(cover_.head)));
        if (arc != nullptr) {
            out.act = arc;
            return out;
        }
    }
    if (shows(LAYER_BAND)) {
        // The sky band stands on the top band, cover_.head off the middle one.
        const char* band = band_.hit(point - ::godot::Vector2(0.0f, static_cast<float>(cover_.head)));
        if (band != nullptr) {
            out.act = band;
            return out;
        }
    }
    return hit_world(point, now);
}

VivariumHudHit VivariumHud::hit_world(const ::godot::Vector2& point, const VivariumHudFrame& now) const {
    VivariumHudHit out;
    if (lays(HITS_WORLD)) {
        const std::string key = world_.hit(point);
        if (!key.empty()) {
            out.act = GODOT_HUD_ACT_ITEM;
            out.key = key;
            out.from = GODOT_HUD_PLACE_GROUND;
            return out;
        }
        // D01: a rack body in the scene is hit by the ray of the camera through the finger (rack_struck), the
        // drawn place by its box in mockup metres (WorldDrop::hit_rack)
        if (now.rack_world.bodied ? now.rack_world.struck : world_.hit_rack(point)) {
            out.act = ACT_RACK;
            return out;
        }
    }
    if (now.tree_placed && lays(HITS_TRUNK) && trunk_rect(now).has_point(point)) {
        out.act = ACT_TRUNK;
        return out;
    }
    if (now.tree_placed && lays(HITS_TREE) && tree_rect(now).has_point(point)) {
        out.act = ACT_TREE;
        return out;
    }
    if (lays(HITS_GROUND)) {
        const double height = mode_ == GODOT_HUD_MODE_TERRAIN ? ground_terrain_h_ : ground_h_;
        const ::godot::Rect2 ground(static_cast<float>(ground_x_), static_cast<float>(ground_y_),
                                    static_cast<float>(ground_w_), static_cast<float>(height));
        if (ground.has_point(point)) {
            out.act = ACT_GROUND;
            return out;
        }
    }
    return out;
}

::godot::Rect2 VivariumHud::trunk_rect(const VivariumHudFrame& now) const {
    // The stand of the birch: its height, never less than the smallest stand (the mockup's max(52, h)).
    const double stand = ::godot::Math::max(tree_min_, now.tree_height);
    const double trunk_h = stand * trunk_share_;
    return ::godot::Rect2(static_cast<float>(now.trunk.x - trunk_half_), static_cast<float>(now.trunk.y - trunk_h),
                          static_cast<float>(trunk_half_ * 2.0), static_cast<float>(trunk_h + trunk_pad_));
}

::godot::Rect2 VivariumHud::tree_rect(const VivariumHudFrame& now) const {
    const double stand = ::godot::Math::max(tree_min_, now.tree_height);
    const double half = ::godot::Math::max(tree_half_min_, stand * tree_half_share_);
    return ::godot::Rect2(static_cast<float>(now.trunk.x - half), static_cast<float>(now.trunk.y - stand),
                          static_cast<float>(half * 2.0), static_cast<float>(stand + tree_pad_));
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD COMPOSE PLACE
 *
 * @file        godot_hud_compose_place.cpp
 * @brief       VivariumHudPlaces: where a tap lands on VivariumHud, the rects hit() and hit_world() ask
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, see godot_hud_compose_place.hpp. Each place is the rect
 *              a block gives for its hit, taken only while the layer or the hit area of the resolved mode shows it,
 *              and shifted onto the band draw() put the block on: the sky band and the debug switch by cover_.head,
 *              the bag icon and the hand by cover_.hand; every other place stays on the middle band.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_compose_place.hpp>

#include <ase/math/math.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The largest part of `place` that `box` leaves free: the piece above, below, left or right of it; `place`
 *  itself when the two do not meet, an empty rect when the box covers it whole. */
::godot::Rect2 beside(const ::godot::Rect2& place, const ::godot::Rect2& box) {
    if (!place.intersects(box)) {
        return place;
    }
    const ::godot::Vector2 end = place.get_end();
    const ::godot::Vector2 cut = box.get_end();
    const ::godot::Rect2 pieces[] = {
        ::godot::Rect2(place.position, ::godot::Vector2(place.size.x, box.position.y - place.position.y)),
        ::godot::Rect2(::godot::Vector2(place.position.x, cut.y), ::godot::Vector2(place.size.x, end.y - cut.y)),
        ::godot::Rect2(place.position, ::godot::Vector2(box.position.x - place.position.x, place.size.y)),
        ::godot::Rect2(::godot::Vector2(cut.x, place.position.y), ::godot::Vector2(end.x - cut.x, place.size.y)),
    };
    ::godot::Rect2 best(place.position, ::godot::Vector2());
    for (const ::godot::Rect2& piece : pieces) {
        if (piece.size.x > 0.0f && piece.size.y > 0.0f && piece.get_area() > best.get_area()) {
            best = piece;
        }
    }
    return best;
}

}  // namespace

bool VivariumHudPlaces::debug_places(const VivariumHud& hud, const VivariumHudFrame& now,
                                     ::godot::Rect2& debug_switch, std::vector<DebugLayerChip>& out,
                                     std::string& failed) {
    debug_switch = hud.band_.debug_switch();
    debug_switch.position.y += static_cast<float>(hud.cover_.head);  // the switch sits in the sky band, on the top band
    out.clear();
    if (!hud.shows(VivariumHud::LAYER_DEBUG) || now.debug_data == nullptr) {
        return true;  // the layer is hidden: only the switch answers
    }
    return hud.debug_.places(*now.debug_data, now.debug, out, failed);
}

void VivariumHudPlaces::system_places(const VivariumHud& hud, ::godot::Rect2& foot, bool& foot_hidden,
                                      ::godot::Rect2& ring, bool& ring_hidden, ::godot::Rect2& close) {
    foot_hidden = !hud.shows(VivariumHud::LAYER_RAIL);
    foot = hud.rail_.system_place();
    ring_hidden = !hud.shows(VivariumHud::LAYER_SYSTEM);
    ring = hud.system_.hold_place();
    close = hud.system_.close_place();
}

void VivariumHudPlaces::tree_place(const VivariumHud& hud, const VivariumHudFrame& now, ::godot::Rect2& tree,
                                   bool& hidden) {
    hidden = !now.tree_placed || !hud.lays(VivariumHud::HITS_TREE);
    tree = hud.tree_rect(now);
    if (hidden) {
        return;
    }
    if (hud.lays(VivariumHud::HITS_TRUNK)) {
        // hit_world() asks the trunk first: the place is the stand left of the trunk, where only the tree answers.
        const ::godot::Rect2 trunk = hud.trunk_rect(now);
        tree.size.x = trunk.position.x - tree.position.x;
    }
    tree = reachable(hud, now, tree);
    hidden = tree.size.x <= 0.0f || tree.size.y <= 0.0f;
}

void VivariumHudPlaces::trunk_place(const VivariumHud& hud, const VivariumHudFrame& now, ::godot::Rect2& trunk,
                                    bool& hidden) {
    // The modes sap and plug lay the trunk: a tap there bores the tap hole or pegs it (ACT_TRUNK, hit_world()).
    hidden = !now.tree_placed || !hud.lays(VivariumHud::HITS_TRUNK);
    trunk = hud.trunk_rect(now);
    if (hidden) {
        return;
    }
    trunk = reachable(hud, now, trunk);
    hidden = trunk.size.x <= 0.0f || trunk.size.y <= 0.0f;
}

::godot::Rect2 VivariumHudPlaces::reachable(const VivariumHud& hud, const VivariumHudFrame& now,
                                            const ::godot::Rect2& place) {
    // A finger reaches only the surface: a birch taller than the screen keeps the part of its stand that shows.
    ::godot::Rect2 out = place.intersection(hud.cover_.surface);
    if (out.size.x <= 0.0f || out.size.y <= 0.0f) {
        return ::godot::Rect2(place.position, ::godot::Vector2());
    }
    // hit() asks the open drawer before the world: the place ends where the drawer's body begins.
    ::godot::Rect2 drawer;
    if (hud.shows(VivariumHud::LAYER_DEBUG) && now.debug_data != nullptr && hud.debug_.drawer_place(now.debug, drawer)) {
        out.size.y = ase::math::min(out.size.y, drawer.position.y - out.position.y);
        if (out.size.y <= 0.0f) {
            return ::godot::Rect2(place.position, ::godot::Vector2());
        }
    }
    // hit() asks the fan, the focus ring and the signs before the world: a chip, the ring or a marker over the birch
    // takes the tap, so the place is the largest part that none of them covers (the mockup draws all three over the
    // tree's rect).
    std::vector<ActionFanPlace> fan;
    fan_places(hud, fan);
    for (const ActionFanPlace& chip : fan) {
        out = beside(out, chip.rect);
    }
    ::godot::Rect2 ring;
    bool ring_hidden = true;
    focus_place(hud, ring, ring_hidden);
    if (!ring_hidden) {
        out = beside(out, ring);
    }
    std::vector<SignMarkerPlace> signs;
    sign_places(hud, signs);
    for (const SignMarkerPlace& sign : signs) {
        out = beside(out, sign.rect);
    }
    return out;
}

void VivariumHudPlaces::arrive_place(const VivariumHud& hud, ::godot::Rect2& play, bool& hidden) {
    hud.trail_.play_place(play, hidden);
    hidden = hidden || !hud.shows(VivariumHud::LAYER_ARRIVE);
}

void VivariumHudPlaces::band_place(const VivariumHud& hud, ::godot::Rect2& sky, bool& hidden) {
    sky = hud.band_.sky_place();
    sky.position.y += static_cast<float>(hud.cover_.head);  // the sky band stands on the top band
    hidden = !hud.shows(VivariumHud::LAYER_BAND);
}

void VivariumHudPlaces::bottom_places(const VivariumHud& hud, ::godot::Rect2& bag, bool& bag_hidden,
                                      HandThingPlace& hand, bool& hand_hidden) {
    // The bag icon and the hand stand on the bottom band, cover_.hand below the middle one (hit() asks there).
    const float band = static_cast<float>(hud.cover_.hand);
    bag_hidden = !hud.bag_icon_.place(bag) || !hud.shows(VivariumHud::LAYER_INVENTORY);
    bag.position.y += band;
    hand = hud.hand_.place();
    hand_hidden = hand.key.empty() || !hud.shows(VivariumHud::LAYER_INVENTORY);
    hand.box.position.y += band;
    hand.stow.position.y += band;
    for (std::pair<std::string, ::godot::Rect2>& verb : hand.verbs) {
        verb.second.position.y += band;
    }
}

void VivariumHudPlaces::fan_places(const VivariumHud& hud, std::vector<ActionFanPlace>& out) {
    out.clear();
    if (hud.shows(VivariumHud::LAYER_FOCUS)) {
        hud.fan_.places(out);
    }
}

void VivariumHudPlaces::focus_place(const VivariumHud& hud, ::godot::Rect2& ring, bool& hidden) {
    // In the mode focus a tap on the birch selects it again and changes nothing; the ring is the way to rest.
    hidden = !hud.focus_.ring().place(ring) || !hud.shows(VivariumHud::LAYER_FOCUS);
}

void VivariumHudPlaces::mode_places(const VivariumHud& hud, std::vector<PruneMarkPlace>& cuts,
                                    std::vector<RackToolPlace>& rows) {
    // Both stay on the middle band, as hit() asks them: the marks of the mode prune, the rows of the mode rack.
    cuts.clear();
    rows.clear();
    if (hud.shows(VivariumHud::LAYER_PRUNE)) {
        hud.prune_.places(cuts);
    }
    if (hud.shows(VivariumHud::LAYER_RACK)) {
        rows = hud.rack_.places();
    }
}

void VivariumHudPlaces::sign_places(const VivariumHud& hud, std::vector<SignMarkerPlace>& out) {
    // The markers stand on the middle band and are asked while the markers or an open sign show, as hit() asks.
    out.clear();
    if (hud.shows(VivariumHud::LAYER_MARKERS) || hud.shows(VivariumHud::LAYER_SIGN_OPEN)) {
        hud.signs_.places(out);
    }
}

void VivariumHudPlaces::rail_places(const VivariumHud& hud, std::vector<SignMarkerPlace>& out) {
    // The rail stands on the middle band; hit() maps the rail's place `index` to shown_.signs[index].sign, and only
    // below the count of the shown signs. The HISTORY glyph answers first where a low dot's target reaches into it.
    out.clear();
    if (!hud.shows(VivariumHud::LAYER_RAIL)) {
        return;
    }
    SignMarkerPlace place;
    for (size_t index = 0; index < hud.shown_.signs.size() && hud.rail_.sign_place(index, place.rect); ++index) {
        place.sign = hud.shown_.signs[index].sign;
        out.push_back(place);
    }
}

void VivariumHudPlaces::event_places(const VivariumHud& hud, std::vector<SignMarkerPlace>& out) {
    // The rows of the trail stand on the middle band and are asked while the mode arrive shows them, as hit() asks.
    out.clear();
    if (!hud.shows(VivariumHud::LAYER_ARRIVE)) {
        return;
    }
    SignMarkerPlace place;
    for (size_t index = 0; hud.trail_.row_place(index, place.rect, place.sign); ++index) {
        out.push_back(place);
    }
}

void VivariumHudPlaces::ground_places(const VivariumHud& hud, std::vector<GroundThingPlace>& out) {
    // hit_world() asks the things on the ground only while the mode lays the world's hits, before the tool place.
    out.clear();
    if (hud.lays(VivariumHud::HITS_WORLD)) {
        out = hud.world_.places();
    }
}

void VivariumHudPlaces::window_places(const VivariumHud& hud, std::vector<BagThingPlace>& pack,
                                      std::vector<BagThingPlace>& pouch, ::godot::Rect2& pack_close,
                                      bool& pack_hidden, ::godot::Rect2& pouch_close, bool& pouch_hidden) {
    // The windows lie over the scene and are asked with the raw point while the bag layer shows, as hit() asks.
    pack.clear();
    pouch.clear();
    bool pack_drawn = false;
    bool pouch_drawn = false;
    hud.window_.close_places(pack_close, pack_drawn, pouch_close, pouch_drawn);
    const bool layer = hud.shows(VivariumHud::LAYER_INVENTORY);
    pack_hidden = !(layer && pack_drawn);
    pouch_hidden = !(layer && pouch_drawn);
    if (!pack_hidden) {
        pack = hud.window_.pack_places();
    }
    if (!pouch_hidden) {
        pouch = hud.window_.pouch_places();
    }
}

void VivariumHudPlaces::grip_places(const VivariumHud& hud, ::godot::Rect2& pack, bool& pack_hidden,
                                    ::godot::Rect2& pouch, bool& pouch_hidden) {
    bool pack_drawn = false;
    bool pouch_drawn = false;
    hud.window_.grip_places(pack, pack_drawn, pouch, pouch_drawn);
    const bool layer = hud.shows(VivariumHud::LAYER_INVENTORY);
    pack_hidden = !(layer && pack_drawn);
    pouch_hidden = !(layer && pouch_drawn);
}

bool VivariumHudPlaces::rack_place(const VivariumHud& hud, ::godot::Rect2& box) {
    // hit_world() answers ACT_RACK only while the mode lays the world's hits, and only off the things lying there.
    return hud.lays(VivariumHud::HITS_WORLD) && hud.world_.rack_place(box);
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD PLACES
 *
 * @file        godot_vivarium_hud_place.cpp
 * @brief       VivariumView - the places a tap reaches the HUD through, logged as lines whenever they change
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5 and 06.11: after each drawn frame (godot_vivarium_hud.cpp:
 *              VivariumView::draw_hud) the view names, in logical pixels, every place the window acceptance taps
 *              or holds: the debug switch on the sky band, the handle or the fold and every chip of the open
 *              debug drawer, the system glyph at the foot of the rail and the ring of the mode sys (Reiniciar),
 *              and the stand of the birch beside its trunk, which a tap selects the tree through (ACT_TREE opens
 *              the mode focus, whose edge the view logs as "hud mode=focus place=N"), and the play chip of the
 *              trail, which a tap leaves the mode arrive through (the edge "hud mode=rest"), the hand's verb chips, the cut
 *              marks of the mode prune, the rows of the mode rack, the markers of the signs, the signs on the rail,
 *              the event rows of the trail and the things on the ground. The lines are written only
 *              when the set changes and close with "debug place end count=N". The HUD is the one source of
 *              these places: each comes from the same rect its hit() asks (godot_hud_compose_place.cpp).
 *              The activity bars stand among them (MIG_ASE_SKILL_RSM B6), and the brook's place, a search of its
 *              own over the terrain, stands in godot_vivarium_hud_place_brook.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-25
 * @version     00.00.40.00040 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_compose_place.hpp>
#include <ase/adp/godot/godot_vivarium_hud_place_brook.hpp>
#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/hud/debug/types.hpp>
#include <ase/adp/godot/hud/drag/types.hpp>
#include <ase/adp/godot/hud/hand/types.hpp>
#include <ase/adp/godot/patch/types.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The part a chip answers as: the word of its act and its index, UINT32_MAX where the act carries none. */
const char* place_part(const DebugLayerHit& act, uint32_t& index) {
    index = UINT32_MAX;
    if (act.handle) {
        return "handle";
    }
    if (act.fold) {
        return "fold";
    }
    if (act.tab != GODOT_HUD_DEBUG_NONE) {
        index = act.tab;
        return "tab";
    }
    if (act.view != GODOT_HUD_DEBUG_NONE) {
        index = act.view;  // GODOT_HUD_DEBUG_VIEW_*: 0 SYSTEM, 1 RSM, 2 STATUS (usecase:101305)
        return "view";
    }
    if (act.button != GODOT_HUD_DEBUG_NONE) {
        index = act.button;
        return "button";
    }
    if (act.jump != GODOT_HUD_DEBUG_NONE) {
        index = act.jump;
        return "jump";
    }
    if (act.step != GODOT_HUD_DEBUG_NONE) {
        index = act.step;
        return "step";
    }
    if (act.go) {
        return "go";
    }
    if (act.now) {
        return "now";
    }
    if (act.field != GODOT_DATE_PART_NONE) {
        index = act.field;  // the GODOT_DATE_PART_* the cell picks
        return "field";
    }
    if (act.key != GODOT_HUD_DEBUG_NONE) {
        index = act.key;
        return "key";
    }
    if (act.clear) {
        return "clear";
    }
    if (act.relief != GODOT_HUD_DEBUG_NONE) {
        index = act.relief;
        return "relief";
    }
    return "none";
}

/** One place line: the part, its index where it has one, the key of a verb or a thing where it has one, the rect
 *  in logical pixels. */
std::string place_line(const char* part, uint32_t index, const std::string& key, const ::godot::Transform2D& frame,
                       const ::godot::Rect2& rect) {
    const ::godot::Vector2 from = frame.xform(rect.position);
    const ::godot::Vector2 to = frame.xform(rect.position + rect.size);
    std::string line = std::string("debug place part=") + part;
    if (index != UINT32_MAX) {
        line += " index=" + std::to_string(index);
    }
    if (!key.empty()) {
        line += " key=" + key;
    }
    line += " x=" + std::to_string(static_cast<int64_t>(from.x)) + " y=" + std::to_string(static_cast<int64_t>(from.y)) +
            " w=" + std::to_string(static_cast<int64_t>(to.x - from.x)) +
            " h=" + std::to_string(static_cast<int64_t>(to.y - from.y));
    return line;
}

}  // namespace

void VivariumView::note_hud_places() {
    // The places a tap reaches the debug layer, Reiniciar and the tree through, in logical pixels: the switch on
    // the band, the handle or the fold and every chip of the open drawer; the system glyph at the foot of the
    // rail, which a long press opens the mode sys with, and in that mode the ring held for Reiniciar; the stand
    // of the birch beside its trunk, which a tap selects the tree through while the mode lays the tree, and the
    // trunk itself while the mode lays it (sap and plug, `trunk`), which a tap bores or pegs; the play
    // chip of the trail, which a tap leaves the mode arrive through while the trail stands; the sky band's rect,
    // which a tap opens and closes the sky sheet through, and while the sheet stands every hourglass chip (`glass
    // key=<factor>`), which a tap sets the pace through; the bag icon's disc, which a tap opens the pack
    // through; the box round the thing in hand, which a hold pours a can through, its stow button and its verb chips (`verb
    // key=<verb>`); every chip of the fan in focus, `rack` where the tool has no item and `take` where it has one;
    // in the mode prune every cut mark (`cut index=<internode>`), in the mode rack every row of the tool place
    // (`row key=<item>`, the item id, since the thing keys k<N> name no tool); while the markers show, every
    // marker's tap target (`sign index=<sign>`), which hit() asks before the tree, so the tree's stand ends where
    // one begins; in the mode focus the square round the ring (`focus`), the one way back to rest, since a tap on
    // the birch there selects it again and opens no edge; while the rail shows, the target of each of its signs
    // (`rail index=<sign>`), which opens that sign; in the mode arrive each event row of the trail (`event
    // index=<sign>`, no index where the row rests the view); and while the mode lays the world's hits, the padded
    // box of each thing on the ground (`ground key=<item>`), which a tap picks up; in the mode water the middle of
    // the brook's line (`brook`), which a tap fills the can of the hand at; and the figure's controls of Phase 07
    // (`cam_mode`, `stick`, `orbit`, `zoom`, `figure index=<object>`, `soul index=<object> key=self|ai`). The view
    // is the one source of these places; the window acceptance taps and holds where these lines name them.
    // While the bag layer shows, every activity bar (`bars index=<action>`, `key=lit` on the lit ones) is one more
    // place: a hold there lights every bar of its action and shows their sum (MIG_ASE_SKILL_RSM B6).
    ::godot::Rect2 debug_switch;
    std::vector<DebugLayerChip> chips;
    std::string failed;
    if (!VivariumHudPlaces::debug_places(hud_state_.hud_, hud_state_.hud_now_, debug_switch, chips, failed)) {
        fail_hud("debug places " + failed);
        return;
    }
    ::godot::Rect2 foot;
    ::godot::Rect2 ring;
    ::godot::Rect2 close;
    bool foot_hidden = true;
    bool ring_hidden = true;
    VivariumHudPlaces::system_places(hud_state_.hud_, foot, foot_hidden, ring, ring_hidden, close);
    ::godot::Rect2 tree;
    bool tree_hidden = true;
    VivariumHudPlaces::tree_place(hud_state_.hud_, hud_state_.hud_now_, tree, tree_hidden);
    ::godot::Rect2 trunk;
    bool trunk_hidden = true;
    VivariumHudPlaces::trunk_place(hud_state_.hud_, hud_state_.hud_now_, trunk, trunk_hidden);
    ::godot::Rect2 play;
    bool play_hidden = true;
    VivariumHudPlaces::arrive_place(hud_state_.hud_, play, play_hidden);
    ::godot::Rect2 sky;
    bool sky_hidden = true;
    VivariumHudPlaces::band_place(hud_state_.hud_, sky, sky_hidden);
    ::godot::Rect2 arc;
    ::godot::Rect2 needle;
    bool arc_hidden = true;
    VivariumHudPlaces::arc_place(hud_state_.hud_, arc, needle, arc_hidden);
    ::godot::Rect2 bag;
    bool bag_hidden = true;
    HandThingPlace hand;
    bool hand_hidden = true;
    VivariumHudPlaces::bottom_places(hud_state_.hud_, bag, bag_hidden, hand, hand_hidden);
    std::vector<ActivityBarPlace> bars;
    VivariumHudPlaces::bars_places(hud_state_.hud_, bars);
    ::godot::Rect2 band_switch;
    bool band_hidden = true;
    uint32_t band_shows = GODOT_RSM_BAND_SLOTS;
    VivariumHudPlaces::band_place(hud_state_.hud_, band_switch, band_hidden, band_shows);
    // The places a drag is carried to on the bottom band, while it shows: the hand's drop square and both holsters.
    // DragLayer::aim asks them at the finger less cover.hand, so they stand that far down in the middle band's frame.
    std::vector<std::pair<std::string, ::godot::Rect2>> drops;
    if (!bag_hidden) {
        drops.emplace_back(GODOT_HUD_PLACE_HAND, hud_state_.hud_.drag().hand_drop());
        hud_state_.hud_.belt().holster_places(drops);
        for (std::pair<std::string, ::godot::Rect2>& drop : drops) {
            drop.second.position.y += static_cast<float>(hud_state_.hud_now_.cover.hand);
        }
    }
    std::vector<ActionFanPlace> fan;
    VivariumHudPlaces::fan_places(hud_state_.hud_, fan);
    ::godot::Rect2 ring_focus;
    bool focus_hidden = true;
    VivariumHudPlaces::focus_place(hud_state_.hud_, ring_focus, focus_hidden);
    std::vector<PruneMarkPlace> cuts;
    std::vector<RackToolPlace> rows;
    VivariumHudPlaces::mode_places(hud_state_.hud_, cuts, rows);
    std::vector<SignMarkerPlace> signs;
    VivariumHudPlaces::sign_places(hud_state_.hud_, signs);
    std::vector<SignMarkerPlace> rail;
    VivariumHudPlaces::rail_places(hud_state_.hud_, rail);
    std::vector<SignMarkerPlace> events;
    VivariumHudPlaces::event_places(hud_state_.hud_, events);
    std::vector<GroundThingPlace> ground;
    VivariumHudPlaces::ground_places(hud_state_.hud_, ground);
    std::vector<BagThingPlace> pack;
    std::vector<BagThingPlace> pouch;
    ::godot::Rect2 pack_close;
    ::godot::Rect2 pouch_close;
    bool pack_hidden = true;
    bool pouch_hidden = true;
    VivariumHudPlaces::window_places(hud_state_.hud_, pack, pouch, pack_close, pack_hidden, pouch_close, pouch_hidden);
    ::godot::Rect2 pack_grip;
    ::godot::Rect2 pouch_grip;
    bool pack_grip_hidden = true;
    bool pouch_grip_hidden = true;
    VivariumHudPlaces::grip_places(hud_state_.hud_, pack_grip, pack_grip_hidden, pouch_grip, pouch_grip_hidden);
    ::godot::Rect2 tool_rack;
    const bool rack_shown = VivariumHudPlaces::rack_place(hud_state_.hud_, tool_rack);
    const std::string none;
    std::vector<std::string> lines;
    lines.push_back(place_line("switch", UINT32_MAX, none, hud_state_.hud_frame_, debug_switch));
    if (!foot_hidden) {
        lines.push_back(place_line("system", UINT32_MAX, none, hud_state_.hud_frame_, foot));
    }
    if (!ring_hidden) {
        lines.push_back(place_line("ring", UINT32_MAX, none, hud_state_.hud_frame_, ring));
        lines.push_back(place_line("close", UINT32_MAX, none, hud_state_.hud_frame_, close));  // the one way out of the mode sys
    }
    if (!tree_hidden) {
        lines.push_back(place_line("tree", UINT32_MAX, none, hud_state_.hud_frame_, tree));
    }
    if (!trunk_hidden) {
        lines.push_back(place_line("trunk", UINT32_MAX, none, hud_state_.hud_frame_, trunk));  // a tap here bores or pegs it
    }
    if (!play_hidden) {
        lines.push_back(place_line("play", UINT32_MAX, none, hud_state_.hud_frame_, play));
    }
    if (!sky_hidden) {
        lines.push_back(place_line("sky", UINT32_MAX, none, hud_state_.hud_frame_, sky));
    }
    if (!arc_hidden) {
        lines.push_back(place_line(GODOT_ARC_PLACE_ARC, UINT32_MAX, none, hud_state_.hud_frame_, arc));  // a tap here opens the sky sheet
        lines.push_back(place_line(GODOT_ARC_PLACE_NEEDLE, UINT32_MAX, none, hud_state_.hud_frame_, needle));  // the fixed needle
    }
    if (!bag_hidden) {
        lines.push_back(place_line("bag", UINT32_MAX, none, hud_state_.hud_frame_, bag));
    }
    if (!hand_hidden) {
        lines.push_back(place_line("hand", UINT32_MAX, none, hud_state_.hud_frame_, hand.box));  // a hold here pours a can in hand
        lines.push_back(place_line("stow", UINT32_MAX, none, hud_state_.hud_frame_, hand.stow));  // a tap here puts it away
        for (const std::pair<std::string, ::godot::Rect2>& verb : hand.verbs) {
            lines.push_back(place_line("verb", UINT32_MAX, verb.first, hud_state_.hud_frame_, verb.second));  // chooses the verb
        }
    }
    for (const ActivityBarPlace& bar : bars) {
        // A hold here lights every bar of the action (index) and shows their sum; key=lit marks the lit ones (B6).
        const ::godot::Rect2 rect(static_cast<float>(bar.x), static_cast<float>(bar.y), static_cast<float>(bar.w),
                                  static_cast<float>(bar.h));
        lines.push_back(place_line("bars", bar.action, bar.lit ? std::string("lit") : none, hud_state_.hud_frame_, rect));
    }
    // The band switch (operator order 2026-11-25): a tap here turns the bottom band; key names what it shows now,
    // index its GODOT_RSM_BAND_* value, so the acceptance reads the content before and after the tap.
    if (!band_hidden) {
        lines.push_back(place_line("band", band_shows,
                                   band_shows == GODOT_RSM_BAND_BARS ? std::string("bars") : std::string("slots"),
                                   hud_state_.hud_frame_, band_switch));
    }
    for (const std::pair<std::string, ::godot::Rect2>& drop : drops) {
        lines.push_back(place_line("drop", UINT32_MAX, drop.first, hud_state_.hud_frame_, drop.second));  // a drag carried here
    }
    for (const ActionFanPlace& chip : fan) {
        lines.push_back(place_line(chip.act.rack ? "rack" : "take", chip.act.tool, none, hud_state_.hud_frame_, chip.rect));
    }
    if (!focus_hidden) {
        lines.push_back(place_line("focus", UINT32_MAX, none, hud_state_.hud_frame_, ring_focus));  // a tap here goes to rest
    }
    // The figure (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07-G), in the mockup's units of the same frame: cam.mode in every
    // camera (`cam_mode`); while the player camera stands the left half a finger takes the stick in (`stick`) and
    // the right half a drag turns the camera in (`orbit`), since the fixed view takes neither; while the seat's
    // figure stands the square round its reach (`figure`), whose middle opens and closes its focus; while that
    // focus is open the square round the comb soul.mode (`soul`), whose middle hands the figure to the AI or back.
    const VivariumFigureControls& figure = figure_state_.figure_controls_;
    lines.push_back(place_line("cam_mode", UINT32_MAX, none, hud_state_.hud_frame_, figure.cam_switch.place()));
    if (figure_state_.figure_view_shown_) {
        lines.push_back(place_line("stick", UINT32_MAX, none, hud_state_.hud_frame_, figure.stick_zone));
        lines.push_back(place_line("orbit", UINT32_MAX, none, hud_state_.hud_frame_, figure.orbit_zone));
        lines.push_back(place_line("zoom", UINT32_MAX, none, hud_state_.hud_frame_, figure.zoom.place()));  // cam.zoom's rail
    }
    ::godot::Rect2 reach;
    if (figure.led_known && figure.focus.reach_place(reach)) {
        lines.push_back(place_line("figure", figure.led, none, hud_state_.hud_frame_, reach));
    }
    ::godot::Rect2 comb;
    if (figure.focus_open && figure.focus.comb_place(comb)) {
        lines.push_back(place_line("soul", figure.led, figure.led_self ? "self" : "ai", hud_state_.hud_frame_, comb));
    }
    for (const PruneMarkPlace& cut : cuts) {
        lines.push_back(place_line("cut", cut.internode, none, hud_state_.hud_frame_, cut.rect));  // a tap here cuts it
    }
    for (const RackToolPlace& row : rows) {
        lines.push_back(place_line("row", UINT32_MAX, row.id, hud_state_.hud_frame_, row.hit));  // a tap here equips it
    }
    for (const SignMarkerPlace& sign : signs) {
        lines.push_back(place_line("sign", sign.sign, none, hud_state_.hud_frame_, sign.rect));  // a tap here opens the sign
    }
    for (const SignMarkerPlace& sign : rail) {
        lines.push_back(place_line("rail", sign.sign, none, hud_state_.hud_frame_, sign.rect));  // a tap opens the rail's sign
    }
    for (const SignMarkerPlace& event : events) {
        lines.push_back(place_line("event", event.sign, none, hud_state_.hud_frame_, event.rect));  // opens its sign or rests
    }
    // The things on the ground by their item id, as the windows name theirs: a tap picks the thing up.
    for (const GroundThingPlace& place : ground) {
        for (const VivariumHudThing& thing : hud_state_.hud_now_.things) {
            if (thing.key == place.key) {
                lines.push_back(place_line("ground", UINT32_MAX, thing.id, hud_state_.hud_frame_, place.hit));
            }
        }
    }
    // In the mode water a point of the first brook's bed under the middle of its line over the drawn surface
    // (`brook`), the point a tap on the ground asks brookTap at (ACT_GROUND sends ASK_BROOK there); the search is
    // brook_place (godot_vivarium_hud_place_brook.cpp). A column that lands no pixel names itself in `unplaced`.
    std::string unplaced;
    const ::godot::PackedVector2Array& brook = hud_state_.hud_now_.signs.brook;
    const std::vector<VivariumStreamView>& streams = resources_.get_streams();
    ::godot::Camera3D* camera = node_->get_viewport()->get_camera_3d();
    if (hud_state_.hud_.mode() == GODOT_HUD_MODE_WATER && brook.size() >= 2 && !streams.empty() && camera != nullptr) {
        ::godot::Rect2 tap;
        std::string detail;
        if (brook_place(hud_state_.hud_ground_, hud_state_.hud_anchor_, hud_state_.hud_frame_,
                        camera->get_global_transform(), brook, streams.front(), resources_.get_terrain(),
                        scene_state_.pick_px_, tap, detail, unplaced)) {
            lines.push_back(place_line("brook", UINT32_MAX, none, hud_state_.hud_frame_, tap) + detail);
        }
    }
    // The open windows: each thing by its item id (the thing keys k<N> name no item), a tap chooses it, a double
    // tap equips it or opens the pouch; the crosses close the pack and the pouch.
    for (const std::pair<const char*, const std::vector<BagThingPlace>*> window :
         {std::pair<const char*, const std::vector<BagThingPlace>*>("pack", &pack),
          std::pair<const char*, const std::vector<BagThingPlace>*>("pouch", &pouch)}) {
        for (const BagThingPlace& place : *window.second) {
            for (const VivariumHudThing& thing : hud_state_.hud_now_.things) {
                if (thing.key == place.key) {
                    lines.push_back(place_line(window.first, UINT32_MAX, thing.id, hud_state_.hud_frame_, place.box));
                }
            }
        }
    }
    if (!pack_hidden) {
        lines.push_back(place_line("packclose", UINT32_MAX, none, hud_state_.hud_frame_, pack_close));  // a tap closes the pack
    }
    if (!pouch_hidden) {
        lines.push_back(place_line("pouchclose", UINT32_MAX, none, hud_state_.hud_frame_, pouch_close));  // closes the pouch
    }
    if (!pack_grip_hidden) {
        lines.push_back(place_line("packgrip", UINT32_MAX, none, hud_state_.hud_frame_, pack_grip));  // a drag moves the pack
    }
    if (!pouch_grip_hidden) {
        lines.push_back(place_line("pouchgrip", UINT32_MAX, none, hud_state_.hud_frame_, pouch_grip));  // a drag moves the pouch
    }
    if (rack_shown) {
        lines.push_back(place_line("toolrack", UINT32_MAX, none, hud_state_.hud_frame_, tool_rack));  // a tap opens the tool place
    }
    for (const DebugLayerChip& chip : chips) {
        uint32_t index = UINT32_MAX;
        const char* part = place_part(chip.act, index);
        lines.push_back(place_line(part, index, none, hud_state_.hud_frame_, chip.rect));
    }
    std::string joined;
    for (const std::string& line : lines) {
        joined += line;
        joined += '\n';
    }
    joined += unplaced;
    if (joined == hud_state_.hud_places_) {
        return;
    }
    hud_state_.hud_places_ = joined;
    if (!unplaced.empty()) {
        note(::godot::String::utf8(unplaced.c_str()));
    }
    for (const std::string& line : lines) {
        note(::godot::String::utf8(line.c_str()));
    }
    note(::godot::String::utf8(("debug place end count=" + std::to_string(lines.size())).c_str()));
}

}  // namespace ase::adp::godot

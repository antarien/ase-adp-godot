#pragma once

/**
 * ASE GODOT ADAPTER - HUD COMPOSE PLACE
 *
 * @file        godot_hud_compose_place.hpp
 * @brief       VivariumHudPlaces: where a tap lands on VivariumHud, the places the view's place lines name
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, see godot_hud_compose.hpp. VivariumHud::hit() asks its
 *              blocks for what a touch hits; this unit answers the other question, where on the frame a tap would
 *              land on each part, so that VivariumView::note_hud_places can name the places and the window
 *              acceptance taps only there (synt_hud_place_lines). Every function answers the rect hit() or
 *              hit_world() asks, in the middle band's mockup units, shifted by the same band offsets draw() used,
 *              and only while the layer or the hit area the mode lays shows it.
 *
 *              A FRIEND OF THE COMPOSITION, NOT A COPY. The places read VivariumHud's blocks and its resolved mode
 *              through `friend struct VivariumHudPlaces`; they keep no state and compute no rect a block does not
 *              already give. The tree's and the trunk's places are trimmed by reachable(): the part of a world place
 *              that lies on the surface, above the open drawer and beside the fan, the focus ring and the markers,
 *              which hit() asks before the world.
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

#include <ase/adp/godot/godot_hud_compose.hpp>

#include <godot_cpp/variant/rect2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The places a tap reaches the parts of VivariumHud through. */
struct VivariumHudPlaces {
    /** The places a tap reaches the debug layer through (mockup units), the same hit() asks: the sky band's
     *  debug switch in `debug_switch`, and while the layer shows its handle, or its fold, tabs and chips, in
     *  `out`; false with the reason in `failed`. */
    static bool debug_places(const VivariumHud& hud, const VivariumHudFrame& now, ::godot::Rect2& debug_switch,
                             std::vector<DebugLayerChip>& out, std::string& failed);

    /** The places Reiniciar is reached through (mockup units), as hit() asks: the system glyph at the rail's foot
     *  in `foot` while the rail shows, else true in `foot_hidden`; the ring's hold target in `ring` while the mode
     *  sys shows it, else true in `ring_hidden`; the close target leaving the mode in `close`, hidden with it. */
    static void system_places(const VivariumHud& hud, ::godot::Rect2& foot, bool& foot_hidden, ::godot::Rect2& ring,
                              bool& ring_hidden, ::godot::Rect2& close);

    /** The place a tap selects the tree through (mockup units), as hit_world() asks: the birch's stand beside its
     *  trunk, which answers the trunk first, while the tree is placed and the mode lays it, else true in `hidden`.
     *  It stays on the surface and ends where the open drawer or a sign's marker begins (hit() asks both first). */
    static void tree_place(const VivariumHud& hud, const VivariumHudFrame& now, ::godot::Rect2& tree, bool& hidden);

    /** The place a tap bores or pegs the birch through (mockup units), as hit_world() asks it before the stand: the
     *  trunk's rect while the tree is placed and the mode lays the trunk (sap, plug), trimmed like the tree's place,
     *  else true in `hidden`. */
    static void trunk_place(const VivariumHud& hud, const VivariumHudFrame& now, ::godot::Rect2& trunk, bool& hidden);

    /** The place a tap leaves the mode arrive through (mockup units): the play chip of the trail, resting the view
     *  on the tree, while the mode shows the trail and the chip stood in the last draw, else true in `hidden`. */
    static void arrive_place(const VivariumHud& hud, ::godot::Rect2& play, bool& hidden);

    /** The place a tap opens and closes the sky sheet through (mockup units): the sky band's rect beside its
     *  debug switch, the same hit() asks, while the band shows, else true in `hidden`. */
    static void band_place(const VivariumHud& hud, ::godot::Rect2& sky, bool& hidden);

    /** The bottom band's places (mockup units, the same hit() asks), true in `*_hidden` where not drawn: the bag
     *  disc's square, a tap opens or closes the pack; the thing in hand, a press pours a can, and its stow. */
    static void bottom_places(const VivariumHud& hud, ::godot::Rect2& bag, bool& bag_hidden, HandThingPlace& hand,
                              bool& hand_hidden);

    /** The places a tap takes a tool or opens the tool place through (mockup units): every chip of the fan drawn
     *  last, the same hit() asks, while the focus shows; empty otherwise. mode_places: the cut marks while the
     *  mode prune shows them, the rows of the tool place while the mode rack shows it, as hit() asks them.
     *  sign_places: the square round each marker while the markers or an open sign show, as hit() asks them.
     *  focus_place: the square round the focus ring while the focus shows, the one way back to the mode rest. */
    static void fan_places(const VivariumHud& hud, std::vector<ActionFanPlace>& out);
    static void focus_place(const VivariumHud& hud, ::godot::Rect2& ring, bool& hidden);
    static void mode_places(const VivariumHud& hud, std::vector<PruneMarkPlace>& cuts, std::vector<RackToolPlace>& rows);
    static void sign_places(const VivariumHud& hud, std::vector<SignMarkerPlace>& out);

    /** The places a tap opens a sign or picks a thing up through (mockup units), as hit() and hit_world() ask
     *  them. rail_places: the target of each sign on the rail while the rail shows, keyed by the sign it opens
     *  (HudShown::signs in the rail's order). event_places: the row of each event of the trail while the mode
     *  arrive shows it, keyed by the sign it opens (UINT32_MAX rests). ground_places: the padded hit box of each
     *  thing on the ground while the mode lays the world's hits, the topmost last as the draw left them. */
    static void rail_places(const VivariumHud& hud, std::vector<SignMarkerPlace>& out);
    static void event_places(const VivariumHud& hud, std::vector<SignMarkerPlace>& out);
    static void ground_places(const VivariumHud& hud, std::vector<GroundThingPlace>& out);

    /** The places of the open windows (mockup units, the raw point hit() asks them with), while the bag layer
     *  shows: every thing of the pack and of the pouch (key = the thing key, VivariumHudThing::key), a tap
     *  chooses it and a double tap equips it or opens the pouch; the crosses, true in `*_hidden` where not drawn. */
    static void window_places(const VivariumHud& hud, std::vector<BagThingPlace>& pack,
                              std::vector<BagThingPlace>& pouch, ::godot::Rect2& pack_close, bool& pack_hidden,
                              ::godot::Rect2& pouch_close, bool& pouch_hidden);

    /** The grips of the open windows (BagWindow::grip_places) while the bag layer shows; hidden ones stay out. */
    static void grip_places(const VivariumHud& hud, ::godot::Rect2& pack, bool& pack_hidden, ::godot::Rect2& pouch,
                            bool& pouch_hidden);

    /** The tool place's hit box while the world answers taps (hit_world: ACT_RACK); false when it does not. */
    static bool rack_place(const VivariumHud& hud, ::godot::Rect2& box);

private:
    /** The part of a world place (mockup units) a tap reaches: on the surface, above the open drawer and beside the
     *  fan's chips, the focus ring and the signs' markers, which hit() asks before the world; empty when none is. */
    static ::godot::Rect2 reachable(const VivariumHud& hud, const VivariumHudFrame& now, const ::godot::Rect2& place);
};

}  // namespace ase::adp::godot

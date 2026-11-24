#pragma once

/**
 * ASE GODOT ADAPTER - HUD COMPOSE
 *
 * @file        godot_hud_compose.hpp
 * @brief       VivariumHud: the composition of every HUD building block, its layers, its hits and its gestures
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, VivariumHud, called from VivariumView::draw and tap:
 *              the one class that owns every building block of the care view, binds them all from one UiTheme,
 *              asks the ContextResolver what the mode shows, draws the layers in the order hud.modes names them,
 *              finds what a touch hits from the topmost layer down, tells the eight gestures of the hand apart and
 *              carries out the acts the mockup's doAct carries out on the view.
 *
 *              THE LAYERS. draw() takes HudShown::layers in their order and draws each through its block: band
 *              SkyBand, rail UiEdgeRail, body SurvivalBodyRail (the body rail CORPO at the right edge) with
 *              SurvivalWarmthScale (the scale CONFORTO over its heat row), markers and
 *              sign_open SignMarker, focus FocusCluster with ActionFan,
 *              prune_marks PruneMarks, root_cut RootCut, reticle TerrainKit, arrive TrailView, sky CalendarSheet,
 *              system SystemRing, rack the list of ToolRack, inventory the hand (HandSlot), the belt (BeltHolster),
 *              the bag (BagIcon), the Toast (the mockup's hand() closes with it, m.html 749) and the windows
 *              (BagWindow, pack then pouch), debug DebugLayer, drag DragLayer. Under every layer, in the mockup's order: the things in the world (WorldDrop,
 *              ToolRack in the world) wherever the mode lays the world (hud.modes.<mode>.hits names world), then
 *              the WeatherVeil, then the ScreenShade; the world's hits (the mockup's bg, tree, trunk and the
 *              ground's rack and things) lie under all of them for the hits of the mode.
 *
 *              THE BANDS. `frame` is the middle band's frame. VivariumHudFrame::cover carries the surface and the
 *              offsets of the outer bands in its units: the sky band (zone band) draws and hits `head` off it,
 *              the hand, the belt, the bag icon and the toast (zone hand) `hand` off it; every other block stays
 *              on it, and the dims, the drawer, the veil and the shade reach the surface. The places the probe
 *              taps through are given in the middle band's units, shifted by the same offsets.
 *
 *              FIVE UNITS AND THE CONTRACT. godot_hud_compose.cpp binds, resolves and fills the frame;
 *              godot_hud_compose_draw.cpp draws the layers; godot_hud_compose_hit.cpp finds what a touch hits;
 *              godot_hud_compose_gesture.cpp tells the gestures apart and carries out the acts;
 *              godot_hud_compose_place.cpp (VivariumHudPlaces, a friend) names where a tap lands on each part.
 *              What crosses between the view and this class - the frame, the hit, the request - stands in
 *              godot_hud_compose_frame.hpp; the steps of the drawing and the CPU time each took (P0692) stand in
 *              godot_hud_compose_share.hpp.
 *
 *              THE HITS. hit() asks the blocks in the reverse order of drawing and gives the first that answers,
 *              with the mockup's data-act id in VivariumHudHit::act, the item's key, the index of a sign, a tool,
 *              a shoot or a debug part. The ids are those of GODOT_HUD_ACT_* where they exist and the members
 *              ACT_* of this class where only the composition names them.
 *
 *              THE GESTURES. press, move, release and tick turn the touches into the mockup's gestures: a tap; a
 *              double tap on the same thing within hud_gesture_double_tap_ms; the long press on the system glyph
 *              (hud_gesture_rail_press_ms, opens the mode sys) and on the ground below the tree's foot
 *              (hud_gesture_ground_press_ms, opens the tool place), cancelled past hud_gesture_press_slop; the
 *              hold of the can (pours each hud_gesture_pour_step_ms) and of the system ring (resets after
 *              hud_gesture_system_hold_ms); the drag of a thing past hud_gesture_drag_slop and of a window by its
 *              grip; the swipe down of hud_gesture_put_away_swipe from below hud_gesture_put_away_zone that puts
 *              the tool away; and in the mode terrain the brush that follows the finger above
 *              hud_gesture_brush_max_y.
 *
 *              WHAT GOES TO THE MODULE. The view's own state (the mode, the open sign, the chosen thing, the
 *              debug switch, the drawer, the tab, the windows) changes here. Every act on the world (take a tool,
 *              equip, stow, send a thing, sow, water the brook, tap the trunk, plug it, cut a shoot, pour, apply a
 *              verb, ask a pace, a jump, a date, a relief, reset the patch) leaves as one VivariumHudRequest for
 *              the view to send over its port; no game value is ever made here.
 *
 *              THE HIT AREAS OF THE TREE AND THE TRUNK. hud.modes.tree_hit and hud.modes.trunk_hit carry the
 *              stand of the birch and of its trunk in pixels of the logical surface (logical_size.width over
 *              hud_frame_width mockup units); load() reads them and turns them into mockup units.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_action_fan.hpp>
#include <ase/adp/godot/godot_hud_bag_icon.hpp>
#include <ase/adp/godot/godot_hud_bag_window.hpp>
#include <ase/adp/godot/godot_hud_belt_holster.hpp>
#include <ase/adp/godot/godot_hud_calendar_sheet.hpp>
#include <ase/adp/godot/godot_hud_cloud_bank.hpp>
#include <ase/adp/godot/godot_hud_compose_frame.hpp>
#include <ase/adp/godot/godot_hud_compose_press.hpp>
#include <ase/adp/godot/godot_hud_compose_share.hpp>
#include <ase/adp/godot/godot_hud_context_resolver.hpp>
#include <ase/adp/godot/godot_hud_debug_layer.hpp>
#include <ase/adp/godot/godot_hud_drag_layer.hpp>
#include <ase/adp/godot/godot_hud_focus_cluster.hpp>
#include <ase/adp/godot/godot_hud_hand_slot.hpp>
#include <ase/adp/godot/godot_hud_leather_pack.hpp>
#include <ase/adp/godot/godot_hud_linen_pouch.hpp>
#include <ase/adp/godot/godot_hud_prune_marks.hpp>
#include <ase/adp/godot/godot_hud_replay.hpp>
#include <ase/adp/godot/godot_hud_root_cut.hpp>
#include <ase/adp/godot/godot_hud_screen_shade.hpp>
#include <ase/adp/godot/godot_hud_select_label.hpp>
#include <ase/adp/godot/godot_hud_sign_marker.hpp>
#include <ase/adp/godot/godot_hud_sky_band.hpp>
#include <ase/adp/godot/godot_hud_sky_overcast.hpp>
#include <ase/adp/godot/godot_hud_sow_hole.hpp>
#include <ase/adp/godot/godot_hud_survival_body_rail.hpp>
#include <ase/adp/godot/godot_hud_survival_fire_card.hpp>
#include <ase/adp/godot/godot_hud_survival_rail_states.hpp>
#include <ase/adp/godot/godot_hud_survival_rest_card.hpp>
#include <ase/adp/godot/godot_hud_survival_air_mark.hpp>
#include <ase/adp/godot/godot_hud_survival_spoil_ring.hpp>
#include <ase/adp/godot/godot_hud_survival_water_mark.hpp>
#include <ase/adp/godot/godot_hud_survival_warmth_scale.hpp>
#include <ase/adp/godot/godot_hud_system_ring.hpp>
#include <ase/adp/godot/godot_hud_tap_hole.hpp>
#include <ase/adp/godot/godot_hud_tree_shade.hpp>
#include <ase/adp/godot/godot_hud_terrain_kit.hpp>
#include <ase/adp/godot/godot_hud_toast.hpp>
#include <ase/adp/godot/godot_hud_tool_rack.hpp>
#include <ase/adp/godot/godot_hud_trail_view.hpp>
#include <ase/adp/godot/godot_hud_ui_edge_rail.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_weather_veil.hpp>
#include <ase/adp/godot/godot_hud_world_drop.hpp>
#include <ase/adp/godot/godot_vivarium_debug.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The HUD of the care view. */
class VivariumHud {
public:
    // The acts only the composition names (the mockup's data-act ids); the others are GODOT_HUD_ACT_*.
    static constexpr const char* ACT_SIGN = "sign";            // a sign on the rail, a marker or a row of the trail
    static constexpr const char* ACT_TREE = "tree";            // the stand of the birch: the focus opens
    static constexpr const char* ACT_TRUNK = "trunk";          // the trunk in sap and plug
    static constexpr const char* ACT_GROUND = "bg";            // the ground of the world
    static constexpr const char* ACT_RACK = "rack";            // the tool place in the world, a fan chip without item
    static constexpr const char* ACT_TAKE = "take";            // a fan chip or an offer: the tool into the hand
    static constexpr const char* ACT_CUT = "cut";              // a prune mark
    static constexpr const char* ACT_SHEET = "sheet";          // the calendar sheet: its close or a pace chip
    static constexpr const char* ACT_SYSTEM_PRESS = "sysp";    // the system glyph at the foot of the rail
    static constexpr const char* ACT_SYSTEM_HOLD = "sysHold";  // the system ring
    static constexpr const char* ACT_DEBUG_PART = "debug";     // a part of the debug layer
    static constexpr const char* ACT_FOCUS = "focus";          // the focus ring itself: takes the touch

    // The modes of hud.modes the play loop names beside GODOT_HUD_MODE_* (the tool modes and rest).
    static constexpr const char* MODE_ARRIVE = "arrive";       // the trail of what happened while away
    static constexpr const char* MODE_SIGN = "sign";           // a sign opened
    static constexpr const char* MODE_FOCUS = "focus";         // the birch in focus with the fan
    static constexpr const char* MODE_HOLD = "hold";           // a thing in the hand that is no tool
    static constexpr const char* MODE_RACK = "rack";           // the tool place's list
    static constexpr const char* MODE_SKY = "sky";             // the calendar sheet
    static constexpr const char* MODE_SYS = "sys";             // the system ring

    // The layers of hud.modes.
    static constexpr const char* LAYER_BAND = "band";
    static constexpr const char* LAYER_RAIL = "rail";
    static constexpr const char* LAYER_BODY = "body";            // the body rail CORPO at the right edge (U01)
    static constexpr const char* LAYER_MARKERS = "markers";
    static constexpr const char* LAYER_SIGN_OPEN = "sign_open";
    static constexpr const char* LAYER_FOCUS = "focus";
    static constexpr const char* LAYER_PRUNE = "prune_marks";
    static constexpr const char* LAYER_ROOT_CUT = "root_cut";
    static constexpr const char* LAYER_RETICLE = "reticle";
    static constexpr const char* LAYER_ARRIVE = "arrive";
    static constexpr const char* LAYER_SKY = "sky";
    static constexpr const char* LAYER_SYSTEM = "system";
    static constexpr const char* LAYER_RACK = "rack";
    static constexpr const char* LAYER_INVENTORY = "inventory";
    static constexpr const char* LAYER_DEBUG = "debug";
    static constexpr const char* LAYER_DRAG = "drag";
    static constexpr const char* LAYER_TAP_PULSE = "tap_pulse";  // the pulse at the tap hole in sap and plug

    // The hit areas of hud.modes.<mode>.hits.
    static constexpr const char* HITS_WORLD = "world";
    static constexpr const char* HITS_TREE = "tree";
    static constexpr const char* HITS_TRUNK = "trunk";
    static constexpr const char* HITS_GROUND = "ground";

    // The view marks the composition adds to the game state.
    static constexpr const char* MARK_DEBUG_ON = "debug_on";
    static constexpr const char* MARK_DRAWER_OPEN = "drawer_open";
    static constexpr const char* MARK_DRAGGING = "dragging";

    // The port marks of hud.tools.requires the patch record decides: its condition and its stage codes.
    static constexpr const char* MARK_ALIVE = "alive";               // the condition equals a code of flora.cond.v1 and ase-plant marks the stand not dead
    static constexpr const char* MARK_DEAD = "dead";                 // ase-plant marks the stand dead (cond_dead of flora.cond.v1)
    static constexpr const char* MARK_STAGE_GROW = "stage_grow";     // VivariumPatchView::grow, the grow code of flora.stage.v1
    static constexpr const char* MARK_STAGE_MATURE = "stage_mature"; // VivariumPatchView::mature, the mature code of flora.stage.v1

    // The terrain operations of hud.verbs.<verb>.op.
    static constexpr const char* OP_LOWER = "lower";       // cavar: a disc of the ground lowered
    static constexpr const char* OP_RAISE = "raise";       // amontoar: a disc of the ground raised
    static constexpr const char* OP_BED = "bed";           // leito: two points lay a brook's bed (lay port)
    static constexpr const char* OP_LOOSEN = "loosen";     // soltar, furar: the soil loosened
    static constexpr const char* OP_FLATTEN = "flatten";   // aplanar: a disc levelled to its centre (flatten port)
    static constexpr const char* OP_LEVEL = "level";       // nivelar: the patch levelled (level port)
    static constexpr const char* OP_MEASURE = "measure";   // medir: height and slope read under the brush

    // The backend gaps only the view's requests meet (the others stand as GODOT_HUD_GAP_* in types.hpp).
    static constexpr const char* GAP_SKILL = "N0453";      // the skill port: the perception level of the keeper
    static constexpr const char* GAP_FILL = "N0458";       // the can filled at the brook
    static constexpr const char* GAP_MEASURE = HandMode::GAP_MEASURE;  // height and slope under the brush

    // The toasts of the can (m.html 1001, 1015), all in 'water': an arrived pour once the patch port shows the
    // rise, the same with the can emptied by it, and a press on an empty can, which pours nothing.
    static constexpr const char* TOAST_WATERED = "toast.watered";              // {mm}: the soil water the pour added
    static constexpr const char* TOAST_WATERED_EMPTY = "toast.watered_empty";  // {mm}: likewise, the can empty after it
    static constexpr const char* TOAST_POT_EMPTY = "toast.pot_empty";          // REGADOR VAZIO · ENCHA NO RIACHO
    static constexpr const char* COLOUR_WATER = "DOMAIN_WATER";                // their domain colour

    // The toast of a cut (usecase:100365 prune a branch): toast('PODOU UM GALHO', 'cut') once the segment port took it.
    static constexpr const char* TOAST_PRUNED = "toast.pruned";  // PODOU UM GALHO
    static constexpr const char* COLOUR_CUT = "DOMAIN_CUT";      // its domain colour

    // The toasts of the tap (trunkTap, plugTap and the tick that fills the trough), in the mockup's 'flow' or
    // 'alert'.
    static constexpr const char* TOAST_TAPPED = "toast.tapped";            // FURO NO TRONCO · A SEIVA ESCORRE
    static constexpr const char* TOAST_SAP_TAKEN = "toast.sap_taken";      // {l}: the litres the plug closed in
    static constexpr const char* TOAST_VESSEL_FULL = "toast.vessel_full";  // the trough reached its litres
    static constexpr const char* TOAST_NO_VESSEL = "sap_no_vessel";        // FALTA O COCHO
    static constexpr const char* TOAST_NO_SPOUT = "sap_no_spout";          // FALTA A BICA
    static constexpr const char* COLOUR_SAP = "DOMAIN_FLOW";               // the domain colour of the sap
    // The kinds of request the view sends over its port are VivariumHudRequest::ASK_* (godot_hud_compose_frame.hpp).

    /** Reads the theme from the start configuration and binds every block; false with the missing name. */
    bool load(const ::godot::Dictionary& root, std::string& failed);

    /** Applies the context's rules to the frame's game state and the view's own; false with the reason. */
    bool resolve(VivariumHudFrame& frame, std::string& failed);

    /** Draws every layer of the resolved mode under `frame`, then the toast; false with the reason. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, VivariumHudFrame& now,
              std::string& failed);

    /** What `point` (mockup units) hits, the topmost layer first. */
    VivariumHudHit hit(const ::godot::Vector2& point, const VivariumHudFrame& now) const;

    /** A finger comes down at `point` at `clock`; false with the reason in `failed` (a toast word missing). */
    bool press(const ::godot::Vector2& point, const VivariumHudFrame& now, double clock, std::string& failed);

    /** The finger moves to `point`; false with the reason in `failed`. */
    bool move(const ::godot::Vector2& point, VivariumHudFrame& now, std::string& failed);

    /** The finger leaves at `point`; the requests in `out`, false with the reason in `failed`. */
    bool release(const ::godot::Vector2& point, VivariumHudFrame& now, double clock,
                 std::vector<VivariumHudRequest>& out, std::string& failed);

    /** The time passes to `clock` (the HUD's capped clock) and `real` seconds of the frame pass on the wall: the long
     *  presses fire on the wall seconds (the mockup's setTimeout), the holds pour and fill on `clock`; the requests in
     *  `out`. */
    bool tick(double clock, double real, VivariumHudFrame& now, std::vector<VivariumHudRequest>& out,
              std::string& failed);

    /** Posts texts.hud.`key` in `colour`, for a refusal the port answered with; false with the missing name. It stands
     *  hud_toast_ms on the wall clock, as the mockup's until: performance.now() + 2400 (m.html 931) does. */
    bool toast(const std::string& key, const std::vector<HudPortValue>& values, const std::string& colour,
               std::string& failed);

    /** putAway() after an act the view carried out (trunkTap, plugTap): the HUD returns to the focus mode. */
    void put_away();

    /**
     * The keeper came back to the garden (usecase:100390 return to the garden): the press under way is let go and the
     * HUD opens on the mode it starts in (hud.modes.start, the arrival with its trail), as at the start of the app.
     */
    void return_to_garden();

    const UiTheme& theme() const { return theme_; }
    /** The thing `key` among the frame's things, nullptr when none; the view asks it whether a hit lies at the rack. */
    const VivariumHudThing* thing(const std::string& key, const VivariumHudFrame& now) const;
    bool pouring() const { return press_.pouring; }  // S.holding: a finger holds the can, read by the hand bridge before compose
    bool holding_system() const { return press_.system; }  // S.sysHold: a finger holds the system ring, read by the refresh cadence (T10)
    bool holding_window() const { return press_.window; }  // S.wdrag: a grip is held; its move renders, its let go does not (T10)
    bool dragging() const { return drag_.moved(); }        // S.drag.moved: a lifted thing follows the finger and renders (T10)
    const DragLayer& drag() const { return drag_; }  // the thing and the place a stow verdict is asked for
    const BeltHolster& belt() const { return belt_; }  // the holsters a drag is carried to, read by the place lines
    const BagWindow& window() const { return window_; }  // the pack and the pouch with this frame's container records, read by the carry bridge's edge
    const CloudBank& clouds() const { return clouds_; }
    const SkyOvercast& overcast() const { return overcast_; }  // the grey and the dimmed light of an overcast sky, laid over the 3D scene (N01)
    const HudShown& shown() const { return shown_; }
    const std::vector<HudDrawShare>& shares() const { return shares_; }  // the steps of the last draw with their CPU time (P0692)
    const std::string& mode() const { return mode_; }
    const std::string& selected() const { return selected_; }
    bool debug_on() const { return debug_on_; }
    const std::string& verb() const { return verb_; }

    /**
     * The mockup's lastKey (m.html 903): the fades and the drawings play again when the mode, the open sign, the
     * perception `level`, the debug view, its drawer, its tab, the `stage` or the `dead` stand changed since the last
     * call, and stand at their end otherwise. Answers the seconds they have played since. They are CSS animations in
     * the mockup (m.html 523: class="draw", animation-delay) and run on the wall clock, never on tick's capped dt
     * (m.html 1024): the seconds are wall_'s, so a stalled frame delays no fade (run 20261113-v11-sown6, sow_done).
     */
    double replay(uint32_t level, uint8_t stage, bool dead);

    /** The perception level the view starts at (hud.perception.start) while the skill port (GAP_SKILL) is open. */
    uint32_t level_start() const { return resolver_.spec().level_start; }

    /** In the mode terrain the brush follows the finger above hud_gesture_brush_max_y; the view lays it. */
    bool brushing() const { return brushing_; }
    const ::godot::Vector2& brush() const { return brush_; }

private:
    // Where a tap lands on each part, for the view's place lines, reads the blocks and the resolved mode here.
    friend struct VivariumHudPlaces;

    /** The mockup's doAct on `hit` at `point` over the things of `now`; the requests in `out`. */
    bool act(const VivariumHudHit& hit, const ::godot::Vector2& point, const VivariumHudFrame& now,
             std::vector<VivariumHudRequest>& out, std::string& failed);

    /** The mockup's useTool for the tool `index` of hud.tools: its mode, the tool place or its item. */
    bool use_tool(uint32_t index, const ::godot::Vector2& point, const VivariumHudFrame& now,
                  std::vector<VivariumHudRequest>& out, std::string& failed);

    /** Two ids are the same word; nullptr is never the same as anything. */
    static bool same(const char* a, const char* b);

    /** The mockup's tapItem: the hand, the rack, the belt, the ground's tool place, the double tap, the choice. */
    bool tap_item(const VivariumHudHit& hit, double clock, const VivariumHudFrame& now,
                  std::vector<VivariumHudRequest>& out, std::string& failed);

    /** The mockup's setMode: the mode `name`, the open sign and the choice cleared. */
    void set_mode(const std::string& name);

    /**
     * The mockup's equip() on the item `id` coming into the hand: an item that names ground verbs brings its first
     * one as the verb in hand (S.terr = D.verbs[0]), an item without verbs leaves the verb as it stands; false with
     * "hud.items.<id>" in `failed` when hud.items does not describe the item.
     */
    bool equip_verb(const std::string& id, std::string& failed);

    /** Where the thing `key` was last drawn, in `box` at scale `k`; false while it was not drawn. */
    bool box_of(const std::string& key, ::godot::Rect2& box, double& k) const;

    /** The thing in the hand, nullptr when none. */
    const VivariumHudThing* hand_thing(const VivariumHudFrame& now) const;

    /** The hit areas of the world: the tool place, the things on the ground, the trunk, the tree, the ground. */
    VivariumHudHit hit_world(const ::godot::Vector2& point, const VivariumHudFrame& now) const;

    /** The trunk's hit area: its share of the stand above the foot, padded below the foot. */
    ::godot::Rect2 trunk_rect(const VivariumHudFrame& now) const;

    /** The tree's hit area: the stand, at least the smallest half width wide, padded below the foot. */
    ::godot::Rect2 tree_rect(const VivariumHudFrame& now) const;

    /** The things in the frame's possession, the hand and the drag set for the blocks. */
    void fill(VivariumHudFrame& now) const;

    /** The rail's word and signs for this frame; false with the reason. */
    bool set_rail(std::string& failed);

    /** One layer `layer` drawn; false with the reason. */
    bool draw_layer(UiPen& canvas, const ::godot::Transform2D& frame, const std::string& layer,
                    VivariumHudFrame& now, std::string& failed);

    /** The frame of the top band (the zone band): the middle band's `frame` moved by cover_.head. */
    ::godot::Transform2D head_frame(const ::godot::Transform2D& frame) const;

    /** The frame of the bottom band (the zone hand): the middle band's `frame` moved by cover_.hand. */
    ::godot::Transform2D hand_frame(const ::godot::Transform2D& frame) const;

    /** The inventory layer: the hand, the belt, the bag, the windows and the reading of a single tap. */
    bool draw_inventory(UiPen& canvas, const ::godot::Transform2D& frame, VivariumHudFrame& now,
                        std::string& failed);

    /** Writes the CPU time since `cpu_step` into shares_ as `step` and moves `cpu_step` to now (P0692). */
    void share_step(const char* step, uint64_t& cpu_step);

    /** selLabel: the reading of the chosen thing at the box it was drawn in this frame (L01). */
    bool draw_select(UiPen& canvas, const ::godot::Transform2D& frame, const VivariumHudFrame& now,
                     std::string& failed);

    /** The layer `name` shows in the resolved mode. */
    bool shows(const char* name) const;

    /** The hit area `name` lies in the resolved mode. */
    bool lays(const char* name) const;

    UiTheme theme_;
    ContextResolver resolver_;
    HudShown shown_;

    SkyBand band_;
    WeatherVeil veil_;
    CloudBank clouds_;          // the cloud banks of an overcast sky, laid for the sky of the 3D scene (N04)
    SkyOvercast overcast_;      // the grey and the dimmed light of an overcast sky, shaded over the 3D scene (N01)
    ScreenShade shade_;
    UiEdgeRail rail_;
    SurvivalBodyRail body_;     // the body rail CORPO at the right edge (U01, mockup VIVARIUM_PHASE_08 S1)
    SurvivalWarmthScale warmth_;// the scale CONFORTO over the rail's heat row (U03, mockup VIVARIUM_PHASE_08 S3)
    SurvivalRailStates states_; // the states of the body under the signs on the rail SINAIS (U02, mockup VIVARIUM_PHASE_08 S2)
    SurvivalFireCard fire_;     // the card of the camp fire on its leader from the hearth (U04, mockup VIVARIUM_PHASE_08 S4)
    SurvivalRestCard rest_;     // the card of the resting figure on its leader from the figure (U05, mockup VIVARIUM_PHASE_08 S5)
    SurvivalAirMark air_;       // the mark AR TÓXICO in the sky band under the wind (U06, mockup VIVARIUM_PHASE_08 S6)
    SurvivalSpoilRing food_;    // the ring ESTRAGO around the food of the bag (U07, mockup VIVARIUM_PHASE_08 S7)
    SurvivalWaterMark water_;   // the drop at the bag, ÁGUA TURVA while poisoned (U08, mockup VIVARIUM_PHASE_08 S8)
    SignMarker signs_;
    FocusCluster focus_;
    ActionFan fan_;
    PruneMarks prune_;
    RootCut root_cut_;          // the ground opened under the trunk in the mode water
    TerrainKit terrain_;
    TrailView trail_;
    CalendarSheet sheet_;
    SystemRing system_;
    ToolRack rack_;
    WorldDrop world_;
    TapHole tap_;               // the tap hole in the birch, drawn after the tool place (N13)
    SowHole sow_;               // the hole the dibber made, drawn after the tap hole (N14)
    TreeShade shade_tree_;      // the shade under the birch or the seed mound, drawn before the world group (N19)
    HandSlot hand_;
    BeltHolster belt_;
    BagIcon bag_icon_;
    BagWindow window_;
    LeatherPack pack_;
    LinenPouch pouch_;
    SelectLabel select_;        // the reading of a single tap, over the windows (L01)
    DebugLayer debug_;
    DragLayer drag_;
    Toast toast_;
    std::vector<HudDrawShare> shares_;  // the steps of the last draw, cleared at its start, the capacity kept

    std::string rail_label_;            // texts.hud.signs_rail
    HudCover cover_;                    // the frame's cover as resolve() last took it: the places answer by it

    // The view's own state (the mockup's S).
    std::string mode_;                  // S.mode
    std::string open_sign_;             // S.sign
    std::string selected_;              // S.sel
    std::string verb_;                  // S.terr
    bool debug_on_ = false;             // S.debug
    bool drawer_open_ = false;          // S.drawer
    uint32_t debug_tab_ = GODOT_HUD_DEBUG_TAB_TEMPO;  // S.dtab
    std::string last_tap_key_;          // S.lastTap.key
    double last_tap_clock_ = 0.0;       // S.lastTap.t
    bool tapped_ = false;               // S.lastTap is set
    HudReplay replay_;                  // lastKey: the clock the fades and drawings play from (T05)
    double clock_ = 0.0;                // the clock of the last call
    double wall_ = 0.0;                 // the real seconds of the frames, never capped: the long presses' setTimeout
    bool brushing_ = false;            // S.brush: the brush lies under the finger
    ::godot::Vector2 brush_;            // where, in mockup units
    HudPress press_;                    // the press under way (godot_hud_compose_press.hpp)

    // The gestures (hud_gesture_*), in seconds and mockup units.
    double double_tap_s_ = 0.0;
    double rail_press_s_ = 0.0;
    double ground_press_s_ = 0.0;
    double pour_step_s_ = 0.0;
    double press_slop_ = 0.0;
    double put_away_swipe_ = 0.0;
    double put_away_zone_ = 0.0;
    double ground_press_rise_ = 0.0;
    double brush_max_y_ = 0.0;

    // The hit areas of the tree and the trunk (hud.modes.tree_hit, trunk_hit), in mockup units.
    double tree_half_share_ = 0.0;
    double tree_half_min_ = 0.0;
    double tree_min_ = 0.0;
    double tree_pad_ = 0.0;
    double trunk_half_ = 0.0;
    double trunk_share_ = 0.0;
    double trunk_pad_ = 0.0;

    // The ground of the world (the mockup's bg rect): from x, y, w and its heights in rest and terrain.
    double ground_x_ = 0.0;
    double ground_y_ = 0.0;
    double ground_w_ = 0.0;
    double ground_h_ = 0.0;
    double ground_terrain_h_ = 0.0;
};

}  // namespace ase::adp::godot

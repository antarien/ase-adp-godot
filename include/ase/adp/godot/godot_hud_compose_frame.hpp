#pragma once

/**
 * ASE GODOT ADAPTER - HUD COMPOSE FRAME
 *
 * @file        godot_hud_compose_frame.hpp
 * @brief       The contract between the care view and VivariumHud: the frame it hands over, the hit, the request
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, VivariumHud, called from VivariumView::draw and tap:
 *              what crosses between the view and the composition each frame, apart from the composition itself.
 *
 *              THE FRAME. VivariumHudFrame is what the view hands the HUD for one frame: the ports' state (the
 *              game marks, the carried things, the trunk, the clock) and one frame per building block, each filled
 *              by the view or by its bridge while a port is silent. The HUD draws every block from its frame and
 *              never asks whether a port delivered; the few fields the composition sets itself are named beside
 *              each frame. The sky frame is the one the view does not hand to a block of the HUD but to the
 *              scene's SkyOvercast: it carries a kind of weather only while the sky port's colours carry none.
 *
 *              THE HIT AND THE REQUEST. VivariumHudHit is what a touch hits, the topmost layer first, with the
 *              mockup's data-act id; VivariumHudRequest is one act on the world the view sends over its port.
 *              Neither carries a game value the HUD made: the request names the thing, the verb, the tool or the
 *              place, the module answers.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.01.00004
 */

#include <ase/adp/godot/godot_hud_action_fan.hpp>
#include <ase/adp/godot/godot_hud_bag_icon.hpp>
#include <ase/adp/godot/godot_hud_bag_window.hpp>
#include <ase/adp/godot/godot_hud_belt_holster.hpp>
#include <ase/adp/godot/godot_hud_calendar_sheet.hpp>
#include <ase/adp/godot/godot_hud_context_resolver.hpp>
#include <ase/adp/godot/godot_hud_debug_layer.hpp>
#include <ase/adp/godot/godot_hud_drag_layer.hpp>
#include <ase/adp/godot/godot_hud_focus_cluster.hpp>
#include <ase/adp/godot/godot_hud_hand_slot.hpp>
#include <ase/adp/godot/godot_hud_prune_marks.hpp>
#include <ase/adp/godot/godot_hud_root_cut.hpp>
#include <ase/adp/godot/godot_hud_sign_marker.hpp>
#include <ase/adp/godot/godot_hud_sky_band.hpp>
#include <ase/adp/godot/godot_hud_sky_overcast.hpp>
#include <ase/adp/godot/godot_hud_sow_hole.hpp>
#include <ase/adp/godot/godot_hud_tap_hole.hpp>
#include <ase/adp/godot/godot_hud_tree_shade.hpp>
#include <ase/adp/godot/godot_hud_terrain_kit.hpp>
#include <ase/adp/godot/godot_hud_tool_rack.hpp>
#include <ase/adp/godot/godot_hud_trail_view.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_weather_veil.hpp>
#include <ase/adp/godot/godot_hud_world_drop.hpp>
#include <ase/adp/godot/godot_vivarium_debug.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** A carried thing as the ports report it: the key a hit names, the item it is and where it lies. */
struct VivariumHudThing {
    std::string key;             // o.key
    std::string id;              // o.id: the item, the key of hud.items
    std::string art;             // the item_art.json name of its drawing
    const char* from = nullptr;  // o.c: GODOT_HUD_CONTAINER_*, GODOT_HUD_PLACE_HAND or GODOT_HUD_PLACE_GROUND
    bool rack = false;           // o.rack: it lies at the tool place
    double length_cm = 0.0;      // D.len
    double kg = 0.0;             // D.kg: one piece
    double weight_kg = 0.0;      // wt(o): its pieces with what it holds (VivariumCarryBridge::weight)
    double litres = 0.0;         // D.l: one piece
    int64_t count = 0;           // o.n
    bool stack = false;          // D.stack
    double level = 0.0;          // the held share 0..1 of a vessel: o.fill, o.sap in D.art(o)
};

/** What the view hands the HUD for one frame: the ports' state and every block's frame. */
struct VivariumHudFrame {
    HudGameState game;                        // the marks, the possession, the reported signs, the level
    std::vector<VivariumHudThing> things;     // flora.carry.v1: every carried thing, empty while none
    bool tree_placed = false;                 // the birch stands on the screen
    ::godot::Vector2 trunk;                   // the foot of the trunk in mockup units (TX, TY())
    double tree_height = 0.0;                 // treeH() in mockup units
    double clock = 0.0;                       // the seconds of the view's clock
    bool still = false;                       // every motion held
    HudCover cover;                           // REQUIRED: the surface and the bands' offsets, set by the view

    SkyBandFrame band;
    WeatherVeilFrame veil;
    SkyOvercastFrame sky;                     // the weather whose tint the sky port does not carry; sun_y is set by the view
                                              // before the scene is lit (D2280: the sky is tinted exactly once)
    SignMarkerFrame signs;                    // shown is set by the HUD
    FocusClusterFrame focus;
    ActionFanFrame fan;                       // tools is set by the HUD
    PruneMarksFrame prune;
    RootCutFrame root_cut;                    // the ground opened under the trunk in the mode water
    TerrainKitFrame terrain;
    TrailViewFrame trail;
    CalendarSheetFrame sheet;
    ToolRackWorld rack_world;
    ToolRackFrame rack;
    WorldDropFrame world;
    TapHoleFrame tap;                         // the tap hole in the birch while a tap runs (N13)
    SowHoleFrame sow;                         // the hole the dibber made until the seed closes it (N14)
    TreeShadeFrame shade;                     // the shade under the birch, or the seed mound at the stage SEED (N19)
    HandSlotFrame hand;                      // mode, dragged, hot and toast_posted are set by the HUD
    BeltHolsterFrame belt;                    // dragged, hot and refused are set by the HUD
    BagIconFrame bag_icon;                    // open, hot and refused are set by the HUD
    BagWindowFrame window;                    // selected, dragged, hot and refused are set by the HUD
    DebugLayerFrame debug;                    // open and tab are set by the HUD
    const VivariumDebug* debug_data = nullptr;// REQUIRED while the debug layer shows
    DragLayerFrame drag;                      // the block pointers are set by the HUD
};

/** What a touch hits, the topmost layer first. */
struct VivariumHudHit {
    const char* act = nullptr;           // GODOT_HUD_ACT_* or VivariumHud::ACT_*; nullptr: nothing
    std::string key;                     // the thing of GODOT_HUD_ACT_ITEM, the verb of GODOT_HUD_ACT_VERB
    uint32_t index = UINT32_MAX;         // the sign, the tool, the shoot of the act
    const char* from = nullptr;          // the place of the thing: GODOT_HUD_CONTAINER_*, GODOT_HUD_PLACE_*
    CalendarSheetHit sheet;              // ACT_SHEET: what the calendar sheet hit
    DebugLayerHit debug;                 // ACT_DEBUG_PART: what the debug layer hit
};

/** One act the view sends over its port: the mockup's acts on the world. */
struct VivariumHudRequest {
    // The kinds of request the view sends over its port.
    static constexpr const char* ASK_TAKE = "take";            // useTool: the tool's item into the hand
    static constexpr const char* ASK_EQUIP = "equip";          // equip: the thing into the hand
    static constexpr const char* ASK_STOW = "stow";            // putAway: the thing in the hand back where it came from
    static constexpr const char* ASK_SEND = "send";            // a drop: the thing to the place of the target
    static constexpr const char* ASK_SOW = "sow";              // sowTap
    static constexpr const char* ASK_BROOK = "brook";          // brookTap
    static constexpr const char* ASK_TRUNK = "trunk";          // trunkTap
    static constexpr const char* ASK_PLUG = "plug";            // plugTap
    static constexpr const char* ASK_CUT = "cut";              // a shoot cut
    static constexpr const char* ASK_POUR = "pour";            // one more measure poured
    static constexpr const char* ASK_POUR_END = "pour_end";    // the finger left the can
    static constexpr const char* ASK_VERB = "verb";            // applyTerr: the chosen verb under the brush
    static constexpr const char* ASK_DEBUG = "debug";          // a debug part: pace, jump, date, relief
    static constexpr const char* ASK_RESET = "reset";          // resetAll: the system ring was held to its end

    const char* kind = nullptr;          // one of the ASK_* above
    std::string key;                     // the thing, the verb
    uint32_t index = UINT32_MAX;         // the tool, the shoot, the debug part
    ::godot::Vector2 point;              // the touch in mockup units
    DragTarget target;                   // ASK_SEND: where the thing goes
    DebugLayerHit debug;                 // ASK_DEBUG: the debug part
};

}  // namespace ase::adp::godot

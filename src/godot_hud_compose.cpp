/**
 * ASE GODOT ADAPTER - HUD COMPOSE
 *
 * @file        godot_hud_compose.cpp
 * @brief       VivariumHud: binds every building block, resolves the mode and fills the frame of the blocks
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, see godot_hud_compose.hpp. This unit reads what the
 *              composition measures (the gestures, the ground as a touch target, the stand of the tree and of the
 *              trunk), binds the blocks from one UiTheme, asks the ContextResolver what the mode shows and hands
 *              each block its part of the frame: the hand, the belt, the bag and the windows learn what is
 *              dragged, where it is aimed and whether the aim is refused; the drag learns where every place lies.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-25
 * @version     00.00.23.00023 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_compose.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/hud/drag/types.hpp>
#include <ase/adp/godot/hud/touch/types.hpp>

#include <cstring>

namespace ase::adp::godot {

namespace {

/** The SHA token `token` in `out`, or its name in `failed`. */
bool measure(const UiTheme& theme, const char* token, double& out, std::string& failed) {
    if (!theme.measure(token, out)) {
        failed = token;
        return false;
    }
    return true;
}

/** The time of a gesture in seconds, or its token in `failed`: one that is not above zero lets the gesture never act. */
bool gesture_time(const UiTheme& theme, const char* token, double& out_s, std::string& failed) {
    const double ms_per_s = 1000.0;
    double ms = 0.0;
    if (!measure(theme, token, ms, failed)) {
        return false;
    }
    if (!(ms > 0.0)) {
        failed = std::string(token) + ": not above zero";
        return false;
    }
    out_s = ms / ms_per_s;
    return true;
}

/** The number `key` of hud.modes.`area_name` of the start configuration, or its path in `failed`. */
bool hit_number(const ::godot::Dictionary& area, const char* area_name, const char* key, double& out,
                std::string& failed) {
    if (!read_number(area, key, out)) {
        failed = std::string("hud.modes.") + area_name + "." + key;
        return false;
    }
    return true;
}

}  // namespace

bool VivariumHud::same(const char* a, const char* b) {
    return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}

bool VivariumHud::load(const ::godot::Dictionary& root, std::string& failed) {
    if (!theme_.load(root, failed) || !resolver_.set_spec(theme_.spec(), failed)) {
        return false;
    }
    if (!arc_.bind(theme_, failed) || !band_.bind(theme_, failed) || !veil_.bind(theme_, failed) || !clouds_.bind(theme_, failed) ||
        !overcast_.bind(theme_, failed) || !shade_.bind(theme_, failed) ||
        !rail_.bind(theme_, failed) ||
        !survival_.bind(theme_, failed) ||
        !signs_.bind(theme_, failed) || !focus_.bind(theme_, failed) || !fan_.bind(theme_, failed) ||
        !prune_.bind(theme_, failed) || !root_cut_.bind(theme_, failed) || !terrain_.bind(theme_, failed) ||
        !trail_.bind(theme_, failed) ||
        !sheet_.bind(theme_, failed) || !system_.bind(theme_, failed) || !rack_.bind(theme_, failed) ||
        !world_.bind(theme_, failed) || !tap_.bind(theme_, failed) || !sow_.bind(theme_, failed) ||
        !shade_tree_.bind(theme_, failed) ||
        !hand_.bind(theme_, failed) || !belt_.bind(theme_, failed) ||
        !bag_icon_.bind(theme_, failed) || !bars_.bind(theme_, root, failed) || !rings_.bind(theme_, failed) ||
        !window_.bind(theme_, failed) ||
        !pack_.bind(theme_, failed) ||
        !pouch_.bind(theme_, failed) || !select_.bind(theme_, failed) || !debug_.bind(theme_, failed) ||
        !drag_.bind(theme_, failed) ||
        !toast_.bind(theme_, failed)) {
        return false;
    }

    // The rail's word, as plain text: texts.hud.signs_rail carries no icon run.
    std::vector<HudRun> runs;
    if (!theme_.words("signs_rail", runs)) {
        failed = "texts.hud.signs_rail";
        return false;
    }
    rail_label_.clear();
    for (const HudRun& run : runs) {
        rail_label_ += run.text;
    }

    // The gestures: the times in milliseconds turned into seconds, the slops and lines in mockup units.
    if (!gesture_time(theme_, "hud_gesture_double_tap_ms", double_tap_s_, failed) ||
        !gesture_time(theme_, "hud_gesture_rail_press_ms", rail_press_s_, failed) ||
        !gesture_time(theme_, "hud_gesture_ground_press_ms", ground_press_s_, failed) ||
        !gesture_time(theme_, "hud_gesture_pour_step_ms", pour_step_s_, failed) ||
        !measure(theme_, "hud_gesture_press_slop", press_slop_, failed) ||
        !measure(theme_, "hud_gesture_put_away_swipe", put_away_swipe_, failed) ||
        !measure(theme_, "hud_gesture_put_away_zone", put_away_zone_, failed) ||
        !measure(theme_, "hud_gesture_ground_press_rise", ground_press_rise_, failed) ||
        !measure(theme_, "hud_gesture_brush_max_y", brush_max_y_, failed)) {
        return false;
    }

    // The ground of the world as a touch target (the mockup's bg), lower in the mode terrain.
    if (!measure(theme_, "hud_world_hit_x", ground_x_, failed) ||
        !measure(theme_, "hud_world_hit_y", ground_y_, failed) ||
        !measure(theme_, "hud_world_hit_w", ground_w_, failed) ||
        !measure(theme_, "hud_world_hit_h", ground_h_, failed) ||
        !measure(theme_, "hud_world_hit_terrain_h", ground_terrain_h_, failed)) {
        return false;
    }

    // The stand of the tree and of the trunk: pixels of the logical surface turned into mockup units.
    double frame_w = 0.0;
    double logical_w = 0.0;
    ::godot::Dictionary logical;
    ::godot::Dictionary hud;
    ::godot::Dictionary modes;
    ::godot::Dictionary tree;
    ::godot::Dictionary trunk;
    if (!measure(theme_, "hud_frame_width", frame_w, failed)) {
        return false;
    }
    if (!read_section(root, "logical_size", logical) || !read_number(logical, "width", logical_w) ||
        !(logical_w > 0.0)) {
        failed = "logical_size.width";
        return false;
    }
    if (!read_section(root, "hud", hud) || !read_section(hud, "modes", modes) ||
        !read_section(modes, "tree_hit", tree) || !read_section(modes, "trunk_hit", trunk)) {
        failed = "hud.modes.tree_hit, hud.modes.trunk_hit";
        return false;
    }
    const double unit = frame_w / logical_w;
    double tree_half_min_px = 0.0;
    double tree_min_px = 0.0;
    double tree_pad_px = 0.0;
    double trunk_half_px = 0.0;
    double trunk_pad_px = 0.0;
    if (!hit_number(tree, "tree_hit", "half_share", tree_half_share_, failed) ||
        !hit_number(tree, "tree_hit", "half_min_px", tree_half_min_px, failed) ||
        !hit_number(tree, "tree_hit", "min_px", tree_min_px, failed) ||
        !hit_number(tree, "tree_hit", "pad_px", tree_pad_px, failed) ||
        !hit_number(trunk, "trunk_hit", "half_px", trunk_half_px, failed) ||
        !hit_number(trunk, "trunk_hit", "share", trunk_share_, failed) ||
        !hit_number(trunk, "trunk_hit", "pulse_px", trunk_pad_px, failed)) {
        return false;
    }
    tree_half_min_ = tree_half_min_px * unit;
    tree_min_ = tree_min_px * unit;
    tree_pad_ = tree_pad_px * unit;
    trunk_half_ = trunk_half_px * unit;
    trunk_pad_ = trunk_pad_px * unit;

    // The view starts where the brain rules say (hud.modes.start).
    mode_ = resolver_.spec().start;
    return true;
}

void VivariumHud::set_mode(const std::string& name) {
    const bool changed = name != mode_;
    mode_ = name;
    if (mode_ != MODE_SIGN) {
        open_sign_.clear();
    }
    if (changed) {
        // The rules move the mode inside resolve, after the view asked replay for this frame: the HUD plays again
        // here and takes the mode and the sign into the key, so the next replay finds nothing new. The mark stands on
        // the wall clock the fades play on (replay).
        HudReplayKey key = replay_.key();
        key.mode = mode_;
        key.sign = open_sign_;
        replay_.mark(key, wall_);
    }
    brushing_ = false;
    selected_.clear();
    press_.pouring = false;
}

double VivariumHud::replay(uint32_t level, uint8_t stage, bool dead) {
    HudReplayKey key;
    key.mode = mode_;
    key.sign = open_sign_;
    key.level = level;
    key.debug = debug_on_;
    key.drawer = drawer_open_;
    key.tab = debug_tab_;
    key.stage = stage;
    key.dead = dead;
    return wall_ - replay_.take(key, wall_);
}

bool VivariumHud::resolve(VivariumHudFrame& frame, std::string& failed) {
    clock_ = frame.clock;
    if (!frame.cover.surface.has_area()) {
        failed = "VivariumHudFrame.cover: the view set no surface";
        return false;
    }
    cover_ = frame.cover;
    HudGameState state = frame.game;
    state.mode = mode_;
    state.open_sign = open_sign_;
    if (debug_on_) {
        state.marks.push_back(MARK_DEBUG_ON);
        if (drawer_open_) {
            state.marks.push_back(MARK_DRAWER_OPEN);
        }
    }
    if (drag_.moved()) {
        state.marks.push_back(MARK_DRAGGING);
    }
    if (!resolver_.resolve(state, shown_, failed)) {
        return false;
    }
    // The rules move the mode where its ground is gone: the sign no longer reported, the item no longer held.
    const HudContextSpec& spec = resolver_.spec();
    if (shown_.mode >= spec.modes.size()) {
        failed = "hud.modes: the resolved mode";
        return false;
    }
    if (spec.modes[shown_.mode].name != mode_) {
        set_mode(spec.modes[shown_.mode].name);
    }
    // The stow bridge answered the drag's place during this frame's fill; the layer takes it now, so a finger
    // held still over a place shows its verdict as the mockup's ghost does.
    drag_.judge(frame.drag);
    // THE BOTTOM BAND (operator order 2026-11-25): a change of the bars turns it to them, GODOT_RSM_BAND_HOLD_SECONDS
    // of quiet or the inventory in use (the pack open, a thing dragged) turn it back to the slots, the switch turns
    // it by hand. The wall clock counts, as the long presses do: the hold is a promise in real seconds.
    bars_.step(frame.bars.bars, wall_, window_.pack_open() || !drag_.dragged().empty());
    fill(frame);
    return set_rail(failed);
}

void VivariumHud::fill(VivariumHudFrame& now) const {
    const char* hot = drag_.hot();
    const bool refused = drag_.refused();
    const std::string dragged = drag_.dragged();
    const VivariumHudThing* held = hand_thing(now);
    const bool on_belt = same(hot, GODOT_HUD_CONTAINER_BELT_FIRST) || same(hot, GODOT_HUD_CONTAINER_BELT_SECOND);

    now.signs.shown = shown_;
    now.fan.tools = shown_.fan;
    now.fan.level = shown_.level;
    now.fan.centre = focus_.ring().centre();
    now.band.level = shown_.level;
    now.band.debug_on = debug_on_;

    now.hand.level = shown_.level;
    now.hand.mode = mode_;
    now.hand.verb = verb_;
    now.hand.brush_on = brushing_;
    now.hand.holding = press_.pouring;
    now.hand.dragged = held != nullptr && held->key == dragged;
    now.hand.hot = same(hot, GODOT_HUD_PLACE_HAND);
    now.hand.hot_bad = now.hand.hot && refused;
    now.hand.toast_posted = toast_.posted();

    now.belt.dragged = dragged;
    now.belt.hot = on_belt ? hot : nullptr;
    now.belt.refused = on_belt && refused;

    now.bag_icon.level = shown_.level;
    now.bag_icon.open = window_.pack_open();
    now.bag_icon.hot = hot;
    now.bag_icon.refused = refused;

    now.window.level = shown_.level;
    now.window.selected = selected_;
    now.window.dragged = dragged;
    now.window.hot = hot;
    now.window.refused = refused;

    now.world.level = shown_.level;
    now.world.selected = selected_;
    now.rack.level = shown_.level;
    now.focus.level = shown_.level;
    now.trail.level = shown_.level;
    now.sheet.year.level = shown_.level;
    now.sheet.forecast.level = shown_.level;

    now.terrain.reticle.verb = verb_;
    now.debug.open = drawer_open_;
    now.debug.tab = debug_tab_;
    now.debug.view = debug_view_;

    // The surface each dim, the drawer and the veil reach.
    now.veil.cover = now.cover;
    now.trail.cover = now.cover;
    now.sheet.cover = now.cover;
    now.rack.cover = now.cover;
    now.debug.reach = now.cover;
    now.drag.hand_band = now.cover.hand;

    now.drag.window = &window_;
    now.drag.pack_things = now.window.pack_things;
    now.drag.belt = &belt_;
    now.drag.bag_icon = &bag_icon_;
    now.drag.rack = &rack_;
    now.drag.rack_world = now.rack_world;
    now.drag.world = &world_;
}

bool VivariumHud::set_rail(std::string& failed) {
    std::vector<UiEdgeRailSign> list;
    for (const HudSignShown& sign : shown_.signs) {
        UiEdgeRailSign entry;
        if (!signs_.tone(sign.sign, entry.tone)) {
            failed = "hud.signs: the tone of the sign " + std::to_string(sign.sign);
            return false;
        }
        entry.alert = sign.alert;
        entry.opened = sign.open;
        list.push_back(entry);
    }
    return rail_.set(rail_label_, list, failed);
}

bool VivariumHud::shows(const char* name) const {
    for (const std::string& layer : shown_.layers) {
        if (layer == name) {
            return true;
        }
    }
    return false;
}

bool VivariumHud::lays(const char* name) const {
    for (const std::string& area : shown_.hits) {
        if (area == name) {
            return true;
        }
    }
    return false;
}

bool VivariumHud::toast(const std::string& key, const std::vector<HudPortValue>& values, const std::string& colour,
                        std::string& failed) {
    return toast_.post(key, values, colour, wall_, failed);
}

void VivariumHud::put_away() {
    // The mockup's putAway(): the hand's thing was stowed by the act's bridge, the mode is setMode('focus').
    set_mode(MODE_FOCUS);
}

void VivariumHud::return_to_garden() {
    // usecase:100390: a finger that held something when the app left holds nothing when it comes back.
    press_ = HudPress{};
    set_mode(resolver_.spec().start);
}

}  // namespace ase::adp::godot

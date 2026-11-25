/**
 * ASE GODOT ADAPTER - HUD COMPOSE GESTURE
 *
 * @file        godot_hud_compose_gesture.cpp
 * @brief       VivariumHud: the gestures of the hand and the acts they carry out on the view and toward the module
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.5, see godot_hud_compose.hpp. press, move, release and
 *              tick are the mockup's pointerdown, pointermove, pointerup and its timers: a press on a thing takes it
 *              up for a drag, on a window's grip moves the window, on the can pours while held, on the system ring
 *              counts toward the reset, on the system glyph and on the ground below the tree's foot arms a long
 *              press; a release drops the drag or taps the thing, ends the pour, swipes the tool away, lays the
 *              terrain verb under the brush or carries out the act it came down on (the mockup's doAct, tapItem,
 *              useTool). The view's own state changes here; every act on the world leaves as a VivariumHudRequest.
 *
 *              A TOOL WHOSE ITEM IS NOT IN POSSESSION. useTool finds the item among the frame's things; where it
 *              is missing the toast texts.hud.missing names it (the item's word items.<id>.name) in
 *              GODOT_HUD_COLOUR_REFUSED, as the mockup's toast('FALTA ' + item, 'alert').
 *
 *              THE EMPTY CAN. A press on the can while the held thing holds no water takes nothing up: the toast
 *              texts.hud.toast.pot_empty in COLOUR_WATER, as the mockup's toast('REGADOR VAZIO · ENCHA NO RIACHO',
 *              'water'); press returns false only when that toast could not be posted (`failed`).
 *
 *              THE FIRST VERB COMES WITH THE TOOL. The mockup's equip() sets S.terr = D.verbs[0] whenever an item
 *              with ground verbs comes into the hand, so the terrain reticle always has a verb whose op it draws
 *              (hud_verb_<v>_op); equip_verb does the same on every path that equips, before the mode changes.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-24
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_compose.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

bool VivariumHud::press(const ::godot::Vector2& point, const VivariumHudFrame& now, double clock,
                        std::string& failed) {
    clock_ = clock;
    press_ = HudPress{};
    press_.down = true;
    press_.at = point;
    press_.wall = wall_;
    press_.hit = hit(point, now);
    const char* act = press_.hit.act;

    if (same(act, GODOT_HUD_ACT_ITEM)) {
        // item:<key>: the thing is taken up; it turns into a drag once it moves past the drag slop.
        const VivariumHudThing* found = thing(press_.hit.key, now);
        ::godot::Rect2 box;
        double k = 0.0;
        if (found != nullptr && box_of(found->key, box, k)) {
            DragThing taken;
            taken.key = found->key;
            taken.id = found->id;
            taken.art = found->art;
            taken.from = found->from;
            taken.length_cm = found->length_cm;
            taken.level = found->level;
            drag_.take(taken, point, box, k);
            press_.thing = true;
        }
        return true;
    }
    if (same(act, GODOT_HUD_ACT_PACK_GRIP) || same(act, GODOT_HUD_ACT_POUCH_GRIP)) {
        window_.grab(act, point);
        press_.window = true;
        return true;
    }
    if (same(act, GODOT_HUD_ACT_POUR)) {
        // An empty can holds nothing to pour (m.html 1001): REGADOR VAZIO · ENCHA NO RIACHO, and the press holds
        // no can, so neither a measure nor a release follows.
        if (now.hand.thing == nullptr || !(now.hand.thing->fill_l > 0.0)) {
            return toast_.post(TOAST_POT_EMPTY, {}, COLOUR_WATER, wall_, failed);
        }
        press_.pouring = true;
        return true;
    }
    if (same(act, ACT_SYSTEM_HOLD)) {
        press_.system = true;
        return true;
    }
    if (same(act, ACT_SYSTEM_PRESS)) {
        press_.long_after = rail_press_s_;
        return true;
    }
    const bool calm = mode_ == GODOT_HUD_MODE_REST || mode_ == MODE_FOCUS || mode_ == MODE_SIGN;
    if ((same(act, ACT_GROUND) || same(act, ACT_TREE)) && calm && now.tree_placed &&
        point.y > now.trunk.y - ground_press_rise_) {
        press_.long_after = ground_press_s_;
    }
    return true;
}

bool VivariumHud::move(const ::godot::Vector2& point, VivariumHudFrame& now, std::string& failed) {
    if (press_.down && press_.long_after > 0.0 && point.distance_to(press_.at) > press_slop_) {
        press_.long_after = 0.0;
    }
    if (drag_.holding()) {
        if (drag_.move(point)) {
            selected_.clear();
        }
        if (drag_.moved()) {
            fill(now);
            return drag_.aim(now.drag, failed);
        }
        return true;
    }
    if (press_.window) {
        window_.move(point);
        return true;
    }
    if (press_.down && mode_ == GODOT_HUD_MODE_TERRAIN && point.y < brush_max_y_) {
        brushing_ = true;
        brush_ = point;
    }
    return true;
}

bool VivariumHud::release(const ::godot::Vector2& point, VivariumHudFrame& now, double clock,
                          std::vector<VivariumHudRequest>& out, std::string& failed) {
    clock_ = clock;
    const HudPress was = press_;
    press_ = HudPress{};

    if (drag_.holding()) {
        const bool moved = drag_.moved();
        fill(now);
        DragRelease let_go;
        if (!drag_.release(now.drag, toast_, clock, let_go, failed)) {
            return false;
        }
        if (!moved) {
            return tap_item(was.hit, clock, now, out, failed);
        }
        if (same(let_go.kind, GODOT_HUD_DRAG_SEND)) {
            VivariumHudRequest send;
            send.kind = VivariumHudRequest::ASK_SEND;
            send.key = let_go.key;
            send.point = point;
            send.target = let_go.target;
            out.push_back(send);
            // dropItem: a thing let go over the hand is equipped (equip(o.key)): its first verb and the mode of its
            // act (setMode(D.act)). A thing that left the hand for another place drops the mode back to the tool
            // place or to focus (if(wasHand)setMode(S.mode==='rack'?'rack':'focus')).
            const VivariumHudThing* sent = thing(let_go.key, now);
            if (sent == nullptr) {
                failed = "hud.drag.sent." + let_go.key;
                return false;
            }
            if (same(let_go.target.place, GODOT_HUD_PLACE_HAND)) {
                std::string item_mode;
                if (!theme_.item_mode(sent->id, item_mode)) {
                    failed = "hud.items." + sent->id + ".mode";
                    return false;
                }
                if (!equip_verb(sent->id, failed)) {
                    return false;
                }
                set_mode(item_mode);
            } else if (same(sent->from, GODOT_HUD_PLACE_HAND)) {
                set_mode(mode_ == MODE_RACK ? MODE_RACK : MODE_FOCUS);
            }
        }
        return true;  // GODOT_HUD_DRAG_REFUSED posted its toast through DragLayer
    }
    if (was.window) {
        window_.release();
        return true;
    }
    if (!was.down) {
        return true;
    }
    if (was.pouring) {
        VivariumHudRequest end;
        end.kind = VivariumHudRequest::ASK_POUR_END;
        end.point = point;
        out.push_back(end);
        return true;
    }
    if (was.system || was.long_fired) {
        return true;  // the ring let go before it filled, or the long press already opened its mode
    }
    const bool tool_mode = mode_ == GODOT_HUD_MODE_WATER || mode_ == GODOT_HUD_MODE_PRUNE ||
                           mode_ == GODOT_HUD_MODE_SOW || mode_ == GODOT_HUD_MODE_TERRAIN ||
                           mode_ == GODOT_HUD_MODE_SAP || mode_ == GODOT_HUD_MODE_PLUG || mode_ == MODE_HOLD;
    // The put-away zone is the foot over the hand band: it moves down with the bottom band (cover_.hand).
    if (tool_mode && point.y - was.at.y > put_away_swipe_ && was.at.y - cover_.hand > put_away_zone_) {
        VivariumHudRequest stow;
        stow.kind = VivariumHudRequest::ASK_STOW;
        out.push_back(stow);
        set_mode(MODE_FOCUS);
        return true;
    }
    if (mode_ == GODOT_HUD_MODE_TERRAIN && (was.hit.act == nullptr || same(was.hit.act, ACT_GROUND)) &&
        point.y < brush_max_y_) {
        brushing_ = true;
        brush_ = point;
        VivariumHudRequest verb;
        verb.kind = VivariumHudRequest::ASK_VERB;
        verb.key = verb_;
        verb.point = point;
        out.push_back(verb);
        return true;
    }
    if (was.hit.act == nullptr) {
        return true;
    }
    if (same(was.hit.act, GODOT_HUD_ACT_ITEM)) {
        return tap_item(was.hit, clock, now, out, failed);
    }
    return act(was.hit, point, now, out, failed);
}

bool VivariumHud::tick(double clock, double real, VivariumHudFrame& now, std::vector<VivariumHudRequest>& out,
                       std::string& failed) {
    (void)now;
    (void)failed;
    clock_ = clock;
    // THE LONG PRESS COUNTS WALL SECONDS (m.html 1003, 1004: setTimeout 450 and 480): the HUD's clock grows at most
    // hud_frame_dt_max a frame, and at 11 frames a second a hold of 0.75 s reached 0.41 s on it and never opened sys
    // (run 20261113-v19-ring1, V0052).
    wall_ += ase::math::max(real, 0.0);
    if (press_.down && press_.long_after > 0.0 && !press_.long_fired && wall_ - press_.wall >= press_.long_after) {
        const bool rail = same(press_.hit.act, ACT_SYSTEM_PRESS);
        set_mode(rail ? MODE_SYS : MODE_RACK);
        press_.long_fired = true;
    }
    // THE CAN AND THE RING COUNT REAL SECONDS TOO: the ring's sentence promises SEGURE 1,5 S and the can a measure each
    // 0.8 s (m.html 1035), and at two frames a second a hold of 9 s reached 0.9 s on the clock capped at
    // hud_frame_dt_max: the ring never reset (run 1124-ui-hud-n, 18 frames in the hold) and the can let 1 L go after
    // 1.6 s, too little for a seed (run camp_hud_1124xb). The first frame after the press began before it: only the
    // frames after it lie wholly inside the hold, so a hold shorter than the token neither resets nor adds a measure.
    if (press_.pouring || press_.system) {
        const double inside = press_.ticked ? ase::math::max(real, 0.0) : 0.0;
        press_.poured_held += press_.pouring ? inside : 0.0;
        press_.system_held += press_.system ? inside : 0.0;
        press_.ticked = true;
    }
    // A frame longer than a step holds several measures; what is left of it counts for the next.
    while (press_.pouring && pour_step_s_ > 0.0 && press_.poured_held - press_.poured_at >= pour_step_s_) {
        press_.poured_at += pour_step_s_;
        VivariumHudRequest pour;
        pour.kind = VivariumHudRequest::ASK_POUR;
        pour.point = press_.at;
        out.push_back(pour);
    }
    if (press_.system && system_.full(press_.system_held)) {
        press_ = HudPress{};
        VivariumHudRequest reset;
        reset.kind = VivariumHudRequest::ASK_RESET;
        out.push_back(reset);
        set_mode(resolver_.spec().start);
    }
    return true;
}

bool VivariumHud::use_tool(uint32_t index, const ::godot::Vector2& point, const VivariumHudFrame& now,
                           std::vector<VivariumHudRequest>& out, std::string& failed) {
    const HudContextSpec& spec = resolver_.spec();
    if (index >= spec.tools.size()) {
        failed = "hud.tools: the tapped tool";
        return false;
    }
    const HudToolSpec& tool = spec.tools[index];
    if (tool.has.empty()) {
        // A tool without an item opens the mode of its name (sky), else the tool place (ground).
        bool is_mode = false;
        for (const HudModeSpec& mode : spec.modes) {
            is_mode = is_mode || mode.name == tool.name;
        }
        set_mode(is_mode ? tool.name : std::string(MODE_RACK));
        return true;
    }
    for (const VivariumHudThing& candidate : now.things) {
        if (candidate.id == tool.has) {
            VivariumHudRequest take;
            take.kind = VivariumHudRequest::ASK_TAKE;
            take.key = candidate.key;
            take.point = point;
            out.push_back(take);
            // The mockup's equip(): the item into the hand with its first verb and the mode of its act (ITEMS[id].act).
            std::string item_mode;
            if (!theme_.item_mode(tool.has, item_mode)) {
                failed = "hud.items." + tool.has + ".mode";
                return false;
            }
            if (!equip_verb(tool.has, failed)) {
                return false;
            }
            set_mode(item_mode);
            return true;
        }
    }
    std::string name;
    if (!word_line(theme_, std::string(GODOT_HUD_WORD_ITEM_HEAD) + tool.has + GODOT_HUD_WORD_ITEM_TAIL, name,
                   failed)) {
        return false;
    }
    return toast_.post("missing", {HudPortValue{"item", name}}, GODOT_HUD_COLOUR_REFUSED, wall_, failed);
}

bool VivariumHud::act(const VivariumHudHit& hit, const ::godot::Vector2& point, const VivariumHudFrame& now,
                      std::vector<VivariumHudRequest>& out, std::string& failed) {
    const char* a = hit.act;
    VivariumHudRequest ask;
    ask.point = point;
    ask.key = hit.key;
    ask.index = hit.index;

    if (same(a, GODOT_HUD_ACT_SKY)) {
        set_mode(mode_ == MODE_SKY ? std::string(GODOT_HUD_MODE_REST) : std::string(MODE_SKY));
    } else if (same(a, GODOT_HUD_ACT_DEBUG)) {
        debug_on_ = !debug_on_;
        drawer_open_ = true;
    } else if (same(a, ACT_DEBUG_PART)) {
        if (hit.debug.handle) {
            drawer_open_ = true;
        } else if (hit.debug.fold) {
            drawer_open_ = false;
        } else if (hit.debug.tab != GODOT_HUD_DEBUG_NONE) {
            debug_tab_ = hit.debug.tab;
        } else {
            ask.kind = VivariumHudRequest::ASK_DEBUG;
            ask.debug = hit.debug;
            out.push_back(ask);
        }
    } else if (same(a, ACT_SIGN)) {
        const HudContextSpec& spec = resolver_.spec();
        if (hit.index >= spec.signs.size()) {
            failed = "hud.signs: the tapped sign";
            return false;
        }
        set_mode(MODE_SIGN);
        open_sign_ = spec.signs[hit.index].name;
    } else if (same(a, ACT_TREE)) {
        set_mode(MODE_FOCUS);
    } else if (same(a, GODOT_HUD_ACT_REST) || same(a, ACT_FOCUS)) {
        set_mode(GODOT_HUD_MODE_REST);
    } else if (same(a, ACT_GROUND)) {
        if (!selected_.empty()) {
            selected_.clear();
        } else if (mode_ == GODOT_HUD_MODE_SOW) {
            ask.kind = VivariumHudRequest::ASK_SOW;
            out.push_back(ask);
        } else if (mode_ == GODOT_HUD_MODE_WATER) {
            ask.kind = VivariumHudRequest::ASK_BROOK;
            out.push_back(ask);
        } else if (mode_ != GODOT_HUD_MODE_PRUNE && mode_ != GODOT_HUD_MODE_TERRAIN && mode_ != GODOT_HUD_MODE_SAP &&
                   mode_ != GODOT_HUD_MODE_PLUG) {
            set_mode(GODOT_HUD_MODE_REST);
        }
    } else if (same(a, ACT_TAKE)) {
        return use_tool(hit.index, point, now, out, failed);
    } else if (same(a, ACT_RACK)) {
        set_mode(MODE_RACK);
    } else if (same(a, GODOT_HUD_ACT_VERB)) {
        verb_ = hit.key;
    } else if (same(a, GODOT_HUD_ACT_STOW)) {
        ask.kind = VivariumHudRequest::ASK_STOW;
        out.push_back(ask);
        set_mode(MODE_FOCUS);
    } else if (same(a, GODOT_HUD_ACT_PACK_CLOSE)) {
        // The mockup's case 'bag': the pack opens or closes, the pouch with it, and the choice is dropped (S.sel=null).
        pack_.tap(window_);
        selected_.clear();
    } else if (same(a, GODOT_HUD_ACT_POUCH_CLOSE)) {
        window_.close_pouch();
    } else if (same(a, ACT_TRUNK)) {
        if (mode_ == GODOT_HUD_MODE_SAP) {
            ask.kind = VivariumHudRequest::ASK_TRUNK;
            out.push_back(ask);
        } else if (mode_ == GODOT_HUD_MODE_PLUG) {
            ask.kind = VivariumHudRequest::ASK_PLUG;
            out.push_back(ask);
        } else {
            set_mode(MODE_FOCUS);
        }
    } else if (same(a, ACT_CUT)) {
        ask.kind = VivariumHudRequest::ASK_CUT;
        out.push_back(ask);
    } else if (same(a, ACT_SHEET)) {
        if (hit.sheet.rest) {
            set_mode(GODOT_HUD_MODE_REST);
        }
    }
    // GODOT_HUD_ACT_WINDOW_BODY, GODOT_HUD_ACT_NOOP, ACT_SYSTEM_PRESS and ACT_SYSTEM_HOLD as a tap take the touch.
    return true;
}

bool VivariumHud::tap_item(const VivariumHudHit& hit, double clock, const VivariumHudFrame& now,
                           std::vector<VivariumHudRequest>& out, std::string& failed) {
    const VivariumHudThing* found = thing(hit.key, now);
    if (found == nullptr) {
        return true;  // the thing left the possession between press and release (the mockup's !o)
    }
    const bool twice = tapped_ && last_tap_key_ == found->key && clock - last_tap_clock_ < double_tap_s_;
    tapped_ = true;
    last_tap_key_ = found->key;
    last_tap_clock_ = clock;

    VivariumHudRequest equip;
    equip.kind = VivariumHudRequest::ASK_EQUIP;
    equip.key = found->key;

    // The mode of the item's act: the mockup's equip() sets it whenever the thing comes into the hand.
    std::string item_mode;
    if (!theme_.item_mode(found->id, item_mode)) {
        failed = "hud.items." + found->id + ".mode";
        return false;
    }
    if (same(found->from, GODOT_HUD_PLACE_HAND)) {
        if (mode_ != item_mode) {
            set_mode(item_mode);
        } else {
            selected_ = selected_ == found->key ? std::string() : found->key;
        }
        return true;
    }
    if (mode_ == MODE_RACK || same(found->from, GODOT_HUD_CONTAINER_BELT_FIRST) ||
        same(found->from, GODOT_HUD_CONTAINER_BELT_SECOND)) {
        out.push_back(equip);
        if (!equip_verb(found->id, failed)) {
            return false;
        }
        set_mode(item_mode);
        return true;
    }
    if (same(found->from, GODOT_HUD_PLACE_GROUND) && found->rack) {
        set_mode(MODE_RACK);
        return true;
    }
    if (twice) {
        if (found->id == pouch_.item().id()) {
            pouch_.double_tap(window_);
        } else {
            out.push_back(equip);
            if (!equip_verb(found->id, failed)) {
                return false;
            }
            set_mode(item_mode);
        }
        selected_.clear();
        return true;
    }
    selected_ = selected_ == found->key ? std::string() : found->key;
    return true;
}

bool VivariumHud::equip_verb(const std::string& id, std::string& failed) {
    std::string tone;
    std::string asset;
    std::vector<std::string> verbs;
    double view_w = 0.0;
    double view_h = 0.0;
    if (!theme_.item_thing(id, tone, asset, verbs, view_w, view_h)) {
        failed = "hud.items." + id;
        return false;
    }
    // if(D.verbs)S.terr=D.verbs[0]: the reticle and the hand's verb chips draw from the first verb on.
    if (!verbs.empty()) {
        verb_ = verbs.front();
    }
    return true;
}

}  // namespace ase::adp::godot

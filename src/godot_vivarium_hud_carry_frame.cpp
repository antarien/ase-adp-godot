/**
 * ASE GODOT ADAPTER - VIVARIUM HUD CARRY BRIDGE FRAME
 *
 * @file        godot_vivarium_hud_carry_frame.cpp
 * @brief       VivariumCarryBridge - the mockup's inventory read from SHA and handed into the HUD's frame
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.7 to 06.9, design decision 2026-11-10. load() reads the
 *              mockup's initInv (hud_bridge_carry_<n>_*, hud_bridge_rack_<n>_item), the facts of its ITEMS
 *              (hud_bridge_item_<id>_*, hud.items for the drawing), its CAPS (hud_bridge_container_<id>_*), the
 *              item of the sow tool (hud.tools.sow.has) and the rules of fits(), layAt() and stow(). fill() hands
 *              every carry group of the frame its things each frame: the things and the possession, the pack and
 *              the pouch with their records and contents, the holsters, the hand, the tool place with its tools
 *              and the ground, and the signs the place of each item in possession (where(findIt(id))). The tool
 *              place is set once, from the camera of the first frame. The rules and the acts are
 *              godot_vivarium_hud_carry.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), N0452 (flora.stow.v1), N0487 (physics.body.v1 bodies of the tool place and
 *              the ground things), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-20
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_hud_carry.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The place constant a token names (ranzen, bolsa, belt0, belt1, hand, ground); nullptr for any other word. */
const char* place_of(const std::string& word) {
    const char* places[] = {GODOT_HUD_CONTAINER_PACK,        GODOT_HUD_CONTAINER_POUCH, GODOT_HUD_CONTAINER_BELT_FIRST,
                            GODOT_HUD_CONTAINER_BELT_SECOND, GODOT_HUD_PLACE_HAND,      GODOT_HUD_PLACE_GROUND};
    for (const char* place : places) {
        if (word == place) {
            return place;
        }
    }
    return nullptr;
}

/** The mockup's where(o): the word GODOT_HUD_WHERE_* of the place `place`, a thing at the tool place `rack`. */
const char* where_of(const char* place, bool rack) {
    const std::string word = place != nullptr ? place : "";
    if (word == GODOT_HUD_PLACE_HAND) {
        return GODOT_HUD_WHERE_HAND;
    }
    if (word == GODOT_HUD_CONTAINER_BELT_FIRST || word == GODOT_HUD_CONTAINER_BELT_SECOND) {
        return GODOT_HUD_WHERE_BELT;
    }
    if (word == GODOT_HUD_CONTAINER_PACK) {
        return GODOT_HUD_WHERE_PACK;
    }
    if (word == GODOT_HUD_CONTAINER_POUCH) {
        return GODOT_HUD_WHERE_POUCH;
    }
    return rack ? GODOT_HUD_WHERE_RACK : GODOT_HUD_WHERE_GROUND;
}

/** The text token `token` as a std::string; false where SHA states none. */
bool text_of(const UiTheme& theme, const std::string& token, std::string& out) {
    ::godot::String text;
    if (!theme.token_text(token, text)) {
        return false;
    }
    out = std::string(text.utf8().get_data());
    return !out.empty();
}

}  // namespace

bool VivariumCarryBridge::load(const UiTheme& theme, std::string& failed) {
    facts_.clear();
    carried_.clear();
    rack_placed_ = false;
    next_order_ = 0;
    engine_water_l_ = -1.0;  // no gear snapshot of the new host applied yet: the can holds nothing until its owner names it

    // The rules of fits(), wt(), layAt() and stow(), and the tool place.
    const char* rule_keys[] = {"hud_bridge_belt_len_cm",    "hud_bridge_fill_kg_per_l",   "hud_bridge_sap_kg_per_l",
                               "hud_bridge_lay_tall",       "hud_bridge_lay_angle",       "hud_bridge_stow_x_from",
                               "hud_bridge_stow_x_span",    "hud_bridge_stow_y_from",     "hud_bridge_stow_y_span",
                               "hud_bridge_rack_across",    "hud_bridge_rack_ahead",      "hud_bridge_rack_slot_first",
                               "hud_bridge_rack_slot_step", "hud_bridge_rack_slot_ahead", "hud_bridge_rack_slot_angle"};
    double* rule_values[] = {&belt_len_cm_, &fill_kg_per_l_, &sap_kg_per_l_, &lay_tall_,    &lay_angle_,
                             &stow_x_from_, &stow_x_span_,   &stow_y_from_,  &stow_y_span_, &rack_across_,
                             &rack_ahead_,  &slot_first_,    &slot_step_,    &slot_ahead_,  &slot_angle_};
    for (size_t index = 0; index < sizeof(rule_keys) / sizeof(rule_keys[0]); ++index) {
        if (!theme.measure(rule_keys[index], *rule_values[index])) {
            failed = rule_keys[index];
            return false;
        }
    }

    // CAPS: the pack and the pouch, each with the asset of its item.
    const char* container_ids[] = {GODOT_HUD_CONTAINER_PACK, GODOT_HUD_CONTAINER_POUCH};
    ContainerDefCarry* container_into[] = {&pack_, &pouch_};
    for (size_t index = 0; index < 2; ++index) {
        const std::string head = std::string("hud_bridge_container_") + container_ids[index] + "_";
        ContainerDefCarry& into = *container_into[index];
        std::string tone;
        std::vector<std::string> verbs;
        double view_w = 0.0;
        double view_h = 0.0;
        if (!theme.measure(head + "opening_cm", into.opening_cm) || !theme.measure(head + "room_l", into.room_l) ||
            !theme.measure(head + "carry_kg", into.carry_kg)) {
            failed = head + "*";
            return false;
        }
        if (!theme.item_thing(container_ids[index], tone, into.asset, verbs, view_w, view_h)) {
            failed = std::string("texts.hud.items.") + container_ids[index];
            return false;
        }
    }
    // placeIn holds a thing inside C.w and C.h less its drawn box: the same content and ik the window draws with.
    if (!pack_room_.bind(theme, "bag", failed) || !pouch_room_.bind(theme, "pouch", failed)) {
        return false;
    }

    // The items the hand counts: sowTap's seed (findIt('sementes')), the sap mode's spouts and trough. Not the
    // sow tool's has: that is the dibber the hand holds (plantador), the seed is a thing of the pouch.
    if (!text_of(theme, "hud_bridge_hand_seed_item", seed_item_) ||
        !text_of(theme, "hud_bridge_hand_spout_item", spout_item_) ||
        !text_of(theme, "hud_bridge_hand_vessel_item", vessel_item_)) {
        failed = "hud_bridge_hand_seed_item, hud_bridge_hand_spout_item, hud_bridge_hand_vessel_item";
        return false;
    }
    // sowTap's reach: RPATCH plus 0.03 of the field around the stem place, else FORA DA ÁREA (m.html 958).
    if (!theme.measure("hud_bridge_hand_sow_reach_m", sow_reach_m_) || !(sow_reach_m_ > 0.0)) {
        failed = "hud_bridge_hand_sow_reach_m (above 0)";
        return false;
    }
    // brookTap's can and reach: handItem().id === 'regador', 0.04 of the field off the brook's line (m.html 968).
    if (!text_of(theme, "hud_bridge_hand_can_item", can_item_) ||
        !theme.measure("hud_bridge_hand_brook_reach_m", brook_reach_m_) || !(brook_reach_m_ > 0.0)) {
        failed = "hud_bridge_hand_can_item, hud_bridge_hand_brook_reach_m (above 0)";
        return false;
    }

    // The tap: plugTap's peg and where it lays the full trough, the litres a game hour fills it and the real seconds
    // of a game hour while the clock port is silent (S.tap.l += dh * 0.08, dh = dt * S.factor / 8).
    if (!text_of(theme, "hud_bridge_hand_plug_item", plug_item_) ||
        !theme.measure("hud_bridge_plug_across", plug_across_) || !theme.measure("hud_bridge_plug_ahead", plug_ahead_) ||
        !theme.measure("hud_bridge_patch_sap_per_hour", sap_per_hour_) || !(sap_per_hour_ > 0.0) ||
        !theme.measure("hud_bridge_pace_hour_s", hour_s_) || !(hour_s_ > 0.0)) {
        failed = "hud_bridge_hand_plug_item, hud_bridge_plug_across, hud_bridge_plug_ahead, "
                 "hud_bridge_patch_sap_per_hour (above 0), hud_bridge_pace_hour_s (above 0)";
        return false;
    }
    // The level a vessel is drawn at is the widgets' rule, the same the tap hole draws the trough with.
    if (!sap_rule_.bind(theme, failed)) {
        return false;
    }

    // ITEMS: one entry per item the start names, read once.
    auto facts_of = [&](const std::string& id, uint32_t& out) {
        for (uint32_t index = 0; index < facts_.size(); ++index) {
            if (facts_[index].id == id) {
                out = index;
                return true;
            }
        }
        Facts facts;
        facts.id = id;
        const std::string head = "hud_bridge_item_" + id + "_";
        double stack = 0.0;
        if (!theme.measure(head + "len_cm", facts.length_cm) || !theme.measure(head + "kg", facts.kg) ||
            !theme.measure(head + "l", facts.litres) || !theme.measure(head + "cap_l", facts.cap_l) ||
            !theme.measure(head + "stack", stack)) {
            failed = head + "*";
            return false;
        }
        facts.stack = stack > 0.0;
        if (!theme.item_thing(id, facts.tone, facts.asset, facts.verbs, facts.view_w, facts.view_h)) {
            failed = "texts.hud.items." + id;
            return false;
        }
        out = static_cast<uint32_t>(facts_.size());
        facts_.push_back(facts);
        return true;
    };

    // initInv: the carried things in their order (o.zo counts along it), then the tools at the place.
    double count = 0.0;
    if (!theme.measure("hud_bridge_carry_count", count) || count < 1.0) {
        failed = "hud_bridge_carry_count";
        return false;
    }
    for (int64_t number = 1; number <= static_cast<int64_t>(count); ++number) {
        const std::string head = "hud_bridge_carry_" + std::to_string(number) + "_";
        std::string id;
        std::string word;
        Carried thing;
        double x = 0.0;
        double y = 0.0;
        double pieces = 0.0;
        if (!text_of(theme, head + "item", id) || !text_of(theme, head + "place", word) ||
            !theme.measure(head + "x", x) || !theme.measure(head + "y", y) || !theme.measure(head + "count", pieces) ||
            !theme.measure(head + "fill_l", thing.fill_l) || !theme.measure(head + "sap_l", thing.sap_l)) {
            failed = head + "*";
            return false;
        }
        thing.place = place_of(word);
        if (thing.place == nullptr) {
            failed = head + "place";
            return false;
        }
        if (!facts_of(id, thing.facts)) {
            return false;
        }
        thing.key = "k" + std::to_string(number);
        thing.at = ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
        thing.count = static_cast<int64_t>(pieces);
        thing.order = ++next_order_;
        carried_.push_back(thing);
    }
    double tools = 0.0;
    if (!theme.measure("hud_bridge_rack_count", tools)) {
        failed = "hud_bridge_rack_count";
        return false;
    }
    for (int64_t number = 1; number <= static_cast<int64_t>(tools); ++number) {
        const std::string token = "hud_bridge_rack_" + std::to_string(number) + "_item";
        std::string id;
        Carried thing;
        if (!text_of(theme, token, id)) {
            failed = token;
            return false;
        }
        if (!facts_of(id, thing.facts)) {
            return false;
        }
        thing.key = "k" + std::to_string(static_cast<int64_t>(count) + number);
        thing.place = GODOT_HUD_PLACE_GROUND;
        thing.rack = true;
        thing.along = slot_first_ + static_cast<double>(number - 1) * slot_step_;
        thing.angle = slot_angle_;
        thing.count = 1;
        thing.order = ++next_order_;
        carried_.push_back(thing);
    }
    return true;
}

::godot::Vector3 VivariumCarryBridge::rack_offset(const ::godot::Vector3& right, const ::godot::Vector3& ahead) {
    // The tool place stands where the camera of the first frame sees it beside the patch.
    if (!rack_placed_) {
        rack_placed_ = true;
        rack_right_ = right;
        rack_view_ = ahead;
        rack_offset_ = right * static_cast<float>(rack_across_) + ahead * static_cast<float>(rack_ahead_);
    }
    return rack_offset_;
}

::godot::Vector3 VivariumCarryBridge::rack_along() const {
    return rack_right_;
}

std::vector<GroundThing>& VivariumCarryBridge::ground_things() {
    return ground_things_;
}

bool VivariumCarryBridge::rack_asked() const {
    return rack_asked_;
}

void VivariumCarryBridge::ask_rack() {
    rack_asked_ = true;
}

void VivariumCarryBridge::fill(VivariumHudFrame& now, const ::godot::Vector3& patch, const ::godot::Vector3& right,
                               const ::godot::Vector3& ahead, const UiGround& ground, bool body_known,
                               const ::godot::Vector3& body) {
    // The tool place is the rack body of ase-pl-toolrack once ase-physics has let it fall (usecase:101625): its
    // place on toolrack.rack.v1 is the place, its height the rest height physics found on the ground there.
    if (body_known) {
        (void)rack_offset(right, ahead);  // the slots run along the camera of the first frame, as the lay did
        rack_place_ = body;
    } else {
        rack_place_ = patch + rack_offset(right, ahead);
        // NOT BOUND (GAP_BODY, N0487): the host offers no rack port yet or its body was not read; the bridge sets
        // the height to the ground as it lies now, a bridge default and no physics. It stops acting the frame the
        // rack body stands on toolrack.rack.v1.
        double rack_height = 0.0;
        if (ground.height(rack_place_.x, rack_place_.z, rack_height)) {
            rack_place_.y = static_cast<float>(rack_height);
        }
    }

    // The stem place plugTap lays the trough beside. The dibber's hole: a sowing apply() accepted gets this frame's
    // clock and the patch's stem place.
    stem_place_ = patch;
    if (sow_pending_) {
        sow_pending_ = false;
        sown_ = true;
        sown_at_ = now.clock;
        sown_place_ = patch;
        dug_ = false;
        seeded_ = false;
        covered_ = true;  // the dibber pricks the hole, the nutlet goes in and closes it in one tap (sowTap)
    }
    now.sow.sown = sown_;
    now.sow.place = sown_place_;
    now.sow.age = now.clock - sown_at_;
    now.sow.open = dug_;  // the planting hole of the spade (usecase:100388, Q02) stands until cover() fills it
    now.sow.seeded = seeded_;
    now.sow.covered = covered_;

    pack_things_.clear();
    pouch_things_.clear();
    rack_tools_.clear();
    ground_things_.clear();
    belt_first_ = BagThing{};
    belt_second_ = BagThing{};
    bool first = false;
    bool second = false;
    bool held = false;
    bool pouch_carried = false;
    now.things.clear();
    now.game.possessed.clear();
    now.signs.places.clear();

    for (const Carried& thing : carried_) {
        const Facts& facts = facts_[thing.facts];
        now.game.possessed.push_back(facts.id);
        // The trough under the tap is still owned but out of reach: useTool and every drag go by findIt, which
        // passes o.c === 'trunk'; TapHole draws it at the bore.
        if (same_place(thing.place, GODOT_HUD_PLACE_TRUNK)) {
            continue;
        }
        // The held share of a vessel as D.art(o) draws it: water from o.fill (WateringPot), sap from o.sap > 0.02
        // (BarkVessel); the bag, the belt, the hand and the drag ghost draw the level the tap hole draws.
        ItemWidgetCarry water;
        water.held_l = thing.fill_l;
        water.capacity_l = facts.cap_l;
        ItemWidgetCarry sap;
        sap.held_l = thing.sap_l;
        sap.capacity_l = facts.cap_l;
        const double level = ase::math::max(WateringPot::filled(UiItemSpritePlace(), water).level,
                                            sap_rule_.filled(UiItemSpritePlace(), sap).level);
        now.things.push_back(VivariumHudThing{thing.key, facts.id, facts.id, thing.place, thing.rack, facts.length_cm,
                                              facts.kg, weight(thing), facts.litres, thing.count, facts.stack, level});
        // where(findIt(id)): the place word of the first carried piece of each item, in the order of INV, for the
        // offer line of the sign readings (N0451, flora.carry.v1.where).
        bool listed = thing.count <= 0;
        for (const SignPlace& known : now.signs.places) {
            listed = listed || known.item == facts.id;
        }
        if (!listed) {
            now.signs.places.push_back(SignPlace{facts.id, where_of(thing.place, thing.rack)});
        }
        pouch_carried = pouch_carried || facts.id == GODOT_HUD_CONTAINER_POUCH;

        BagThing bag;
        bag.key = thing.key;
        bag.art = facts.id;
        bag.at = thing.at;
        bag.length_cm = facts.length_cm;
        bag.kg = weight(thing);
        bag.litres = room(thing);
        bag.order = thing.order;
        bag.count = thing.count;
        bag.stack = facts.stack;
        bag.level = level;

        if (same_place(thing.place, GODOT_HUD_CONTAINER_PACK)) {
            pack_things_.push_back(bag);
        } else if (same_place(thing.place, GODOT_HUD_CONTAINER_POUCH)) {
            pouch_things_.push_back(bag);
        } else if (same_place(thing.place, GODOT_HUD_CONTAINER_BELT_FIRST)) {
            belt_first_ = bag;
            first = true;
        } else if (same_place(thing.place, GODOT_HUD_CONTAINER_BELT_SECOND)) {
            belt_second_ = bag;
            second = true;
        } else if (same_place(thing.place, GODOT_HUD_PLACE_HAND)) {
            hand_ = HandThing{thing.key, facts.id, facts.id, facts.stack, static_cast<uint32_t>(thing.count),
                              weight(thing), thing.fill_l + thing.sap_l, facts.cap_l, facts.verbs, level,
                              thing.mm_per_l};
            held = true;
        } else if (same_place(thing.place, GODOT_HUD_PLACE_GROUND)) {
            // A tool at the place leans at its slot along the place, at the height of the place; any other lies
            // where it was let go.
            ::godot::Vector3 place = thing.ground;
            if (thing.rack) {
                place = rack_place_ + rack_right_ * static_cast<float>(thing.along) +
                        rack_view_ * static_cast<float>(slot_ahead_);
            } else {
                // NOT BOUND (GAP_BODY, N0487): the height of a loose thing on the ground is to come from its
                // ase-physics body (HUB_PHYS_BODY_Y); until that port delivers, the bridge lays it on the ground.
                double height = 0.0;
                if (ground.height(place.x, place.z, height)) {
                    place.y = static_cast<float>(height);
                }
            }
            ground_things_.push_back(GroundThing{thing.key, facts.id, facts.tone, place, thing.angle,
                                                 facts.length_cm, thing.order, thing.rack});
            if (thing.rack) {
                rack_tools_.push_back(RackTool{thing.key, facts.id, facts.id, thing.along, facts.verbs,
                                               facts.length_cm, facts.kg, facts.asset});
            }
        }
    }

    now.window.pack = &pack_;
    now.window.pouch = &pouch_;
    now.window.pack_things = &pack_things_;
    now.window.pouch_things = &pouch_things_;
    now.window.pouch_carried = pouch_carried;
    now.bag_icon.things = &pack_things_;
    now.belt.first = first ? &belt_first_ : nullptr;
    now.belt.second = second ? &belt_second_ : nullptr;
    now.hand.thing = held ? &hand_ : nullptr;
    const Carried* seeds = find(seed_item_);
    const Carried* spouts = find(spout_item_);
    now.hand.seeds = seeds != nullptr ? static_cast<uint32_t>(seeds->count) : 0u;          // seedsLeft()
    now.hand.spouts = spouts != nullptr ? static_cast<uint32_t>(spouts->count) : 0u;       // findIt('bica').n
    now.hand.has_vessel = find(vessel_item_) != nullptr;                                   // have('cocho')
    now.rack_world.place = rack_place_;
    now.rack.tools = &rack_tools_;
    now.rack.place = rack_place_;
    now.world.things = &ground_things_;
    now.world.rack = rack_place_;
}

}  // namespace ase::adp::godot

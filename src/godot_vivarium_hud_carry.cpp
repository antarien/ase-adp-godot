/**
 * ASE GODOT ADAPTER - VIVARIUM HUD CARRY BRIDGE
 *
 * @file        godot_vivarium_hud_carry.cpp
 * @brief       VivariumCarryBridge - the mockup's carry rules and acts while flora.stow.v1 judges no move
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.7 to 06.9, operator decision 2026-11-10. The rules the mockup
 *              applies to its INV: wt() and vol() of a thing, fits() of a thing at a place with the refusal code
 *              and the values its word fit_<code> names, layAt() onto the ground or the next slot of the tool
 *              place, stow() and equip(); the acts apply() follows on them: take and equip, stow and a drop
 *              (send); and sowTap in sow(), which judges the place, the birch and the pouch in the mockup's order
 *              and names the toast of its answer. Every number is a hud_bridge_* token load() read
 *              (godot_vivarium_hud_carry_frame.cpp); the gap stays on the view's edge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), N0452 (flora.stow.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.01.00003
 */

#include <ase/adp/godot/godot_vivarium_hud_carry.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>
#include <ase/math/random.hpp>

#include <godot_cpp/variant/string.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/** `value` with `decimals` places as the mockup prints it (f1() with one); a negative count prints it whole. */
std::string number_text(double value, int decimals) {
    if (decimals < 0) {
        return std::string(::godot::String::num(value).utf8().get_data());
    }
    return std::string(::godot::String::num(value, decimals).utf8().get_data());
}

/** kgf(): two places under 0.1 kg, one above. */
std::string kg_text(double kg) {
    return number_text(kg, kg < 0.1 ? 2 : 1);
}

}  // namespace

bool VivariumCarryBridge::same_place(const char* a, const char* b) {
    return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}

double VivariumCarryBridge::weight(const Carried& thing) const {
    const Facts& facts = facts_[thing.facts];
    double kg = facts.kg * static_cast<double>(thing.count) + thing.fill_l * fill_kg_per_l_ +
                thing.sap_l * sap_kg_per_l_;
    // The pouch weighs what it holds.
    if (facts.id == GODOT_HUD_CONTAINER_POUCH) {
        for (const Carried& inner : carried_) {
            if (same_place(inner.place, GODOT_HUD_CONTAINER_POUCH)) {
                kg += weight(inner);
            }
        }
    }
    return kg;
}

double VivariumCarryBridge::room(const Carried& thing) const {
    const Facts& facts = facts_[thing.facts];
    return facts.stack ? facts.litres * static_cast<double>(thing.count) : facts.litres;
}

std::string VivariumCarryBridge::fits(const Carried& thing, const char* place,
                                      std::vector<HudPortValue>& values) const {
    values.clear();
    const Facts& facts = facts_[thing.facts];
    if (same_place(place, GODOT_HUD_PLACE_HAND) || same_place(place, GODOT_HUD_PLACE_GROUND)) {
        return "";
    }
    // A holster takes no vessel, nothing longer than the belt allows and nothing beside what it holds.
    if (same_place(place, GODOT_HUD_CONTAINER_BELT_FIRST) || same_place(place, GODOT_HUD_CONTAINER_BELT_SECOND)) {
        if (facts.cap_l > 0.0) {
            return "belt_vessel";
        }
        if (facts.length_cm > belt_len_cm_) {
            values.push_back(HudPortValue{"cm", number_text(facts.length_cm, -1)});
            return "belt_long";
        }
        for (const Carried& other : carried_) {
            if (&other != &thing && same_place(other.place, place)) {
                return "belt_taken";
            }
        }
        return "";
    }
    // The pack and the pouch: length against the opening, room against the room, weight against the carry.
    const bool pouch = same_place(place, GODOT_HUD_CONTAINER_POUCH);
    const ContainerDefCarry& container = pouch ? pouch_ : pack_;
    if (pouch && facts.id == GODOT_HUD_CONTAINER_POUCH) {
        return "self";
    }
    if (facts.length_cm > container.opening_cm) {
        values.push_back(HudPortValue{"cm", number_text(facts.length_cm, -1)});
        return "long";
    }
    double litres = room(thing);
    double kg = weight(thing);
    for (const Carried& other : carried_) {
        if (&other != &thing && same_place(other.place, place)) {
            litres += room(other);
            kg += weight(other);
        }
    }
    if (litres > container.room_l + GODOT_HUD_FIT_EPSILON) {
        values.push_back(HudPortValue{"l", number_text(litres, 1)});
        values.push_back(HudPortValue{"cap_l", number_text(container.room_l, -1)});
        return "full";
    }
    if (kg > container.carry_kg + GODOT_HUD_FIT_EPSILON) {
        values.push_back(HudPortValue{"kg", kg_text(kg)});
        return "heavy";
    }
    return "";
}

void VivariumCarryBridge::lay(Carried& thing, const ::godot::Vector3& point, bool rack) {
    const Facts& facts = facts_[thing.facts];
    // rackSlot: one slot further for each other tool at the place.
    if (rack) {
        int64_t others = 0;
        for (const Carried& other : carried_) {
            if (&other != &thing && same_place(other.place, GODOT_HUD_PLACE_GROUND) && other.rack) {
                ++others;
            }
        }
        thing.along = slot_first_ + static_cast<double>(others) * slot_step_;
    }
    thing.place = GODOT_HUD_PLACE_GROUND;
    thing.ground = point;
    thing.rack = rack;
    thing.angle = rack ? slot_angle_ : (facts.view_h > facts.view_w * lay_tall_ ? lay_angle_ : 0.0);
    thing.order = ++next_order_;
}

void VivariumCarryBridge::stow(Carried& thing) {
    const Home home = thing.home;
    thing.home = Home{};
    std::vector<HudPortValue> values;
    // Back home where it came from and still fits.
    if (home.set && !same_place(home.place, GODOT_HUD_PLACE_HAND) && fits(thing, home.place, values).empty()) {
        thing.place = home.place;
        thing.at = home.at;
        thing.ground = home.ground;
        thing.along = home.along;
        thing.angle = home.angle;
        thing.rack = home.rack;
        thing.order = ++next_order_;
        return;
    }
    // Else into the pack at a free spot, else to the tool place.
    if (fits(thing, GODOT_HUD_CONTAINER_PACK, values).empty()) {
        place_in(thing, GODOT_HUD_CONTAINER_PACK,
                 ::godot::Vector2(ase::math::random_float(static_cast<float>(stow_x_from_),
                                                          static_cast<float>(stow_x_from_ + stow_x_span_)),
                                  ase::math::random_float(static_cast<float>(stow_y_from_),
                                                          static_cast<float>(stow_y_from_ + stow_y_span_))));
        return;
    }
    lay(thing, rack_place_, true);
}

void VivariumCarryBridge::place_in(Carried& thing, const char* place, const ::godot::Vector2& at) {
    const Facts& facts = facts_[thing.facts];
    const ContainerDef& room = same_place(place, GODOT_HUD_CONTAINER_POUCH) ? pouch_room_ : pack_room_;
    // k = ik(o, c): the drawn box over the drawing's longer side; x within [0, C.w - D.vw*k], y within [0, C.h - D.vh*k].
    const double k = room.drawn_box(facts.length_cm) / ase::math::max(facts.view_w, facts.view_h);
    const double x = ase::math::max(0.0, ase::math::min(room.content_width() - facts.view_w * k,
                                                        static_cast<double>(at.x)));
    const double y = ase::math::max(0.0, ase::math::min(room.content_height() - facts.view_h * k,
                                                        static_cast<double>(at.y)));
    thing.place = place;
    thing.at = ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
    thing.order = ++next_order_;
}

void VivariumCarryBridge::equip(Carried& thing) {
    for (Carried& held : carried_) {
        if (&held != &thing && same_place(held.place, GODOT_HUD_PLACE_HAND)) {
            stow(held);
        }
    }
    if (same_place(thing.place, GODOT_HUD_PLACE_HAND)) {
        return;
    }
    thing.home.set = true;
    thing.home.place = thing.place;
    thing.home.at = thing.at;
    thing.home.ground = thing.ground;
    thing.home.along = thing.along;
    thing.home.angle = thing.angle;
    thing.home.rack = thing.rack;
    thing.place = GODOT_HUD_PLACE_HAND;
    thing.order = ++next_order_;
}

VivariumCarryBridge::Carried* VivariumCarryBridge::find(const std::string& key) {
    // findIt passes the trough under the tap (o.c !== 'trunk'): it hangs at the birch, no hand reaches it.
    for (Carried& thing : carried_) {
        if (thing.key == key && !same_place(thing.place, GODOT_HUD_PLACE_TRUNK)) {
            return &thing;
        }
    }
    for (Carried& thing : carried_) {
        if (facts_[thing.facts].id == key && thing.count > 0 && !same_place(thing.place, GODOT_HUD_PLACE_TRUNK)) {
            return &thing;
        }
    }
    return nullptr;
}

const VivariumCarryBridge::Carried* VivariumCarryBridge::find(const std::string& key) const {
    for (const Carried& thing : carried_) {
        if (thing.key == key && !same_place(thing.place, GODOT_HUD_PLACE_TRUNK)) {
            return &thing;
        }
    }
    for (const Carried& thing : carried_) {
        if (facts_[thing.facts].id == key && thing.count > 0 && !same_place(thing.place, GODOT_HUD_PLACE_TRUNK)) {
            return &thing;
        }
    }
    return nullptr;
}

bool VivariumCarryBridge::answer(const std::string& key, const char* place, bool rack, StowAnswer& out) const {
    const Carried* thing = find(key);
    if (thing == nullptr || place == nullptr) {
        return false;
    }
    out.key = thing->key;
    out.place = place;
    out.rack = rack;
    out.code = fits(*thing, place, out.values);
    return true;
}

bool VivariumCarryBridge::apply(const VivariumHudRequest& request, std::string& note) {
    const std::string kind = request.kind != nullptr ? request.kind : "";
    std::vector<HudPortValue> values;

    // useTool and equip: the thing into the hand, what the hand held stowed first.
    if (kind == VivariumHudRequest::ASK_TAKE || kind == VivariumHudRequest::ASK_EQUIP) {
        Carried* thing = find(request.key);
        if (thing == nullptr) {
            note = "missing " + request.key;
            return false;
        }
        equip(*thing);
        note = "equipped " + thing->key;
        return true;
    }

    // putAway: the hand's thing back where it came from.
    if (kind == VivariumHudRequest::ASK_STOW) {
        for (Carried& thing : carried_) {
            if (same_place(thing.place, GODOT_HUD_PLACE_HAND)) {
                stow(thing);
                note = std::string("stowed ") + thing.key + " to " + thing.place;
                return true;
            }
        }
        note = "hand empty";
        return false;
    }

    // A drop: the thing to the target's place when fits() lets it.
    if (kind == VivariumHudRequest::ASK_SEND) {
        Carried* thing = find(request.key);
        const char* place = request.target.place;
        if (thing == nullptr || place == nullptr) {
            note = "send without thing or place";
            return false;
        }
        const std::string code = fits(*thing, place, values);
        if (!code.empty()) {
            note = "refused " + thing->key + " to " + place + ": " + code;
            return false;
        }
        if (same_place(place, GODOT_HUD_PLACE_HAND)) {
            equip(*thing);
        } else if (same_place(place, GODOT_HUD_PLACE_GROUND)) {
            thing->home = Home{};
            lay(*thing, request.target.ground, request.target.rack);
        } else {
            // f = tg.auto ? freeSpot(tg.c, o) : tg; placeIn(o, tg.c, f.x, f.y): at the bag icon the spot is the
            // free cell DragLayer::free_spot found at the release, else the spot under the finger.
            thing->home = Home{};
            place_in(*thing, place, request.target.spot);
        }
        note = "sent " + thing->key + " to " + place;
        return true;
    }

    note = "not a carry act: " + kind;
    return false;
}

bool VivariumCarryBridge::brook(bool picked, const ::godot::Vector3& point, bool laid, const ::godot::Vector3& from,
                                const ::godot::Vector3& to, const char*& toast, double& litres, std::string& note) {
    // brookTap (m.html 968 to 970): only the can of the hand, only on the brook's line, then the fill.
    Carried* can = nullptr;
    for (Carried& thing : carried_) {
        if (same_place(thing.place, GODOT_HUD_PLACE_HAND) && facts_[thing.facts].id == can_item_) {
            can = &thing;
        }
    }
    if (!picked || !laid || can == nullptr) {
        note = can == nullptr ? "the hand holds no " + can_item_ : "the tap found no ground or no brook";
        return false;
    }
    // The distance of the tap to the brook's bed on the ground, the nearest point of the segment from to to.
    const float run_x = to.x - from.x;
    const float run_z = to.z - from.z;
    const float span = run_x * run_x + run_z * run_z;
    const float share =
        span > 0.0f
            ? ase::math::min(1.0f, ase::math::max(0.0f, ((point.x - from.x) * run_x + (point.z - from.z) * run_z) / span))
            : 0.0f;
    const float off_x = point.x - (from.x + run_x * share);
    const float off_z = point.z - (from.z + run_z * share);
    const double off = static_cast<double>(ase::math::sqrt(off_x * off_x + off_z * off_z));
    if (off > brook_reach_m_) {
        // The ground point the tap found and the bed point it was measured against stand in the line, so a miss
        // says where the finger landed (run 20261113-v12b-brook named only the distance, 3.375 m).
        note = "the tap at " + std::to_string(point.x) + "," + std::to_string(point.z) + " lies " +
               std::to_string(off) + " m from the brook at " + std::to_string(from.x + run_x * share) + "," +
               std::to_string(from.z + run_z * share);
        return false;
    }
    const double cap = facts_[can->facts].cap_l;
    litres = can->fill_l;
    if (can->fill_l >= cap) {
        toast = TOAST_POT_FULL;
        note = "the can is full";
        return false;
    }
    can->fill_l = cap;
    litres = cap;
    toast = TOAST_FILLED_BROOK;
    note = "filled " + can->key + " to " + std::to_string(cap) + " L";
    return true;
}

bool VivariumCarryBridge::sow(bool picked, const ::godot::Vector3& point, bool dead, const char*& toast,
                              std::string& note) {
    // sowTap's order (m.html 958 to 960): the place first, then the birch, then the pouch.
    const float across_x = point.x - stem_place_.x;
    const float across_z = point.z - stem_place_.z;
    const double off = static_cast<double>(ase::math::sqrt(across_x * across_x + across_z * across_z));
    if (!picked || off > sow_reach_m_) {
        toast = TOAST_OFF_PATCH;
        note = picked ? "the tap lies " + std::to_string(off) + " m from the stem place" : "the tap found no ground";
        return false;
    }
    if (!dead) {
        toast = TOAST_ALIVE;
        note = "the birch still lives";
        return false;
    }
    for (size_t index = 0; index < carried_.size(); ++index) {
        Carried& seed = carried_[index];
        if (facts_[seed.facts].id == seed_item_ && seed.count > 0) {
            --seed.count;
            if (seed.count <= 0) {
                carried_.erase(carried_.begin() + static_cast<std::ptrdiff_t>(index));
            }
            sow_pending_ = true;  // S.hole: the dibber's hole opens at the stem place (SowHole, N14)
            toast = TOAST_SOWN;
            note = "sowed one " + seed_item_;
            return true;
        }
    }
    toast = TOAST_NO_SEED;
    note = "no " + seed_item_ + " left";
    return false;
}

bool VivariumCarryBridge::poured(const std::string& key, double litres) {
    // The mockup's release: it.fill = max(0, it.fill - round(mm / 25)), the litres the pour held.
    Carried* can = find(key);
    if (can == nullptr || !(litres > 0.0) || !(can->fill_l > 0.0)) {
        return false;
    }
    can->fill_l = ase::math::max(0.0, can->fill_l - litres);
    return true;
}

bool VivariumCarryBridge::tap(const char*& toast, std::string& note) {
    // trunkTap: the trough first, then a spout; each refusal is the mockup's alert toast.
    Carried* vessel = find(vessel_item_);
    if (vessel == nullptr) {
        toast = VivariumHud::TOAST_NO_VESSEL;
        note = "no " + vessel_item_;
        return false;
    }
    Carried* spout = find(spout_item_);
    if (spout == nullptr) {
        toast = VivariumHud::TOAST_NO_SPOUT;
        note = "no " + spout_item_;
        return false;
    }
    // c.c = 'trunk', c.sap = 0, c.home = null, before the spout may leave INV and move every pointer.
    vessel->place = GODOT_HUD_PLACE_TRUNK;
    vessel->sap_l = 0.0;
    vessel->home = Home{};
    note = "tapped: " + vessel->key + " under the bore, one " + spout_item_;
    --spout->count;
    if (spout->count <= 0) {
        carried_.erase(carried_.begin() + (spout - carried_.data()));
    }
    // putAway(): the hand's thing (the auger) back where it came from.
    for (Carried& held : carried_) {
        if (same_place(held.place, GODOT_HUD_PLACE_HAND)) {
            stow(held);
            note += ", stowed " + held.key;
            break;
        }
    }
    toast = VivariumHud::TOAST_TAPPED;
    return true;
}

bool VivariumCarryBridge::plug(bool running, double held_l, std::string& note) {
    // plugTap: only while a tap runs and the hand holds a peg.
    size_t peg = carried_.size();
    for (size_t index = 0; index < carried_.size(); ++index) {
        if (same_place(carried_[index].place, GODOT_HUD_PLACE_HAND)) {
            peg = index;
        }
    }
    if (!running || peg == carried_.size() || facts_[carried_[peg].facts].id != plug_item_) {
        note = running ? "the hand holds no " + plug_item_ : std::string("no tap runs");
        return false;
    }
    // The trough takes what ran and is laid beside the stem: layAt(c, U0 + 0.028, Z0 + 0.014, false).
    for (Carried& vessel : carried_) {
        if (same_place(vessel.place, GODOT_HUD_PLACE_TRUNK) && facts_[vessel.facts].id == vessel_item_) {
            vessel.sap_l = held_l;
            lay(vessel,
                stem_place_ + rack_right_ * static_cast<float>(plug_across_) +
                    rack_view_ * static_cast<float>(plug_ahead_),
                false);
            note = vessel.key + " laid with " + number_text(held_l, 1) + " L, ";
            break;
        }
    }
    // it.n--, the last peg leaves INV; then putAway() stows what the hand still holds.
    Carried& held = carried_[peg];
    --held.count;
    note += "one " + plug_item_ + " closed the bore";
    if (held.count <= 0) {
        carried_.erase(carried_.begin() + static_cast<std::ptrdiff_t>(peg));
    } else {
        stow(held);
    }
    return true;
}

bool VivariumCarryBridge::flow(double seconds, double factor, double cap_l, double& held_l) const {
    // S.tap.l = min(1.5, S.tap.l + dh * 0.08) with dh = dt * S.factor / 8; 'COCHO CHEIO' on the crossing.
    const double was = held_l;
    held_l = ase::math::min(cap_l, held_l + seconds * factor / hour_s_ * sap_per_hour_);
    return was < cap_l && held_l >= cap_l;
}

}  // namespace ase::adp::godot

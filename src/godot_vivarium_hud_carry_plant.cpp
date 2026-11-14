/**
 * ASE GODOT ADAPTER - VIVARIUM HUD CARRY BRIDGE, THE PLANTING ACTS
 *
 * @file        godot_vivarium_hud_carry_plant.cpp
 * @brief       VivariumCarryBridge - dig the planting hole, lay the seed in, cover it, and the dibber's sowTap
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.7, operator decision 2026-11-13 (Q01 to Q04 of the TASK, before
 *              the mockup): the start shows bare soil at the stem place. The spade's cavar there opens the
 *              planting hole (dig), the sow tool lays one seed from the pouch into the open hole (sow), the spade's
 *              amontoar fills the hole over its seed (cover); the seed shows only while it lies open in the hole
 *              (TreeShade::draw). The mockup's one-tap dibber stays: sow() on a stem place without a dug hole
 *              judges the place, the birch and the pouch in the mockup's order (sowTap, m.html 958 to 960) and
 *              pricks, fills and closes in one tap.
 *
 *              THE PORT. flora.seed.v1 carries no planting hole yet (register N0485, carrier N0448): until it
 *              delivers place, hole_open, seeded, covered and at, the bridge holds dug_, seeded_ and covered_ and
 *              the view marks the gap on its edge (VivariumView::open_gap with GAP_PLANT). Every reach is the
 *              hud_bridge_hand_sow_reach_m token load() read (godot_vivarium_hud_carry_frame.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100388 - sow a new seed
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0485 (flora.seed.v1 planting hole), N0451 (flora.carry.v1 seed taken), carrier N0448
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_vivarium_hud_carry.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/vector3.hpp>

#include <cstddef>
#include <string>

namespace ase::adp::godot {

namespace {

/** The distance on the ground plane from `stem` to `point`, metres. */
double ground_off(const ::godot::Vector3& point, const ::godot::Vector3& stem) {
    const float across_x = point.x - stem.x;
    const float across_z = point.z - stem.z;
    return static_cast<double>(ase::math::sqrt(across_x * across_x + across_z * across_z));
}

}  // namespace

bool VivariumCarryBridge::sow(bool picked, const ::godot::Vector3& point, bool dead, const char*& toast,
                              std::string& note) {
    // sowTap's order (m.html 958 to 960): the place first, then the birch, then the pouch.
    const double off = ground_off(point, stem_place_);
    if (!picked || off > sow_reach_m_) {
        toast = TOAST_OFF_PATCH;
        note = picked ? "the tap lies " + std::to_string(off) + " m from the stem place" : "the tap found no ground";
        return false;
    }
    // THE PLANTING HOLE (Q03): a hole the spade dug holds no seed and no birch, so the seed goes into it without the
    // birch's word and without the dibber's closing; amontoar closes it. A second seed finds the hole taken.
    if (dug_ && seeded_) {
        toast = nullptr;
        note = "a seed already lies in the planting hole";
        return false;
    }
    if (!dug_ && !dead) {
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
            toast = TOAST_SOWN;
            if (dug_) {
                seeded_ = true;  // the seed lies open in the hole until amontoar covers it (TreeShade::draw)
                note = "laid one " + seed_item_ + " into the planting hole";
                return true;
            }
            sow_pending_ = true;  // S.hole: the dibber's hole opens at the stem place (SowHole, N14)
            note = "sowed one " + seed_item_;
            return true;
        }
    }
    toast = TOAST_NO_SEED;
    note = "no " + seed_item_ + " left";
    return false;
}

bool VivariumCarryBridge::dig(bool picked, const ::godot::Vector3& point, ::godot::Vector3& hole, std::string& note) {
    // cavar at the stem place (Q02): the same reach as sowTap's, RPATCH plus 0.03 of the field.
    const double off = ground_off(point, stem_place_);
    if (!picked || off > sow_reach_m_) {
        note = picked ? "the dig lies " + std::to_string(off) + " m from the stem place" : "the dig found no ground";
        return false;
    }
    if (covered_) {
        note = "a covered seed lies at the stem place";
        return false;
    }
    if (dug_) {
        note = "the planting hole is open already";
        return false;
    }
    dug_ = true;
    seeded_ = false;
    sown_ = false;              // the dibber's short hole gives way to the hole the spade dug
    sown_place_ = stem_place_;  // SowHole draws the open hole at the stem place
    hole = stem_place_;         // the spade digs there too, never at the tap within the reach (B0297)
    note = "dug the planting hole at the stem place";
    return true;
}

bool VivariumCarryBridge::cover(bool picked, const ::godot::Vector3& point, ::godot::Vector3& hole, std::string& note) {
    // amontoar over the planting hole (Q04): only a hole with its seed is filled.
    const double off = ground_off(point, stem_place_);
    if (!picked || off > sow_reach_m_) {
        note = picked ? "the fill lies " + std::to_string(off) + " m from the stem place" : "the fill found no ground";
        return false;
    }
    if (!dug_ || !seeded_) {
        note = dug_ ? "the planting hole holds no seed yet" : "no planting hole is open";
        return false;
    }
    dug_ = false;
    seeded_ = false;
    covered_ = true;    // the mound over the seed (TreeShade::draw), the hole is filled
    hole = sown_place_;  // the fill goes where dig() dug, so no heap rises beside the hole (B0297)
    note = "covered the seed in the planting hole";
    return true;
}

}  // namespace ase::adp::godot

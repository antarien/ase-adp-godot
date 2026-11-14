/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD VERBS
 *
 * @file        godot_vivarium_hud_verb.cpp
 * @brief       VivariumView::send_hud_verb - a terrain verb of the hand tool through its port, toasted once it acted
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5, the mockup's applyTerr (m.html 979 to 991): a verb of
 *              hud.verbs (bed, flatten, level, lower, raise, loosen, measure) reads the ground under the tap with its
 *              own brush, goes through the port that carries it, and toasts only once it acted; over the root zone
 *              the toast turns into the verb's warning.
 *
 *              THE PLANTING HOLE (operator 2026-11-13, before the mockup's one-tap dibber, Q02 and Q04 of the TASK):
 *              the spade's cavar at the stem place opens the planting hole and amontoar over its seed fills it
 *              (VivariumCarryBridge::dig and cover, godot_vivarium_hud_carry_plant.cpp); the sow tool lays the seed
 *              in between. flora.seed.v1 carries no hole yet (register N0485, carrier N0448), so the bridge holds it
 *              and its gap is logged once on the edge (open_gap), never shown.
 *
 *              BACKEND GAPS OF THE VERBS: N0456 loosen, N0464 measure, N0485 the planting hole.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100388 - sow a new seed
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

void VivariumView::send_hud_verb(const VivariumHudRequest& request) {
    const auto found = hud_verbs_.find(request.key);
    if (found == hud_verbs_.end()) {
        fail_hud("hud.verbs." + request.key);
        return;
    }
    const VivariumHudVerb& verb = found->second;
    const std::string& op = verb.op;
    settle_bed_choice();  // a verb or mode chosen earlier in this frame drops an old start before this tap lays one
    // The brush lies in mockup units; the ports read the ground under it on the canvas.
    const ::godot::Vector2 at = hud_frame_.xform(request.point);
    // THE READINGS AT THE TAP. hud_now_ was laid in the frame before the finger set down, its brush still where the
    // last tap left it (run v13-enxada-014635: soltar beside the ring toasted plain while the panel named the zone,
    // amontoar at the same spot warned from the stem tap before). applyTerr asks inRoot, hAt and slopeAt where the
    // finger set down (m.html 979), so the hand's ground is read again at this tap with the verb's own brush.
    HandSlotFrame hand = hud_now_.hand;
    hand.brush_m = verb.brush_m;
    GroundReticleFrame reticle = hud_now_.terrain.reticle;
    float tap_x = 0.0f;
    float tap_z = 0.0f;
    float nearest_px = -1.0f;
    reticle.brush_on = pick_ground(at, tap_x, tap_z, nearest_px);
    reticle.brush_x = static_cast<double>(tap_x);
    reticle.brush_z = static_cast<double>(tap_z);
    hud_hand_.ground(hand, reticle);
    // applyTerr (m.html 979 to 991): a verb toasts only once it acted. The ports that carry an act answer whether
    // they took it (bed, flatten, level, lower, raise; the bed's first tap lays nothing); the acts no port carries yet act in the
    // bridge's frame, their gap logged once on the edge.
    bool acted = false;
    std::vector<HudPortValue> values;
    if (op == VivariumHud::OP_BED) {
        acted = bed(at);
    } else if (op == VivariumHud::OP_FLATTEN) {
        acted = flatten(at, static_cast<float>(verb.brush_m));
    } else if (op == VivariumHud::OP_LEVEL) {
        if (selected_place_ == GODOT_NO_PLACE && !start_.patches.empty()) {
            select(patch_focus_place(selected_place_, start_.patches.front().place));
        }
        acted = level();
    } else if (op == VivariumHud::OP_LOWER || op == VivariumHud::OP_RAISE) {
        const bool lowers = op == VivariumHud::OP_LOWER;
        // THE PLANTING HOLE (usecase:100388, Q02 and Q04): cavar at the stem place opens it, amontoar over its seed
        // fills it. flora.seed.v1 carries no hole yet (N0485): the bridge holds it and the gap is marked once. The
        // bridge answers first, so a hole is dug and filled on its own ground and never on the two taps within the
        // reach (B0297, run 20261113-133247: lower at z 15.88, raise at z 15.62, the heap rose beside the hole).
        std::string planted;
        const ::godot::Vector3 ground(tap_x, 0.0f, tap_z);
        ::godot::Vector3 hole = ground;
        const bool holed = lowers ? hud_carry_.dig(reticle.brush_on, ground, hole, planted)
                                  : hud_carry_.cover(reticle.brush_on, ground, hole, planted);
        if (holed) {
            open_gap(VivariumCarryBridge::GAP_PLANT, request.kind);
        }
        note(::godot::String(("hud plant " + std::string(lowers ? "dig " : "cover ") +
                              (holed ? "done: " : "refused: ") + planted).c_str()));
        if (!reticle.brush_on) {
            note("hud verb " + ::godot::String(request.key.c_str()) + " at " + ::godot::String::num(at.x, 0) + "," +
                 ::godot::String::num(at.y, 0) + " result=none nearest_px=" + ::godot::String::num(nearest_px, 1));
        } else {
            acted = lowers ? lower(hole.x, hole.z, static_cast<float>(verb.brush_m), static_cast<float>(verb.depth_m))
                           : raise(hole.x, hole.z, static_cast<float>(verb.brush_m), static_cast<float>(verb.depth_m));
        }
    } else if (op == VivariumHud::OP_LOOSEN) {
        // applyTerr soltar (m.html 980): the bridge loosens the points under the brush while flora.patch.v1 carries
        // no loose soil (N0456); the crumbs and the patch's share read it from there
        open_gap(GODOT_HUD_GAP_LOOSE, request.kind);
        if (reticle.brush_on) {
            hud_loose_.loosen(reticle.brush_x, reticle.brush_z, verb.brush_m);
        }
        acted = true;
    } else if (op == VivariumHud::OP_MEASURE) {
        // ALT f1(hAt) M · DECLIVE round(slopeAt)°: the ground under the brush as the bridge reads it (N0464).
        open_gap(VivariumHud::GAP_MEASURE, request.kind);
        values.push_back(HudPortValue{"m", hud_decimals(hand.height_m, 1)});
        values.push_back(HudPortValue{"deg", hud_decimals(hand.slope_deg, 0)});
        acted = true;
    } else {
        fail_hud("hud.verbs." + request.key + ".op");
        return;
    }
    if (!acted) {
        return;
    }
    // THE ROOT ZONE. A brush over the roots turns the toast into the verb's warning in DOMAIN_ALERT: loosening hurts
    // only living roots (inRoot && !S.dead), shaping warns of the zone whatever the birch's state; a verb whose
    // roots word is empty warns of nothing (the bed, the level of the whole patch, the measure).
    const bool warn = !verb.roots.empty() && hand.in_roots && (op != VivariumHud::OP_LOOSEN || !hand.dead);
    if (warn && op == VivariumHud::OP_LOOSEN) {
        // RAÍZES FERIDAS costs the bridge's growth (S.grow - 4 of 100, m.html 980), N0454 and N0456
        hud_bridge_growth_ = ase::math::max(0.0, hud_bridge_growth_ - hud_loose_.root_cost());
    }
    const std::vector<HudPortValue> none;
    const std::string colour =
        warn ? std::string(GODOT_HUD_COLOUR_REFUSED)
             : std::string("DOMAIN_") + ::godot::String(verb.tone.c_str()).to_upper().utf8().get_data();
    note(::godot::String(("hud verb " + request.key + " toast=" + (warn ? verb.roots : verb.toast)).c_str()));
    std::string failed;
    if (!hud_.toast(warn ? verb.roots : verb.toast, warn ? none : values, colour, failed)) {
        fail_hud(failed);
    }
}

}  // namespace ase::adp::godot

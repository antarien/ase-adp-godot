/**
 * ASE GODOT ADAPTER - VIVARIUM HUD HAND BRIDGE
 *
 * @file        godot_vivarium_hud_hand.cpp
 * @brief       VivariumHandBridge - the hand's readings from the frame and the mockup's rules, see the header
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.4 and the bridge section, see godot_vivarium_hud_hand.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0454 (flora.patch.v1 preview after the pour), N0463 (flora.root.v1 zone and segments), N0462
 *              (flora.scene.v1 cuttable, usecase:100365 prune a branch), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_hud_hand.hpp>
#include <ase/adp/godot/godot_hud_ui_rng.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/core/math.hpp>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <unordered_map>
#include <utility>

namespace ase::adp::godot {

bool VivariumHandBridge::load(const UiTheme& theme, std::string& failed) {
    pouring_ = false;
    measures_ = 0;
    soil_start_ = 0.0;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_bridge_hand_mm_per_l", &mm_per_l_},
             {"hud_bridge_hand_rel_per_mm", &rel_per_mm_},
             {"hud_bridge_hand_root_m", &root_m_},
             {"hud_bridge_hand_slope_step_m", &slope_step_m_}}) {
        if (!theme.measure(slot.first, *slot.second) || !(*slot.second > 0.0)) {
            failed = std::string(slot.first) + ": a number above 0";
            return false;
        }
    }
    // TREE.cand's rule: whole counts, a step of at least one, a phase below the step, at least one shoot.
    chosen_.clear();
    cut_body_ = GODOT_NO_OBJECT;
    double skip = 0.0;
    double step = 0.0;
    double phase = 0.0;
    double most = 0.0;
    if (!theme.measure("hud_bridge_hand_cut_skip", skip) || !theme.measure("hud_bridge_hand_cut_step", step) ||
        !theme.measure("hud_bridge_hand_cut_phase", phase) || !theme.measure("hud_bridge_hand_cut_most", most) ||
        skip < 0.0 || step < 1.0 || phase < 0.0 || phase >= step || most < 1.0 ||
        skip != ::godot::Math::floor(skip) || step != ::godot::Math::floor(step) ||
        phase != ::godot::Math::floor(phase) || most != ::godot::Math::floor(most)) {
        failed = "hud_bridge_hand_cut_skip, _step, _phase, _most: whole, step from 1, phase below step, most from 1";
        return false;
    }
    cut_skip_ = static_cast<uint32_t>(skip);
    cut_step_ = static_cast<uint32_t>(step);
    cut_phase_ = static_cast<uint32_t>(phase);
    cut_most_ = static_cast<uint32_t>(most);
    return load_roots(theme, failed);
}

bool VivariumHandBridge::load_roots(const UiTheme& theme, std::string& failed) {
    // drawRootCut's tree: whole counts for the seed, the stage and the depth; lengths above 0, the spreads from 0.
    roots_.clear();
    double seed = 0.0;
    double stage = 0.0;
    double depth = 0.0;
    if (!theme.measure("hud_bridge_root_seed", seed) || !theme.measure("hud_bridge_root_stage", stage) ||
        !theme.measure("hud_bridge_root_depth", depth) || seed < 0.0 || seed > static_cast<double>(UINT32_MAX) ||
        stage < 0.0 || stage >= static_cast<double>(GODOT_STAGE_COUNT) || depth < 0.0 ||
        seed != ::godot::Math::floor(seed) || stage != ::godot::Math::floor(stage) ||
        depth != ::godot::Math::floor(depth)) {
        failed = "hud_bridge_root_seed, _stage, _depth: whole, seed from 0, stage a GODOT_STAGE_*, depth from 0";
        return false;
    }
    root_seed_ = static_cast<uint32_t>(seed);
    root_stage_ = static_cast<uint32_t>(stage);
    root_depth_ = static_cast<uint32_t>(depth);
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_bridge_root_length", &root_length_}, {"hud_bridge_root_shrink", &root_shrink_}}) {
        if (!theme.measure(slot.first, *slot.second) || !(*slot.second > 0.0)) {
            failed = std::string(slot.first) + ": a number above 0";
            return false;
        }
    }
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_bridge_root_length_grown", &root_length_grown_},
             {"hud_bridge_root_turn", &root_turn_},
             {"hud_bridge_root_turn_spread", &root_turn_spread_},
             {"hud_bridge_root_shrink_spread", &root_shrink_spread_}}) {
        if (!theme.measure(slot.first, *slot.second) || *slot.second < 0.0) {
            failed = std::string(slot.first) + ": a number from 0";
            return false;
        }
    }
    root_scale_.clear();
    for (uint32_t index = 0; index < GODOT_STAGE_COUNT; ++index) {
        const std::string token = "hud_bridge_root_stage_" + std::to_string(index + 1) + "_scale";
        double scale = 0.0;
        if (!theme.measure(token, scale) || scale < 0.0) {
            failed = token + ": a number from 0";
            return false;
        }
        root_scale_.push_back(scale);
    }
    // The centre of the cut is the cut's own token: the tree starts where RootCut opens the ground.
    if (!theme.measure("hud_root_cut_dy", root_dy_)) {
        failed = "hud_root_cut_dy";
        return false;
    }
    return true;
}

void VivariumHandBridge::pour() {
    if (pouring_) {
        ++measures_;
    }
}

double VivariumHandBridge::pour_end(double fill_l) {
    const double taken = pouring_ ? litres(fill_l) : 0.0;
    pouring_ = false;
    measures_ = 0;
    return taken;
}

double VivariumHandBridge::litres(double fill_l) const {
    return ase::math::min(ase::math::max(0.0, fill_l), 1.0 + static_cast<double>(measures_));
}

void VivariumHandBridge::frame(HandSlotFrame& hand, const VivariumPatchView* patch, const GroundReticleFrame& reticle,
                               double sap_l, bool holding) {
    // THE WATER: the soil of the patch record; the press keeps it as the mockup's S.soil until the release. The
    // hold is the HUD's own (`holding`): this frame was laid anew by fill_hud, and compose writes hand.holding only
    // after the bridges ran, so the field read here would always stand false and no pour would ever start.
    const double soil = patch != nullptr ? patch->soil_rel : 0.0;
    const double fill_l = hand.thing != nullptr ? hand.thing->fill_l : 0.0;
    if (holding && !pouring_) {
        pouring_ = true;
        measures_ = 0;
        soil_start_ = soil;
    }
    hand.soil_rel = pouring_ ? soil_start_ : soil;
    hand.poured_l = pouring_ ? litres(fill_l) : 0.0;
    hand.pour_mm = hand.poured_l * mm_per_l_;
    const double preview_mm = pouring_ ? hand.pour_mm : fill_l > 0.0 ? mm_per_l_ : 0.0;
    hand.after_rel = ase::math::min(1.0, hand.soil_rel + preview_mm * rel_per_mm_);
    hand.dead = patch != nullptr && patch->condition == GODOT_COND_DEAD;
    ground(hand, reticle);

    // THE SAP of a running tap: the bridge's.
    hand.tap_l = sap_l;
}

void VivariumHandBridge::ground(HandSlotFrame& hand, const GroundReticleFrame& reticle) const {
    // THE GROUND under the brush: the terrain port's height, the slope across slopeAt's step, the mockup's roots.
    hand.brush_on = reticle.brush_on;
    hand.in_roots = false;
    if (reticle.brush_on) {
        const double x = reticle.brush_x;
        const double z = reticle.brush_z;
        double here = 0.0;
        double east = 0.0;
        double west = 0.0;
        double north = 0.0;
        double south = 0.0;
        const UiGround* ground = reticle.ground;
        if (ground != nullptr && ground->height(x, z, here) && ground->height(x + slope_step_m_, z, east) &&
            ground->height(x - slope_step_m_, z, west) && ground->height(x, z + slope_step_m_, north) &&
            ground->height(x, z - slope_step_m_, south)) {
            const double dx = (east - west) / (2.0 * slope_step_m_);
            const double dz = (north - south) / (2.0 * slope_step_m_);
            const auto rise = ase::math::sqrt(static_cast<float>(dx * dx + dz * dz));
            hand.height_m = here;
            hand.slope_deg = static_cast<double>(::godot::Math::rad_to_deg(ase::math::atan2(rise, 1.0f)));
        }
        const double ex = x - reticle.patch_x;
        const double ez = z - reticle.patch_z;
        const double reach = root_m_ + hand.brush_m;
        hand.in_roots = ex * ex + ez * ez < reach * reach;
    }
}

void VivariumHandBridge::cut(HandSlotFrame& hand, std::vector<uint32_t>& cuttable, uint32_t body,
                             const std::vector<VivariumOrganView>& organs,
                             const std::vector<VivariumInstanceView>& segments) {
    // THE CUT: the records of the plant's internodes name their order and radius; the stem is the lowest order.
    std::unordered_map<uint32_t, const VivariumOrganView*> record_of;
    double stem_order = 0.0;
    bool any = false;
    for (const VivariumOrganView& organ : organs) {
        if (organ.body != body) {
            continue;
        }
        record_of[organ.object_id] = &organ;
        if (!any || organ.order < stem_order) {
            stem_order = organ.order;
            any = true;
        }
    }

    // A side shoot's base (its origin row) lies on a stem internode: nearer its tube's axis (origin along the y
    // column) than the two radii together. Rows: x, y, z axis and origin per row (GODOT_INSTANCE_VALUES).
    std::vector<std::pair<double, uint32_t>> roots;  // the height of the base, the shoot
    for (const VivariumInstanceView& shoot : segments) {
        const auto own = record_of.find(shoot.object_id);
        if (own == record_of.end() || own->second->order != stem_order + 1.0) {
            continue;
        }
        const double px = static_cast<double>(shoot.rows[3]);
        const double py = static_cast<double>(shoot.rows[7]);
        const double pz = static_cast<double>(shoot.rows[11]);
        for (const VivariumInstanceView& stem : segments) {
            const auto bearer = record_of.find(stem.object_id);
            if (bearer == record_of.end() || bearer->second->order != stem_order) {
                continue;
            }
            const double ax = static_cast<double>(stem.rows[1]);
            const double ay = static_cast<double>(stem.rows[5]);
            const double az = static_cast<double>(stem.rows[9]);
            const double bx = px - static_cast<double>(stem.rows[3]);
            const double by = py - static_cast<double>(stem.rows[7]);
            const double bz = pz - static_cast<double>(stem.rows[11]);
            const double span = ax * ax + ay * ay + az * az;
            const double along =
                span > 0.0 ? ase::math::min(1.0, ase::math::max(0.0, (bx * ax + by * ay + bz * az) / span)) : 0.0;
            const double dx = bx - ax * along;
            const double dy = by - ay * along;
            const double dz = bz - az * along;
            const double reach = bearer->second->radius + own->second->radius;
            if (dx * dx + dy * dy + dz * dz <= reach * reach) {
                roots.emplace_back(py, shoot.object_id);
                break;
            }
        }
    }
    std::sort(roots.begin(), roots.end());

    // The mockup's TREE.cand, chosen once per plant in focus and kept while it stands, as the mockup keeps TREE.
    if (body != cut_body_ || chosen_.empty()) {
        cut_body_ = body;
        chosen_.clear();
        for (size_t k = 0; k < roots.size() && chosen_.size() < cut_most_; ++k) {
            if (k >= cut_skip_ && k % cut_step_ == cut_phase_) {
                chosen_.push_back(roots[k].second);
            }
        }
    }

    // The chosen shoots still on the organ port: a cut takes its shoot away there.
    cuttable.clear();
    for (const uint32_t shoot : chosen_) {
        if (record_of.find(shoot) != record_of.end()) {
            cuttable.push_back(shoot);
        }
    }
    hand.prune_left = static_cast<uint32_t>(cuttable.size());

    // The counts behind the reading, told on their flank (cut_changed): a side axis without an internode has no
    // record here, so side=0 under a standing stem means the plant offers nothing to cut yet (run
    // 20261113-c01-motion3: stage 2 with axes 8 but internodes 9, all on the stem).
    size_t side = 0;
    for (const auto& entry : record_of) {
        if (entry.second->order == stem_order + 1.0) {
            ++side;
        }
    }
    cut_line_ = "hud prune body=" + std::to_string(body) +
                " stem_order=" + std::to_string(static_cast<uint32_t>(stem_order)) +
                " internodes=" + std::to_string(record_of.size()) + " side=" + std::to_string(side) +
                " on_stem=" + std::to_string(roots.size()) + " chosen=" + std::to_string(chosen_.size()) +
                " left=" + std::to_string(hand.prune_left);
}

bool VivariumHandBridge::cut_changed(std::string& line) {
    if (cut_line_ == cut_told_) {
        return false;
    }
    cut_told_ = cut_line_;
    line = cut_line_;
    return true;
}

double VivariumHandBridge::zone_m() const {
    return root_m_;
}

const std::vector<RootSegment>& VivariumHandBridge::roots(const VivariumPatchView* patch, const UiAnchor& anchor,
                                                          const PlantOrigin& origin) {
    // THE ROOTS: drawRootCut's root(TX, by + 2, PI / 2, 16 + SCALE[S.stage] * 16, 0) under the projected trunk.
    roots_.clear();
    ::godot::Vector2 trunk;
    if (root_scale_.size() < GODOT_STAGE_COUNT || !anchor.trunk(origin, trunk)) {
        return roots_;
    }
    const uint32_t stage =
        patch != nullptr && patch->stage < GODOT_STAGE_COUNT ? static_cast<uint32_t>(patch->stage) : root_stage_;
    uint32_t state = root_seed_;
    branch(anchor, ::godot::Vector3(origin.x, origin.y, origin.z),
           trunk + ::godot::Vector2(0.0f, static_cast<float>(root_dy_)), static_cast<double>(ase::math::HALF_PI),
           root_length_ + root_length_grown_ * root_scale_[stage], 0, state);
    return roots_;
}

void VivariumHandBridge::branch(const UiAnchor& anchor, const ::godot::Vector3& base, const ::godot::Vector2& at,
                                double angle, double length, uint32_t depth, uint32_t& state) {
    const float turn = static_cast<float>(angle);
    const ::godot::Vector2 end =
        at + ::godot::Vector2(ase::math::cos(turn), ase::math::sin(turn)) * static_cast<float>(length);
    RootSegment segment;
    segment.order = depth;
    // A segment the camera cannot lift stays out; its forks still take their draws in the mockup's order.
    if (anchor.lift(at, base, segment.from) && anchor.lift(end, base, segment.to)) {
        roots_.push_back(segment);
    }
    if (depth >= root_depth_) {
        return;
    }
    // root(x2, y2, a + (k ? 1 : -1) * (0.35 + r() * 0.5), l * (0.62 + r() * 0.15), d + 1): the turn's draw first.
    for (const double side : {-1.0, 1.0}) {
        const double bend = root_turn_ + ui_rng_draw(state) * root_turn_spread_;
        const double shrink = root_shrink_ + ui_rng_draw(state) * root_shrink_spread_;
        branch(anchor, base, end, angle + side * bend, length * shrink, depth + 1, state);
    }
}

}  // namespace ase::adp::godot

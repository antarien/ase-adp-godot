#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM HUD HAND BRIDGE
 *
 * @file        godot_vivarium_hud_hand.hpp
 * @brief       The readings of the hand the bridge writes into the HUD's frame while their ports are silent
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.4 and the bridge section, design decision 2026-11-10: the HUD
 *              draws every element from its frame and never asks whether a port delivered. The hand's readings
 *              stand on what this frame carries and on the mockup's render() and frame() where a port reports
 *              nothing yet, every rule a hud_bridge_hand_* token of SHA (theme.css --ase-hud-bridge-hand-*).
 *
 *              THE WATER. The soil water is the patch record's (flora.patch.v1). Its preview after the pour
 *              (N0454) is the mockup's: a press pours one litre at once and one more with each measure the
 *              held can asks, never more than the can holds; each litre is hud_bridge_hand_mm_per_l, and the
 *              soil after is the soil at the press plus the millimetres as the mockup adds them to its per
 *              cent (hud_bridge_hand_rel_per_mm). While no finger holds the can the preview is one litre.
 *
 *              THE GROUND. Height and slope under the brush are the terrain port's samples (UiGround), the
 *              slope read hud_bridge_hand_slope_step_m to each side as the mockup's slopeAt. The root zone
 *              (N0463) is the mockup's ring of hud_bridge_hand_root_m around the patch: the brush lies in it
 *              while its distance is under that ring plus the brush.
 *
 *              THE SAP. The litres of a running tap are the bridge's tap (VivariumView, hud_bridge_patch_sap_l).
 *
 *              THE CUT (usecase:100365 prune a branch). While flora.scene.v1 marks no internode that may be
 *              cut (N0462), the marks are the mockup's TREE.cand on the real plant: the side shoots are the
 *              internodes one order above the stem whose base lies on a stem internode (closer to its axis
 *              than both radii together), counted from the lowest base up; of them every one from index
 *              hud_bridge_hand_cut_skip whose index leaves hud_bridge_hand_cut_phase over
 *              hud_bridge_hand_cut_step, at most hud_bridge_hand_cut_most. The choice is kept while the plant
 *              stays in focus, as the mockup keeps TREE: a cut removes its shoot from the organ port, and the
 *              marks and the reading prune_left count the chosen shoots still standing.
 *
 *              THE ROOTS IN THE CUT. While flora.root.v1 grows no segment (N0463), the cut under the trunk
 *              shows the tree of the mockup's drawRootCut(): from the centre of the cut, hud_root_cut_dy below
 *              the trunk, straight down, hud_bridge_root_length plus hud_bridge_root_length_grown times the
 *              stage's scale (hud_bridge_root_stage_<n>_scale) long; each segment forks into two down to
 *              hud_bridge_root_depth, the first turned to the left, the second to the right, each by
 *              hud_bridge_root_turn plus a draw times hud_bridge_root_turn_spread and as long as
 *              hud_bridge_root_shrink plus a draw times hud_bridge_root_shrink_spread of its parent. Every draw
 *              is the mockup's rng(hud_bridge_root_seed), taken in the mockup's order. The stage is the
 *              patch's, without one hud_bridge_root_stage. The tree is laid in mockup units and lifted into the
 *              world at the depth of the stem base (UiAnchor::lift), so the cut places each segment where the
 *              mockup draws it; a segment's order is its depth, which is the width rule of the cut.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0454 (flora.patch.v1 preview after the pour), N0463 (flora.root.v1 zone and segments), N0462
 *              (flora.scene.v1 cuttable), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_hud_ground_reticle.hpp>
#include <ase/adp/godot/godot_hud_hand_mode.hpp>
#include <ase/adp/godot/godot_hud_root_cut.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The hand's readings while flora.patch.v1 previews no pour and flora.root.v1 reports no root zone. */
class VivariumHandBridge {
public:
    /** Reads the hud_bridge_hand_* rules of SHA; false with the missing token in `failed`. */
    bool load(const UiTheme& theme, std::string& failed);

    /** ASK_POUR: the held can pours one more litre. */
    void pour();

    /** ASK_POUR_END: the finger left the can; the litres the pour took from the can of `fill_l`. */
    double pour_end(double fill_l);

    /** The millimetres one litre of the can adds on the patch (hud_bridge_hand_mm_per_l): the pour's amount. */
    double mm_per_l() const { return mm_per_l_; }

    /**
     * Writes the soil now and after, the pour, height, slope, root zone and the tap into `hand`, from the
     * patch record, the brush on `reticle`, the bridge's tap of `sap_l` litres and `holding`, the HUD's hold
     * on the can (VivariumHud::pouring): the frame is laid anew before compose writes hand.holding.
     */
    void frame(HandSlotFrame& hand, const VivariumPatchView* patch, const GroundReticleFrame& reticle, double sap_l,
               bool holding);

    /**
     * Writes the ground under the brush of `reticle` into `hand`: brush_on, height, slope and in_roots against the
     * ring around reticle's patch widened by hand.brush_m. frame() reads it at the frame's brush; a verb reads it
     * at its own tap (VivariumView::send_hud_verb), as the mockup's applyTerr asks inRoot where the finger set down.
     */
    void ground(HandSlotFrame& hand, const GroundReticleFrame& reticle) const;

    /**
     * Writes the side shoots of the plant on patch object `body` that may be cut into `cuttable` and their
     * number into hand.prune_left, from the internode records `organs` and their instances `segments`.
     */
    void cut(HandSlotFrame& hand, std::vector<uint32_t>& cuttable, uint32_t body,
             const std::vector<VivariumOrganView>& organs, const std::vector<VivariumInstanceView>& segments);

    /**
     * The flank of the cut's reading: true once when the counts behind hand.prune_left changed since the last
     * call, with the line naming them in `line` (body, stem order, internodes, side internodes, side shoots on the
     * stem, chosen, left). A plant whose side axes carry no internode yet reads 0 here: nothing stands to be cut.
     */
    bool cut_changed(std::string& line);

    /** The radius of the root zone, metres: the mockup's RROOT while flora.root.v1 reports none (N0463). */
    [[nodiscard]] double zone_m() const;

    /**
     * The mockup's root tree under the stem base `origin` of the plant on `patch`, lifted into the world
     * through `anchor`; empty while the trunk lies behind the camera.
     */
    const std::vector<RootSegment>& roots(const VivariumPatchView* patch, const UiAnchor& anchor,
                                          const PlantOrigin& origin);

private:
    /** The litres of the pour held now for a can of `fill_l`: one at the press, one more per measure. */
    double litres(double fill_l) const;

    /** Reads the hud_bridge_root_* rules and hud_root_cut_dy of SHA; false with the failing token in `failed`. */
    bool load_roots(const UiTheme& theme, std::string& failed);

    /**
     * The mockup's root(x, y, a, l, d): the segment from `at` (mockup units) along `angle` over `length`,
     * lifted at the depth of `base`, then its two forks while `depth` lies under hud_bridge_root_depth.
     */
    void branch(const UiAnchor& anchor, const ::godot::Vector3& base, const ::godot::Vector2& at, double angle,
                double length, uint32_t depth, uint32_t& state);

    double mm_per_l_ = 0.0;      // hud_bridge_hand_mm_per_l: 1 L = 25 MM
    double rel_per_mm_ = 0.0;    // hud_bridge_hand_rel_per_mm: the mockup's per cent of soil water per millimetre
    double root_m_ = 0.0;        // hud_bridge_hand_root_m: RROOT of the mockup's field
    double slope_step_m_ = 0.0;  // hud_bridge_hand_slope_step_m: slopeAt's e of the mockup's field
    bool pouring_ = false;       // a held can pours since its press
    uint32_t measures_ = 0;      // the measures the held can asked since its press
    double soil_start_ = 0.0;    // the soil water at the press, 0..1
    uint32_t cut_skip_ = 0;      // hud_bridge_hand_cut_skip: TREE.cand's k >= 2
    uint32_t cut_step_ = 0;      // hud_bridge_hand_cut_step: TREE.cand's k % 2
    uint32_t cut_phase_ = 0;     // hud_bridge_hand_cut_phase: TREE.cand's === 1
    uint32_t cut_most_ = 0;      // hud_bridge_hand_cut_most: TREE.cand's slice(0, 5)
    uint32_t cut_body_ = 0;      // the patch object the choice was made on
    std::vector<uint32_t> chosen_;  // the side shoots chosen on it, lowest first
    std::string cut_line_;          // the counts behind prune_left of the last cut
    std::string cut_told_;          // the counts last told on the flank
    uint32_t root_seed_ = 0;          // hud_bridge_root_seed: drawRootCut's rng(3)
    double root_length_ = 0.0;        // hud_bridge_root_length: the 16 of 16 + SCALE[S.stage] * 16
    double root_length_grown_ = 0.0;  // hud_bridge_root_length_grown: the SCALE's 16
    uint32_t root_stage_ = 0;         // hud_bridge_root_stage: the mockup's S.stage without a patch
    std::vector<double> root_scale_;  // hud_bridge_root_stage_<n>_scale: SCALE, one per GODOT_STAGE_*
    uint32_t root_depth_ = 0;         // hud_bridge_root_depth: the d < 4 of root()
    double root_turn_ = 0.0;          // hud_bridge_root_turn: the 0.35 of a fork's turn
    double root_turn_spread_ = 0.0;   // hud_bridge_root_turn_spread: the 0.5 times a draw
    double root_shrink_ = 0.0;        // hud_bridge_root_shrink: the 0.62 of a fork's length
    double root_shrink_spread_ = 0.0; // hud_bridge_root_shrink_spread: the 0.15 times a draw
    double root_dy_ = 0.0;            // hud_root_cut_dy: the centre of the cut below the trunk, mockup units
    std::vector<RootSegment> roots_;  // the tree of the last roots() call
};

}  // namespace ase::adp::godot

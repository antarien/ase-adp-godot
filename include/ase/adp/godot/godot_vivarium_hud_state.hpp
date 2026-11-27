#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW: THE HUD STATE
 *
 * @file        godot_vivarium_hud_state.hpp
 * @brief       VivariumHudState - the HUD of the care view, its frame, its clock and the bridges it reads while a port is silent
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5 and the survival HUD of phase 08: the HUD with every block, the frame
 *              it was last resolved with, the horizon, ground, leaf and brook looks bound from its theme, the anchor
 *              and the ground of the scene camera, the bridges for the carry, the hand, the loose soil, the signs,
 *              the trail and the stow verdict, the verbs, what the last frame saw, the frame onto the canvas, the
 *              HUD's clock, its refresh and the frames per second, the still mode, the mode and the finger, the gaps
 *              logged, the mockup's values of hud_bridge_* of SHA and the debug places last logged.
 *
 *              ONE RESPONSIBILITY, ITS OWN HEADER: VivariumView (godot_vivarium_node.hpp) holds this record as its
 *              member hud_state_ (composition, the validator forbids inheritance), so its member functions in
 *              godot_vivarium_hud*.cpp, ..._frame_rate.cpp and the scene and figure files read these members as
 *              hud_state_.<name>. A new member of the HUD's state is written here, never into the view's header
 *              (operator order 2026-11-25: the view's header stood at the limit of 500 lines).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    R0221 (the split of the view's header)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_clock_view.hpp>
#include <ase/adp/godot/godot_host_organ_view.hpp>
#include <ase/adp/godot/godot_hud_brook_reflection.hpp>
#include <ase/adp/godot/godot_hud_compose.hpp>
#include <ase/adp/godot/godot_hud_forecast_strip.hpp>
#include <ase/adp/godot/godot_hud_ground_season.hpp>
#include <ase/adp/godot/godot_hud_horizon_ridge.hpp>
#include <ase/adp/godot/godot_hud_leaf_season.hpp>
#include <ase/adp/godot/godot_hud_survival_air_mark.hpp>
#include <ase/adp/godot/godot_hud_survival_body_rail.hpp>
#include <ase/adp/godot/godot_hud_survival_fire_card.hpp>
#include <ase/adp/godot/godot_hud_survival_rest_card.hpp>
#include <ase/adp/godot/godot_hud_survival_spoil_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_activity_art.hpp>
#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_frame_rate.hpp>
#include <ase/adp/godot/godot_hud_ui_ground.hpp>
#include <ase/adp/godot/godot_hud_ui_refresh.hpp>
#include <ase/adp/godot/godot_vivarium_hud_arc.hpp>
#include <ase/adp/godot/godot_vivarium_hud_carry.hpp>
#include <ase/adp/godot/godot_vivarium_hud_hand.hpp>
#include <ase/adp/godot/godot_vivarium_hud_loose.hpp>
#include <ase/adp/godot/godot_vivarium_hud_sample.hpp>
#include <ase/adp/godot/godot_vivarium_hud_sign_rules.hpp>
#include <ase/adp/godot/godot_vivarium_hud_trail.hpp>
#include <ase/adp/godot/godot_vivarium_view_config.hpp>
#include <ase/adp/godot/patch/types.hpp>

#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/** The HUD of the care view and everything it holds between two frames (06.5, survival HUD of phase 08). */
struct VivariumHudState {
    VivariumHud              hud_;              // the HUD of the care view (06.5): every block, its mode and its gestures
    VivariumHudFrame         hud_now_;          // the frame the HUD was last resolved and drawn with
    HorizonRidge             hud_ridge_;        // the two ridges of hills on the horizon, bound from the HUD's theme (N05)
    GroundSeason             hud_ground_season_; // the ground's colour by season, snow, depth and horizon, bound from the HUD's theme (N06)
    LeafSeason               hud_leaf_season_;  // the leaf's colour by season and drought, bound from the HUD's theme (B0278)
    BrookReflection          hud_brook_reflection_; // the sky the brook reflects and the strokes of its current, bound from the HUD's theme (N07)
    UiActivityArt            hud_activities_;   // every activity symbol of activity_art.json, read once (MIG_ASE_SKILL_RSM B7)
    UiAnchor                 hud_anchor_;      // the scene camera through the inverse of hud_frame_
    UiGround                 hud_ground_;       // the terrain port's ground on its lattice
    VivariumCarryBridge      hud_carry_;        // the mockup's inventory while flora.carry.v1 files no record (N0451, N0452)
    VivariumHandBridge       hud_hand_;         // the hand's readings while flora.patch.v1 previews no pour (N0454, N0463)
    VivariumLooseBridge      hud_loose_;        // the loosened soil point by point while flora.patch.v1 carries none (N0456)
    VivariumSignRules        hud_sign_rules_;   // the thresholds and values of the mockup's signs() while no port reports a sign (N0461, N0449)
    VivariumTrailBridge      hud_trail_;        // the mockup's S.log while flora.trail.v1 files no event (N0450)
    VivariumArcBridge        hud_arc_;          // the sky arc's mockup shares and the pair of arc and set that agreed (N0448)
    StowAnswer               hud_stow_;         // the bridge's verdict for the drag's place while flora.stow.v1 judges none
    std::vector<VivariumInstanceView> hud_segments_;  // the internodes of the plant in focus this frame
    std::unordered_map<std::string, VivariumHudVerb> hud_verbs_;  // hud.verbs: what each ground tool does
    uint8_t                  hud_stage_seen_ = GODOT_STAGE_NONE;  // the patch stage of the last frame: a rise is NOVO ESTÁGIO (m.html 1033)
    BrookSight               hud_brook_;        // the brook's stations as the eye last saw them and the flank last told (P0692, B0285)
    std::string              hud_verb_seen_;    // the HUD's verb of the last frame: a change drops the bed's start (S.bedA=null, m.html 947)
    std::string              hud_mode_seen_;    // the HUD's mode of the last frame: setMode drops the bed's start too (m.html 933)
    ::godot::Transform2D     hud_frame_;        // mockup units onto the canvas: the middle band, logical over mockup width
    double                   hud_unit_ = 0.0;   // logical pixels per mockup unit (logical_size.width / hud_frame_width)
    double                   hud_clock_ = 0.0;  // seconds the HUD's clock has run: the real time of the frames
    double                   hud_dt_ = 0.0;     // seconds of the last frame, at most hud_frame_dt_max (UiRefresh::step)
    UiRefresh                hud_refresh_;      // hud_refresh_ms: when the HUD samples the simulation again (T10, m.html 1037)
    UiFrameRate              frame_rate_;       // the frames per second of every frame against the floor of view.frame_budget_ms
    VivariumHudSample        hud_shown_;        // what the HUD read at its last sample (godot_vivarium_hud_sample.hpp)
    bool                     hud_still_ = false;        // every HUD motion held (read_hud_still)
    bool                     hud_still_read_ = false;   // the platform was asked once
    int32_t                  hud_still_platform_ = -1;  // its last answer: 1 still, 0 moving, -1 cannot say
    double                   hud_cloud_run_ = 0.0;  // seconds the cloud banks have drifted, each weighed by the wind's pace across the picture (cloud_drift_share, D2293)
    std::string              hud_mode_;         // the mode the HUD was in last frame
    double                   hud_mode_since_ = 0.0;  // hud_clock_ when that mode opened
    bool                     hud_down_ = false; // a finger is down on the HUD
    ::godot::Vector2         hud_finger_;       // where it is, canvas units
    std::vector<std::string> hud_gaps_;         // the gaps already logged, each as id/request or id/port field
    VivariumClockView        hud_bridge_clock_; // hud_bridge_clock_* of SHA: the mockup's date, day, phases and seasons (N0460)
    const char*              hud_bridge_weather_ = nullptr;   // hud_bridge_weather_kind: the mockup's sky, a GODOT_HUD_WEATHER_* key (N0449)
    uint32_t                 hud_bridge_level_ = 0;           // hud.perception.start, or the level modes.perception names on the launch (N0453)
    double                   hud_bridge_temperature_ = 0.0;   // hud_bridge_weather_temperature
    double                   hud_bridge_wind_ = 0.0;          // hud_bridge_weather_wind
    std::vector<ForecastPhase> hud_bridge_forecast_;          // hud_bridge_forecast_<n>_*: the coming phases in their order
    double                   hud_bridge_growth_ = 0.0;        // hud_bridge_patch_growth_share: the mockup's way to the next stage (N0454)
    double                   hud_bridge_grow_per_hour_ = 0.0; // hud_bridge_patch_grow_per_hour: the mockup's growth an hour, a share
    double                   hud_bridge_loose_ = 0.0;         // hud_bridge_patch_loose_share: the mockup's loosened soil (N0456)
    bool                     hud_bridge_sap_running_ = false; // hud_bridge_patch_sap_running: the mockup's tap (N0457)
    double                   hud_bridge_pace_factor_ = 0.0;   // hud_bridge_pace_factor: the mockup's S.factor, the clock's pace (N0459)
    SurvivalBodyFrame        hud_bridge_body_;                // hud_bridge_survival_*: the mockup's body for the rail CORPO (N0530)
    SurvivalFireFrame        hud_bridge_fire_;                // hud_bridge_survival_fire_*: the mockup's camp fire for the card FOGUEIRA (B0657)
    SurvivalAirFrame         hud_bridge_air_;                 // hud_bridge_survival_air_toxic: the mockup's poisoned air for the air mark (N0530)
    SurvivalSpoilFrame       hud_bridge_food_;                // hud_bridge_survival_food_*: the mockup's food of the bag for the spoil ring (N0530)
    SurvivalRestFrame        hud_bridge_rest_;                // hud_bridge_survival_rest_*: the mockup's resting figure for the rest card (N0530)
    double                   hud_bridge_sap_l_ = 0.0;         // hud_bridge_patch_sap_l: litres in the vessel
    double                   hud_bridge_sap_cap_l_ = 0.0;     // hud_bridge_patch_sap_cap_l: litres the vessel holds
    double                   hud_bridge_next_share_ = 0.0;    // hud_bridge_patch_next_share: the share of the year of the next stage's day (N0454)
    double                   hud_bridge_soil_full_ = 0.0;     // hud_bridge_mark_soil_full_rel: the soil share from which water has no room
    bool                     hud_bridge_branch_uncut_ = false; // hud_bridge_mark_branch_uncut: the mockup's crown has an uncut candidate
    std::string              hud_places_;       // the debug places last logged, every line joined: a change logs them anew
    std::string              hud_surface_;      // the debug surface line last logged (safe area, cut-out): a change logs it anew
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE POSE
 *
 * @file        godot_figure_pose.hpp
 * @brief       The pose of a figure at the camp fire from what ase-metabolism wrote: stand, sit on the ground or a stone, sleep
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.11: the figure file carries the clips of the camp fire
 *              (villager_body.json build.clips, scripts/figure.py:clip_pose), and the figure scene drives its mix
 *              toward one held pose. Which one is said by the owner of the rest, never by the adapter:
 *              ase-metabolism writes MTB_IS_RESTING, MTB_IS_SLEEPING and MTB_REST_QUALITY per figure
 *              (MetabolismRstSystem, MetabolismHubDrvPubSystem), and survival.rest.v1 lays them for the host
 *              together with the rest quality the body recovers toward, MTB_CONST_REST_QUALITY_TARGET
 *              (SurvivalHostRstExptSystem).
 *
 *              NO FIGURE SLEEPS STANDING. A record that says the figure sleeps gives the sleep, whatever it says of
 *              the rest; a figure that rests awake sits; one that does not rest, or whose record does not say,
 *              stands. The seat is the ring stone that lies within the radius of the body - the capsule ase-physics
 *              collides the figure as - else the ground.
 *
 *              THE SIDE OF THE SLEEP is chosen from the rest quality, as the plan names it (the figure chooses its
 *              pose from MTB_IS_RESTING and MTB_REST_QUALITY): a sleeping figure whose rest quality stands below the
 *              target its owner recovers it toward has not reached good rest and lies on its side; one at or above
 *              it lies on its back. A record that carries neither of the two gives the back, the sleep lie_down
 *              ends in. No Godot type stands here, so the choice is tested without a Godot process
 *              (tests/test_godot_figure_pose.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:2157 - go to Sleep
 * @actor       Sleeping Phases (SLEEPPHASES)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** The held pose a figure is driven toward; the passages between them are played by the mix (godot_figure_rig.hpp). */
constexpr uint8_t FIGURE_POSE_STAND = 0u;       // the gait mix: idle, walk, run, turn, bend
constexpr uint8_t FIGURE_POSE_SIT_GROUND = 1u;  // sit_ground, reached by sit_down
constexpr uint8_t FIGURE_POSE_SIT_STONE = 2u;   // sit_stone, reached by sit_down_stone
constexpr uint8_t FIGURE_POSE_SLEEP = 3u;       // sleep_back, reached from the ground seat by lie_down
constexpr uint8_t FIGURE_POSE_SLEEP_SIDE = 4u;  // sleep_side, rolled into from sleep_back

/**
 * Where one ring stone of the camp fire lies in the scene frame, as ase-physics holds its body on ingle.body.v1
 * (T.3); a resting figure over it sits on it.
 */
struct FigureSceneStone {
    double x = 0.0;  // metres east in the scene frame
    double z = 0.0;  // metres north in the scene frame
};

/**
 * What the rest record of one figure says, as survival.rest.v1 carries it; a key the record does not carry is NaN.
 */
struct FigureSceneRest {
    double is_resting = 0.0;    // MTB_IS_RESTING, 1 while the figure rests, else 0
    double sleeping = 0.0;      // MTB_IS_SLEEPING, 1 while the resting figure sleeps, else 0
    double rest_quality = 0.0;  // MTB_REST_QUALITY, 0 to 1
    double rest_target = 0.0;   // MTB_CONST_REST_QUALITY_TARGET, the rest quality of good rest
};

/**
 * The pose of one figure: `rest` its rest record (NaN for every key of a figure without one), `east` and `north`
 * its place in the scene frame, `radius` the radius of its body in metres, `stones` the ring stones of the camp.
 */
uint8_t figure_pose(const FigureSceneRest& rest, double east, double north, double radius,
                    const std::vector<FigureSceneStone>& stones);

}  // namespace ase::adp::godot

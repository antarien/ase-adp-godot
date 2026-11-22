/**
 * ASE ADAPTER TESTS - THE POSE OF A FIGURE AT THE CAMP FIRE WITHOUT GODOT
 *
 * @file        test_godot_figure_pose.cpp
 * @design      DSGN_030
 * @brief       No figure sleeps standing; a resting one sits on the stone under it or on the ground
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.11, acceptance "keine Figur schläft im Stehen": the pose the
 *              figure scene drives the mix toward is chosen from the rest record ase-metabolism's values fill
 *              (MTB_IS_RESTING, MTB_IS_SLEEPING, MTB_REST_QUALITY and MTB_CONST_REST_QUALITY_TARGET on
 *              survival.rest.v1) and the ring stones ase-physics holds (godot_figure_pose.hpp). Every combination
 *              of the two rest keys, said, unsaid and missing, is held against the rule that a sleeping figure lies,
 *              and the rest quality against good rest chooses the side or the back.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
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

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_figure_pose.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr double kYes = 1.0;             // MTB_IS_RESTING / MTB_IS_SLEEPING as ase-metabolism writes them while true
constexpr double kNo = 0.0;              // and while false
constexpr double kTarget = 0.8;          // MTB_CONST_REST_QUALITY_TARGET as hub_constants.json holds it
constexpr double kPoorRest = 0.5;        // a rest quality below the target
constexpr double kGoodRest = 0.9;        // and one above it
constexpr double kEast = 22.625;         // a place at the camp, metres east in the scene frame
constexpr double kNorth = 10.5;          // metres north
constexpr double kRadius = 0.19;         // radius of the body, metres, the order of the rig's half shoulder span
constexpr double kInside = 0.1;          // a stone this far east of the figure lies under its body
constexpr double kOutside = 0.6;         // a stone this far east of the figure lies beside it

double unsaid() { return std::nan(""); }

FigureSceneRest rest_of(double is_resting, double sleeping, double quality, double target) {
    FigureSceneRest rest;
    rest.is_resting = is_resting;
    rest.sleeping = sleeping;
    rest.rest_quality = quality;
    rest.rest_target = target;
    return rest;
}

bool lies(uint8_t pose) {
    return pose == FIGURE_POSE_SLEEP || pose == FIGURE_POSE_SLEEP_SIDE;
}

}  // namespace

TEST_CASE("usecase:2157 go to Sleep: a figure ase-metabolism holds asleep lies, whatever its record says of the rest") {
    const std::vector<FigureSceneStone> none;
    const std::vector<FigureSceneStone> under = {FigureSceneStone{kEast + kInside, kNorth}};
    for (const double resting : {kYes, kNo, unsaid()}) {
        for (const double quality : {kPoorRest, kGoodRest, unsaid()}) {
            CHECK(lies(figure_pose(rest_of(resting, kYes, quality, kTarget), kEast, kNorth, kRadius, none)));
            CHECK(lies(figure_pose(rest_of(resting, kYes, quality, kTarget), kEast, kNorth, kRadius, under)));
        }
    }
}

TEST_CASE("usecase:2157 go to Sleep: no awake or unsaid record gives the sleep, and no sleeping one a stand") {
    const std::vector<FigureSceneStone> none;
    for (const double resting : {kYes, kNo, unsaid()}) {
        for (const double sleeping : {kYes, kNo, unsaid()}) {
            const uint8_t pose = figure_pose(rest_of(resting, sleeping, kPoorRest, kTarget), kEast, kNorth, kRadius,
                                             none);
            const bool asleep = !std::isnan(sleeping) && sleeping != 0.0;
            const bool lying = lies(pose);
            const bool stands_asleep = pose == FIGURE_POSE_STAND && asleep;
            CHECK(lying == asleep);
            CHECK_FALSE(stands_asleep);
        }
    }
}

TEST_CASE("usecase:2157 go to Sleep: below good rest the figure sleeps on its side, at good rest on its back") {
    const std::vector<FigureSceneStone> none;
    CHECK(figure_pose(rest_of(kYes, kYes, kPoorRest, kTarget), kEast, kNorth, kRadius, none) ==
          FIGURE_POSE_SLEEP_SIDE);
    CHECK(figure_pose(rest_of(kYes, kYes, kGoodRest, kTarget), kEast, kNorth, kRadius, none) == FIGURE_POSE_SLEEP);
    CHECK(figure_pose(rest_of(kYes, kYes, kTarget, kTarget), kEast, kNorth, kRadius, none) == FIGURE_POSE_SLEEP);
    CHECK(figure_pose(rest_of(kYes, kYes, unsaid(), kTarget), kEast, kNorth, kRadius, none) == FIGURE_POSE_SLEEP);
    CHECK(figure_pose(rest_of(kYes, kYes, kPoorRest, unsaid()), kEast, kNorth, kRadius, none) == FIGURE_POSE_SLEEP);
}

TEST_CASE("usecase:2157 go to Sleep: a figure resting awake sits, on the stone under its body or on the ground") {
    const std::vector<FigureSceneStone> under = {FigureSceneStone{kEast + kInside, kNorth}};
    const std::vector<FigureSceneStone> beside = {FigureSceneStone{kEast + kOutside, kNorth}};
    CHECK(figure_pose(rest_of(kYes, kNo, kPoorRest, kTarget), kEast, kNorth, kRadius, under) ==
          FIGURE_POSE_SIT_STONE);
    CHECK(figure_pose(rest_of(kYes, kNo, kPoorRest, kTarget), kEast, kNorth, kRadius, beside) ==
          FIGURE_POSE_SIT_GROUND);
    CHECK(figure_pose(rest_of(kYes, unsaid(), kPoorRest, kTarget), kEast, kNorth, kRadius, beside) ==
          FIGURE_POSE_SIT_GROUND);
}

TEST_CASE("usecase:2157 go to Sleep: a figure that does not rest, or whose record says nothing, stands") {
    const std::vector<FigureSceneStone> under = {FigureSceneStone{kEast + kInside, kNorth}};
    CHECK(figure_pose(rest_of(kNo, kNo, kPoorRest, kTarget), kEast, kNorth, kRadius, under) == FIGURE_POSE_STAND);
    CHECK(figure_pose(rest_of(unsaid(), unsaid(), unsaid(), unsaid()), kEast, kNorth, kRadius, under) ==
          FIGURE_POSE_STAND);
    CHECK(figure_pose(rest_of(unsaid(), kNo, kPoorRest, kTarget), kEast, kNorth, kRadius, under) == FIGURE_POSE_STAND);
}

}  // namespace ase::adp::godot

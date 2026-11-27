/**
 * ASE ADAPTER TESTS - GODOT HOST CLOCK
 *
 * @file        test_godot_host_clock.cpp
 * @design      DSGN_021
 * @brief       Pins the clock of one game host against the real bundle: the return from the background as one filed
 *              real position, the time lapse as a pace of ase-time's clock, and the refusal of a broken frame
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG, the parts of L5 that do not need a window. The manager boots the
 *              SAME bundle Godot loads, with the configuration and the game host of test_godot_host_support.hpp.
 *              One purpose cut out of test_godot_host.cpp by what it reads: these cases read the clock record,
 *              the tick count and the pending seconds of the host, and none of them reads a patch, an organ or a state a
 *              case before it left, so they run on a game host of their own and never wait for a pour.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside test_godot_host.cpp, which boots the set,
 *              pours, sprouts and tears down on a game host of its own.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <cstdint>
#include <limits>
#include <string>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::environment;

TEST_CASE("L5 without a window: a long frame is one tick and one filed real position, laid by ase-time in one write") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    advance_frames(resources, 4u);
    const double game_before = resources.get_clock().elapsed_s;
    const uint64_t ticks_before = resources.tick_count();
    const double simulated_before = resources.simulation_seconds();

    // Ten seconds in the background come back as one frame: one tick, and the real position moves by the
    // whole span on flora.real.v1. Nothing is replayed; the seconds stand as pending until the clock
    // record shows the position laid (real_laid_s), which ase-time does at its Dissemination run of 1 Hz.
    REQUIRE(resources.advance(10.0) == ase::kernel::HostStatusOk);
    CHECK(resources.last_steps() == 1u);
    CHECK(resources.pending_seconds() > 0.0);
    // Every frame names the real position the clock record shows laid (real_laid_s, TIM_REAL_LAID_S through
    // ase-pl-flora's clock record) and the pending seconds after it: seconds left pending print the frame at which
    // ase-time laid the position and the frame the record carried it, the measured chain of the return.
    // THE BOUND IS THE CHAIN, NEVER A NUMBER OF FRAMES. ase-pl-flora takes the filed position in Reception
    // (FloraHostRealSystem), ase-time lays it at its next Dissemination run (1 Hz) unless its own Reception read
    // (TimeHubPaceSyncSystem) came first in that tick, then at the run after. ase-pl-flora takes TIM_REAL_LAID_S at
    // its Modulation run (2 Hz, FloraHubTimSyncSystem), files the clock record in the Production of the tick after,
    // and the port publishes it with its next set: the hops lie within one more Dissemination interval. So the
    // frames run until three Dissemination runs passed since the long frame: a late lay at the second, the hops
    // up to the third.
    uint64_t runs_before = 0u;
    REQUIRE(resources.schedule_runs("Dissemination", &runs_before) == ase::kernel::HostStatusOk);
    uint64_t runs = runs_before;
    std::string laid_by_frame = "laid before=" + std::to_string(resources.get_clock().real_laid_s);
    uint32_t frames = 0u;
    for (; runs - runs_before < 3u && resources.pending_seconds() > 0.0; ++frames) {
        REQUIRE(resources.advance(0.25) == ase::kernel::HostStatusOk);
        CHECK(resources.last_steps() == 1u);
        REQUIRE(resources.schedule_runs("Dissemination", &runs) == ase::kernel::HostStatusOk);
        laid_by_frame += " f" + std::to_string(frames) + "=" + std::to_string(resources.get_clock().real_laid_s) +
                         "/" + std::to_string(resources.pending_seconds()) + "@" + std::to_string(runs - runs_before);
    }
    INFO(laid_by_frame);
    CHECK(resources.pending_seconds() == 0.0);
    CHECK(resources.tick_count() - ticks_before == 1u + frames);  // one tick per frame, the long one too
    CHECK(resources.simulation_seconds() - simulated_before == doctest::Approx(10.0 + 0.25 * frames));
    CHECK(resources.get_clock().elapsed_s > game_before);

    // A broken frame is refused by the host, the host keeps running and nothing is counted.
    const uint64_t ticks = resources.tick_count();
    CHECK(resources.advance(std::numeric_limits<double>::quiet_NaN()) == ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.advance(std::numeric_limits<double>::infinity()) == ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.advance(-0.25) == ase::kernel::HostStatusInvalidArgument);
    CHECK(resources.tick_count() == ticks);
    CHECK(resources.running());
    CHECK(std::string(resources.failure_step()).empty());
}

TEST_CASE("L5 without a window: the time lapse is a pace of ase-time's clock, one tick per frame, faster than real time") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    advance_frames(resources, 4u);

    // The game seconds of eight frames at the default pace.
    const double base_from = resources.get_clock().elapsed_s;
    advance_frames(resources, 8u);
    const double base_span = resources.get_clock().elapsed_s - base_from;
    CHECK(base_span > 0.0);

    // The lapse is a factor on the pace port: ase-pl-flora multiplies it with ase-time's base pace and
    // ase-time runs this host's clock at the product. Every frame stays one tick; no tick is added.
    REQUIRE(resources.pace(10.0) == ase::kernel::HostStatusOk);
    advance_frames(resources, 8u);
    const uint64_t ticks_before = resources.tick_count();
    const double fast_from = resources.get_clock().elapsed_s;
    advance_frames(resources, 8u);
    CHECK(resources.tick_count() - ticks_before == 8u);
    CHECK(resources.last_steps() == 1u);
    CHECK(resources.pending_seconds() == 0.0);
    CHECK(resources.get_clock().elapsed_s - fast_from > base_span * 2.0);

    // The default pace back.
    REQUIRE(resources.pace(GODOT_PACE_NONE) == ase::kernel::HostStatusOk);
    advance_frames(resources, 4u);
    CHECK(resources.running());
    CHECK(std::string(resources.failure_step()).empty());
}

}  // namespace ase::adp::godot

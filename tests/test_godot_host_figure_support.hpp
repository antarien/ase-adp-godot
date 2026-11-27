#pragma once

/**
 * ASE ADAPTER TESTS - GODOT HOST FIGURE SUPPORT
 *
 * @file        test_godot_host_figure_support.hpp
 * @design      DSGN_030
 * @brief       The figure ports of the start configuration, the game host booted with them, and the waits on the figure
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE: two executables drive the figure ports of ase-pl-avatar,
 *              each on the one game host of its run - test_godot_host_figure.cpp the body, the gear and the
 *              motion of the figure, test_godot_host_camera.cpp the cameras that look at it (cam.mode,
 *              cam.drag, cam.zoom, the rim of the area). Both configure the figure ports exactly as the
 *              client's start configuration (config/vivarium_start.json, "ports" figure_*) names them, both
 *              ask for the figure of the seat at the same spawn place, and both wait for the plugin's answer
 *              the same bounded number of frames; these functions are that one shared part.
 *
 *              THE WAITS ARE BOUNDED, NEVER OPEN: a frame of the game host is a quarter second, and the
 *              plugin answers a request within kFigureBoundFrames of them. A wait that runs out returns what
 *              the port carries then, so a case fails on its CHECK instead of hanging the run.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101588 - move player figure over ground
 * @actor       SS/PC
 * @created     2026-11-18
 * @modified    2026-11-18
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/host/types.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot::test {

/** The seat and spawn place of the client's start configuration ("figure"): the planting place by the birch. */
inline constexpr uint32_t kFigureSeat = 1u;
inline constexpr double kFigureSpawnX = 16.5;
inline constexpr double kFigureSpawnZ = 13.6;
/** Frames the plugin gets to answer a request, a quarter second each. */
inline constexpr uint32_t kFigureBoundFrames = 32u;
/**
 * The skeleton of the figure as the scene measures it, metres. The Godot host measures it on every figure it
 * spawns (rig port); until then ase-physics gives the walking body no capsule, and without a capsule
 * PhysicsWalkBndySystem never holds it at a chunk border.
 */
inline constexpr double kFigureEyeHeight = 1.62;
inline constexpr double kFigureHeight = 1.78;
inline constexpr double kFigureRadius = 0.19;  // half the span of the shoulder joints of villager_body (L/R_Upperarm, glb_bones.out)

/** The figure ports and operations as the client's start configuration names them. */
inline void add_figure_ports(VivariumStartConfig& config) {
    config.figure_body_port = "avatar.body.v1";
    config.figure_camera_port = "avatar.camera.v1";
    config.figure_spawn_port = "avatar.spawn.v1";
    config.figure_stick_port = "avatar.stick.v1";
    config.figure_orbit_port = "avatar.orbit.v1";
    config.figure_switch_port = "avatar.switch.v1";
    config.figure_release_port = "avatar.release.v1";
    config.figure_seize_port = "avatar.seize.v1";
    config.figure_hold_port = "avatar.hold.v1";
    config.figure_drop_port = "avatar.drop.v1";
    config.figure_don_port = "avatar.don.v1";  // N0590: the pack on the back
    config.figure_fill_port = "avatar.fill.v1";
    config.figure_drain_port = "avatar.drain.v1";
    config.figure_gear_port = "avatar.gear.v1";
    config.figure_rig_port = "avatar.rig.v1";
    config.figure_turn_port = "avatar.turn.v1";
    config.figure_area_port = "avatar.area.v1";
    config.figure_code_port = "avatar.code.v1";
    config.figure_boundary_port = "avatar.boundary.v1";  // 07.13: the bumps against the chunk edge
    config.skill_port = "avatar.skill.v1";               // MIG_ASE_SKILL_RSM B3: the learning entries of each figure
    config.skill_qnt_port = "avatar.skill.qnt.v1";       // the running time slice of each entry
    config.skill_bar_port = "avatar.skill.bar.v1";       // every booked time slice
    config.skill_prg_port = "avatar.skill.prg.v1";       // the progress, rate, measured time and work of each entry
    config.op_spawn = "spawn";
    config.op_steer = "steer";
    config.op_turn = "turn";
    config.op_switch = "switch";
    config.op_release = "release";
    config.op_seize = "seize";
    config.op_hold = "hold";
    config.op_drop = "drop";
    config.op_don = "don";
    config.op_fill = "fill";
    config.op_drain = "drain";
    config.op_measure = "measure";
    config.op_bind = "bind";
    // The day and the four seasons as the start configuration names them (N0460): hours_to_summer reads the day of the
    // year from the day record and the first LUNBREX of IGNIX from the season set (T0415).
    config.day_port = "flora.day.v1";
    config.season_port = "flora.season.v1";
    config.figure_seat = kFigureSeat;
    config.figure_x = kFigureSpawnX;
    config.figure_z = kFigureSpawnZ;
}

/** The environment of this run, booted with the figure ports. */
inline AdapterEnvironment& figure_environment() {
    return environment(&add_figure_ports);
}

/** The figure of the seat once the body port carries it; frames advance until then, a bounded number. */
inline const VivariumFigureView* the_figure(GodotHostResourceManager& resources) {
    for (uint32_t frame = 0u; frame < kFigureBoundFrames && resources.get_figures().empty(); ++frame) {
        advance_frames(resources, 1u);
    }
    return resources.get_figures().empty() ? nullptr : &resources.get_figures().front();
}

/** The host's lines of the next frames, until one carries `text` or the bound runs out. */
inline bool frames_carry(GodotHostResourceManager& resources, const std::string& text) {
    std::vector<std::string> lines;
    for (uint32_t frame = 0u; frame < kFigureBoundFrames; ++frame) {
        advance_frames(resources, 1u);
        (void)resources.remove_log_lines(lines);
        if (any_line_carries(lines, text)) {
            return true;
        }
    }
    return false;
}

/** Frames a wait on the grown shoot gets, a quarter second each. */
inline constexpr uint32_t kShootBoundFrames = 256u;
/** Frames the organ port holds its internode count before the shoot counts as standing: more than one Dissemination run. */
inline constexpr uint32_t kShootStillFrames = 16u;
/**
 * Game days the shoot lives after the jump to MADURA: the buds of the jump burst in the cycles of the days after it,
 * as the birch of modules/ase-phytomer/tests/test_phytomer.cpp bears its main branches only in the two days after its
 * jump (B0299). Each day is one seek of hours_per_day ahead.
 */
inline constexpr uint32_t kDaysAfterJump = 2u;

/**
 * Game hours from now to the first day of IGNIX, the summer, as the calendar ports carry it (CAL_SEASON on the date
 * record, the first LUNBREX of each season on flora.season.v1): 0 while the clock stands in IGNIX, a negative value
 * while a port has not carried the calendar yet. The leaf season runs from PLT_SPEC_FLW_LEAF_FLUSH to
 * PLT_SPEC_FLW_LEAF_FALL, and the birch of modules/ase-phytomer/tests/test_phytomer.cpp bears its main branches in
 * summer days (B0299); a jump to MADURA that lands in autumn reaches them only after the winter (register T0415).
 */
inline double hours_to_summer(const VivariumClockView& clock) {
    const auto summer = static_cast<std::size_t>(clock.season_ignix);
    if (!clock.date_known || !clock.day_known || !clock.season_codes_known || !clock.seasons_known ||
        clock.days_per_month <= 0.0 ||
        clock.months_per_year <= 0.0 || clock.hours_per_day <= 0.0 || summer >= clock.season_start_lun.size()) {
        return -1.0;
    }
    if (clock.season == clock.season_ignix) {
        return 0.0;
    }
    const double days_per_year = clock.months_per_year * clock.days_per_month;
    const double first_day = (clock.season_start_lun[summer] - 1.0) * clock.days_per_month + 1.0;
    double ahead_d = first_day - clock.year_day;
    if (ahead_d <= 0.0) {
        ahead_d += days_per_year;
    }
    return ahead_d * clock.hours_per_day;
}

/** Frames until the clock of the host stands at game hour `hour` or later, a bounded number; returns the frames taken. */
inline uint32_t await_hour(GodotHostResourceManager& resources, double hour) {
    uint32_t frames = 0u;
    for (; frames < kShootBoundFrames; ++frames) {
        advance_frames(resources, 1u);
        const VivariumClockView& clock = resources.get_clock();
        if (clock.seconds_per_hour > 0.0 && clock.elapsed_s / clock.seconds_per_hour >= hour) {
            break;
        }
    }
    return frames;
}

/**
 * Frames until the organ port held its internode count kShootStillFrames frames, a bounded number; returns the frames
 * it held. Every change of the internode and leaf count lands in `steps` as frame:internodes/leaves/o<top order>
 * (B0988): a top order of 0 at every step says no bud burst, a later 1 that the buds burst in the cycles after it.
 */
inline uint32_t await_still_shoot(GodotHostResourceManager& resources, std::string& steps) {
    std::size_t organ_size = 0u;
    std::size_t leaf_size = 0u;
    uint32_t still = 0u;
    for (uint32_t frame = 0u; frame < kShootBoundFrames && still < kShootStillFrames; ++frame) {
        advance_frames(resources, 1u);
        const std::size_t size = resources.get_organs(GODOT_ORGAN_SEGMENT).size();
        const std::size_t leaves = resources.get_organs(GODOT_ORGAN_LEAF).size();
        if (size != organ_size || leaves != leaf_size) {
            double top_order = 0.0;
            for (const VivariumOrganView& organ : resources.get_organs(GODOT_ORGAN_SEGMENT)) {
                top_order = organ.order > top_order ? organ.order : top_order;
            }
            steps += std::to_string(frame) + ":" + std::to_string(size) + "/" + std::to_string(leaves) + "/o" +
                     std::to_string(static_cast<uint32_t>(top_order)) + " ";
        }
        still = size == organ_size && size > 0u ? still + 1u : 0u;
        organ_size = size;
        leaf_size = leaves;
    }
    return still;
}

/** The figure `object_id` as the body port carries it now, nullptr when it carries no such figure. */
inline const VivariumFigureView* figure_of(GodotHostResourceManager& resources, uint32_t object_id) {
    for (const VivariumFigureView& figure : resources.get_figures()) {
        if (figure.object_id == object_id) {
            return &figure;
        }
    }
    return nullptr;
}

}  // namespace ase::adp::godot::test

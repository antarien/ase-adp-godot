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
    config.figure_fill_port = "avatar.fill.v1";
    config.figure_drain_port = "avatar.drain.v1";
    config.figure_gear_port = "avatar.gear.v1";
    config.figure_rig_port = "avatar.rig.v1";
    config.figure_turn_port = "avatar.turn.v1";
    config.figure_area_port = "avatar.area.v1";
    config.figure_code_port = "avatar.code.v1";
    config.figure_boundary_port = "avatar.boundary.v1";  // 07.13: the bumps against the chunk edge
    config.op_spawn = "spawn";
    config.op_steer = "steer";
    config.op_turn = "turn";
    config.op_switch = "switch";
    config.op_release = "release";
    config.op_seize = "seize";
    config.op_hold = "hold";
    config.op_drop = "drop";
    config.op_fill = "fill";
    config.op_drain = "drain";
    config.op_measure = "measure";
    config.op_bind = "bind";
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

/**
 * ASE ADAPTER TESTS - GODOT HOST SKILL PORTS
 *
 * @file        test_godot_host_skill.cpp
 * @design      DSGN_074
 * @brief       Pins what the skill ports of one game host carry: the learning entry of a working figure and its progress
 * @description MIG_ASE_SKILL_RSM B3 against the real bundle, with the configuration of test_godot_host_support.hpp
 *              and the figure and skill ports of the client's start configuration (test_godot_host_figure_support.hpp):
 *              a dig of the led figure is booked as its work, ase-skill keeps a learning entry for it, and
 *              ase-pl-avatar lays that entry on avatar.skill.v1 and its four measurements on avatar.skill.prg.v1 -
 *              the progress toward the next plateau (0 to 100), the smoothed rate of experience, the seconds the
 *              entry has been measured and the units of work booked while the session runs. read_skill_prgs joins
 *              the four to the entry by its id; the RSM table of the debug layer shows them in the rows PRG, VEL,
 *              MSR and VOL.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside those of test_godot_host_figure.cpp and
 *              test_godot_host_camera.cpp, with the figure ports added to the shared configuration.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101305 - show skill progress
 * @actor       SS/PC
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_figure_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::figure_environment;
using test::kFigureBoundFrames;
using test::the_figure;

namespace {

/** Half the side of the dig stroke, metres: the stroke of usecase:100411 the figure test books as work. */
constexpr double kStrokeHalfM = 0.5;
/** Depth of the dig stroke, metres. */
constexpr double kStrokeDepthM = 0.1;
/** Metres beside the figure the stroke lands, east and north. */
constexpr double kStrokeOffsetM = 1.0;
/** The top of the progress toward the next plateau, HUB_SKL_RSM_PRG: progress_percent runs 0 to 100. */
constexpr double kPercentMax = 100.0;

}  // namespace

TEST_CASE("usecase:101305 show skill progress: the host offers the skill ports of the start configuration") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    // RED when: ase-pl-avatar offers no avatar.skill.prg.v1 - the rows PRG VEL MSR VOL of the RSM table stay a dash.
    CHECK(resources.skill_offered());
    CHECK(resources.skill_qnt_offered());
    CHECK(resources.skill_bar_offered());
    CHECK(resources.skill_prg_offered());
}

TEST_CASE("usecase:101305 show skill progress: the learning entry of a digging figure carries its progress, rate, measured time and work") {
    AdapterEnvironment& env = figure_environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;
    REQUIRE(resources.skill_prg_offered());
    const VivariumFigureView* first = the_figure(resources);
    REQUIRE(first != nullptr);
    const uint32_t figure = first->object_id;
    CHECK(resources.lower(first->x + kStrokeOffsetM, first->z + kStrokeOffsetM, kStrokeHalfM, kStrokeDepthM, figure) ==
          ase::kernel::HostStatusOk);

    // The skill and progress ports are laid in every Regulation pass: frames advance until the figure's entry stands
    // with its four measurements, a bounded number.
    bool found = false;
    VivariumSkillView entry;
    for (uint32_t frame = 0u; frame < kFigureBoundFrames && !found; ++frame) {
        advance_frames(resources, 1u);
        for (const VivariumSkillView& skill : resources.get_skills()) {
            if (skill.bearer == static_cast<double>(figure) && std::isfinite(skill.prg)) {
                entry = skill;
                found = true;
                break;
            }
        }
    }
    // RED when: the four values never join the entry by its id - read_skill_prgs drops the record or the host never
    // lays it, and the table shows no progress for a figure that works.
    REQUIRE(found);
    CHECK(entry.prg >= 0.0);
    CHECK(entry.prg <= kPercentMax);
    CHECK(std::isfinite(entry.vel));
    CHECK(entry.msr >= 0.0);
    CHECK(entry.vol >= 0.0);
}

}  // namespace ase::adp::godot

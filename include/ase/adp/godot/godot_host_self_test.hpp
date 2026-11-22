#pragma once

/**
 * ASE GODOT ADAPTER - HOST SELF TEST
 *
 * @file        godot_host_self_test.hpp
 * @brief       What one self-test run of the embedded host counted, and the state one scenario carries
 * @description The counts GodotHostResourceManager::self_test() returns and the state it carries from action to
 *              action of one scenario. One purpose of the dissolved godot_host_resource_manager.hpp
 *              (WRFL_ASE_TIER_WIPE, the header ratchet): only the self-test and the units that show its result
 *              read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/kernel/kernel_host_port.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * What one self-test run counted. A scenario that could not run to its end (no host, a unit not
 * loadable, a broken table entry) counts as one error of its own - a self-test that did not run
 * is never a self-test without errors.
 */
struct VivariumSelfTestResult {
    uint32_t cases = 0;    // scenarios run to their end, in order on the one test host
    uint32_t checks = 0;   // comparisons made: one per expected value, status and record count
    uint32_t errors = 0;   // failed comparisons, runs no action checked, and scenarios that could not run
};

/**
 * The state one scenario of the self-test carries from action to action: the values it
 * remembered for a later `plus`, the snapshot buffer, and the runs it checked.
 */
struct VivariumSelfTestState {
    std::unordered_map<std::string, double>     remembered;
    std::vector<ase::kernel::KernelHostRecord>  records;
    std::vector<std::string>                    checked;
    double                                      tolerance = 0.0;
    double                                      frame_s = 0.0;
    double                                      real_s = 0.0;  // real position the test host was last filed, carried from scenario to scenario on the one host
};

}  // namespace ase::adp::godot

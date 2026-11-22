#pragma once

/**
 * ASE GODOT ADAPTER - HOST BUNDLE INFO
 *
 * @file        godot_host_bundle_info.hpp
 * @brief       What the bundle index of the embedded host names: the units, the data root and the data files
 * @description The bundle GodotHostResourceManager::boot() read from the stage, every unit with its library,
 *              manifest and versions. One purpose of the dissolved godot_host_resource_manager.hpp
 *              (WRFL_ASE_TIER_WIPE, the header ratchet): only the boot path and the units that show or check
 *              the bundle read it.
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

#include <string>
#include <vector>

namespace ase::adp::godot {

/** One unit as the bundle index names it. */
struct VivariumBundleUnit {
    std::string unit;         // unit name (ase-hub, ase-pl-flora, ...)
    std::string library;      // file name of its library
    std::string manifest;     // file name of its staged manifest
    std::string version;      // its version
    std::string api_version;  // the ASE API it was built against
};

/** What the bundle index says: the units, the data root and the data files below it. */
struct VivariumBundleInfo {
    std::string                     data_root;  // directory of the data files, relative to the stage
    std::vector<VivariumBundleUnit> units;      // every unit, in the index's order
    std::vector<std::string>        data;       // every data file, relative to the data root
};

}  // namespace ase::adp::godot

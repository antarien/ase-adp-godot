/**
 * ASE GODOT ADAPTER - THE SCALE AND THE NAMES OF A FRUIT BODY
 *
 * @file        godot_cap_scale.cpp
 * @brief       The scale that gives a drawn fruit body the cap radius of its individual, and the role and node of a species
 * @description DSGN_018 ORGXFORM, usecase:101362 scale the organ templates, see godot_cap_scale.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101362 - scale the organ templates
 * @actor       Organ Transform Shaper (ORGATRASHA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/cap/godot_cap_scale.hpp>
#include <ase/adp/godot/cap/types.hpp>
#include <ase/math/math.hpp>

namespace ase::adp::godot {

float cap_scale(float radius_m, float width_x, float width_z) {
    const float reach = 0.5f * ase::math::max(width_x, width_z);
    if (!(reach > 0.0f) || !(radius_m > 0.0f)) {
        return 0.0f;
    }
    return radius_m / reach;
}

std::string cap_role_of(const std::string& species) {
    return std::string(GODOT_ROLE_CAP) + species;
}

std::string cap_species_of(const std::string& role) {
    const std::string prefix = GODOT_ROLE_CAP;
    if (role.size() <= prefix.size() || role.compare(0, prefix.size(), prefix) != 0) {
        return std::string();
    }
    return role.substr(prefix.size());
}

std::string cap_node_of(const std::string& species) {
    return std::string(GODOT_PLANT_CAP_KIND) + species;
}

}  // namespace ase::adp::godot

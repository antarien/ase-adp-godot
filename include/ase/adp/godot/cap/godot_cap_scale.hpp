#pragma once

/**
 * ASE GODOT ADAPTER - THE SCALE AND THE NAMES OF A FRUIT BODY
 *
 * @file        godot_cap_scale.hpp
 * @brief       The scale that gives a drawn fruit body the cap radius of its individual, and the role and node of a species
 * @description DSGN_018 ORGXFORM, usecase:101362 scale the organ templates: the part of the cap drawing that carries
 *              no Godot type, so the pure tests of the adapter pin it. cap_scale reads the radius ase-phytomer passed
 *              on for the individual (PHY_CAP_RADIUS, flora.cap.v1) against the horizontal extent of the mesh the
 *              view draws; cap_role_of names the template role of a species (GODOT_ROLE_CAP before its catalog id,
 *              fungus.amanita_phalloides), cap_species_of reads the species back off a role, and cap_node_of names
 *              the child of the caps node that holds the template of that species (GODOT_PLANT_CAP_KIND).
 *
 *              WHY A HEADER OF ITS OWN: godot_plant_cap.hpp carries Godot types and links no pure test; this header
 *              compiles into ase-adp-godot-host, the library the pure tests link.
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

#include <string>

namespace ase::adp::godot {

/**
 * The uniform scale that gives a mesh whose horizontal extent is `width_x` by `width_z` a widest reach from its
 * axis of `radius_m`: radius over half the wider side. A mesh without a horizontal extent, or a radius that is not
 * positive, has no scale (0) and is not drawn.
 */
float cap_scale(float radius_m, float width_x, float width_z);

/** The template role of the fruit body of `species`: GODOT_ROLE_CAP before its catalog id. */
std::string cap_role_of(const std::string& species);

/** The species a cap role names, the part after GODOT_ROLE_CAP; empty for a role that is no cap role or names none. */
std::string cap_species_of(const std::string& role);

/** The name of the child of the caps node that holds the template of `species`: GODOT_PLANT_CAP_KIND before its id. */
std::string cap_node_of(const std::string& species);

}  // namespace ase::adp::godot

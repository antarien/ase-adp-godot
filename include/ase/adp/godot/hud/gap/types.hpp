#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the register entries of the backend gaps the care view's bridge marks
 * @description While a port does not deliver a value, the bridge writes the mockup's value from SHA into the
 *              frame and the picture stays whole; VivariumView::open_gap logs the gap once on its edge with its
 *              register entry, never in the picture (PLAN_ASE_VIVARIUM_PHASE_06_HUD bridge section, carrier
 *              N0448). The register is the one list; these name its entries.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. This purpose changes whenever a gap opens or closes, and only
 *              the bridge units read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

namespace ase::adp::godot {

constexpr const char* GODOT_HUD_GAP_CARRY = "N0451";                  // flora.carry.v1: things, place, containers, load (ase-inventory)
constexpr const char* GODOT_HUD_GAP_STOW = "N0452";                   // flora.stow.v1: moving a thing with its refusal code
constexpr const char* GODOT_HUD_GAP_WEATHER = "N0449";                // sky.weather.v1: the weather kind, wind and the forecast (ase-atmosphere, ase-pl-sky)
constexpr const char* GODOT_HUD_GAP_PATCH = "N0454";                  // flora.patch.v1: soil water and the preview after watering
constexpr const char* GODOT_HUD_GAP_LOOSE = "N0456";                  // the loosened share of the soil after the spade (loosen the soil)
constexpr const char* GODOT_HUD_GAP_SAP = "N0457";                    // birch sap: a running tap and the fill of its vessel
constexpr const char* GODOT_HUD_GAP_PACE = "N0459";                   // flora.pace.v1: the factor ase-time runs this host's clock at
constexpr const char* GODOT_HUD_GAP_ROOT = "N0463";                   // flora.organ.root.v1: the root segments of the patch's body under the trunk
constexpr const char* GODOT_HUD_GAP_CLOCK = "N0460";                  // flora.clock.v1: season, light, phase starts, shares of the day and the year (ase-calendar, ase-time)
constexpr const char* GODOT_HUD_GAP_TRAIL = "N0450";                  // flora.trail.v1: the past events with their share of the year and domain
constexpr const char* GODOT_HUD_GAP_SIGN = "N0461";                   // the state fields of each sign case on the patch set (wilt, dead, flush, shed, rest, catkin, sap, brook)
constexpr const char* GODOT_HUD_GAP_PRUNE = "N0462";                  // flora.scene.v1: the internodes that may be cut
constexpr const char* GODOT_HUD_GAP_SALT = "D2666";                   // hydrogeology.salt.v1: the world total of the salt, a GLOBAL value of ase-gis beside GIS_SLNT_CHK_KG

}  // namespace ase::adp::godot

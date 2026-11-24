#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the lineage of the organs the view reads from the lineage port
 * @description All compile-time constants of the lineage sub-type; NO runtime values - those belong in the views.
 *              PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.9, the round trip S0, S1, S0: the lineage ase-phytomer gave an
 *              organ is the one name of it that a release and a new resolution of its tree keep, while the entity
 *              of the organ, and with it the object id on every organ port, is a new one after the workshop.
 *              ase-pl-flora files one record per shown organ on its lineage port (flora.lineage.v1), its object id
 *              the organ; the view writes it into the organ's view and computes nothing of it.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter port/types.hpp is included by more than a hundred own
 *              units of the avt build and takes no new line (header ratchet, operator decision 2026-11-21); these
 *              values are read only by the organ view, the lineage reader and the host test of the round trip, so
 *              they stand in the narrowest header those include.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:101330 - watch the growing plant body
 * @actor       Plant Organ Telemetry Watcher (ORGNWATCH)
 * @created     2026-11-23
 * @modified    2026-11-23
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

/**
 * ABBREVIATIONS: none - the lineage sub-type names no file part of its own.
 */

constexpr const char* GODOT_KEY_LINEAGE = "lineage";   // lineage record: the stable name of the organ's seeded draws, a whole number below 2^32
constexpr double GODOT_LINEAGE_NONE = -1.0;            // lineage of an organ without a lineage record: below every value a uint32_t carries
constexpr double GODOT_LINEAGE_MAX = 4294967295.0;     // largest lineage a record may carry: UINT32_MAX, exact in a double

}  // namespace ase::adp::godot

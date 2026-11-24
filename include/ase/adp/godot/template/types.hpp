#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the record keys and template codes of the organ template ports the adapter reads
 * @description The organ templates (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2, usecase:101011 tessellate the
 *              organ templates): ase-render-data builds the fine and the coarse cylinder, the fine 3 x 3 unit leaf,
 *              the coarse 1 x 1 unit leaf (detail stage 1, TREEGEN T.6) and the unit fruit body on the unit, ase-pl-flora lays their corners on one port, their triangles on another and
 *              the tangent of every corner on a third (flora.template.tangent.v1, PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN
 *              T.6), each record with the code of its template. The codes and keys are the plugin's (ase-pl-flora
 *              types.hpp FLORA_HOST_TMPL_*, types/template/types.hpp FLRA_HOST_TMPL_FNGS and
 *              FLRA_HOST_TMPL_LEAF_CORS, FLORA_HOST_KEY_*),
 *              repeated here because the adapter reads the port and never links the plugin. A corner record
 *              carries the code, its index and x, y, z, nx, ny, nz, u, v; a triangle record the code and three
 *              corner indices, clockwise seen from outside; a tangent record the code, the index and the tangent.
 *              GodotHostResourceManager::read_templates joins every tangent record to the corner of the same
 *              template and index; build_template hands it to ARRAY_TANGENT, the bark shader turns it with its
 *              knot. The adapter builds no ring, decides no winding and computes no tangent.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter port/types.hpp is included by more than a hundred own
 *              units of the avt build and takes no new or changed line (header ratchet, operator decision
 *              2026-11-21). These keys and codes are read by the template reader of GodotHostResourceManager, by
 *              the plant scene that hands a template to an organ and by their tests: a further template code
 *              rebuilds those units, never the hundred the port header reaches.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:101342 - hand organ shapes to app
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-23
 * @modified    2026-11-24
 * @version     00.00.02.00002 [core]
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

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: tmpl = template, an organ template of ase-render-data; fngs = fungus, the fruit body of a
 * mushroom (stem, hymenium, cap).
 */
constexpr const char* GODOT_KEY_TEMPLATE = "template";        // template records: the code of the template, GODOT_TMPL_*
constexpr const char* GODOT_KEY_INDEX = "index";              // corner record: its place in the template, named by the triangles
constexpr const char* GODOT_KEY_CORNER_A = "a";               // triangle record: index of its first corner
constexpr const char* GODOT_KEY_CORNER_B = "b";               // triangle record: index of its second corner
constexpr const char* GODOT_KEY_CORNER_C = "c";               // triangle record: index of its third corner
constexpr const char* GODOT_KEY_TX = "tx";  // tangent record: the unit tangent of a template corner along growing u, x share (HUB_TMPL_VTX_TX)
constexpr const char* GODOT_KEY_TY = "ty";  // tangent record: y share (HUB_TMPL_VTX_TY)
constexpr const char* GODOT_KEY_TZ = "tz";  // tangent record: z share (HUB_TMPL_VTX_TZ)
constexpr const char* GODOT_KEY_TW = "tw";  // tangent record: sign of the bitangent, +1 with growing v and -1 against it (HUB_TMPL_VTX_TW); the fourth value of ARRAY_TANGENT
constexpr uint32_t GODOT_TMPL_FINE = 0u;                      // template code: the fine cylinder, the internodes up to the shown orders
constexpr uint32_t GODOT_TMPL_CORS = 1u;                      // template code: the coarse cylinder, the internodes above them
constexpr uint32_t GODOT_TMPL_LEAF = 2u;                      // template code: the unit leaf, the stand-in of a leaf or a seed whose template file is missing
constexpr uint32_t GODOT_TMPL_FNGS = 3u;                      // template code: the unit fruit body, stem, hymenium and cap of a mushroom
constexpr uint32_t GODOT_TMPL_LEAF_CORS = 4u;                 // template code: the coarse unit leaf, the flat 1 x 1 card of a leaf of a body in the far band (TREEGEN T.6, FLRA_HOST_TMPL_LEAF_CORS)
constexpr uint32_t GODOT_TMPL_COUNT = 5u;                     // template codes the ports carry

}  // namespace ase::adp::godot

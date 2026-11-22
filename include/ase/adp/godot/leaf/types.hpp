#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the constants of the leaf cards the view bends and colours from their ports
 * @description All compile-time constants of the leaf sub-type; NO runtime values - those belong in the views.
 *              PLAN_ASE_VIVARIUM_TREEGEN T.6, Gitter mit Biegung: one record per bent leaf crosses the bend port of
 *              ase-pl-flora (flora.bend.v1), its object id the leaf, with the bend of its card across and along the
 *              midrib in leaf lengths. The view lays both into the custom data of the leaf instance, and the vertex
 *              shader lifts the leaf lattice by y += across * ramp(|x * 2|) + along * ramp(z). Farbe, Variante,
 *              Herbst: one record per leaf with a colour offset crosses the tint port (flora.tint.v1), and the
 *              fragment shader lays 1 + offset on the albedo of the card; one record per leaf with an atlas cell
 *              crosses the atlas port (flora.atlas.v1), and the fragment shader samples the texture at
 *              (u + cell) / cells. These are the keys of the three records, the values of a leaf without one, and
 *              the width of the custom data that carries them.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); these values are
 *              read only by the bend reader, the instance rows and the instance buffer, so they stand in the
 *              narrowest header those include.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @usecase     usecase:6540 - generate plant organs
 * @actor       Organ Arrangement Generator (ORGAARRGEN)
 * @created     2026-11-21
 * @modified    2026-11-21
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

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: none - the leaf sub-type names no file part of its own.
 */

constexpr const char* GODOT_KEY_BEND_ACROSS = "across";       // bend record: lift of the blade edge over the midrib, leaf lengths (FLORA_HOST_KEY_BEND_ACROSS of ase-pl-flora)
constexpr const char* GODOT_KEY_BEND_ALONG = "along";         // bend record: lift of the blade tip over its foot, leaf lengths (FLORA_HOST_KEY_BEND_ALONG of ase-pl-flora)
constexpr double GODOT_BEND_NONE = 0.0;                       // both bend values of an organ without a bend record, and the spare custom value: its card is drawn flat
constexpr const char* GODOT_KEY_TINT_OFFSET = "offset";       // tint record: colour offset of the leaf card, laid on its albedo as 1 + offset (FLORA_HOST_KEY_TINT_OFFSET of ase-pl-flora)
constexpr double GODOT_TINT_NONE = 0.0;                       // colour offset of an organ without a tint record: its card keeps the colour of its species, the factor 1 exactly
constexpr const char* GODOT_KEY_ATLS_CELL = "cell";           // atlas record: the cell of the leaf texture its card shows, a whole number below its cells (FLORA_HOST_KEY_ATLS_CELL of ase-pl-flora)
constexpr const char* GODOT_KEY_ATLS_CELLS = "cells";         // atlas record: the cells of the leaf texture of its species side by side along u (FLORA_HOST_KEY_ATLS_CELLS of ase-pl-flora)
constexpr double GODOT_ATLS_CELL_NONE = 0.0;                  // atlas cell of an organ without an atlas record: the first and only cell
constexpr double GODOT_ATLS_CELLS_NONE = 1.0;                 // atlas cells of an organ without an atlas record: the whole texture, so (u + 0) / 1 keeps u exactly
constexpr double GODOT_ATLS_PACK = 16.0;                      // the cell and the cells share channel a as cell * 16 + (cells - 1): above PLT_CONST_LEAF_ATLS_CNT_MAX 8, whole numbers a float holds exactly, and 0 for an organ without a record
constexpr uint32_t GODOT_INSTANCE_CUSTOM_VALUES = 4u;         // custom data of one instance on the MultiMesh buffer after its twelve values (INSTANCE_CUSTOM, RGBA float): r = bend across, g = bend along, b = colour offset of its leaf card, a = its atlas cell packed by GODOT_ATLS_PACK; the bark and its tiling reach the tube on CUSTOM0 of its vertices

}  // namespace ase::adp::godot

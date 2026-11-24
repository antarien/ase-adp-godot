#pragma once
/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the counts of the round-joined stroke mesh of the HUD
 * @description All compile-time constants of the stroke sub-type; NO runtime values - those belong in the drawer.
 *              ui_stroke (godot_hud_ui_stroke.cpp) lays its mesh through write pointers into arrays sized once
 *              for the most corners and indices the run can need, and cuts them to what it laid (P0692): a
 *              push_back per corner crossed the GDExtension border each time. These are the counts that bound: the
 *              corners and indices each piece of the mesh lays, read from the pieces as they are built.
 *
 *              WHY A TYPES HEADER OF ITS OWN: its readers are the stroke's units and their host test; a change
 *              compiles them alone, never every unit that sees godot_hud_ui_stroke.hpp (header ratchet,
 *              WRFL_ASE_TIER_WIPE). It carries no Godot type, so the host library reaches it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
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

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: none.
 */

/**
 * Measures of the stroke mesh - read by stroke_run and stroke_lay (godot_stroke_mesh.cpp), the Godot-free part of
 * ui_stroke the host test reaches.
 */
constexpr float GODOT_STROKE_POINT_EPS_PX = 1.0e-3f;  // screen-pixel distance at which two points of a stroke count as one
constexpr float GODOT_STROKE_JOIN_EPS = 1.0e-4f;      // smallest 1 + cosine of a knee's turn at which its inner sides still meet in one point
constexpr float GODOT_STROKE_INNER_SHARE = 0.5f;      // share of each segment the inner meeting point of a knee may reach back along it
constexpr float GODOT_STROKE_CORE_OPACITY = 1.0f;     // share of the ink's alpha at a corner of the core
constexpr float GODOT_STROKE_FEATHER_OPACITY = 0.0f;  // share of the ink's alpha at a corner on the feather's end

/**
 * Counts of the stroke mesh - the bound its arrays are sized to before the mesh is laid.
 */
constexpr int64_t GODOT_STROKE_KNEE_CORNERS = 9;      // corners a knee lays beside its fan: four outer, up to five inner (two pairs and the hub)
constexpr int64_t GODOT_STROKE_CAP_CORNERS = 5;       // corners a round end lays beside its fan: the hub and a core and feather pair on each side
constexpr int64_t GODOT_STROKE_DISC_CORNERS = 3;      // corners a stroke of no length lays beside its fan: the hub, one core and one feather point
constexpr int64_t GODOT_STROKE_STEP_CORNERS = 2;      // corners a fan lays for every step but its last: a core and a feather point
constexpr int64_t GODOT_STROKE_STEP_INDICES = 9;      // indices a fan lays for every step: one triangle from the hub and the two of its feather band
constexpr int64_t GODOT_STROKE_SEGMENT_INDICES = 18;  // indices the body of one segment lays: two core triangles and the two feather bands of two each
constexpr int64_t GODOT_STROKE_SPARE_STEPS = 1;       // a step added to the bound of an arc, against a turn that rounds past pi

}  // namespace ase::adp::godot

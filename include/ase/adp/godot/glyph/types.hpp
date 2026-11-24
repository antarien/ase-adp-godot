#pragma once
/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the measures of a HUD glyph at the text server
 * @description All compile-time constants of the glyph sub-type; NO runtime values - those belong in the drawers.
 *              The oversampling a HUD glyph is rastered with and the size a face's ascent and descent are read at
 *              once. Read by the icon (godot_hud_ui_icon.cpp) and the text line (godot_hud_ui_text_block.cpp).
 *
 *              WHY A TYPES HEADER OF ITS OWN: godot_hud_ui_icon.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These measures have two readers, so
 *              a change to one compiles those units again, never every unit that shows a symbol.
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
 * ABBREVIATIONS: PX = a pixel of the face's size, the unit the text server rasters a glyph in.
 */

/**
 * Measures of a HUD glyph - the text server's raster of an icon and of a line of text.
 */
constexpr float GODOT_TEXT_OVERSAMPLING = 1.0f;  // the text server's oversampling for a HUD glyph: its raster size already is the screen's pixels
constexpr int32_t GODOT_TEXT_DESIGN_PX = 4096;   // the size the face's ascent and descent are read at once: their share of the size to 1/4096, where a raster size rounds them to whole pixels

}  // namespace ase::adp::godot

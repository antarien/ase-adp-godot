#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the drawing style and the layout of AseVivariumView
 * @description The colours (RGBA, 8 bit per channel, Godot Color::hex), the font sizes, the line widths and the
 *              layout measures the view draws its patches, bars, buttons and panels with, and the form of a value
 *              drawn on the surface.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The style changes with the look of the view and is read by its
 *              drawing units.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
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

#include <cstdint>

namespace ase::adp::godot {

constexpr uint32_t GODOT_COLOR_BACKGROUND = 0x14181AFFu;    // surface behind everything
constexpr uint32_t GODOT_COLOR_PROGRESS = 0x8BC34AFFu;      // filled part of the start screen progress bar
constexpr uint32_t GODOT_COLOR_PROGRESS_TRACK = 0x37474FFFu;  // empty part of the start screen progress bar
constexpr uint32_t GODOT_COLOR_HEADER = 0x1F2A2EFFu;        // band behind title and build line
constexpr uint32_t GODOT_COLOR_SOIL = 0x5B4030FFu;          // bare soil of a patch
constexpr uint32_t GODOT_COLOR_SEED = 0x9C7A44FFu;          // seed dots on bare soil
constexpr uint32_t GODOT_COLOR_SPROUT = 0x8BC34AFFu;        // cover of a seedling
constexpr uint32_t GODOT_COLOR_GROW = 0x4CAF50FFu;          // cover of a growing stand
constexpr uint32_t GODOT_COLOR_MATURE = 0x2E7D32FFu;        // cover of a mature stand
constexpr uint32_t GODOT_COLOR_WILTED = 0xA1887FFFu;        // cover of a wilted stand
constexpr uint32_t GODOT_COLOR_DEAD = 0x6E6552FFu;          // cover of a dead stand
constexpr uint32_t GODOT_COLOR_UNKNOWN = 0x9E9E9EFFu;       // cover of a code the view does not know
constexpr uint32_t GODOT_COLOR_WATER = 0x42A5F5FFu;         // filled part of the soil water bar
constexpr uint32_t GODOT_COLOR_WATER_TRACK = 0x1C2B3AFFu;   // empty part of the soil water bar
constexpr uint32_t GODOT_COLOR_SELECTION = 0xFFD54FFFu;     // frame around the selected patch
constexpr uint32_t GODOT_COLOR_TEXT = 0xECEFF1FFu;          // regular text
constexpr uint32_t GODOT_COLOR_TEXT_DIM = 0x90A4AEFFu;      // secondary text
constexpr uint32_t GODOT_COLOR_BUTTON = 0x37474FFFu;        // enabled button face
constexpr uint32_t GODOT_COLOR_BUTTON_OFF = 0x23292CFFu;    // disabled button face
constexpr uint32_t GODOT_COLOR_ERROR = 0xC62828FFu;         // error panel

constexpr int32_t GODOT_FONT_TITLE = 44;                    // title size (logical pixels)
constexpr int32_t GODOT_FONT_BODY = 28;                     // info lines and button labels
constexpr int32_t GODOT_FONT_SMALL = 20;                    // build line, patch labels, notes
constexpr int32_t GODOT_FONT_MIN = 12;                      // smallest size a line shrinks to so that it fits its width
constexpr float GODOT_FRAME_WIDTH = 6.0f;                   // selection frame line width
constexpr float GODOT_BORDER_WIDTH = 2.0f;                  // patch and button outline width
constexpr float GODOT_BORDER_WIDTH_ACTIVE = 4.0f;           // outline of the debug button whose time lapse is running
constexpr float GODOT_PATCH_INSET = 14.0f;                  // gap between patch edge and cover
constexpr float GODOT_BAR_HEIGHT = 16.0f;                   // height of the soil water bar
constexpr float GODOT_LABEL_OFFSET = 34.0f;                 // baseline of a label below a rect's top
constexpr float GODOT_HALF = 0.5f;                          // centre of a span
constexpr float GODOT_LAYOUT_SCALE_MAX = 1.0f;              // a layout is never magnified: spare space is distributed, not filled
constexpr float GODOT_SEED_DOT_RADIUS = 5.0f;               // radius of one seed dot on bare soil
constexpr int32_t GODOT_SEED_DOTS = 3;                      // seed dots per row and per column
constexpr int32_t GODOT_ERROR_LINES = 6;                    // most lines the error panel wraps to
constexpr int64_t GODOT_TEXT_DECIMALS = 1;                  // decimals of a value drawn on the surface
constexpr double GODOT_PERCENT = 100.0;                     // a share drawn as a percentage

}  // namespace ase::adp::godot

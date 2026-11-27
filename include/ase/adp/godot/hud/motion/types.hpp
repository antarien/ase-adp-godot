#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the motion of the HUD: the CSS easings and the cubic-bezier solver
 * @description The motion of the HUD (06.3 UiMotion): a design token names a CSS easing, these are the control
 *              points CSS Easing Functions Level 1 defines for its keywords, and the solver of cubic-bezier().
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The easings are read by UiMotion only.
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

#include <cstdint>

namespace ase::adp::godot {

constexpr const char* GODOT_EASE_LINEAR = "linear";                 // CSS keyword: progress equals time
constexpr const char* GODOT_EASE = "ease";                          // CSS keyword ease
constexpr const char* GODOT_EASE_IN = "ease-in";                    // CSS keyword ease-in
constexpr const char* GODOT_EASE_OUT = "ease-out";                  // CSS keyword ease-out
constexpr const char* GODOT_EASE_IN_OUT = "ease-in-out";            // CSS keyword ease-in-out
constexpr const char* GODOT_EASE_BEZIER = "cubic-bezier(";          // head of a CSS cubic-bezier(x1, y1, x2, y2)
constexpr const char* GODOT_EASE_BEZIER_END = ")";                  // tail of that function
constexpr const char* GODOT_EASE_BEZIER_SEPARATOR = ",";            // separator of its four numbers
constexpr int64_t GODOT_EASE_BEZIER_POINTS = 4;                     // x1, y1, x2, y2: the numbers a cubic-bezier() carries
constexpr double GODOT_EASE_DEFAULT_X1 = 0.25;                      // ease = cubic-bezier(0.25, 0.1, 0.25, 1): first control x
constexpr double GODOT_EASE_DEFAULT_Y1 = 0.1;                       // ease: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_DEFAULT_X2 = 0.25;                      // ease: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_DEFAULT_Y2 = 1.0;                       // ease: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_X1 = 0.42;                           // ease-in = cubic-bezier(0.42, 0, 1, 1): first control x
constexpr double GODOT_EASE_IN_Y1 = 0.0;                            // ease-in: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_X2 = 1.0;                            // ease-in: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_Y2 = 1.0;                            // ease-in: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_X1 = 0.0;                           // ease-out = cubic-bezier(0, 0, 0.58, 1): first control x
constexpr double GODOT_EASE_OUT_Y1 = 0.0;                           // ease-out: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_X2 = 0.58;                          // ease-out: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_Y2 = 1.0;                           // ease-out: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_X1 = 0.42;                       // ease-in-out = cubic-bezier(0.42, 0, 0.58, 1): first control x
constexpr double GODOT_EASE_IN_OUT_Y1 = 0.0;                        // ease-in-out: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_X2 = 0.58;                       // ease-in-out: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_Y2 = 1.0;                        // ease-in-out: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_LINEAR_X1 = 0.0;                        // linear as a bezier on the diagonal: first control x
constexpr double GODOT_EASE_LINEAR_Y1 = 0.0;                        // linear on the diagonal: first control y
constexpr double GODOT_EASE_LINEAR_X2 = 1.0;                        // linear on the diagonal: second control x
constexpr double GODOT_EASE_LINEAR_Y2 = 1.0;                        // linear on the diagonal: second control y
constexpr int32_t GODOT_EASE_SOLVE_STEPS = 24;                      // bisection steps that find the curve parameter of a time (2^-24 of the span)
constexpr double GODOT_EASE_SOLVE_EPSILON = 1.0e-6;                 // time distance (share of the duration) at which the parameter counts as found

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - THE SHADERS OF THE CARE VIEW
 *
 * @file        godot_plant_shader.hpp
 * @brief       Loads a shader of the care view's 3D scene from its file below res://shaders/
 * @description Every shader of the care view is a file of its own in adapter/ase-adp-godot/shaders/ - the ground,
 *              the bark, the organs, the water, the sky, the stars, the clouds, the halo of the sun, the moon and
 *              the shimmer of hot air - and the swing of a branch is an include block those of the organs pull in
 *              (`#include "res://shaders/sway_*.gdshaderinc"`). The stage copies them to godot/shaders/
 *              (scripts/stage_native.py, --shader-dir), the view loads them at run time.
 *
 *              A SHADER IS DATA, NEVER A STRING IN A HEADER. Held as a C string in types.hpp, which 115 of the
 *              adapter's units include, one changed character of one shader compiled every one of them again (a
 *              tier wipe, WRFL_ASE_TIER_WIPE cause 3: a shared thing kept in a header instead of in data). Held as
 *              a file, a change to it compiles no unit at all: the next stage copies it and the view reads it.
 *              The names of a shader's uniforms stand in the narrow header of the one unit that sets them, never
 *              in types.hpp, so a new uniform recompiles that unit and its test and nothing else.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/shader.hpp>

namespace ase::adp::godot {

/**
 * The shader of `file` (a res:// path below res://shaders/), loaded once and shared by every material that asks
 * for it again (ResourceLoader's cache). A file the stage did not copy is reported as an error with its path and
 * gives an empty reference: the material then draws without a shader, and the log names which one is missing.
 */
::godot::Ref<::godot::Shader> load_plant_shader(const char* file);

}  // namespace ase::adp::godot

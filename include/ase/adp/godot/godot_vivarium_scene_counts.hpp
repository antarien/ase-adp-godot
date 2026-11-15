/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW SCENE COUNTS
 *
 * @file        godot_vivarium_scene_counts.hpp
 * @brief       scene_counts - what the 3D scene draws, in one line of the window acceptance
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 and register D1647: the ground samples and cells, the
 *              internodes and leaves, the light of the sky port, the stars, moons and sun the camera frames and
 *              the starfield as it stands. A black window and a drawn one show apart only here, not in the patch
 *              values. follow_scene (godot_vivarium_scene.cpp) writes the line on the events of ground, organs,
 *              hour and star set.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#pragma once

#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ase::adp::godot {

/**
 * What the scene draws, in one line: the ground samples and cells, the internodes and leaves, and the light of the
 * sky port, with the stars, moons and sun the camera frames. The window acceptance reads it (register D1647).
 *
 * @param resources the ports the frame was filled from
 * @param scene     the 3D scene the view built (build_plant_scene); its camera and starfield are read
 * @return the fields of the line, separated by spaces
 */
::godot::String scene_counts(const GodotHostResourceManager& resources, ::godot::Node3D* scene);

}  // namespace ase::adp::godot

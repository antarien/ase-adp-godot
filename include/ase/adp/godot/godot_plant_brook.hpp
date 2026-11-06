#pragma once

/**
 * ASE GODOT ADAPTER - THE WATER OF THE CARE VIEW
 *
 * @file        godot_plant_brook.hpp
 * @brief       The node of the water in the care view's 3D scene and its fill
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3: the water stands at NN0. build_water_table
 *              (godot_plant_water.hpp) lays the one plane over every ground cell below the table; the stream
 *              port (FloraHostStrmExptSystem) marks where the plane flows. This file hangs it into the scene
 *              in a ShaderMaterial of GODOT_WATER_SHADER_CODE. Nothing of the water is computed here.
 *              THE COLOUR OF THE WATER: the material sets the two colour uniforms of the shader to the
 *              green of a lowland brook (algae and fine silt), so the water reads green under a grey or a
 *              blue sky; the shallow colour lets the gravel of the bed show through, the deep one is the
 *              middle of the strip. They stand here and not in types.hpp, which nearly every unit of the
 *              adapter includes.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-01
 * @modified    2026-11-06
 * @version     00.00.03.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <godot_cpp/classes/node3d.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

constexpr const char* GODOT_WATER_SHALLOW = "shallow_color";  // uniform of GODOT_WATER_SHADER_CODE: the colour at no depth
constexpr const char* GODOT_WATER_DEEP = "deep_color";        // uniform of GODOT_WATER_SHADER_CODE: the colour at GODOT_WATER_DEPTH_FULL_M
constexpr float GODOT_WATER_SHALLOW_R = 0.24f;                // shallow brook water over gravel, red
constexpr float GODOT_WATER_SHALLOW_G = 0.40f;                // shallow brook water over gravel, green
constexpr float GODOT_WATER_SHALLOW_B = 0.24f;                // shallow brook water over gravel, blue
constexpr float GODOT_WATER_DEEP_R = 0.08f;                   // deep brook water, red
constexpr float GODOT_WATER_DEEP_G = 0.26f;                   // deep brook water, green
constexpr float GODOT_WATER_DEEP_B = 0.16f;                   // deep brook water, blue

/**
 * Hangs the empty water node (GODOT_PLANT_WATER) under the scene root, in a ShaderMaterial of
 * GODOT_WATER_SHADER_CODE with the green of GODOT_WATER_SHALLOW_* and GODOT_WATER_DEEP_*, casting no shadow.
 * build_plant_scene calls it once, after the ground stands.
 */
void build_plant_water(::godot::Node3D* scene);

/**
 * Replaces the water with build_water_table over the stream records, the terrain samples and the ground
 * cells; the node keeps its material. A scene without the node draws no water.
 *
 * @return the ground cells under water
 */
uint32_t fill_plant_water(::godot::Node3D* scene, const std::vector<VivariumStreamView>& streams,
                          const std::vector<VivariumTerrainView>& samples,
                          const std::vector<VivariumGroundCellView>& cells);

}  // namespace ase::adp::godot

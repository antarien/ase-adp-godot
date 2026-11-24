#pragma once

/**
 * ASE GODOT ADAPTER - THE LAKE, SEA AND GLACIER WATER OF THE CARE VIEW
 *
 * @file        godot_plant_water_kinds.hpp
 * @brief       The three nodes of lake, sea and glacier water in the care view's 3D scene and their fill
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO, "Fuenf Shader im Adapter, je einer Wasserart": the groundwater of a pit
 *              and the river stand in godot_plant_brook.hpp; this file adds the other three. Each kind is one node in
 *              a ShaderMaterial of its own shader: still water of the lakes and ponds (lake_water.gdshader, the surface
 *              at the top face of the box the port carries), open salt water at NN0 (sea_water.gdshader, one plane per
 *              salt chunk) and the ice over the ground of a chunk with the meltwater on it (glacier_water.gdshader).
 *              THE VIEW SETS NO FALLBACK: a value a record lacks draws no water of that kind and is counted as a gap
 *              the caller reports; the view never sets NN0 for a lake and never an ice thickness. The names, shader
 *              paths and colours stand here and not in scene/types.hpp or godot_plant_brook.hpp, which other units
 *              include (WRFL_ASE_TIER_WIPE, the header ratchet): only the unit that lays the three kinds includes it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_glacier_view.hpp>
#include <ase/adp/godot/godot_host_lake_view.hpp>
#include <ase/adp/godot/godot_host_sea_view.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <godot_cpp/classes/node3d.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

// The three nodes and their shaders (shaders/lake_water.gdshader, sea_water.gdshader, glacier_water.gdshader, loaded
// by godot_plant_shader.hpp).
constexpr const char* GODOT_PLANT_LAKE_WATER = "PlantLakeWater";        // the still water of the lakes and ponds
constexpr const char* GODOT_PLANT_SEA_WATER = "PlantSeaWater";          // the open salt water at NN0
constexpr const char* GODOT_PLANT_GLACIER_WATER = "PlantGlacierWater";  // the ice over the ground of the glacier chunks
constexpr const char* GODOT_LAKE_WATER_SHADER_FILE = "res://shaders/lake_water.gdshader";
constexpr const char* GODOT_SEA_WATER_SHADER_FILE = "res://shaders/sea_water.gdshader";
constexpr const char* GODOT_GLACIER_WATER_SHADER_FILE = "res://shaders/glacier_water.gdshader";

// The uniforms of the three shaders the material sets.
constexpr const char* GODOT_KINDS_SHALLOW = "shallow_color";  // colour of the water at no depth
constexpr const char* GODOT_KINDS_DEEP = "deep_color";        // colour of the water at its full depth
constexpr const char* GODOT_KINDS_ICE = "ice_color";          // colour of the ice
constexpr const char* GODOT_KINDS_MELT = "melt_color";        // colour of the meltwater sheen on the ice

// The height datum of the engine: the sea surface lies at NN0, y = 0 of the frame the ground mesh stands in (the y of a
// terrain vertex is its height above NN0). It is the datum of the world, no default a missing value falls back to.
constexpr float GODOT_WATER_DATUM_NN0_M = 0.0f;

// The full scales of the shares the vertex colours carry (water_kinds_* in godot_plant_water_kinds_geom.hpp).
constexpr float GODOT_LAKE_DEPTH_FULL_M = 10.0f;    // a lake of this depth and deeper reads fully deep (the deepest the generator lays, HYD_LAKE_MAX_DEPTH)
constexpr float GODOT_SEA_DEPTH_FULL_M = 100.0f;    // sea over ground this far under NN0 reads fully deep
constexpr float GODOT_GLACIER_ICE_FULL_M = 5.0f;    // ice of this thickness, metres of water column, reads fully thick
constexpr float GODOT_GLACIER_MELT_FULL_MM_H = 50.0f;  // melt of this many mm of water column per hour reads fully melting

constexpr float GODOT_LAKE_SHALLOW_R = 0.20f;   // lake water at its shore, red
constexpr float GODOT_LAKE_SHALLOW_G = 0.34f;   // lake water at its shore, green
constexpr float GODOT_LAKE_SHALLOW_B = 0.30f;   // lake water at its shore, blue
constexpr float GODOT_LAKE_DEEP_R = 0.04f;      // deep lake water, red
constexpr float GODOT_LAKE_DEEP_G = 0.14f;      // deep lake water, green
constexpr float GODOT_LAKE_DEEP_B = 0.17f;      // deep lake water, blue
constexpr float GODOT_SEA_SHALLOW_R = 0.10f;    // sea water over a shallow shelf, red
constexpr float GODOT_SEA_SHALLOW_G = 0.40f;    // sea water over a shallow shelf, green
constexpr float GODOT_SEA_SHALLOW_B = 0.46f;    // sea water over a shallow shelf, blue
constexpr float GODOT_SEA_DEEP_R = 0.02f;       // open deep sea, red
constexpr float GODOT_SEA_DEEP_G = 0.10f;       // open deep sea, green
constexpr float GODOT_SEA_DEEP_B = 0.22f;       // open deep sea, blue
constexpr float GODOT_GLACIER_ICE_R = 0.78f;    // glacier ice, red
constexpr float GODOT_GLACIER_ICE_G = 0.90f;    // glacier ice, green
constexpr float GODOT_GLACIER_ICE_B = 0.96f;    // glacier ice, blue
constexpr float GODOT_GLACIER_MELT_R = 0.30f;   // meltwater sheen on the ice, red
constexpr float GODOT_GLACIER_MELT_G = 0.55f;   // meltwater sheen on the ice, green
constexpr float GODOT_GLACIER_MELT_B = 0.70f;   // meltwater sheen on the ice, blue

/** What one fill laid: the drawn bodies of each kind and the records that lacked a value and so drew nothing. */
struct WaterKindsFilled {
    uint32_t lakes = 0;              // lake surfaces laid
    uint32_t seas = 0;               // sea planes laid
    uint32_t glaciers = 0;           // glacier chunks that laid ice
    uint32_t glacier_triangles = 0;  // triangles of ground the ice lies on
    uint32_t lake_gaps = 0;          // lake records short of a value of their box or depth
    uint32_t sea_gaps = 0;           // sea records short of a value of their chunk
    uint32_t glacier_gaps = 0;       // glacier records short of a value of their chunk, ice or melt
};

/**
 * Hangs the three empty nodes (GODOT_PLANT_LAKE_WATER, GODOT_PLANT_SEA_WATER, GODOT_PLANT_GLACIER_WATER) under the
 * scene root, each in a ShaderMaterial of its shader with its colours, casting no shadow. build_plant_water calls it
 * once beside the brook and pit nodes.
 */
void build_plant_water_kinds(::godot::Node3D* scene);

/**
 * Replaces the three kinds with the surfaces of the records and the ground mesh `ground` as fill_plant_ground left it:
 * the lakes at the top face of their box, one sea plane per salt chunk at NN0, the ice over the ground of every
 * glacier chunk. A record short of a value draws nothing and is counted in the gap of its kind; a scene without a node
 * draws none of that kind.
 */
WaterKindsFilled fill_plant_water_kinds(::godot::Node3D* scene, const std::vector<VivariumLakeView>& lakes,
                                        const std::vector<VivariumSeaView>& seas,
                                        const std::vector<VivariumGlacierView>& glaciers,
                                        const PlantMeshArrays& ground);

}  // namespace ase::adp::godot

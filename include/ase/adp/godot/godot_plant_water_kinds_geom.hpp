#pragma once

/**
 * ASE GODOT ADAPTER - THE GEOMETRY OF THE LAKE, SEA AND GLACIER SURFACES
 *
 * @file        godot_plant_water_kinds_geom.hpp
 * @brief       The surface of a lake, of a sea chunk and of the ice over a chunk as mesh arrays, without a Godot type
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO, section "Geometrie-Schicht: Wasser im Bild", items 3 to 5. The three
 *              surfaces are laid in the frame the ground mesh stands in: x metres east, y metres of height above NN0
 *              (the height of a terrain sample is the y of its vertex), z metres north. The engine writes the box of a
 *              lake with z as its height, so the surface of the lake is the top face of the box, centre z plus half z,
 *              at the east and north of its centre x and y. The sea surface lies at NN0, the zero of the world height
 *              of the engine, which is y = 0 of that frame and no fallback: a chunk that lacks one of its values
 *              lays nothing. The ice lies over the ground vertices of its chunk, each lifted by the thickness the
 *              record gives; the chunk of grid index i spans [i * edge, (i + 1) * edge) as the water bands tile it.
 *
 *              The vertex colour carries the shares the shaders read, each already over its full scale and within 0
 *              to 1, alpha 1: the lake its depth in red, the sea the depth below NN0 in red, the ice its thickness
 *              in red and its melt in green. Nothing but the drawing geometry and these shares is computed here.
 *
 *              NO GODOT TYPE: values in, mesh arrays out, so the host tests check it without a Godot process.
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

namespace ase::adp::godot {

/** The full scales of the shares the vertex colours carry; a value at or over its scale is 1. */
struct WaterKindsScale {
    float lake_depth_m = 0.0f;   // depth of a lake that reads fully deep
    float sea_depth_m = 0.0f;    // depth below NN0 that reads fully deep
    float ice_m = 0.0f;          // thickness of ice, metres of water column, that reads fully thick
    float melt_mm_h = 0.0f;      // melt, millimetres of water column per hour, that reads fully melting
};

/**
 * Appends the surface of `lake`, one quad at the top face of its box, to `out`. Nothing is appended when the lake is
 * not drawable (lake_drawable) or the scale has no depth. Returns whether a quad was laid.
 */
bool water_kinds_lake_surface(const VivariumLakeView& lake, const WaterKindsScale& scale, PlantMeshArrays& out);

/**
 * Appends the sea of one chunk, one quad over the chunk at the height `datum_m` (NN0), to `out`. Nothing is appended
 * when the chunk is not drawable (sea_drawable). Returns whether a quad was laid.
 */
bool water_kinds_sea_plane(const VivariumSeaView& sea, float datum_m, const WaterKindsScale& scale,
                           PlantMeshArrays& out);

/**
 * Appends the ice of one chunk to `out`: every triangle of `ground` whose centre lies in the chunk, its three vertices
 * lifted by the ice thickness (glcr_ice_mm over 1000, metres of water column). Nothing is appended when the chunk is
 * not drawable (glacier_drawable). Returns the triangles laid.
 */
uint32_t water_kinds_glacier_cap(const VivariumGlacierView& glacier, const PlantMeshArrays& ground,
                                 const WaterKindsScale& scale, PlantMeshArrays& out);

}  // namespace ase::adp::godot

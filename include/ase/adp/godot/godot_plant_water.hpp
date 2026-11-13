#pragma once

/**
 * ASE GODOT ADAPTER - THE WATER OF THE BROOKS AT THE LEVEL ASE-HYDRO RECKONS
 *
 * @file        godot_plant_water.hpp
 * @brief       The water plane in the bed of every brook at its bed plus the ase-hydro depth, and the ribbon along a course
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3, B0281. ase-hydro reckons the water and the view
 *              reckons none: build_water_table lays the plane in the bed of every stream record at its bed plus
 *              the depth ase-hydro gives it (HydroHubStrmPubSystem over the stream port), and no water where no
 *              bed lies. The plane carries the velocity and course of the stream within its bed. The care view
 *              draws this plane as its water and nothing else.
 *
 *              build_water lays a ribbon as wide as a bed along the deepest line of that bed, its surface the
 *              depth above the bed: the water of a river running along a course above the table, a brook
 *              falling from a mountain. The care view lays none now.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check it without a Godot
 *              process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-01
 * @modified    2026-11-13
 * @version     00.00.05.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/**
 * Appends the water of `streams` to `out`: per stream GODOT_WATER_STATIONS sections from the start of the bed
 * to its end, two vertices per station at half the width to either side of the deepest line of the bed, its
 * height the depth above the bed at the station. The deepest line is followed station by station: the lowest
 * sample of the cross section around the line followed so far is the bed, and the middle moves to the centre
 * of the wetted section, never farther off the filed course than GODOT_WATER_SWING_SHARE of the bed width. A
 * winding bed carries its water round its bends; a straight one keeps it on the filed course. The sides stand
 * normal to the course the middles draw. Where the cross section holds no sample, the bed is the lowest sample
 * within the half width, else the nearest; the surface never rises along the course. A station farther than
 * `step_m` beyond the half width from any sample lies outside the ground port and breaks the ribbon there. The
 * vertex colour carries velocity and depth (GODOT_WATER_SHADER_FILE). A stream without length, width or depth
 * draws nothing.
 *
 * @return the streams drawn
 */
uint32_t build_water(const std::vector<VivariumStreamView>& streams, const std::vector<VivariumTerrainView>& samples,
                     float step_m, PlantMeshArrays& out);

/**
 * Appends the water table to `out` over the ground `ground` as build_ground laid it: every six indices of the
 * ground are one cell, its triangles (sw, se, ne) and (sw, ne, nw). The water level of a stream at each of its
 * GODOT_WATER_STATIONS sections is the lowest ground vertex of the section within the bed (half the bed width
 * plus GODOT_WATER_SWING_SHARE of it from the line between its ends) plus the depth ase-hydro gives the stream,
 * never sinking toward the source under the level below it; a dry stream holds no water. A ground vertex in a
 * bed carries the highest level of the streams whose beds hold it. For every cell with such a corner below its
 * level, the quad of that cell at the level, a bank corner outside every bed at the lowest level of the cell's bed
 * corners, its two triangles in the ground's order (the front face looking up). One vertex per ground vertex a
 * wet cell names, shared by the cells that name it; its texture coordinates the world metres east and north. The
 * vertex colour carries the water: red the velocity share of the fastest stream whose bed holds the vertex, green
 * the depth of the water above the ground vertex over GODOT_WATER_DEPTH_FULL_M, blue and alpha that stream's
 * unit course mapped into [0, 1] (north where no stream flows). A cell in no bed, or wholly above its water,
 * draws nothing; a cell whose corner the terrain port did not carry is no cell of the ground and draws nothing
 * either.
 *
 * The ground is read as the dig left it: patch_ground writes the moved samples into the same arrays, so the
 * table follows a dig without a lookup by object id (P0690).
 *
 * @return the cells drawn
 */
uint32_t build_water_table(const std::vector<VivariumStreamView>& streams, const PlantMeshArrays& ground,
                           PlantMeshArrays& out);

}  // namespace ase::adp::godot

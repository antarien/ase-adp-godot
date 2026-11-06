#pragma once

/**
 * ASE GODOT ADAPTER - THE WATER TABLE AT NN0 AND THE WATER ALONG A COURSE
 *
 * @file        godot_plant_water.hpp
 * @brief       The one water plane at NN0 over every ground cell below it, and the ribbon of a river along a course
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3. The water stands at NN0 (height 0) and nowhere
 *              else, over the whole ground: build_water_table lays one plane at GODOT_WATER_TABLE_M over every
 *              ground cell that reaches below it, so whatever is dug or carved below NN0 holds water and the
 *              groundwater of the roots is this table. The stream port only marks where the plane flows:
 *              ase-hydro computes velocity and depth, the plane carries the velocity and course of the stream
 *              within its bed. The care view draws this plane as its water and nothing else.
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
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-01
 * @modified    2026-11-05
 * @version     00.00.03.00000
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
 * vertex colour carries velocity and depth (GODOT_WATER_SHADER_CODE). A stream without length, width or depth
 * draws nothing.
 *
 * @return the streams drawn
 */
uint32_t build_water(const std::vector<VivariumStreamView>& streams, const std::vector<VivariumTerrainView>& samples,
                     float step_m, PlantMeshArrays& out);

/**
 * Appends the water table to `out`: for every cell of `cells` whose four corners `samples` carries and one of
 * which lies below GODOT_WATER_TABLE_M, the quad of that cell at the table height, its two triangles in the
 * order the cell names its corners (the ground's order, the front face looking up). One vertex per corner
 * sample, shared by the cells that name it; its texture coordinates the world metres east and north. The vertex
 * colour carries the water: red the velocity share of the fastest stream whose bed holds the vertex (half the
 * bed width plus GODOT_WATER_SWING_SHARE of it from the line between its ends), green the depth of the table
 * above the sample over GODOT_WATER_DEPTH_FULL_M, blue and alpha that stream's unit course mapped into [0, 1]
 * (north where no stream flows). A cell wholly above the table, or one with a corner the port does not carry,
 * draws nothing.
 *
 * @return the cells drawn
 */
uint32_t build_water_table(const std::vector<VivariumStreamView>& streams,
                           const std::vector<VivariumTerrainView>& samples,
                           const std::vector<VivariumGroundCellView>& cells, PlantMeshArrays& out);

}  // namespace ase::adp::godot

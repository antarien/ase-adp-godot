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
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_pit_view.hpp>
#include <ase/adp/godot/godot_host_stream_view.hpp>
#include <ase/adp/godot/godot_host_terrain_view.hpp>
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
 * never sinking toward the source under the level below it nor under the groundwater table ase-hydro gives the
 * stream (its groundwater_level_m, none while the record carries none, N0484); a dry stream holds no water of its
 * own. A ground vertex in a bed carries the highest level of the streams whose beds hold it. The corners of a
 * ground cell that holds the centre of a pit record carry the water of that pit (pit_water_levels, D2506). A
 * ground vertex in no bed and no pit carries no water: the view lays no table of its own. For every cell with a
 * corner below its level, the quad of that cell at the level, a bank corner outside every bed at the lowest level
 * of the cell's bed corners where that stands above its own water, a corner neither bed nor pit names at the
 * lowest water of the cell, its two triangles in the ground's order (the front face looking up). One vertex per ground vertex a wet
 * cell names, shared by the cells that name it; its texture coordinates the world metres east and north. The
 * vertex colour carries the water: red the velocity share of the fastest stream whose bed holds the vertex, green
 * the depth of the water above the ground vertex over GODOT_WATER_DEPTH_FULL_M, blue and alpha that stream's
 * unit course mapped into [0, 1] (north where no stream flows). A cell wholly above its water draws nothing; a
 * cell whose corner the terrain port did not carry is no cell of the ground and draws nothing either.
 *
 * The ground is read as the dig left it: patch_ground writes the moved samples into the same arrays, so the
 * table follows a dig without a lookup by object id (P0690).
 *
 * @return the cells drawn
 */
uint32_t build_water_table(const std::vector<VivariumStreamView>& streams, const std::vector<VivariumPitView>& pits,
                           const PlantMeshArrays& ground, PlantMeshArrays& out);

/**
 * The same table with each kind of water in its own arrays (D2508, PLAN_ASE_VIVARIUM_PHASE_10_HYDRO "Geometrie-
 * Schicht", five shaders): a cell with a corner some bed holds goes to `brook_out`, the river shader draws it; a cell
 * only a pit floods goes to `pit_out`, the groundwater of the pit, drawn still by GODOT_PIT_WATER_SHADER_FILE. Each
 * arrays lays its own vertices; the same arrays passed twice lay them once, as the overload above does.
 *
 * @return the cells drawn, both kinds
 */
uint32_t build_water_table(const std::vector<VivariumStreamView>& streams, const std::vector<VivariumPitView>& pits,
                           const PlantMeshArrays& ground, PlantMeshArrays& brook_out, PlantMeshArrays& pit_out);

/**
 * Lays the water of `pits` into `level_of` and `flooded`, one entry per vertex of `ground` (godot_plant_water_pit.cpp):
 * every ground cell, six indices with corners sw, se, ne, nw, whose extent read from its own corners holds the
 * centre (x, z) of a pit record with a finite positive depth carries on its four corners the floor of the cell, its
 * lowest corner, plus that depth - the open water ase-hydro holds in the pit (HydroCellPitCalcSystem, answered by
 * ase-gis as GIS_HYD_CELL_PIT_DEPTH, filed on hydrogeology.pit.v1). A corner already flooded keeps the higher
 * water. A pit ase-gis has not answered (NaN), a dry pit and a pit on no cell lay nothing; a cell with an index
 * past the ground is no cell; vectors shorter than the ground vertices take nothing.
 */
void pit_water_levels(const std::vector<VivariumPitView>& pits, const PlantMeshArrays& ground,
                      std::vector<float>& level_of, std::vector<uint8_t>& flooded);

/**
 * Appends the line the water of `stream` runs along to `out`, x, height and z per point, from the start of the
 * bed to its end (B0285): one point per station of the GODOT_WATER_STATIONS sections that holds water, at the
 * level build_water_table lays there, and across the bed at the centre of the ground vertices of the section
 * under that level, each weighted by the water over it. The water of a winding bed stands off the line between
 * the ends, and this line follows it, so a mark laid along it lies on the water the table draws. Only the
 * vertices whose water `seen` answers true for (`context`, x, level, z) count: the camera looks flat over the
 * bank of a carved bed, the lip hides the near water, and the centre of all the water landed on the bank under
 * the strip the eye sees (run 20261113-1951-m05-rain). A station no ground vertex falls into, or none of whose
 * vertices lies under its level with its water seen, lays no point; a dry stream lays none.
 *
 * @return the points appended
 */
uint32_t build_water_line(const VivariumStreamView& stream, const PlantMeshArrays& ground,
                          bool (*seen)(const void* context, float x, float y, float z), const void* context,
                          std::vector<float>& out);

/**
 * Appends the line the carved bed of `stream` runs along as the eye sees it to `out`, x, height and z per point,
 * from the start of the bed to its end (B0285): one point per station of the GODOT_WATER_STATIONS sections, at
 * the centre of the ground vertices of the section `seen` answers true for (`context`, x, height, z), each
 * weighted by how far it lies under the highest vertex of its section, so the bank around the bed weighs nothing
 * and the walls and floor of the bed carry the point, which stands on that ground. A bed carved 0.6 m deep with
 * water 0.17 m deep in it shows the eye 1.9 m high no water over its near lip (run 20261113-d2309-tri: 1716 of
 * 1716 water points hidden, no water colour in the picture); what the eye sees of the brook then is its bed, and
 * a mark laid along this line lies on it. A station without a seen vertex under its highest one lays no point.
 *
 * @return the points appended
 */
uint32_t build_bed_line(const VivariumStreamView& stream, const PlantMeshArrays& ground,
                        bool (*seen)(const void* context, float x, float y, float z), const void* context,
                        std::vector<float>& out);

}  // namespace ase::adp::godot

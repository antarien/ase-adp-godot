#pragma once

/**
 * ASE GODOT ADAPTER - GROUND AROUND THE PLANT
 *
 * @file        godot_plant_ground.hpp
 * @brief       The ground mesh from the terrain and ground cell ports, the marks its shader paints by, and its patch
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 and 03.11: build_ground takes the terrain samples and the
 *              ground cells as ase-terrain and ase-render-data laid them; mark_ground gives every vertex the
 *              colour the ground shader reads as its cover - levelled ground of a patch's disc or the player's,
 *              the bed of a brook (GODOT_GROUND_SHADER_FILE). Nothing is computed about the terrain: no grid,
 *              no normal, no winding.
 *
 *              P0690, a dig reaches the drawing as a change (CAUSA_ASE_TERRAIN): the terrain port replaces the
 *              moved samples in place and keeps the order of the rest, so patch_ground writes only the vertices
 *              that moved and the colours that changed into the bytes Godot keeps of the surface, and names the
 *              spans to hand on. The whole ground is built anew only where the samples or the cells are no
 *              longer the ones the mesh was built from.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays and bytes out, so the host tests check it without a
 *              Godot process. Where Godot keeps a vertex is read from Godot (godot_plant_ground_mesh.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-10-26
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_terrain_view.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/**
 * A disc of levelled ground (03.8): ase-terrain levels the disc under the plant of a patch, and the player
 * levels one where he taps. Its centre and radius, its height the one the ground stands at once levelled.
 * Ground that stands at this height inside the disc is levelled ground - bare soil, no grass.
 */
struct PlantGroundDisc {
    float x = 0.0f;       // centre, world metres east
    float z = 0.0f;       // centre, world metres north
    float radius = 0.0f;  // metres
    float height = 0.0f;  // levelled height, metres
};

/** The bed of one brook (03.8), configured or laid by the player: the two ends and half its width, metres. */
struct PlantGroundBed {
    float x = 0.0f;
    float z = 0.0f;
    float end_x = 0.0f;
    float end_z = 0.0f;
    float half_m = 0.0f;
};

/** One sample of the terrain port in the order the port carried it: its object id and its vertex, -1 when an
 *  earlier sample already named that id. */
struct PlantGroundSlot {
    uint32_t object_id = 0u;
    int32_t  vertex = -1;
};

/**
 * Where Godot keeps one vertex of the ground in the two byte streams of its surface, as RenderingServer names it
 * for the surface's format (mesh_surface_get_format_offset and its strides): the vertex stream holds the place
 * as three floats and the normal as two uint16 of its octahedral code, the attribute stream the colour as four
 * uint8 and the texture coordinates as two floats. A normal stride of 8 carries behind the normal the tangent
 * Godot 4.5 derives for a surface that came without tangents, two uint16 more.
 */
struct PlantGroundLayout {
    uint32_t position_offset = 0u;
    uint32_t position_stride = 0u;
    uint32_t normal_offset = 0u;
    uint32_t normal_stride = 0u;
    uint32_t color_offset = 0u;
    uint32_t uv_offset = 0u;
    uint32_t attribute_stride = 0u;
};

/** Bytes of one stream to hand to Godot, from `begin` up to `end`; begin == end hands nothing. */
struct PlantGroundSpan {
    uint32_t begin = 0u;
    uint32_t end = 0u;
};

/**
 * The ground as the view last handed it to Godot: its arrays, the samples and cells it was built from, and the
 * bytes Godot keeps of its surface. `patchable` holds only once the encoding of a vertex here agreed with the
 * bytes Godot made of it; the spans and `grown` name what the last patch_ground changed.
 */
struct PlantGroundSurface {
    PlantMeshArrays                     arrays;
    std::vector<PlantGroundSlot>        slots;
    std::vector<VivariumGroundCellView> cells;
    PlantGroundLayout                   layout;
    std::vector<uint8_t>                vertex_bytes;
    std::vector<uint8_t>                attribute_bytes;
    float                               low_m = 0.0f;   // the lowest height the mesh's bounds take in
    float                               high_m = 0.0f;  // the highest
    bool                                patchable = false;
    PlantGroundSpan                     positions;      // vertex stream: the places the last patch wrote
    PlantGroundSpan                     normals;        // vertex stream: the normals it wrote
    PlantGroundSpan                     attributes;     // attribute stream: the colours and coordinates it wrote
    bool                                grown = false;  // the last patch moved a vertex past low_m or high_m
};

/**
 * Appends the ground of the scene to `out`, as the ports carried it: one vertex per terrain sample - its
 * world place and height (ase-terrain), its normal and texture coordinates (ase-render-data) - and per
 * ground cell the two triangles (sw, se, ne) and (sw, ne, nw) over the vertices its corners name. A cell
 * whose corner the terrain port did not carry is not drawn; a sample named twice is drawn once. Every sample
 * appends its slot to `slots`, in the order of the port.
 *
 * @return the cells drawn
 */
uint32_t build_ground(const std::vector<VivariumTerrainView>& samples,
                      const std::vector<VivariumGroundCellView>& cells, PlantMeshArrays& out,
                      std::vector<PlantGroundSlot>& slots);

/**
 * Gives every vertex of a ground `out` built by build_ground its colour, which the ground shader reads as
 * the cover of the ground (GODOT_GROUND_SHADER_FILE): red 1 where the vertex is levelled ground - inside a
 * disc of `discs` and within GODOT_GROUND_LEVEL_TOLERANCE_M of its height, so the disc is soil only once
 * ase-terrain levelled it -, green 1 where it lies in the bed of a brook of `beds` (no farther from the
 * line between its ends than half its width), blue 0, alpha 1. Replaces the colours `out` carried.
 *
 * @return the vertices marked levelled or bed
 */
uint32_t mark_ground(const std::vector<PlantGroundDisc>& discs, const std::vector<PlantGroundBed>& beds,
                     PlantMeshArrays& out);

/**
 * Writes vertex `vertex` of `surface.arrays` into the bytes of `surface` at the places its layout names, coded
 * as Godot codes an uncompressed surface: the place as floats, the normal as the octahedral code of
 * Vector3::octahedron_encode in two uint16 and, at a normal stride of 8, the tangent Godot derives from that
 * normal (Vector3::octahedron_tangent_encode) in two uint16 behind it, the colour as four uint8, the texture
 * coordinates as floats.
 *
 * @return false where the bytes are too short for the vertex - nothing is written then
 */
bool encode_ground(PlantGroundSurface& surface, uint32_t vertex);

/**
 * The positive control after a full build: encodes the first vertex as encode_ground would and holds it
 * against the bytes Godot made of it, the tangent behind the normal included. True only where the normal stride
 * is 4 or 8 and every byte agrees - a patch is written in Godot's code or not at all.
 */
bool ground_code_holds(const PlantGroundSurface& surface);

/**
 * Brings `surface` to the samples, cells, discs and beds of this frame by writing only what changed: the place,
 * normal and texture coordinates of every sample that moved and the colour of every vertex whose mark changed,
 * into the arrays and into the bytes, and names the spans of each stream that hold them. False - and nothing
 * written - where the surface is not patchable, the cells differ from those it was built from, or the samples
 * are not the ones it was built from in the same order; the ground is then built anew.
 */
bool patch_ground(const std::vector<VivariumTerrainView>& samples, const std::vector<VivariumGroundCellView>& cells,
                  const std::vector<PlantGroundDisc>& discs, const std::vector<PlantGroundBed>& beds,
                  PlantGroundSurface& surface);

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - BARK TUBES OF THE PLANTS
 *
 * @file        godot_plant_tubes.hpp
 * @brief       Lays the vertices of the bark port out into one mesh per body, the continuous tube of every axis
 * @description Register B0295 point 4: instead of one cylinder instance per internode the view draws one continuous
 *              tube per axis, whose knot rings parent and child share. ase-render-data builds the rings with
 *              parallel transported frames whenever a body ticked and ase-pl-flora files them on the bark port, one
 *              record per vertex with the next vertex on its ring and the vertex below it. tubes_by_body lays those
 *              records out per body: the place, the normal and (u around, v in metres along) as the port carried
 *              them, two triangles per quad between a ring and the ring below, and beside every vertex the bark
 *              share, the tiling and the sway slot of the internode its ring belongs to. Nothing is computed: no
 *              ring, no frame, no radius and no place - the view only draws.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check it without a Godot
 *              process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-13
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_bark_view.hpp>
#include <ase/adp/godot/godot_host_organ_view.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * The bark tube of one body: the mesh arrays (positions, normals, uvs with u around the ring and v in metres
 * along the axis, indices), and per vertex its GODOT_BARK_CUSTOM_VALUES twice - custom0 the bark share, the
 * tiles of the white bark around and along per metre and the tiles of the base bark per white tile; custom1
 * the sway slot of its internode, the share of the branch displacement at its ring, the radius of its ring and
 * the order of its axis - and per sway slot the GODOT_SWAY_VALUES texels of the sway data texture.
 */
struct PlantBodyTube {
    PlantMeshArrays arrays;
    std::vector<float> custom0;  // per vertex: bark share, tiles around, tiles along per metre, base tiles per white tile
    std::vector<float> custom1;  // per vertex: sway slot, sway share at its ring, ring radius in metres, axis order
    std::vector<float> custom2;  // per vertex of the knot form: the frame of its knot, qx, qy, qz, qw; empty in the vertex form
    std::vector<float> custom3;  // per vertex of the knot form: the noise key of its axis, which the bark shader spreads to o_axis; empty in the vertex form
    std::vector<float> tangents; // per vertex of the knot form: the template direction of growing u and the binormal sign; empty in the vertex form
    std::vector<float> sway;     // per sway slot: frequency, mean x and z, gust x and z, share at base and top, one spare
    std::vector<int32_t> twig_indices;  // the same vertices closed on evenly spaced corners of every ring, the coarse tube of the twigs
    std::vector<int32_t> scar_indices;  // the wood disks closing the tip ring of every stub a cut left; empty in the vertex form
};

/**
 * Lays the records of the bark port out per body (usecase:101613 show the plant bark tubes). A vertex whose
 * internode the segment port does not show is not drawn: both ports are read in the same snapshot, and the
 * bark set the host still holds while a new one is filed page by page may name internodes that are gone. A
 * vertex with a ring below and a next vertex closes the quad (below, vertex, next) and (below, next, below of
 * next); a quad with a corner that is not drawn is left out. The tip ring of an internode takes the share of its
 * sway at the top, the foot ring of an axis the share at the base of its first internode. A tip ring that the
 * continuation child closes onto takes the radius of that child (the tip radius ase-render-data gave the cone);
 * a foot ring and the tip ring at the end of an axis keep the radius of their own internode.
 *
 * The coarse tube of the twigs (register B0317) closes the same vertices on fewer corners: a ring of n quads is
 * walked from its start - the vertex no `next` of its ring names - in steps of the largest divisor s of n that
 * leaves n / s >= GODOT_TWIG_LEAST_CORNERS, and every step closes the quad (below a, a, b, below b) to the vertex b
 * s links along. A ring whose n has no such divisor closes no surface and lays no twig quad. Which internodes the
 * fine and which the coarse tube shows decides the bark shader by the order on custom1 (OrderBudget::shown).
 */
std::unordered_map<uint32_t, PlantBodyTube> tubes_by_body(const std::vector<VivariumBarkVertexView>& vertices,
                                                          const std::vector<VivariumOrganView>& internodes);

/**
 * Lays the knots of the knot and foot ports out per body (PLAN_ASE_VIVARIUM_TREEGEN T.6, usecase:101613 show the plant
 * bark tubes): the knot form. Around every knot stands one ring of the template with the corners K of its axis
 * (usecase:101644 count ring corners by size; the foot record carries K, axis_foot walks `below` down to it), K + 1
 * vertices at the centre of the knot, the last at the place of the first; the normal of corner k is the template
 * direction (cos a, 0, -sin a) with a = 2 pi k / K in the frame of the knot, custom2 carries that frame and custom1
 * the radius, so the bark shader turns the corner by the frame and sets it out by the radius - the same ring
 * ase-render-data laid (render_data_cyl_vtx_calc_sys.cpp). uv is (k / K, v of the knot). A knot whose chain reaches
 * no foot record has no K and lays no ring (tube/godot_tube_ring.hpp).
 *
 * A knot whose internode the segment port does not show is not drawn: the knot set the host still holds while a new
 * one is filed page by page may name internodes that are gone. The tube piece of an internode closes its tip ring
 * onto the ring of the knot `below` names, or onto its foot ring where `below` names the internode itself; a piece
 * whose lower ring is not drawn or stands in another body is left out. A foot ring takes the sway share at the base
 * of its internode and the radius of the foot record, a tip ring the share at the top and the radius of its knot,
 * which is already the tip radius the continuation child starts with. The coarse tube of the twigs closes the same
 * rings on every s-th corner, s the largest divisor of K that leaves at least
 * GODOT_TWIG_LEAST_CORNERS corners.
 *
 * THE BEND (usecase:101643 ring the tubes by curvature): a knot whose `rings` is above 0 lays that many intermediate
 * rings between its lower ring and its tip ring, at most ceil(K / 2), the half turn at the step 2 pi / K that
 * RenderDataCylCrvtCalcSystem bounds them by, and never above GODOT_TUBE_RING_MAX. bend_rings (tube/godot_tube_bend.hpp)
 * stands them on the Hermite curve through both knots and carries the lower frame on by the least turn; each takes
 * the radius, v and sway share of its place t linearly between the lower knot and the tip, with the bark, tiling and
 * sway slot of its internode. The quads and the coarse tube then close every ring onto the next, lower ring first.
 *
 * THE CUT FACE (PLAN_ASE_VIVARIUM_TREEGEN T.6): the tip of a stub a keeper's cut left is no cone but open wood. Every
 * cut face of the scar port whose knot is drawn in the same body closes that tip with a disk of its own vertices -
 * a rim ring laid like the tip ring (same knot, frame, radius and sway share at the top), its uv the wood texture
 * from GODOT_SCAR_UV_CENTRE out by GODOT_SCAR_UV_HALF, and a centre vertex at the knot with radius 0 and the axis as
 * its template direction - fanned into `scar_indices`, whose triangles face along the axis, out of the stub. A cut
 * face whose knot or internode is not drawn, or that names another body than its knot, lays no disk.
 */
std::unordered_map<uint32_t, PlantBodyTube> tubes_by_body(const std::vector<VivariumKnotView>& knots,
                                                          const std::vector<VivariumFootView>& feet,
                                                          const std::vector<VivariumScarView>& scars,
                                                          const std::vector<VivariumOrganView>& internodes);

}  // namespace ase::adp::godot

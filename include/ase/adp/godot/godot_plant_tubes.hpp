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
 * @modified    2026-11-14
 * @version     00.00.02.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

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
    std::vector<float> sway;     // per sway slot: frequency, mean x and z, gust x and z, share at base and top, one spare
    std::vector<int32_t> twig_indices;  // the same vertices closed on evenly spaced corners of every ring, the coarse tube of the twigs
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

}  // namespace ase::adp::godot

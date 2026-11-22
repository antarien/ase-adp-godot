#pragma once

/**
 * ASE GODOT ADAPTER - THE RING OF A BARK TUBE WITH THE CORNERS OF ITS AXIS
 *
 * @file        godot_tube_ring.hpp
 * @brief       lay_tube_ring, lay_tube_disk, axis_foot, twig_stride - one ring of K corners and the foot of its axis
 * @description PLAN_ASE_VIVARIUM_TREEGEN T.6, section "Ecken je Achse nach Dicke" (usecase:101644 count ring corners
 *              by size): ase-render-data counts the corners K of every axis once at its foot, from the corner
 *              density of the species, the foot radius and the band of the body (RenderDataCylSideCalcSystem), and
 *              lays every ring of the axis with it; the foot port carries K once per axis (key sides), and with it
 *              the key of the place of the axis in the bark noise field (key noise, "Rindenrauschen"). The view
 *              lays the ring template of every knot with the foot of its axis: K + 1 vertices, the last at the
 *              place of the first with u = 1, each with the noise key of the axis. A knot finds its foot by walking
 *              `below` down to the knot that names itself; the coarse tube of the twigs closes every s-th corner, s
 *              the largest divisor of K that leaves at least GODOT_TWIG_LEAST_CORNERS corners.
 *
 *              WHY A UNIT OF ITS OWN: the ring and its corner count are one responsibility, read by the tube
 *              layout of godot_plant_tubes.cpp, which walks the ports and closes the quads between the rings.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101644 - count ring corners by size
 * @actor       Tube Ring Divider (TUBERINDIV)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>

#include <cstdint>
#include <unordered_map>

namespace ase::adp::godot {

/**
 * One ring of the template around a knot: K + 1 vertices at its centre, K the sides of the foot of its axis, each
 * with its template direction (cos a, 0, -sin a), a = 2 pi k / K, as normal, (k / K, v) as uv, the bark of its
 * internode, its sway slot and share, the radius of the knot and the order of the axis, the knot frame and the noise
 * key of the axis. Answers the index of its first vertex.
 */
int32_t lay_tube_ring(PlantBodyTube& tube, double x, double y, double z, double radius, double qx, double qy, double qz,
                      double qw, double v, const VivariumOrganView& organ, float slot, double share,
                      const VivariumFootView& axis);

/**
 * The wood disk on the cut face of a stub: a rim ring of the corners of its axis laid like the tip ring of its knot,
 * its uv turned into the wood texture, and one centre vertex at the knot with radius 0. Answers the index of the
 * centre.
 */
int32_t lay_tube_disk(PlantBodyTube& tube, const VivariumKnotView& knot, const VivariumOrganView& organ, float slot,
                      const VivariumFootView& axis);

/**
 * The value `foot_value` holds for the foot a knot's `below` chain reaches: the corners K of that foot, or its place
 * in the list of feet plus one, as the caller fills the map. Every knot walked on the way takes the same value into
 * `known`; 0 where the chain breaks before a foot: no foot, no ring.
 */
uint32_t axis_foot(uint32_t object_id, const std::unordered_map<uint32_t, const VivariumKnotView*>& knot_by_id,
                   const std::unordered_map<uint32_t, uint32_t>& foot_value,
                   std::unordered_map<uint32_t, uint32_t>& known);

/** The step of the coarse tube on a ring of `sides` corners: the largest divisor leaving GODOT_TWIG_LEAST_CORNERS. */
uint32_t twig_stride(uint32_t sides);

}  // namespace ase::adp::godot

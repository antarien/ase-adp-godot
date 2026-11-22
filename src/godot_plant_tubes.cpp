/**
 * ASE GODOT ADAPTER - BARK TUBES OF THE PLANTS
 *
 * @file        godot_plant_tubes.cpp
 * @brief       Lays the vertices of the bark port out into one mesh per body, the continuous tube of every axis
 * @description See godot_plant_tubes.hpp.
 *
 *              THE RINGS ARE THE PORT'S: every vertex arrives with the place, the normal and the texture
 *              coordinates ase-render-data computed (render_data_cyl_vtx_calc_sys.cpp), the tip ring of one
 *              internode being the foot ring of the next. The quads are closed by the links the port carried -
 *              the next vertex on the ring and the vertex below - so no ring is searched by place and no seam is
 *              guessed: two internodes that share a knot ring share its vertices.
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

#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/tube/godot_tube_bend.hpp>
#include <ase/adp/godot/tube/godot_tube_ring.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

std::unordered_map<uint32_t, PlantBodyTube> tubes_by_body(const std::vector<VivariumBarkVertexView>& vertices,
                                                          const std::vector<VivariumOrganView>& internodes) {
    // where a drawn vertex stands: the body whose mesh holds it, its index there and the vertex below it
    struct DrawnVertex {
        uint32_t body;
        int32_t index;
        uint32_t below;
    };
    // the segment record of every internode: a port names each internode once
    std::unordered_map<uint32_t, std::size_t> internode_at;
    internode_at.reserve(internodes.size());
    for (std::size_t i = 0u; i < internodes.size(); ++i) {
        internode_at.emplace(internodes[i].object_id, i);
    }
    // the internode of the ring above every vertex: a tip ring that a continuation child closes onto takes the
    // radius of that child (render_data_cyl_cone_calc_sys.cpp, tip_radius); a foot ring and the tip ring at the
    // end of an axis keep the radius of their own internode
    std::unordered_map<uint32_t, uint32_t> organ_above;
    organ_above.reserve(vertices.size());
    for (const VivariumBarkVertexView& vertex : vertices) {
        if (vertex.below != GODOT_LINK_NONE) {
            organ_above.emplace(vertex.below, vertex.organ);
        }
    }
    std::unordered_map<uint32_t, PlantBodyTube> bodies;
    std::unordered_map<uint32_t, DrawnVertex> drawn;
    drawn.reserve(vertices.size());
    std::unordered_map<uint32_t, float> slot_of;
    for (const VivariumBarkVertexView& vertex : vertices) {
        const auto found = internode_at.find(vertex.organ);
        if (found == internode_at.end()) {
            continue;
        }
        const VivariumOrganView& organ = internodes[found->second];
        PlantBodyTube& tube = bodies[vertex.body];
        // the sway slot of the internode in its body: the first of its vertices hands its sway to the texture
        auto slot = slot_of.find(organ.object_id);
        if (slot == slot_of.end()) {
            const std::size_t slots = tube.sway.size() / GODOT_SWAY_VALUES;
            slot = slot_of.emplace(organ.object_id, static_cast<float>(slots)).first;
            tube.sway.push_back(static_cast<float>(organ.sway_freq));
            tube.sway.push_back(static_cast<float>(organ.sway_mean_x));
            tube.sway.push_back(static_cast<float>(organ.sway_mean_z));
            tube.sway.push_back(static_cast<float>(organ.sway_gust_x));
            tube.sway.push_back(static_cast<float>(organ.sway_gust_z));
            tube.sway.push_back(static_cast<float>(organ.sway_base));
            tube.sway.push_back(static_cast<float>(organ.sway_top));
            tube.sway.push_back(static_cast<float>(GODOT_SWAY_NONE));
        }
        const int32_t index = static_cast<int32_t>(tube.arrays.positions.size() / 3u);
        drawn.emplace(vertex.object_id, DrawnVertex{vertex.body, index, vertex.below});
        tube.arrays.positions.push_back(static_cast<float>(vertex.x));
        tube.arrays.positions.push_back(static_cast<float>(vertex.y));
        tube.arrays.positions.push_back(static_cast<float>(vertex.z));
        tube.arrays.normals.push_back(static_cast<float>(vertex.nx));
        tube.arrays.normals.push_back(static_cast<float>(vertex.ny));
        tube.arrays.normals.push_back(static_cast<float>(vertex.nz));
        tube.arrays.uvs.push_back(static_cast<float>(vertex.u));
        tube.arrays.uvs.push_back(static_cast<float>(vertex.v));
        // v runs in metres, the tile record counts the tiles along the whole internode: per metre of it
        const double along = organ.length > 0.0 ? organ.tile_along / organ.length : organ.tile_along;
        tube.custom0.push_back(static_cast<float>(organ.bark));
        tube.custom0.push_back(static_cast<float>(organ.tile_around));
        tube.custom0.push_back(static_cast<float>(along));
        tube.custom0.push_back(static_cast<float>(organ.tile_base));
        // a foot ring stands at the base of the first internode of its axis, a tip ring at the top of its own
        const double share = vertex.below == GODOT_LINK_NONE ? organ.sway_base : organ.sway_top;
        // the radius of the ring and the order of its axis: the shader draws a ring thinner than a pixel and an
        // order the budget does not show onto the axis, so its quads close to nothing
        double radius = organ.radius;
        const auto above = organ_above.find(vertex.object_id);
        if (above != organ_above.end()) {
            const auto child = internode_at.find(above->second);
            if (child != internode_at.end()) {
                radius = internodes[child->second].radius;
            }
        }
        tube.custom1.push_back(slot->second);
        tube.custom1.push_back(static_cast<float>(share));
        tube.custom1.push_back(static_cast<float>(radius));
        tube.custom1.push_back(static_cast<float>(organ.order));
    }
    // two triangles per quad between a ring and the ring below, closed by the links the port carried
    for (const VivariumBarkVertexView& vertex : vertices) {
        if (vertex.next == GODOT_LINK_NONE || vertex.below == GODOT_LINK_NONE) {
            continue;
        }
        const auto at = drawn.find(vertex.object_id);
        const auto next = drawn.find(vertex.next);
        const auto below = drawn.find(vertex.below);
        if (at == drawn.end() || next == drawn.end() || below == drawn.end() ||
            next->second.below == GODOT_LINK_NONE) {
            continue;
        }
        const auto next_below = drawn.find(next->second.below);
        if (next_below == drawn.end() || next->second.body != vertex.body || below->second.body != vertex.body ||
            next_below->second.body != vertex.body) {
            continue;
        }
        std::vector<int32_t>& indices = bodies[vertex.body].arrays.indices;
        indices.push_back(below->second.index);
        indices.push_back(at->second.index);
        indices.push_back(next->second.index);
        indices.push_back(below->second.index);
        indices.push_back(next->second.index);
        indices.push_back(next_below->second.index);
    }
    // the coarse tube of the twigs: every ring above another, walked from its start on evenly spaced corners
    std::unordered_map<uint32_t, uint32_t> next_of;
    next_of.reserve(vertices.size());
    std::unordered_map<uint32_t, uint32_t> named_by_next;
    named_by_next.reserve(vertices.size());
    for (const VivariumBarkVertexView& vertex : vertices) {
        next_of.emplace(vertex.object_id, vertex.next);
        if (vertex.next != GODOT_LINK_NONE) {
            named_by_next.emplace(vertex.next, vertex.object_id);
        }
    }
    std::vector<uint32_t> chain;
    for (const VivariumBarkVertexView& vertex : vertices) {
        if (vertex.below == GODOT_LINK_NONE || vertex.next == GODOT_LINK_NONE ||
            named_by_next.count(vertex.object_id) != 0u) {
            continue;
        }
        chain.clear();
        for (uint32_t at = vertex.object_id; at != GODOT_LINK_NONE && chain.size() <= next_of.size();) {
            chain.push_back(at);
            const auto link = next_of.find(at);
            at = link == next_of.end() ? GODOT_LINK_NONE : link->second;
        }
        // n quads around the ring; the step is the largest divisor of n that still leaves the least corners
        const std::size_t quads = chain.size() - 1u;
        std::size_t step = 0u;
        for (std::size_t divisor = 1u; divisor <= quads; ++divisor) {
            if (quads % divisor == 0u && quads / divisor >= GODOT_TWIG_LEAST_CORNERS) {
                step = divisor;
            }
        }
        if (step == 0u) {
            continue;
        }
        for (std::size_t corner = 0u; corner + step <= quads; corner += step) {
            const auto at = drawn.find(chain[corner]);
            const auto along = drawn.find(chain[corner + step]);
            if (at == drawn.end() || along == drawn.end() || at->second.below == GODOT_LINK_NONE ||
                along->second.below == GODOT_LINK_NONE) {
                continue;
            }
            const auto at_below = drawn.find(at->second.below);
            const auto along_below = drawn.find(along->second.below);
            if (at_below == drawn.end() || along_below == drawn.end() || at->second.body != vertex.body ||
                along->second.body != vertex.body || at_below->second.body != vertex.body ||
                along_below->second.body != vertex.body) {
                continue;
            }
            std::vector<int32_t>& twigs = bodies[vertex.body].twig_indices;
            twigs.push_back(at_below->second.index);
            twigs.push_back(at->second.index);
            twigs.push_back(along->second.index);
            twigs.push_back(at_below->second.index);
            twigs.push_back(along->second.index);
            twigs.push_back(along_below->second.index);
        }
    }
    return bodies;
}

namespace {

/** The sway slot of an internode in its body: the first ring of it hands its sway to the texture of the body. */
float sway_slot_of(PlantBodyTube& tube, std::unordered_map<uint32_t, float>& slot_of, const VivariumOrganView& organ) {
    const auto found = slot_of.find(organ.object_id);
    if (found != slot_of.end()) {
        return found->second;
    }
    const std::size_t slots = tube.sway.size() / GODOT_SWAY_VALUES;  // whole slots held so far: the index of the next
    const float slot = static_cast<float>(slots);
    slot_of.emplace(organ.object_id, slot);
    tube.sway.push_back(static_cast<float>(organ.sway_freq));
    tube.sway.push_back(static_cast<float>(organ.sway_mean_x));
    tube.sway.push_back(static_cast<float>(organ.sway_mean_z));
    tube.sway.push_back(static_cast<float>(organ.sway_gust_x));
    tube.sway.push_back(static_cast<float>(organ.sway_gust_z));
    tube.sway.push_back(static_cast<float>(organ.sway_base));
    tube.sway.push_back(static_cast<float>(organ.sway_top));
    tube.sway.push_back(static_cast<float>(GODOT_SWAY_NONE));
    return slot;
}

}  // namespace

std::unordered_map<uint32_t, PlantBodyTube> tubes_by_body(const std::vector<VivariumKnotView>& knots,
                                                          const std::vector<VivariumFootView>& feet,
                                                          const std::vector<VivariumScarView>& scars,
                                                          const std::vector<VivariumOrganView>& internodes) {
    // where a drawn ring stands: the body whose mesh holds it and the index of its first vertex there
    struct DrawnRing {
        uint32_t body;
        int32_t first;
    };
    std::unordered_map<uint32_t, std::size_t> internode_at;
    internode_at.reserve(internodes.size());
    for (std::size_t i = 0u; i < internodes.size(); ++i) {
        internode_at.emplace(internodes[i].object_id, i);
    }
    std::unordered_map<uint32_t, PlantBodyTube> bodies;
    std::unordered_map<uint32_t, float> slot_of;
    // the place of every foot record in `feet` plus one, and every knot of the ports by its internode: a knot walks
    // `below` down to its foot for the corners K (usecase:101644 count ring corners by size) and the noise key of its
    // axis, drawn or not, as the axis is one whether the view shows all its internodes or not
    std::unordered_map<uint32_t, uint32_t> foot_place;
    foot_place.reserve(feet.size());
    for (std::size_t i = 0u; i < feet.size(); ++i) {
        foot_place.emplace(feet[i].object_id, static_cast<uint32_t>(i + 1u));
    }
    std::unordered_map<uint32_t, const VivariumKnotView*> knot_by_id;
    knot_by_id.reserve(knots.size());
    for (const VivariumKnotView& knot : knots) {
        knot_by_id.emplace(knot.object_id, &knot);
    }
    std::unordered_map<uint32_t, uint32_t> place_at;
    place_at.reserve(knots.size());
    // the foot ring of every axis: the lower ring of its first internode, at the share of its sway at the base
    std::unordered_map<uint32_t, DrawnRing> foot_ring;
    foot_ring.reserve(feet.size());
    // the foot knot whose ring is drawn, by its internode: the lower knot of a bent first piece (usecase:101643)
    std::unordered_map<uint32_t, const VivariumFootView*> foot_at;
    foot_at.reserve(feet.size());
    for (const VivariumFootView& foot : feet) {
        const auto found = internode_at.find(foot.object_id);
        if (found == internode_at.end()) {
            continue;
        }
        const VivariumOrganView& organ = internodes[found->second];
        PlantBodyTube& tube = bodies[foot.body];
        const float slot = sway_slot_of(tube, slot_of, organ);
        const int32_t first = lay_tube_ring(tube, foot.x, foot.y, foot.z, foot.radius, foot.qx, foot.qy, foot.qz,
                                            foot.qw, foot.v, organ, slot, organ.sway_base, foot);
        foot_ring.emplace(foot.object_id, DrawnRing{foot.body, first});
        foot_at.emplace(foot.object_id, &foot);
    }
    // the tip ring of every internode, at the share of its sway at the top
    std::unordered_map<uint32_t, DrawnRing> tip_ring;
    tip_ring.reserve(knots.size());
    // the knot whose tip ring is drawn, by its internode: a cut face lays its disk on that knot
    std::unordered_map<uint32_t, const VivariumKnotView*> knot_at;
    knot_at.reserve(knots.size());
    for (const VivariumKnotView& knot : knots) {
        const auto found = internode_at.find(knot.object_id);
        const uint32_t place = axis_foot(knot.object_id, knot_by_id, foot_place, place_at);
        if (found == internode_at.end() || place == 0u) {
            continue;
        }
        const VivariumOrganView& organ = internodes[found->second];
        PlantBodyTube& tube = bodies[knot.body];
        const float slot = sway_slot_of(tube, slot_of, organ);
        const int32_t first = lay_tube_ring(tube, knot.x, knot.y, knot.z, knot.radius, knot.qx, knot.qy, knot.qz,
                                            knot.qw, knot.v, organ, slot, organ.sway_top, feet[place - 1u]);
        tip_ring.emplace(knot.object_id, DrawnRing{knot.body, first});
        knot_at.emplace(knot.object_id, &knot);
    }
    // the wood disk on the tip of every stub whose knot is drawn in the body its cut face names
    for (const VivariumScarView& scar : scars) {
        const auto drawn = knot_at.find(scar.object_id);
        const auto found = internode_at.find(scar.object_id);
        if (drawn == knot_at.end() || found == internode_at.end() || drawn->second->body != scar.body) {
            continue;
        }
        const VivariumOrganView& organ = internodes[found->second];
        PlantBodyTube& tube = bodies[scar.body];
        (void)lay_tube_disk(tube, *drawn->second, organ, sway_slot_of(tube, slot_of, organ),
                            feet[place_at.at(scar.object_id) - 1u]);
    }
    // two triangles per corner K of the axis between consecutive rings of every internode: the lower ring, the
    // intermediate rings of its bend (usecase:101643 ring the tubes by curvature) and its tip ring; the coarse tube
    // of the twigs closes every twig_stride(K)-th corner
    for (const VivariumKnotView& knot : knots) {
        const auto upper = tip_ring.find(knot.object_id);
        if (upper == tip_ring.end()) {
            continue;
        }
        const bool at_foot = knot.below == knot.object_id;
        const auto lower = at_foot ? foot_ring.find(knot.object_id) : tip_ring.find(knot.below);
        if (lower == (at_foot ? foot_ring.end() : tip_ring.end()) || lower->second.body != knot.body) {
            continue;
        }
        PlantBodyTube& tube = bodies[knot.body];
        // the tip ring stands, so the foot of its axis does: every ring of the axis has its K and its noise key, the
        // lower ring included
        const VivariumFootView& axis = feet[place_at.at(knot.object_id) - 1u];
        const uint32_t sides = axis.sides;
        // half a turn bounds the rings of a bend, ceil(K / 2) at the step 2 pi / K (RenderDataCylCrvtCalcSystem)
        const uint32_t most = (sides + 1u) / 2u < GODOT_TUBE_RING_MAX ? (sides + 1u) / 2u : GODOT_TUBE_RING_MAX;
        const uint32_t rings = knot.rings < most ? knot.rings : most;
        int32_t chain[GODOT_TUBE_RING_MAX + 2u] = {};
        chain[0] = lower->second.first;
        chain[rings + 1u] = upper->second.first;
        if (rings > 0u) {
            // the lower knot as a knot view: the foot of the axis, or the tip knot of the internode below
            VivariumKnotView below_knot;
            if (at_foot) {
                const VivariumFootView& base = *foot_at.at(knot.object_id);
                below_knot.x = base.x;
                below_knot.y = base.y;
                below_knot.z = base.z;
                below_knot.radius = base.radius;
                below_knot.qx = base.qx;
                below_knot.qy = base.qy;
                below_knot.qz = base.qz;
                below_knot.qw = base.qw;
                below_knot.v = base.v;
            } else {
                below_knot = *knot_at.at(knot.below);
            }
            const VivariumOrganView& organ = internodes[internode_at.at(knot.object_id)];
            const float slot = sway_slot_of(tube, slot_of, organ);
            TubeBendRing bend[GODOT_TUBE_RING_MAX] = {};
            bend_rings(below_knot, knot, rings, bend);
            for (uint32_t i = 0u; i < rings; ++i) {
                const double t = static_cast<double>(bend[i].t);
                chain[i + 1u] = lay_tube_ring(tube, bend[i].x, bend[i].y, bend[i].z,
                                              below_knot.radius + t * (knot.radius - below_knot.radius), bend[i].qx,
                                              bend[i].qy, bend[i].qz, bend[i].qw,
                                              below_knot.v + t * (knot.v - below_knot.v), organ, slot,
                                              organ.sway_base + t * (organ.sway_top - organ.sway_base), axis);
            }
        }
        const int32_t stride = static_cast<int32_t>(twig_stride(sides));
        for (uint32_t piece = 0u; piece <= rings; ++piece) {
            const int32_t low = chain[piece];
            const int32_t up = chain[piece + 1u];
            for (int32_t k = 0; k < static_cast<int32_t>(sides); ++k) {
                tube.arrays.indices.push_back(low + k);
                tube.arrays.indices.push_back(up + k);
                tube.arrays.indices.push_back(up + k + 1);
                tube.arrays.indices.push_back(low + k);
                tube.arrays.indices.push_back(up + k + 1);
                tube.arrays.indices.push_back(low + k + 1);
            }
            for (int32_t k = 0; stride > 0 && k + stride <= static_cast<int32_t>(sides); k += stride) {
                tube.twig_indices.push_back(low + k);
                tube.twig_indices.push_back(up + k);
                tube.twig_indices.push_back(up + k + stride);
                tube.twig_indices.push_back(low + k);
                tube.twig_indices.push_back(up + k + stride);
                tube.twig_indices.push_back(low + k + stride);
            }
        }
    }
    return bodies;
}

}  // namespace ase::adp::godot

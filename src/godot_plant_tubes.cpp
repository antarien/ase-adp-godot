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
 * @modified    2026-11-14
 * @version     00.00.02.00000
 */

#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/types.hpp>

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

}  // namespace ase::adp::godot

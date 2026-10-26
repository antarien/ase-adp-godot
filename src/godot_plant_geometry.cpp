/**
 * ASE GODOT ADAPTER - PLANT GEOMETRY IMPLEMENTATION
 *
 * @file        godot_plant_geometry.cpp
 * @brief       Templates, ground and instance buffers of the plants, as plain arrays
 * @description See godot_plant_geometry.hpp. Every organ and every template corner arrives as the record
 *              its ports reported; the functions here sort the records and change none of their numbers.
 *
 *              THE WINDING IS THE PORT'S: Godot takes the CLOCKWISE triangle of a mesh, seen from outside,
 *              as its front face, and ase-render-data lays every template triangle in that order
 *              (RenderDataTmplCylCalcSystem, RenderDataTmplLeafCalcSystem). The corners go to the index
 *              buffer in the order the triangle record names them.
 *
 *              THE MATRIX IS THE PORT'S: every drawn organ arrives with the 3x4 ase-render-data built
 *              from its quaternion and its size (render_data_part_mtrx_rad_sys.hpp,
 *              render_data_part_mtrx_unif_sys.hpp) - the template's y along the organ, scaled to its
 *              length, x along its roll axis and z their right-handed completion. No axis and no scale
 *              is chosen here, so a leaf turns about its stalk the way the phyllotaxis turned it
 *              (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11, step 2).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-25
 * @version     00.00.07.00000
 */

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

void push3(std::vector<float>& out, float a, float b, float c) {
    out.push_back(a);
    out.push_back(b);
    out.push_back(c);
}

}  // namespace

std::unordered_map<uint32_t, PlantBodyRows> rows_by_body(const std::vector<VivariumInstanceView>& instances,
                                                         const std::vector<VivariumOrganView>& organs,
                                                         double split_order) {
    // the organ record of every object id: a port names each organ once
    std::unordered_map<uint32_t, std::size_t> organ_at;
    organ_at.reserve(organs.size());
    for (std::size_t o = 0u; o < organs.size(); ++o) {
        organ_at.emplace(organs[o].object_id, o);
    }
    std::unordered_map<uint32_t, PlantBodyRows> bodies;
    for (const VivariumInstanceView& instance : instances) {
        const auto found = organ_at.find(instance.object_id);
        if (found == organ_at.end()) {
            continue;
        }
        const VivariumOrganView& organ = organs[found->second];
        PlantBodyRows& rows = bodies[organ.body];
        const bool above = organ.order > split_order;
        std::vector<float>& side = above ? rows.above : rows.up_to;
        side.insert(side.end(), instance.rows, instance.rows + GODOT_INSTANCE_VALUES);
        // the custom data of the same instance: the bark share, then the bark tiling of its section
        std::vector<float>& custom = above ? rows.above_custom : rows.up_to_custom;
        custom.push_back(static_cast<float>(organ.bark));
        custom.push_back(static_cast<float>(organ.tile_around));
        custom.push_back(static_cast<float>(organ.tile_along));
        custom.push_back(static_cast<float>(organ.tile_base));
    }
    return bodies;
}

uint32_t build_template(const std::vector<VivariumTemplateCornerView>& corners,
                        const std::vector<VivariumTemplateTriangleView>& triangles, uint32_t code,
                        PlantMeshArrays& out) {
    // one vertex per corner of the template, in the order of the port; its slot by index
    std::unordered_map<uint32_t, int32_t> slot;
    for (const VivariumTemplateCornerView& corner : corners) {
        if (corner.code != code) {
            continue;
        }
        const auto vertex = static_cast<int32_t>(out.positions.size() / 3u);
        if (!slot.emplace(corner.index, vertex).second) {
            continue;
        }
        push3(out.positions, static_cast<float>(corner.x), static_cast<float>(corner.y), static_cast<float>(corner.z));
        push3(out.normals, static_cast<float>(corner.nx), static_cast<float>(corner.ny), static_cast<float>(corner.nz));
        out.uvs.push_back(static_cast<float>(corner.u));
        out.uvs.push_back(static_cast<float>(corner.v));
    }
    // per triangle of the template its three corners, in the order the record names them (ase-render-data)
    uint32_t drawn = 0u;
    for (const VivariumTemplateTriangleView& triangle : triangles) {
        if (triangle.code != code) {
            continue;
        }
        const auto a = slot.find(triangle.a);
        const auto b = slot.find(triangle.b);
        const auto c = slot.find(triangle.c);
        if (a == slot.end() || b == slot.end() || c == slot.end()) {
            continue;
        }
        out.indices.insert(out.indices.end(), {a->second, b->second, c->second});
        drawn += 1u;
    }
    return drawn;
}

uint32_t build_ground(const std::vector<VivariumTerrainView>& samples,
                      const std::vector<VivariumGroundCellView>& cells, PlantMeshArrays& out) {
    // one vertex per sample, in the order of the port; its slot by object id
    std::unordered_map<uint32_t, int32_t> slot;
    slot.reserve(samples.size());
    for (const VivariumTerrainView& sample : samples) {
        const auto vertex = static_cast<int32_t>(out.positions.size() / 3u);
        if (!slot.emplace(sample.object_id, vertex).second) {
            continue;
        }
        push3(out.positions, static_cast<float>(sample.x), static_cast<float>(sample.height),
              static_cast<float>(sample.z));
        push3(out.normals, static_cast<float>(sample.nx), static_cast<float>(sample.ny), static_cast<float>(sample.nz));
        out.uvs.push_back(static_cast<float>(sample.u));
        out.uvs.push_back(static_cast<float>(sample.v));
    }
    // per cell the two triangles in the order the cell names its corners (ase-render-data)
    uint32_t drawn = 0u;
    for (const VivariumGroundCellView& cell : cells) {
        const auto sw = slot.find(cell.sw);
        const auto se = slot.find(cell.se);
        const auto ne = slot.find(cell.ne);
        const auto nw = slot.find(cell.nw);
        if (sw == slot.end() || se == slot.end() || ne == slot.end() || nw == slot.end()) {
            continue;
        }
        out.indices.insert(out.indices.end(),
                           {sw->second, se->second, ne->second, sw->second, ne->second, nw->second});
        drawn += 1u;
    }
    return drawn;
}

uint32_t mark_ground(const std::vector<PlantGroundDisc>& discs, const std::vector<PlantGroundBed>& beds,
                     PlantMeshArrays& out) {
    const std::size_t vertices = out.positions.size() / 3u;
    out.colors.assign(vertices * 4u, 0.0f);
    uint32_t marked = 0u;
    for (std::size_t v = 0u; v < vertices; ++v) {
        const float x = out.positions[v * 3u];
        const float y = out.positions[v * 3u + 1u];
        const float z = out.positions[v * 3u + 2u];
        // levelled: inside the disc and at its height - a disc ase-terrain has not levelled yet stays grass
        bool levelled = false;
        for (const PlantGroundDisc& disc : discs) {
            const float dx = x - disc.x;
            const float dz = z - disc.z;
            levelled = levelled || (dx * dx + dz * dz <= disc.radius * disc.radius &&
                                    std::fabs(y - disc.height) <= GODOT_GROUND_LEVEL_TOLERANCE_M);
        }
        // bed: no farther from the line between the two ends than half the width
        bool bed = false;
        for (const PlantGroundBed& brook : beds) {
            const float ax = brook.end_x - brook.x;
            const float az = brook.end_z - brook.z;
            const float length2 = ax * ax + az * az;
            const float along = length2 > 0.0f ? ((x - brook.x) * ax + (z - brook.z) * az) / length2 : 0.0f;
            const float t = along < 0.0f ? 0.0f : (along > 1.0f ? 1.0f : along);
            const float dx = x - (brook.x + t * ax);
            const float dz = z - (brook.z + t * az);
            bed = bed || dx * dx + dz * dz <= brook.half_m * brook.half_m;
        }
        out.colors[v * 4u] = levelled ? 1.0f : 0.0f;
        out.colors[v * 4u + 1u] = bed ? 1.0f : 0.0f;
        out.colors[v * 4u + 3u] = 1.0f;
        marked += (levelled || bed) ? 1u : 0u;
    }
    return marked;
}

std::unordered_map<uint32_t, PlantOrigin> plant_origins(const std::vector<VivariumPatchView>& patches) {
    // the stem base as the patch record carried it; a value it did not carry is NaN (read_snapshot)
    std::unordered_map<uint32_t, PlantOrigin> origins;
    for (const VivariumPatchView& patch : patches) {
        if (!std::isfinite(patch.x) || !std::isfinite(patch.y) || !std::isfinite(patch.z)) {
            continue;
        }
        origins[patch.object_id] =
            PlantOrigin{static_cast<float>(patch.x), static_cast<float>(patch.y), static_cast<float>(patch.z)};
    }
    return origins;
}

std::unordered_map<uint32_t, PlantStand> plant_stands(const std::vector<VivariumPatchView>& patches) {
    // soil water and condition as the patch record carried them; a value it did not carry is NaN or
    // GODOT_CODE_UNKNOWN (read_snapshot) and stays unknown
    std::unordered_map<uint32_t, PlantStand> stands;
    for (const VivariumPatchView& patch : patches) {
        PlantStand stand;
        stand.soil_known = std::isfinite(patch.soil_rel);
        stand.soil_rel = stand.soil_known ? static_cast<float>(patch.soil_rel) : 0.0f;
        stand.condition_known = patch.condition < GODOT_COND_COUNT;
        stand.condition = stand.condition_known ? static_cast<float>(patch.condition) : 0.0f;
        stands[patch.object_id] = stand;
    }
    return stands;
}

}  // namespace ase::adp::godot

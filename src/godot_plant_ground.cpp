/**
 * ASE GODOT ADAPTER - GROUND AROUND THE PLANT
 *
 * @file        godot_plant_ground.cpp
 * @brief       The ground mesh from the terrain and ground cell ports and the marks its shader paints by
 * @description See godot_plant_ground.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Application Layer Perception Control (ALPCTL)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

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
        append3(out.positions, static_cast<float>(sample.x), static_cast<float>(sample.height),
                static_cast<float>(sample.z));
        append3(out.normals, static_cast<float>(sample.nx), static_cast<float>(sample.ny), static_cast<float>(sample.nz));
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

}  // namespace ase::adp::godot

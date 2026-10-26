/**
 * ASE GODOT ADAPTER - ORGAN TEMPLATES OF THE PLANTS
 *
 * @file        godot_plant_templates.cpp
 * @brief       Builds one organ template from the corner and triangle records of the template ports
 * @description See godot_plant_templates.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101342 - hand organ shapes to app
 * @actor       Application Layer Perception Control (ALPCTL)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_templates.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

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
        append3(out.positions, static_cast<float>(corner.x), static_cast<float>(corner.y), static_cast<float>(corner.z));
        append3(out.normals, static_cast<float>(corner.nx), static_cast<float>(corner.ny), static_cast<float>(corner.nz));
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

}  // namespace ase::adp::godot

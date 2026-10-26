/**
 * ASE GODOT ADAPTER - PLANT GEOMETRY IMPLEMENTATION
 *
 * @file        godot_plant_geometry.cpp
 * @brief       The arrays of a mesh surface and where each plant stands, as plain numbers
 * @description See godot_plant_geometry.hpp. Every record arrives as its port reported it; the functions here
 *              change none of its numbers.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-26
 * @version     00.00.08.00000
 */

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

void append3(std::vector<float>& out, float a, float b, float c) {
    out.push_back(a);
    out.push_back(b);
    out.push_back(c);
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

/**
 * ASE GODOT ADAPTER - INSTANCE BUFFERS OF THE PLANTS
 *
 * @file        godot_plant_instances.cpp
 * @brief       Sorts the records of an instance port by body and order into the buffers MultiMesh takes
 * @description See godot_plant_instances.hpp.
 *
 *              THE MATRIX IS THE PORT'S: every drawn organ arrives with the 3x4 ase-render-data built from its
 *              quaternion and its size (render_data_part_mtrx_rad_sys.hpp, render_data_part_mtrx_unif_sys.hpp) -
 *              the template's y along the organ, scaled to its length, x along its roll axis and z their
 *              right-handed completion. No axis and no scale is chosen here, so a leaf turns about its stalk the
 *              way the phyllotaxis turned it (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11, step 2).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101348 - hand organ instances to app
 * @actor       Plant Instance Perception Control (PLNTINST)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

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

}  // namespace ase::adp::godot

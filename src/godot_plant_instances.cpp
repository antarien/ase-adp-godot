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
 * @actor       Organ Instance Courier (ORGNINST)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/leaf/types.hpp>

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
        // the custom data of the same instance: the bend of its leaf card across and along, its colour offset, then its
        // atlas cell and cells packed into the fourth value Godot carries as cell * GODOT_ATLS_PACK + (cells - 1), 0
        // for an organ without an atlas record (the bark and its tiling reach the tube on CUSTOM0 of its vertices)
        std::vector<float>& custom = above ? rows.above_custom : rows.up_to_custom;
        custom.push_back(static_cast<float>(organ.bend_across));
        custom.push_back(static_cast<float>(organ.bend_along));
        custom.push_back(static_cast<float>(organ.tint_offset));
        custom.push_back(static_cast<float>(organ.atlas_cell * GODOT_ATLS_PACK + (organ.atlas_cells - GODOT_ATLS_CELLS_NONE)));
        // the sway of the same instance, as the port carried it: the texels of the sway data texture
        std::vector<float>& sway = above ? rows.above_sway : rows.up_to_sway;
        sway.push_back(static_cast<float>(organ.sway_freq));
        sway.push_back(static_cast<float>(organ.sway_mean_x));
        sway.push_back(static_cast<float>(organ.sway_mean_z));
        sway.push_back(static_cast<float>(organ.sway_gust_x));
        sway.push_back(static_cast<float>(organ.sway_gust_z));
        sway.push_back(static_cast<float>(organ.sway_base));
        sway.push_back(static_cast<float>(organ.sway_top));
        sway.push_back(static_cast<float>(GODOT_SWAY_NONE));
    }
    return bodies;
}

}  // namespace ase::adp::godot

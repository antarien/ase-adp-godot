#pragma once

/**
 * ASE GODOT ADAPTER - INSTANCE BUFFERS OF THE PLANTS
 *
 * @file        godot_plant_instances.hpp
 * @brief       Sorts the records of an instance port by body and order into the buffers MultiMesh takes
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 4 and 03.11 step 2: every organ the view draws is an
 *              instance of a template, and its 3x4 is the one ase-render-data computed - the view copies it.
 *              rows_by_body sorts the records of one instance port by the body and the order their organ
 *              record names and puts beside each its custom data, the bark share and tiling. Nothing is
 *              computed: no ring, no frame, no scale and no place - the values of a record go to the buffer as
 *              the port carried them.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check it without a Godot
 *              process.
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

#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * One body's instance records of one organ class, split at an order: the twelve values of each record
 * as the port carried them, record after record in the order the port listed them - the buffer
 * MultiMesh::set_buffer takes for TRANSFORM_3D (basis row 0 and origin x, basis row 1 and origin y,
 * basis row 2 and origin z).
 */
struct PlantBodyRows {
    std::vector<float> up_to;         // the organs of an order up to the split
    std::vector<float> above;         // the organs of an order above it
    std::vector<float> up_to_custom;  // per organ of up_to its GODOT_INSTANCE_CUSTOM_VALUES: the bark share, then the tiles around, along and of the base bark
    std::vector<float> above_custom;  // per organ of above its GODOT_INSTANCE_CUSTOM_VALUES: the bark share, then the tiles around, along and of the base bark
};

/**
 * Sorts the records of one instance port by the body and the order their organ record names - the
 * organ record of the same object id carries both (VivariumInstanceView) - and beside each record its
 * custom data: the bark share its organ record carries (usecase:100395 age the bark; 0 for an organ
 * class without bark), then the bark tiling of its section, up to GODOT_INSTANCE_CUSTOM_VALUES. A record
 * without an organ record is not drawn: both ports are read in the same snapshot, so the gap is a broken
 * port, never an organ of an unknown plant. GODOT_ORDER_EVERY as `split_order` puts every organ above it -
 * a class the order budget does not split.
 */
std::unordered_map<uint32_t, PlantBodyRows> rows_by_body(const std::vector<VivariumInstanceView>& instances,
                                                         const std::vector<VivariumOrganView>& organs,
                                                         double split_order);

}  // namespace ase::adp::godot

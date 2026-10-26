#pragma once

/**
 * ASE GODOT ADAPTER - PLANT GEOMETRY
 *
 * @file        godot_plant_geometry.hpp
 * @brief       The arrays of a mesh surface and where each plant stands, as plain numbers
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 4 and 03.11 step 2: the arrays every mesh of the
 *              care view is handed to ArrayMesh in (PlantMeshArrays), the stem base and the stand of every
 *              plant taken from its patch record. The templates stand in godot_plant_templates.hpp, the ground
 *              in godot_plant_ground.hpp, the instance buffers in godot_plant_instances.hpp.
 *
 *              WHERE A PLANT STANDS: an instance's origin is plant-local (origin at the stem base on the
 *              ground). The scene stands each plant as one node at the origin of its body - the stem base its
 *              patch record carries (plant_origins: the patch centre and the height ase-terrain levelled it
 *              to) - and a body without an origin is not drawn: a plant whose ground is not levelled yet is no
 *              plant to draw at the world's origin. The root is not drawn: it lies in the ground of the patch
 *              (03.1 point 4), and it has no instance port.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check it without a Godot
 *              process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-26
 * @version     00.00.07.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * The arrays of one mesh surface: three floats per position and normal, two per uv, three indices per
 * triangle, and four floats per colour where a surface carries vertex colours (the ground: mark_ground).
 */
struct PlantMeshArrays {
    std::vector<float>   positions;
    std::vector<float>   normals;
    std::vector<float>   uvs;
    std::vector<int32_t> indices;
    std::vector<float>   colors;
};

/** Where the stem base of the plant on one patch stands in the scene, metres. */
struct PlantOrigin {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

/**
 * What the materials of a patch's ground and leaves are told, taken from its record: the soil water
 * over field capacity and the condition code. The look of wet soil and of a wilted leaf is the asset
 * shader's (GODOT_SHADER_SOIL_REL, GODOT_SHADER_CONDITION); a default stand is an unknown one.
 */
struct PlantStand {
    bool  soil_known = false;       // the record carried soil_rel as a number
    float soil_rel = 0.0f;          // soil water over field capacity, 0..1, as the record carried it
    bool  condition_known = false;  // the record carried a condition code the view knows
    float condition = 0.0f;         // GODOT_COND_* as the record carried it
};

/** Appends one three-component value - a place or a normal - to the array of a mesh surface. */
void append3(std::vector<float>& out, float a, float b, float c);

/**
 * Where the stem base of the plant on each patch stands, taken from its record: x and z, the centre
 * of the patch square the host laid, and y, the height ase-terrain levelled it to (TRN_FLAT_HGT, laid
 * on the record by ase-pl-flora). Keyed by the patch's object id, the body of its organ records. A
 * patch whose record does not carry all three as numbers - its disc is not levelled yet - has no
 * origin, and its plant is not drawn. Nothing is looked up: no grid, no sample.
 */
std::unordered_map<uint32_t, PlantOrigin> plant_origins(const std::vector<VivariumPatchView>& patches);

/**
 * The stand of every patch, taken from its record and keyed by its object id like plant_origins:
 * soil_rel as the number it carried, the condition as the GODOT_COND_* code it carried. A soil_rel the
 * record did not carry (NaN) and a condition outside the documented codes stay unknown - the material
 * then draws its shader's own default, never a value made up here.
 */
std::unordered_map<uint32_t, PlantStand> plant_stands(const std::vector<VivariumPatchView>& patches);

}  // namespace ase::adp::godot

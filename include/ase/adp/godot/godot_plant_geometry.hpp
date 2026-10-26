#pragma once

/**
 * ASE GODOT ADAPTER - PLANT GEOMETRY
 *
 * @file        godot_plant_geometry.hpp
 * @brief       The templates, the ground and the instance buffers of the plants, as plain arrays
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 4 and 03.11 step 2: every organ the view
 *              draws is an instance of a template, and its 3x4 is the one ase-render-data computed -
 *              the view copies it. These functions take the templates of the template ports (the fine
 *              and the coarse cylinder and the unit leaf ase-render-data builds on the unit), the ground
 *              from the terrain samples, the stem base and the stand of every plant, and they sort the
 *              records of an instance port by body and order into the buffers MultiMesh::set_buffer
 *              takes. They compute no organ and no template: no ring, no frame, no scale and no place -
 *              the values of a record go to the buffer as the port carried them. The root is not drawn:
 *              it lies in the ground of the patch (03.1 point 4), and it has no instance port.
 *
 *              WHERE A PLANT STANDS: an instance's origin is plant-local (origin at the stem base on
 *              the ground). The scene stands each plant as one node at the origin of its body - the
 *              stem base its patch record carries (plant_origins: the patch centre and the height
 *              ase-terrain levelled it to) - and a body without an origin is not drawn: a plant whose
 *              ground is not levelled yet is no plant to draw at the world's origin.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check the
 *              geometry without a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-09
 * @modified    2026-10-25
 * @version     00.00.06.00000
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

/**
 * The disc ase-terrain levels under the plant of one patch (03.8): its centre and radius from the start
 * configuration, its height the one the patch record carries (TRN_FLAT_HGT). Ground that stands at this
 * height inside the disc is levelled ground - bare soil, no grass.
 */
struct PlantGroundDisc {
    float x = 0.0f;       // centre, world metres east
    float z = 0.0f;       // centre, world metres north
    float radius = 0.0f;  // metres
    float height = 0.0f;  // levelled height, metres
};

/** The bed of one brook as the start configuration lays it (03.8): the two ends and half its width, metres. */
struct PlantGroundBed {
    float x = 0.0f;
    float z = 0.0f;
    float end_x = 0.0f;
    float end_z = 0.0f;
    float half_m = 0.0f;
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
 * class without bark), then zeros up to GODOT_INSTANCE_CUSTOM_VALUES. A record without an organ
 * record is not drawn: both ports are read in the same snapshot, so the gap is a broken port, never an
 * organ of an unknown plant. GODOT_ORDER_EVERY as `split_order` puts every organ above it - a class the
 * order budget does not split.
 */
std::unordered_map<uint32_t, PlantBodyRows> rows_by_body(const std::vector<VivariumInstanceView>& instances,
                                                         const std::vector<VivariumOrganView>& organs,
                                                         double split_order);

/**
 * Appends one organ template to `out`, taken as the template ports carried it (ase-render-data,
 * usecase:101011 tessellate the organ templates): one vertex per corner record of `code` - its place on the
 * unit, its normal and its texture coordinates - and per triangle record of `code` the triangle over
 * the corners its three indices name, in the order the record gives them. GODOT_TMPL_FINE is the
 * cylinder of the internodes up to the shown orders, GODOT_TMPL_CORS the one above them, and the fine
 * one stands in for a catkin or a fruit stand whose template asset is missing; GODOT_TMPL_LEAF stands
 * in for a leaf or a seed. A triangle whose corner the port did not carry is not drawn; an index named
 * twice is drawn once. Nothing is computed: no ring, no normal, no winding.
 *
 * @return the triangles drawn
 */
uint32_t build_template(const std::vector<VivariumTemplateCornerView>& corners,
                        const std::vector<VivariumTemplateTriangleView>& triangles, uint32_t code,
                        PlantMeshArrays& out);

/**
 * Appends the ground of the scene to `out`, as the ports carried it: one vertex per terrain sample - its
 * world place and height (ase-terrain), its normal and texture coordinates (ase-render-data) - and per
 * ground cell the two triangles (sw, se, ne) and (sw, ne, nw) over the vertices its corners name. A cell
 * whose corner the terrain port did not carry is not drawn; a sample named twice is drawn once. Nothing
 * is computed: no grid, no normal, no winding.
 *
 * @return the cells drawn
 */
uint32_t build_ground(const std::vector<VivariumTerrainView>& samples,
                      const std::vector<VivariumGroundCellView>& cells, PlantMeshArrays& out);

/**
 * Gives every vertex of a ground `out` built by build_ground its colour, which the ground shader reads as
 * the cover of the ground (GODOT_GROUND_SHADER_CODE): red 1 where the vertex is levelled ground - inside a
 * disc of `discs` and within GODOT_GROUND_LEVEL_TOLERANCE_M of its height, so the disc is soil only once
 * ase-terrain levelled it -, green 1 where it lies in the bed of a brook of `beds` (no farther from the
 * line between its ends than half its width), blue 0, alpha 1. Replaces the colours `out` carried.
 *
 * @return the vertices marked levelled or bed
 */
uint32_t mark_ground(const std::vector<PlantGroundDisc>& discs, const std::vector<PlantGroundBed>& beds,
                     PlantMeshArrays& out);

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

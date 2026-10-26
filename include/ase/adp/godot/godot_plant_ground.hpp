#pragma once

/**
 * ASE GODOT ADAPTER - GROUND AROUND THE PLANT
 *
 * @file        godot_plant_ground.hpp
 * @brief       The ground mesh from the terrain and ground cell ports and the marks its shader paints by
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 and 03.11: build_ground takes the terrain samples and the
 *              ground cells as ase-terrain and ase-render-data laid them; mark_ground gives every vertex the
 *              colour the ground shader reads as its cover - levelled ground of a patch's disc or the player's,
 *              the bed of a brook (GODOT_GROUND_SHADER_CODE). Nothing is computed about the terrain: no grid,
 *              no normal, no winding.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check it without a Godot
 *              process.
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

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/**
 * A disc of levelled ground (03.8): ase-terrain levels the disc under the plant of a patch, and the player
 * levels one where he taps. Its centre and radius, its height the one the ground stands at once levelled.
 * Ground that stands at this height inside the disc is levelled ground - bare soil, no grass.
 */
struct PlantGroundDisc {
    float x = 0.0f;       // centre, world metres east
    float z = 0.0f;       // centre, world metres north
    float radius = 0.0f;  // metres
    float height = 0.0f;  // levelled height, metres
};

/** The bed of one brook (03.8), configured or laid by the player: the two ends and half its width, metres. */
struct PlantGroundBed {
    float x = 0.0f;
    float z = 0.0f;
    float end_x = 0.0f;
    float end_z = 0.0f;
    float half_m = 0.0f;
};

/**
 * Appends the ground of the scene to `out`, as the ports carried it: one vertex per terrain sample - its
 * world place and height (ase-terrain), its normal and texture coordinates (ase-render-data) - and per
 * ground cell the two triangles (sw, se, ne) and (sw, ne, nw) over the vertices its corners name. A cell
 * whose corner the terrain port did not carry is not drawn; a sample named twice is drawn once.
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

}  // namespace ase::adp::godot

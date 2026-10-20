#pragma once

/**
 * ASE GODOT ADAPTER - THE SELECTED PATCH
 *
 * @file        godot_patch_focus.hpp
 * @brief       Which patch the care view looks at, and where the camera stands against its stem base
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2 and V2: tapping a patch moves the camera to its
 *              birch. The view looks at the selected place, and without a selection at the first place of
 *              the start configuration; the camera stands `distance_m` south of the stem base and
 *              `height_m` above it and looks at the point `look_m` above it. The stem base is the patch's
 *              origin as its record carries it (PlantOrigin) - nothing is computed about the plant itself.
 *
 *              ONE RULE FOR THE FOCUS: the scene aims its camera and draws the ground of the focused patch,
 *              and both take the place from patch_focus_place, so the two never look at different patches.
 *
 *              NO GODOT TYPE: the scene turns the aim into a Camera3D, the plant test drives it without a
 *              Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100386 - select a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-12
 * @modified    2026-10-12
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <cstdint>

namespace ase::adp::godot {

/** The near view of the start configuration ("view"): where the camera stands against the focused stem base. */
struct PlantSceneCamera {
    float distance_m = 0.0f;  // metres from the stem base to the camera, along the ground (from the south)
    float height_m = 0.0f;    // metres of the camera above the stem base
    float look_m = 0.0f;      // metres above the stem base the camera looks at
    float fov_deg = 0.0f;     // vertical field of view, degrees
};

/** Where the camera stands and the point it looks at, in world metres (x east, y up, z north). */
struct PatchFocusAim {
    float eye_x = 0.0f;
    float eye_y = 0.0f;
    float eye_z = 0.0f;
    float target_x = 0.0f;
    float target_y = 0.0f;
    float target_z = 0.0f;
};

/** The place the view looks at: the selected one, and without a selection (GODOT_NO_PLACE) `first_place`. */
[[nodiscard]] uint32_t patch_focus_place(uint32_t selected_place, uint32_t first_place);

/** The near view onto `origin`: `distance_m` south of it and `height_m` above, looking at `look_m` above it. */
[[nodiscard]] PatchFocusAim patch_focus_aim(const PlantSceneCamera& camera, const PlantOrigin& origin);

}  // namespace ase::adp::godot

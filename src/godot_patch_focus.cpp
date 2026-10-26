/**
 * ASE GODOT ADAPTER - THE SELECTED PATCH
 *
 * @file        godot_patch_focus.cpp
 * @brief       Which patch the care view looks at, and where the camera stands against its stem base
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2 and V2, see godot_patch_focus.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100386 - select a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-12
 * @modified    2026-10-25
 * @version     00.00.02.00000
 */

#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/types.hpp>

namespace ase::adp::godot {

uint32_t patch_focus_place(uint32_t selected_place, uint32_t first_place) {
    return selected_place != GODOT_NO_PLACE ? selected_place : first_place;
}

PatchFocusAim patch_focus_aim(const PlantSceneCamera& camera, const PlantOrigin& origin) {
    PatchFocusAim aim;
    aim.eye_x = origin.x;
    aim.eye_y = origin.y + camera.height_m;
    aim.eye_z = origin.z - camera.distance_m;
    aim.target_x = origin.x;
    aim.target_y = origin.y + camera.look_m;
    aim.target_z = origin.z;
    return aim;
}

float patch_focus_zoom(float zoom, int32_t steps) {
    float next = zoom;
    for (int32_t s = 0; s < steps; ++s) {
        next *= GODOT_ZOOM_STEP;
    }
    for (int32_t s = 0; s > steps; --s) {
        next /= GODOT_ZOOM_STEP;
    }
    return next < GODOT_ZOOM_MIN ? GODOT_ZOOM_MIN : (next > GODOT_ZOOM_MAX ? GODOT_ZOOM_MAX : next);
}

PlantSceneCamera patch_focus_zoomed(const PlantSceneCamera& camera, float zoom) {
    PlantSceneCamera zoomed = camera;
    zoomed.distance_m = camera.distance_m * zoom;
    zoomed.height_m = camera.height_m * zoom;
    zoomed.look_m = camera.look_m * zoom;
    return zoomed;
}

}  // namespace ase::adp::godot

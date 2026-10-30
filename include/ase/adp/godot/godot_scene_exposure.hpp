#pragma once

/**
 * ASE GODOT ADAPTER - EXPOSURE OF THE SCENE CAMERA
 *
 * @file        godot_scene_exposure.hpp
 * @brief       The exposure multiplier of the scene camera for the light the sky port carries
 * @description Register D1721: the eye adapts to the night, a fixed camera does not. The sky port hands the
 *              light as ase-pl-sky computes it, and the view keeps those numbers; the camera lifts its
 *              exposure in part, as an eye does (GODOT_EXPOSURE_DAY_LIGHT, GODOT_EXPOSURE_LIFT_MAX in
 *              types.hpp). light_plant_scene sets the multiplier every frame; both renderers, mobile and
 *              Compatibility, take it.
 *
 *              NO GODOT TYPE: plain numbers in, one number out, so the host tests check it without a Godot
 *              process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101415 - hand sky light to app
 * @actor       cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>

namespace ase::adp::godot {

/**
 * The exposure multiplier of the scene camera for the light `sky` carries: the light of the scene is the sun's
 * intensity plus that of every moon the port carries plus the ambient intensity times the mean
 * of its colour, and the lift is the square root of
 * GODOT_EXPOSURE_DAY_LIGHT over that light - the eye adapts only in part, so the night stays darker than the
 * day -, never below 1 and at most GODOT_EXPOSURE_LIFT_MAX. A sky the port did not carry yet, or no light at
 * all, takes the largest lift. Nothing of the sky is computed: its numbers are read as the port carried them.
 */
[[nodiscard]] float scene_exposure(const VivariumSkyView& sky);

}  // namespace ase::adp::godot

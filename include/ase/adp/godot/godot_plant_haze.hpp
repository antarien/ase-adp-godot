#pragma once

/**
 * ASE GODOT ADAPTER - THE SHIMMER OF HOT AIR IN THE CARE VIEW
 *
 * @file        godot_plant_haze.hpp
 * @brief       The shimmer of hot air over the care view's 3D scene
 * @description DSGN_130 130/FOGWR: from 313 K the rising warm air breaks the light unevenly, the horizon
 *              begins to waver, and up to the heat damage limit of 323 K the shimmer grows to full strength;
 *              whoever watches a shimmering plain from afar makes out shapes in it only blurred.
 *              ase-atmosphere computes the strength from the temperature (AtmoSimTmpHazeSystem,
 *              ATM_VIS_HEAT_HAZE), ase-pl-sky hands it on as the shimmer of the fog record
 *              (SkyHostScneExptSystem). The view shows it: a quad in front of the camera reads the drawn
 *              image and shifts its rows sideways, the more the farther away what a row shows lies, between
 *              where the fog of the same record begins and where it is full. Nothing of the shimmer is
 *              computed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_130
 * @category    render/pipeline/stage
 * @usecase     usecase:101515 - veil sight with weather
 * @actor       Fog of War (FOW)
 * @created     2026-10-28
 * @modified    2026-10-28
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>

#include <godot_cpp/classes/node3d.hpp>

namespace ase::adp::godot {

/**
 * Hangs the hidden haze quad (GODOT_PLANT_HAZE) under the camera of the scene (GODOT_PLANT_CAMERA),
 * GODOT_HAZE_QUAD_DEPTH in front of it, in a ShaderMaterial of GODOT_HAZE_SHADER_CODE whose wave stands
 * at GODOT_HAZE_WAVE_ROWS, GODOT_HAZE_WAVE_SPEED and GODOT_HAZE_WAVE_REACH. A scene without its camera
 * gets none. build_plant_scene calls it once, after the camera stands.
 */
void build_plant_haze(::godot::Node3D* scene);

/**
 * Shows the haze quad while the fog record carries a shimmer above 0 and hands it the shimmer and the near
 * and far distance of the fog; at 0 the quad is hidden and the image goes unshifted. A sky the port did not
 * carry yet changes nothing.
 */
void veil_plant_scene(::godot::Node3D* scene, const VivariumSkyView& sky);

}  // namespace ase::adp::godot

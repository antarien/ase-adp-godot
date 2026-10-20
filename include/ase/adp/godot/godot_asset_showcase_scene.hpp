#pragma once

/**
 * ASE GODOT ADAPTER - ASSET SHOWCASE SCENE
 *
 * @file        godot_asset_showcase_scene.hpp
 * @brief       Builds the asset showcase in Godot from its plan and lights it by the sun of the host
 * @description PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7. The plan (godot_asset_showcase.hpp) says
 *              where every name goes; this part makes the nodes. Each bound file is read at run time
 *              through GLTFDocument - the stage exports it as it is (`.import` with importer "keep") -
 *              and the material of the first surface of its first mesh is the asset's material: a
 *              material is shown on a sphere and on a plane, a template as its own model scaled to one
 *              slot. A name without its file, or a file that does not load, is drawn in the error
 *              material of asset_color; there is no stand-in. The showcase carries no light of its own:
 *              it is lit by the sun the view reads from the game host (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS
 *              03.11, one port for sun and sky), handed in through light_asset_showcase.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:100347 - show approved assets
 * @actor       Asset Showcase (ASSETSHOW)
 * @created     2026-10-09
 * @modified    2026-10-10
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/godot_asset_showcase.hpp>

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>

namespace ase::adp::godot {

/**
 * Builds the showcase of `plan` under `parent` and returns its root: a camera from showcase_camera,
 * per material a sphere and a plane carrying the asset's material, per template its model. Files
 * are read below `asset_root` (res://).
 */
::godot::Node3D* build_asset_showcase(::godot::Node* parent, const AssetShowcasePlan& plan,
                                      const VivariumAssetBinding& binding, const ::godot::String& asset_root);

/**
 * Lights the showcase by the sun record of the host's sky port (SKY_HOST_RECORD_SUN): `toward_sun`
 * (x east, y up, z north) points from the scene to the sun, `sun_rgb` (r, g, b) is its colour,
 * `energy` its strength. The first call adds the light, every later one moves it.
 */
void light_asset_showcase(::godot::Node3D* showcase, const float* toward_sun, const float* sun_rgb, float energy);

}  // namespace ase::adp::godot

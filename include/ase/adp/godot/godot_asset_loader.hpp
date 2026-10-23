#pragma once

/**
 * ASE GODOT ADAPTER - ASSET LOADER
 *
 * @file        godot_asset_loader.hpp
 * @brief       The one way a bound asset file becomes a Godot node, mesh or material
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2: the adapter binds every material and organ
 *              template of config/vivarium_assets.json by name to a file of the manifest
 *              (godot_asset_binding.hpp); these functions read such a file at run time with
 *              GLTFDocument and hand out what it carries. The asset showcase (04.7) and the plant scene
 *              (03.1) both draw through them - one loader, never a second.
 *
 *              A NAME WITHOUT ITS FILE IS DRAWN IN THE ERROR MATERIAL (03.2 point 3): one signal colour,
 *              asset_color's, for every name the binding holds no file for and for a file that does not
 *              load or carries no surface. There is no silent stand-in material.
 *
 *              A TEMPLATE IS AUTHORED ON THE UNIT: its base at the origin and its length along +y to 1,
 *              like the unit cylinder of godot_plant_geometry.hpp, so the 3x4 of ase-render-data that
 *              scales y to the organ's length places it unchanged (03.11 step 2). The loader hands the
 *              mesh out as the file carries it and scales nothing.
 *
 *              A THIN ORGAN LETS THE LIGHT THROUGH: a file whose image list carries a map ending in
 *              GODOT_IMAGE_TRANSLUCENCY gets it bound as the backlight texture of its materials, so a
 *              leaf transmits the sun in the app as it does in the curator's backlight view.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    render/pipeline/stage
 * @usecase     usecase:6451 - load flora blueprints
 * @actor       Basic Flora Model Ressources (BFLOMR)
 * @created     2026-10-11
 * @modified    2026-10-22
 * @version     00.00.02.00002
 */

#include <ase/adp/godot/godot_asset_binding.hpp>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <string>

namespace ase::adp::godot {

/** The scene GLTFDocument generates from a bound file below `asset_root` at run time; nullptr when it does not load. */
::godot::Node* load_asset(const ::godot::String& asset_root, const std::string& file);

/** The first MeshInstance3D below `node`, depth first; nullptr without one. */
::godot::MeshInstance3D* first_mesh(::godot::Node* node);

/** The error material of a name: a new lit surface in the colour asset_color gives a name the binding holds no file for. */
::godot::Ref<::godot::StandardMaterial3D> error_material(const VivariumAssetBinding& binding, const std::string& name);

/** The material of a bound material asset - the first surface of its file, its translucency bound as backlight - or the error material when its file or its surface is missing. */
::godot::Ref<::godot::Material> bound_material(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                               const std::string& name);

/**
 * The mesh of a bound organ template - its surfaces with the materials of its file, their translucency bound
 * as backlight - or an empty reference
 * when the binding holds no file for the name, the file does not load or carries no mesh. The caller draws
 * an empty one as its stand-in shape in the error material.
 */
::godot::Ref<::godot::Mesh> bound_template(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                           const std::string& name);

}  // namespace ase::adp::godot

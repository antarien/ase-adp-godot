#pragma once

/**
 * ASE GODOT ADAPTER - PLANT LOOK
 *
 * @file        godot_plant_look.hpp
 * @brief       The materials and organ template files the care view's 3D scene draws with, bound once
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2: every role of the stage - bark, ground, leaf,
 *              catkins, fruit stand, nutlet - takes the material or the template its file carries
 *              (godot_asset_loader.hpp, the loader the asset showcase shares). The binding happens once,
 *              when the configuration is read; the scene fills only hand the bound resources on
 *              (godot_plant_scene.hpp). A role the stage does not name, or whose file is missing, is drawn
 *              in the error material, a missing template file as the port template that stands in for it
 *              (usecase:101011 lay out organ templates).
 *
 *              THE BARK IS ONE MATERIAL (usecase:100395 age the bark): a ShaderMaterial that mixes the
 *              white bark with the stem-base bark by the share each internode instance carries as custom
 *              data. Its uniforms take the colour, textures, roughness and UV scale of both bound
 *              materials; the share itself is ase-plant's, never chosen here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/string.hpp>

#include <string>
#include <unordered_map>

namespace ase::adp::godot {

/** One organ template role: the mesh its file carries and the material it is drawn with. */
struct PlantSceneTemplate {
    ::godot::Ref<::godot::Mesh> shape;         // the bound template; empty while its file is missing - the port template stands in
    ::godot::Ref<::godot::Material> material;  // empty for a bound template, which draws its own surfaces; else the error material
};

/**
 * What the scene draws with, each resolved through the asset binding once, when the configuration is read
 * (bind_plant_look) - a file is read at the start, never per frame.
 */
struct PlantSceneLook {
    ::godot::Ref<::godot::Material> bark;                      // the internodes on both cylinders: white bark mixed with stem-base bark by the share of each instance (GODOT_ROLE_BARK, GODOT_ROLE_BARK_BASE)
    ::godot::Ref<::godot::Material> ground[GODOT_SOIL_COUNT];  // the ground by the soil code of the focused patch (GODOT_ROLE_GROUND_*)
    ::godot::Ref<::godot::Material> soil;                      // the ground while the focused patch carries no soil code: bare soil
    PlantSceneTemplate leaf;                                   // leaves (GODOT_ROLE_LEAF), stand-in the leaf template GODOT_TMPL_LEAF
    PlantSceneTemplate male;                                   // male catkins (GODOT_ROLE_MALE), stand-in the fine cylinder GODOT_TMPL_FINE
    PlantSceneTemplate female;                                 // female catkins (GODOT_ROLE_FEMALE), stand-in the fine cylinder GODOT_TMPL_FINE
    PlantSceneTemplate fruit;                                  // fruit stands (GODOT_ROLE_FRUIT), stand-in the fine cylinder GODOT_TMPL_FINE
    PlantSceneTemplate nutlet;                                 // seed bodies (GODOT_ROLE_NUTLET), stand-in the leaf template at GODOT_NUTLET_STAND_IN_M
};

/**
 * Binds the look of the scene: per role of the stage (`roles`, role → asset name of
 * config/vivarium_assets.json) the material or the template its file carries (bound_material,
 * bound_template). A role the stage does not name, or whose file is missing, is drawn in the error
 * material; a missing template file as the port template that stands in for it.
 */
PlantSceneLook bind_plant_look(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                               const std::unordered_map<std::string, std::string>& roles);

}  // namespace ase::adp::godot

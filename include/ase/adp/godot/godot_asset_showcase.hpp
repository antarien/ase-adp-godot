#pragma once

/**
 * ASE GODOT ADAPTER - ASSET SHOWCASE PLAN
 *
 * @file        godot_asset_showcase.hpp
 * @brief       Where the asset showcase puts every name of the asset list, and what its log line says
 * @description PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7: launched with the argument of modes.assets
 *              (`--avt-assets`), the view shows every asset of the list beside the game host - each
 *              material on a sphere and on a plane, each organ template as its model. This part plans
 *              the showcase without a Godot type: the names split into materials and templates in list
 *              order, each once; the x of every slot, each row centred on the origin; the camera that
 *              sees the widest row; the scale that fits a template into one slot; and the log line
 *              with the number of names and of missing ones. It reads the binding the view made at
 *              configuration (bind_assets) and never binds a second time. The scene itself is built by
 *              godot_asset_showcase_scene.hpp.
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

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The showcase of one play stage: the names per row, their slots and how many lack their asset. */
struct AssetShowcasePlan {
    std::vector<std::string> materials;   // material names in list order, each once
    std::vector<std::string> templates;   // organ template names in list order, each once
    std::vector<float>       material_x;  // x of the sphere and of the plane of each material
    std::vector<float>       template_x;  // x of each template
    uint32_t                 missing = 0u;  // names of the plan the binding holds no file for
};

/**
 * Plans the showcase of `names` (the list of one play stage in list order); a name `materials` holds
 * is a material, every other a template.
 */
[[nodiscard]] AssetShowcasePlan plan_showcase(const std::vector<std::string>& names,
                                              const std::vector<std::string>& materials,
                                              const VivariumAssetBinding& binding);

/** The log line: "asset showcase names=<n> materials=<m> templates=<t> missing=<k>". */
[[nodiscard]] std::string showcase_line(const AssetShowcasePlan& plan);

/** The scale that brings a template whose largest extent is `largest_extent` metres to one slot. */
[[nodiscard]] float template_scale(float largest_extent);

/**
 * The camera of the showcase: `position` (x, y, z) behind the rows and above them, so that the
 * widest row fills the horizontal field of view; `target` (x, y, z) is the centre of the rows.
 */
void showcase_camera(const AssetShowcasePlan& plan, float* position, float* target);

}  // namespace ase::adp::godot

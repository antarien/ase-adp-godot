/**
 * ASE GODOT ADAPTER - ASSET SHOWCASE PLAN IMPLEMENTATION
 *
 * @file        godot_asset_showcase.cpp
 * @brief       Rows, slots, camera and log line of the asset showcase, without a Godot type
 * @description See godot_asset_showcase.hpp. A row of n slots stands centred on the origin,
 *              GODOT_SHOWCASE_SPACING apart. The camera stands on the axis of the rows, behind them by
 *              the distance at which the widest row plus one slot on either side fills the horizontal
 *              field of view, and above them by a share of that distance.
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

#include <ase/adp/godot/godot_asset_showcase.hpp>
#include <ase/adp/godot/types.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

bool listed(const std::vector<std::string>& list, const std::string& name) {
    for (const std::string& entry : list) {
        if (entry == name) {
            return true;
        }
    }
    return false;
}

/** The x of `count` slots, centred on the origin, GODOT_SHOWCASE_SPACING apart. */
std::vector<float> row_x(std::size_t count) {
    std::vector<float> x;
    const float span = count > 0u ? static_cast<float>(count - 1u) : 0.0f;
    const float first = -0.5f * GODOT_SHOWCASE_SPACING * span;
    for (std::size_t i = 0u; i < count; ++i) {
        x.push_back(first + GODOT_SHOWCASE_SPACING * static_cast<float>(i));
    }
    return x;
}

}  // namespace

AssetShowcasePlan plan_showcase(const std::vector<std::string>& names, const std::vector<std::string>& materials,
                                const VivariumAssetBinding& binding) {
    AssetShowcasePlan plan;
    for (const std::string& name : names) {
        if (name.empty() || listed(plan.materials, name) || listed(plan.templates, name)) {
            continue;
        }
        if (listed(materials, name)) {
            plan.materials.push_back(name);
        } else {
            plan.templates.push_back(name);
        }
        if (binding.files.count(name) == 0u) {
            plan.missing += 1u;
        }
    }
    plan.material_x = row_x(plan.materials.size());
    plan.template_x = row_x(plan.templates.size());
    return plan;
}

std::string showcase_line(const AssetShowcasePlan& plan) {
    return std::string(GODOT_SHOWCASE_LINE) + " names=" + std::to_string(plan.materials.size() + plan.templates.size()) +
           " materials=" + std::to_string(plan.materials.size()) + " templates=" +
           std::to_string(plan.templates.size()) + " missing=" + std::to_string(plan.missing);
}

float template_scale(float largest_extent) {
    // A template without extent has nothing to fit; it keeps its own size.
    return largest_extent > GODOT_GEOMETRY_EPSILON ? GODOT_SHOWCASE_SLOT / largest_extent : 1.0f;
}

void showcase_camera(const AssetShowcasePlan& plan, float* position, float* target) {
    const std::size_t widest = plan.materials.size() > plan.templates.size() ? plan.materials.size()
                                                                              : plan.templates.size();
    const float half_width = 0.5f * GODOT_SHOWCASE_SPACING * static_cast<float>(widest) + GODOT_SHOWCASE_SLOT;
    const float distance = half_width / GODOT_SHOWCASE_HALF_FOV_TAN;
    const float centre_z = 0.5f * GODOT_SHOWCASE_TEMPLATE_ROW;
    target[0] = 0.0f;
    target[1] = 0.0f;
    target[2] = centre_z;
    position[0] = 0.0f;
    position[1] = distance * GODOT_SHOWCASE_CAMERA_HEIGHT_SHARE;
    position[2] = centre_z - distance;
}

}  // namespace ase::adp::godot

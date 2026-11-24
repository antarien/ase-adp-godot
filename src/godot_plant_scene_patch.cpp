/**
 * ASE GODOT ADAPTER - THE BODIES ON THE PATCH BESIDE THE PLANTS
 *
 * @file        godot_plant_scene_patch.cpp
 * @brief       The seed bodies of ase-physics and the fruit bodies of the cap patches in the care view's 3D scene
 * @description See godot_plant_scene.hpp (fill_plant_seeds) and cap/godot_plant_cap.hpp (stamp_plant_caps,
 *              fill_plant_caps). Both draw instances of a template at the stem base of a body, scaled uniformly:
 *              the nutlet at scale one or the stand-in leaf at the width of a winged nutlet, the fruit body at the
 *              scale cap_scale reads off its drawn mesh for the radius ase-phytomer passed on. Neither builds a
 *              shape; both hand the template the stage bound, or while its file is missing the template port's, to
 *              fill_instances (godot_plant_scene_mesh.cpp) with one 3x4 per body in the row order of MultiMesh.
 *
 *              WHY A UNIT OF ITS OWN: the plant scene (godot_plant_scene.cpp) lays the plants - bark, organ
 *              bundles, impostor and the stands of their leaves. Seeds and caps are bodies of their own on the
 *              patch, read from their own ports (seed port, cap port) and drawn under their own nodes
 *              (GODOT_PLANT_SEEDS, GODOT_PLANT_CAPS); nothing of a plant node reaches them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101362 - scale the organ templates
 * @actor       Organ Transform Shaper (ORGATRASHA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_plant_scene_parts.hpp>
#include <ase/adp/godot/cap/godot_cap_scale.hpp>
#include <ase/adp/godot/cap/godot_plant_cap.hpp>
#include <ase/adp/godot/cap/types.hpp>
#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/adp/godot/template/types.hpp>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ase::adp::godot {

void fill_plant_seeds(::godot::Node3D* scene, const GodotHostResourceManager& resources,
                      const std::unordered_map<uint32_t, PlantOrigin>& origins, const PlantSceneLook& look) {
    ::godot::MultiMeshInstance3D* node = part_of<::godot::MultiMeshInstance3D>(scene, GODOT_PLANT_SEEDS);
    if (node == nullptr) {
        return;
    }
    // the nutlet template carries its own size; the leaf template of the ports stands in at the width of a
    // winged nutlet
    const bool bound = look.nutlet.shape.is_valid();
    const ::godot::Ref<::godot::Mesh> shape = bound ? look.nutlet.shape : template_mesh(resources, GODOT_TMPL_LEAF);
    const float scale = bound ? 1.0f : GODOT_NUTLET_STAND_IN_M;
    // one 3x4 per seed in the row order of MultiMesh: the axes at that scale, the origin at the stem base of its
    // body plus the place the port carried
    const std::vector<VivariumSeedView>& seeds = resources.get_seeds();
    std::vector<float> rows;
    rows.reserve(seeds.size() * GODOT_INSTANCE_VALUES);
    for (const VivariumSeedView& seed : seeds) {
        const auto origin = origins.find(seed.body);
        if (origin == origins.end()) {
            continue;
        }
        // all three axes in the frame of the stem foot, the height too: ase-physics lands the seed on the ground
        // its contact measures at the terrain sample, over the levelled disc too, so a hole dug into the disc
        // takes the seed down (D2315); the view adds no ground height of its own (D2312)
        const float at[3] = {static_cast<float>(origin->second.x + seed.x), static_cast<float>(origin->second.y + seed.y),
                             static_cast<float>(origin->second.z + seed.z)};
        for (uint32_t r = 0u; r < 3u; ++r) {
            for (uint32_t c = 0u; c < 3u; ++c) {
                rows.push_back(r == c ? scale : 0.0f);
            }
            rows.push_back(at[r]);
        }
    }
    fill_instances(node, shape, rows, nullptr, look.nutlet.material, nullptr);
}

void stamp_plant_caps(::godot::Node3D* scene, const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                      const std::unordered_map<std::string, std::string>& roles) {
    ::godot::Node3D* caps = part_of<::godot::Node3D>(scene, GODOT_PLANT_CAPS);
    if (caps == nullptr) {
        return;
    }
    // one child per species the stage names a cap role for (fungus.<catalog id>): each carries only the bound shape
    // of its species and the material of it until the cap port fills it
    for (const auto& [role, name] : roles) {
        (void)name;
        const std::string species = cap_species_of(role);
        if (species.empty()) {
            continue;
        }
        const PlantSceneTemplate cap = bind_plant_cap(binding, asset_root, roles, species);
        ::godot::MultiMeshInstance3D* child = add_part<::godot::MultiMeshInstance3D>(caps, cap_node_of(species).c_str());
        fill_instances(child, cap.shape, std::vector<float>(), nullptr, cap.material, nullptr);
    }
}

void fill_plant_caps(::godot::Node3D* scene, const GodotHostResourceManager& resources,
                     const std::unordered_map<uint32_t, PlantOrigin>& origins,
                     const std::unordered_map<uint32_t, double>& caps, const std::vector<VivariumPatchSpec>& patches) {
    ::godot::Node3D* holder = part_of<::godot::Node3D>(scene, GODOT_PLANT_CAPS);
    if (holder == nullptr) {
        return;
    }
    // the species sown on every place, from the start configuration the view sowed (every place stands once)
    std::unordered_map<uint32_t, const std::string*> sown;
    for (const VivariumPatchSpec& patch : patches) {
        sown[patch.place] = &patch.species;
    }
    // every cap patch under the child of its species: the patch names its place, the place its species
    std::unordered_map<std::string, std::vector<std::pair<PlantOrigin, double>>> bodies;
    for (const auto& [body, radius] : caps) {
        const auto origin = origins.find(body);
        const VivariumPatchView* patch = resources.get_patch(body);
        if (origin == origins.end() || patch == nullptr) {
            continue;
        }
        const auto species = sown.find(patch->place);
        if (species == sown.end()) {
            continue;
        }
        bodies[cap_node_of(*species->second)].emplace_back(origin->second, radius);
    }
    const ::godot::Ref<::godot::Mesh> unit = template_mesh(resources, GODOT_TMPL_FNGS);
    const int32_t count = holder->get_child_count();
    for (int32_t index = 0; index < count; ++index) {
        ::godot::MultiMeshInstance3D* node = ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(holder->get_child(index));
        if (node == nullptr) {
            continue;
        }
        // the template the child holds since the start (stamp_plant_caps): fill_instances hands the same shape and
        // material back every frame, so the child keeps holding it
        const ::godot::Ref<::godot::MultiMesh> held = node->get_multimesh();
        const ::godot::Ref<::godot::Mesh> bound = held.is_valid() ? held->get_mesh() : ::godot::Ref<::godot::Mesh>();
        const ::godot::Ref<::godot::Material> material = node->get_material_override();
        // the bound fruit body carries its own proportions; while its file is missing the unit fruit body of the
        // template port stands in (GODOT_TMPL_FNGS, ground at 0, apex at 1, its widest reach the cap). The scale is
        // read off the drawn mesh, so the cap of either reaches the radius ase-phytomer passed on for the individual
        const ::godot::Ref<::godot::Mesh> shape = bound.is_valid() ? bound : unit;
        const auto mine = bodies.find(utf8_of(::godot::String(node->get_name())));
        if (shape.is_null() || mine == bodies.end()) {
            fill_instances(node, shape, std::vector<float>(), nullptr, material, nullptr);
            continue;
        }
        const ::godot::AABB box = shape->get_aabb();
        // one 3x4 per cap patch in the row order of MultiMesh: the axes at its scale, the origin at its stem base
        std::vector<float> rows;
        rows.reserve(mine->second.size() * GODOT_INSTANCE_VALUES);
        for (const auto& [origin, radius] : mine->second) {
            const float scale = cap_scale(static_cast<float>(radius), static_cast<float>(box.size.x),
                                          static_cast<float>(box.size.z));
            if (!(scale > 0.0f)) {
                continue;
            }
            const float at[3] = {static_cast<float>(origin.x), static_cast<float>(origin.y), static_cast<float>(origin.z)};
            for (uint32_t r = 0u; r < 3u; ++r) {
                for (uint32_t c = 0u; c < 3u; ++c) {
                    rows.push_back(r == c ? scale : 0.0f);
                }
                rows.push_back(at[r]);
            }
        }
        fill_instances(node, shape, rows, nullptr, material, nullptr);
    }
}

}  // namespace ase::adp::godot

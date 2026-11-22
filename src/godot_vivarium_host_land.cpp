/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HOST LAND
 *
 * @file        godot_vivarium_host_land.cpp
 * @brief       VivariumView - the patches, the measured ground and the brooks of the start configuration
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8: the land the game host lays stands in the start
 *              configuration - every patch with place, species, seed, soil and extent, the measured square
 *              of the ground with its relief, and the brooks with their beds and flow. A missing or broken
 *              key stops the view with that key on its error panel; GodotHostResourceManager::boot checks
 *              the same land again before a host exists. load_host_config in godot_vivarium_host_config.cpp
 *              calls the three readers in the order of the configuration.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <godot_cpp/variant/array.hpp>

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

bool VivariumView::load_patches(const ::godot::Dictionary& root) {
    // THE PATCHES OF THE START CONFIGURATION (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, section on patch and species):
    // number, ground and species are data here; the game host lays each through the scene port. That
    // every place stands once and every ground is known, the resource manager checks again at boot.
    // Place and seed are whole numbers up to UINT32_MAX - beyond a float - so they are checked by
    // their round trip through uint32_t after the range, never by a float floor.
    const ::godot::Variant patches_value = root.get("patches", ::godot::Variant());
    if (patches_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("patches");
    }
    const ::godot::Array patches = patches_value;
    start_.patches.clear();
    for (int64_t i = 0; i < patches.size(); ++i) {
        if (patches[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("patches[]");
        }
        const ::godot::Dictionary entry = patches[i];
        VivariumPatchSpec patch;
        double place = 0.0;
        double seed = 0.0;
        ::godot::String species;
        ::godot::String soil;
        if (!read_number(entry, "place", place) || place < 0.0 || place > static_cast<double>(UINT32_MAX) ||
            static_cast<double>(static_cast<uint32_t>(place)) != place) {
            return fail_config("patches[].place");
        }
        if (!read_text(entry, "species", species) || species.is_empty()) {
            return fail_config("patches[].species");
        }
        if (!read_number(entry, "seed", seed) || seed < 0.0 || seed > static_cast<double>(UINT32_MAX) ||
            static_cast<double>(static_cast<uint32_t>(seed)) != seed) {
            return fail_config("patches[].seed");
        }
        if (!read_text(entry, "soil", soil)) {
            return fail_config("patches[].soil");
        }
        // Where the patch lies in the world and its half edge (03.8): ase-terrain levels the ground
        // there. Finite numbers and a positive extent; the terrain judges whether they are in reach.
        if (!read_number(entry, "x", patch.x) || !std::isfinite(patch.x)) {
            return fail_config("patches[].x");
        }
        if (!read_number(entry, "z", patch.z) || !std::isfinite(patch.z)) {
            return fail_config("patches[].z");
        }
        if (!read_number(entry, "half_m", patch.half_m) || !(patch.half_m > 0.0)) {
            return fail_config("patches[].half_m");
        }
        // The field capacity of its soil and its gravity: data of the scene, which ase-pl-flora files at
        // ase-hydro and ase-physics. Finite numbers here; the bounds are ase-pl-flora's, which refuses the
        // lay outside them (FLORA_WATER_MIN_MM..MAX_MM, GRV_CONST_AREA_REL_MIN..MAX).
        if (!read_number(entry, GODOT_KEY_CAPACITY_MM, patch.capacity_mm) || !std::isfinite(patch.capacity_mm)) {
            return fail_config("patches[].capacity_mm");
        }
        if (!read_number(entry, GODOT_KEY_GRAVITY_REL, patch.gravity_rel) || !std::isfinite(patch.gravity_rel)) {
            return fail_config("patches[].gravity_rel");
        }
        // The Startstand of the plant (usecase:100390): a finite number here; the bounds are ase-pl-flora's.
        if (!read_number(entry, GODOT_KEY_AGE_DAYS, patch.age_days) || !std::isfinite(patch.age_days)) {
            return fail_config("patches[].age_days");
        }
        patch.place = static_cast<uint32_t>(place);
        patch.species = utf8_of(species);
        patch.seed = static_cast<uint32_t>(seed);
        patch.soil = GodotHostResourceManager::soil_code(utf8_of(soil));
        if (patch.soil >= GODOT_SOIL_COUNT) {
            return fail_config("patches[].soil");
        }
        for (const VivariumPatchSpec& before : start_.patches) {
            if (before.place == patch.place) {
                return fail_config("patches[].place");
            }
        }
        start_.patches.push_back(patch);
    }
    if (start_.patches.empty()) {
        return fail_config("patches");
    }
    return true;
}

bool VivariumView::load_ground(const ::godot::Dictionary& root) {
    // THE MEASURED SQUARE OF THE SCENE (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8): its centre, half edge
    // and sample step are data here; ase-terrain measures it and the terrain port shows the heights.
    ::godot::Dictionary ground;
    if (!read_section(root, "ground", ground)) {
        return fail_config("ground");
    }
    if (!read_number(ground, "x", start_.ground.x) || !std::isfinite(start_.ground.x)) {
        return fail_config("ground.x");
    }
    if (!read_number(ground, "z", start_.ground.z) || !std::isfinite(start_.ground.z)) {
        return fail_config("ground.z");
    }
    if (!read_number(ground, "half_m", start_.ground.half_m) || !(start_.ground.half_m > 0.0)) {
        return fail_config("ground.half_m");
    }
    if (!read_number(ground, "step_m", start_.ground.step_m) || !(start_.ground.step_m > 0.0)) {
        return fail_config("ground.step_m");
    }
    // THE RELIEF OF THE WHOLE GROUND: the height amplitude and the ground level ase-terrain derives every
    // height from. Their spans are ase-pl-flora's to check; here only that both are finite numbers.
    if (!read_number(ground, "height_scale_m", start_.ground.height_scale_m) ||
        !std::isfinite(start_.ground.height_scale_m)) {
        return fail_config("ground.height_scale_m");
    }
    if (!read_number(ground, "base_height_m", start_.ground.base_height_m) ||
        !std::isfinite(start_.ground.base_height_m)) {
        return fail_config("ground.base_height_m");
    }
    return true;
}

bool VivariumView::load_brooks(const ::godot::Dictionary& root) {
    // THE BROOKS OF THE SCENE (03.8): the two ends of each bed, its half width and depth, its base flow
    // and catchment are data here; ase-terrain carves the bed, ase-hydro lets the water run. A scene may
    // have none - an empty list lays no brook.
    const ::godot::Variant brooks_value = root.get("brooks", ::godot::Variant());
    if (brooks_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("brooks");
    }
    const ::godot::Array brooks = brooks_value;
    start_.brooks.clear();
    for (int64_t i = 0; i < brooks.size(); ++i) {
        if (brooks[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("brooks[]");
        }
        const ::godot::Dictionary entry = brooks[i];
        VivariumBrookSpec brook;
        if (!read_number(entry, "x", brook.x) || !std::isfinite(brook.x)) {
            return fail_config("brooks[].x");
        }
        if (!read_number(entry, "z", brook.z) || !std::isfinite(brook.z)) {
            return fail_config("brooks[].z");
        }
        if (!read_number(entry, "end_x", brook.end_x) || !std::isfinite(brook.end_x)) {
            return fail_config("brooks[].end_x");
        }
        if (!read_number(entry, "end_z", brook.end_z) || !std::isfinite(brook.end_z)) {
            return fail_config("brooks[].end_z");
        }
        if (!read_number(entry, "half_m", brook.half_m) || !(brook.half_m > 0.0)) {
            return fail_config("brooks[].half_m");
        }
        if (!read_number(entry, "depth_m", brook.depth_m) || !(brook.depth_m > 0.0)) {
            return fail_config("brooks[].depth_m");
        }
        if (!read_number(entry, "base_m3_s", brook.base_m3_s) || !std::isfinite(brook.base_m3_s) ||
            brook.base_m3_s < 0.0) {
            return fail_config("brooks[].base_m3_s");
        }
        if (!read_number(entry, "catchment_m2", brook.catchment_m2) || !std::isfinite(brook.catchment_m2) ||
            brook.catchment_m2 < 0.0) {
            return fail_config("brooks[].catchment_m2");
        }
        start_.brooks.push_back(brook);
    }
    return true;
}

}  // namespace ase::adp::godot

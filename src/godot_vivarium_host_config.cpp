/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HOST CONFIGURATION
 *
 * @file        godot_vivarium_host_config.cpp
 * @brief       VivariumView - the ports, operations, patches, ground and brooks the game host is booted with
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 and PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8: everything
 *              GodotHostResourceManager::boot needs beside the stage stands in the start configuration -
 *              the name of every port and operation the view reads or writes, the patches the game host
 *              lays, the measured square of the ground and its brooks, the pour and the catch-up steps.
 *              A missing or broken key stops the view with that key on its error panel; the manager
 *              checks the same configuration again before a host exists.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-10-18
 * @modified    2026-10-26
 * @version     00.00.01.00003
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/array.hpp>

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

bool VivariumView::load_host_config(const ::godot::Dictionary& root) {
    ::godot::String text;
    ::godot::Dictionary ports;
    if (!read_section(root, "ports", ports)) {
        return fail_config("ports");
    }
    if (!read_text(ports, "patch", text)) {
        return fail_config("ports.patch");
    }
    start_.patch_port = utf8_of(text);
    if (!read_text(ports, "clock", text)) {
        return fail_config("ports.clock");
    }
    start_.clock_port = utf8_of(text);
    if (!read_text(ports, "scene", text)) {
        return fail_config("ports.scene");
    }
    start_.scene_port = utf8_of(text);
    // The ground of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8): the square, the brooks and the
    // measured heights, one port each.
    if (!read_text(ports, "ground", text)) {
        return fail_config("ports.ground");
    }
    start_.ground_port = utf8_of(text);
    if (!read_text(ports, "brook", text)) {
        return fail_config("ports.brook");
    }
    start_.brook_port = utf8_of(text);
    if (!read_text(ports, "terrain", text)) {
        return fail_config("ports.terrain");
    }
    start_.terrain_port = utf8_of(text);
    // The cells of the ground (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2): ase-render-data names the
    // four corners of every grid square; the view hands them to the index buffer as they are.
    if (!read_text(ports, "ground_cell", text)) {
        return fail_config("ports.ground_cell");
    }
    start_.ground_cell_port = utf8_of(text);
    // The organ templates (usecase:101011 tessellate the organ templates): ase-render-data lays the corners and the
    // triangles of the fine and the coarse cylinder and of the unit leaf; the view builds its meshes from
    // them and computes no ring, no normal and no winding of its own.
    if (!read_text(ports, "template_corner", text)) {
        return fail_config("ports.template_corner");
    }
    start_.template_corner_port = utf8_of(text);
    if (!read_text(ports, "template_triangle", text)) {
        return fail_config("ports.template_triangle");
    }
    start_.template_triangle_port = utf8_of(text);
    // The sky and the light of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3): the one port
    // of ase-pl-sky; the view lights the care scene from it and computes no light of its own.
    if (!read_text(ports, "sky", text)) {
        return fail_config("ports.sky");
    }
    start_.sky_port = utf8_of(text);
    // The seed bodies of ase-physics (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 4, register N0173): the
    // view draws every seed where the port says and computes none of its flight.
    if (!read_text(ports, "seed", text)) {
        return fail_config("ports.seed");
    }
    start_.seed_port = utf8_of(text);
    // The bark tiling of the internodes (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2, the material data of
    // a section): ase-render-data computed it, the view scales the bark texture by it and computes none.
    if (!read_text(ports, "tile", text)) {
        return fail_config("ports.tile");
    }
    start_.tile_port = utf8_of(text);
    // The level requests of the player (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 point 2): a port of their
    // own, the patch port takes only pours.
    if (!read_text(ports, "level", text)) {
        return fail_config("ports.level");
    }
    start_.level_port = utf8_of(text);
    // The flatten requests of the player at a place (03.8): a port of their own, they name no patch.
    if (!read_text(ports, "flatten", text)) {
        return fail_config("ports.flatten");
    }
    start_.flatten_port = utf8_of(text);
    // The pace requests of the debug mode (the time lapse of this host's clock): a port of their own,
    // they name no object.
    if (!read_text(ports, "pace", text)) {
        return fail_config("ports.pace");
    }
    start_.pace_port = utf8_of(text);
    // The place of the view's camera (03.11 point 7, the level of detail): a port of its own, it names no
    // object.
    if (!read_text(ports, "view", text)) {
        return fail_config("ports.view");
    }
    start_.view_port = utf8_of(text);
    // One port per organ class, keyed by the class's word (GodotHostResourceManager::organ_word).
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        if (!read_text(ports, GodotHostResourceManager::organ_word(c), text)) {
            return fail_config("ports.<organ>");
        }
        start_.organ_ports[c] = utf8_of(text);
    }
    // One instance port per drawn class, keyed by the class's word and GODOT_INSTANCE_SUFFIX (03.11 step 2).
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        const std::string key = std::string(GodotHostResourceManager::organ_word(c)) + GODOT_INSTANCE_SUFFIX;
        if (!read_text(ports, key.c_str(), text)) {
            return fail_config("ports.<organ>_instance");
        }
        start_.instance_ports[c] = utf8_of(text);
    }

    ::godot::Dictionary operations;
    if (!read_section(root, "operations", operations)) {
        return fail_config("operations");
    }
    if (!read_text(operations, "irrigate", text)) {
        return fail_config("operations.irrigate");
    }
    start_.op_irrigate = utf8_of(text);
    if (!read_text(operations, "prune", text)) {
        return fail_config("operations.prune");
    }
    start_.op_prune = utf8_of(text);
    if (!read_text(operations, "level", text)) {
        return fail_config("operations.level");
    }
    start_.op_level = utf8_of(text);
    if (!read_text(operations, "flatten", text)) {
        return fail_config("operations.flatten");
    }
    start_.op_flatten = utf8_of(text);
    if (!read_text(operations, "pace", text)) {
        return fail_config("operations.pace");
    }
    start_.op_pace = utf8_of(text);
    if (!read_text(operations, "view", text)) {
        return fail_config("operations.view");
    }
    start_.op_view = utf8_of(text);
    if (!read_text(operations, "lay", text)) {
        return fail_config("operations.lay");
    }
    start_.op_lay = utf8_of(text);
    if (!load_patches(root)) {
        return false;  // the failing key stands on the error panel already
    }
    if (!load_ground(root)) {
        return false;
    }
    if (!load_brooks(root)) {
        return false;
    }

    if (!read_number(root, "irrigate_amount_mm", start_.irrigate_amount_mm) ||
        !(start_.irrigate_amount_mm > 0.0)) {
        return fail_config("irrigate_amount_mm");
    }
    double steps = 0.0;
    if (!read_number(root, "catch_up_steps", steps) || !(steps >= 1.0) ||
        ase::math::floor(static_cast<float>(steps)) != static_cast<float>(steps)) {
        return fail_config("catch_up_steps");
    }
    start_.catch_up_steps = static_cast<uint32_t>(steps);
    return true;
}

bool VivariumView::load_patches(const ::godot::Dictionary& root) {
    // THE PATCHES OF THE START CONFIGURATION (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, "Fläche und Art"):
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

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW SCENE
 *
 * @file        godot_vivarium_scene.cpp
 * @brief       VivariumView - which part of the 3D scene a frame fills from the ports
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 points 3 and 5: the scene stands once
 *              (build_plant_scene, godot_plant_scene.hpp) and follow_scene fills it frame by frame from
 *              what the ports carry. The ground is rebuilt when the terrain port changed; the plant when
 *              the organ ports or the template ports changed (usecase:101011 lay out organ templates), the
 *              frame time moved the line between fine and coarse cylinder (03.6, V8) or a patch record
 *              moved its stem base; the light and the seeds every frame. Nothing grows here, no matrix
 *              and no template is computed and no height is looked up: the instance ports carry the
 *              matrices, the template ports the meshes, the patch records the stem bases.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ase::adp::godot {

namespace {

/**
 * The stem bases stand where they stood: the same patches with the same three numbers. Exact equality,
 * not a tolerance - both sides are read from the same port values, the last placed and this frame's.
 */
bool same_origins(const std::unordered_map<uint32_t, PlantOrigin>& now,
                  const std::unordered_map<uint32_t, PlantOrigin>& before) {
    if (now.size() != before.size()) {
        return false;
    }
    for (const auto& [body, origin] : now) {
        const auto was = before.find(body);
        if (was == before.end() || was->second.x != origin.x || was->second.y != origin.y ||
            was->second.z != origin.z) {
            return false;
        }
    }
    return true;
}

}  // namespace

void VivariumView::follow_scene(double elapsed_s) {
    if (scene_ == nullptr || start_.patches.empty()) {
        return;
    }
    const bool orders_moved = order_budget_sample(order_budget_, elapsed_s * GODOT_MS_PER_SECOND);
    const uint64_t terrain = resources_.terrain_generation();
    const bool ground_moved = terrain != scene_terrain_;
    if (ground_moved) {
        scene_terrain_ = terrain;
        fill_plant_ground(scene_, resources_.get_terrain(), resources_.get_ground_cells(), ground_material());
    }
    // the stem base of every configured patch, as its record carries it (plant_origins reads it)
    std::vector<VivariumPatchView> patches;
    for (const VivariumPatchSpec& spec : start_.patches) {
        const VivariumPatchView* patch = resources_.get_patch_at(spec.place);
        if (patch != nullptr) {
            patches.push_back(*patch);
        }
    }
    std::unordered_map<uint32_t, PlantOrigin> origins = plant_origins(patches);
    const bool stood_moved = !same_origins(origins, scene_origins_);
    const uint64_t organs = resources_.organ_generation();
    // the templates arrive with the plugin's first Production run and stay; their generation changes then
    // and with every new host (usecase:101011 lay out organ templates)
    const uint64_t templates = resources_.template_generation();
    if (organs != scene_organs_ || templates != scene_templates_ || orders_moved || stood_moved) {
        scene_organs_ = organs;
        scene_templates_ = templates;
        scene_origins_ = std::move(origins);
        fill_plant_organs(scene_, resources_, scene_origins_, order_budget_.shown, scene_look_);
        scene_focus_ = GODOT_NO_PLACE;  // the stem bases may have moved: aim again
    }
    // select a patch (03.1 point 2): the camera moves to the plant on the selected place, without a
    // selection to the first place of the configuration (godot_patch_focus.hpp).
    const uint32_t focus = patch_focus_place(selected_place_, start_.patches.front().place);
    const VivariumPatchView* focused = resources_.get_patch_at(focus);
    if (focus != scene_focus_ && focused != nullptr) {
        const auto origin = scene_origins_.find(focused->object_id);
        if (origin != scene_origins_.end()) {
            aim_plant_camera(scene_, scene_camera_, origin->second);
            fill_plant_ground(scene_, resources_.get_terrain(), resources_.get_ground_cells(), ground_material());
            scene_focus_ = focus;
        }
    }
    // soil water and condition to the materials (03.2), every frame as the records carry them: the
    // ground takes the focused patch's soil water, each plant's leaves the condition of its patch
    const std::unordered_map<uint32_t, PlantStand> stands = plant_stands(patches);
    const auto ground = focused == nullptr ? stands.end() : stands.find(focused->object_id);
    mark_plant_stands(scene_, stands, ground == stands.end() ? PlantStand{} : ground->second);
    light_plant_scene(scene_, resources_.get_sky());
    // the seeds every frame (register N0173): ase-physics moves a flying seed in every kinetic step, the seed
    // port carries where it is, and the view places it at the stem base of its plant without moving it
    fill_plant_seeds(scene_, resources_, scene_origins_, scene_look_);
}

::godot::Ref<::godot::Material> VivariumView::ground_material() const {
    // The ground of the focused patch (03.2): the material of the role its soil code names, bound once
    // (bind_plant_look); a place without a record, or with a soil code the view does not know, shows
    // bare soil - it names no role to bind. The focus is the camera's (godot_patch_focus.hpp).
    const uint32_t focus = patch_focus_place(selected_place_, start_.patches.front().place);
    const VivariumPatchView* patch = resources_.get_patch_at(focus);
    if (patch == nullptr || patch->soil >= GODOT_SOIL_COUNT) {
        return scene_look_.soil;
    }
    return scene_look_.ground[patch->soil];
}

}  // namespace ase::adp::godot

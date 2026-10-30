/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW SCENE
 *
 * @file        godot_vivarium_scene.cpp
 * @brief       VivariumView - which part of the 3D scene a frame fills from the ports
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 points 3 and 5: the scene stands once
 *              (build_plant_scene, godot_plant_scene.hpp) and follow_scene fills it frame by frame from
 *              what the ports carry. The ground is rebuilt when the terrain port changed; the plant when
 *              the organ ports or the template ports changed (usecase:101011 tessellate the organ templates), the
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
 * @modified    2026-10-28
 * @version     00.00.02.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <cmath>
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

/**
 * What the scene draws, in one line: the ground samples and cells, the internodes and leaves, and the
 * light of the sky port. The window acceptance reads it (register D1647) - a black window and a drawn one
 * show apart only here, not in the patch values.
 */
::godot::String scene_counts(const GodotHostResourceManager& resources) {
    const VivariumSkyView& sky = resources.get_sky();
    return "ground_samples=" + ::godot::String::num_uint64(resources.get_terrain().size()) +
           " ground_cells=" + ::godot::String::num_uint64(resources.get_ground_cells().size()) +
           " segments=" + ::godot::String::num_uint64(resources.get_organs(GODOT_ORGAN_SEGMENT).size()) +
           " leaves=" + ::godot::String::num_uint64(resources.get_organs(GODOT_ORGAN_LEAF).size()) +
           " sky_known=" + (sky.known ? "1" : "0") +
           " sun_intensity=" + ::godot::String::num(sky.sun_intensity, GODOT_LOG_DECIMALS) +
           " ambient_intensity=" + ::godot::String::num(sky.ambient_intensity, GODOT_LOG_DECIMALS) +
           // the start picture stood red and dim at game hour 15 (run 20261028-phase03-v52, grey 36.2):
           // elevation and colours of the sky port say whether the port or the picture carries it
           " sun_up=" + ::godot::String::num(sky.sun_y, GODOT_LOG_DECIMALS) +
           " sun_rgb=" + ::godot::String::num(sky.sun_r, GODOT_LOG_DECIMALS) + "/" +
           ::godot::String::num(sky.sun_g, GODOT_LOG_DECIMALS) + "/" + ::godot::String::num(sky.sun_b, GODOT_LOG_DECIMALS) +
           " sky_rgb=" + ::godot::String::num(sky.background_r, GODOT_LOG_DECIMALS) + "/" +
           ::godot::String::num(sky.background_g, GODOT_LOG_DECIMALS) + "/" +
           ::godot::String::num(sky.background_b, GODOT_LOG_DECIMALS);
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
        fill_ground();
        // an event: the terrain port changed (a levelling, a carved bed), never once per frame
        note("scene ground " + scene_counts(resources_) + " " + measure(GODOT_NO_PLACE));
    }
    // an event of the clock: a new game hour of the day. The ground and organ lines come with a rebuild
    // only, so without this line the scene of noon and of the night stood in no line at all (run
    // 20261028-phase03-v52: `day`, `night picture` and `scene` found no line after the first second).
    const VivariumClockView& clock = resources_.get_clock();
    if (clock.known && std::isfinite(clock.hour)) {
        const int64_t hour = static_cast<int64_t>(ase::math::floor(static_cast<float>(clock.hour)));
        if (hour != scene_hour_) {
            scene_hour_ = hour;
            note("scene hour " + scene_counts(resources_) + " " + measure(GODOT_NO_PLACE));
        }
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
    // and with every new host (usecase:101011 tessellate the organ templates)
    const uint64_t templates = resources_.template_generation();
    if (organs != scene_organs_ || templates != scene_templates_ || orders_moved || stood_moved) {
        // the organ views are read anew with every Dissemination run, so only the first fill after a
        // boot gets a line; later counts stand in the next `scene ground` line
        if (scene_organs_ == 0u) {
            note("scene organs " + scene_counts(resources_) + " " + measure(GODOT_NO_PLACE));
        }
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
            const PlantSceneCamera zoomed = patch_focus_zoomed(scene_camera_, zoom_);
            aim_plant_camera(scene_, zoomed, origin->second);
            // the engine learns where the camera stands (03.11 point 7): the level of detail of
            // ase-render-data measures from it; aiming happens on a new focus or zoom only
            const PatchFocusAim aim = patch_focus_aim(zoomed, origin->second);
            const ase::kernel::HostStatus viewed = resources_.view(aim.eye_x, aim.eye_y, aim.eye_z);
            note("view eye " + ::godot::String::num(aim.eye_x, 2) + " " + ::godot::String::num(aim.eye_y, 2) + " " +
                 ::godot::String::num(aim.eye_z, 2) + " status " + ::godot::String::num_int64(viewed));
            fill_ground();
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

void VivariumView::fill_ground() {
    // The state of the ground the shader paints (GODOT_GROUND_SHADER_CODE): the disc of every patch whose
    // record carries its levelled height - a patch not levelled yet marks nothing - and the bed of every
    // brook of the start configuration and of every bed the player laid (godot_vivarium_bed.cpp); the grid
    // at the distance of two height samples.
    std::vector<PlantGroundDisc> discs;
    for (const VivariumPatchSpec& spec : start_.patches) {
        const VivariumPatchView* patch = resources_.get_patch_at(spec.place);
        if (patch == nullptr || !std::isfinite(patch->x) || !std::isfinite(patch->y) || !std::isfinite(patch->z)) {
            continue;
        }
        discs.push_back(PlantGroundDisc{static_cast<float>(patch->x), static_cast<float>(patch->z),
                                        static_cast<float>(spec.half_m), static_cast<float>(patch->y)});
    }
    // a disc the player levelled stands at the height of the ground at its centre: the sample nearest it
    for (const PlantGroundDisc& flat : player_flats_) {
        float best = -1.0f;
        PlantGroundDisc disc = flat;
        for (const VivariumTerrainView& sample : resources_.get_terrain()) {
            const float dx = static_cast<float>(sample.x) - flat.x;
            const float dz = static_cast<float>(sample.z) - flat.z;
            if (best < 0.0f || dx * dx + dz * dz < best) {
                best = dx * dx + dz * dz;
                disc.height = static_cast<float>(sample.height);
            }
        }
        if (best >= 0.0f) {
            discs.push_back(disc);
        }
    }
    std::vector<PlantGroundBed> beds;
    for (const std::vector<VivariumBrookSpec>* laid : {&start_.brooks, &player_beds_}) {
        for (const VivariumBrookSpec& brook : *laid) {
            beds.push_back(PlantGroundBed{static_cast<float>(brook.x), static_cast<float>(brook.z),
                                          static_cast<float>(brook.end_x), static_cast<float>(brook.end_z),
                                          static_cast<float>(brook.half_m)});
        }
    }
    fill_plant_ground(scene_, resources_.get_terrain(), resources_.get_ground_cells(), discs, beds,
                      static_cast<float>(start_.ground.step_m), ground_material());
}

}  // namespace ase::adp::godot

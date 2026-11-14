/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW SCENE
 *
 * @file        godot_vivarium_scene.cpp
 * @brief       VivariumView - which part of the 3D scene a frame fills from the ports
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 points 3 and 5: the scene stands once
 *              (build_plant_scene, godot_plant_scene.hpp) and follow_scene fills it frame by frame from
 *              what the ports carry. The ground is rebuilt when the terrain port changed; the plant when
 *              the organ ports or the template ports changed (usecase:101011 tessellate the organ templates), the
 *              frame time moved the line between fine and coarse cylinder (03.6, V8) or the stem base moved;
 *              the light and the seeds every frame. Nothing grows here, no matrix and no template is computed:
 *              the instance ports carry the matrices, the template ports the meshes, the patch records the place
 *              of the stem base. Its height is the ground's (B0304): the terrain port's height at the stem base,
 *              read on the lattice of the start square (UiGround), so a hole dug under a plant takes the plant
 *              down with the ground. A seed stands at the stem base plus the place the seed port carries, all three
 *              axes as ase-physics holds them; its height is the physics' ground contact, never a ground height
 *              read here (D2312); that contact measures a hole dug into the levelled disc too (D2315). The line that counts
 *              what the scene draws stands in godot_vivarium_scene_counts.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0487 (the stem base as a body its owner reports through its port), carrier N0448
 * @created     2026-10-18
 * @modified    2026-11-13
 * @version     00.00.02.00022
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_plant_brook.hpp>
#include <ase/adp/godot/godot_plant_brook_reflection.hpp>
#include <ase/adp/godot/godot_plant_ground_season.hpp>
#include <ase/adp/godot/godot_plant_horizon_ridge.hpp>
#include <ase/adp/godot/godot_plant_leaf_season.hpp>
#include <ase/adp/godot/godot_vivarium_scene_counts.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/vector3.hpp>

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

}  // namespace

void VivariumView::follow_scene(double elapsed_s) {
    if (scene_ == nullptr || start_.patches.empty()) {
        return;
    }
    const bool orders_moved = order_budget_sample(order_budget_, elapsed_s * GODOT_MS_PER_SECOND);
    const uint64_t terrain = resources_.terrain_generation();
    const bool ground_moved = terrain != scene_terrain_;
    // the main thread's time for laying ground and water, in the line of the event: the frame after a dig holds
    // it (P0690, GODOT_GROUND_WATCH_BUDGET_USEC)
    ::godot::Time* ticks = ::godot::Time::get_singleton();
    if (ground_moved) {
        scene_terrain_ = terrain;
        const uint64_t fill_start_usec = ticks->get_ticks_usec();
        fill_ground();
        const uint64_t fill_usec = ticks->get_ticks_usec() - fill_start_usec;
        // an event: the terrain port changed (a levelling, a carved bed), never once per frame
        note("scene ground " + scene_counts(resources_, scene_) + " fill_usec=" + ::godot::String::num_uint64(fill_usec) +
             " " + measure(GODOT_NO_PLACE));
    }
    // the water of the brooks (usecase:100399 carry brook water): in every bed at its bed plus the depth ase-hydro
    // gives it, and beside it at the groundwater level ase-hydro gives that brook (build_water_table); laid anew
    // when ase-hydro changed a stream or the ground under the water moved, never once per frame
    const uint64_t streams = resources_.stream_generation();
    if (ground_moved || streams != scene_streams_) {
        scene_streams_ = streams;
        // the edge of the bridge for the groundwater beside a brook (N0484): the stream port carries no level
        // until ase-hydro publishes one; logged once on its edge, the water stays in the beds meanwhile
        constexpr const char* GAP_GROUNDWATER = "N0484";
        constexpr const char* GROUNDWATER_FIELD = "flora.brook.v1.groundwater_level_m";
        bool groundwater = false;
        for (const VivariumStreamView& stream : resources_.get_streams()) {
            groundwater = groundwater || std::isfinite(stream.groundwater_level_m);
        }
        if (groundwater) {
            close_gap(GAP_GROUNDWATER, GROUNDWATER_FIELD);
        } else if (!resources_.get_streams().empty()) {
            open_gap(GAP_GROUNDWATER, GROUNDWATER_FIELD);
        }
        const uint64_t water_start_usec = ticks->get_ticks_usec();
        // the ground as fill_ground just left it: the table reads its vertices, never the ports by object id
        const uint32_t wet = fill_plant_water(scene_, resources_.get_streams(), ground_surface_.arrays);
        const uint64_t water_usec = ticks->get_ticks_usec() - water_start_usec;
        note("scene water cells " + ::godot::String::num_int64(static_cast<int64_t>(wet)) + " brooks " +
             ::godot::String::num_int64(static_cast<int64_t>(resources_.get_streams().size())) +
             " fill_usec=" + ::godot::String::num_uint64(water_usec));
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
    // BRIDGE GAP (register N0487, carrier N0448): the view sets the stem base onto the terrain height at the stem (B0304), so a
    // hole dug under the birch takes it down. That is the plant's own place computed here, which the owner of the
    // plant has to report through its port as the body ase-physics holds; the seed port already does so (D2312).
    // Until the patch port carries that height, the terrain at the stem stands in on the lattice and the record's
    // level off it. This is a bridge, never evidence that the plant stands on the ground; its edge is logged once
    hud_ground_.follow(resources_.get_terrain(), start_.ground, resources_.terrain_generation());
    if (!origins.empty()) {
        open_gap(VivariumCarryBridge::GAP_BODY, "flora.patch.v1.base_y");  // stem base as the owner's body (N0487)
    }
    for (auto& origin : origins) {
        double ground = 0.0;
        if (hud_ground_.height(origin.second.x, origin.second.z, ground)) {
            origin.second.y = static_cast<float>(ground);
        }
    }
    const bool stood_moved = !same_origins(origins, scene_origins_);
    const uint64_t organs = resources_.organ_generation();
    // the templates arrive with the plugin's first Production run and stay; their generation changes then
    // and with every new host (usecase:101011 tessellate the organ templates)
    const uint64_t templates = resources_.template_generation();
    // the bark tubes are laid anew only when the bark port carried a new set, which ase-render-data builds only
    // when a body ticked (register B0295 point 4, usecase:101613)
    const uint64_t bark = resources_.bark_generation();
    if (organs != scene_organs_ || templates != scene_templates_ || bark != scene_bark_ || orders_moved ||
        stood_moved) {
        // the organ views are read anew with every Dissemination run, so only the first fill after a
        // boot gets a line; later counts stand in the next `scene ground` line
        if (scene_organs_ == 0u) {
            note("scene organs " + scene_counts(resources_, scene_) + " " + measure(GODOT_NO_PLACE));
        }
        // the edge of the grounded stem base (B0304): the plant moved because the ground under it moved, told
        // once with its new height, so a picture of a dug hole names the level the birch stands on
        if (stood_moved && !scene_origins_.empty()) {
            for (const auto& [body, origin] : origins) {
                note("scene stem base body=" + ::godot::String::num_uint64(body) +
                     " ground_y=" + ::godot::String::num(origin.y, GODOT_LOG_DECIMALS));
            }
        }
        scene_organs_ = organs;
        scene_templates_ = templates;
        scene_bark_ = bark;
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
            // the engine learns where the camera stands and where it looks (03.11 point 7, 07.4): the level
            // of detail of ase-render-data measures from the eye, the stick of the figure steers along the
            // heading from eye to target; aiming happens on a new focus or zoom only
            const PatchFocusAim aim = patch_focus_aim(zoomed, origin->second);
            const ase::kernel::HostStatus viewed =
                resources_.view(aim.eye_x, aim.eye_y, aim.eye_z, aim.target_x, aim.target_y, aim.target_z);
            note("view eye " + ::godot::String::num(aim.eye_x, 2) + " " + ::godot::String::num(aim.eye_y, 2) + " " +
                 ::godot::String::num(aim.eye_z, 2) + " target " + ::godot::String::num(aim.target_x, 2) + " " +
                 ::godot::String::num(aim.target_y, 2) + " " + ::godot::String::num(aim.target_z, 2) + " status " +
                 ::godot::String::num_int64(viewed));
            fill_ground();
            scene_focus_ = focus;
        }
    }
    // soil water and condition to the materials (03.2), every frame as the records carry them: the
    // ground takes the focused patch's soil water, each plant's leaves the condition of its patch
    const std::unordered_map<uint32_t, PlantStand> stands = plant_stands(patches);
    const auto ground = focused == nullptr ? stands.end() : stands.find(focused->object_id);
    mark_plant_stands(scene_, stands, ground == stands.end() ? PlantStand{} : ground->second);
    // an overcast sky turns the sky grey and dims the light (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N01): SkyOvercast
    // derives the shade from the sky frame's kind of weather and the sun's height. The frame carries a kind only
    // while the sky port's colours carry no tint of it (the bridge's weather, N0449); the weather of
    // flora.weather.v1 is already in them through ase-pl-sky, so the sky is tinted exactly once (D2280). The sky's
    // own colours stand from the first frame the configuration is loaded in (SkyOvercast is bound by load_hud),
    // not only once the HUD stands: before that the frame carries no kind, so the sky shows the mockup's colours
    // by the sun's height under an open sky, never the depth fog's grey of the sky port as its horizon
    //
    // the mockup's HOR, where its ridges stand and its ground rows begin (m.html 358): in the 3D scene the ground
    // ends at the far edge of the start square, so the ridges stand on that edge and the ground's rows count from it.
    // It is found first, since the fog's reach there decides how much of the fog colour the sky's horizon takes
    ::godot::Vector3 ground_edge;
    const bool edged = plant_ground_far_edge(scene_, start_.ground.x, start_.ground.z, 2.0 * start_.ground.half_m,
                                             ground_edge);
    SkyOvercastShade overcast;
    if (configured_) {
        const VivariumSkyView& sky = resources_.get_sky();
        hud_now_.sky.sun_y = sky.sun_y;
        // the fog record is the colour of the depth fog over the ground: the sky's horizon takes it only by the
        // share the fog reaches at the square's far edge, so the sky is grey in a fog phase and nowhere else
        hud_now_.sky.fog = ::godot::Color(static_cast<float>(sky.fog_r), static_cast<float>(sky.fog_g),
                                          static_cast<float>(sky.fog_b));
        hud_now_.sky.fog_share = edged ? plant_fog_reach(scene_, sky, ground_edge) : 0.0;
        // the halo around the sun is measured in mockup units (row N03): the frame's pixels per unit size it
        hud_now_.sky.frame_pixels = hud_frame_.get_scale().y;
        hud_.overcast().shade(hud_now_.sky, overcast);
    }
    light_plant_scene(scene_, resources_.get_sky(), overcast, edged ? &ground_edge : nullptr);
    // the two ridges of hills on the horizon (row N05, m.html drawSky 422 to 424): the horizon's colour as the sky
    // shader draws it, the land's light (SkyOvercast::land_light over the sun's height and the overcast dimming) and
    // the start relief of the landscape (the start file's ground height scale), in the HUD frame's units. The ridges are the far scenery: the RELEVO debug chip
    // reshapes the square under the camera and never the horizon. Before the HUD stands the shade shows no ridges
    HorizonRidgeShade ridge;
    if (hud_live()) {
        HorizonRidgeFrame horizon;
        horizon.horizon = overcast.sky_horizon.lerp(overcast.horizon, overcast.mix);
        horizon.land_light = hud_.overcast().land_light(resources_.get_sky().sun_y, overcast.light);
        horizon.relief = start_.ground.height_scale_m;
        horizon.frame_pixels = hud_frame_.get_scale().y;
        horizon.frame_origin_x = hud_frame_.get_origin().x;
        hud_ridge_.shade(horizon, ridge);
    }
    light_plant_horizon_ridge(scene_, ridge, edged ? &ground_edge : nullptr);
    // the ground in the colour of its season (row N06, m.html groundCol): the clock's season where flora.day.v1
    // names it, else the mockup's (hud_bridge_clock_season, N0460), the frame's kind of weather for the snow and the
    // same horizon colour the ridges take. Before the HUD stands the shade leaves the cover as its soils paint it
    GroundSeasonShade ground_shade;
    if (hud_live()) {
        const VivariumClockView& clock = resources_.get_clock();
        GroundSeasonFrame season;
        season.season = static_cast<int32_t>(ase::math::floor(
            static_cast<float>(clock.day_known ? clock.season : hud_bridge_clock_.season)));
        season.weather = hud_now_.sky.kind;
        season.horizon = overcast.sky_horizon.lerp(overcast.horizon, overcast.mix);
        season.frame_top = hud_frame_.get_origin().y;
        season.frame_pixels = hud_frame_.get_scale().y;
        std::string failed;
        if (!hud_ground_season_.shade(season, ground_shade, failed)) {
            fail_hud("ground season " + failed);
        }
    }
    light_plant_ground_season(scene_, ground_shade, edged ? &ground_edge : nullptr);
    // the leaf in the colour of its season (register B0278, m.html leafCol 486): the same season the ground takes, and
    // the olive a leaf turns toward while its stand wilts; the shader lays the olive only where the leaves' instance
    // uniform condition names a wilting stand. Before the HUD stands the shade leaves the leaf as its material paints it
    LeafSeasonShade leaf_shade;
    if (hud_live()) {
        const VivariumClockView& clock = resources_.get_clock();
        LeafSeasonFrame leaf;
        leaf.season = static_cast<int32_t>(ase::math::floor(
            static_cast<float>(clock.day_known ? clock.season : hud_bridge_clock_.season)));
        std::string failed;
        if (!hud_leaf_season_.shade(leaf, leaf_shade, failed)) {
            fail_hud("leaf season " + failed);
        }
    }
    light_plant_leaf_season(leaf_shade);
    // the brook with the sky it reflects and the strokes of its current (row N07, m.html drawWater 429 to 433): the
    // horizon's and the sky's colours as the sky shader draws them under the overcast, the HUD frame's pixels per
    // mockup unit for the strokes' width and the first stream ase-hydro carries for their course. Before the HUD
    // stands, or while no stream is carried, the shade leaves the water as its own colours paint it
    BrookReflectionShade brook_shade;
    if (hud_live() && !resources_.get_streams().empty()) {
        BrookReflectionFrame brook;
        brook.horizon = overcast.sky_horizon.lerp(overcast.horizon, overcast.mix);
        brook.above = overcast.sky_zenith.lerp(overcast.zenith, overcast.mix);
        brook.frame_pixels = hud_frame_.get_scale().y;
        brook.stream = &resources_.get_streams().front();
        std::string failed;
        if (!hud_brook_reflection_.shade(brook, brook_shade, failed)) {
            fail_hud("brook reflection " + failed);
        }
    }
    light_plant_brook_reflection(brook_shade);
    // THE LINE DESCRIBES THE FRAME AS IT IS DRAWN (register B0165). It stands after the camera was aimed
    // and after light_plant_scene laid the starfield and the moons of this frame's sky port; written
    // before them, it counted the sky of the frame before.
    // An event of the clock: a new game hour of the day. The ground and organ lines come with a rebuild
    // only, so without this line the scene of noon and of the night stood in no line at all (run
    // 20261028-phase03-v52: `day`, `night picture` and `scene` found no line after the first second).
    const VivariumClockView& clock = resources_.get_clock();
    if (clock.known && std::isfinite(clock.hour)) {
        const int64_t hour = static_cast<int64_t>(ase::math::floor(static_cast<float>(clock.hour)));
        if (hour != scene_hour_) {
            scene_hour_ = hour;
            note("scene hour " + scene_counts(resources_, scene_) + " " + measure(GODOT_NO_PLACE));
        }
    }
    // An event of the sky port: a new star set. It arrives within an hour as often as at its start, and
    // a night whose stars came after the hour line otherwise stood in no line with its stars.
    const uint64_t stars = resources_.get_sky().star_generation;
    if (stars != scene_stars_) {
        scene_stars_ = stars;
        note("scene sky " + scene_counts(resources_, scene_) + " " + measure(GODOT_NO_PLACE));
    }
    // the seeds every frame (register N0173): ase-physics moves a flying seed in every kinetic step, the seed
    // port carries where it is in the frame of the stem foot, and the view places it at the stem base of its
    // plant plus that place without moving it. Its height is the one ase-physics holds (D2312): the body's ground
    // contact measures the terrain sample, over the levelled disc too, so a dug hole takes it down (D2315), and the
    // view never reads a ground height of its own for it
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
    // The state of the ground the shader paints (GODOT_GROUND_SHADER_FILE): the disc of every patch whose
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
                      static_cast<float>(start_.ground.step_m), ground_material(), ground_surface_);
}

}  // namespace ase::adp::godot

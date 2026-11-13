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
 * @modified    2026-11-13
 * @version     00.00.02.00019
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
#include <ase/adp/godot/godot_scene_exposure.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/transform3d.hpp>
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

/**
 * What the scene draws, in one line: the ground samples and cells, the internodes and leaves, and the
 * light of the sky port. The window acceptance reads it (register D1647) - a black window and a drawn one
 * show apart only here, not in the patch values.
 */
::godot::String scene_counts(const GodotHostResourceManager& resources, ::godot::Node3D* scene) {
    const VivariumSkyView& sky = resources.get_sky();
    // what of the sky the camera frames: a star or moon direction stands at GODOT_SKY_DISC_DISTANCE from the
    // middle of the sky dome (sky_dome_centre, godot_plant_sky_disc.hpp), where light_plant_stars and
    // stand_sky_disc put it, and counts when that point lies in the frustum. The middle is read from the
    // starfield node, which light_plant_stars stands on it every frame; before the first sky it is the eye.
    // The night pictures of run frames-20261106-181311 showed no star while the port carried 64: the
    // records, the framed ones and the drawn instances side by side say which link of the chain drops them.
    // A star record holds its place on the celestial sphere; the turn record of the sky port turns it into the
    // world as the starfield node does, so the world direction here is the same the picture draws
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    const ::godot::Node3D* dome_node =
        ::godot::Object::cast_to<::godot::Node3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_STARS)));
    const ::godot::Vector3 dome = dome_node != nullptr ? dome_node->get_position()
                                                       : (eye != nullptr ? eye->get_position() : ::godot::Vector3());
    const ::godot::Basis turn = sky.turn_known
        ? ::godot::Basis(::godot::Quaternion(static_cast<float>(sky.turn_x), static_cast<float>(sky.turn_y),
                                             static_cast<float>(sky.turn_z), static_cast<float>(sky.turn_w)).normalized())
        : ::godot::Basis();
    uint32_t stars_framed = 0u;
    uint32_t moons_framed = 0u;
    double star_low = 0.0;
    double star_high = 0.0;
    const uint32_t held = static_cast<uint32_t>(sky.star.size());
    const uint32_t stars = held < GODOT_SKY_STARS_MAX ? held : GODOT_SKY_STARS_MAX;
    ::godot::Vector3 first_world(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0u; i < stars; ++i) {
        const VivariumStarView& star = sky.star[i];
        const ::godot::Vector3 world =
            turn.xform(::godot::Vector3(static_cast<float>(star.x), static_cast<float>(star.y), static_cast<float>(star.z)));
        first_world = i == 0u ? world : first_world;
        star_low = i == 0u || world.y < star_low ? world.y : star_low;
        star_high = i == 0u || world.y > star_high ? world.y : star_high;
        if (eye != nullptr && sky.turn_known &&
            eye->is_position_in_frustum(dome + world * GODOT_SKY_DISC_DISTANCE)) {
            ++stars_framed;
        }
    }
    // every moon's azimuth and altitude in degrees, so the picture and the line say together where each moon
    // stands against the fixed camera, so the probe shows whether the simulation brings it through the frame.
    // The azimuth reads the sky encoding of ase-ephemeris (ephemeris_hrzn_math.hpp: x = cos(az) * across,
    // z = sin(az) * across); while EPH_CONST_GRID_BEARING_DEG puts +z to the east it is the compass azimuth,
    // clockwise from north, east at 90
    ::godot::String moon_dirs;
    const uint32_t moons = sky.moons < GODOT_SKY_MOONS_MAX ? sky.moons : GODOT_SKY_MOONS_MAX;
    for (uint32_t i = 0u; i < moons; ++i) {
        const VivariumMoonView& moon = sky.moon[i];
        const float moon_up = static_cast<float>(moon.y) < -1.0f ? -1.0f : (static_cast<float>(moon.y) > 1.0f ? 1.0f : static_cast<float>(moon.y));
        float moon_az = ase::math::atan2(static_cast<float>(moon.z), static_cast<float>(moon.x)) * ase::math::RAD_TO_DEG;
        moon_az = moon_az < 0.0f ? moon_az + 360.0f : moon_az;
        moon_dirs += (i == 0u ? "" : ",") + ::godot::String::num(moon_az, 0) + "/" +
                     ::godot::String::num(ase::math::asin(moon_up) * ase::math::RAD_TO_DEG, 0);
        if (eye != nullptr &&
            eye->is_position_in_frustum(dome +
                                        ::godot::Vector3(static_cast<float>(moon.x), static_cast<float>(moon.y),
                                                         static_cast<float>(moon.z)) * GODOT_SKY_DISC_DISTANCE)) {
            ++moons_framed;
        }
    }
    // the sun as the moons: its azimuth and altitude in the same encoding and whether its disc point lies in the
    // frustum, so a picture without the sun (row N03, the halo) says whether the sky or the camera hides it
    const float sun_up = static_cast<float>(sky.sun_y) < -1.0f ? -1.0f : (static_cast<float>(sky.sun_y) > 1.0f ? 1.0f : static_cast<float>(sky.sun_y));
    float sun_az = ase::math::atan2(static_cast<float>(sky.sun_z), static_cast<float>(sky.sun_x)) * ase::math::RAD_TO_DEG;
    sun_az = sun_az < 0.0f ? sun_az + 360.0f : sun_az;
    const uint32_t sun_framed =
        eye != nullptr && sky.sun_y > 0.0 &&
                eye->is_position_in_frustum(dome +
                                            ::godot::Vector3(static_cast<float>(sky.sun_x), static_cast<float>(sky.sun_y),
                                                             static_cast<float>(sky.sun_z)) * GODOT_SKY_DISC_DISTANCE)
            ? 1u
            : 0u;
    // the starfield as it stands: its visible instances and the width of the first one in metres; a width of
    // 0 is a starfield that draws nothing although its count says otherwise
    int64_t drawn = 0;
    double first_width = 0.0;
    const ::godot::MultiMeshInstance3D* field =
        ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_STARS)));
    if (field != nullptr && field->get_multimesh().is_valid()) {
        drawn = field->get_multimesh()->get_visible_instance_count();
        if (drawn > 0) {
            first_width = field->get_multimesh()->get_instance_transform(0).basis.get_scale().x;
        }
    }
    // the world direction of the first star record, east and north share: between two lines of a night it
    // moves when the sky turns and stands when it does not
    const double first_x = static_cast<double>(first_world.x);
    const double first_z = static_cast<double>(first_world.z);
    const ::godot::String sky_frame =
        " stars_framed=" + ::godot::String::num_uint64(stars_framed) +
        " stars_drawn=" + ::godot::String::num_int64(drawn) +
        " star_width_m=" + ::godot::String::num(first_width, GODOT_LOG_DECIMALS) +
        " star_up=" + ::godot::String::num(star_low, GODOT_LOG_DECIMALS) + ".." +
        ::godot::String::num(star_high, GODOT_LOG_DECIMALS) +
        " star0=" + ::godot::String::num(first_x, GODOT_LOG_DECIMALS) + "/" + ::godot::String::num(first_z, GODOT_LOG_DECIMALS) +
        " moons=" + ::godot::String::num_uint64(sky.moons) +
        " moons_framed=" + ::godot::String::num_uint64(moons_framed) +
        " moon_az_alt=" + (moon_dirs.is_empty() ? ::godot::String("-") : moon_dirs) +
        " sun_framed=" + ::godot::String::num_uint64(sun_framed) +
        " sun_az_alt=" + ::godot::String::num(sun_az, 0) + "/" +
        ::godot::String::num(ase::math::asin(sun_up) * ase::math::RAD_TO_DEG, 0);
    // the relief the ground is drawn from: lowest and highest sample and the steepest normal; a flat
    // field (every height equal, every normal straight up) shows here before it shows in the picture
    double low = 0.0;
    double high = 0.0;
    double flattest = 1.0;
    bool first = true;
    for (const VivariumTerrainView& sample : resources.get_terrain()) {
        low = first || sample.height < low ? sample.height : low;
        high = first || sample.height > high ? sample.height : high;
        flattest = sample.ny < flattest ? sample.ny : flattest;
        first = false;
    }
    return "ground_samples=" + ::godot::String::num_uint64(resources.get_terrain().size()) +
           " ground_low=" + ::godot::String::num(low, GODOT_LOG_DECIMALS) +
           " ground_high=" + ::godot::String::num(high, GODOT_LOG_DECIMALS) +
           " ground_min_ny=" + ::godot::String::num(flattest, GODOT_LOG_DECIMALS) +
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
           ::godot::String::num(sky.background_b, GODOT_LOG_DECIMALS) +
           // the heat shimmer of the fog record: above 0 the haze quad reads the drawn frame and writes it back
           // over the whole picture, the sky included (veil_plant_scene) - a sky darker than sky_rgb says by this
           // field whether the veil stood over it (register B0184)
           " shimmer=" + ::godot::String::num(sky.fog_shimmer, GODOT_LOG_DECIMALS) +
           // the exposure lift the camera and the background take this frame (scene_exposure): a sky pixel is
           // the background colour times this lift, so the field says in which colour space it reached the
           // screen - sRGB-encoded, or linear without the encoding (register B0184)
           " lift=" + ::godot::String::num(scene_exposure(sky), GODOT_LOG_DECIMALS) +
           // the star set of the stars port, the instances of the starfield (light_plant_stars): a night
           // picture without stars says by this count whether the port carried none or the picture hid them
           " stars=" + ::godot::String::num_uint64(sky.star.size()) +
           " star_set=" + ::godot::String::num_uint64(sky.star_generation) +
           " sidereal=" + ::godot::String::num(sky.set_sidereal, GODOT_LOG_DECIMALS) + sky_frame;
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
    // the water table at NN0 (usecase:100399 carry brook water): laid anew when ase-hydro changed a stream or
    // the ground under the table moved, never once per frame
    const uint64_t streams = resources_.stream_generation();
    if (ground_moved || streams != scene_streams_) {
        scene_streams_ = streams;
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
    const bool stood_moved = !same_origins(origins, scene_origins_);
    const uint64_t organs = resources_.organ_generation();
    // the templates arrive with the plugin's first Production run and stay; their generation changes then
    // and with every new host (usecase:101011 tessellate the organ templates)
    const uint64_t templates = resources_.template_generation();
    if (organs != scene_organs_ || templates != scene_templates_ || orders_moved || stood_moved) {
        // the organ views are read anew with every Dissemination run, so only the first fill after a
        // boot gets a line; later counts stand in the next `scene ground` line
        if (scene_organs_ == 0u) {
            note("scene organs " + scene_counts(resources_, scene_) + " " + measure(GODOT_NO_PLACE));
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

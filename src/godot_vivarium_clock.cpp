/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW CLOCK MOMENTS
 *
 * @file        godot_vivarium_clock.cpp
 * @brief       VivariumView - the start screen over the initialisation, the measured game hour and clock rate, the jump feed
 * @description How the view brings this host's game clock to a moment, measured against the clock port and
 *              never carrying a constant of ase-time, and the start screen that stands before the clock runs.
 *
 *              THE START SCREEN (usecase:100387 view the plant) covers everything the engine loads: the relief,
 *              the patches, the brook beds, the organ templates, the scene and the pipelines of every material it
 *              shows. Under it the host runs only the ticks its loading needs, at the default pace: the embedded
 *              host ticks from real time (KernelEmbeddedHost::advance), and the generator, the beds and the
 *              templates are those ticks. No time lapse and no jump runs under it and nothing is tapped. It lifts
 *              onto the first complete frame, and from that frame on the view measures the clock - no time lapse
 *              and no jump starts by itself (operator order of 2026-11-06).
 *
 *              THE GAME HOUR AND THE RATE OF THE CLOCK are measured in every simulated frame: two readings of the
 *              clock port within one day give the game seconds of one game hour, and the clock's game seconds over
 *              the real seconds the host simulated between two frames give its rate at the running pace. The hour
 *              jump (godot_vivarium_debug.cpp, usecase:101120 clock the growth cycle) computes its target from the
 *              hour length and its feed from the rate.
 *
 *              THE FEED OF A JUMP (ARCH_ASE_KERNEL_EMBEDDED, section "Two Cases of Time"): during a jump the frame
 *              hands the host real seconds beyond its own and lets it run more ticks; every one of them is an
 *              ordinary tick of the client's clock, computed by every system, nothing loaded and nothing skipped.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_046
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @usecase     usecase:100390 - return to the garden
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Plant Keeper (PLNTKEEPER), Growth Clock Controller (GROWCLOCON)
 * @created     2026-11-06
 * @modified    2026-11-06
 * @version     00.00.03.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/rendering_server.hpp>

#include <cstdint>

namespace ase::adp::godot {

void VivariumView::follow_hour_length() {
    // THE LENGTH OF A GAME HOUR, measured in every simulated frame: two readings of the clock port within one
    // game day give the game seconds per hour. Across a day turn the hour falls back and the pair says nothing.
    // THE RATE OF THE CLOCK: the game seconds it ran over the real seconds the host simulated for them - the
    // running pace as the clock shows it, across a day turn too, since the game seconds only grow.
    if (!initialised_) {
        return;  // under the start screen the host runs its loading ticks; the clock is measured from the lift on
    }
    const VivariumClockView& clock = resources_.get_clock();
    if (!clock.known) {
        return;
    }
    const double rose = clock.hour - hour_length_.last_hour;
    const double passed = clock.elapsed_s - hour_length_.last_elapsed_s;
    if (hour_length_.last_hour >= 0.0 && clock.day == hour_length_.last_day && rose > 0.0 && passed > 0.0) {
        hour_length_.seconds_per_hour = passed / rose;
    }
    const double simulated_s = resources_.simulation_seconds();
    const double ran_s = simulated_s - hour_length_.last_simulated_s;
    if (hour_length_.last_elapsed_s >= 0.0 && hour_length_.last_simulated_s >= 0.0 && passed > 0.0 && ran_s > 0.0) {
        hour_length_.game_per_real = passed / ran_s;
    }
    hour_length_.last_elapsed_s = clock.elapsed_s;
    hour_length_.last_day = clock.day;
    hour_length_.last_hour = clock.hour;
    hour_length_.last_simulated_s = simulated_s;
}

uint32_t VivariumView::scene_parts_built() const {
    // The five parts of the scene the first picture needs: the ground of the latest relief, the beds of the
    // latest brooks, the organ templates, the organs, and the camera aimed at the focused birch. Without a
    // scene (a headless host) there is nothing to build, and every part counts as standing.
    if (scene_ == nullptr) {
        return GODOT_INIT_SCENE_PARTS;
    }
    uint32_t built = 0u;
    built += scene_terrain_ == resources_.terrain_generation() ? 1u : 0u;
    built += scene_streams_ == resources_.stream_generation() ? 1u : 0u;
    built += resources_.template_generation() > 0u ? 1u : 0u;
    built += scene_organs_ > 0u ? 1u : 0u;
    built += scene_focus_ != GODOT_NO_PLACE ? 1u : 0u;
    return built;
}

double VivariumView::initialisation_progress() const {
    // The share of the initialisation the start screen shows, one equal step each: the patches announced,
    // the brooks carried, the parts of the scene built, the quiet frames rendered. It is read from the same
    // state follow_initialisation lifts the start screen on, so the bar stands full in the frame it lifts.
    if (initialised_) {
        return 1.0;
    }
    const double brooks = start_.brooks.empty()
                              ? 1.0
                              : static_cast<double>(resources_.get_streams().size()) /
                                    static_cast<double>(start_.brooks.size());
    const double parts = static_cast<double>(scene_parts_built()) / static_cast<double>(GODOT_INIT_SCENE_PARTS);
    const double frames = static_cast<double>(init_render_frames_) / static_cast<double>(GODOT_INIT_RENDER_FRAMES);
    const double sum = (patches_announced_ ? 1.0 : 0.0) + ase::math::min(brooks, 1.0) + parts +
                       ase::math::min(frames, 1.0);
    return sum / GODOT_INIT_STEPS;
}

void VivariumView::follow_initialisation() {
    // THE START SCREEN (usecase:100387 view the plant): the engine loads everything before anyone sees it. Under
    // it the host runs only the ticks its loading needs, at the default pace and from real time - no time lapse,
    // no jump, no tap. The relief, the patches, the square and the brooks are submitted with the boot
    // (GodotHostResourceManager::boot), the generator ticks the ground and cuts the beds, the plugin tessellates
    // its templates and the view builds the scene. Behind the opaque start screen the finished scene renders,
    // and every material it shows compiles its pipelines. The screen lifts once every part stands and
    // GODOT_INIT_RENDER_FRAMES frames in a row rendered without a new pipeline compilation - the shaders are
    // loaded, and the first visible frame is the complete one.
    if (initialised_) {
        return;
    }
    ::godot::RenderingServer* server = ::godot::RenderingServer::get_singleton();
    const uint64_t pipelines =
        server == nullptr
            ? 0u
            : server->get_rendering_info(::godot::RenderingServer::RENDERING_INFO_PIPELINE_COMPILATIONS_CANVAS) +
                  server->get_rendering_info(::godot::RenderingServer::RENDERING_INFO_PIPELINE_COMPILATIONS_MESH) +
                  server->get_rendering_info(::godot::RenderingServer::RENDERING_INFO_PIPELINE_COMPILATIONS_SURFACE) +
                  server->get_rendering_info(::godot::RenderingServer::RENDERING_INFO_PIPELINE_COMPILATIONS_DRAW) +
                  server->get_rendering_info(
                      ::godot::RenderingServer::RENDERING_INFO_PIPELINE_COMPILATIONS_SPECIALIZATION);
    const bool compiled = pipelines != init_pipelines_;
    init_pipelines_ = pipelines;
    const bool brooks_carried = resources_.get_streams().size() >= start_.brooks.size();
    const bool scene_built = scene_parts_built() == GODOT_INIT_SCENE_PARTS;
    // The sky and the clock belong to the complete picture: the sky port carried its records (sun, light,
    // stars and moons through the hub star) and the clock port its clock row, from which the day and with
    // it the season follow. A quiet frame before both stand would lift the screen onto a sky that is still
    // empty and a season that still jumps; such a frame does not count.
    const bool sky_and_clock = resources_.get_sky().known && resources_.get_clock().known;
    if (!patches_announced_ || !brooks_carried || !scene_built || !sky_and_clock || compiled) {
        init_render_frames_ = 0u;
        return;
    }
    ++init_render_frames_;
    if (init_render_frames_ < GODOT_INIT_RENDER_FRAMES) {
        return;
    }
    initialised_ = true;
    note("start_screen lifted brooks=" + ::godot::String::num_uint64(resources_.get_streams().size()) +
         " frames=" + ::godot::String::num_uint64(init_render_frames_) +
         " pipelines=" + ::godot::String::num_uint64(pipelines) + " " + measure(selected_place_));
    node_->queue_redraw();
}

double VivariumView::jump_feed() const {
    // THE FEED OF A JUMP: real seconds this frame hands the host beyond its own, topped up only while the
    // backlog holds less than one feed, so a frame that could not run all of it never piles up more than the
    // next one runs. A stage jump takes the whole feed; an hour jump only the real time still missing to its
    // target at the measured rate of the clock, and on its way to the day change one game hour at most.
    const VivariumJump& jump = debug_.jump;
    const double backlog_s = resources_.backlog_seconds();
    if (!jump.running || !(backlog_s < debug_.lapse_s)) {
        return 0.0;
    }
    if (jump.stage != GODOT_STAGE_NONE) {
        return debug_.lapse_s;
    }
    const VivariumClockView& clock = resources_.get_clock();
    const double rate = hour_length_.game_per_real;
    if (!clock.known || !(rate > 0.0)) {
        return 0.0;  // the frame's own time runs on; the next frame measures the rate
    }
    const double missing_s = jump.target_s < 0.0 ? hour_length_.seconds_per_hour / rate - backlog_s
                                                 : (jump.target_s - clock.elapsed_s) / rate - backlog_s;
    return ase::math::max(0.0, ase::math::min(missing_s, debug_.lapse_s));
}

}  // namespace ase::adp::godot

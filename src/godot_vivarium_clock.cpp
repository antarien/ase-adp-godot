/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW CLOCK MOMENTS
 *
 * @file        godot_vivarium_clock.cpp
 * @brief       VivariumView - the start screen over the initialisation, the clock constants and rate, the jump share
 * @description How the view reads this host's game clock, computed with ase-time's constants as the clock record
 *              carries them, and the start screen that stands before the clock runs.
 *
 *              THE START SCREEN (usecase:100387 view the plant) covers everything the engine loads: the relief,
 *              the patches, the brook beds, the organ templates, the scene and the pipelines of every material it
 *              shows. Under it the host runs only the ticks its loading needs, at the default pace: the embedded
 *              host ticks from real time (KernelEmbeddedHost::advance), and the generator, the beds and the
 *              templates are those ticks. No time lapse runs under it and nothing is tapped. It lifts onto the
 *              first complete frame, and from that frame on the view measures the clock - no time lapse and no
 *              jump starts by itself (operator order of 2026-11-06). A jump raises it again until the world
 *              stands at the new time.
 *
 *              THE GAME HOUR AND THE GAME DAY are ase-time's constants, carried on every clock record
 *              (TIM_CONST_SECONDS_PER_HOUR and TIM_CONST_HOURS_PER_DAY); the view copies them and measures only the
 *              rate of the clock - its game seconds over the real seconds the host simulated between two frames.
 *              The jump (godot_vivarium_jump.cpp, usecase:101120 clock the growth cycle) computes no target from
 *              them: ase-time lays and judges it from its own clock.
 *
 *              A JUMP IS ONE WRITE (CAUSA_ASE_TIME, section "The Seek Contract"): the target goes once into
 *              ase-time's clock through the seek or the date port, and every module takes the new time from the
 *              star and computes its own state at it in its own schedule; nothing is replayed and no second is
 *              fed beyond the frame's own (GodotHostResourceManager::advance). The jump holds the start screen
 *              only until ase-time's verdict on its request number; follow_debug ends it there, and the screen
 *              lifts on the criteria of a start of the app, its fourth step the quiet frames as on a start.
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
 * @modified    2026-11-21
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_start_hold.hpp>
#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

// THE SHARE OF A RUNNING JUMP (usecase:101120 clock the growth cycle), 0..1, the fourth step of the start screen
// while the jump holds it. The clock is set in one write, so no game seconds are owed on the way. A jump runs
// only until ase-time's verdict on its own request number (follow_debug ends it there, D2281), and before that
// verdict the share is 0, as the quiet frames are: nothing is estimated, the view computes no game time of its
// own. The settle since the write (sought_run_s against debug.settle_s) holds only the landing note.
double jump_share(const VivariumJump& jump, const VivariumClockView& clock, double simulated_s, double settle_s) {
    if (!clock.known || !clock.seek_known || clock.seek_seq != jump.seq || jump.sought_run_s < 0.0) {
        return 0.0;
    }
    if (!(settle_s > 0.0)) {
        return 1.0;
    }
    return ase::math::min((simulated_s - jump.sought_run_s) / settle_s, 1.0);
}

}  // namespace

void VivariumView::follow_hour_length() {
    // THE CONSTANTS OF ase-time: the hours of a game day and the game seconds of a game hour travel on every
    // clock record (FloraHostClkExptSystem files them from the star), and the view copies them - under the start
    // screen too, so a jump asked there computes with them. Nothing of them is measured.
    // THE RATE OF THE CLOCK: the game seconds it ran over the real seconds the host simulated for them - the
    // running pace as the clock shows it, across a day turn too, since the game seconds only grow.
    const VivariumClockView& clock = resources_.get_clock();
    if (!clock.known) {
        return;
    }
    hour_length_.hours_per_day = clock.hours_per_day;
    hour_length_.seconds_per_hour = clock.seconds_per_hour;
    if (!initialised_) {
        return;  // under the start screen the host runs its loading ticks; the rate is measured from the lift on
    }
    const double passed = clock.elapsed_s - hour_length_.last_elapsed_s;
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
    if (scene_state_.scene_ == nullptr) {
        return GODOT_INIT_SCENE_PARTS;
    }
    uint32_t built = 0u;
    built += scene_state_.scene_terrain_ == resources_.terrain_generation() ? 1u : 0u;
    built += scene_state_.water_marks_.streams == resources_.stream_generation() ? 1u : 0u;
    built += resources_.template_generation() > 0u ? 1u : 0u;
    built += scene_state_.scene_organs_ > 0u ? 1u : 0u;
    built += scene_state_.scene_focus_ != GODOT_NO_PLACE ? 1u : 0u;
    return built;
}

double VivariumView::initialisation_progress() const {
    // The share of the initialisation the start screen shows, one equal step each: the patches announced,
    // the brooks carried, the parts of the scene built, the sky and the clock known, the figure file loaded,
    // the quiet frames rendered. It is read from the same state follow_initialisation lifts the start screen
    // on, so every hold of the screen stands in the bar and the bar stands full in the frame it lifts (B0357).
    // The figure file counts its blocks read and then its parse, scene and pack (figure_load_share).
    // After a jump the last step is the same quiet frames as on a start: the jump holds the screen only
    // until ase-time's verdict on its request number (follow_debug), and the quiet frames stay at 0 until then.
    if (initialised_) {
        return 1.0;
    }
    const double sky_and_clock = ((resources_.get_sky().known ? 1.0 : 0.0) +
                                  (resources_.get_clock().known ? 1.0 : 0.0)) / 2.0;
    const double figure = figure_load_share(figure_state_.figure_look_.load);
    const double brooks = start_.brooks.empty()
                              ? 1.0
                              : static_cast<double>(resources_.get_streams().size()) /
                                    static_cast<double>(start_.brooks.size());
    const double parts = static_cast<double>(scene_parts_built()) / static_cast<double>(GODOT_INIT_SCENE_PARTS);
    const double frames = static_cast<double>(start_hold_.render_frames) / static_cast<double>(GODOT_INIT_RENDER_FRAMES);
    const VivariumJump& jump = debug_.jump;
    const double last = jump.running ? jump_share(jump, resources_.get_clock(), resources_.stage_seconds(),
                                                  debug_.settle_s)
                                     : ase::math::min(frames, 1.0);
    const double sum = (patches_announced_ ? 1.0 : 0.0) + ase::math::min(brooks, 1.0) + parts + sky_and_clock +
                       figure + last;
    return sum / GODOT_INIT_STEPS;
}

void VivariumView::follow_initialisation() {
    // THE START SCREEN (usecase:100387 view the plant): the engine loads everything before anyone sees it. Under
    // it the host runs only the steps its loading needs, from real time - no time lapse, no tap; after a jump
    // it stands until the world stands at the new time. The relief, the patches, the square and the brooks are
    // submitted with the boot (GodotHostResourceManager::boot), the generator ticks the ground and cuts the beds,
    // the plugin tessellates its templates and the view builds the scene. Behind the opaque start screen the
    // finished scene renders, and every material it shows compiles its pipelines. The screen lifts once every
    // part stands and GODOT_INIT_RENDER_FRAMES frames in a row rendered without a new pipeline compilation - the
    // shaders are loaded, and the first visible frame is the complete one.
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
    const bool compiled = pipelines != start_hold_.pipelines;
    start_hold_.pipelines = pipelines;
    const bool brooks_carried = resources_.get_streams().size() >= start_.brooks.size();
    const bool scene_built = scene_parts_built() == GODOT_INIT_SCENE_PARTS;
    // The sky and the clock belong to the complete picture: the sky port carried its records (sun, light,
    // stars and moons through the hub star) and the clock port its clock row, from which the day and with
    // it the season follow. A quiet frame before both stand would lift the screen onto a sky that is still
    // empty and a season that still jumps; such a frame does not count.
    const bool sky_and_clock = resources_.get_sky().known && resources_.get_clock().known;
    // A running jump holds the screen until ase-time's verdict on its request number: before it the clock has
    // not moved yet. On the verdict follow_debug ends the jump, and from there the screen lifts on exactly the
    // criteria of a start; the settle of the landing note holds no frame (D2281).
    const bool jumping = debug_.jump.running;
    // the figure file loads in a task of the WorkerThreadPool (godot_figure_load.hpp); its figures and its
    // pipelines belong to the first visible frame, so the screen holds until follow_figure took the pack
    const bool figure_loading = figure_load_running(figure_state_.figure_look_.load);
    // THE LOG NAMES THE STEP THAT HOLDS THE SCREEN (B0357): the first step of the bar still missing, in the
    // order of initialisation_progress, chosen once by start_hold_step (godot_start_hold.hpp); a line stands
    // each time that step changes, never per frame. The new pipelines and the quiet frames are one step,
    // render: a compilation restarts the quiet frames, so the two alternate inside that step and name no new hold.
    StartHoldReading reading;
    reading.patches_announced = patches_announced_;
    reading.brooks_carried = brooks_carried;
    reading.scene_built = scene_built;
    reading.sky_and_clock = sky_and_clock;
    reading.figure_loading = figure_loading;
    reading.jumping = jumping;
    const uint8_t step = start_hold_step(reading);
    const char* hold = start_hold_name(step);
    // the bar names the same step in the keeper's words: the word of texts.hold_*, one per step in the order of
    // the GODOT_HOLD_* values
    const ::godot::String* const words[GODOT_HOLD_STEPS] = {
        &texts_.hold_patches, &texts_.hold_brooks, &texts_.hold_scene, &texts_.hold_sky_clock,
        &texts_.hold_figure,  &texts_.hold_jump,   &texts_.hold_render};
    start_hold_.hold_word = *words[step];
    // THE LINE CARRIES THE DURATION OF THE STEP BEFORE (B0357): prev= names the step that held until this
    // frame and held_ms= its wall-clock time since it began, so a run compares figure against the other steps
    // from the log alone; the first line of a boot has no step before it and names none
    const ::godot::Time* wall = ::godot::Time::get_singleton();
    const uint64_t now_ms = wall == nullptr ? 0u : wall->get_ticks_msec();
    if (start_hold_.hold != ::godot::String(hold)) {
        const ::godot::String before =
            start_hold_.hold.is_empty() ? ::godot::String()
                                        : " prev=" + start_hold_.hold + " held_ms=" +
                                              ::godot::String::num_uint64(now_ms - start_hold_.hold_since_ms);
        start_hold_.hold = ::godot::String(hold);
        start_hold_.hold_since_ms = now_ms;
        note("start_screen held_by=" + start_hold_.hold + before + " brooks=" +
             ::godot::String::num_uint64(resources_.get_streams().size()) + "/" +
             ::godot::String::num_uint64(start_.brooks.size()) + " parts=" +
             ::godot::String::num_uint64(scene_parts_built()) + "/" +
             ::godot::String::num_uint64(GODOT_INIT_SCENE_PARTS) + " figure=" +
             ::godot::String::num(figure_load_share(figure_state_.figure_look_.load), 2) + " " + measure(GODOT_NO_PLACE));
    }
    if (step != GODOT_HOLD_RENDER || compiled) {
        start_hold_.render_frames = 0u;
        return;
    }
    ++start_hold_.render_frames;
    if (start_hold_.render_frames < GODOT_INIT_RENDER_FRAMES) {
        return;
    }
    initialised_ = true;
    // the figure file compiled its pipelines behind the screen (warm_figure); its warming instance leaves now
    end_figure_warmth(scene_state_.scene_);
    note("start_screen lifted prev=" + start_hold_.hold + " held_ms=" +
         ::godot::String::num_uint64(now_ms - start_hold_.hold_since_ms) +
         " brooks=" + ::godot::String::num_uint64(resources_.get_streams().size()) +
         " frames=" + ::godot::String::num_uint64(start_hold_.render_frames) +
         " pipelines=" + ::godot::String::num_uint64(pipelines) + " " + measure(selected_place_));
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

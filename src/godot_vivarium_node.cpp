/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW IMPLEMENTATION
 *
 * @file        godot_vivarium_node.cpp
 * @brief       VivariumView - lifecycle, host boot and the frame of the Vivarium frontend
 * @design      DSGN_066
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3/01.4. The view reads its configuration as data
 *              (res://config/vivarium_start.json), boots one embedded host through
 *              GodotHostResourceManager from the bundle Godot loaded this library from - the whole
 *              set of units in the order of their manifests, with the bundle's data root - hands
 *              it the real time of every frame and draws what the last snapshots reported.
 *
 *              ONE CLASS, ITS BEHAVIOUR BY RESPONSIBILITY: this file carries the lifecycle, the boot
 *              and the frame; the log stream (the patch baseline, the events of stage and condition,
 *              the measure, the drain) stands in godot_vivarium_log.cpp, the configuration in
 *              godot_vivarium_config.cpp, godot_vivarium_host_config.cpp, godot_vivarium_layout.cpp and
 *              godot_vivarium_assets.cpp, the input in godot_vivarium_input.cpp, the canvas in
 *              godot_vivarium_draw.cpp, the 3D scene in godot_vivarium_scene.cpp, the explicit modes in
 *              godot_vivarium_measure.cpp, the pour and its arrival in godot_vivarium_water.cpp, the debug
 *              buttons and their pace in godot_vivarium_debug.cpp, the jumps in godot_vivarium_jump.cpp and
 *              godot_vivarium_date.cpp, the background, the foreground and the return to the garden in
 *              godot_vivarium_return.cpp.
 *
 *              EVERY ENGINE CALL GOES THROUGH node_: drawing, redraw requests, processing, input
 *              mapping and the viewport are those of the engine object this view is the instance
 *              data of (see godot_vivarium_node.hpp, COMPOSED, NOT DERIVED).
 *
 *              THE LOG IS ONE STREAM (godot_vivarium_log.cpp). This file writes the boot line, every
 *              sowing and every restart through the same note, never around it.
 *
 *              THE ASSET SET IS BOUND WITH THE CONFIGURATION (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS
 *              03.2): every boot writes `asset missing <name>` once per missing name, after the boot
 *              line that counts them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-05
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_asset_showcase.hpp>
#include <ase/adp/godot/godot_asset_showcase_scene.hpp>
#include <ase/adp/godot/godot_new_seed.hpp>
#include <ase/adp/godot/godot_plant_look_shader.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/utils/boot_clock.hpp>
#include <ase/utils/clock.hpp>

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

namespace {

::godot::String identity_value(const ::godot::Dictionary& identity, const char* key) {
    return static_cast<::godot::String>(identity.get(key, "?"));
}

}  // anonymous namespace

// LIFETIME

VivariumView::VivariumView(::godot::Node2D* node) : node_(node) {}

VivariumView::~VivariumView() {
    // An object freed without _exit_tree (an orphan) still tears its host down in the binding
    // order. node_ is not touched here: Godot frees this view before the object's handle.
    resources_.clear_all();
}

void VivariumView::ready() {
    // L0: importing or editing the scene starts no host and loads no unit.
    if (in_editor()) {
        node_->set_process(false);
        return;
    }
    // usecase:101019 start the client window: name the window the view opened in before anything draws into it
    announce_window();
    configured_ = load_config();
    if (configured_) {
        // PLAN 02.3: the self-test runs on one test host of its own BEFORE the game host exists, and
        // only when the launch asks for it; its report goes through the game host once that is up.
        if (launched_with(modes_.selftest_argument)) {
            run_self_test();
        }
        measurement_.active = launched_with(modes_.measure_argument);
        measurement_.warmup_left = modes_.measure_warmup_seconds;
        measurement_.record_left = modes_.measure_record_seconds;
        // THE START SCREEN HOLDS THE WORLD until the keeper chose how it starts (start_input, operator order
        // 2026-11-19); a launch that takes no taps boots at once on the switches at rest.
        if (launched_with(modes_.selftest_argument) || measurement_.active || launched_with(modes_.assets_argument)) {
            begin_host(start_choice_.chosen());
        }
        // PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2: the care view is a 3D scene; it stands once and
        // follow_scene fills it from the ports frame by frame.
        scene_ = build_plant_scene(node_, scene_camera_);
        // 03.2: the materials and organ template files of the stage are read once, here, through the
        // loader the asset showcase shares; a missing one is drawn in the error material
        scene_look_ = bind_plant_look(assets_, asset_root_, asset_roles_);
        // B0482: the least screen width of a bark ring (view.least_ring_px) on the shared bark material, which
        // fill_tube duplicates per plant, so every tube holds a ring narrower than it at this width
        const ::godot::Ref<::godot::ShaderMaterial> bark = scene_look_.bark;
        if (bark.is_valid()) {
            bark->set_shader_parameter(GODOT_BARK_LEAST_PIXELS, least_ring_px_);
        }
        // 07.9: the figure file of the stage (role figure) or the error material of its stand-in body. The file
        // (169.6 MB) is parsed and packed by a task of the WorkerThreadPool, so this ready() returns and the
        // start screen draws while it loads; follow_figure takes the pack and warms it behind the screen
        figure_look_ = bind_figure_look(assets_, asset_roles_, figure_look_.motion);  // the motion load_figure_hud read
        start_figure_load(figure_look_.load, asset_root_, figure_look_.file);
        // 07.10: each tool of the hand loads in its own task; its node stays a block until its pack is taken
        for (auto& [kind, tool] : figure_look_.tools) {
            start_figure_load(tool.load, asset_root_, tool.file);
        }
        // 06 8.3: the body of the tool place (role rack) loads the same way; lay_tools stands it once taken
        start_figure_load(figure_look_.rack.load, asset_root_, figure_look_.rack.file);
        // PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7: the asset showcase beside the one game host, from
        // the binding of the configuration; its line follows the boot line.
        if (launched_with(modes_.assets_argument)) {
            const AssetShowcasePlan plan = plan_showcase(asset_names_, asset_materials_, assets_);
            showcase_ = build_asset_showcase(node_, plan, assets_, asset_root_);
            note(text_of(showcase_line(plan)));
        }
        for (const std::string& line : selftest_report_) {
            note(text_of(line));
        }
        selftest_report_.clear();
        drain_log();
    }
    node_->set_process(true);
    node_->queue_redraw();
}

void VivariumView::exit_tree() {
    if (in_editor()) {
        return;
    }
    note("exit " + measure(GODOT_NO_PLACE));
    // a figure file still loading writes into figure_look_: the view waits for its task before it leaves
    end_figure_load(figure_look_.load);
    for (auto& [kind, tool] : figure_look_.tools) {
        end_figure_load(tool.load);
    }
    resources_.clear_all();
    drain_log();
}

// HOST LIFECYCLE

void VivariumView::boot_host(bool new_seeds) {
    failure_text_ = ::godot::String();
    selected_place_ = GODOT_NO_PLACE;
    water_pending_place_ = GODOT_NO_PLACE;
    pruning_ = false;
    bedding_ = false;
    bed_started_ = false;
    player_beds_.clear();  // a new host carries only the configured brooks
    flattening_ = false;
    player_flats_.clear();  // and only the ground it lays
    // The first frame after a boot starts the clock: the time the boot took is no simulation time.
    last_frame_ns_ = 0;
    // Every boot starts without a jump and measures the game hour and the clock rate anew - the first one
    // and every Reiniciar, whose new seed starts as a seed again; a jump runs only when its debug button asks.
    debug_.jump = VivariumJump{};
    hour_length_ = VivariumHourLength{};
    init_pipelines_ = 0u;
    garden_return_ = GardenReturn{};
    // sow a new seed (03.1 point 2, V7): the first boot sows the seeds of the start configuration, every
    // Reiniciar a new seed per patch (godot_new_seed.hpp) - a new birch of the same species in another shape.
    // A rewind (a date behind the clock, rewind_host) keeps the sowing of this run: the same birch from its start.
    if (new_seeds) {
        run_sowing_ = host_boots_;
    }
    const uint32_t sowing = run_sowing_;
    host_boots_ += 1u;
    VivariumStartConfig sown = start_;
    // offline from the initial state: the kernel reads no file of the last state (KernelEmbeddedHost::set_start_fresh)
    sown.start_fresh = start_mode_ == GODOT_START_MODE_OFFLINE_INITIAL;
    for (VivariumPatchSpec& patch : sown.patches) {
        patch.seed = sown_seed(patch.seed, sowing);
    }

    const HostStatus status = resources_.boot(sown);
    if (status != ase::kernel::HostStatusOk) {
        failure_text_ = texts_.error + ": " + resources_.failure_step() + " (" +
                        GodotHostResourceManager::status_name(status) + ")";
        ::godot::UtilityFunctions::push_error(failure_text_);
        drain_log();
        return;
    }

    // The boot shaped the ground with the start's relief (GodotHostResourceManager::start submits
    // ground.height_scale_m before the first patch): that relief stands until a RELEVO chip asks another.
    debug_.height_scale_m = sown.ground.height_scale_m;

    const VivariumBundleInfo& bundle = resources_.get_bundle();
    // The adapter's version comes from build.identity like the client's: stage_native.py writes it at stage time,
    // so a version step never reaches a compile line and never rebuilds a unit.
    build_line_ =::godot::String("adapter ") + identity_value(identity_, "adapter_version") + texts_.separator + "client " +
                  identity_value(identity_, "client_commit") + texts_.separator +
                  ::godot::String::num_uint64(bundle.units.size()) + " units";
    // The build line keeps to the surface: it shrinks until it fits beside the title's margins.
    build_font_size_ = fitting_size(build_line_, layout_.size.x - 2.0f * layout_.build_at.x, GODOT_FONT_SMALL);

    // Stage and condition of every place are unknown until each place's record carries them;
    // announce_patches takes them as the baseline, and later changes are the events the log carries.
    // The condition is the raw code of ase-plant, NaN while no record carries one.
    stages_.assign(layout_.place_rects.size(), GODOT_CODE_UNKNOWN);
    conditions_.assign(layout_.place_rects.size(), std::nan(""));
    patches_announced_ = false;
    // Every boot, a Reiniciar too, initialises the world anew under the start screen.
    initialised_ = false;
    init_render_frames_ = 0u;
    note(boot_line());
    // V7: every patch names the seed it was sown with, so a Reiniciar shows its new seed in the log.
    for (const VivariumPatchSpec& patch : sown.patches) {
        note("sow place=" + ::godot::String::num_uint64(patch.place) + " seed=" +
             ::godot::String::num_uint64(patch.seed) + " sowing=" + ::godot::String::num_uint64(sowing));
    }
    // 03.2 point 3: one line per missing name and boot, after the boot line that counts them.
    for (const std::string& missing : assets_.missing) {
        note(text_of(asset_missing_line(missing)));
    }
    announce_patches();
    drain_log();
}

void VivariumView::restart() {
    // Reiniciar: the same teardown as exit_tree, then a new host with the whole set. Selection,
    // pending pour and drawing values of the old run go with it.
    note("restart " + measure(GODOT_NO_PLACE));
    resources_.clear_all();
    // The new host boots at the default pace of its clock: a time lapse and a jump end with the old one.
    debug_.factor = GODOT_PACE_NONE;
    debug_.jump = VivariumJump{};
    drain_log();
    if (!configured_) {
        configured_ = load_config();
    }
    if (configured_ && start_mode_ != GODOT_START_MODE_NONE) {
        boot_host(true);  // the new host starts the way the keeper chose for the first one
    } else if (configured_) {
        failure_text_ = ::godot::String();  // no choice made yet: the start screen shows it instead of the panel
    }
    node_->queue_redraw();
}

::godot::String VivariumView::boot_line() const {
    ::godot::Engine* engine = ::godot::Engine::get_singleton();
    ::godot::RenderingServer* rendering = ::godot::RenderingServer::get_singleton();
    const VivariumBundleInfo& bundle = resources_.get_bundle();
    const ::godot::Dictionary version_info = engine->get_version_info();
    // THE WHOLE SET WITH VERSIONS (L1): the bundle's units as the index names them - the host's own
    // boot line names them in load order beside this one.
    ::godot::String set;
    ::godot::String api;
    for (const VivariumBundleUnit& unit : bundle.units) {
        set += (set.is_empty() ? "" : ",") + text_of(unit.unit) + "@" + text_of(unit.version);
        api = text_of(unit.api_version);
    }
    return ::godot::String("boot")
        + " client=" + identity_value(identity_, "client_commit")
        + " client_version=" + identity_value(identity_, "client_version")
        + " adapter=" + identity_value(identity_, "adapter_version")
        + " kernel=" + identity_value(identity_, "kernel_version")
        + " set=" + set
        + " ase_api=" + api
        + " host_port_abi=" + ::godot::String::num_uint64(ase::kernel::HostPortAbiV2)
        + " entt=" + identity_value(identity_, "entt")
        + " godot=" + static_cast<::godot::String>(version_info.get("string", "?"))
        + " godot_cpp=" + identity_value(identity_, "godot_cpp")
        + " display=" + ::godot::DisplayServer::get_singleton()->get_name()
        + " renderer=" + rendering->get_current_rendering_driver_name() + "/"
        + rendering->get_current_rendering_method()
        + " gpu=" + rendering->get_video_adapter_name()
        + " pid=" + ::godot::String::num_int64(::godot::OS::get_singleton()->get_process_id())
        + " abi=" + engine->get_architecture_name()
        + " platform_abi=" + identity_value(identity_, "platform_abi")
        + " config=" + identity_value(identity_, "config")
        + " stage=" + stage_dir_
        + " library_load=" + (start_.library_dir.empty() ? "name" : "path")
        + " data_root=" + text_of(start_.files_dir) + "/" + text_of(bundle.data_root)
        + " selftest=" + (selftest_ran_ ? "errors:" + ::godot::String::num_uint64(selftest_errors_) : "off")
        + " measure=" + (measurement_.active ? "on" : "off")
        + " debug=" + (debug_.on ? "on" : "off")
        + " play_stage=" + ::godot::String::num_uint64(play_stage_)
        + " assets=" + ::godot::String::num_uint64(assets_.files.size() + assets_.missing.size())
        + " asset_missing=" + ::godot::String::num_uint64(assets_.missing.size())
        + " asset_manifest=" + asset_manifest_
        + " boots=" + ::godot::String::num_uint64(host_boots_)
        + " units=" + ::godot::String::num_uint64(resources_.unit_count())
        + " patches=" + ::godot::String::num_uint64(resources_.patch_count())
        + " systems=" + ::godot::String::num_uint64(resources_.system_count());
}

// FRAME

void VivariumView::process(double delta) {
    // Godot's delta may be smoothed, and Godot's own clock stops while the device sleeps
    // (OS_Unix::get_ticks_usec reads CLOCK_MONOTONIC_RAW, OS_Android inherits it): the host gets
    // the real time that passed, a suspend of the device included, from the boot clock
    // (ase/utils/boot_clock.hpp, PLAN 01.3 point 4). Godot's clock still times the frame itself
    // for the measuring mode - a span within one waking period, which is what it is right for.
    (void)delta;
    drain_log();
    // A surface of another size than the one the last frame was placed on - a resized window, a
    // folded handset - needs a new placement.
    const ::godot::Vector2 visible = node_->get_viewport_rect().size;
    if (visible.x != drawn_surface_.width || visible.y != drawn_surface_.height) {
        node_->queue_redraw();
    }
    if (!resources_.running() || !failure_text_.is_empty()) {
        last_frame_ns_ = 0;
        return;
    }
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t frame_start_usec = clock->get_ticks_usec();
    const int64_t frame_ns = ase::utils::boot_nanos();
    if (last_frame_ns_ == 0 || frame_ns <= last_frame_ns_) {
        // The first frame after a boot starts the clock; it brings no time of its own.
        last_frame_ns_ = frame_ns;
        return;
    }
    const double elapsed_s =
        static_cast<double>(frame_ns - last_frame_ns_) / static_cast<double>(ase::utils::NANOS_PER_SECOND);
    last_frame_ns_ = frame_ns;
    watch_ground_frame(frame_start_usec);  // the frames after a ground act, measured in every mode (P0690)
    // The host gets the real time the frame took and nothing beyond it: a jump is one write into ase-time's
    // clock (CAUSA_ASE_TIME, "The Seek Contract"), and every module computes its state at the new time in its
    // own ticks at the running pace. Host time (advance + snapshots) is measured apart from the whole _process
    // interval (PLAN 02.3).
    const HostStatus status = resources_.advance(elapsed_s);
    const uint64_t host_usec = clock->get_ticks_usec() - frame_start_usec;
    watch_ground_host(host_usec);  // the host's share of the frames after a ground act (P0690)
    if (status == ase::kernel::HostStatusOk) {
        announce_patches();
        follow_catch_up(elapsed_s, static_cast<double>(frame_ns) / static_cast<double>(ase::utils::NANOS_PER_SECOND));
        follow_stages();
        follow_debug();
        follow_hour_length();
        follow_water();
        const uint64_t scene_start_usec = clock->get_ticks_usec();
        follow_scene(elapsed_s);
        follow_figure(elapsed_s);  // after the scene: the player camera overrides the fixed view's aim of this frame (07.6)
        follow_initialisation();
        const uint64_t hud_start_usec = clock->get_ticks_usec();
        tick_hud(elapsed_s);  // the HUD's clock: long presses, held pours (godot_vivarium_hud_input.cpp)
        const uint64_t hud_end_usec = clock->get_ticks_usec();
        // the view's own shares of a frame after a ground act: which step besides the host filled the interval (P0690)
        watch_ground_phases(scene_start_usec - frame_start_usec - host_usec, hud_start_usec - scene_start_usec,
                            hud_end_usec - hud_start_usec, hud_end_usec - frame_start_usec);
        // The asset showcase (PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7) is lit by the sun of the host's
        // sky port; until the port carried all four records the showcase keeps its light unset.
        const VivariumSkyView& sky = resources_.get_sky();
        if (showcase_ != nullptr && sky.known) {
            const float toward_sun[3] = {static_cast<float>(sky.sun_x), static_cast<float>(sky.sun_y),
                                         static_cast<float>(sky.sun_z)};
            const float sun_rgb[3] = {static_cast<float>(sky.sun_r), static_cast<float>(sky.sun_g),
                                      static_cast<float>(sky.sun_b)};
            light_asset_showcase(showcase_, toward_sun, sun_rgb, static_cast<float>(sky.sun_intensity));
        }
        if (measurement_.active) {
            measure_frame(frame_start_usec, host_usec);
        }
    } else if (!resources_.running()) {
        // The manager stopped the host (a broken snapshot) - the panel says where.
        failure_text_ = texts_.error + ": " + resources_.failure_step() + " (" +
                        GodotHostResourceManager::status_name(status) + ")";
        ::godot::UtilityFunctions::push_error(failure_text_);
    }
    // A refused frame with a running host only drops this frame; the host has logged why.
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

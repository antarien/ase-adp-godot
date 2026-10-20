/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW IMPLEMENTATION
 *
 * @file        godot_vivarium_node.cpp
 * @brief       VivariumView - lifecycle, host boot, the frame and the one log stream of the Vivarium frontend
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3/01.4. The view reads its configuration as data
 *              (res://config/vivarium_start.json), boots one embedded host through
 *              GodotHostResourceManager from the bundle Godot loaded this library from - the whole
 *              set of units in the order of their manifests, with the bundle's data root - hands
 *              it the real time of every frame and draws what the last snapshots reported.
 *
 *              ONE CLASS, ITS BEHAVIOUR BY RESPONSIBILITY: this file carries the lifecycle, the boot,
 *              the frame and the log; the configuration stands in godot_vivarium_config.cpp,
 *              godot_vivarium_host_config.cpp, godot_vivarium_layout.cpp and godot_vivarium_assets.cpp,
 *              the input in godot_vivarium_input.cpp, the canvas in godot_vivarium_draw.cpp, the 3D scene
 *              in godot_vivarium_scene.cpp, the explicit modes in godot_vivarium_measure.cpp.
 *
 *              EVERY ENGINE CALL GOES THROUGH node_: drawing, redraw requests, processing, input
 *              mapping and the viewport are those of the engine object this view is the instance
 *              data of (see godot_vivarium_node.hpp, COMPOSED, NOT DERIVED).
 *
 *              THE LOG IS ONE STREAM. The host's logger (ase-log inside the runtime library)
 *              writes the file under user://logs and hands every line to the resource manager;
 *              the view prints them once per frame (on Android the manager writes them to logcat
 *              itself). Its own lines - the boot line, every action with its target and result,
 *              every change of stage or condition, every return from the background - go through
 *              the SAME logger via KernelEmbeddedHost::note, never around it. Nothing is logged per
 *              render frame.
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
 * @modified    2026-10-18
 * @version     00.00.04.00007
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_asset_showcase.hpp>
#include <ase/adp/godot/godot_asset_showcase_scene.hpp>
#include <ase/adp/godot/godot_new_seed.hpp>
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
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstddef>
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
        boot_host();
        // PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2: the care view is a 3D scene; it stands once and
        // follow_scene fills it from the ports frame by frame.
        scene_ = build_plant_scene(node_, scene_camera_);
        // 03.2: the materials and organ template files of the stage are read once, here, through the
        // loader the asset showcase shares; a missing one is drawn in the error material
        scene_look_ = bind_plant_look(assets_, asset_root_, asset_roles_);
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
    resources_.clear_all();
    drain_log();
}

// HOST LIFECYCLE

void VivariumView::boot_host() {
    failure_text_ = ::godot::String();
    selected_place_ = GODOT_NO_PLACE;
    water_pending_place_ = GODOT_NO_PLACE;
    pruning_ = false;
    // The first frame after a boot starts the clock: the time the boot took is no simulation time.
    last_frame_ns_ = 0;
    garden_return_ = GardenReturn{};
    // sow a new seed (03.1 point 2, V7): the first boot sows the seeds of the start configuration, every
    // Reiniciar a new seed per patch (godot_new_seed.hpp) - a new birch of the same species in another shape.
    const uint32_t sowing = host_boots_;
    host_boots_ += 1u;
    VivariumStartConfig sown = start_;
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

    const VivariumBundleInfo& bundle = resources_.get_bundle();
    build_line_ = ::godot::String("adapter ") + ASE_ADP_GODOT_VERSION + texts_.separator + "client " +
                  identity_value(identity_, "client_commit") + texts_.separator +
                  ::godot::String::num_uint64(bundle.units.size()) + " units";
    // The build line keeps to the surface: it shrinks until it fits beside the title's margins.
    build_font_size_ = fitting_size(build_line_, layout_.size.x - 2.0f * layout_.build_at.x, GODOT_FONT_SMALL);

    // Stage and condition of every place are unknown until each place's record carries them;
    // announce_patches takes them as the baseline, and later changes are the events the log carries.
    stages_.assign(layout_.place_rects.size(), GODOT_CODE_UNKNOWN);
    conditions_.assign(layout_.place_rects.size(), GODOT_CODE_UNKNOWN);
    patches_announced_ = false;
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

void VivariumView::announce_patches() {
    // L1: one line per patch, once every place has its record with a stage the modules published.
    // The boot runs no tick, and ase-pl-flora writes a value only while the input row behind it
    // stands (PLAN 01.3 point 1), so the lines come with the first frame that has them all - never
    // a line about a patch the port does not carry yet.
    if (patches_announced_) {
        return;
    }
    for (std::size_t place = 0; place < layout_.place_rects.size(); ++place) {
        const VivariumPatchView* patch = resources_.get_patch_at(static_cast<uint32_t>(place));
        if (patch == nullptr || patch->stage == GODOT_CODE_UNKNOWN || patch->condition == GODOT_CODE_UNKNOWN) {
            return;
        }
    }
    patches_announced_ = true;
    for (std::size_t place = 0; place < layout_.place_rects.size(); ++place) {
        const VivariumPatchView* patch = resources_.get_patch_at(static_cast<uint32_t>(place));
        stages_[place] = patch->stage;
        conditions_[place] = patch->condition;
        note("patch " + measure(static_cast<uint32_t>(place)));
    }
}

void VivariumView::restart() {
    // Reiniciar: the same teardown as exit_tree, then a new host with the whole set. Selection,
    // pending pour and drawing values of the old run go with it.
    note("restart " + measure(GODOT_NO_PLACE));
    resources_.clear_all();
    drain_log();
    if (!configured_) {
        configured_ = load_config();
    }
    if (configured_) {
        boot_host();
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
        + " adapter=" + ASE_ADP_GODOT_VERSION
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
    // Host time (advance + snapshots) is measured apart from the whole _process interval (PLAN 02.3).
    const HostStatus status = resources_.advance(elapsed_s);
    const uint64_t host_usec = clock->get_ticks_usec() - frame_start_usec;
    if (status == ase::kernel::HostStatusOk) {
        announce_patches();
        follow_catch_up(elapsed_s, static_cast<double>(frame_ns) / static_cast<double>(ase::utils::NANOS_PER_SECOND));
        follow_stages();
        follow_water();
        follow_scene(elapsed_s);
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

void VivariumView::notification(int32_t what) {
    if (in_editor()) {
        return;
    }
    // THE WORLD DOES NOT STOP (PLAN 01.3 point 4): the background is logged as a fact, nothing is
    // held. The next frame brings the time it lasted, and follow_catch_up logs its return.
    if (what == ::godot::Node::NOTIFICATION_APPLICATION_PAUSED) {
        note("background reason=application " + measure(GODOT_NO_PLACE));
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_RESUMED) {
        note("foreground reason=application " + measure(GODOT_NO_PLACE));
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_FOCUS_OUT) {
        note("background reason=focus " + measure(GODOT_NO_PLACE));
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_FOCUS_IN) {
        note("foreground reason=focus " + measure(GODOT_NO_PLACE));
    }
}

void VivariumView::follow_catch_up(double elapsed_s, double now_s) {
    // The return to the garden (godot_garden_return.hpp) decides what this frame says: the frame that
    // noticed a span the app spent away, and the frame in which its backlog ran dry with the seconds
    // the catch-up took (V6). This function only writes the two lines. The ticks of this one frame
    // are frame_ticks: measure() carries steps= (the Dissemination runs), and a key standing twice in
    // one line keeps only its last value for every key=value reader.
    const GardenReturnFrame frame =
        garden_return_frame(garden_return_, now_s, elapsed_s, resources_.backlog_seconds());
    if (frame.noticed) {
        measurement_.catch_ups += measurement_.recording ? 1u : 0u;
        note("catch_up elapsed_s=" + ::godot::String::num(elapsed_s, GODOT_LOG_DECIMALS) +
             " frame_ticks=" + ::godot::String::num_uint64(resources_.last_steps()) +
             " backlog_s=" + ::godot::String::num(resources_.backlog_seconds(), GODOT_LOG_DECIMALS) + " " +
             measure(GODOT_NO_PLACE));
    }
    if (frame.caught_up) {
        note("caught_up catch_up_s=" + ::godot::String::num(frame.catch_up_s, GODOT_LOG_DECIMALS) + " " +
             measure(GODOT_NO_PLACE));
    }
}

void VivariumView::follow_stages() {
    // A change of stage or condition is an EVENT and gets one line - the acceptance reads growth,
    // ripening and drying from these lines (L4), never from a line per frame. Before the patches
    // were announced there is no baseline, and the first published values are no change.
    if (!patches_announced_) {
        return;
    }
    for (std::size_t place = 0; place < stages_.size() && place < conditions_.size(); ++place) {
        const VivariumPatchView* patch = resources_.get_patch_at(static_cast<uint32_t>(place));
        if (patch == nullptr) {
            continue;
        }
        if (patch->stage != stages_[place]) {
            note("stage " + measure(static_cast<uint32_t>(place)) + " from=" +
                 ::godot::String::num_uint64(stages_[place]));
            stages_[place] = patch->stage;
        }
        if (patch->condition != conditions_[place]) {
            note("condition " + measure(static_cast<uint32_t>(place)) + " from=" +
                 ::godot::String::num_uint64(conditions_[place]));
            conditions_[place] = patch->condition;
        }
    }
}

void VivariumView::follow_water() {
    // Input → plugin (Reception) → ase-hydro (Reception) → balance and publish (Dissemination) →
    // snapshot: the pour has arrived once the patch's soil water rose above its value at the
    // press, and only then is the result logged (L3). It may also be refused on its way - by
    // ase-hydro above the field capacity - and then no rise ever comes: after
    // GODOT_WATER_WAIT_STEPS Dissemination runs that is a line of its own.
    if (water_pending_place_ == GODOT_NO_PLACE) {
        return;
    }
    const VivariumPatchView* patch = resources_.get_patch_at(water_pending_place_);
    if (patch == nullptr) {
        water_pending_place_ = GODOT_NO_PLACE;
        return;
    }
    if (patch->soil_mm > water_pending_from_) {
        note("water applied " + measure(water_pending_place_) + " soil_mm_before=" +
             ::godot::String::num(water_pending_from_, GODOT_LOG_DECIMALS));
        water_pending_place_ = GODOT_NO_PLACE;
        return;
    }
    uint64_t steps = 0u;
    if (resources_.schedule_runs(GODOT_LOG_STEP_SCHEDULE, &steps) == ase::kernel::HostStatusOk &&
        steps >= water_pending_steps_ + GODOT_WATER_WAIT_STEPS) {
        note("water not applied " + measure(water_pending_place_) + " soil_mm_before=" +
             ::godot::String::num(water_pending_from_, GODOT_LOG_DECIMALS));
        water_pending_place_ = GODOT_NO_PLACE;
    }
}

// THE LOG

::godot::String VivariumView::measure(uint32_t place) const {
    const VivariumClockView& clock = resources_.get_clock();
    ::godot::String line = "t=" + ::godot::String::num(resources_.simulation_seconds(), GODOT_LOG_DECIMALS) +
                           " ticks=" + ::godot::String::num_uint64(resources_.tick_count());
    // Steps are the scheduler's count of the modules' step schedule, never t divided by its
    // interval. A host that cannot answer (none yet, stopped) gives no steps= at all instead of a
    // false 0; the host logs its own refusal.
    uint64_t steps = 0u;
    if (resources_.schedule_runs(GODOT_LOG_STEP_SCHEDULE, &steps) == ase::kernel::HostStatusOk) {
        line += " steps=" + ::godot::String::num_uint64(steps);
    }
    if (clock.known) {
        line += " game_s=" + ::godot::String::num(clock.elapsed_s, GODOT_LOG_DECIMALS) +
                " day=" + ::godot::String::num(clock.day, GODOT_LOG_DECIMALS) +
                " hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS);
    }
    line += " patches=" + ::godot::String::num_uint64(resources_.patch_count());
    const VivariumPatchView* patch = place == GODOT_NO_PLACE ? nullptr : resources_.get_patch_at(place);
    if (patch == nullptr) {
        return line;
    }
    return "place=" + ::godot::String::num_uint64(place) + " object=" + ::godot::String::num_uint64(patch->object_id) +
           " " + line + " coverage=" + ::godot::String::num(patch->coverage, GODOT_LOG_DECIMALS) +
           " stage=" + ::godot::String::num_uint64(patch->stage) +
           " condition=" + ::godot::String::num_uint64(patch->condition) +
           " soil_mm=" + ::godot::String::num(patch->soil_mm, GODOT_LOG_DECIMALS) +
           " soil_rel=" + ::godot::String::num(patch->soil_rel, GODOT_LOG_DECIMALS) +
           " capacity_mm=" + ::godot::String::num(patch->capacity_mm, GODOT_LOG_DECIMALS);
}

void VivariumView::drain_log() {
    log_lines_.clear();
    if (resources_.remove_log_lines(log_lines_) == 0u) {
        return;
    }
    for (const std::string& line : log_lines_) {
        ::godot::UtilityFunctions::print(text_of(line));
    }
}

void VivariumView::note(const ::godot::String& line) {
    const ::godot::CharString text = line.utf8();
    if (resources_.note(text.get_data()) != ase::kernel::HostStatusOk) {
        // No host (before the first boot, after a failed create): Godot's own output carries it.
        ::godot::UtilityFunctions::print(::godot::String("[") + GODOT_LOG_SOURCE + "] " + line);
    }
}

}  // namespace ase::adp::godot

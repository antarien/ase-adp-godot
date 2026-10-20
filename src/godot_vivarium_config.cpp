/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW CONFIGURATION
 *
 * @file        godot_vivarium_config.cpp
 * @brief       VivariumView - the start configuration, the stage of the running build, the modes and the view section
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3: the view reads its configuration as data
 *              (res://config/vivarium_start.json) and never falls back to a number of the code - a missing
 *              or broken key stops it with that key on its error panel (fail_config). This file reads the
 *              whole file and hands its sections on: the host part (godot_vivarium_host_config.cpp), the
 *              layout and the texts (godot_vivarium_layout.cpp), the asset set (godot_vivarium_assets.cpp);
 *              the modes and the view section it reads itself.
 *
 *              THE STAGE IS THE RUNNING BUILD'S (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1):
 *              res://native/<os>-<arch>/<config>, named by Godot's own words for this build.
 *              Bundle index, manifests and build identity are read from there through FileAccess,
 *              so a stage packed into an APK reads like one on disk. The units are opened beside
 *              the adapter when Godot loaded the adapter from a file on disk, else by their bare
 *              names in the app's linker namespace - never through res:// or a package path. The
 *              data files the units read lie below a REAL directory: the stage itself when it is
 *              one on disk, else the same files written out below user://.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

/** build.identity: one `key = value` per line, written by scripts/stage_native.py. */
::godot::Dictionary read_identity(const ::godot::String& path) {
    ::godot::Dictionary identity;
    const ::godot::String separator(" = ");
    const ::godot::PackedStringArray lines = ::godot::FileAccess::get_file_as_string(path).split("\n", false);
    for (int64_t i = 0; i < lines.size(); ++i) {
        const ::godot::String& line = lines[i];
        const int64_t at = line.find(separator);
        if (line.begins_with("#") || at <= 0) {
            continue;
        }
        identity[line.substr(0, at)] = line.substr(at + separator.length());
    }
    return identity;
}

/**
 * VivariumStageReadFn of the view: one text file of the stage (bundle index, manifest), through
 * FileAccess - so a stage inside an exported package reads like one on disk. user is the view's
 * stage directory (res://). Both files are UTF-8 text, so their bytes survive the round trip.
 */
bool read_stage_file(const char* name, std::string& out, void* user) {
    const ::godot::String path = static_cast<const ::godot::String*>(user)->path_join(::godot::String::utf8(name));
    if (!::godot::FileAccess::file_exists(path)) {
        return false;
    }
    const ::godot::CharString text = ::godot::FileAccess::get_file_as_string(path).utf8();
    out.assign(text.get_data(), static_cast<std::size_t>(text.length()));
    return true;
}

}  // namespace

bool VivariumView::load_config() {
    const ::godot::String text = ::godot::FileAccess::get_file_as_string(GODOT_CONFIG_PATH);
    if (text.is_empty()) {
        return fail_config(GODOT_CONFIG_PATH);
    }
    const ::godot::Variant parsed = ::godot::JSON::parse_string(text);
    if (parsed.get_type() != ::godot::Variant::DICTIONARY) {
        return fail_config(GODOT_CONFIG_PATH);
    }
    const ::godot::Dictionary root = parsed;
    if (!load_host_config(root) || !load_layout(root) || !load_texts(root) || !load_modes(root) ||
        !load_view(root)) {
        return false;  // the failing key stands on the error panel already
    }
    if (!load_stage() || !load_assets(root)) {
        return false;
    }
    start_.log_file = utf8_of(::godot::ProjectSettings::get_singleton()->globalize_path(GODOT_LOG_PATH));
    return true;
}

bool VivariumView::load_stage() {
    // THE STAGE OF THE RUNNING BUILD, named by Godot's own words for it - the words the
    // avt.gdextension sections are keyed by. Never the working directory.
    ::godot::OS* os = ::godot::OS::get_singleton();
    stage_dir_ = ::godot::String(GODOT_STAGE_ROOT)
                     .path_join(os->get_name().to_lower() + GODOT_STAGE_JOIN +
                                ::godot::Engine::get_singleton()->get_architecture_name())
                     .path_join(os->is_debug_build() ? GODOT_STAGE_DEBUG : GODOT_STAGE_RELEASE);
    start_.read_stage = &read_stage_file;
    start_.read_stage_user = &stage_dir_;

    // WHERE THE UNITS ARE OPENED: beside the adapter when Godot loaded the adapter from a file on
    // disk (desktop) - and then that file MUST lie in this stage, or the view would read one
    // stage's index and load another stage's libraries. When Godot opened the adapter by its bare
    // name (Android: the APK's native library directory, searched by the app's linker
    // namespace), the units are opened by their bare names too (PLAN 02.1).
    ::godot::String library_path;
    ::godot::internal::gdextension_interface_get_library_path(::godot::internal::library,
                                                              library_path._native_ptr());
    if (library_path.begins_with("res://") || library_path.begins_with("user://")) {
        library_path = ::godot::ProjectSettings::get_singleton()->globalize_path(library_path);
    }
    start_.library_dir.clear();
    if (library_path.is_absolute_path() && ::godot::FileAccess::file_exists(library_path)) {
        const ::godot::String library_dir = library_path.get_base_dir().simplify_path();
        const ::godot::String stage_on_disk =
            ::godot::ProjectSettings::get_singleton()->globalize_path(stage_dir_).simplify_path();
        if (library_dir != stage_on_disk) {
            return fail_config("stage");
        }
        start_.library_dir = utf8_of(library_dir);
    }
    identity_ = read_identity(stage_dir_.path_join(GODOT_IDENTITY_FILE));
    return stage_data();
}

bool VivariumView::stage_data() {
    // THE DATA FILES LIE BELOW A REAL DIRECTORY (PLAN 01.2 point 4): the units read them with the
    // file system, never through res://. A stage on disk (the desktop) is that directory itself;
    // a stage inside a package (an APK) has its data files written out below user:// first - exactly
    // the files the bundle index names, below the same data root.
    const ::godot::String stage_on_disk =
        ::godot::ProjectSettings::get_singleton()->globalize_path(stage_dir_).simplify_path();
    if (::godot::DirAccess::dir_exists_absolute(stage_on_disk)) {
        start_.files_dir = utf8_of(stage_on_disk);
        return true;
    }
    std::string index;
    VivariumBundleInfo bundle;
    if (!read_stage_file(GODOT_BUNDLE_INDEX, index, &stage_dir_) ||
        !GodotHostResourceManager::parse_index(index, bundle)) {
        return fail_config(GODOT_BUNDLE_INDEX);
    }
    const ::godot::String copy_root(GODOT_DATA_COPY_ROOT);
    for (const std::string& relative : bundle.data) {
        const ::godot::String inside = text_of(bundle.data_root).path_join(text_of(relative));
        const ::godot::PackedByteArray bytes = ::godot::FileAccess::get_file_as_bytes(stage_dir_.path_join(inside));
        const ::godot::String target = copy_root.path_join(inside);
        if (bytes.is_empty() ||
            ::godot::DirAccess::make_dir_recursive_absolute(target.get_base_dir()) != ::godot::OK) {
            return fail_config("data");
        }
        const ::godot::Ref<::godot::FileAccess> file = ::godot::FileAccess::open(target, ::godot::FileAccess::WRITE);
        if (file.is_null() || !file->store_buffer(bytes)) {
            return fail_config("data");
        }
    }
    start_.files_dir = utf8_of(::godot::ProjectSettings::get_singleton()->globalize_path(copy_root).simplify_path());
    return true;
}

bool VivariumView::load_modes(const ::godot::Dictionary& root) {
    // Both modes are configured always and run only on request: a missing section is a broken
    // configuration, not a switched-off mode.
    ::godot::Dictionary modes;
    ::godot::Dictionary selftest;
    ::godot::Dictionary measure;
    if (!read_section(root, "modes", modes)) {
        return fail_config("modes");
    }
    if (!read_section(modes, "selftest", selftest) || !read_text(selftest, "argument", modes_.selftest_argument) ||
        !read_text(selftest, "table", modes_.selftest_table)) {
        return fail_config("modes.selftest");
    }
    if (!read_section(modes, "measure", measure) || !read_text(measure, "argument", modes_.measure_argument) ||
        !read_number(measure, "warmup_seconds", modes_.measure_warmup_seconds) ||
        !read_number(measure, "record_seconds", modes_.measure_record_seconds) ||
        !(modes_.measure_warmup_seconds >= 0.0) || !(modes_.measure_record_seconds > 0.0)) {
        return fail_config("modes.measure");
    }
    ::godot::Dictionary assets;
    if (!read_section(modes, "assets", assets) || !read_text(assets, "argument", modes_.assets_argument)) {
        return fail_config("modes.assets");
    }
    return true;
}

bool VivariumView::load_view(const ::godot::Dictionary& root) {
    // THE 3D SCENE (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2): the near view and the visible orders
    // by frame time are numbers of the configuration, never of the code. The organ templates are no
    // number of the view: ase-render-data lays them on the template ports (usecase:101011 lay out organ
    // templates). A missing or broken key is a broken configuration: the scene has no default to fall
    // back on.
    ::godot::Dictionary view;
    if (!read_section(root, GODOT_VIEW_KEY, view)) {
        return fail_config(GODOT_VIEW_KEY);
    }
    double distance = 0.0;
    double height = 0.0;
    double look = 0.0;
    double fov = 0.0;
    if (!read_number(view, GODOT_VIEW_KEY_DISTANCE, distance) || !(distance > 0.0) ||
        !read_number(view, GODOT_VIEW_KEY_HEIGHT, height) || !read_number(view, GODOT_VIEW_KEY_LOOK, look) ||
        !read_number(view, GODOT_VIEW_KEY_FOV, fov) || !(fov > 0.0)) {
        return fail_config("view.camera");
    }
    scene_camera_.distance_m = static_cast<float>(distance);
    scene_camera_.height_m = static_cast<float>(height);
    scene_camera_.look_m = static_cast<float>(look);
    scene_camera_.fov_deg = static_cast<float>(fov);
    double budget_ms = 0.0;
    double headroom = 0.0;
    double ceiling = 0.0;
    double start = 0.0;
    double calm = 0.0;
    double over = 0.0;
    if (!read_number(view, GODOT_VIEW_KEY_BUDGET, budget_ms) || !read_number(view, GODOT_VIEW_KEY_HEADROOM, headroom) ||
        !read_number(view, GODOT_VIEW_KEY_CEILING, ceiling) || !read_number(view, GODOT_VIEW_KEY_START, start) ||
        !read_number(view, GODOT_VIEW_KEY_CALM, calm) || !read_number(view, GODOT_VIEW_KEY_OVER, over) ||
        !(ceiling >= 0.0) || !(start >= 0.0) || !(calm >= 1.0) || !(over >= 1.0) ||
        ase::math::floor(static_cast<float>(ceiling)) != static_cast<float>(ceiling) ||
        ase::math::floor(static_cast<float>(start)) != static_cast<float>(start) ||
        ase::math::floor(static_cast<float>(calm)) != static_cast<float>(calm) ||
        ase::math::floor(static_cast<float>(over)) != static_cast<float>(over)) {
        return fail_config("view.orders");
    }
    order_budget_ = OrderBudget{};
    order_budget_.frame_budget_ms = budget_ms;
    order_budget_.headroom = headroom;
    order_budget_.ceiling = static_cast<uint32_t>(ceiling);
    order_budget_.shown = static_cast<uint32_t>(start);
    order_budget_.calm_frames = static_cast<uint32_t>(calm);
    order_budget_.over_frames = static_cast<uint32_t>(over);
    if (!order_budget_valid(order_budget_)) {
        return fail_config("view.orders");
    }
    double pick = 0.0;
    if (!read_number(view, GODOT_VIEW_KEY_PICK, pick) || !(pick > 0.0)) {
        return fail_config("view.pick_px");
    }
    pick_px_ = static_cast<float>(pick);
    return true;
}

bool VivariumView::launched_with(const ::godot::String& argument) const {
    // Godot's user arguments: everything after `--` on the command line, and on Android the
    // strings of the intent extra command_line_params after their own `--`.
    return !argument.is_empty() && ::godot::OS::get_singleton()->get_cmdline_user_args().has(argument);
}

bool VivariumView::fail_config(const char* key) {
    // UTF-8 on purpose: String(const char*) reads Latin-1, and the title carries "ç" and "ã".
    failure_text_ = ::godot::String::utf8(GODOT_TEXT_CONFIG_ERROR) + ": " + key;
    ::godot::UtilityFunctions::push_error(failure_text_);
    return false;
}

}  // namespace ase::adp::godot

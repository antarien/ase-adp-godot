/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW IMPLEMENTATION
 *
 * @file        godot_vivarium_node.cpp
 * @brief       VivariumView - lifecycle, tick, input and drawing of the Vivarium frontend
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3/01.4. The view reads its configuration as
 *              data (res://config/vivarium_start.json), boots one embedded host through
 *              GodotHostResourceManager from the bundle Godot loaded this library from, advances
 *              it once per frame with dt capped, and draws what the last snapshot reported.
 *
 *              EVERY ENGINE CALL GOES THROUGH node_: drawing, redraw requests, processing, input
 *              mapping and the viewport are those of the engine object this view is the instance
 *              data of (see godot_vivarium_node.hpp, COMPOSED, NOT DERIVED).
 *
 *              THE LOG IS ONE STREAM. The host's logger (ase-log inside the runtime library)
 *              writes the file under user://logs and hands every line to the resource manager;
 *              the view prints them once per frame (on Android the manager writes them to logcat
 *              itself). Its own lines - the boot line, every action with its target and result,
 *              every stage change - go through the SAME logger via KernelEmbeddedHost::note,
 *              never around it. Nothing is logged per render frame.
 *
 *              THE STAGE IS THE RUNNING BUILD'S (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1):
 *              res://native/<os>-<arch>/<config>, named by Godot's own words for this build.
 *              Bundle index, manifest and build identity are read from there through FileAccess,
 *              so a stage packed into an APK reads like one on disk. The plugin is opened beside
 *              the adapter when Godot loaded the adapter from a file on disk, else by its bare
 *              name in the app's linker namespace - never through res:// or a package path.
 *
 *              TWO EXPLICIT MODES (PLAN 02.3), off unless the launch names them: the self-test
 *              runs the table config/vivarium_selftest.toml on test hosts of its own BEFORE the
 *              game host boots; the measurement records host and frame times after a warm-up and
 *              writes one block when the recording is complete.
 *
 *              THE LAYOUT IS PLACED, NOT STRETCHED (PLAN 02.3): the expand aspect of the project
 *              hands the view the whole visible surface, of any aspect ratio. draw() fills it with
 *              the background, places the three bands of the layout through place_view and draws
 *              every element with the transform of its band; the header colour reaches up to the
 *              top edge, over a camera cut-out. tap() matches the rectangles as that frame placed
 *              them, so a tap lands where the button was seen.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-06
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

namespace {

// Configuration readers: each answers false for a missing key or a wrong type.

bool numeric(const ::godot::Variant& value) {
    return value.get_type() == ::godot::Variant::FLOAT || value.get_type() == ::godot::Variant::INT;
}

bool read_number(const ::godot::Dictionary& from, const char* key, double& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (!numeric(value)) {
        return false;
    }
    out = static_cast<double>(value);
    return std::isfinite(out);
}

bool read_text(const ::godot::Dictionary& from, const char* key, ::godot::String& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (value.get_type() != ::godot::Variant::STRING) {
        return false;
    }
    out = static_cast<::godot::String>(value);
    return !out.is_empty();
}

bool read_section(const ::godot::Dictionary& from, const char* key, ::godot::Dictionary& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (value.get_type() != ::godot::Variant::DICTIONARY) {
        return false;
    }
    out = static_cast<::godot::Dictionary>(value);
    return true;
}

bool read_numbers(const ::godot::Variant& value, int64_t count, ::godot::Array& out) {
    if (value.get_type() != ::godot::Variant::ARRAY) {
        return false;
    }
    out = static_cast<::godot::Array>(value);
    if (out.size() != count) {
        return false;
    }
    for (int64_t i = 0; i < count; ++i) {
        if (!numeric(out[i])) {
            return false;
        }
    }
    return true;
}

bool read_rect(const ::godot::Dictionary& from, const char* key, ::godot::Rect2& out) {
    ::godot::Array values;
    if (!read_numbers(from.get(key, ::godot::Variant()), GODOT_RECT_FIELDS, values)) {
        return false;
    }
    out = ::godot::Rect2(static_cast<float>(values[0]), static_cast<float>(values[1]),
                         static_cast<float>(values[2]), static_cast<float>(values[3]));
    return out.size.x > 0.0f && out.size.y > 0.0f;
}

bool read_point(const ::godot::Dictionary& from, const char* key, ::godot::Vector2& out) {
    ::godot::Array values;
    if (!read_numbers(from.get(key, ::godot::Variant()), GODOT_POINT_FIELDS, values)) {
        return false;
    }
    out = ::godot::Vector2(static_cast<float>(values[0]), static_cast<float>(values[1]));
    return true;
}

std::string utf8_of(const ::godot::String& text) {
    return std::string(text.utf8().get_data());
}

::godot::String text_of(const std::string& text) {
    return ::godot::String::utf8(text.c_str(), static_cast<int64_t>(text.size()));
}

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

::godot::String identity_value(const ::godot::Dictionary& identity, const char* key) {
    return static_cast<::godot::String>(identity.get(key, "?"));
}

/** value / full, held inside 0..1 - a drawn share, never a simulation value. */
double share(double value, double full) {
    if (!(full > 0.0) || !(value > 0.0)) {
        return 0.0;
    }
    const double part = value / full;
    return part < 1.0 ? part : 1.0;
}

/** The port's own stage word, for log lines (stable across the UI language). */
const char* stage_key(uint8_t stage) {
    if (stage == GODOT_STAGE_DEAD) {
        return GODOT_KEY_DEAD;
    }
    if (stage == GODOT_STAGE_MATURE) {
        return GODOT_KEY_MATURE;
    }
    if (stage == GODOT_STAGE_SPROUT) {
        return GODOT_KEY_SPROUT;
    }
    if (stage == GODOT_STAGE_SEED) {
        return GODOT_KEY_SEED;
    }
    return "none";
}

::godot::Color stage_color(uint8_t stage) {
    if (stage == GODOT_STAGE_DEAD) {
        return ::godot::Color::hex(GODOT_COLOR_DEAD);
    }
    if (stage == GODOT_STAGE_MATURE) {
        return ::godot::Color::hex(GODOT_COLOR_MATURE);
    }
    if (stage == GODOT_STAGE_SPROUT) {
        return ::godot::Color::hex(GODOT_COLOR_SPROUT);
    }
    return ::godot::Color::hex(GODOT_COLOR_SEED);
}

::godot::Ref<::godot::Font> view_font() {
    return ::godot::ThemeDB::get_singleton()->get_fallback_font();
}

/** The largest size from `size` down to GODOT_FONT_MIN at which `text` fits into `width`. */
int32_t fitting_size(const ::godot::String& text, float width, int32_t size) {
    const ::godot::Ref<::godot::Font> font = view_font();
    while (size > GODOT_FONT_MIN &&
           font->get_string_size(text, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, size).x > width) {
        --size;
    }
    return size;
}

bool in_editor() {
    return ::godot::Engine::get_singleton()->is_editor_hint();
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

/** Nearest-rank percentile of `samples` (taken by copy: the order of the recording stays). */
uint32_t percentile(std::vector<uint32_t> samples, uint32_t percent) {
    if (samples.empty()) {
        return 0u;
    }
    const std::size_t rank =
        (static_cast<std::size_t>(percent) * samples.size() + GODOT_MEASURE_PERCENT - 1u) / GODOT_MEASURE_PERCENT;
    const std::size_t index = rank == 0u ? 0u : rank - 1u;
    std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(index), samples.end());
    return samples[index];
}

/** "p50=.. p95=.. p99=.. max=.." of one recorded series, in microseconds. */
::godot::String series(const std::vector<uint32_t>& samples) {
    const uint32_t largest = samples.empty() ? 0u : *std::max_element(samples.begin(), samples.end());
    return "p50=" + ::godot::String::num_uint64(percentile(samples, GODOT_MEASURE_P50)) +
           " p95=" + ::godot::String::num_uint64(percentile(samples, GODOT_MEASURE_P95)) +
           " p99=" + ::godot::String::num_uint64(percentile(samples, GODOT_MEASURE_P99)) +
           " max=" + ::godot::String::num_uint64(largest);
}

/** A microsecond count as a sample: a frame longer than 71 minutes is a stall, held at the top. */
uint32_t sample_of(uint64_t usec) {
    return usec > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(usec);
}

}  // anonymous namespace

/**
 * LIFETIME
 */

VivariumView::VivariumView(::godot::Node2D* node) : node_(node) {}

VivariumView::~VivariumView() {
    // An object freed without _exit_tree (an orphan) still tears its host down in the binding
    // order. node_ is not touched here: Godot frees this view before the object's handle.
    resources_.clear_all();
}

void VivariumView::ready() {
    // V0: importing or editing the scene starts no host and loads no plugin.
    if (in_editor()) {
        node_->set_process(false);
        return;
    }
    configured_ = load_config();
    if (configured_) {
        // PLAN 02.3: the self-test runs on test hosts of its own BEFORE the game host exists, and
        // only when the launch asks for it; its report goes through the game host once that is up.
        if (launched_with(modes_.selftest_argument)) {
            run_self_test();
        }
        measurement_.active = launched_with(modes_.measure_argument);
        measurement_.warmup_left = modes_.measure_warmup_seconds;
        measurement_.record_left = modes_.measure_record_seconds;
        boot_host();
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
    note("exit " + measure(GODOT_NO_OBJECT));
    resources_.clear_all();
    drain_log();
}

/**
 * CONFIGURATION (res://config/vivarium_start.json)
 */

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
    if (!load_host_config(root) || !load_layout(root) || !load_texts(root) || !load_modes(root)) {
        return false;  // the failing key stands on the error panel already
    }
    if (!load_stage()) {
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

    // WHERE THE PLUGIN IS OPENED: beside the adapter when Godot loaded the adapter from a file on
    // disk (desktop) - and then that file MUST lie in this stage, or the view would read one
    // stage's index and load another stage's libraries. When Godot opened the adapter by its bare
    // name (Android: the APK's native library directory, searched by the app's linker
    // namespace), the plugin is opened by its bare name too (PLAN 02.1).
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

bool VivariumView::load_host_config(const ::godot::Dictionary& root) {
    ::godot::String text;
    if (!read_text(root, "port", text)) {
        return fail_config("port");
    }
    start_.port = utf8_of(text);

    ::godot::Dictionary operations;
    if (!read_section(root, "operations", operations)) {
        return fail_config("operations");
    }
    if (!read_text(operations, "create", text)) {
        return fail_config("operations.create");
    }
    start_.op_create = utf8_of(text);
    if (!read_text(operations, "irrigate", text)) {
        return fail_config("operations.irrigate");
    }
    start_.op_irrigate = utf8_of(text);

    double number = 0.0;
    if (!read_number(root, "tick_max_seconds", number)) {
        return fail_config("tick_max_seconds");
    }
    start_.tick_max_seconds = static_cast<float>(number);
    if (!read_number(root, "irrigate_amount", start_.irrigate_amount)) {
        return fail_config("irrigate_amount");
    }

    ::godot::Dictionary start;
    if (!read_section(root, "start", start)) {
        return fail_config("start");
    }
    if (!read_number(start, "biomass", start_.start_biomass)) {
        return fail_config("start.biomass");
    }
    if (!read_number(start, "age_seconds", start_.start_age_seconds)) {
        return fail_config("start.age_seconds");
    }

    const ::godot::Variant patches_value = root.get("patches", ::godot::Variant());
    if (patches_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("patches");
    }
    const ::godot::Array patches = patches_value;
    start_.patches.clear();
    layout_.patch_rects.clear();
    for (int64_t i = 0; i < patches.size(); ++i) {
        if (patches[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("patches[]");
        }
        const ::godot::Dictionary patch = patches[i];
        double object_id = 0.0;
        VivariumPatchStart entry;
        ::godot::Rect2 rect;
        if (!read_number(patch, "id", object_id) ||
            ase::math::floor(static_cast<float>(object_id)) != static_cast<float>(object_id) ||
            object_id <= static_cast<double>(GODOT_NO_OBJECT)) {
            return fail_config("patches[].id");
        }
        if (!read_number(patch, "moisture", entry.moisture)) {
            return fail_config("patches[].moisture");
        }
        if (!read_rect(patch, "rect", rect)) {
            return fail_config("patches[].rect");
        }
        entry.object_id = static_cast<uint32_t>(object_id);
        start_.patches.push_back(entry);
        layout_.patch_rects.push_back(rect);
    }
    if (start_.patches.empty()) {
        return fail_config("patches");
    }
    return true;
}

bool VivariumView::load_layout(const ::godot::Dictionary& root) {
    double number = 0.0;
    ::godot::Dictionary size;
    if (!read_section(root, "logical_size", size)) {
        return fail_config("logical_size");
    }
    if (!read_number(size, "width", number)) {
        return fail_config("logical_size.width");
    }
    layout_.size.x = static_cast<float>(number);
    if (!read_number(size, "height", number)) {
        return fail_config("logical_size.height");
    }
    layout_.size.y = static_cast<float>(number);

    ::godot::Dictionary display;
    if (!read_section(root, "display", display)) {
        return fail_config("display");
    }
    if (!read_number(display, "biomass_full", layout_.biomass_full)) {
        return fail_config("display.biomass_full");
    }
    if (!read_number(display, "moisture_full", layout_.moisture_full)) {
        return fail_config("display.moisture_full");
    }

    ::godot::Dictionary layout;
    if (!read_section(root, "layout", layout)) {
        return fail_config("layout");
    }
    if (!read_number(layout, "header_height", number)) {
        return fail_config("layout.header_height");
    }
    layout_.header_height = static_cast<float>(number);
    if (!read_point(layout, "title", layout_.title_at)) {
        return fail_config("layout.title");
    }
    if (!read_point(layout, "build", layout_.build_at)) {
        return fail_config("layout.build");
    }
    if (!read_point(layout, "info", layout_.info_at)) {
        return fail_config("layout.info");
    }
    if (!read_number(layout, "info_line", number)) {
        return fail_config("layout.info_line");
    }
    layout_.info_line = static_cast<float>(number);
    if (!read_point(layout, "note", layout_.note_at)) {
        return fail_config("layout.note");
    }

    ::godot::Dictionary buttons;
    ::godot::Dictionary button;
    if (!read_section(root, "buttons", buttons)) {
        return fail_config("buttons");
    }
    if (!read_section(buttons, "water", button) || !read_rect(button, "rect", layout_.water_rect) ||
        !read_text(button, "label", texts_.water)) {
        return fail_config("buttons.water");
    }
    if (!read_section(buttons, "pause", button) || !read_rect(button, "rect", layout_.pause_rect) ||
        !read_text(button, "label", texts_.pause) || !read_text(button, "label_resume", texts_.resume)) {
        return fail_config("buttons.pause");
    }
    if (!read_section(buttons, "reset", button) || !read_rect(button, "rect", layout_.reset_rect) ||
        !read_text(button, "label", texts_.reset)) {
        return fail_config("buttons.reset");
    }
    // The bands come last: they are checked against every element read above.
    return load_bands(layout);
}

bool VivariumView::load_bands(const ::godot::Dictionary& layout) {
    ::godot::Dictionary bands;
    double top_end = 0.0;
    double bottom_start = 0.0;
    if (!read_section(layout, "bands", bands) || !read_number(bands, "top_end", top_end) ||
        !read_number(bands, "bottom_start", bottom_start)) {
        return fail_config("layout.bands");
    }
    ViewBands& placed_bands = layout_.bands;
    placed_bands.width = layout_.size.x;
    placed_bands.height = layout_.size.y;
    placed_bands.top_end = static_cast<float>(top_end);
    placed_bands.bottom_start = static_cast<float>(bottom_start);
    if (!(placed_bands.width > 0.0f) || !(placed_bands.top_end > 0.0f) ||
        !(placed_bands.top_end <= placed_bands.bottom_start) || !(placed_bands.bottom_start < placed_bands.height)) {
        return fail_config("layout.bands");
    }
    // EVERY ELEMENT IN ONE BAND: on a taller surface the bands move apart, and an element across
    // an edge would be drawn in two pieces. A text line belongs to the band of its baseline; the
    // info block is three lines, so its third baseline is checked with the first.
    if (!in_one_band(placed_bands, 0.0f, layout_.header_height)) {
        return fail_config("layout.bands: header_height");
    }
    if (!in_one_band(placed_bands, layout_.title_at.y, layout_.title_at.y)) {
        return fail_config("layout.bands: title");
    }
    if (!in_one_band(placed_bands, layout_.build_at.y, layout_.build_at.y)) {
        return fail_config("layout.bands: build");
    }
    if (!in_one_band(placed_bands, layout_.info_at.y, layout_.info_at.y + 2.0f * layout_.info_line)) {
        return fail_config("layout.bands: info");
    }
    if (!in_one_band(placed_bands, layout_.note_at.y, layout_.note_at.y)) {
        return fail_config("layout.bands: note");
    }
    for (const ::godot::Rect2& rect : layout_.patch_rects) {
        if (!in_one_band(placed_bands, rect.position.y, rect.get_end().y)) {
            return fail_config("layout.bands: patches[].rect");
        }
    }
    if (!in_one_band(placed_bands, layout_.water_rect.position.y, layout_.water_rect.get_end().y)) {
        return fail_config("layout.bands: buttons.water");
    }
    if (!in_one_band(placed_bands, layout_.pause_rect.position.y, layout_.pause_rect.get_end().y)) {
        return fail_config("layout.bands: buttons.pause");
    }
    if (!in_one_band(placed_bands, layout_.reset_rect.position.y, layout_.reset_rect.get_end().y)) {
        return fail_config("layout.bands: buttons.reset");
    }
    return true;
}

bool VivariumView::load_texts(const ::godot::Dictionary& root) {
    ::godot::Dictionary texts;
    if (!read_section(root, "texts", texts)) {
        return fail_config("texts");
    }
    if (!read_text(texts, "title", texts_.title)) {
        return fail_config("texts.title");
    }
    if (!read_text(texts, "selection", texts_.selection)) {
        return fail_config("texts.selection");
    }
    if (!read_text(texts, "selection_none", texts_.selection_none)) {
        return fail_config("texts.selection_none");
    }
    if (!read_text(texts, "area", texts_.area)) {
        return fail_config("texts.area");
    }
    if (!read_text(texts, "simulation", texts_.simulation)) {
        return fail_config("texts.simulation");
    }
    if (!read_text(texts, "ticks", texts_.ticks)) {
        return fail_config("texts.ticks");
    }
    if (!read_text(texts, "biomass", texts_.biomass)) {
        return fail_config("texts.biomass");
    }
    if (!read_text(texts, "moisture", texts_.moisture)) {
        return fail_config("texts.moisture");
    }
    if (!read_text(texts, "age", texts_.age)) {
        return fail_config("texts.age");
    }
    if (!read_text(texts, "stage_seed", texts_.stage_seed)) {
        return fail_config("texts.stage_seed");
    }
    if (!read_text(texts, "stage_sprout", texts_.stage_sprout)) {
        return fail_config("texts.stage_sprout");
    }
    if (!read_text(texts, "stage_mature", texts_.stage_mature)) {
        return fail_config("texts.stage_mature");
    }
    if (!read_text(texts, "stage_dead", texts_.stage_dead)) {
        return fail_config("texts.stage_dead");
    }
    if (!read_text(texts, "paused", texts_.paused)) {
        return fail_config("texts.paused");
    }
    if (!read_text(texts, "restart_note", texts_.restart_note)) {
        return fail_config("texts.restart_note");
    }
    if (!read_text(texts, "error", texts_.error)) {
        return fail_config("texts.error");
    }
    if (!read_text(texts, "separator", texts_.separator)) {
        return fail_config("texts.separator");
    }
    return true;
}

/**
 * HOST LIFECYCLE
 */

void VivariumView::boot_host() {
    failure_text_ = ::godot::String();
    selected_id_ = GODOT_NO_OBJECT;
    water_pending_id_ = GODOT_NO_OBJECT;
    // The frame after a boot carries the boot time, not simulation time.
    drop_next_delta_ = true;
    stages_.assign(start_.patches.size(), GODOT_STAGE_NONE);
    host_boots_ += 1u;

    const HostStatus status = resources_.boot(start_);
    if (status != ase::kernel::HostStatusOk) {
        failure_text_ = texts_.error + ": " + resources_.failure_step() + " (" +
                        GodotHostResourceManager::status_name(status) + ")";
        ::godot::UtilityFunctions::push_error(failure_text_);
        drain_log();
        return;
    }

    const VivariumBundleInfo& bundle = resources_.get_bundle();
    build_line_ = ::godot::String("adapter ") + ASE_ADP_GODOT_VERSION + texts_.separator + "client " +
                  identity_value(identity_, "client_commit") + texts_.separator + text_of(bundle.plugin) +
                  " " + text_of(bundle.version);
    // The build line keeps to the surface: it shrinks until it fits beside the title's margins.
    build_font_size_ = fitting_size(build_line_, layout_.size.x - 2.0f * layout_.build_at.x, GODOT_FONT_SMALL);
    note(boot_line());
    follow_stages();
    drain_log();
}

void VivariumView::restart() {
    // Reiniciar: the same teardown as exit_tree, then a new host with the four start patches.
    // Inputs, ids and drawing values of the old run go with it.
    note("restart " + measure(GODOT_NO_OBJECT));
    resources_.clear_all();
    drain_log();
    user_paused_ = false;
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
    return ::godot::String("boot")
        + " client=" + identity_value(identity_, "client_commit")
        + " client_version=" + identity_value(identity_, "client_version")
        + " adapter=" + ASE_ADP_GODOT_VERSION
        + " kernel=" + identity_value(identity_, "kernel_version")
        + " plugin=" + text_of(bundle.plugin) + "@" + text_of(bundle.version)
        + " ase_api=" + text_of(bundle.api_version)
        + " host_port_abi=" + ::godot::String::num_uint64(ase::kernel::HostPortAbiV1)
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
        + " plugin_load=" + (start_.library_dir.empty() ? "name" : "path")
        + " selftest=" + (selftest_ran_ ? "errors:" + ::godot::String::num_uint64(selftest_errors_) : "off")
        + " measure=" + (measurement_.active ? "on" : "off")
        + " boots=" + ::godot::String::num_uint64(host_boots_)
        + " patches=" + ::godot::String::num_uint64(resources_.patch_count())
        + " systems=" + ::godot::String::num_uint64(resources_.system_count());
}

/**
 * EXPLICIT MODES (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3)
 */

void VivariumView::run_self_test() {
    // The table is a staged file like the start configuration; a missing one is reported by the
    // manager as a self-test that could not run - never as one without errors.
    const ::godot::String table = ::godot::FileAccess::get_file_as_string(modes_.selftest_table);
    selftest_report_.clear();
    const VivariumSelfTestResult result = resources_.self_test(start_, utf8_of(table), selftest_report_);
    selftest_ran_ = true;
    selftest_errors_ = result.errors;
    drain_log();  // the test hosts' own lines come before the game host's
}

void VivariumView::measure_frame(uint64_t frame_start_usec, uint64_t host_usec) {
    VivariumMeasurement& measurement = measurement_;
    const uint64_t previous = measurement.last_usec;
    measurement.last_usec = frame_start_usec;
    // The first ticking frame after a start or a pause has no interval of its own.
    if (previous == 0u || frame_start_usec <= previous) {
        return;
    }
    const uint64_t interval = frame_start_usec - previous;
    const double seconds = static_cast<double>(interval) / GODOT_USEC_PER_SECOND;
    if (!measurement.recording) {
        measurement.warmup_left -= seconds;
        if (measurement.warmup_left > 0.0) {
            return;
        }
        measurement.recording = true;
        const auto reserved = static_cast<std::size_t>(modes_.measure_record_seconds * GODOT_MEASURE_FRAMES_RESERVED);
        measurement.host_usec.reserve(reserved);
        measurement.frame_usec.reserve(reserved);
        measurement.pss_start_read = resources_.read_pss_kib(measurement.pss_start_kib);
        measurement.boots_at_start = host_boots_;
        note("measure begin warmup_s=" + ::godot::String::num(modes_.measure_warmup_seconds, GODOT_LOG_DECIMALS) +
             " record_s=" + ::godot::String::num(modes_.measure_record_seconds, GODOT_LOG_DECIMALS) + " " +
             measure(GODOT_NO_OBJECT));
        return;
    }
    measurement.host_usec.push_back(sample_of(host_usec));
    measurement.frame_usec.push_back(sample_of(interval));
    measurement.record_left -= seconds;
    if (measurement.record_left <= 0.0) {
        report_measurement();
    }
}

void VivariumView::report_measurement() {
    VivariumMeasurement& measurement = measurement_;
    uint64_t pss_end_kib = 0u;
    const bool pss_end_read = resources_.read_pss_kib(pss_end_kib);
    uint64_t recorded_usec = 0u;
    for (const uint32_t interval : measurement.frame_usec) {
        recorded_usec += interval;
    }
    const double recorded_seconds = static_cast<double>(recorded_usec) / GODOT_USEC_PER_SECOND;
    const double fps = recorded_seconds > 0.0 ? static_cast<double>(measurement.frame_usec.size()) / recorded_seconds : 0.0;
    // ONE BLOCK, written when the recording is complete: nothing was logged per frame.
    note("measure host_usec frames=" + ::godot::String::num_uint64(measurement.host_usec.size()) + " " +
         series(measurement.host_usec));
    note("measure frame_usec frames=" + ::godot::String::num_uint64(measurement.frame_usec.size()) + " " +
         series(measurement.frame_usec) + " fps=" + ::godot::String::num(fps, GODOT_LOG_DECIMALS) +
         " recorded_s=" + ::godot::String::num(recorded_seconds, GODOT_LOG_DECIMALS));
    note("measure end pss_kib_start=" +
         (measurement.pss_start_read ? ::godot::String::num_uint64(measurement.pss_start_kib) : ::godot::String("unknown")) +
         " pss_kib_end=" + (pss_end_read ? ::godot::String::num_uint64(pss_end_kib) : ::godot::String("unknown")) +
         " restarts=" + ::godot::String::num_uint64(host_boots_ - measurement.boots_at_start) +
         " pauses=" + ::godot::String::num_uint64(measurement.pauses) + " " + measure(GODOT_NO_OBJECT));
    measurement = VivariumMeasurement{};
}

/**
 * FRAME
 */

bool VivariumView::ticking() const {
    return resources_.running() && failure_text_.is_empty() && !user_paused_ && !app_paused_ &&
           !focus_lost_;
}

void VivariumView::process(double delta) {
    drain_log();
    // A surface of another size than the one the last frame was placed on - a resized window, a
    // folded handset - needs a new placement even while nothing ticks.
    const ::godot::Vector2 visible = node_->get_viewport_rect().size;
    if (visible.x != drawn_surface_.width || visible.y != drawn_surface_.height) {
        node_->queue_redraw();
    }
    if (!ticking()) {
        // A pause breaks the chain of frame intervals: the next ticking frame starts a new one.
        measurement_.last_usec = 0u;
        return;
    }
    if (drop_next_delta_) {
        // After a boot or a resume the first delta holds the time spent outside the simulation.
        drop_next_delta_ = false;
        measurement_.last_usec = 0u;
        return;
    }
    // Host time (tick + snapshot) is measured apart from the whole _process interval (PLAN 02.3),
    // on Godot's monotonic clock, never from delta - Godot may smooth that.
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t frame_start_usec = clock->get_ticks_usec();
    const HostStatus status = resources_.advance(static_cast<float>(delta));
    const uint64_t host_usec = clock->get_ticks_usec() - frame_start_usec;
    if (status == ase::kernel::HostStatusOk) {
        follow_stages();
        follow_water();
        if (measurement_.active) {
            measure_frame(frame_start_usec, host_usec);
        }
    } else if (!resources_.running()) {
        // The manager stopped the host (a broken snapshot) - the panel says where.
        failure_text_ = texts_.error + ": " + resources_.failure_step() + " (" +
                        GodotHostResourceManager::status_name(status) + ")";
        ::godot::UtilityFunctions::push_error(failure_text_);
    }
    // A refused dt with a running host drops this frame only; the host has logged why.
    node_->queue_redraw();
}

void VivariumView::notification(int32_t what) {
    if (in_editor()) {
        return;
    }
    // The pause reasons stay apart: an application resume or a regained focus never lifts the
    // pause the user chose (V5), and no reason lets ticks through in the background.
    if (what == ::godot::Node::NOTIFICATION_APPLICATION_PAUSED) {
        app_paused_ = true;
        measurement_.pauses += measurement_.recording ? 1u : 0u;
        note("pause reason=application " + measure(GODOT_NO_OBJECT));
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_RESUMED) {
        app_paused_ = false;
        drop_next_delta_ = true;
        note("resume reason=application " + measure(GODOT_NO_OBJECT));
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_FOCUS_OUT) {
        focus_lost_ = true;
        measurement_.pauses += measurement_.recording ? 1u : 0u;
        note("pause reason=focus " + measure(GODOT_NO_OBJECT));
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_FOCUS_IN) {
        focus_lost_ = false;
        drop_next_delta_ = true;
        note("resume reason=focus " + measure(GODOT_NO_OBJECT));
    }
}

uint8_t VivariumView::stage_of(uint32_t object_id) const {
    const VivariumPatchView* patch = resources_.get_patch(object_id);
    if (patch == nullptr) {
        return GODOT_STAGE_NONE;
    }
    if (patch->dead) {
        return GODOT_STAGE_DEAD;
    }
    if (patch->mature) {
        return GODOT_STAGE_MATURE;
    }
    if (patch->sprout) {
        return GODOT_STAGE_SPROUT;
    }
    return patch->seed ? GODOT_STAGE_SEED : GODOT_STAGE_NONE;
}

::godot::String VivariumView::stage_text(uint8_t stage) const {
    if (stage == GODOT_STAGE_DEAD) {
        return texts_.stage_dead;
    }
    if (stage == GODOT_STAGE_MATURE) {
        return texts_.stage_mature;
    }
    if (stage == GODOT_STAGE_SPROUT) {
        return texts_.stage_sprout;
    }
    return texts_.stage_seed;
}

void VivariumView::follow_stages() {
    // A stage change is an EVENT and gets one line - the plan's measurement, never a line per frame.
    for (size_t i = 0; i < start_.patches.size() && i < stages_.size(); ++i) {
        const uint32_t object_id = start_.patches[i].object_id;
        const uint8_t stage = stage_of(object_id);
        if (stage == stages_[i]) {
            continue;
        }
        if (stages_[i] != GODOT_STAGE_NONE) {
            note("stage " + measure(object_id) + " from=" + stage_key(stages_[i]));
        }
        stages_[i] = stage;
    }
}

void VivariumView::follow_water() {
    // Input → plugin pending → next Regulation step → snapshot: the step has run once the patch's
    // own age moved, and only then is the result logged (PLAN 01.3).
    if (water_pending_id_ == GODOT_NO_OBJECT) {
        return;
    }
    const VivariumPatchView* patch = resources_.get_patch(water_pending_id_);
    if (patch == nullptr) {
        water_pending_id_ = GODOT_NO_OBJECT;
        return;
    }
    if (patch->age_seconds == water_pending_age_) {
        return;
    }
    note("water applied " + measure(water_pending_id_) + " moisture_before=" +
         ::godot::String::num(water_pending_from_, GODOT_LOG_DECIMALS));
    water_pending_id_ = GODOT_NO_OBJECT;
}

::godot::String VivariumView::measure(uint32_t object_id) const {
    ::godot::String line = "t=" + ::godot::String::num(resources_.simulation_seconds(), GODOT_LOG_DECIMALS) +
                           " ticks=" + ::godot::String::num_uint64(resources_.tick_count());
    // Steps are the scheduler's count of the step schedule, never t divided by its interval. A host
    // that cannot answer (none yet, stopped) gives no steps= at all instead of a false 0; the host
    // logs its own refusal.
    uint64_t steps = 0u;
    if (resources_.schedule_runs(GODOT_LOG_STEP_SCHEDULE, &steps) == ase::kernel::HostStatusOk) {
        line += " steps=" + ::godot::String::num_uint64(steps);
    }
    line += " patches=" + ::godot::String::num_uint64(resources_.patch_count());
    const VivariumPatchView* patch = resources_.get_patch(object_id);
    if (patch == nullptr) {
        return line;
    }
    return "target=" + ::godot::String::num_uint64(object_id) + " " + line +
           " biomass=" + ::godot::String::num(patch->biomass, GODOT_LOG_DECIMALS) +
           " moisture=" + ::godot::String::num(patch->moisture, GODOT_LOG_DECIMALS) +
           " age=" + ::godot::String::num(patch->age_seconds, GODOT_LOG_DECIMALS) +
           " stage=" + stage_key(stage_of(object_id));
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

/**
 * INPUT
 */

void VivariumView::unhandled_input(const ::godot::Ref<::godot::InputEvent>& event) {
    if (in_editor() || !node_->is_visible_in_tree()) {
        return;
    }
    // ONE PATH PER PHYSICAL TAP: touch arrives as ScreenTouch, a mouse as MouseButton - the
    // project switches Godot's touch-to-mouse emulation off, so an Android tap waters once.
    // make_input_local maps through the inverse canvas transform onto the visible surface; tap()
    // matches that point against the layout as the last frame placed it.
    const ::godot::Ref<::godot::InputEvent> local = node_->make_input_local(event);
    const ::godot::Ref<::godot::InputEventScreenTouch> touch = local;
    const ::godot::Ref<::godot::InputEventMouseButton> mouse = local;
    ::godot::Vector2 at;
    if (touch.is_valid()) {
        if (!touch->is_pressed()) {
            return;
        }
        at = touch->get_position();
    } else if (mouse.is_valid() && mouse->get_button_index() == ::godot::MOUSE_BUTTON_LEFT) {
        if (!mouse->is_pressed()) {
            return;
        }
        at = mouse->get_position();
    } else {
        return;
    }
    node_->get_viewport()->set_input_as_handled();
    tap(at);
}

void VivariumView::tap(const ::godot::Vector2& at) {
    // Without a placement - no configuration, a surface without room - nothing was drawn to tap.
    if (!(placement_.scale > 0.0f)) {
        return;
    }
    if (placed(layout_.reset_rect).has_point(at)) {
        restart();
        return;
    }
    if (!configured_ || !failure_text_.is_empty()) {
        return;  // on the error panel only Reiniciar answers
    }
    if (placed(layout_.pause_rect).has_point(at)) {
        toggle_pause();
        return;
    }
    if (placed(layout_.water_rect).has_point(at)) {
        water();
        return;
    }
    for (size_t i = 0; i < layout_.patch_rects.size() && i < start_.patches.size(); ++i) {
        if (placed(layout_.patch_rects[i]).has_point(at)) {
            select(start_.patches[i].object_id);
            return;
        }
    }
}

void VivariumView::select(uint32_t object_id) {
    // A selection frame, no change to the simulation.
    selected_id_ = object_id;
    note("select " + measure(object_id) + " result=ok");
    node_->queue_redraw();
}

bool VivariumView::can_water() const {
    // No target, a dead patch, water already pending for its step, a paused or broken host:
    // the button is drawn disabled and a press only logs that it was.
    if (selected_id_ == GODOT_NO_OBJECT || water_pending_id_ != GODOT_NO_OBJECT || !ticking()) {
        return false;
    }
    const VivariumPatchView* patch = resources_.get_patch(selected_id_);
    return patch != nullptr && !patch->dead;
}

void VivariumView::water() {
    if (!can_water()) {
        note("water " + measure(selected_id_) + " result=disabled");
        return;
    }
    const VivariumPatchView* patch = resources_.get_patch(selected_id_);
    const double moisture_before = patch->moisture;
    const double age_before = patch->age_seconds;
    const HostStatus status = resources_.irrigate(selected_id_);
    note("water " + measure(selected_id_) + " amount=" +
         ::godot::String::num(start_.irrigate_amount, GODOT_LOG_DECIMALS) + " result=" +
         GodotHostResourceManager::status_name(status));
    if (status == ase::kernel::HostStatusOk) {
        water_pending_id_ = selected_id_;
        water_pending_from_ = moisture_before;
        water_pending_age_ = age_before;
    }
    node_->queue_redraw();
}

void VivariumView::toggle_pause() {
    user_paused_ = !user_paused_;
    if (!user_paused_) {
        drop_next_delta_ = true;
    } else {
        measurement_.pauses += measurement_.recording ? 1u : 0u;
    }
    note(::godot::String(user_paused_ ? "pause" : "resume") + " reason=user " +
         measure(GODOT_NO_OBJECT));
    node_->queue_redraw();
}

/**
 * PLACEMENT - the layout's three bands on the visible surface (godot_view_layout.hpp)
 */

ViewSurface VivariumView::surface() const {
    // The visible part of the canvas. With the expand aspect it starts at the origin and carries
    // the window's aspect ratio, in the logical units of the layout.
    const ::godot::Rect2 visible = node_->get_viewport_rect();
    ViewSurface surface;
    surface.width = visible.size.x;
    surface.height = visible.size.y;
    // Camera cut-out and gesture bar exist on a handset only. On the desktop the safe area is the
    // screen's, not the window's, and would push the layout off any window that is not maximised.
    if (!::godot::OS::get_singleton()->has_feature(GODOT_FEATURE_MOBILE)) {
        return surface;
    }
    // DisplayServer reports the safe area in window pixels; the viewport's final transform maps
    // the canvas onto them, so its inverse maps the safe area back onto the canvas.
    const ::godot::Rect2i safe = ::godot::DisplayServer::get_singleton()->get_display_safe_area();
    const ::godot::Transform2D to_canvas = node_->get_viewport()->get_final_transform().affine_inverse();
    const ::godot::Vector2 safe_start = to_canvas.xform(::godot::Vector2(safe.position));
    const ::godot::Vector2 safe_end = to_canvas.xform(::godot::Vector2(safe.position + safe.size));
    surface.inset_left = ase::math::max(0.0f, safe_start.x - visible.position.x);
    surface.inset_top = ase::math::max(0.0f, safe_start.y - visible.position.y);
    surface.inset_right = ase::math::max(0.0f, visible.get_end().x - safe_end.x);
    surface.inset_bottom = ase::math::max(0.0f, visible.get_end().y - safe_end.y);
    return surface;
}

::godot::Rect2 VivariumView::placed(const ::godot::Rect2& rect) const {
    // The same mapping use_band hands the canvas: scale, then the column and the band's offset.
    const float scale = placement_.scale;
    return ::godot::Rect2(placement_.offset_x + rect.position.x * scale,
                          band_offset(layout_.bands, placement_, rect.position.y) + rect.position.y * scale,
                          rect.size.x * scale, rect.size.y * scale);
}

void VivariumView::use_band(float layout_y) {
    const float scale = placement_.scale;
    node_->draw_set_transform(
        ::godot::Vector2(placement_.offset_x, band_offset(layout_.bands, placement_, layout_y)), 0.0f,
        ::godot::Vector2(scale, scale));
}

void VivariumView::use_canvas() {
    node_->draw_set_transform(::godot::Vector2(), 0.0f, ::godot::Vector2(1.0f, 1.0f));
}

/**
 * DRAWING - projections of the last snapshot, nothing else
 */

void VivariumView::draw() {
    if (in_editor()) {
        return;
    }
    drawn_surface_ = surface();
    const ::godot::Rect2 visible(0.0f, 0.0f, drawn_surface_.width, drawn_surface_.height);
    use_canvas();
    node_->draw_rect(visible, ::godot::Color::hex(GODOT_COLOR_BACKGROUND));
    if (!configured_) {
        // Without a configuration there is no layout to place: the error panel takes the surface.
        placement_ = ViewPlacement{};
        draw_failure(visible.grow(-GODOT_PATCH_INSET));
        return;
    }
    placement_ = place_view(layout_.bands, drawn_surface_);
    if (!(placement_.scale > 0.0f)) {
        return;  // a surface without room for the layout: the background stands alone
    }
    // The header colour runs over the whole width and up to the top edge - over a camera
    // cut-out too - and ends where the placed header band ends.
    node_->draw_rect(::godot::Rect2(0.0f, 0.0f, visible.size.x,
                                    placement_.top_y + layout_.header_height * placement_.scale),
                     ::godot::Color::hex(GODOT_COLOR_HEADER));
    const ::godot::Ref<::godot::Font> font = view_font();
    use_band(layout_.title_at.y);
    node_->draw_string(font, layout_.title_at, texts_.title, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       GODOT_FONT_TITLE, ::godot::Color::hex(GODOT_COLOR_TEXT));
    use_band(layout_.build_at.y);
    node_->draw_string(font, layout_.build_at, build_line_, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       build_font_size_, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));

    if (failure_text_.is_empty()) {
        for (size_t i = 0; i < layout_.patch_rects.size() && i < start_.patches.size(); ++i) {
            use_band(layout_.patch_rects[i].position.y);
            draw_patch(layout_.patch_rects[i], start_.patches[i].object_id);
        }
    } else {
        // The error panel stands where the patches stand, in their band.
        ::godot::Rect2 panel = layout_.patch_rects.front();
        for (const ::godot::Rect2& rect : layout_.patch_rects) {
            panel = panel.merge(rect);
        }
        use_band(panel.position.y);
        draw_failure(panel);
    }
    draw_info();
    use_band(layout_.water_rect.position.y);
    draw_button(layout_.water_rect, texts_.water, can_water());
    use_band(layout_.pause_rect.position.y);
    draw_button(layout_.pause_rect, user_paused_ ? texts_.resume : texts_.pause,
                resources_.running() && failure_text_.is_empty());
    use_band(layout_.reset_rect.position.y);
    draw_button(layout_.reset_rect, texts_.reset, true);
    use_canvas();
}

void VivariumView::draw_patch(const ::godot::Rect2& rect, uint32_t object_id) {
    node_->draw_rect(rect, ::godot::Color::hex(GODOT_COLOR_SOIL));
    const VivariumPatchView* patch = resources_.get_patch(object_id);
    const ::godot::Rect2 inner = rect.grow(-GODOT_PATCH_INSET);
    if (patch != nullptr) {
        const uint8_t stage = stage_of(object_id);
        // Biomass decides the covered AREA: the cover square grows with its square root.
        const double cover = share(patch->biomass, layout_.biomass_full);
        if (cover > 0.0) {
            const ::godot::Vector2 cover_size = inner.size * ase::math::sqrt(static_cast<float>(cover));
            const ::godot::Rect2 cover_rect(inner.get_center() - cover_size * GODOT_HALF, cover_size);
            node_->draw_rect(cover_rect, stage_color(stage));
            if (stage == GODOT_STAGE_DEAD) {
                const ::godot::Vector2 end = cover_rect.get_end();
                node_->draw_line(cover_rect.position, end, ::godot::Color::hex(GODOT_COLOR_SOIL),
                                 GODOT_BORDER_WIDTH);
                node_->draw_line(::godot::Vector2(cover_rect.position.x, end.y),
                                 ::godot::Vector2(end.x, cover_rect.position.y),
                                 ::godot::Color::hex(GODOT_COLOR_SOIL), GODOT_BORDER_WIDTH);
            }
        }
        if (stage == GODOT_STAGE_SEED) {
            // Sown soil shows its seeds while nothing covers it yet.
            const float step_x = inner.size.x / static_cast<float>(GODOT_SEED_DOTS + 1);
            const float step_y = inner.size.y / static_cast<float>(GODOT_SEED_DOTS + 1);
            for (int32_t row = 1; row <= GODOT_SEED_DOTS; ++row) {
                for (int32_t column = 1; column <= GODOT_SEED_DOTS; ++column) {
                    node_->draw_circle(::godot::Vector2(inner.position.x + step_x * static_cast<float>(column),
                                                        inner.position.y + step_y * static_cast<float>(row)),
                                       GODOT_SEED_DOT_RADIUS, ::godot::Color::hex(GODOT_COLOR_SEED));
                }
            }
        }
        // Moisture as a bar along the bottom edge.
        const ::godot::Rect2 track(inner.position.x, inner.get_end().y - GODOT_BAR_HEIGHT, inner.size.x,
                                   GODOT_BAR_HEIGHT);
        node_->draw_rect(track, ::godot::Color::hex(GODOT_COLOR_MOISTURE_TRACK));
        const double wet = share(patch->moisture, layout_.moisture_full);
        if (wet > 0.0) {
            node_->draw_rect(::godot::Rect2(track.position,
                                            ::godot::Vector2(track.size.x * static_cast<float>(wet), track.size.y)),
                             ::godot::Color::hex(GODOT_COLOR_MOISTURE));
        }
        node_->draw_string(view_font(), rect.position + ::godot::Vector2(GODOT_PATCH_INSET, GODOT_LABEL_OFFSET),
                           texts_.area + " " + ::godot::String::num_uint64(object_id) + texts_.separator +
                               stage_text(stage),
                           ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, GODOT_FONT_SMALL,
                           ::godot::Color::hex(GODOT_COLOR_TEXT));
    }
    node_->draw_rect(rect, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM), false, GODOT_BORDER_WIDTH);
    if (object_id == selected_id_) {
        node_->draw_rect(rect.grow(GODOT_FRAME_WIDTH * GODOT_HALF), ::godot::Color::hex(GODOT_COLOR_SELECTION),
                         false, GODOT_FRAME_WIDTH);
    }
}

void VivariumView::draw_button(const ::godot::Rect2& rect, const ::godot::String& label, bool enabled) {
    const ::godot::Ref<::godot::Font> font = view_font();
    node_->draw_rect(rect, ::godot::Color::hex(enabled ? GODOT_COLOR_BUTTON : GODOT_COLOR_BUTTON_OFF));
    node_->draw_rect(rect, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM), false, GODOT_BORDER_WIDTH);
    const float baseline = rect.get_center().y +
                           (font->get_ascent(GODOT_FONT_BODY) - font->get_descent(GODOT_FONT_BODY)) * GODOT_HALF;
    node_->draw_string(font, ::godot::Vector2(rect.position.x, baseline), label,
                       ::godot::HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, GODOT_FONT_BODY,
                       ::godot::Color::hex(enabled ? GODOT_COLOR_TEXT : GODOT_COLOR_TEXT_DIM));
}

void VivariumView::draw_info() {
    // Each line in the band of its baseline (load_bands holds the three info lines in one).
    const ::godot::Ref<::godot::Font> font = view_font();
    ::godot::Vector2 at = layout_.info_at;
    const VivariumPatchView* patch = resources_.get_patch(selected_id_);
    const ::godot::String selection =
        patch == nullptr ? texts_.selection_none
                         : texts_.area + " " + ::godot::String::num_uint64(selected_id_) + texts_.separator +
                               stage_text(stage_of(selected_id_));
    use_band(at.y);
    node_->draw_string(font, at, texts_.selection + ": " + selection, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       GODOT_FONT_BODY, ::godot::Color::hex(GODOT_COLOR_TEXT));
    at.y += layout_.info_line;
    if (patch != nullptr) {
        use_band(at.y);
        node_->draw_string(font, at,
                           texts_.biomass + " " + ::godot::String::num(patch->biomass, GODOT_TEXT_DECIMALS) +
                               texts_.separator + texts_.moisture + " " +
                               ::godot::String::num(patch->moisture, GODOT_TEXT_DECIMALS) + texts_.separator +
                               texts_.age + " " + ::godot::String::num(patch->age_seconds, GODOT_TEXT_DECIMALS) +
                               " s",
                           ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, GODOT_FONT_BODY,
                           ::godot::Color::hex(GODOT_COLOR_TEXT));
    }
    at.y += layout_.info_line;
    ::godot::String clock = texts_.simulation + " " +
                            ::godot::String::num(resources_.simulation_seconds(), GODOT_TEXT_DECIMALS) + " s" +
                            texts_.separator + ::godot::String::num_uint64(resources_.tick_count()) + " " +
                            texts_.ticks;
    if (user_paused_ || app_paused_ || focus_lost_) {
        clock += texts_.separator + texts_.paused;
    }
    use_band(at.y);
    node_->draw_string(font, at, clock, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, GODOT_FONT_BODY,
                       ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));
    use_band(layout_.note_at.y);
    node_->draw_string(font, layout_.note_at, texts_.restart_note, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       GODOT_FONT_SMALL, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));
}

void VivariumView::draw_failure(const ::godot::Rect2& panel) {
    // The error picture instead of an empty surface: where it broke and with which status. The
    // caller hands the panel in the coordinates of the transform it set.
    node_->draw_rect(panel, ::godot::Color::hex(GODOT_COLOR_ERROR));
    node_->draw_multiline_string(view_font(),
                                 panel.position + ::godot::Vector2(GODOT_PATCH_INSET, GODOT_LABEL_OFFSET),
                                 failure_text_, ::godot::HORIZONTAL_ALIGNMENT_LEFT,
                                 panel.size.x - GODOT_PATCH_INSET - GODOT_PATCH_INSET, GODOT_FONT_BODY,
                                 GODOT_ERROR_LINES, ::godot::Color::hex(GODOT_COLOR_TEXT));
}

}  // namespace ase::adp::godot

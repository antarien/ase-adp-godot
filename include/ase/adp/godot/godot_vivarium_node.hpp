#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW
 *
 * @file        godot_vivarium_node.hpp
 * @brief       VivariumView - instance data and behaviour of the Godot class AseVivariumView
 * @description The demo frontend of PLAN_ASE_VIVARIUM_PHASE_01_INTEG: four patches seen from
 *              above, the actions Regar, Pausar/Continuar and Reiniciar, a header with the build
 *              identity. One view owns exactly ONE GodotHostResourceManager and through it one
 *              KernelEmbeddedHost - no global App, no singleton. Godot draws and takes input; the
 *              simulation stays in the ASE registry of that host.
 *
 *              COMPOSED, NOT DERIVED: an object of the Godot class AseVivariumView is an engine
 *              Node2D. godot_registration.cpp registers that class through the GDExtension class
 *              interface and gives every such object one VivariumView as its instance data. The
 *              view draws and reads input through node_, the godot-cpp handle of exactly that
 *              object - no C++ type of this adapter derives from a Godot type.
 *
 *              EVERY HANDSET WITHOUT BARS (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3): the project's
 *              expand aspect hands the view the whole visible surface of the window, whatever its
 *              aspect ratio. The view fills it with its background and places the layout's three
 *              bands on it (godot_view_layout.hpp): the header at the top of the part no camera
 *              cut-out or gesture bar covers, the buttons at its bottom, the patches in its middle,
 *              the column centred, never magnified. A tap is matched against the rectangles as
 *              they were placed for the last drawn frame.
 *
 *              WHAT THE VIEW NEVER DOES: it computes no growth, writes no simulation value back
 *              and shows no optimistic number. Every drawn value is the copy the port reported
 *              after the last tick; the stage colour is the projection of the port's own flags.
 *
 *              THE EDITOR STAYS SIMULATION-FREE (V0): ready() returns when Engine::is_editor_hint,
 *              so importing or editing the scene starts no host and loads no plugin.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-06
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/**
 * Where things stand on the logical 720 x 1280 surface and how values are scaled for drawing -
 * read from res://config/vivarium_start.json, never written into the code. Every rectangle and
 * every text line lies inside ONE of the three bands; load_layout refuses a configuration where
 * one crosses a band edge, because the bands move apart on a taller surface.
 */
struct VivariumLayout {
    ::godot::Vector2 size;
    double           biomass_full = 0.0;   // cover drawn full at this biomass
    double           moisture_full = 0.0;  // moisture bar drawn full at this moisture
    ViewBands        bands;                // logical size and the two band edges (layout.bands)
    float            header_height = 0.0f;
    ::godot::Vector2 title_at;
    ::godot::Vector2 build_at;
    ::godot::Vector2 info_at;
    float            info_line = 0.0f;
    ::godot::Vector2 note_at;
    std::vector<::godot::Rect2> patch_rects;  // same order as VivariumStartConfig::patches
    ::godot::Rect2   water_rect;
    ::godot::Rect2   pause_rect;
    ::godot::Rect2   reset_rect;
};

/**
 * The explicit run modes of the start configuration (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3).
 * A mode runs only when the launch names its argument after `--` (Godot's user arguments; on
 * Android the intent extra command_line_params) - never on a normal start.
 */
struct VivariumModes {
    ::godot::String selftest_argument;       // runs the self-test table before the game host boots
    ::godot::String selftest_table;          // res:// path of the self-test table
    ::godot::String measure_argument;        // measures host and frame times after a warm-up
    double          measure_warmup_seconds = 0.0;  // foreground seconds before the recording starts
    double          measure_record_seconds = 0.0;  // foreground seconds recorded
};

/**
 * One measurement run (acceptance A10): foreground time counts down the warm-up, then the
 * recording, and every ticking frame in the recording stores its host time (tick + snapshot)
 * and its whole _process interval. The block is written ONCE, when the recording is complete -
 * nothing is logged per frame while it runs.
 */
struct VivariumMeasurement {
    bool                  active = false;       // requested at launch and not yet reported
    bool                  recording = false;    // warm-up over, samples are stored
    double                warmup_left = 0.0;    // foreground seconds of warm-up still to run
    double                record_left = 0.0;    // foreground seconds of recording still to run
    uint64_t              last_usec = 0;        // Time::get_ticks_usec of the previous ticking frame, 0 = none
    std::vector<uint32_t> host_usec;            // per recorded frame: KernelEmbeddedHost tick + snapshot
    std::vector<uint32_t> frame_usec;           // per recorded frame: the interval since the previous frame
    uint64_t              pss_start_kib = 0;    // PSS when the recording began
    bool                  pss_start_read = false;
    uint32_t              pauses = 0;           // pauses that fell into the recording
    uint32_t              boots_at_start = 0;   // host boots counted when the recording began
};

/** Every text the view draws, Brazilian Portuguese, from the same configuration. */
struct VivariumTexts {
    ::godot::String title;
    ::godot::String selection;
    ::godot::String selection_none;
    ::godot::String area;
    ::godot::String simulation;
    ::godot::String ticks;
    ::godot::String biomass;
    ::godot::String moisture;
    ::godot::String age;
    ::godot::String stage_seed;
    ::godot::String stage_sprout;
    ::godot::String stage_mature;
    ::godot::String stage_dead;
    ::godot::String paused;
    ::godot::String restart_note;
    ::godot::String error;
    ::godot::String water;
    ::godot::String pause;
    ::godot::String resume;
    ::godot::String reset;
    ::godot::String separator;   // between the parts of one line, e.g. "área 2 · Broto"
};

/**
 * @brief The Vivarium view: one Godot object, one embedded ASE host, four patches.
 *
 * Godot calls the six entry points below through the class binding in godot_registration.cpp:
 * ready (_ready), process (_process), draw (_draw), unhandled_input (_unhandled_input),
 * exit_tree (_exit_tree) and notification (every notification of the object).
 *
 * Lifecycle (PLAN 01.3): ready boots the host through the resource manager; process advances it
 * with dt capped and reads the snapshot; notification keeps the pause reasons apart (user,
 * application, focus) and drops the first delta after a resume; exit_tree and Reiniciar run the
 * same teardown. Input: one press per physical tap, ScreenTouch or the left mouse button, mapped
 * onto the canvas through make_input_local and matched against the placed layout.
 */
class VivariumView {
public:
    /** node: the godot-cpp handle of the engine object this view is the instance data of. */
    explicit VivariumView(::godot::Node2D* node);
    ~VivariumView();

    VivariumView(const VivariumView&) = delete;
    VivariumView& operator=(const VivariumView&) = delete;

    void ready();
    void process(double delta);
    void draw();
    void unhandled_input(const ::godot::Ref<::godot::InputEvent>& event);
    void exit_tree();
    void notification(int32_t what);

private:
    bool load_config();
    bool load_stage();
    bool load_host_config(const ::godot::Dictionary& root);
    bool load_layout(const ::godot::Dictionary& root);
    bool load_bands(const ::godot::Dictionary& layout);
    bool load_texts(const ::godot::Dictionary& root);
    bool load_modes(const ::godot::Dictionary& root);
    bool fail_config(const char* key);

    [[nodiscard]] bool launched_with(const ::godot::String& argument) const;
    void run_self_test();
    void measure_frame(uint64_t frame_start_usec, uint64_t host_usec);
    void report_measurement();

    void boot_host();
    void restart();
    void tap(const ::godot::Vector2& at);
    void select(uint32_t object_id);
    void water();
    void toggle_pause();
    void follow_stages();
    void follow_water();
    void drain_log();
    void note(const ::godot::String& line);

    [[nodiscard]] bool can_water() const;
    [[nodiscard]] bool ticking() const;
    [[nodiscard]] uint8_t stage_of(uint32_t object_id) const;
    [[nodiscard]] ::godot::String stage_text(uint8_t stage) const;
    [[nodiscard]] ::godot::String boot_line() const;
    [[nodiscard]] ::godot::String measure(uint32_t object_id) const;

    [[nodiscard]] ViewSurface surface() const;
    [[nodiscard]] ::godot::Rect2 placed(const ::godot::Rect2& rect) const;
    void use_band(float layout_y);
    void use_canvas();

    void draw_patch(const ::godot::Rect2& rect, uint32_t object_id);
    void draw_button(const ::godot::Rect2& rect, const ::godot::String& label, bool enabled);
    void draw_info();
    void draw_failure(const ::godot::Rect2& panel);

    ::godot::Node2D*         node_ = nullptr;      // the engine object; Godot owns it, it owns this view
    GodotHostResourceManager resources_;
    ::godot::String          stage_dir_;       // res://native/<os>-<arch>/<config> of the running build
    VivariumStartConfig      start_;           // its read_stage reads below stage_dir_
    VivariumLayout           layout_;
    VivariumTexts            texts_;
    VivariumModes            modes_;
    VivariumMeasurement      measurement_;
    ViewSurface              drawn_surface_;   // the surface the last frame was placed on
    ViewPlacement            placement_;       // the layout's placement on that surface
    std::vector<std::string> selftest_report_;  // written through the game host once it is up
    uint32_t                 selftest_errors_ = 0;
    bool                     selftest_ran_ = false;
    uint32_t                 host_boots_ = 0;   // boot_host calls of this view: restarts = boots - 1
    ::godot::Dictionary      identity_;        // build.identity of the staged bundle
    ::godot::String          build_line_;      // drawn under the title
    int32_t                  build_font_size_ = GODOT_FONT_SMALL;  // fitted to the width at boot
    ::godot::String          failure_text_;    // not empty: the error panel stands instead of the patches
    std::vector<std::string> log_lines_;       // drained once per frame and printed
    std::vector<uint8_t>     stages_;          // last drawn stage per configured patch
    uint32_t                 selected_id_ = GODOT_NO_OBJECT;
    uint32_t                 water_pending_id_ = GODOT_NO_OBJECT;  // watered, its step not yet seen
    double                   water_pending_from_ = 0.0;            // its moisture when watered
    double                   water_pending_age_ = 0.0;             // its age when watered
    bool                     configured_ = false;
    bool                     user_paused_ = false;
    bool                     app_paused_ = false;
    bool                     focus_lost_ = false;
    bool                     drop_next_delta_ = false;
};

}  // namespace ase::adp::godot

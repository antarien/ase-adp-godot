#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW
 *
 * @file        godot_vivarium_node.hpp
 * @brief       VivariumView - instance data and behaviour of the Godot class AseVivariumView
 * @description The frontend of PLAN_ASE_VIVARIUM_PHASE_01_INTEG: the four patches of ase-pl-flora
 *              seen from above - cover, stage and soil water as the modules compute them - the
 *              game clock, the actions Regar and Reiniciar, a header with the build identity. One
 *              view owns exactly ONE GodotHostResourceManager and through it one
 *              KernelEmbeddedHost - no global App, no singleton. Godot draws and takes input; the
 *              simulation stays in the ASE registry of that host.
 *
 *              COMPOSED, NOT DERIVED: an object of the Godot class AseVivariumView is an engine
 *              Node2D. godot_registration.cpp registers that class through the GDExtension class
 *              interface and gives every such object one VivariumView as its instance data. The
 *              view draws and reads input through node_, the godot-cpp handle of exactly that
 *              object - no C++ type of this adapter derives from a Godot type.
 *
 *              NO PAUSE (PLAN 01.3 point 4): there is no pause button and no stopped host. Every
 *              frame hands the host the real time since the previous one, measured on Godot's
 *              monotonic clock; a span in the background comes back as one long frame, and the
 *              host catches it up in ordinary ticks over the next frames. The world the player
 *              finds has gone on - like the server, which never stops.
 *
 *              EVERY HANDSET WITHOUT BARS (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3): the project's
 *              expand aspect hands the view the whole visible surface of the window, whatever its
 *              aspect ratio. The view fills it with its background and places the layout's three
 *              bands on it (godot_view_layout.hpp): the header at the top of the part no camera
 *              cut-out or gesture bar covers, the buttons at its bottom, the patches in its middle,
 *              the column centred, never magnified. A tap is matched against the rectangles as
 *              they were placed for the last drawn frame.
 *
 *              WHAT THE VIEW NEVER DOES: it computes nothing a module computes, writes no
 *              simulation value back and shows no optimistic number. Every drawn value is the value
 *              the ports reported after the last advance.
 *
 *              THE CARE VIEW IS A 3D SCENE (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2): once the
 *              host booted, the view builds one scene under its Node2D (godot_plant_scene.hpp) - camera
 *              on the near view of the "view" section, sun and sky from the sky port, the ground from
 *              the terrain port, the plant from the instance and organ ports and the organ templates of
 *              the template ports (usecase:101011 tessellate the organ templates), the line between fine and
 *              coarse cylinder by the order budget of the frame time - and the canvas draws its bands
 *              over it: the patches
 *              as frames with label, soil water and selection, the buttons below.
 *
 *              THE ASSET SET IS BOUND BY NAME (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2): with the
 *              configuration the view reads the asset list of its play stage and the manifest of the
 *              delivered set and binds every name (godot_asset_binding.hpp). Every boot writes one
 *              `asset missing <name>` per missing name after the boot line, which counts them; a role
 *              whose asset is missing is drawn in the error material. A missing manifest is not a
 *              broken configuration but an undelivered set: every name is missing, and the view runs.
 *
 *              THE ASSET SHOWCASE (PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7) runs on request only,
 *              launched with the argument of modes.assets, beside the one game host - never a second
 *              host. It shows every name of the bound list (godot_asset_showcase.hpp, ..._scene.hpp)
 *              and reads the binding of the configuration; it binds nothing again.
 *
 *              THE EDITOR STAYS SIMULATION-FREE (L0): ready() returns when Engine::is_editor_hint,
 *              so importing or editing the scene starts no host and loads no unit.
 *
 *              ONE CLASS, ITS BEHAVIOUR BY RESPONSIBILITY: the members below are defined in
 *              godot_vivarium_node.cpp (lifecycle, boot, frame, log), godot_vivarium_config.cpp,
 *              godot_vivarium_host_config.cpp, godot_vivarium_layout.cpp, godot_vivarium_assets.cpp (the
 *              configuration), godot_vivarium_input.cpp (taps and the selection),
 *              godot_vivarium_water.cpp (Regar), godot_vivarium_level.cpp (Nivelar),
 *              godot_vivarium_prune.cpp (the cutting mode), godot_vivarium_bed.cpp (the bed mode and the
 *              ground under the finger), godot_vivarium_flatten.cpp (the flatten mode),
 *              godot_vivarium_draw.cpp (placement and canvas), godot_vivarium_scene.cpp (the 3D scene per
 *              frame), godot_vivarium_debug.cpp (the debug buttons and the time lapse of this host's clock)
 *              and godot_vivarium_measure.cpp (self-test and measurement).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-05
 * @modified    2026-10-28
 * @version     00.00.06.00008
 */

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/godot_garden_return.hpp>
#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_order_budget.hpp>
#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * Where things stand on the logical 720 x 1280 surface - read from
 * res://config/vivarium_start.json, never written into the code. Every rectangle and every text
 * line lies inside ONE of the three bands; load_layout refuses a configuration where one crosses a
 * band edge, because the bands move apart on a taller surface.
 */
struct VivariumLayout {
    ::godot::Vector2 size;
    ViewBands        bands;                // logical size and the two band edges (layout.bands)
    float            header_height = 0.0f;
    ::godot::Vector2 title_at;
    ::godot::Vector2 build_at;
    ::godot::Vector2 info_at;
    float            info_line = 0.0f;
    ::godot::Vector2 note_at;
    std::vector<::godot::Rect2> place_rects;  // the rectangle of each place, index = place
    ::godot::Rect2   water_rect;
    ::godot::Rect2   level_rect;               // the field that levels the selected patch (03.8 point 2)
    ::godot::Rect2   prune_rect;               // the button of the cutting mode (03.10 step 9)
    ::godot::Rect2   bed_rect;                 // the button of the bed mode: two taps on the ground lay a brook's bed (03.8)
    ::godot::Rect2   flatten_rect;             // the button of the flatten mode: a tap on the ground levels a disc there (03.8)
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
    ::godot::String assets_argument;         // shows every asset of the list beside the game host (PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7)
    ::godot::String debug_argument;          // shows the debug buttons: the time lapse of this host's clock (godot_vivarium_debug.cpp)
};

/**
 * One button of the debug mode (section "debug" of the start configuration): it asks this host's game
 * clock for a time scale, and when it names a stage the lapse runs until the selected patch reached it.
 */
struct VivariumDebugButton {
    ::godot::Rect2  rect;
    ::godot::String label;
    double          scale = GODOT_PACE_NONE;    // the time scale the button asks; GODOT_PACE_NONE gives the default back
    uint8_t         stage = GODOT_STAGE_NONE;   // the stage the lapse runs to on the selected patch; NONE: the lapse only
};

/**
 * The debug mode: drawn and answered only when the launch names modes.debug.argument. The time lapse is
 * this host's alone - the request goes through the pace port to ase-time of this process and never
 * reaches a server clock; a restart boots a host at its default scale.
 *
 * A LAPSE TO A STAGE RUNS FASTER THAN THE FRAMES: on top of the frame's real time the view hands the host
 * lapse_s and lets it run lapse_steps more ticks in that frame (GodotHostResourceManager::advance). Every
 * tick stays an ordinary tick of the client's own clock - the client computes the years itself, nothing
 * is loaded and no server clock moves.
 */
struct VivariumDebug {
    bool            on = false;                 // launched with modes.debug.argument
    ::godot::Color  fill;                       // face of a debug button (debug.fill), unlike every game button
    ::godot::Color  text;                       // label and outline of a debug button (debug.text)
    double          keep_soil_rel = 0.0;        // during a lapse to a stage the patch is watered below this share of its capacity
    double          lapse_s = 0.0;              // real seconds a frame of a lapse to a stage hands the host beyond its own (debug.lapse_s)
    uint32_t        lapse_steps = 0u;           // ticks a frame of a lapse to a stage may run beyond catch_up_steps (debug.lapse_steps)
    std::vector<VivariumDebugButton> buttons;
    double          scale = GODOT_PACE_NONE;    // the scale asked of the running host, GODOT_PACE_NONE = its default
    uint8_t         target_stage = GODOT_STAGE_NONE;  // the stage the running lapse ends at, NONE = no end
    uint32_t        target_place = GODOT_NO_PLACE;    // the place whose patch the lapse watches
};

/**
 * One measurement run (acceptance A10): real time counts down the warm-up, then the recording,
 * and every frame in the recording stores its host time (advance + snapshots) and its whole
 * _process interval. The block is written ONCE, when the recording is complete - nothing is
 * logged per frame while it runs.
 */
struct VivariumMeasurement {
    bool                  active = false;       // requested at launch and not yet reported
    bool                  recording = false;    // warm-up over, samples are stored
    double                warmup_left = 0.0;    // seconds of warm-up still to run
    double                record_left = 0.0;    // seconds of recording still to run
    uint64_t              last_usec = 0;        // Time::get_ticks_usec of the previous frame, 0 = none
    std::vector<uint32_t> host_usec;            // per recorded frame: advance + snapshots
    std::vector<uint32_t> frame_usec;           // per recorded frame: the interval since the previous frame
    uint64_t              pss_start_kib = 0;    // PSS when the recording began
    bool                  pss_start_read = false;
    uint32_t              catch_ups = 0;        // returns from the background that fell into the recording
    uint32_t              boots_at_start = 0;   // host boots counted when the recording began
    CycleMeasure          cycles;               // the line of one game day (03.6), cursors since the recording began
    bool                  cycles_ready = false; // units, schedules and organ ports named, cursors set
    double                cycle_day = -1.0;     // the game day the current cycle began on, -1 = clock not yet read
};

/** Every text the view draws, Brazilian Portuguese, from the same configuration. */
struct VivariumTexts {
    ::godot::String title;
    ::godot::String selection;
    ::godot::String selection_none;
    ::godot::String area;
    ::godot::String simulation;
    ::godot::String ticks;
    ::godot::String day;
    ::godot::String hour;
    ::godot::String coverage;
    ::godot::String soil;
    ::godot::String stage_seed;
    ::godot::String stage_sprout;
    ::godot::String stage_grow;
    ::godot::String stage_mature;
    ::godot::String condition_wilted;
    ::godot::String condition_dead;
    ::godot::String unknown;
    ::godot::String restart_note;
    ::godot::String error;
    ::godot::String water;
    ::godot::String level;
    ::godot::String prune;
    ::godot::String bed;
    ::godot::String flatten;
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
 * Lifecycle (PLAN 01.3): ready boots the host through the resource manager; process hands it
 * the real time of every frame and reads the snapshots; notification logs the app's way into
 * and out of the background, nothing more; exit_tree and Reiniciar run the same teardown.
 * Input: one press per physical tap, ScreenTouch or the left mouse button, mapped onto the
 * canvas through make_input_local and matched against the placed layout.
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
    bool stage_data();
    bool load_host_config(const ::godot::Dictionary& root);
    bool load_patches(const ::godot::Dictionary& root);
    bool load_ground(const ::godot::Dictionary& root);
    bool load_brooks(const ::godot::Dictionary& root);
    bool load_layout(const ::godot::Dictionary& root);
    bool load_places(const ::godot::Dictionary& root);
    bool load_bands(const ::godot::Dictionary& layout);
    bool load_texts(const ::godot::Dictionary& root);
    bool load_modes(const ::godot::Dictionary& root);
    bool load_debug(const ::godot::Dictionary& root);
    bool load_view(const ::godot::Dictionary& root);
    bool load_assets(const ::godot::Dictionary& root);
    bool load_asset_roles(const ::godot::Dictionary& stage, const char* section);
    void load_asset_manifest(const ::godot::String& path, std::vector<VivariumAssetEntry>& out);
    bool fail_config(const char* key);

    [[nodiscard]] bool launched_with(const ::godot::String& argument) const;
    void run_self_test();
    void measure_frame(uint64_t frame_start_usec, uint64_t host_usec);
    void begin_cycles();   // name units, schedules and organ ports for the cycle line, set its cursors
    void measure_cycle();  // a new game day: the line of the day that ended (03.6)
    void report_measurement();

    void boot_host();
    void restart();
    void tap(const ::godot::Vector2& at);
    void select(uint32_t place);
    void zoom(int32_t steps);              // the mouse wheel or a pinch: the near view towards the birch or away (patch_focus_zoom)
    void water();
    void level();                          // the field Nivelar: level the ground of the selected patch (03.8 point 2)
    void toggle_pruning();                // the button Podar: the cutting mode on or off (03.10 step 9)
    void prune(const ::godot::Vector2& at);  // in the cutting mode: cut the branch of the internode under the tap
    void toggle_bedding();                // the button Cauce: the bed mode on or off (03.8)
    void bed(const ::godot::Vector2& at);    // in the bed mode: the first tap marks where the bed starts, the second lays it
    void toggle_flattening();             // the button Aplanar: the flatten mode on or off (03.8)
    void flatten(const ::godot::Vector2& at);  // in the flatten mode: level a disc of the ground under the tap
    [[nodiscard]] bool debug_tap(const ::godot::Vector2& at);  // a tap on a debug button, answered; false: none was hit
    void ask_pace(const VivariumDebugButton& button);  // the time lapse of the button, its end at a stage
    void follow_debug();                  // the lapse to a stage: keep the soil watered, end it at the stage
    void announce_patches();
    void follow_stages();
    void follow_water();
    void follow_scene(double elapsed_s);
    void follow_catch_up(double elapsed_s, double now_s);  // the return to the garden (godot_garden_return.hpp)
    void drain_log();
    void note(const ::godot::String& line);

    [[nodiscard]] bool can_water() const;
    [[nodiscard]] bool can_level() const;
    [[nodiscard]] bool can_prune() const;
    // the internode under a tap, GODOT_NO_OBJECT for none; `candidates` counts the internodes of the focused
    // plant in front of the camera, `nearest_px` the distance of the closest of them from the tap
    [[nodiscard]] uint32_t pick_segment(const ::godot::Vector2& at, uint32_t& candidates, float& nearest_px) const;
    [[nodiscard]] bool can_bed() const;
    [[nodiscard]] bool can_flatten() const;
    // the terrain sample under a tap: its world place east and north; false when none lies within pick_px
    [[nodiscard]] bool pick_ground(const ::godot::Vector2& at, float& x, float& z, float& nearest_px) const;
    [[nodiscard]] ::godot::String stage_text(uint8_t stage) const;
    [[nodiscard]] ::godot::String patch_text(const VivariumPatchView& patch) const;
    [[nodiscard]] ::godot::String boot_line() const;
    [[nodiscard]] ::godot::String measure(uint32_t place) const;
    [[nodiscard]] uint32_t ground_color(const VivariumPatchView* patch) const;
    [[nodiscard]] ::godot::Ref<::godot::Material> ground_material() const;
    void fill_ground();

    [[nodiscard]] ViewSurface surface() const;
    [[nodiscard]] ::godot::Rect2 placed(const ::godot::Rect2& rect) const;
    void use_band(float layout_y);
    void use_canvas();

    void draw_patch(const ::godot::Rect2& rect, uint32_t place);
    void draw_button(const ::godot::Rect2& rect, const ::godot::String& label, bool enabled);
    void draw_info();
    void draw_debug();
    void draw_failure(const ::godot::Rect2& panel);

    ::godot::Node2D*         node_ = nullptr;      // the engine object; Godot owns it, it owns this view
    GodotHostResourceManager resources_;
    ::godot::String          stage_dir_;       // res://native/<os>-<arch>/<config> of the running build
    VivariumStartConfig      start_;           // its read_stage reads below stage_dir_
    VivariumLayout           layout_;
    VivariumTexts            texts_;
    VivariumModes            modes_;
    VivariumDebug            debug_;           // the debug buttons and the running time lapse (modes.debug)
    VivariumMeasurement      measurement_;
    ViewSurface              drawn_surface_;   // the surface the last frame was placed on
    ViewPlacement            placement_;       // the layout's placement on that surface
    uint32_t                 play_stage_ = 0;  // play stage whose asset names the view binds (play_stage)
    ::godot::String          asset_root_;      // res:// directory of the manifest's files
    std::unordered_map<std::string, std::string> asset_roles_;  // role the view draws with → asset name of that stage
    std::vector<std::string> asset_names_;     // the stage's asset names in list order, materials first
    ::godot::String          asset_manifest_;  // the manifest of the asset set: read, broken or absent
    VivariumAssetBinding     assets_;          // bound files and missing names, bound at configuration
    std::vector<std::string> asset_materials_;  // the stage's material names; every other name of asset_names_ is an organ template
    ::godot::Node3D*         showcase_ = nullptr;  // root of the asset showcase (modes.assets), nullptr without its launch argument
    ::godot::Node3D*         scene_ = nullptr;     // root of the 3D scene of the care view, built once the host booted (03.1)
    PlantSceneCamera         scene_camera_;        // the near view (view.camera_*)
    float                    zoom_ = GODOT_ZOOM_START;  // the near view's zoom, moved by the mouse wheel or a pinch
    PlantSceneLook           scene_look_;         // materials and organ templates of the scene, bound once at the start (03.2)
    OrderBudget              order_budget_;        // the line between fine and coarse cylinder by frame time (view, 03.6 V8)
    std::unordered_map<uint32_t, PlantOrigin> scene_origins_;  // stem base per patch object, last placed
    uint64_t                 scene_organs_ = 0;    // organ generation the plant was last filled from
    uint64_t                 scene_terrain_ = 0;   // terrain generation the ground was last built from
    int64_t                  scene_hour_ = -1;     // whole game hour of the day of the last `scene hour` line, -1 before the clock
    uint64_t                 scene_templates_ = 0; // template generation the plant was last filled from (usecase:101011 tessellate the organ templates)
    uint32_t                 scene_focus_ = GODOT_NO_PLACE;  // the place the camera last looked at
    float                    pick_px_ = 0.0f;      // logical pixels around a tap that hit an internode's middle (view.pick_px)
    bool                     pruning_ = false;     // the cutting mode is on: a tap in the scene cuts, it selects no place
    bool                     bedding_ = false;     // the bed mode is on: a tap in the scene marks a bed's end, it selects no place
    bool                     bed_started_ = false; // the first end of the bed is marked
    float                    bed_x_ = 0.0f;        // the first end, world metres east
    float                    bed_z_ = 0.0f;        // the first end, world metres north
    std::vector<VivariumBrookSpec> player_beds_;   // the beds the player laid in this host, marked on the ground like the configured ones
    bool                     flattening_ = false;  // the flatten mode is on: a tap in the scene levels a disc, it selects no place
    float                    flatten_m_ = 0.0f;    // radius of the disc one tap levels, metres (view.flatten_m)
    std::vector<PlantGroundDisc> player_flats_;    // the discs the player levelled in this host; their height is the ground's at the centre
    std::vector<std::string> selftest_report_;  // written through the game host once it is up
    uint32_t                 selftest_errors_ = 0;
    bool                     selftest_ran_ = false;
    uint32_t                 host_boots_ = 0;   // boot_host calls of this view: restarts = boots - 1
    ::godot::Dictionary      identity_;        // build.identity of the staged bundle
    ::godot::String          build_line_;      // drawn under the title
    int32_t                  build_font_size_ = GODOT_FONT_SMALL;  // fitted to the width at boot
    ::godot::String          failure_text_;    // not empty: the error panel stands instead of the patches
    std::vector<std::string> log_lines_;       // drained once per frame and printed
    std::vector<uint8_t>     stages_;          // last logged stage per place
    std::vector<uint8_t>     conditions_;      // last logged condition per place
    bool                     patches_announced_ = false;  // every place's record stood once and got its line
    uint32_t                 selected_place_ = GODOT_NO_PLACE;
    uint32_t                 water_pending_place_ = GODOT_NO_PLACE;  // watered, its arrival not yet seen
    double                   water_pending_from_ = 0.0;              // its soil water when watered, mm
    uint64_t                 water_pending_steps_ = 0;               // Dissemination runs when watered
    int64_t                  last_frame_ns_ = 0;                     // the previous frame on the boot clock, 0 = none yet
    GardenReturn             garden_return_;                         // a return from the background and since when it is caught up
    bool                     configured_ = false;
};

}  // namespace ase::adp::godot

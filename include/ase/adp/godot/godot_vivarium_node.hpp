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
 *              frame), godot_vivarium_debug.cpp (the debug buttons, the time lapse and the jump of this
 *              host's clock), godot_vivarium_clock.cpp (the hour length, the rate and the feed of a jump)
 *              godot_vivarium_measure.cpp (self-test and measurement), godot_vivarium_hud.cpp (the HUD's
 *              configuration, its frame from the ports and its drawing) and godot_vivarium_hud_input.cpp (the
 *              touches through the HUD, its clock and the requests it sends over the ports). The records of the
 *              debug mode stand in godot_vivarium_debug.hpp, the records of the measurement run and the ground
 *              watch in godot_vivarium_watch.hpp.
 *
 *              ITS STATE BY RESPONSIBILITY, EACH IN ITS OWN HEADER: the view holds three records as members,
 *              scene_state_ (godot_vivarium_scene_state.hpp: the asset set, the 3D scene and its ground modes),
 *              figure_state_ (godot_vivarium_figure_state.hpp: the figures and the controls that lead one) and
 *              hud_state_ (godot_vivarium_hud_state.hpp: the HUD, its frame, clock, frame rate and bridges). A
 *              new member of one of them is written into its record, never into this header; this header keeps
 *              the lifecycle, the configuration, the host, the start screen and the log.
 *
 *              THE CARE VIEW SHOWS THE HUD (PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5): once the world stands
 *              initialised and no error panel stands, the canvas over the scene is VivariumHud
 *              (godot_hud_compose.hpp) and every touch goes through it. Its frame maps the mockup's units
 *              (hud_frame_width wide) onto the middle band of the placed layout; UiAnchor projects the scene's
 *              world places through the inverse of that frame. The error panel with Reiniciar stays the old
 *              canvas: a HUD that cannot draw names why there.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-05
 * @modified    2026-11-25
 * @version     00.00.28.00028 [refine]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_garden_return.hpp>
#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/godot_vivarium_debug.hpp>
#include <ase/adp/godot/godot_vivarium_figure_state.hpp>
#include <ase/adp/godot/godot_vivarium_hud_state.hpp>
#include <ase/adp/godot/godot_vivarium_scene_state.hpp>
#include <ase/adp/godot/godot_vivarium_start_choice.hpp>
#include <ase/adp/godot/godot_vivarium_start_hold.hpp>
#include <ase/adp/godot/godot_vivarium_start_mode.hpp>
#include <ase/adp/godot/godot_vivarium_view_config.hpp>
#include <ase/adp/godot/godot_vivarium_camp.hpp>
#include <ase/adp/godot/godot_vivarium_water_pending.hpp>
#include <ase/adp/godot/godot_vivarium_watch.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

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
    bool load_figure_config(const ::godot::Dictionary& root);  // the figure's ports, operations, seat and place (godot_vivarium_figure.cpp)
    bool load_layout(const ::godot::Dictionary& root);
    bool load_places(const ::godot::Dictionary& root);
    bool load_bands(const ::godot::Dictionary& layout);
    bool load_texts(const ::godot::Dictionary& root);
    bool load_modes(const ::godot::Dictionary& root);
    bool load_debug(const ::godot::Dictionary& root);
    bool load_debug_date(const ::godot::Dictionary& debug);  // the free date entry (debug.date)
    bool load_debug_keys(const ::godot::Dictionary& date);   // the typed entry of debug.date: digits, base, keys, clear
    bool load_view(const ::godot::Dictionary& root);
    bool load_assets(const ::godot::Dictionary& root);
    bool load_asset_roles(const ::godot::Dictionary& stage, const char* section);
    void load_asset_manifest(const ::godot::String& path, std::vector<VivariumAssetEntry>& out);
    bool fail_config(const char* key);

    [[nodiscard]] bool launched_with(const ::godot::String& argument) const;
    void run_self_test();
    void measure_frame(uint64_t frame_start_usec, uint64_t host_usec);
    void watch_ground_act(const char* op, uint64_t act_usec);  // a ground act begins the watch of its frames (P0690)
    void watch_ground_frame(uint64_t frame_start_usec);        // one frame of that watch, the line when it ends
    void watch_ground_host(uint64_t host_usec);                // the host advance of a watched frame
    void watch_ground_phases(uint64_t follow_usec, uint64_t scene_usec, uint64_t hud_usec,
                             uint64_t process_usec);           // the view's shares of a watched frame
    void report_ground_watch();                                // the line of the running watch
    void lay_cycles(CycleMeasure& cycles) const;  // name every unit, schedule and organ port of a cycle line
    void begin_cycles();   // name units, schedules and organ ports for the cycle line, set its cursors
    void measure_cycle();  // a new game day: the line of the day that ended (03.6)
    void report_measurement();

    void boot_host(bool new_seeds);        // a host with the whole set; new_seeds: the next sowing, else the seeds of this run
    void begin_host(uint8_t mode);         // the start screen's choice: start_mode_ takes `mode`, then the first boot
    bool start_input(const ::godot::Ref<::godot::InputEvent>& local);  // a tap on the start screen's choice; true when taken
    void restart();
    void rewind_host(const VivariumJump& jump);  // ase-time judged a date behind its clock: the host anew with this run's seeds and pace, the date sent again once its clock stands
    void follow_rewind(const VivariumClockView& clock);  // a rewound host's clock stands: the date sent again with a new number, ase-time judges it
    void tap(const ::godot::Vector2& at);
    void select(uint32_t place);
    void zoom(int32_t steps);              // the mouse wheel or a pinch: the near view towards the birch or away (patch_focus_zoom)
    bool water(double amount_mm, bool emptied);  // pour amount_mm on the selected patch (usecase:100367); emptied: the HUD's can is empty after it; true when the port took it
    bool water_from_can(double litres, bool emptied);  // the HUD's can lets litres go on the selected patch through the drain port of the figure led; ase-equipment lays the column (N0517); true when the port took it
    bool level();                         // the field Nivelar: level the ground of the selected patch (03.8 point 2); true when the port took it
    void toggle_pruning();                // the button Podar: the cutting mode on or off (03.10 step 9)
    void prune(const ::godot::Vector2& at);  // in the cutting mode: cut the branch of the internode under the tap
    void toggle_bedding();                // the button Cauce: the bed mode on or off (03.8)
    bool bed(const ::godot::Vector2& at);    // in the bed mode: the first tap marks where the bed starts, the second lays it; true once laid
    void toggle_flattening();             // the button Aplanar: the flatten mode on or off (03.8)
    bool flatten(const ::godot::Vector2& at, float radius_m);  // level a disc of radius_m of the ground under the tap (the flatten mode, a HUD verb); true when the port took it
    bool lower(float x, float z, float radius_m, float depth_m);  // dig a disc of radius_m depth_m down at the ground point x, z (the verb cavar); true when the port took it
    bool raise(float x, float z, float radius_m, float height_m);  // heap a disc of radius_m height_m up at the ground point x, z (the verb amontoar); true when the port took it
    bool loosen(float x, float z, float half_m, float depth_m);  // loosen a block of half side half_m depth_m deep at the ground point x, z (the verbs soltar, furar; B0649); true when the port took it
    [[nodiscard]] bool debug_tap(const ::godot::Vector2& at);  // a tap on a debug button, answered; false: none was hit
    void ask_pace(const VivariumDebugButton& button);  // the time lapse of the button: the factor of the base pace
    void ask_jump(const VivariumDebugJump& jump, uint32_t index);  // the jump of the button to a stage or an hour
    [[nodiscard]] bool date_tap(const ::godot::Vector2& at);  // a tap on the free date entry, answered; false: none of it was hit
    void step_date(const VivariumDebugDateStep& step);  // the free date entry: one part moved by the step
    void pick_date(uint8_t part);         // the typed entry: a tap on a field picks its part for the keys
    void type_date(const VivariumDebugDateKey* key);  // the typed entry: the key's digit onto the picked part; nullptr: clear empties it
    void ask_date();                      // the free date entry: the jump to the date it holds, ahead or back
    [[nodiscard]] bool take_seq(VivariumJump& jump);  // the next request number of this run onto the jump; false: the numbers a float slot holds are spent
    [[nodiscard]] ase::kernel::HostStatus send_date(const VivariumClockView& clock);  // the one write of a date jump: its target date and number through the date port
    [[nodiscard]] ase::kernel::HostStatus send_seek(VivariumJump& jump, double game_hour, double ahead_h, double day_hour);  // the one write of every other jump: its target in the form its source names, with its number, through the seek port
    [[nodiscard]] bool seek_jump(VivariumJump& next, double game_hour, double ahead_h, double day_hour, const ::godot::String& words);  // a jump through the seek port: one write, the start screen raised
    void raise_jump_screen();             // a jump begins like a start of the app: the start screen stands again, the clock is measured anew
    void ask_moonrise(VivariumJump& next);  // the jump to ase-ephemeris' forecast of the next moonrise, one write through the seek port
    void follow_moonrise(const ::godot::String& at_words);  // the landing of a moonrise jump: the moons in the camera's frustum noted
    void ask_shape(const VivariumDebugRelief& relief);  // the relief of the whole ground the button asks (usecase:6018)
    void follow_debug();                  // the running jump: ended on ase-time's verdict on its request, settled at the run pace
    void follow_hour_length();            // ase-time's hour and day constants from the clock record, and the clock's rate
    void follow_initialisation();         // lifts the start screen once the world stands complete, compiled and rendered
    [[nodiscard]] uint32_t scene_parts_built() const;  // the parts of the scene that stand, of GODOT_INIT_SCENE_PARTS
    [[nodiscard]] double initialisation_progress() const;  // the share of the initialisation done, 0..1
    void draw_start_screen(const ::godot::Rect2& visible);  // the opaque start screen over the whole surface
    void announce_window();               // the client window the view opened in, one log line (usecase:101019 start the client window)
    void announce_patches();
    void follow_stages();
    void follow_water();
    void follow_scene(double elapsed_s);
    void follow_figure(double elapsed_s);  // the figures in the scene, the player camera and the keys that lead the seat's figure (07.4 to 07.9)
    void follow_camp();                    // the camp fire: laid once through ase-pl-ingle, its bodies drawn, wood laid for a keeper (T.3, T.7)
    bool load_figure_hud(const UiTheme& theme);  // the hud_figure_* measures, colours and words of the phone's controls (07.9)
    [[nodiscard]] bool figure_input(const ::godot::Ref<::godot::InputEvent>& local);  // the stick, the orbit drag and cam.mode; true: taken
    void draw_figure_hud(UiPen& pen);      // the stick and cam.mode over the HUD (07.9)
    void follow_return(double elapsed_s, double now_s);  // the return to the garden (godot_garden_return.hpp)
    void follow_frame_rate(double elapsed_s);  // the frames per second in the status line's gap, a line on every edge of the floor (godot_vivarium_frame_rate.cpp)
    void drain_log();
    void note(const ::godot::String& line);

    [[nodiscard]] bool can_water() const;
    [[nodiscard]] bool can_level() const;
    [[nodiscard]] bool can_prune() const;
    // the internode under a tap, GODOT_NO_OBJECT for none; `candidates` counts the internodes of the focused
    // plant in front of the camera, `nearest_px` the distance of the closest of them from the tap
    [[nodiscard]] uint32_t pick_segment(const ::godot::Vector2& at, uint32_t& candidates, float& nearest_px) const;
    void plant_segments(uint32_t patch_object, std::vector<VivariumInstanceView>& out) const;  // the internode instances of the plant of that patch
    [[nodiscard]] bool can_bed() const;
    [[nodiscard]] bool can_flatten() const;
    // the terrain sample under a tap: its world place east and north; false when none lies within pick_px
    [[nodiscard]] bool pick_ground(const ::godot::Vector2& at, float& x, float& z, float& nearest_px) const;
    [[nodiscard]] ::godot::String stage_text(const VivariumPatchView& patch) const;
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
    void draw_debug_date(float text_lift);  // the free date entry: its fields with the shown date, its steps, go and now
    [[nodiscard]] bool shown_date(double (&shown)[GODOT_DATE_PARTS]) const;  // the date the entry shows: held or the clock record's; false while neither is known
    void draw_failure(const ::godot::Rect2& panel);

    bool load_hud(const ::godot::Dictionary& root);  // the HUD's theme and blocks, the frame's width and the verbs (hud)
    bool load_hud_bridge(const UiTheme& theme);  // the mockup's values the bridge writes while a port is silent (hud_bridge_* of SHA)
    bool load_hud_bridge_weather(const UiTheme& theme);  // the mockup's weather and forecast of hud_bridge_weather_* and hud_bridge_forecast_<n>_* of SHA, the stage's kind from modes.weather (N0449)
    bool load_hud_bridge_survival(const UiTheme& theme);  // the mockup's body, camp fire, rest and air of hud_bridge_survival_* of SHA (N0530, B0657)
    void read_hud_still();  // the still mode: modes.still on the launch or the platform's reduced-motion setting, logged on change (row T09)
    void bridge_clock(VivariumClockView& clock);  // every clock group the ports leave empty from hud_bridge_clock_* of SHA, its gap logged
    void bridge_weather(SkyBandFrame& band, ForecastStripFrame& forecast, WeatherVeilFrame& veil, HandSlotFrame& hand,
                        SkyOvercastFrame& sky);  // the weather of hud_bridge_weather_* of SHA while the weather port is silent, its gap logged; sky takes the kind only then, since the sky port carries no tint of it
    void bridge_patch(FocusClusterFrame& focus, const VivariumPatchView* patch, const VivariumClockView& clock,
                      YearRingFrame& year);  // growth, forecast days, next stage, loose soil and sap of hud_bridge_patch_* of SHA while no port carries them, each gap logged
    void bridge_signs(HudGameState& game, SignMarkerFrame& signs, const VivariumPatchView* patch,
                      const VivariumClockView& clock, const SkyBandFrame& band,
                      const HandSlotFrame& hand);  // the signs the mockup's signs() reports over this frame's values while no port reports one (N0461, N0449), its gap logged
    void bridge_stow(DragLayerFrame& drag);  // the verdict the mockup's fits() gives the drag's place while flora.stow.v1 judges none (N0452), its gap logged
    void drag_ground(DragLayerFrame& drag) const;  // where the finger meets the ground (pick_ground, hud_ground_): at every frame AND at the move and the release of a drag, as the mockup's drop reads toUZ at the release point (B0275)
    void bridge_pace(HourglassFrame& glass);  // the factor the clock runs at while flora.pace.v1 reports none: the last asked, else hud_bridge_pace_factor (N0459), its gap logged
    void bridge_skill(HudGameState& game);  // the level while flora.skill.v1 files none: the start level of SHA, hud.perception.start (N0453), its gap logged
    void bridge_trail(TrailViewFrame& trail, YearRingFrame& year);  // the past events and the ring's dots of hud_bridge_trail_* of SHA while flora.trail.v1 files none (N0450), its gap logged per record
    void bridge_arc(SkyArcFrame& arc, const VivariumClockView& clock);  // the sky arc from flora.arc.v1, flora.almanac.v1, flora.event_code.v1, hud_bridge_arc_* of SHA while a port files none (N0448), its gap logged per port
    void bridge_body(SurvivalBodyFrame& body);  // the figure's body for the rail CORPO of hud_bridge_survival_* of SHA while no host port reports its MTB_* (N0530), its gap logged
    void bridge_fire(SurvivalFireFrame& fire);  // the camp fire for the card FOGUEIRA from ingle.fire.v1, hud_bridge_survival_fire_* of SHA while no cell stands (B0657, B0662), its gap logged
    void bridge_air(SurvivalAirFrame& air);  // the owner's grade of the air for the air mark from ingle.fire.v1 breath_state, hud_bridge_survival_air_toxic of SHA while no graded record stands (N0530), its gap logged
    void bridge_food(SurvivalSpoilFrame& food);  // the spoil of the bag's food for the spoil ring of hud_bridge_survival_food_* of SHA while no port carries it (N0530), its gap logged
    void bridge_rest(SurvivalRestFrame& rest);  // the figure's rest for the rest card of hud_bridge_survival_rest_* of SHA while no record of survival.rest.v1 carries is_resting and sleeping (N0530, N0555), its gap logged
    [[nodiscard]] bool hud_live() const;  // the HUD takes the canvas and the touches: configured, initialised, no error panel
    void place_hud();                     // the HUD's frame on the placed layout, the camera of the scene and the ground under it
    [[nodiscard]] bool fill_hud(std::string& failed);  // every block's frame from the ports of this frame
    void fill_hud_world(const VivariumPatchView* patch, const PlantOrigin* origin, const VivariumPatchSpec& spec,
                        double opened);  // the plant, the ground, the brush and the things in the world in the frame
    void settle_bed_choice();             // a new HUD verb or mode drops the bed's start, read at every frame AND before a ground verb acts
    void fill_hud_debug(const VivariumPatchView* patch);  // the debug layer's frame: identity, clock, patch and the date entry
    void draw_hud();                      // the HUD over the scene; a failure raises the error panel
    void note_hud_places();               // the places a tap reaches the debug layer through, logged on each change (`debug place ...`)
    void hud_input(const ::godot::Ref<::godot::InputEvent>& local);  // a touch, a drag or a release through the HUD
    void tick_hud(double elapsed_s);      // the HUD's clock runs on: long presses, holds and the requests they fire
    void send_hud(const std::vector<VivariumHudRequest>& requests);  // each request of the HUD through its port, or its gap named
    void send_hud_verb(const VivariumHudRequest& request);  // a ground verb under the brush through the port of its operation
    void send_hud_debug(const DebugLayerHit& part);         // a part of the HUD's debug layer: pace, jump, date step, go, now, relief
    void open_gap(const char* gap, const char* act);        // a value or request no port carries yet (carrier task N0448): logged once per id on the edge of the bridge, the request (act) named in the line; never shown
    void close_gap(const char* gap, const char* act);       // the port now carries it: the closing edge logged once, the gap armed again for the next opening
    void fail_hud(const std::string& failed);               // the HUD stopped: the error panel names why

    ::godot::Node2D*         node_ = nullptr;      // the engine object; Godot owns it, it owns this view
    GodotHostResourceManager resources_;
    ::godot::String          stage_dir_;       // res://native/<os>-<arch>/<config> of the running build
    VivariumStartConfig      start_;           // its read_stage reads below stage_dir_
    VivariumLayout           layout_;
    VivariumTexts            texts_;
    VivariumModes            modes_;
    VivariumDebug            debug_;           // the debug buttons, the asked factor and the running jump (modes.debug)
    VivariumHourLength       hour_length_;     // ase-time's clock constants and the measured rate of the clock, read by the jump
    VivariumMeasurement      measurement_;
    VivariumGroundWatch      ground_watch_;    // the frames after the last ground act (P0690)
    VivariumSurvivalWatch    survival_watch_;  // the survival states the HUD last logged (U0245)
    ViewSurface              drawn_surface_;   // the surface the last frame was placed on
    ViewPlacement            placement_;       // the layout's placement on that surface
    VivariumSceneState       scene_state_;     // the bound asset set and the 3D scene with its ground modes (godot_vivarium_scene_state.hpp)
    VivariumFigureState      figure_state_;    // the figures and the controls that lead one (godot_vivarium_figure_state.hpp)
    VivariumHudState         hud_state_;       // the HUD, its frame, clock, frame rate and bridges (godot_vivarium_hud_state.hpp)
    std::vector<std::string> selftest_report_;  // written through the game host once it is up
    uint32_t                 selftest_errors_ = 0;
    bool                     selftest_ran_ = false;
    uint32_t                 host_boots_ = 0;   // boot_host calls of this view: restarts = boots - 1
    uint8_t                  start_mode_ = GODOT_START_MODE_NONE;  // the start screen's choice (godot_vivarium_start_mode.hpp)
    VivariumStartChoice      start_choice_;     // the switches of the start screen, bound with the HUD's theme
    ::godot::Transform2D     start_frame_;      // mockup units onto the canvas while the start screen stands
    uint32_t                 run_sowing_ = 0;   // the sowing the running host's seeds came from (sown_seed); a rewind keeps it
    ::godot::Dictionary      identity_;        // build.identity of the staged bundle
    ::godot::String          build_line_;      // drawn under the title
    int32_t                  build_font_size_ = GODOT_FONT_SMALL;  // fitted to the width at boot
    ::godot::String          failure_text_;    // not empty: the error panel stands instead of the patches
    std::vector<std::string> log_lines_;       // drained once per frame and printed
    std::vector<uint8_t>     stages_;          // last logged stage per place
    std::vector<double>      conditions_;      // last logged raw condition code of ase-plant per place, NaN while none came
    bool                     patches_announced_ = false;  // every place's record stood once and got its line
    bool                     initialised_ = false;        // the world stands complete and rendered: the start screen lifted
    VivariumStartHold        start_hold_;                 // quiet frames, pipelines and the step that holds the start screen (godot_vivarium_start_hold.hpp)
    uint32_t                 selected_place_ = GODOT_NO_PLACE;
    VivariumWaterPending     water_pending_;                         // the pour whose arrival is not yet seen (godot_vivarium_water_pending.hpp)
    VivariumCampState        camp_;                                  // the camp fire laid through ase-pl-ingle and its bodies (T.3, T.7)
    int64_t                  last_frame_ns_ = 0;                     // the previous frame on the boot clock, 0 = none yet
    GardenReturn             garden_return_;                         // a return from the background and since when it is pending
    bool                     configured_ = false;
};

}  // namespace ase::adp::godot

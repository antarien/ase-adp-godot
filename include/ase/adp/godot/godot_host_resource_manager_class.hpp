#pragma once

/**
 * ASE RESOURCE MANAGER (NOT A COMPONENT!)
 *
 * @file        godot_host_resource_manager_class.hpp
 * @brief       GodotHostResourceManager - External resource manager for the embedded ASE host of AseVivariumView
 * @description Holds the ONE std::unique_ptr<ase::kernel::KernelEmbeddedHost> of a Vivarium
 *              view, the start configuration it was booted with, the bundle it loaded, the views
 *              of the last snapshots of the patch, clock, organ and template ports, and the queue of
 *              log lines the host's logging thread hands over. Lives OUTSIDE every ECS registry - the
 *              App inside the host keeps its own - and carries no Godot type, so its tests run
 *              against the real bundle without Godot.
 *              THE CLASS STANDS APART FROM ITS VIEWS: the start configuration and each port view stand in a
 *              sub-header of their own (godot_host_start_config.hpp, godot_host_*_view.hpp), which the units
 *              that draw from them include directly; godot_host_resource_manager.hpp only collects them. This
 *              header holds the class and the survival body view, and only the units that name the class
 *              include it, so a new member or a new field of the body view compiles them and no other unit
 *              (wipe_guard.py ratchet, operator decision 2026-11-21; data/probes/b0680_class_reach.py counts
 *              them).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * FLYWEIGHT PATTERN (INST_ASE_ECS_SER)
 *
 *   Godot node (OOP)             ResourceManager (NOT ECS!)
 *   ┌────────────────┐          ┌────────────────────────────────────┐
 *   │ uint32_t       │ ───────> │ map<uint32_t, VivariumPatchView>   │
 *   │ selected_id    │          │ unique_ptr<KernelEmbeddedHost>     │
 *   └────────────────┘          └────────────────────────────────────┘
 *                                          ▲
 *   Host log sink (any thread)             │
 *   ┌────────────────┐                     │
 *   │ queue_log_line │ ────────────────────┘
 *   │                │   under log_mutex_, drained by the node
 *   └────────────────┘   .get_patch(selected_id)
 *
 * USAGE:
 *   // VivariumView::ready() (Godot's _ready) - play mode only, never in the editor
 *   GodotHostResourceManager resources;          // member of the view: no global, no singleton
 *   resources.boot(config);                      // factory: create, data root, load the set, start
 *
 *   // VivariumView::process (Godot's _process)
 *   resources.advance(elapsed_s);                // the real time since the last frame, never capped
 *   const VivariumPatchView* patch = resources.get_patch(selected_id);
 *
 *   // Reiniciar and VivariumView::exit_tree() (Godot's _exit_tree)
 *   resources.clear_all();                       // the binding teardown, idempotent
 *
 * THIS IS NOT A COMPONENT!
 * This class holds an external C++ object (the embedded host, its units, its logger). The node
 * stores ONLY uint32_t object ids, the manager owns everything behind them.
 *
 * ECS RESOURCE MANAGER HEADER COMPLIANCE
 *
 * [ ] NOT a Component - lives outside ECS registry
 * [ ] Accessed via registry.ctx().get<ResourceManager&>()
 * [ ] Components store ONLY uint32_t IDs
 * [ ] Header contains ONLY declarations (implementations in .cpp!)
 * [ ] Thread-safe method signatures (const where possible)
 * [ ] store_*(), get_*(), remove_*(), has_*() method pattern
 * [ ] clear_all() for shutdown cleanup
 * [ ] Private mutex for thread safety
 * [ ] Private maps for resource storage
 * [ ] NO inline implementations (prevents mass rebuilds!)
 */

#include <ase/adp/godot/godot_cycle_measure.hpp>
#include <ase/adp/godot/godot_host_bark_view.hpp>
#include <ase/adp/godot/godot_host_bundle_info.hpp>
#include <ase/adp/godot/godot_host_clock_view.hpp>
#include <ase/adp/godot/godot_host_figure_view.hpp>
#include <ase/adp/godot/godot_host_fire_view.hpp>
#include <ase/adp/godot/godot_host_organ_view.hpp>
#include <ase/adp/godot/godot_host_patch_view.hpp>
#include <ase/adp/godot/godot_host_camp_view.hpp>
#include <ase/adp/godot/godot_host_rack_view.hpp>
#include <ase/adp/godot/godot_host_seed_view.hpp>
#include <ase/adp/godot/godot_host_self_test.hpp>
#include <ase/adp/godot/godot_host_sky_view.hpp>
#include <ase/adp/godot/godot_host_start_config.hpp>
#include <ase/adp/godot/godot_host_stream_view.hpp>
#include <ase/adp/godot/godot_host_current_view.hpp>
#include <ase/adp/godot/godot_host_pit_view.hpp>
#include <ase/adp/godot/godot_host_groundwater_view.hpp>
#include <ase/adp/godot/godot_host_sap_view.hpp>
#include <ase/adp/godot/godot_host_loose_view.hpp>
#include <ase/adp/godot/godot_host_template_view.hpp>
#include <ase/adp/godot/godot_host_terrain_view.hpp>
#include <ase/adp/godot/godot_host_weather_view.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/kernel/kernel_types.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

/**
 * The body of one figure as the last snapshot of the survival body port showed it - a copy (PLAN_ASE_VIVARIUM_PHASE_06_HUD
 * U01, U03 and U05, N0530). ase-metabolism writes the MTB_* values per figure, its felt temperature from the ase-gis
 * layer GIS_LOCAL_TEMP; the port lays them per figure. A key the record does not carry stands as NaN, and the rail
 * does not show that row. The view computes none of the values.
 */
struct VivariumSurvivalBodyView {
    uint32_t object_id = 0;        // the figure the body belongs to
    double   effective_temp = 0.0; // MTB_EFFECTIVE_TEMP: the felt temperature in degrees
    double   temp_stress = 0.0;    // MTB_TEMP_STRESS: 0 to 1
    double   hunger = 0.0;         // MTB_HUNGER: 0 to 100
    double   thirst = 0.0;         // MTB_THIRST: 0 to 100
    double   energy = 0.0;         // MTB_ENERGY: 0 to 100
    double   fatigue = 0.0;        // MTB_FATIGUE: 0 to 1
    double   health = 0.0;         // MTB_HEALTH: 0 to 100
    double   poisoned = 0.0;       // TAG_MTB_POISONED: 1 or 0 by its rule in hub_thresholds.json (U02)
    double   bleeding = 0.0;       // TAG_MTB_BLEEDING: 1 or 0
    double   emaciated = 0.0;      // TAG_MTB_EMACIATED: 1 or 0
    double   well_rested = 0.0;    // TAG_MTB_WELL_RESTED: 1 or 0
    double   breath = 0.0;         // GIS_LOCAL_BREATH: the air at the figure, 0 normal or better to 1 none (N0532)
};

/**
 * The rest of one figure as the last snapshot of the survival rest port showed it - a copy (PLAN_ASE_VIVARIUM_PHASE_06_HUD
 * U05, N0555). ase-metabolism derives both values from the action ase-bdi chose (BDI_CONST_ACTION_WAIT and
 * BDI_CONST_ACTION_SLEEP); a port of its own, because the body record is full at the width of the host. A key the
 * record does not carry stands as NaN, and the rest card keeps the mockup's pose for it.
 */
struct VivariumSurvivalRestView {
    uint32_t object_id = 0;        // the figure the rest belongs to
    double   is_resting = 0.0;     // MTB_IS_RESTING (MetabolismRstSystem, 2 Hz): 1 while the figure rests, else 0 (U05, T0307)
    double   sleeping = 0.0;       // MTB_IS_SLEEPING (MetabolismRstSystem): 1 while the resting figure sleeps, else 0 (U05, N0555)
    double   rest_quality = 0.0;   // MTB_REST_QUALITY (MetabolismHubDrvPubSystem): 0 to 1, the side of the sleep (FIRECAMP T.11, N0578)
    double   rest_target = 0.0;    // MTB_CONST_REST_QUALITY_TARGET on GLOBAL: the rest quality of good rest (T.11, N0578)
};

/**
 * The spoil of the food one figure carries as the last snapshot of the survival food port showed it - a copy
 * (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.8, the ring ESTRAGO U07, N0530). ase-entropy rots every piece of the bag by
 * its temperature and lays the mass-weighted share of the figure as ENT_FOOD_ROT_CARRIED; a figure without food has
 * no record. A key the record does not carry stands as NaN, and the ring then keeps the mockup's share.
 */
struct VivariumSurvivalFoodView {
    uint32_t object_id = 0;  // the figure whose bag the food lies in
    double   spoil = 0.0;    // ENT_FOOD_ROT_CARRIED (EntropyFoodDcySystem): 0 fresh to 1 rotten, weighted by mass
};

/**
 * The soot in the air of the cell one figure stands in, as the last snapshot of the survival smoke port showed it - a
 * copy (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.8, the smoke plume of the air mark U06, D2500). ase-fire lays the soot of
 * every fire cell on the raster cell of its fuel body (FireSmokCellCalcSystem), ase-gis sums it per cell under each
 * figure as GIS_LOCAL_SOOT (GisLyrNthrSmokCellSystem); a key the record does not carry stands as NaN.
 */
struct VivariumSurvivalSmokeView {
    uint32_t object_id = 0;  // the figure whose cell the soot hangs in
    double   soot = 0.0;     // GIS_LOCAL_SOOT: kg of soot the fires gave to the air of the figure's cell, 0 where none smoked
    double   smell = 0.0;    // PER_SMELL_SMOKE: share of smoke the figure's nose caught in the sphere around it (wind-carried, B3)
};

class GodotKnotResourceManager;  // tube/godot_knot_resource_manager.hpp: the knots and feet of the bark tubes (N0543)

/**
 * @brief External resource manager for the embedded ASE host of one Vivarium view (NOT ECS!)
 *
 * This class lives OUTSIDE every registry. Its owner is the VivariumView of one AseVivariumView
 * object; the App inside the host is reached only through the host facade.
 *
 * Thread-safe where a second thread exists: the log queue is protected by log_mutex_, because
 * the host's sink may run on a logging worker. Every other method runs on the thread that
 * booted the host - the host itself refuses calls from any other.
 *
 * NOTE: Implementations are in godot_host_resource_manager.cpp to avoid
 * triggering mass rebuilds when methods change.
 */
class GodotHostResourceManager {
public:
    GodotHostResourceManager();
    ~GodotHostResourceManager();

    GodotHostResourceManager(const GodotHostResourceManager&) = delete;
    GodotHostResourceManager& operator=(const GodotHostResourceManager&) = delete;
    GodotHostResourceManager(GodotHostResourceManager&&) = delete;
    GodotHostResourceManager& operator=(GodotHostResourceManager&&) = delete;

    /**
     * The bundle index text, parsed: one `data_root`, one `unit` line per unit with six fields, a
     * `data` line per data file, '#' lines as comments. false - and `out` emptied - for an index
     * without data root or unit, a unit line of another shape, or a line of an unknown key: an
     * index this adapter does not understand is never half read.
     */
    [[nodiscard]] static bool parse_index(const std::string& text, VivariumBundleInfo& out);

    /**
     * The ground code a word of the start configuration names (GODOT_SOIL_WORD_*);
     * GODOT_CODE_UNKNOWN for any other word. The one mapping of word to code - the node reads the
     * configuration with it, the self-test its table.
     */
    [[nodiscard]] static uint8_t soil_code(const std::string& word);
    /**
     * The word of an organ class (GODOT_ORGAN_*): the key of its port in the start configuration and
     * its record in the self-test table; "" for a number that is no class.
     */
    [[nodiscard]] static const char* organ_word(uint32_t organ_class);

    /**
     * FACTORY - one host per boot
     *
     * Tears down a previous host first (Reiniciar), then: create the host with this manager's
     * log sink, read the bundle index and every staged manifest, name the data root, load the
     * whole set in the order of its manifests, check both ports, start, read the first snapshots.
     * The plugin creates its patches itself; the view finds them by their place.
     *
     * @return HostStatusOk, or the status of the first step that failed. failure_step() names
     *         that step. A host that failed after create() is KEPT, stopped, so its log still
     *         carries the cause until clear_all().
     */
    ase::kernel::HostStatus boot(const VivariumStartConfig& config);

    /**
     * SELF-TEST (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3) - the runs of the composition test
     * (config/vivarium_selftest.toml) through the real host facade and the real set of the
     * stage, BEFORE any game host: ONE test host is created and started once, every scenario
     * runs on it in the table's order, each a check of one step within a few frames, and it is
     * stopped and destroyed before the game host exists - never a second simulation beside
     * another, never a second start for the next scenario. The test host writes no log file -
     * opening one would rotate the game host's file - and hands its lines to this manager's sink
     * like the game host does.
     *
     * Refused (one error, nothing run) while this manager holds a host: the game host boots
     * after the self-test, never beside it.
     *
     * @param config  the start configuration boot() takes: stage reader, directories, ports
     * @param table   the text of the self-test table
     * @param report  receives one line per comparison ("selftest check=pour place=0 key=soil_mm
     *                actual=... expected=gt ... result=ok") and one closing line with the counts
     */
    VivariumSelfTestResult self_test(const VivariumStartConfig& config, const std::string& table,
                                     std::vector<std::string>& report);

    /**
     * RUNNING
     */
    /**
     * File the absolute real position (all frames so far plus `elapsed_s`, never capped, never
     * dropped - the world has no pause) on the real port, run the host for its one tick
     * (KernelEmbeddedHost::advance) and read both snapshots. A long frame is a return from the
     * background: ase-time sets its clock forward to the filed position in one write and reports
     * the position it laid (TIM_REAL_LAID_S); until then the span is the backlog.
     *
     * The host gets the frame's own seconds and nothing more. A jump is one write into ase-time's
     * clock through the seek or date port; every module computes the span up to the new time in
     * its own pass, so no time is handed in beyond the frame.
     */
    ase::kernel::HostStatus advance(double elapsed_s);
    /**
     * Pour `amount_mm` on one patch; the plugin takes it in its next Reception run. Its one caller is the
     * button Regar (VivariumView::water), which pours irrigate_amount_mm (usecase:100367). The HUD's can never
     * comes here: it lets its litres go through the drain port of the figure (VivariumView::water_from_can,
     * drain_figure), and ase-equipment lays their column on the soil (EquipmentReqWatrSystem, N0517).
     */
    ase::kernel::HostStatus irrigate(uint32_t object_id, double amount_mm);
    /**
     * Cut the branch the internode `object_id` sits on (03.10 step 9): a prune request on the segment
     * port; the plugin files it in its next Reception run, ase-phytomer fells the branch.
     */
    ase::kernel::HostStatus prune(uint32_t object_id);
    /**
     * Level the ground of the patch `object_id` (03.8 point 2): a level request on the level port, no
     * value; the plugin files it in its next Reception run, ase-terrain levels the disc of the patch.
     */
    ase::kernel::HostStatus level(uint32_t object_id);
    /**
     * Lay one more brook bed while the host runs (03.8, the bed the player draws): the lay request of the
     * start configuration's brooks on the brook port, the same form; ase-pl-flora files it in its next
     * Reception run, ase-terrain carves the bed and ase-hydro lets its water run.
     */
    ase::kernel::HostStatus lay_brook(const VivariumBrookSpec& brook);
    /**
     * Lay the tool rack of the patch `patch` at `x`, `z`, metres east and north of its stem foot
     * (usecase:101625): a lay request on the toolrack scene port; ase-pl-toolrack files it in its next
     * Reception run as one prop body to ase-physics, which drops it onto the ground. A patch holds one rack;
     * a second request on it is refused by the plugin. HostStatusNotFound while the host offers no rack port.
     */
    ase::kernel::HostStatus lay_rack(uint32_t patch, double x, double z);
    /**
     * Flatten a disc of the ground where the player tapped (03.8): a flatten request on the flatten port
     * with the centre `x`, `z` and the radius `radius_m`, metres; ase-pl-flora files it in its next
     * Reception run, ase-terrain levels the disc to the height of its centre.
     */
    ase::kernel::HostStatus flatten(double x, double z, double radius_m);
    /**
     * Dig the ground down where the player tapped (a spade stroke at run time): a lower request on the lower
     * port with the centre `x`, `z`, the radius `radius_m` and the depth `depth_m`, metres; ase-pl-flora files
     * it in its next Reception run as a dig order to ase-terrain, which lowers the disc. Water stands wherever
     * the ground then lies at or below the zero level.
     */
    ase::kernel::HostStatus lower(double x, double z, double radius_m, double depth_m);
    /**
     * Heap the ground up where the player tapped (a spade stroke at run time): a raise request on the raise
     * port with the centre `x`, `z`, the radius `radius_m` and the height `height_m`, metres; ase-pl-flora
     * files it as a heap order to ase-terrain, which raises the disc.
     */
    ase::kernel::HostStatus raise(double x, double z, double radius_m, double height_m);
    /**
     * Loosen the soil where the player tapped with the hoe or the digging stick (soltar, furar; B0649): a loosen
     * request on the loosen port with the centre `x`, `z`, the half side `half_m` and the tilth depth `depth_m`,
     * metres; ase-pl-flora files it as a cultivation order to ase-terrain (FloraHostCultSystem), whose compaction
     * the patch reads back through the soil layer of ase-gis (CAUSA_ASE_TERRAIN, loosened soil). A host that does
     * not hold the port answers HostStatusNotFound, and the view keeps its bridge under N0456.
     */
    ase::kernel::HostStatus loosen(double x, double z, double half_m, double depth_m);
    /**
     * Run the host's own game clock at a multiple of ase-time's base pace - the time lapse of the debug
     * mode: a pace request on the pace port with `factor` (1, 2, 4, 10, 100 ...), GODOT_PACE_NONE for the
     * default pace back. ase-pl-flora multiplies it with the base pace it reads from the star and writes
     * the product as TIM_REQ_TIME_SCALE, ase-time runs this host's clock at it; no server clock is touched.
     */
    ase::kernel::HostStatus pace(double factor);
    /**
     * Move the host's own game clock: a seek request on the seek port numbered `seq` with ONE of three
     * forms, the others GODOT_SEEK_FORM_NONE - `game_hour` an absolute game hour, `ahead_h` game hours
     * ahead of the clock, `day_hour` the next occurrence of an hour of the game day. ase-pl-flora writes
     * them as TIM_REQ_TIME_SEEK_*, ase-time lays the target from its own clock and judges it: ahead, at,
     * behind or refused, filed with `seq` on the clock record (VivariumClockView::seek_*). The view
     * computes no game time and compares no clock part: the verdict is ase-time's.
     */
    ase::kernel::HostStatus seek(double seq, double game_hour, double ahead_h, double day_hour);
    /**
     * Set the host's own game clock to a SURA date: a date request on the date port numbered `seq` with
     * VECTUM `year`, LUNBREX `month`, TUMBRAE `month_day` and HORAX `hour`. ase-pl-flora writes them as
     * CAL_REQ_DATE_SEEK_*, ase-calendar turns the date with its own constants into a game hour, and
     * ase-time moves this host's clock there, keeping its pace, and files its verdict with `seq`. The
     * view computes no date: what a year, a month or a day is stands in ase-calendar, never here.
     */
    ase::kernel::HostStatus seek_date(double seq, double year, double month, double month_day, double hour);
    /**
     * Shape the relief of the whole ground - a shape request on the elevation port with the height
     * amplitude `height_scale_m` and the ground level `base_height_m`, metres. ase-pl-flora writes them
     * as TER_GEN_HGT_SCL and TER_GEN_BAS_HGT, ase-terrain derives every height from them; the seed keeps
     * the shape of the landscape. Which values are valid is the plugin's to check.
     */
    ase::kernel::HostStatus shape(double height_scale_m, double base_height_m);
    /**
     * Tell the engine where the view's camera stands (`x`, `y`, `z`) and the point it looks at (`aim_x`,
     * `aim_y`, `aim_z`), world metres east, up and north - a view request on the view port. ase-pl-flora
     * files it for ase-camera, whose camera the level of detail of ase-render-data measures from
     * (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 point 7) and whose heading the stick of the figure steers
     * along in the fixed view (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.4). Which points are valid is the
     * plugin's to check.
     */
    ase::kernel::HostStatus view(double x, double y, double z, double aim_x, double aim_y, double aim_z);
    /**
     * Ask for the figure of this view's seat (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.8): a spawn request on the
     * spawn port, object id the seat, with the place `x` east and `z` north, metres. ase-pl-avatar files it as
     * HUB_PLR_SPWN_REQ and ase-player spawns the figure there; boot() asks once for the configured seat.
     * HostStatusNotFound while the host offers no figure port (ase-pl-avatar not in the unit set).
     */
    ase::kernel::HostStatus spawn_figure(uint32_t seat, double x, double z);
    /**
     * Lead the figure with the stick of the left half (07.4): a steer request on the stick port, object id the
     * figure, with the forward and sideways axis and the levels of the sprint and jump wish. The course the
     * stick points along is no value of the request: it is the course of the active camera, which ase-camera
     * owns and ase-pl-avatar reads. Which values are valid is the plugin's to check.
     */
    ase::kernel::HostStatus steer_figure(uint32_t figure, double forward, double strafe, double sprint, double jump);
    /** Turn the player camera of the figure by a drag of the right half (07.5): radians around and up. */
    ase::kernel::HostStatus turn_figure_camera(uint32_t figure, double yaw_delta, double pitch_delta);
    /**
     * cam.zoom (07.4, 07.6): move the player camera of the figure along its zoom rail by `zoom_delta` metres,
     * negative towards the figure. The same turn request on the orbit port as the drag, course unchanged;
     * ase-camera clamps the distance to the rail HUB_CAM_ORBT_DIST_MIN..MAX, down to the face band.
     */
    ase::kernel::HostStatus zoom_figure_camera(uint32_t figure, double zoom_delta);
    /** cam.mode (07.5): move the active mark between the fixed view and the camera following the figure. */
    ase::kernel::HostStatus switch_figure_camera(uint32_t figure);
    /** soul.mode (07.11) on a figure the player leads: the AI leads it on, the same figure keeps place and pose. */
    ase::kernel::HostStatus release_figure(uint32_t figure);
    /** soul.mode (07.11) on a figure the AI leads: its player takes it back. */
    ase::kernel::HostStatus seize_figure(uint32_t figure);
    /**
     * The hand of Phase 06 took a thing (07.10): a hold request on the hold port, object id the figure, with the
     * item as its catalog id (KernelEmbeddedHost::name_id of the item key). ase-pl-avatar files it as
     * HUB_EQP_SLOT_REQ at socket_hand_r; which item is valid is the plugin's to check.
     */
    ase::kernel::HostStatus hold_figure(uint32_t figure, uint32_t item);
    /** The hand of Phase 06 let its thing go (07.10): a drop request on the drop port, object id the figure. */
    ase::kernel::HostStatus drop_figure(uint32_t figure);
    /**
     * The pack of the seat goes on the back (N0590, usecase:101332 - wear equipment in slots): a don request on the
     * don port, object id the figure, with the item as its catalog id (KernelEmbeddedHost::name_id of the item key,
     * never of its asset name). ase-pl-avatar files it as HUB_EQP_SLOT_REQ at socket_back; which item is valid is
     * the plugin's to check, and the gear port shows the result.
     */
    ase::kernel::HostStatus don_figure(uint32_t figure, uint32_t item);
    /**
     * brookTap under SS/PC (07.14, usecase:100399 carry brook water): a fill request on the fill port, object id the
     * figure, no value. ase-pl-avatar files it as HUB_EQP_WATR_REQ under HUB_EQP_WATR_DRAW, ase-equipment fills the
     * vessel in the hand to its room; the level comes back on the gear port as water_l.
     */
    ase::kernel::HostStatus fill_figure(uint32_t figure);
    /**
     * The release of the can under SS/PC (07.14): a drain request on the drain port, object id the figure, with the
     * litres the pour let go and the patch object the water falls on. ase-pl-avatar files it under HUB_EQP_WATR_POUR,
     * ase-equipment lets go no more than the vessel holds and lays its column on that soil (N0517); which litres and
     * which patch are valid is the plugin's to check.
     */
    ase::kernel::HostStatus drain_figure(uint32_t figure, double litres, uint32_t patch);
    /**
     * Report the skeleton of a figure as the scene built it (07.6): a measure request on the rig port, object id
     * the figure, with the eye height and the height of its body above the soles and the radius of the capsule
     * around its whole body, metres. ase-pl-avatar files them for ase-camera, whose face landing lifts its aim to
     * that eye height, and for ase-physics, which collides the walking body as that capsule. Which values are valid
     * is the plugin's.
     */
    ase::kernel::HostStatus measure_figure(uint32_t figure, double eye_height, double height, double radius);
    /**
     * fig.turn (07.7): turn the figure itself on the spot by `yaw_delta` radians, a turn request on the turn port.
     * ase-pl-avatar lays it down as NET_INP_TURN; ase-player turns the figure and shows the gait of turning.
     */
    ase::kernel::HostStatus turn_figure(uint32_t figure, double yaw_delta);
    /**
     * Bind the figure to the area plate it stands on (07.7, usecase:101612 keep figure in loaded area): a bind
     * request on the area port, object id the figure, with the plate - the patch whose anchor ase-terrain filed
     * (TRN_FLAT_HGT, TRN_FLAT_X, TRN_FLAT_Z). ase-pl-avatar files it as HUB_PLR_AREA_PLATE; ase-player bounds the
     * figure to the chunk borders of the loaded ase-gis raster (HUB_GIS_GRND) and marks the rim when it reaches one.
     */
    ase::kernel::HostStatus bind_figure_area(uint32_t figure, uint32_t plate);
    /** One line of the view itself (boot line, action, measurement) through the host's logger. */
    ase::kernel::HostStatus note(const char* text);

    /**
     * VIEWS - the records of the last snapshots
     *
     * The pointers are valid until the next advance(), boot() or clear_all(): every snapshot
     * rebuilds the views. Hold the id or the place, never the pointer, across frames.
     */
    [[nodiscard]] const VivariumPatchView* get_patch(uint32_t object_id) const;
    [[nodiscard]] const VivariumPatchView* get_patch_at(uint32_t place) const;
    // The game hour ase-plant forecasts the patch to reach the stage at a position of the stage record
    // (usecase:101635, D2455): the grow position takes grow_at_h, the mature position mtur_at_h, by the key
    // the position is read under; every other position and an unknown one have no forecast (GODOT_HOUR_NONE).
    [[nodiscard]] double stage_due_h(const VivariumPatchView& patch, uint8_t position) const;
    [[nodiscard]] bool has_patch(uint32_t object_id) const;
    /**
     * The cap radius of the fruit body of every patch whose species carries one, in metres, keyed by the patch's
     * object id (flora.cap.v1, PHY_CAP_RADIUS, usecase:101362 scale the organ templates). Read with every snapshot;
     * a patch without a record of the cap port has no entry and shows no fruit body.
     */
    [[nodiscard]] const std::unordered_map<uint32_t, double>& get_caps() const;
    [[nodiscard]] uint32_t patch_count() const;
    [[nodiscard]] const VivariumClockView& get_clock() const;
    /**
     * The weather of the sky as sky.weather.v1 and sky.weather.code.v1 showed it (usecase:5606 observe the sky,
     * N0449): the current kind, temperature and wind, the forecast of each coming period and ase-atmosphere's codes.
     * Read with every snapshot; `known` and `forecast_known` stay false while a port is not offered or its records
     * are not complete, and the HUD's bridge then draws the weather of SHA.
     */
    [[nodiscard]] const VivariumWeatherView& get_weather() const;
    /**
     * The tap in the birch and its bark vessel as flora.sap.v1 showed them (N0457): running, the litres held now and
     * the litres held when full. Read with every snapshot; `known` stays false while the port is not offered or files
     * no record, and the HUD's bridge then draws the tap of SHA.
     */
    [[nodiscard]] const VivariumSapView& get_sap() const;
    /**
     * The looseness raster of the soil as flora.loose.v1 showed it (N0456): one cell per cell of the owner's ase-gis
     * layer, row by row from the south-west cell. Read with every snapshot; `known` stays false while the port is not
     * offered or files no cell, and the HUD's loose bridge then draws the mockup's grid.
     */
    [[nodiscard]] const VivariumLooseView& get_loose() const;
    /**
     * The looseness 0..1 of the cell that holds the world point `x`, `z` (N0456): the reader asks its cell, it never
     * computes one. NaN while the raster is not known or the point lies outside it.
     */
    [[nodiscard]] double loose_at(double x, double z) const;
    /**
     * The organs of one class (GODOT_ORGAN_*) as their port showed them. The organ ports are read
     * when a Dissemination run passed since the last read - the only schedule in which the modules
     * change the organ rows - so a frame without one reuses the views. An unknown class answers
     * an empty set.
     */
    [[nodiscard]] const std::vector<VivariumOrganView>& get_organs(uint32_t organ_class) const;
    /**
     * The instances of one drawn class (GODOT_ORGAN_* before the root) as their instance port showed
     * them, read together with the organs. A class the view does not draw answers an empty set.
     */
    [[nodiscard]] const std::vector<VivariumInstanceView>& get_instances(uint32_t organ_class) const;
    /** Grows by one every time the organ views were read anew; the view rebuilds its geometry then. */
    [[nodiscard]] uint64_t organ_generation() const;
    /**
     * The vertices of the bark tubes of every plant as the bark port showed them (register B0295 point 4).
     * ase-pl-flora lays a new set page by page, one page per GODOT_BARK_SCHEDULE pass, and the host keeps the
     * old set until the last page is filed; so after a GODOT_ORGAN_SCHEDULE run the port is read with every
     * snapshot until the new set is in: until a read differs from the held set, or names exactly the held
     * internodes once the passes since the run covered the organ reads and every page of the set.
     */
    [[nodiscard]] const std::vector<VivariumBarkVertexView>& get_bark() const;
    /** Grows by one every time a read of the bark port differed from the held set; the view lays the tubes anew then. */
    [[nodiscard]] uint64_t bark_generation() const;
    /** The knots and feet of the bark tubes as the knot and foot ports showed them, read after the organs (N0543). */
    [[nodiscard]] const GodotKnotResourceManager& knots() const;
    /**
     * The terrain samples of the ground as the terrain port showed them. Read like the organs: when a
     * GODOT_TERRAIN_SCHEDULE run passed since the last read, that frame and the next.
     */
    [[nodiscard]] const std::vector<VivariumTerrainView>& get_terrain() const;
    /**
     * The cells of the ground as the ground cell port showed them, read together with the terrain
     * samples their corners name.
     */
    [[nodiscard]] const std::vector<VivariumGroundCellView>& get_ground_cells() const;
    /** Grows by one every time the terrain views were read anew; the view rebuilds the ground then. */
    [[nodiscard]] uint64_t terrain_generation() const;
    /** The brooks with water as the stream port showed them, read with every snapshot. */
    [[nodiscard]] const std::vector<VivariumStreamView>& get_streams() const;
    /** Grows by one every time the water of a brook differs from the last read; the view lays it anew then. */
    [[nodiscard]] uint64_t stream_generation() const;
    /**
     * The river cells with flowing water as the current port of ase-pl-water showed them, read with every snapshot
     * once the plugin offers the port; empty before.
     */
    [[nodiscard]] const std::vector<VivariumCurrentView>& get_currents() const;
    /** Grows by one every time the current of a cell differs from the last read; the view lays it anew then. */
    [[nodiscard]] uint64_t current_generation() const;
    /**
     * The dug pits with their water as the pit port of ase-pl-hydrogeology showed them, read with every snapshot
     * once the plugin offers the port; empty before.
     */
    [[nodiscard]] const std::vector<VivariumPitView>& get_pits() const;
    /** Grows by one every time the water of a pit differs from the last read; the view lays it anew then. */
    [[nodiscard]] uint64_t pit_generation() const;
    /**
     * The groundwater of every chunk as the groundwater port of ase-pl-hydrogeology showed it, read with every
     * snapshot once the plugin offers the port; empty before.
     */
    [[nodiscard]] const std::vector<VivariumGroundwaterView>& get_groundwater() const;
    /** Grows by one every time the groundwater of a chunk differs from the last read. */
    [[nodiscard]] uint64_t groundwater_generation() const;
    /**
     * The salt of every chunk as the salt port of ase-pl-hydrogeology showed it (H.9, I-4): water half, soil half
     * and sum, read with every snapshot once the plugin offers the port; empty before.
     */
    [[nodiscard]] const std::vector<VivariumSaltView>& get_salt() const;
    /** Grows by one every time the salt of a chunk differs from the last read. */
    [[nodiscard]] uint64_t salt_generation() const;
    /**
     * The corners of the organ templates as the template corner port showed them, every code together
     * (usecase:101011 tessellate the organ templates). ase-render-data builds the templates once when the host
     * starts, so the ports are read with every snapshot until both carry a set, and not again until a
     * new host boots.
     */
    [[nodiscard]] const std::vector<VivariumTemplateCornerView>& get_template_corners() const;
    /** The triangles of the organ templates as the template triangle port showed them, read with the corners. */
    [[nodiscard]] const std::vector<VivariumTemplateTriangleView>& get_template_triangles() const;
    /** Grows by one every time the template views were read; the view rebuilds its plants then. */
    [[nodiscard]] uint64_t template_generation() const;
    /**
     * The sky and the light of the scene as the sky port showed them, read with every snapshot: four
     * records, a few numbers, and ase-pl-sky lays them anew every frame. known stays false until the
     * plugin's light pipeline has written its state and the port carries all four records.
     */
    [[nodiscard]] const VivariumSkyView& get_sky() const;
    /**
     * The seed bodies as the seed port showed them, read with every snapshot: ase-physics moves them in
     * every kinetic step, and a seed that came down keeps its place. Empty while no fruit stand shattered.
     */
    [[nodiscard]] const std::vector<VivariumSeedView>& get_seeds() const;
    /**
     * The tool rack bodies as the rack port showed them, read with every snapshot: ase-physics drops a laid
     * rack in its kinetic steps and keeps its rest place. Empty while no rack is laid or the host offers no
     * rack port.
     */
    [[nodiscard]] const std::vector<VivariumRackView>& get_racks() const;
    /** true when the running host offers both rack ports of the start configuration. */
    [[nodiscard]] bool rack_offered() const;
    /** The body of every figure as the survival body port showed it, read with every snapshot; empty without the port. */
    [[nodiscard]] const std::vector<VivariumSurvivalBodyView>& get_survival_bodies() const;
    /** true when the running host offers the survival body port of the start configuration. */
    [[nodiscard]] bool survival_body_offered() const;
    /** The rest of every figure as the survival rest port showed it, read with every snapshot; empty without the port. */
    [[nodiscard]] const std::vector<VivariumSurvivalRestView>& get_survival_rests() const;
    /** true when the running host offers the survival rest port of the start configuration. */
    [[nodiscard]] bool survival_rest_offered() const;
    /** The spoil of the food of every figure that carries food, read with every snapshot; empty without the port. */
    [[nodiscard]] const std::vector<VivariumSurvivalFoodView>& get_survival_foods() const;
    /** true when the running host offers the survival food port of the start configuration. */
    [[nodiscard]] bool survival_food_offered() const;
    /** The soot in the air of the cell of every figure ase-gis tracks, read with every snapshot; empty without the port. */
    [[nodiscard]] const std::vector<VivariumSurvivalSmokeView>& get_survival_smokes() const;
    /** true when the running host offers the survival smoke port of the start configuration. */
    [[nodiscard]] bool survival_smoke_offered() const;
    /**
     * Lays one piece of the start stock into the bag of a figure (T.8): the item travels as the hash of its
     * item_art.json key, the mass in kg; ase-pl-survival files it as HUB_EQP_FOOD_REQ.
     */
    ase::kernel::HostStatus stock_figure(uint32_t figure, uint32_t item, double kg);
    /**
     * Lays the camp place `place` (the index of the entry camp) at `east`, `north` world metres on ingle.camp.v1
     * (T.0, T.3); IngleHostCampSystem lays one camp entity per request. HostStatusNotFound without the camp ports.
     */
    ase::kernel::HostStatus lay_camp(uint32_t place, double east, double north);
    /**
     * Files one corner `corner` of the convex hull of the make `make` on ingle.hull.v1 (T.1): metres east, up and
     * north of its centre of mass, as the node extras of its export carry them. Once per corner of every make.
     */
    ase::kernel::HostStatus file_hull(uint32_t corner, uint32_t make, double hx, double hy, double hz);
    /**
     * Lays one part of the camp as a rigid body on ingle.part.v1 (T.3): in the frame of the patch `patch`,
     * `east`, `north` of its stem foot, its centre of mass at `drop` above it, in the turn `turn_x`, `turn_y`,
     * `turn_z` (rotation vector, radians) the pyramid of the camp lays it in, with fall speed, mass, make and the
     * roughness `rough` of its export; `fuel` gives it a fire cell of ase-fire. The plugin files it to
     * ase-physics, which lets it settle on the ground and on the parts laid before it.
     */
    ase::kernel::HostStatus file_part(uint32_t patch, double east, double north, double drop, double fall,
                                      double mass, uint32_t make, double rough, double turn_x, double turn_y,
                                      double turn_z, bool fuel);
    /** true when the running host offers the camp place, hull and part ports of the start configuration. */
    [[nodiscard]] bool camp_offered() const;
    /** Every camp part body as ingle.body.v1 showed it, read with every snapshot; empty without the port. */
    [[nodiscard]] const std::vector<VivariumCampBodyView>& get_camp_bodies() const;
    /** true when the running host offers the camp body port of the start configuration. */
    [[nodiscard]] bool camp_body_offered() const;
    /** Every keeper laying wood as ingle.refuel.v1 showed it, read with every snapshot; empty without the port. */
    [[nodiscard]] const std::vector<VivariumCampRefuelView>& get_camp_refuels() const;
    /** true when the running host offers the camp refuel port of the start configuration. */
    [[nodiscard]] bool camp_refuel_offered() const;
    /** Every fire cell as the fire port showed it, read with every snapshot; empty without the port or a fire. */
    [[nodiscard]] const std::vector<VivariumSurvivalFireView>& get_survival_fires() const;
    /** true when the running host offers the fire port of the start configuration. */
    [[nodiscard]] bool survival_fire_offered() const;
    /** true when the running host offers the port of the ash and the burn time of the start configuration. */
    [[nodiscard]] bool survival_left_offered() const;
    /**
     * The figures as the body port showed them, read with every snapshot: ase-player moves them in every
     * frame. Empty while the host offers no body port or no figure stands.
     */
    [[nodiscard]] const std::vector<VivariumFigureView>& get_figures() const;
    /** The player camera as the camera port showed it, read with every snapshot; known false in the fixed view. */
    [[nodiscard]] const VivariumFigureCameraView& get_figure_camera() const;
    /**
     * The pieces the figures carry as the gear port showed them, read with every snapshot (07.10): the plugin
     * closes the set every Production run, so a figure without a piece holds nothing. Empty without figures.
     */
    [[nodiscard]] const std::vector<VivariumFigureGearView>& get_figure_gear() const;
    /**
     * The codes of the body record as the code port showed them, read with every snapshot (usecase:101611): the
     * plugin files the one code record every Production run. known false without figures.
     */
    [[nodiscard]] const VivariumFigureCodeView& get_figure_code() const;
    /**
     * The bumps of the walking bodies against the chunk edge as the boundary port showed them, read with every
     * snapshot (07.13, usecase:101612): the plugin closes the set every Production run, so a body without a
     * record has never bumped. Empty without figures.
     */
    [[nodiscard]] const std::vector<VivariumFigureBumpView>& get_figure_bumps() const;
    /** true when the running host offers every figure port of the start configuration. */
    [[nodiscard]] bool figure_offered() const;

    /**
     * LOG LINES - any thread in, the node's thread out
     *
     * On Android the line goes straight to logcat (__android_log_write, tag GODOT_LOG_SOURCE)
     * from the thread that wrote it and is NOT queued: logcat is the platform's console, and a
     * line held for the next frame would be lost with a process that never reaches it. Elsewhere
     * it waits in the queue for the node, which prints it to Godot's output.
     */
    void store_log_line(const char* line, uint32_t len, int level);
    /** Move every queued line into `out` (appended); returns how many were moved. */
    uint32_t remove_log_lines(std::vector<std::string>& out);

    /**
     * STATE AND MEASUREMENT
     */
    [[nodiscard]] bool running() const;
    [[nodiscard]] bool has_host() const;
    [[nodiscard]] const char* failure_step() const;
    [[nodiscard]] ase::kernel::HostStatus failure_status() const;
    [[nodiscard]] const VivariumBundleInfo& get_bundle() const;
    /** Real seconds the host has simulated: filed, less the span of a return ase-time has not laid yet. */
    [[nodiscard]] double simulation_seconds() const;
    /**
     * Simulated seconds the host's App has ticked, each tick clamped to the scheduler's frame
     * (KernelEmbeddedHost::ticked_seconds) - the time a slow tier counts its interval in. 0 without
     * a running host. The landing note of a jump waits debug.settle_s of this time.
     */
    [[nodiscard]] double stage_seconds() const;
    /** Ticks the host ran since boot, all calls together. */
    [[nodiscard]] uint64_t tick_count() const;
    /**
     * Ticks of the last advance(), and the real seconds of a return from the background the clock
     * has not been set forward over yet: the filed real position less TIM_REAL_LAID_S, 0 once laid.
     */
    [[nodiscard]] uint32_t last_steps() const;
    [[nodiscard]] double backlog_seconds() const;
    [[nodiscard]] uint32_t system_count() const;
    [[nodiscard]] uint32_t unit_count() const;

    /**
     * The proportional set size of this process in KiB, read from /proc/self/smaps_rollup - the
     * memory figure of acceptance A10 and of the reset check (PLAN 02.3). false, and out 0, when
     * the kernel offers no rollup: a missing measurement is never reported as a small one.
     */
    [[nodiscard]] bool read_pss_kib(uint64_t& out) const;

    /**
     * The measurement line of 03.6 on this manager's host (godot_cycle_measure.hpp): begin sets the
     * cursors of every (unit, schedule) to the host's totals now, cycle writes the line of the cycle
     * that just ended. Both answer false without a host; cycle then writes "refused=no_host".
     */
    [[nodiscard]] bool measure_begin(CycleMeasure& measure) const;
    bool measure_cycle(CycleMeasure& measure, std::string& line);

    /**
     * How often the host's App ran the named schedule - counted by its tick scheduler, never
     * derived here from simulated time. Answers like KernelEmbeddedHost::schedule_runs;
     * HostStatusInvalidState without a running host, which is then not asked at all. runs is 0
     * on every answer but HostStatusOk.
     */
    [[nodiscard]] ase::kernel::HostStatus schedule_runs(const char* schedule, uint64_t* runs) const;

    /** Readable name of a HostStatus for the error panel and the log. */
    [[nodiscard]] static const char* status_name(ase::kernel::HostStatus status);

    /**
     * BULK CLEANUP - stop and destroy the host, drop every view and the bundle info.
     * Idempotent; the destructor calls it.
     */
    void clear_all();

private:
    ase::kernel::HostStatus fail(const char* step, ase::kernel::HostStatus status);
    ase::kernel::HostStatus read_bundle();
    ase::kernel::HostStatus read_manifests();
    ase::kernel::HostStatus load_set(ase::kernel::KernelEmbeddedHost& host);
    ase::kernel::HostStatus read_snapshot();
    ase::kernel::HostStatus read_organs();
    ase::kernel::HostStatus read_bark();
    ase::kernel::HostStatus read_knots();
    ase::kernel::HostStatus read_terrain();
    ase::kernel::HostStatus read_streams();
    ase::kernel::HostStatus read_currents();
    ase::kernel::HostStatus read_pits();
    ase::kernel::HostStatus read_groundwater();
    ase::kernel::HostStatus read_salt();
    ase::kernel::HostStatus read_templates();
    ase::kernel::HostStatus read_sky();
    ase::kernel::HostStatus read_stars();
    ase::kernel::HostStatus read_seeds();
    ase::kernel::HostStatus read_racks();
    ase::kernel::HostStatus read_survival_bodies();
    ase::kernel::HostStatus read_survival_rests();
    ase::kernel::HostStatus read_survival_foods();
    ase::kernel::HostStatus read_survival_smokes();
    ase::kernel::HostStatus read_survival_fires();
    ase::kernel::HostStatus read_survival_left();
    ase::kernel::HostStatus read_weather();
    ase::kernel::HostStatus read_sap();
    ase::kernel::HostStatus read_loose();
    ase::kernel::HostStatus read_camp_bodies();
    ase::kernel::HostStatus read_camp_refuels();
    ase::kernel::HostStatus read_figures();
    ase::kernel::HostStatus lay_patches();
    ase::kernel::HostStatus lay_ground();
    ase::kernel::HostStatus lay_brooks();
    /** The string load_units takes for one library: its path under library_dir, or its bare name. */
    [[nodiscard]] std::string library_path(const std::string& library) const;

    // The log queue is declared BEFORE the host: members die in reverse order, so the host - and
    // the logger it owns, whose sink writes into this queue - is gone before the queue is.
    std::mutex                                       log_mutex_;
    std::vector<std::string>                         log_lines_;

    VivariumStartConfig                              config_;
    VivariumBundleInfo                               bundle_;
    std::vector<std::string>                         manifests_;   // text of each unit's manifest, index order
    std::vector<ase::kernel::KernelHostRecord>       records_;
    std::unordered_map<uint32_t, VivariumPatchView>  patches_;
    std::unordered_map<uint32_t, uint32_t>           places_;      // place → object id of the last snapshot
    std::unordered_map<uint32_t, double>             caps_;        // patch object id → cap radius in metres of the cap port, last read (usecase:101362 scale the organ templates)
    VivariumClockView                                clock_;
    VivariumWeatherView                              weather_;     // the weather of the sky, last read (N0449)
    VivariumSapView                                  sap_;         // the birch's tap and bark vessel, last read (N0457)
    VivariumLooseView                                loose_;       // the soil's looseness raster, last read (N0456)
    std::vector<VivariumOrganView>                   organs_[GODOT_ORGAN_COUNT];  // index = GODOT_ORGAN_*
    const std::vector<VivariumOrganView>             no_organs_;  // the answer for an unknown class: always empty
    std::vector<VivariumInstanceView>                instances_[GODOT_DRAWN_COUNT];  // index = GODOT_ORGAN_* before the root
    const std::vector<VivariumInstanceView>          no_instances_;  // the answer for a class not drawn: always empty
    std::vector<ase::kernel::KernelHostRecord>       organ_records_;  // snapshot buffer of the organ ports
    uint64_t                                         organ_generation_ = 0;
    uint64_t                                         organ_runs_ = 0;  // GODOT_ORGAN_SCHEDULE runs at the last organ read
    uint32_t                                         organ_reads_due_ = 0;  // reads still owed for the last new run
    std::vector<VivariumBarkVertexView>              bark_;     // the vertices of the bark tubes, last set read
    uint64_t                                         bark_generation_ = 0;
    uint64_t                                         bark_runs_ = 0;    // GODOT_ORGAN_SCHEDULE runs at the last new run the bark read followed
    uint64_t                                         bark_passes_ = 0;  // GODOT_BARK_SCHEDULE passes at that run
    bool                                             bark_owed_ = false;  // a run's new set is not read yet
    std::unique_ptr<GodotKnotResourceManager>        knots_;    // the knots and feet of the bark tubes, last set read (N0543)
    std::vector<VivariumTerrainView>                 terrain_;  // the samples of the ground, last read
    std::vector<VivariumGroundCellView>              ground_cells_;  // the cells of the ground, read with the samples
    uint64_t                                         terrain_generation_ = 0;
    uint64_t                                         terrain_runs_ = 0;  // GODOT_TERRAIN_SCHEDULE runs at the last terrain read
    uint32_t                                         terrain_reads_due_ = 0;  // reads still owed for the last new run
    std::vector<VivariumStreamView>                  streams_;  // the brooks with water, last read
    uint64_t                                         stream_generation_ = 0;
    std::vector<VivariumCurrentView>                 currents_;  // the river cells with flowing water, last read
    uint64_t                                         current_generation_ = 0;
    std::vector<VivariumPitView>                     pits_;      // the dug pits with their water, last read
    uint64_t                                         pit_generation_ = 0;
    std::vector<VivariumGroundwaterView>             groundwater_;  // the groundwater of every chunk, last read
    uint64_t                                         groundwater_generation_ = 0;
    std::vector<VivariumSaltView>                    salt_;      // the salt of every chunk, last read
    uint64_t                                         salt_generation_ = 0;
    std::vector<VivariumTemplateCornerView>          template_corners_;    // the corners of the organ templates, every code
    std::vector<VivariumTemplateTriangleView>        template_triangles_;  // the triangles of the organ templates, every code
    uint64_t                                         template_generation_ = 0;
    VivariumSkyView                                  sky_;      // the sky and the light of the scene, last read
    uint64_t                                         star_generation_ = 0;  // star sets read since the start, never reset
    std::vector<VivariumSeedView>                    seeds_;    // the seed bodies, last read
    std::vector<VivariumRackView>                    racks_;    // the tool rack bodies, last read
    std::vector<VivariumSurvivalBodyView>            survival_bodies_;  // each figure's body of the survival body port, last read
    std::vector<VivariumSurvivalRestView>            survival_rests_;   // each figure's rest of the survival rest port, last read
    std::vector<VivariumSurvivalFoodView>            survival_foods_;   // the spoil of each figure's food of the survival food port, last read
    std::vector<VivariumSurvivalSmokeView>           survival_smokes_;  // the soot in the air of each figure's cell of the survival smoke port, last read
    std::vector<VivariumSurvivalFireView>            survival_fires_;   // every fire cell of the fire port, last read
    std::vector<VivariumCampBodyView>                camp_bodies_;      // every camp part body of ingle.body.v1, last read
    std::vector<VivariumCampRefuelView>              camp_refuels_;     // every keeper laying wood of ingle.refuel.v1, last read
    std::vector<VivariumFigureView>                  figures_;  // the figures of the body port, last read
    VivariumFigureCameraView                         figure_camera_;  // the player camera of the camera port, last read
    std::vector<VivariumFigureGearView>              figure_gear_;    // the pieces of the gear port, last read
    VivariumFigureCodeView                           figure_code_;    // the codes of the code port, last read
    std::vector<VivariumFigureBumpView>              figure_bumps_;   // the bumps of the boundary port, last read
    const char*                                     failure_step_;
    ase::kernel::HostStatus                          failure_status_;
    double                                           elapsed_in_s_ = 0.0;
    double                                           backlog_s_ = 0.0;   // filed real seconds of a return not laid yet
    double                                           return_real_s_ = 0.0;  // real position a long frame filed, 0 while no return is open
    uint64_t                                         tick_count_ = 0;
    uint32_t                                         last_steps_ = 0;
    std::unique_ptr<ase::kernel::KernelEmbeddedHost> host_;
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE RESOURCE MANAGER (NOT A COMPONENT!)
 *
 * @file        godot_host_resource_manager.hpp
 * @brief       GodotHostResourceManager - External resource manager for the embedded ASE host of AseVivariumView
 * @description Holds the ONE std::unique_ptr<ase::kernel::KernelEmbeddedHost> of a Vivarium
 *              view, the start configuration it was booted with, the bundle it loaded, the views
 *              of the last snapshots of the patch, clock, organ and template ports, and the queue of
 *              log lines the host's logging thread hands over. Lives OUTSIDE every ECS registry - the
 *              App inside the host keeps its own - and carries no Godot type, so its tests run
 *              against the real bundle without Godot.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Component System (ECS)
 * @created     2026-10-05
 * @modified    2026-10-28
 * @version     00.00.04.00010 [seed]
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
 *   resources.advance(elapsed_s, lapse_s, lapse_steps);  // the real time since the last frame, never capped; the debug lapse on top
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
#include <ase/adp/godot/types.hpp>
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
 * Reads one file of the staged bundle by its name (the bundle index, a manifest it names) into
 * `out`; false when the stage holds no such file. The node reads through Godot's FileAccess, so
 * the stage may lie inside the exported package (Android); a test reads the build's bundle
 * directory. The manager itself never opens a stage file.
 */
using VivariumStageReadFn = bool (*)(const char* name, std::string& out, void* user);

/**
 * One patch of the start configuration (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS, "Fläche und Art"): the
 * game host asks ase-pl-flora for it with one lay request on the scene port after its start.
 */
struct VivariumPatchSpec {
    uint32_t    place = 0;     // where the view draws it; every place stands once
    std::string species;       // catalog id of the species sown on it (modules/ase-plant/data/plant_species.json)
    uint32_t    seed = 0;      // seed of the plant: the same seed grows the same plant
    uint8_t     soil = 0;      // GODOT_SOIL_* ground the view draws
    double      x = 0.0;       // centre of the patch, world metres east; ase-terrain levels the ground there
    double      z = 0.0;       // centre of the patch, world metres north
    double      half_m = 0.0;  // half edge of the patch square, metres; the levelled disc has this radius
};

/**
 * The measured square of terrain the view draws as the ground of the scene (PLAN_ASE_VIVARIUM_PHASE_03_
 * ASSETS 03.8): one lay request on the ground port of ase-pl-flora, which orders the samples at
 * ase-terrain.
 */
struct VivariumGroundSpec {
    double x = 0.0;       // centre of the square, world metres east
    double z = 0.0;       // centre of the square, world metres north
    double half_m = 0.0;  // half edge of the square, metres
    double step_m = 0.0;  // distance of two samples, metres
};

/**
 * One brook of the start configuration: one lay request on the brook port of ase-pl-flora, which has
 * ase-terrain carve its bed and registers its water with ase-hydro.
 */
struct VivariumBrookSpec {
    double x = 0.0;             // start of the bed, world metres east
    double z = 0.0;             // start of the bed, world metres north
    double end_x = 0.0;         // end of the bed, world metres east
    double end_z = 0.0;         // end of the bed, world metres north
    double half_m = 0.0;        // half width of the bed, metres
    double depth_m = 0.0;       // depth of the bed below the ground, metres
    double base_m3_s = 0.0;     // base flow without rain, m3/s; ase-hydro holds its bounds
    double catchment_m2 = 0.0;  // area whose rain runs into the brook, m2
};

/**
 * Everything boot() and self_test() need. The node fills it from res://config/vivarium_start.json
 * and from the stage Godot loaded this library from; no value of it lives in a unit.
 */
struct VivariumStartConfig {
    VivariumStageReadFn read_stage = nullptr;  // REQUIRED: reads the bundle index and the manifests
    void*       read_stage_user = nullptr;     // handed back to read_stage unchanged
    std::string library_dir;         // ABSOLUTE directory the unit libraries lie in; EMPTY: every
                                     // library is opened by its file name and the platform linker
                                     // resolves it in the app's own namespace (Android, PLAN 02.1)
    std::string files_dir;           // ABSOLUTE directory of the stage on this device's file system:
                                     // the bundle's data root lies below it (the stage itself on a
                                     // desktop, the directory its data files are placed in where the
                                     // stage is a package)
    std::string log_file;            // ABSOLUTE log file under user://, empty = callback only
    std::string patch_port;          // host port of the patches (ase-pl-flora: flora.patch.v1)
    std::string clock_port;          // host port of the game clock (ase-pl-flora: flora.clock.v1)
    std::string scene_port;          // host port the patches are laid through (ase-pl-flora: flora.scene.v1)
    std::string organ_ports[GODOT_ORGAN_COUNT];  // one host port per organ class, index = GODOT_ORGAN_*
    std::string instance_ports[GODOT_DRAWN_COUNT];  // one instance port per drawn class, index = GODOT_ORGAN_* before the root
    std::string ground_port;         // host port the ground square is laid through (ase-pl-flora: flora.ground.v1)
    std::string brook_port;          // host port the brooks are laid through (ase-pl-flora: flora.brook.v1)
    std::string terrain_port;        // host port of the terrain samples (ase-pl-flora: flora.terrain.v1)
    std::string ground_cell_port;    // host port of the ground cells (ase-pl-flora: flora.ground.cell.v1)
    std::string sky_port;            // host port of the sky and the light of the scene (ase-pl-sky: sky.scene.v1)
    std::string seed_port;           // host port of the seed bodies of ase-physics (ase-pl-flora: flora.seed.v1)
    std::string tile_port;           // host port of the bark tiling of the internodes (ase-pl-flora: flora.tile.v1)
    std::string level_port;          // host port the player levels a patch through (ase-pl-flora: flora.level.v1)
    std::string flatten_port;        // host port the player flattens the ground at a place through (ase-pl-flora: flora.flatten.v1)
    std::string pace_port;           // host port the debug mode asks the pace of the game clock through (ase-pl-flora: flora.pace.v1)
    std::string view_port;           // host port the view hands the place of its camera through (ase-pl-flora: flora.view.v1)
    std::string template_corner_port;   // host port of the corners of the organ templates (ase-pl-flora: flora.template.corner.v1)
    std::string template_triangle_port;  // host port of the triangles of the organ templates (ase-pl-flora: flora.template.triangle.v1)
    std::string op_irrigate;         // submit operation that pours on one patch
    std::string op_prune;            // submit operation that cuts the branch an internode sits on (03.10 step 9)
    std::string op_level;            // submit operation that levels the ground of one patch (03.8 point 2)
    std::string op_flatten;          // submit operation that flattens a disc of the ground at a place (03.8)
    std::string op_pace;             // submit operation that runs the host's game clock at a time scale (debug time lapse)
    std::string op_view;             // submit operation that tells the engine where the view's camera stands (level of detail)
    std::string op_lay;              // submit operation that lays one patch, the ground square or a brook
    double      irrigate_amount_mm = 0.0;  // water one press of Regar pours, mm
    uint32_t    catch_up_steps = 0u;       // most ticks one frame may run while the host catches up
    std::vector<VivariumPatchSpec> patches;  // the patches the game host lays, every place once
    VivariumGroundSpec ground;               // the square of terrain the view draws
    std::vector<VivariumBrookSpec> brooks;   // the brooks the game host lays
};

/**
 * One terrain sample as the last snapshot of the terrain port showed it - a copy: a vertex of the
 * ground. Place and height are ase-terrain's, normal and texture coordinates ase-render-data's; the
 * view takes all of them into the ground mesh and computes none of its own.
 */
struct VivariumTerrainView {
    uint32_t object_id = 0;   // the sample, as the plugin names it on its port
    uint32_t body = 0;        // the ground the sample belongs to
    double   x = 0.0;         // world metres east
    double   z = 0.0;         // world metres north
    double   height = 0.0;    // ground height, metres
    double   nx = 0.0;        // unit normal of the ground, east share
    double   ny = 0.0;        // unit normal of the ground, upward share
    double   nz = 0.0;        // unit normal of the ground, north share
    double   u = 0.0;         // texture coordinate, world metres east
    double   v = 0.0;         // texture coordinate, world metres north
};

/**
 * One ground cell as the last snapshot of the ground cell port showed it - a copy. The four corners
 * name terrain samples by their object id, in the order of the index buffer: the view draws the
 * triangles (sw, se, ne) and (sw, ne, nw), Godot's front face looking up (ase-render-data,
 * RenderDataGrndCellCalcSystem).
 */
struct VivariumGroundCellView {
    uint32_t object_id = 0;   // the cell's south-west sample, as the plugin names it on its port
    uint32_t body = 0;        // the ground the cell belongs to
    uint32_t sw = 0;          // object id of the south-west corner sample
    uint32_t se = 0;          // object id of the south-east corner sample
    uint32_t ne = 0;          // object id of the north-east corner sample
    uint32_t nw = 0;          // object id of the north-west corner sample
};

/**
 * One corner of an organ template as the last snapshot of the template corner port showed it - a copy
 * (usecase:101011 tessellate the organ templates). ase-render-data builds the corner on the unit - base at the
 * origin, length 1 along +y - and the view takes its place, normal and texture coordinates into the
 * template mesh of its code and computes none of them.
 */
struct VivariumTemplateCornerView {
    uint32_t object_id = 0;   // the corner row, as the plugin names it on its port
    uint32_t code = 0;        // GODOT_TMPL_* of the template it belongs to
    uint32_t index = 0;       // its place in the template, named by the triangles
    double   x = 0.0;         // place on the unit, across
    double   y = 0.0;         // place on the unit, along the organ
    double   z = 0.0;         // place on the unit, across
    double   nx = 0.0;        // unit normal, x share
    double   ny = 0.0;        // unit normal, y share
    double   nz = 0.0;        // unit normal, z share
    double   u = 0.0;         // texture coordinate around the ring or across the leaf
    double   v = 0.0;         // texture coordinate along the organ
};

/**
 * One triangle of an organ template as the last snapshot of the template triangle port showed it - a
 * copy. Its three corners are indices of the corners of the same code, clockwise seen from outside -
 * Godot's front face - in the order ase-render-data laid them; the view decides no winding.
 */
struct VivariumTemplateTriangleView {
    uint32_t object_id = 0;   // the triangle row, as the plugin names it on its port
    uint32_t code = 0;        // GODOT_TMPL_* of the template it belongs to
    uint32_t a = 0;           // index of its first corner
    uint32_t b = 0;           // index of its second corner
    uint32_t c = 0;           // index of its third corner
};

/**
 * One organ as the last snapshot of its port showed it - a copy, never a reference into the App. The
 * place is plant-local (origin at the stem base on the ground, y up, x east, z north); body names the
 * patch the view stands the plant on.
 *
 * Every organ carries its frame in place of a direction: the unit quaternion ase-render-data built
 * from the roll of ase-phytomer (shoot) or ase-radix (root). It turns the plant frame into the organ's -
 * local y along the organ, local x along its roll axis.
 */
struct VivariumOrganView {
    uint32_t object_id = 0;   // the organ, as the plugin names it on its port
    uint32_t body = 0;        // object id of the patch the plant stands on
    double   order = 0.0;     // branching order of its axis (stem 0)
    double   x = 0.0;         // metres east of the stem base
    double   y = 0.0;         // metres above the ground at the stem base
    double   z = 0.0;         // metres north of the stem base
    double   qx = 0.0;        // frame, vector part, east share
    double   qy = 0.0;        // frame, vector part, upward share
    double   qz = 0.0;        // frame, vector part, north share
    double   qw = 0.0;        // frame, scalar part
    double   length = 0.0;    // metres (leaf: edge of the equal square; catkin and fruit stand: hang)
    double   radius = 0.0;    // metres (internode and root segment; else 0)
    double   bark = 0.0;      // share of the stem-base bark at its middle, 0..1 (internode, GODOT_KEY_BARK; else 0)
    double   tile_around = GODOT_TILE_NONE;  // tiles of the white bark around the tube (internode, tile port; else none)
    double   tile_along = GODOT_TILE_NONE;   // tiles of the white bark along the tube (internode, tile port; else none)
    double   tile_base = GODOT_TILE_NONE;    // tiles of the stem-base bark per white tile (internode, tile port; else none)
};

/**
 * One organ's instance as the last snapshot of its instance port showed it - a copy. The twelve values
 * are the plant-local 3x4 ase-render-data computed, in the row order of MultiMesh::set_buffer for
 * TRANSFORM_3D; the view hands them over unchanged and stands the plant at its patch as a scene node.
 * The organ record of the same object id names body and order.
 */
struct VivariumInstanceView {
    uint32_t object_id = 0;                    // the organ, as the plugin names it on its port
    float    rows[GODOT_INSTANCE_VALUES] = {};  // row 0 (east), row 1 (up), row 2 (north): x, y, z axis and origin
};

/** One unit as the bundle index names it. */
struct VivariumBundleUnit {
    std::string unit;         // unit name (ase-hub, ase-pl-flora, ...)
    std::string library;      // file name of its library
    std::string manifest;     // file name of its staged manifest
    std::string version;      // its version
    std::string api_version;  // the ASE API it was built against
};

/** What the bundle index says: the units, the data root and the data files below it. */
struct VivariumBundleInfo {
    std::string                     data_root;  // directory of the data files, relative to the stage
    std::vector<VivariumBundleUnit> units;      // every unit, in the index's order
    std::vector<std::string>        data;       // every data file, relative to the data root
};

/**
 * What one self-test run counted. A scenario that could not run to its end (no host, a unit not
 * loadable, a broken table entry) counts as one error of its own - a self-test that did not run
 * is never a self-test without errors.
 */
struct VivariumSelfTestResult {
    uint32_t cases = 0;    // scenarios run to their end, in order on the one test host
    uint32_t checks = 0;   // comparisons made: one per expected value, status and record count
    uint32_t errors = 0;   // failed comparisons, runs no action checked, and scenarios that could not run
};

/**
 * The state one scenario of the self-test carries from action to action: the values it
 * remembered for a later `plus`, the snapshot buffer, and the runs it checked.
 */
struct VivariumSelfTestState {
    std::unordered_map<std::string, double>     remembered;
    std::vector<ase::kernel::KernelHostRecord>  records;
    std::vector<std::string>                    checked;
    double                                      tolerance = 0.0;
    double                                      frame_s = 0.0;
};

/** One patch as the last snapshot showed it - a copy, never a reference into the App. */
struct VivariumPatchView {
    uint32_t object_id = 0;     // the patch, as the plugin names it on its port
    uint32_t place = 0;         // where the view draws it
    double   coverage = 0.0;    // covered share, 0..1
    uint8_t  stage = 0;         // GODOT_STAGE_* or GODOT_CODE_UNKNOWN
    uint8_t  condition = 0;     // GODOT_COND_* or GODOT_CODE_UNKNOWN
    uint8_t  soil = 0;          // GODOT_SOIL_* ground or GODOT_CODE_UNKNOWN
    double   soil_mm = 0.0;     // soil water, mm
    double   soil_rel = 0.0;    // soil water over field capacity, 0..1
    double   capacity_mm = 0.0; // field capacity, mm
    double   x = 0.0;           // stem base, world metres east: the centre of the patch square
    double   y = 0.0;           // stem base, world metres up: the height ase-terrain levelled the patch to
    double   z = 0.0;           // stem base, world metres north: the centre of the patch square
};

/** The game clock as the last snapshot showed it. */
struct VivariumClockView {
    bool   known = false;     // false until a snapshot carried the clock record
    double elapsed_s = 0.0;   // game time, seconds
    double day = 0.0;         // game day
    double hour = 0.0;        // hour of the game day
};

/**
 * The sky and the light of the scene as the last snapshot of the sky port showed them - a copy
 * (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3). ase-pl-sky computes all of it; the view sets its
 * directional light, its environment and its fog from these numbers and computes none of its own.
 * The sun and moon directions point TOWARDS their body in the world frame of the organ records (x east,
 * y up, z north); a directional light shines along its negative. Colours are shares 0..1. Every moon
 * record of the snapshot stands in one slot of `moon`, in the order the port carried them; `moons` says
 * how many - 0 while no moon stands in the sky.
 */
struct VivariumMoonView {
    double x = 0.0;           // unit direction towards the moon, east share
    double y = 0.0;           // unit direction towards the moon, upward share
    double z = 0.0;           // unit direction towards the moon, north share
    double r = 0.0;           // moonlight colour, red share
    double g = 0.0;           // moonlight colour, green share
    double b = 0.0;           // moonlight colour, blue share
    double intensity = 0.0;   // strength of its light as ase-pl-sky files it: CEL_MOON_CONTRIB for Amilox, 0 for Scorpii
    double size = 0.0;        // angular diameter of its disc, radians, as ase-ephemeris holds it
};

/**
 * A visible comet as the comet record of the sky port carries it (usecase:101274 - track the comets):
 * the unit direction towards it and the unit direction its tail runs on the sky, both in the world
 * frame, its brightness and the angular length of its tail. ase-pl-sky turns the tail away from the sun
 * and files the record only while the comet is visible and above the horizon.
 */
struct VivariumCometView {
    double x = 0.0;           // unit direction towards the comet, east share
    double y = 0.0;           // unit direction towards the comet, upward share
    double z = 0.0;           // unit direction towards the comet, north share
    double intensity = 0.0;   // brightness of the comet, 0..1 (EPH_COM_BRIGHTNESS)
    double tail_x = 0.0;      // unit direction the tail runs on the sky, east share
    double tail_y = 0.0;      // unit direction the tail runs on the sky, upward share
    double tail_z = 0.0;      // unit direction the tail runs on the sky, north share
    double length = 0.0;      // angular length of the tail on the sky, radians
};

/**
 * Luca, the neighbour planet, as the planet record of the sky port carries it (usecase:101273 - track the
 * neighbour planet): the unit direction towards it in the world frame, its angular diameter and its
 * illuminated share. ase-pl-sky files the record only while the planet stands above the horizon.
 */
struct VivariumPlanetView {
    double x = 0.0;           // unit direction towards the planet, east share
    double y = 0.0;           // unit direction towards the planet, upward share
    double z = 0.0;           // unit direction towards the planet, north share
    double size = 0.0;        // angular diameter of its disc, radians, as ase-ephemeris holds it
    double phase = 0.0;       // illuminated share of the disc, 0 new to 1 full
};

/**
 * Arcturus, the companion star, as the companion record of the sky port carries it (usecase:101272 - track
 * the companion star): the unit direction towards it in the world frame, its angular diameter and the
 * strength of its nebula glow. ase-pl-sky files the record only while it stands above the horizon.
 */
struct VivariumCompanionView {
    double x = 0.0;           // unit direction towards the companion star, east share
    double y = 0.0;           // unit direction towards the companion star, upward share
    double z = 0.0;           // unit direction towards the companion star, north share
    double size = 0.0;        // angular diameter of its disc, radians, as ase-ephemeris holds it
    double intensity = 0.0;   // strength of its nebula glow, 0..1
};

/**
 * A half of the split moon Aurora as a fragment record of the sky port carries it (usecase:101384 - place sun
 * and moon spatially): the unit direction towards it in the world frame, its angular diameter and its
 * brightness. ase-pl-sky files both halves while Aurora stands above the horizon.
 */
struct VivariumFragmentView {
    double x = 0.0;           // unit direction towards the moon half, east share
    double y = 0.0;           // unit direction towards the moon half, upward share
    double z = 0.0;           // unit direction towards the moon half, north share
    double size = 0.0;        // angular diameter of its disc, radians
    double intensity = 0.0;   // brightness of the half, 0..1
};

/**
 * The asteroid belt as the belt record of the sky port carries it (usecase:2758 - generated asteroids): the
 * unit direction towards the centre of its arc, the span of the arc along the orbit, the glitter of its grains
 * and their density. ase-pl-sky files the record only while the belt shines above the horizon.
 */
struct VivariumBeltView {
    double x = 0.0;           // unit direction towards the centre of the arc, east share
    double y = 0.0;           // unit direction towards the centre of the arc, upward share
    double z = 0.0;           // unit direction towards the centre of the arc, north share
    double span = 0.0;        // angle the arc spans along the orbit, radians
    double intensity = 0.0;   // glitter of the grains, 0..1
    double density = 0.0;     // share of the grains that show, 0..1
};

/**
 * The meteor shower as the shower record of the sky port carries it (usecase:2760 - show meteorite in the
 * nightsky): the unit direction towards its radiant, its intensity and the meteors it throws per hour.
 * ase-pl-sky files the record only while the shower throws and its radiant is above the horizon.
 */
struct VivariumShowerView {
    double x = 0.0;           // unit direction towards the radiant, east share
    double y = 0.0;           // unit direction towards the radiant, upward share
    double z = 0.0;           // unit direction towards the radiant, north share
    double intensity = 0.0;   // strength of the shower, 0..1
    double rate = 0.0;        // meteors per hour
};

struct VivariumSkyView {
    bool   known = false;          // false until a snapshot carried all four records of the sky port
    uint32_t showers = 0;          // shower records of the snapshot, at most GODOT_SKY_SHOWERS_MAX
    VivariumShowerView shower[GODOT_SKY_SHOWERS_MAX] = {};  // the meteor showers throwing above the horizon
    uint32_t belts = 0;            // belt records of the snapshot, at most GODOT_SKY_BELTS_MAX
    VivariumBeltView belt[GODOT_SKY_BELTS_MAX] = {};  // the asteroid belts shining above the horizon
    uint32_t fragments = 0;        // moon half records of the snapshot, at most GODOT_SKY_FRAGMENTS_MAX
    VivariumFragmentView fragment[GODOT_SKY_FRAGMENTS_MAX] = {};  // the halves of Aurora, in the order of the port
    uint32_t companions = 0;       // companion star records of the snapshot, at most GODOT_SKY_COMPANIONS_MAX
    VivariumCompanionView companion[GODOT_SKY_COMPANIONS_MAX] = {};  // the companion stars above the horizon
    uint32_t planets = 0;          // planet records of the snapshot, at most GODOT_SKY_PLANETS_MAX
    VivariumPlanetView planet[GODOT_SKY_PLANETS_MAX] = {};  // the planets above the horizon, in the order of the port
    uint32_t moons = 0;            // moon records of the snapshot, at most GODOT_SKY_MOONS_MAX
    VivariumMoonView moon[GODOT_SKY_MOONS_MAX] = {};  // the visible moons, in the order of the port
    uint32_t comets = 0;           // comet records of the snapshot, at most GODOT_SKY_COMETS_MAX
    VivariumCometView comet[GODOT_SKY_COMETS_MAX] = {};  // the visible comets, in the order of the port
    double sun_x = 0.0;            // unit direction towards the sun, east share
    double sun_y = 0.0;            // unit direction towards the sun, upward share (the sine of its elevation)
    double sun_z = 0.0;            // unit direction towards the sun, north share
    double sun_r = 0.0;            // sunlight colour, red share
    double sun_g = 0.0;            // sunlight colour, green share
    double sun_b = 0.0;            // sunlight colour, blue share
    double sun_intensity = 0.0;    // strength of the sunlight
    double sun_size = 0.0;         // angular diameter of the sun disc, radians (EPH_SUN_APPARENT_SIZE)
    double sun_disc_r = 0.0;       // colour of the sun disc, red share - the sunlight darkened by an eclipse (usecase:101276)
    double sun_disc_g = 0.0;       // colour of the sun disc, green share
    double sun_disc_b = 0.0;       // colour of the sun disc, blue share
    double ambient_r = 0.0;        // ambient light colour, red share
    double ambient_g = 0.0;        // ambient light colour, green share
    double ambient_b = 0.0;        // ambient light colour, blue share
    double ambient_intensity = 0.0; // strength of the ambient light
    double background_r = 0.0;     // colour of the sky behind the scene, red share
    double background_g = 0.0;     // colour of the sky behind the scene, green share
    double background_b = 0.0;     // colour of the sky behind the scene, blue share
    double fog_r = 0.0;            // fog colour, red share
    double fog_g = 0.0;            // fog colour, green share
    double fog_b = 0.0;            // fog colour, blue share
    double fog_near = 0.0;         // distance where the fog begins, metres
    double fog_far = 0.0;          // distance where the fog is full, metres
    double fog_shimmer = 0.0;      // shimmer of hot air, 0 none to 1 full (ATM_VIS_HEAT_HAZE; usecase:101515)
};

/**
 * One seed body as the last snapshot of the seed port showed it - a copy (register N0173, usecase:100711
 * publish the seed places). ase-physics moves the seed and sets it down on the ground; the place is
 * plant-local like an organ's (origin at the stem base, y up, x east, z north), and body names the patch the
 * plant stands on. The view draws the seed there and computes none of its flight.
 */
struct VivariumSeedView {
    uint32_t object_id = 0;   // the seed body, as the plugin names it on its port
    uint32_t body = 0;        // object id of the patch the plant stands on
    double   x = 0.0;         // metres east of the stem base
    double   y = 0.0;         // metres above the ground at the stem base
    double   z = 0.0;         // metres north of the stem base
};

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
     * Hand the host `elapsed_s` of real time (never capped, never dropped - the world has no
     * pause) and read both snapshots. The host ticks at most catch_up_steps times in this call;
     * what remains waits in its backlog for the next frame (KernelEmbeddedHost::advance).
     *
     * THE LAPSE OF THE DEBUG MODE: `lapse_s` is time the view hands in beyond the real time of the
     * frame, and `lapse_steps` the ticks it may run on top of catch_up_steps. The host ticks that time
     * like any other - ordinary ticks of its own clock, every system in every one, nothing of the
     * world skipped - only faster than the wall clock, as fast as the frame allows. 0 and 0 outside a
     * lapse. Both count into simulation_seconds: the host simulated them.
     */
    ase::kernel::HostStatus advance(double elapsed_s, double lapse_s, uint32_t lapse_steps);
    /** Pour irrigate_amount_mm on one patch; the plugin takes it in its next Reception run. */
    ase::kernel::HostStatus irrigate(uint32_t object_id);
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
     * Flatten a disc of the ground where the player tapped (03.8): a flatten request on the flatten port
     * with the centre `x`, `z` and the radius `radius_m`, metres; ase-pl-flora files it in its next
     * Reception run, ase-terrain levels the disc to the height of its centre.
     */
    ase::kernel::HostStatus flatten(double x, double z, double radius_m);
    /**
     * Run the host's own game clock at a time scale - the time lapse of the debug mode: a pace request on
     * the pace port, GODOT_PACE_NONE for the default scale back. ase-pl-flora writes it as
     * TIM_REQ_TIME_SCALE, ase-time runs this host's clock at it; no server clock is touched.
     */
    ase::kernel::HostStatus pace(double scale);
    /**
     * Tell the engine where the view's camera stands, world metres east, up and north - a view request on
     * the view port. ase-pl-flora files it for ase-camera, whose camera the level of detail of
     * ase-render-data measures from (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 point 7). Which place is valid
     * is the plugin's to check.
     */
    ase::kernel::HostStatus view(double x, double y, double z);
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
    [[nodiscard]] bool has_patch(uint32_t object_id) const;
    [[nodiscard]] uint32_t patch_count() const;
    [[nodiscard]] const VivariumClockView& get_clock() const;
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
    /** Real seconds the host has simulated: handed in, less what still waits in its backlog. */
    [[nodiscard]] double simulation_seconds() const;
    /** Ticks the host ran since boot, all calls together. */
    [[nodiscard]] uint64_t tick_count() const;
    /** Ticks of the last advance() and the real seconds still waiting after it. */
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
    ase::kernel::HostStatus read_terrain();
    ase::kernel::HostStatus read_templates();
    ase::kernel::HostStatus read_sky();
    ase::kernel::HostStatus read_seeds();
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
    VivariumClockView                                clock_;
    std::vector<VivariumOrganView>                   organs_[GODOT_ORGAN_COUNT];  // index = GODOT_ORGAN_*
    const std::vector<VivariumOrganView>             no_organs_;  // the answer for an unknown class: always empty
    std::vector<VivariumInstanceView>                instances_[GODOT_DRAWN_COUNT];  // index = GODOT_ORGAN_* before the root
    const std::vector<VivariumInstanceView>          no_instances_;  // the answer for a class not drawn: always empty
    std::vector<ase::kernel::KernelHostRecord>       organ_records_;  // snapshot buffer of the organ ports
    uint64_t                                         organ_generation_ = 0;
    uint64_t                                         organ_runs_ = 0;  // GODOT_ORGAN_SCHEDULE runs at the last organ read
    uint32_t                                         organ_reads_due_ = 0;  // reads still owed for the last new run
    std::vector<VivariumTerrainView>                 terrain_;  // the samples of the ground, last read
    std::vector<VivariumGroundCellView>              ground_cells_;  // the cells of the ground, read with the samples
    uint64_t                                         terrain_generation_ = 0;
    uint64_t                                         terrain_runs_ = 0;  // GODOT_TERRAIN_SCHEDULE runs at the last terrain read
    uint32_t                                         terrain_reads_due_ = 0;  // reads still owed for the last new run
    std::vector<VivariumTemplateCornerView>          template_corners_;    // the corners of the organ templates, every code
    std::vector<VivariumTemplateTriangleView>        template_triangles_;  // the triangles of the organ templates, every code
    uint64_t                                         template_generation_ = 0;
    VivariumSkyView                                  sky_;      // the sky and the light of the scene, last read
    std::vector<VivariumSeedView>                    seeds_;    // the seed bodies, last read
    const char*                                      failure_step_;
    ase::kernel::HostStatus                          failure_status_;
    double                                           elapsed_in_s_ = 0.0;
    double                                           backlog_s_ = 0.0;
    uint64_t                                         tick_count_ = 0;
    uint32_t                                         last_steps_ = 0;
    std::unique_ptr<ase::kernel::KernelEmbeddedHost> host_;
};

}  // namespace ase::adp::godot

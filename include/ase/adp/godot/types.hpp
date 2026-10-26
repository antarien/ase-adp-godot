#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the constants of the Godot adapter
 * @description Every value the adapter reads that does NOT change at runtime and that is NOT
 *              configuration: the value keys and codes of the host ports of ase-pl-flora, the
 *              table of contents of the staged bundle, the form of the self-test table, the names
 *              of the boot steps a failure is reported under, the GDExtension class binding of
 *              AseVivariumView (class names, the engine callbacks it takes over and their
 *              signature hashes), and its drawing style (colours, font sizes, line widths).
 *
 *              WHAT IS NOT HERE: positions, rectangles, texts, the pour amount and the port and
 *              operation names. They are the client's configuration and come from
 *              res://config/vivarium_start.json (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.4).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-05
 * @modified    2026-10-25
 * @version     00.00.09.00000
 *
 * WHY THE PORT KEYS ARE WRITTEN A SECOND TIME
 *
 * ase-pl-flora carries the same names (FLORA_HOST_KEY_* in its types.hpp). The adapter sees only
 * the kernel facade - no component, no hub, no unit header (ARCH_ASE_KERNEL_EMBEDDED.md,
 * "Schichten und Grenzen") - so it holds the DOCUMENTED contract of the ports
 * (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 point 1), exactly as
 * clients/avt-client-android/tests/avt_headless.cpp does. A renamed key breaks the host test of
 * this adapter, which runs against the real bundle.
 *
 * ABBREVIATIONS USED IN THIS ADAPTER:
 *   adp   adapter   third-party isolation module
 *   avt   Antares Vivarium, the client this adapter draws for
 *   tmpl  template  an organ template of ase-render-data
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * Host ports of ase-pl-flora - value keys (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 point 1). The
 * patch port carries one record per patch, its object id is the patch; the clock port one record.
 */
constexpr const char* GODOT_KEY_PLACE = "place";              // patch record: where the view draws the patch
constexpr const char* GODOT_KEY_COVERAGE = "coverage";        // patch record: covered share of the patch, 0..1
constexpr const char* GODOT_KEY_STAGE = "stage";              // patch record: growth stage code (PLT_STAGE)
constexpr const char* GODOT_KEY_CONDITION = "condition";      // patch record: condition code (PLT_CONDITION)
constexpr const char* GODOT_KEY_SOIL_MM = "soil_mm";          // patch record: soil water, mm (HYD_SOIL_WATER)
constexpr const char* GODOT_KEY_SOIL_REL = "soil_rel";        // patch record: soil water over field capacity, 0..1
constexpr const char* GODOT_KEY_CAPACITY_MM = "capacity_mm";  // patch record: field capacity, mm
constexpr const char* GODOT_KEY_AMOUNT_MM = "amount_mm";      // irrigate request: water poured on the patch, mm
constexpr const char* GODOT_KEY_ELAPSED = "elapsed_s";        // clock record: game time, seconds (TIM_ELAPSED_GAME)
constexpr const char* GODOT_KEY_DAY = "day";                  // clock record: game day (TIM_DAY_COUNT)
constexpr const char* GODOT_KEY_HOUR = "hour";                // clock record: hour of the game day (TIM_CURRENT_HOUR)
constexpr const char* GODOT_KEY_SOIL = "soil";                // patch record and lay request: ground code (GODOT_SOIL_*)
constexpr const char* GODOT_KEY_SPECIES = "species";          // lay request: catalog id of the species (KernelEmbeddedHost::name_id)
constexpr const char* GODOT_KEY_SEED = "seed";                // lay request: seed of the plant
constexpr const char* GODOT_KEY_BODY = "body";                // organ record: object id of the patch the plant stands on
constexpr const char* GODOT_KEY_ORDER = "order";              // organ record: branching order of its axis (stem 0)
constexpr const char* GODOT_KEY_X = "x";                      // organ record: metres east of the stem base; patch record: the stem base, world metres east; corner record: place on the unit
constexpr const char* GODOT_KEY_Y = "y";                      // organ record: metres above the ground at the stem base; patch record: the stem base, world metres up (TRN_FLAT_HGT); corner record: place along the unit
constexpr const char* GODOT_KEY_Z = "z";                      // organ record: metres north of the stem base; patch record: the stem base, world metres north; corner record: place on the unit
// An organ record carries the organ's frame in place of a direction: the unit quaternion ase-render-data
// builds from the direction and roll of ase-phytomer or ase-radix (hub_plnt_part_quat_comp.hpp). It turns
// the plant frame (y up) into the organ's: local y along the organ, local x along its roll axis.
constexpr const char* GODOT_KEY_QX = "qx";                    // organ record: frame, vector part, east share
constexpr const char* GODOT_KEY_QY = "qy";                    // organ record: frame, vector part, upward share
constexpr const char* GODOT_KEY_QZ = "qz";                    // organ record: frame, vector part, north share
constexpr const char* GODOT_KEY_QW = "qw";                    // organ record: frame, scalar part
constexpr const char* GODOT_KEY_LENGTH = "length";            // organ record: length, metres (leaf: edge of the equal square)
constexpr const char* GODOT_KEY_RADIUS = "radius";            // organ record: radius, metres (internode and root segment, else 0)
constexpr const char* GODOT_KEY_BARK = "bark";                // segment record only: share of the stem-base bark at the middle of the internode, 0 white to 1 base bark (usecase:100395)
constexpr double GODOT_BARK_WHITE = 0.0;                      // share of the stem-base bark of the white bark, the lower end of its range
constexpr double GODOT_BARK_BASE = 1.0;                       // share of the stem-base bark of the base bark whole, the upper end of its range
// The bark tiling of an internode (03.11 step 2, the material data of a section; usecase:100395), one record per
// internode with lenticels on the tile port, its object id the internode; ase-render-data computed all three.
constexpr const char* GODOT_KEY_TILE_AROUND = "around";       // tile record: tiles of the white bark around the tube
constexpr const char* GODOT_KEY_TILE_ALONG = "along";         // tile record: tiles of the white bark along the tube
constexpr const char* GODOT_KEY_TILE_BASE = "base";           // tile record: tiles of the stem-base bark per tile of the white bark
constexpr double GODOT_TILE_NONE = 1.0;                       // tiling of an internode without a tile record: the scale of the material itself

/**
 * The organ ports of ase-pl-flora (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.3): one port per organ class,
 * one record per organ, its object id the organ. The class is the index of every per-class store of
 * the adapter; its word names the port in the start configuration ("ports") and the record in the
 * self-test table - the same word in both places.
 */
constexpr uint32_t GODOT_ORGAN_SEGMENT = 0u;                  // internodes of every shoot
constexpr uint32_t GODOT_ORGAN_LEAF = 1u;                     // leaves
constexpr uint32_t GODOT_ORGAN_MALE = 2u;                     // male catkins
constexpr uint32_t GODOT_ORGAN_FEMALE = 3u;                   // female catkins
constexpr uint32_t GODOT_ORGAN_FRUIT = 4u;                    // fruit stands
constexpr uint32_t GODOT_ORGAN_ROOT = 5u;                     // root segments of every root
constexpr uint32_t GODOT_ORGAN_COUNT = 6u;                    // organ classes
constexpr const char* GODOT_ORGAN_WORD_SEGMENT = "segment";   // word of GODOT_ORGAN_SEGMENT
constexpr const char* GODOT_ORGAN_WORD_LEAF = "leaf";         // word of GODOT_ORGAN_LEAF
constexpr const char* GODOT_ORGAN_WORD_MALE = "male";         // word of GODOT_ORGAN_MALE
constexpr const char* GODOT_ORGAN_WORD_FEMALE = "female";     // word of GODOT_ORGAN_FEMALE
constexpr const char* GODOT_ORGAN_WORD_FRUIT = "fruit";       // word of GODOT_ORGAN_FRUIT
constexpr const char* GODOT_ORGAN_WORD_ROOT = "root";         // word of GODOT_ORGAN_ROOT
constexpr const char* GODOT_ORGAN_SCHEDULE = "Dissemination"; // schedule in which ase-phytomer and ase-radix change the organ rows
constexpr uint32_t GODOT_ORGAN_READS_PER_RUN = 2u;            // organ reads after a new run: that frame, and the next, in which the plugin's sync system (Reception) and export (Production) carry the run's rows to the ports

/**
 * The instance ports of ase-pl-flora (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2, build form of the
 * instance matrix): one port per DRAWN organ class - every class before the root, which lies in the
 * ground and is not drawn (03.1 point 4) - one record per organ, its object id the organ, the twelve
 * values of its plant-local 3x4 in the row order of MultiMesh::set_buffer. ase-render-data computes them;
 * the view copies them. The start configuration names the port under "<word>_instance".
 */
constexpr uint32_t GODOT_DRAWN_COUNT = 5u;                    // organ classes the view draws: GODOT_ORGAN_SEGMENT up to GODOT_ORGAN_FRUIT
constexpr const char* GODOT_INSTANCE_SUFFIX = "_instance";    // start configuration: ports.<word>_instance names the instance port of a class
constexpr uint32_t GODOT_INSTANCE_VALUES = 12u;               // values of one instance record: three rows of basis and origin
constexpr uint32_t GODOT_INSTANCE_CUSTOM_VALUES = 4u;         // custom data of one instance on the MultiMesh buffer after its twelve values: r = share of the stem-base bark, g b = tiles of the white bark around and along, a = tiles of the base bark per white tile
constexpr const char* GODOT_KEY_M00 = "m00";                  // instance record, row 0 (east): the template's x axis
constexpr const char* GODOT_KEY_M01 = "m01";                  // instance record, row 0 (east): the template's y axis
constexpr const char* GODOT_KEY_M02 = "m02";                  // instance record, row 0 (east): the template's z axis
constexpr const char* GODOT_KEY_M03 = "m03";                  // instance record, row 0 (east): the origin, metres
constexpr const char* GODOT_KEY_M10 = "m10";                  // instance record, row 1 (up): the template's x axis
constexpr const char* GODOT_KEY_M11 = "m11";                  // instance record, row 1 (up): the template's y axis
constexpr const char* GODOT_KEY_M12 = "m12";                  // instance record, row 1 (up): the template's z axis
constexpr const char* GODOT_KEY_M13 = "m13";                  // instance record, row 1 (up): the origin, metres
constexpr const char* GODOT_KEY_M20 = "m20";                  // instance record, row 2 (north): the template's x axis
constexpr const char* GODOT_KEY_M21 = "m21";                  // instance record, row 2 (north): the template's y axis
constexpr const char* GODOT_KEY_M22 = "m22";                  // instance record, row 2 (north): the template's z axis
constexpr const char* GODOT_KEY_M23 = "m23";                  // instance record, row 2 (north): the origin, metres

/**
 * The ground of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8): where a patch lies, the measured
 * square of terrain and the brook are data of the start configuration; ase-pl-flora takes them on its
 * scene, ground and brook ports and shows the measured heights on its terrain port, one record per
 * sample, its object id the sample. World positions are metres, x east, z north - the same keys as
 * the organ records, here in the world instead of plant-local. The adapter computes none of it.
 */
constexpr const char* GODOT_KEY_HALF = "half_m";              // lay requests: half edge of a patch or ground square, half width of a bed; patch record: half edge
constexpr const char* GODOT_KEY_STEP = "step_m";              // ground lay request: distance of two terrain samples, metres
constexpr const char* GODOT_KEY_END_X = "end_x";              // brook lay request: east end of the bed, world metres
constexpr const char* GODOT_KEY_END_Z = "end_z";              // brook lay request: north end of the bed, world metres
constexpr const char* GODOT_KEY_DEPTH = "depth_m";            // brook lay request: depth of the bed below the ground, metres
constexpr const char* GODOT_KEY_BASE = "base_m3_s";           // brook lay request: base flow of the brook without rain, m3/s (ase-hydro)
constexpr const char* GODOT_KEY_CATCHMENT = "catchment_m2";   // brook lay request: area whose rain runs into the brook, m2 (ase-hydro)
constexpr const char* GODOT_KEY_HEIGHT = "height";            // terrain record: ground height at the sample, metres
constexpr const char* GODOT_KEY_NX = "nx";                    // terrain record: unit normal of the ground, east share (ase-render-data); corner record: its normal, x share
constexpr const char* GODOT_KEY_NY = "ny";                    // terrain record: unit normal of the ground, upward share; corner record: its normal, y share
constexpr const char* GODOT_KEY_NZ = "nz";                    // terrain record: unit normal of the ground, north share; corner record: its normal, z share
constexpr const char* GODOT_KEY_U = "u";                      // terrain record: texture coordinate, world metres east; corner record: around or across, 0..1
constexpr const char* GODOT_KEY_V = "v";                      // terrain record: texture coordinate, world metres north; corner record: along, 0..1
constexpr const char* GODOT_KEY_SW = "sw";                    // ground cell record: object id of the south-west corner sample
constexpr const char* GODOT_KEY_SE = "se";                    // ground cell record: object id of the south-east corner sample
constexpr const char* GODOT_KEY_NE = "ne";                    // ground cell record: object id of the north-east corner sample
constexpr const char* GODOT_KEY_NW = "nw";                    // ground cell record: object id of the north-west corner sample
constexpr const char* GODOT_TERRAIN_SCHEDULE = "Dissemination"; // schedule in which ase-terrain publishes changed sample heights
constexpr uint32_t GODOT_TERRAIN_READS_PER_RUN = 2u;          // terrain reads after a new run: that frame and the next, as for the organs

/**
 * The organ templates (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2, usecase:101011 tessellate the organ templates):
 * ase-render-data builds the fine and the coarse cylinder and the unit leaf on the unit, ase-pl-flora lays
 * their corners on one port and their triangles on another, each record with the code of its template. The
 * codes and keys are the plugin's (ase-pl-flora types.hpp, FLORA_HOST_TMPL_*, FLORA_HOST_KEY_*), repeated
 * here because the adapter reads the port and never links the plugin. A corner record carries the code,
 * its index and x, y, z, nx, ny, nz, u, v; a triangle record the code and three corner indices, clockwise
 * seen from outside. The adapter builds no ring and decides no winding.
 */
constexpr const char* GODOT_KEY_TEMPLATE = "template";        // template records: the code of the template, GODOT_TMPL_*
constexpr const char* GODOT_KEY_INDEX = "index";              // corner record: its place in the template, named by the triangles
constexpr const char* GODOT_KEY_CORNER_A = "a";               // triangle record: index of its first corner
constexpr const char* GODOT_KEY_CORNER_B = "b";               // triangle record: index of its second corner
constexpr const char* GODOT_KEY_CORNER_C = "c";               // triangle record: index of its third corner
constexpr uint32_t GODOT_TMPL_FINE = 0u;                      // template code: the fine cylinder, the internodes up to the shown orders
constexpr uint32_t GODOT_TMPL_CORS = 1u;                      // template code: the coarse cylinder, the internodes above them
constexpr uint32_t GODOT_TMPL_LEAF = 2u;                      // template code: the unit leaf, the stand-in of a leaf or a seed whose template file is missing
constexpr uint32_t GODOT_TMPL_COUNT = 3u;                     // template codes the ports carry

/**
 * The sky and the light of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3): ase-pl-sky
 * lays four records on its scene port every frame (SkyHostScneExptSystem), one per object id below.
 * The sun record carries the unit direction TOWARDS the sun in the world frame of the organ records
 * (x east, y up, z north) - a directional light shines along its negative; colours are shares 0..1.
 * The ids and keys are the plugin's (ase-pl-sky types.hpp, SKY_HOST_*), repeated here because the
 * adapter reads the port and never links the plugin. The adapter computes none of the light.
 */
constexpr uint32_t GODOT_SKY_RECORD_SUN = 1u;                 // sky record object id: the sun - direction towards it, colour, strength
constexpr uint32_t GODOT_SKY_RECORD_AMBIENT = 2u;             // sky record object id: the ambient light - colour, strength
constexpr uint32_t GODOT_SKY_RECORD_BACKGROUND = 3u;          // sky record object id: the background of the sky - colour
constexpr uint32_t GODOT_SKY_RECORD_FOG = 4u;                 // sky record object id: the fog - colour, near and far distance
constexpr uint32_t GODOT_SKY_RECORD_COUNT = 4u;               // records of one sky publication: sun, ambient, background, fog
constexpr const char* GODOT_KEY_R = "r";                      // sky records: red share of the colour, 0..1
constexpr const char* GODOT_KEY_G = "g";                      // sky records: green share of the colour, 0..1
constexpr const char* GODOT_KEY_B = "b";                      // sky records: blue share of the colour, 0..1
constexpr const char* GODOT_KEY_INTENSITY = "intensity";      // sun and ambient record: strength of the light
constexpr const char* GODOT_KEY_NEAR = "near";                // fog record: distance where the fog begins, metres
constexpr const char* GODOT_KEY_FAR = "far";                  // fog record: distance where the fog is full, metres

/**
 * Plant geometry (godot_plant_geometry.hpp): the template, ground and instance records go unchanged into the
 * arrays the instances are drawn with - no organ and no template is computed here.
 */
constexpr float GODOT_GEOMETRY_EPSILON = 1.0e-6f;   // a length or extent shorter than this counts as none
constexpr float GODOT_GEOMETRY_UPRIGHT = 0.99f;     // |up share| above which a direction counts as upright: its frame is taken against east, not against up
constexpr double GODOT_ORDER_EVERY = -1.0;          // split order below the stem's (0): every organ of a class lies above it

/**
 * The 3D scene of the care view (godot_plant_scene.hpp, PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2):
 * the names of its nodes and the keys of the "view" section of the start configuration - the near
 * view and the visible orders by frame time (03.6, V8) are numbers of that section. Every plant stands as
 * one node per body (GODOT_PLANT_BODY_PREFIX and the patch's object id) under GODOT_PLANT_BODIES, its six
 * instance nodes under it.
 */
constexpr const char* GODOT_PLANT_SCENE_ROOT = "PlantScene";      // name of the scene's root node
constexpr const char* GODOT_PLANT_CAMERA = "PlantCamera";         // the camera of the near view
constexpr const char* GODOT_PLANT_SUN = "PlantSun";               // the directional light the sun of the sky port sets
constexpr const char* GODOT_PLANT_ENVIRONMENT = "PlantEnvironment";  // sky colour, ambient light and fog of the sky port
constexpr const char* GODOT_PLANT_GROUND = "PlantGround";         // the ground from the terrain samples
constexpr const char* GODOT_PLANT_BODIES = "PlantBodies";         // the node the plants stand under, one child per body
constexpr const char* GODOT_PLANT_BODY_PREFIX = "PlantBody";      // name of a plant's node: this and the object id of its patch
constexpr const char* GODOT_PLANT_TUBES = "PlantTubes";           // the internodes up to the shown orders, on the fine cylinder
constexpr const char* GODOT_PLANT_TWIGS = "PlantTwigs";           // the internodes above them, on the coarse cylinder
constexpr const char* GODOT_PLANT_LEAVES = "PlantLeaves";         // the leaves as instances
constexpr const char* GODOT_PLANT_MALE = "PlantMale";             // the male catkins as instances
constexpr const char* GODOT_PLANT_FEMALE = "PlantFemale";         // the female catkins as instances
constexpr const char* GODOT_PLANT_FRUIT = "PlantFruit";           // the fruit stands as instances
constexpr const char* GODOT_PLANT_SEEDS = "PlantSeeds";           // the seed bodies of ase-physics as instances, in world places (seed port)
constexpr const char* GODOT_VIEW_KEY = "view";                    // start configuration: the section of the 3D scene
constexpr const char* GODOT_VIEW_KEY_DISTANCE = "camera_distance_m";  // view: metres from the focused stem base to the camera, along the ground
constexpr const char* GODOT_VIEW_KEY_HEIGHT = "camera_height_m";  // view: metres of the camera above the focused stem base
constexpr const char* GODOT_VIEW_KEY_LOOK = "camera_look_m";      // view: metres above the stem base the camera looks at
constexpr const char* GODOT_VIEW_KEY_FOV = "camera_fov_deg";      // view: vertical field of view, degrees
constexpr const char* GODOT_VIEW_KEY_BUDGET = "frame_budget_ms";  // view: frame time the scene may take (OrderBudget)
constexpr const char* GODOT_VIEW_KEY_HEADROOM = "headroom";       // view: share of the budget below which a frame is calm
constexpr const char* GODOT_VIEW_KEY_CEILING = "order_ceiling";   // view: most orders ever drawn on the fine cylinder
constexpr const char* GODOT_VIEW_KEY_START = "order_start";       // view: orders drawn on the fine cylinder at the start
constexpr const char* GODOT_VIEW_KEY_CALM = "calm_frames";        // view: calm frames in a row that give one order more
constexpr const char* GODOT_VIEW_KEY_OVER = "over_frames";        // view: over frames in a row that take one order back
constexpr const char* GODOT_VIEW_KEY_PICK = "pick_px";            // view: logical pixels around a tap within which the middle of an internode is hit (cutting mode, 03.10 step 9)
constexpr const char* GODOT_VIEW_KEY_FLATTEN = "flatten_m";       // view: radius of the disc one tap in the flatten mode levels, metres (03.8)
/**
 * The exposure of the scene camera (register D1721): the eye adapts to the night, a fixed camera does not. The
 * sky port hands the light as ase-pl-sky computes it - at night the sun light 0.05 and the ambient
 * 0.02/0.02/0.06 at intensity 0.15 (ATM_CONST_NIGHT_AMBIENT_*), by day at noon the sun 0.95 and the ambient
 * 0.7 (run 20261025-phase03-v46) - and the view keeps those numbers. It lifts the camera's exposure by the
 * square root of the ratio of GODOT_EXPOSURE_DAY_LIGHT to the light of the scene (scene_exposure), the partial
 * adaptation of an eye, never below 1 and at most
 * GODOT_EXPOSURE_LIFT_MAX, as an eye adapts to starlight: the night stays darker than the day, and the ground
 * and the birch stay to be seen. The multiplier is the camera's, so both renderers, mobile and Compatibility,
 * take it.
 */
constexpr float GODOT_EXPOSURE_DAY_LIGHT = 1.0f;   // light of the scene that needs no lift: the sun at noon plus its ambient share
constexpr float GODOT_EXPOSURE_LIFT_MAX = 32.0f;   // largest lift of the exposure: the night stays at least this much darker than the day's light
constexpr float GODOT_ZOOM_START = 1.0f;   // zoom of the near view at the start: the camera as the view section places it
constexpr float GODOT_ZOOM_STEP = 0.8f;    // one wheel turn or pinch towards the birch: distance, height and look point times this
constexpr float GODOT_ZOOM_MIN = 0.08f;    // closest zoom: 0.8 m from a seedling at the configured 10 m
constexpr float GODOT_ZOOM_MAX = 1.6f;     // farthest zoom: the whole ground square in sight
constexpr float GODOT_TUBE_MID = 0.5f;                            // middle of the unit tube template (y from 0 to 1): where a tap picks an internode
constexpr double GODOT_MS_PER_SECOND = 1000.0;                    // the order budget takes frame times in milliseconds
constexpr const char* GODOT_ROLE_BARK = "bark_white";             // material role of the white bark of the tubes and twigs
constexpr const char* GODOT_ROLE_BARK_BASE = "bark_base";         // material role of the stem-base bark, mixed in by the share each internode carries
constexpr const char* GODOT_ROLE_LEAF = "leaf";                   // template role of a leaf
constexpr const char* GODOT_ROLE_MALE = "male";                   // template role of a male catkin
constexpr const char* GODOT_ROLE_FEMALE = "female";               // template role of a female catkin
constexpr const char* GODOT_ROLE_FRUIT = "fruit";                 // template role of a fruit stand
constexpr const char* GODOT_ROLE_NUTLET = "nutlet";               // template role of a winged nutlet, built in metres (ase-assgen birch_nutlet)
/**
 * The stand-in of a seed while the nutlet template is missing: the unit leaf at the width of a birch nutlet with
 * both wings, ϑ OTL/NUT/WIDTH of DSGN_024 (ONWInit 0.004 m; BSBI Plant Crib: "total width of nutlet with wings
 * 3-5 mm"). A bound template carries its own size and is drawn at scale one.
 */
constexpr float GODOT_NUTLET_STAND_IN_M = 0.004f;                 // m - width the stand-in leaf of a seed is drawn at

/**
 * Asset binding (godot_asset_binding.hpp, PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2).
 */
constexpr uint32_t GODOT_SHA256_HEX_DIGITS = 64u;                            // hex digits of one SHA-256 checksum
constexpr const char* GODOT_ASSET_MISSING = "asset missing ";               // boot line of a name without its asset, the name follows
constexpr const char* GODOT_ASSETS_PATH = "res://config/vivarium_assets.json";  // asset list of the play stages, staged beside the start configuration
constexpr const char* GODOT_ASSETS_KEY_MANIFEST = "manifest";               // asset list: res:// path of the manifest of the asset set
constexpr const char* GODOT_ASSETS_KEY_ROOT = "asset_root";                 // asset list: res:// directory the manifest's files lie in
constexpr const char* GODOT_ASSETS_KEY_STAGES = "stages";                   // asset list: one entry per play stage
constexpr const char* GODOT_ASSETS_KEY_STAGE = "stage";                     // stage entry: the play stage it lists
constexpr const char* GODOT_ASSETS_KEY_MATERIALS = "materials";             // stage entry: role → material of the list
constexpr const char* GODOT_ASSETS_KEY_TEMPLATES = "templates";             // stage entry: role → organ template of the list
constexpr const char* GODOT_ASSETS_KEY_ENTRIES = "assets";                  // manifest: its entries (ase-web-curator scripts/export-avt-assets.cjs)
constexpr const char* GODOT_ASSETS_KEY_ASSET = "name";                      // manifest entry: which asset of the list it delivers
constexpr const char* GODOT_ASSETS_KEY_FILE = "file";                       // manifest entry: its file below the asset root
constexpr const char* GODOT_ASSETS_KEY_SHA256 = "sha256";                   // manifest entry: SHA-256 of that file
constexpr const char* GODOT_ASSETS_COMMENT = "_";                           // a key of the list starting with it is a comment, never a role
constexpr const char* GODOT_ROLE_GROUND_MEADOW = "ground_meadow";           // material role of meadow soil (GODOT_SOIL_MEADOW)
constexpr const char* GODOT_ROLE_GROUND_FOREST = "ground_forest";           // material role of forest floor (GODOT_SOIL_FOREST)
constexpr uint32_t GODOT_COLOR_ASSET_MISSING = 0xFF00FFFFu;                 // signal colour of a name the asset list gives no stand-in colour
constexpr const char* GODOT_GLTF_KEY_IMAGES = "images";                     // glTF document: its image list, in the order GLTFState::get_images holds them
constexpr const char* GODOT_GLTF_KEY_IMAGE = "name";                        // glTF image: which map of ase-assgen it is (assgen_kit.save_map)
constexpr const char* GODOT_IMAGE_TRANSLUCENCY = "_translucency";           // glTF image ending of the light a thin organ lets through (leaf.py, fruit.py, grass.py), bound as backlight
constexpr const char* GODOT_SHADER_SOIL_REL = "soil_rel";                   // instance uniform of the ground material: soil water over field capacity of the focused patch, 0..1
constexpr const char* GODOT_SHADER_CONDITION = "condition";                 // instance uniform of the leaf material: GODOT_COND_* of the plant's patch

/**
 * The stand-in material (godot_plant_look.cpp, operator order of 2026-10-23): a role whose file is missing is
 * drawn in its GODOT_COLOR_STAND_IN_* colour through this shader, so the scene reads as meadow, bark and leaf
 * before Phase 04 delivers a file. The ground darkens with the soil water of the focused patch
 * (GODOT_SHADER_SOIL_REL, -1 while unknown), a leaf takes the wilted or the dead colour by the condition of
 * its patch (GODOT_SHADER_CONDITION). Both faces are drawn: the stand-in leaf is a card without thickness.
 */
constexpr const char* GODOT_STAND_IN_SHADER_CODE =
    "shader_type spatial;\n"
    "render_mode cull_disabled;\n"
    "uniform vec4 stand_in_color : source_color = vec4(1.0);\n"
    "uniform vec4 wilted_color : source_color = vec4(1.0);\n"
    "uniform vec4 dead_color : source_color = vec4(1.0);\n"
    "uniform int wilted_code = 1;\n"
    "uniform int dead_code = 2;\n"
    "uniform float wet_shade = 1.0;\n"
    "uniform float stand_in_roughness = 1.0;\n"
    "instance uniform float soil_rel = -1.0;\n"
    "instance uniform float condition = 0.0;\n"
    "void fragment() {\n"
    "    vec3 color = stand_in_color.rgb;\n"
    "    if (soil_rel >= 0.0) {\n"
    "        color *= mix(1.0, wet_shade, clamp(soil_rel, 0.0, 1.0));\n"
    "    }\n"
    "    int code = int(round(condition));\n"
    "    if (code == dead_code) {\n"
    "        color = dead_color.rgb;\n"
    "    } else if (code == wilted_code) {\n"
    "        color = wilted_color.rgb;\n"
    "    }\n"
    "    ALBEDO = color;\n"
    "    ROUGHNESS = stand_in_roughness;\n"
    "}\n";
constexpr const char* GODOT_STAND_IN_COLOR = "stand_in_color";            // stand-in shader uniform: the colour of the name
constexpr const char* GODOT_STAND_IN_WILTED = "wilted_color";             // stand-in shader uniform: colour of a wilted stand
constexpr const char* GODOT_STAND_IN_DEAD = "dead_color";                 // stand-in shader uniform: colour of a dead stand
constexpr const char* GODOT_STAND_IN_WILTED_CODE = "wilted_code";         // stand-in shader uniform: GODOT_COND_WILTED
constexpr const char* GODOT_STAND_IN_DEAD_CODE = "dead_code";             // stand-in shader uniform: GODOT_COND_DEAD
constexpr const char* GODOT_STAND_IN_WET = "wet_shade";                   // stand-in shader uniform: GODOT_STAND_IN_WET_SHADE
constexpr const char* GODOT_STAND_IN_ROUGH = "stand_in_roughness";        // stand-in shader uniform: GODOT_STAND_IN_ROUGHNESS
constexpr float GODOT_STAND_IN_WET_SHADE = 0.55f;                         // share of its colour the ground keeps at field capacity: wet earth is darker
constexpr float GODOT_STAND_IN_ROUGHNESS = 0.9f;                          // roughness of every stand-in surface: matt, no texture to carry a sheen
constexpr uint32_t GODOT_COLOR_STAND_IN_BARK = 0xE6E1D3FFu;               // stand-in of GODOT_ROLE_BARK: the white bark of the birch
constexpr uint32_t GODOT_COLOR_STAND_IN_BARK_BASE = 0x3E3630FFu;          // stand-in of GODOT_ROLE_BARK_BASE: the dark cracked bark of the stem base
constexpr uint32_t GODOT_COLOR_STAND_IN_MEADOW = 0x587A34FFu;             // stand-in of GODOT_ROLE_GROUND_MEADOW: grass over earth
constexpr uint32_t GODOT_COLOR_STAND_IN_FOREST = 0x5B4632FFu;             // stand-in of GODOT_ROLE_GROUND_FOREST: leaf litter over earth
constexpr uint32_t GODOT_COLOR_STAND_IN_LEAF = 0x5E8F32FFu;               // stand-in of GODOT_ROLE_LEAF: a fresh birch leaf
constexpr uint32_t GODOT_COLOR_STAND_IN_MALE = 0xA38A4CFFu;               // stand-in of GODOT_ROLE_MALE: a male catkin
constexpr uint32_t GODOT_COLOR_STAND_IN_FEMALE = 0x7D8C3FFFu;             // stand-in of GODOT_ROLE_FEMALE: a female catkin
constexpr uint32_t GODOT_COLOR_STAND_IN_FRUIT = 0x8A6A3EFFu;              // stand-in of GODOT_ROLE_FRUIT: a ripe fruit stand
constexpr uint32_t GODOT_COLOR_STAND_IN_NUTLET = 0x9C7A44FFu;             // stand-in of GODOT_ROLE_NUTLET: a winged nutlet

/**
 * The ground of the care view (usecase:100387 view the plant, operator order of 2026-10-25: "ein Terrain
 * hat je nach Schräglage andere Farben ... wo sieht man die Gitternetzlinien"). One ShaderMaterial per soil
 * over the whole terrain mesh, its cover the material of the soil's role (godot_plant_look.cpp):
 * - THE SLOPE PAINTS: the world normal of a fragment gives its slope; above GODOT_GROUND_SLOPE_BANK_DEG the
 *   cover gives way to bare earth, above GODOT_GROUND_SLOPE_STEEP_DEG to the colour of an open bank.
 * - THE GROUND STATE PAINTS: the vertex colour mark_ground lays (godot_plant_geometry.hpp) - red where
 *   ase-terrain levelled the disc of a patch, which turns to bare soil, green where a brook's bed runs,
 *   which turns to wet gravel. A Nivelar the player taps changes the terrain port and with it the colour.
 * - THE GRID SHOWS THE SHAPE: thin lines along x and z at the distance of two height samples (the edges of
 *   the terrain cells), a stronger one every GODOT_GROUND_GRID_MAJOR cells, and contour lines every
 *   GODOT_GROUND_CONTOUR_M of height, so slopes, the levelled disc and the banks of the bed read as relief.
 * The grain of the cover is value noise over the world place, so a meadow is no flat colour before Phase 04
 * delivers its texture; the soil water of the focused patch darkens the ground as the stand-in did.
 */
constexpr const char* GODOT_GROUND_SHADER_CODE =
    "shader_type spatial;\n"
    "render_mode cull_back;\n"
    "uniform vec4 cover_color : source_color = vec4(1.0);\n"
    "uniform sampler2D cover_albedo : source_color, filter_linear_mipmap, repeat_enable, hint_default_white;\n"
    "uniform vec2 cover_uv = vec2(1.0);\n"
    "uniform vec4 soil_color : source_color = vec4(1.0);\n"
    "uniform vec4 bank_color : source_color = vec4(1.0);\n"
    "uniform vec4 bed_color : source_color = vec4(1.0);\n"
    "uniform vec4 grid_color : source_color = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "uniform float slope_bank = 0.0;\n"
    "uniform float slope_steep = 0.0;\n"
    "uniform float grid_step = 1.0;\n"
    "uniform float grid_major = 1.0;\n"
    "uniform float contour_step = 1.0;\n"
    "uniform float grid_weight = 0.0;\n"
    "uniform float major_weight = 0.0;\n"
    "uniform float contour_weight = 0.0;\n"
    "uniform float wet_shade = 1.0;\n"
    "uniform float ground_roughness = 1.0;\n"
    "instance uniform float soil_rel = -1.0;\n"
    "varying vec3 world_pos;\n"
    "varying vec3 world_normal;\n"
    "float grain_hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }\n"
    "float grain_at(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    vec2 u = f * f * (3.0 - 2.0 * f);\n"
    "    return mix(mix(grain_hash(i), grain_hash(i + vec2(1.0, 0.0)), u.x),\n"
    "               mix(grain_hash(i + vec2(0.0, 1.0)), grain_hash(i + vec2(1.0, 1.0)), u.x), u.y);\n"
    "}\n"
    "float line_at(float c, float width) {\n"
    "    float d = abs(fract(c - 0.5) - 0.5) / max(width, 0.00001);\n"
    "    return 1.0 - clamp(d, 0.0, 1.0);\n"
    "}\n"
    "void vertex() {\n"
    "    world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;\n"
    "    world_normal = (MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz;\n"
    "}\n"
    "void fragment() {\n"
    "    float slope = 1.0 - clamp(normalize(world_normal).y, 0.0, 1.0);\n"
    "    float grain = grain_at(world_pos.xz * 1.3) * 0.6 + grain_at(world_pos.xz * 6.1) * 0.4;\n"
    "    vec3 cover = cover_color.rgb * texture(cover_albedo, UV * cover_uv).rgb * mix(0.78, 1.18, grain);\n"
    "    vec3 soil = soil_color.rgb * mix(0.85, 1.12, grain_at(world_pos.xz * 4.7));\n"
    "    vec3 bank = bank_color.rgb * mix(0.85, 1.12, grain);\n"
    "    vec3 bed = bed_color.rgb * mix(0.8, 1.1, grain_at(world_pos.xz * 9.0));\n"
    "    vec3 color = mix(cover, soil, smoothstep(slope_bank * 0.5, slope_bank, slope));\n"
    "    color = mix(color, bank, smoothstep(slope_steep * 0.7, slope_steep, slope));\n"
    "    color = mix(color, soil, COLOR.r);\n"
    "    color = mix(color, bed, COLOR.g);\n"
    "    if (soil_rel >= 0.0) {\n"
    "        color *= mix(1.0, wet_shade, clamp(soil_rel, 0.0, 1.0));\n"
    "    }\n"
    "    vec2 cell = world_pos.xz / grid_step;\n"
    "    vec2 block = world_pos.xz / (grid_step * grid_major);\n"
    "    float level = world_pos.y / contour_step;\n"
    "    float minor = max(line_at(cell.x, fwidth(cell.x)), line_at(cell.y, fwidth(cell.y)));\n"
    "    float major = max(line_at(block.x, fwidth(block.x)), line_at(block.y, fwidth(block.y)));\n"
    "    float contour = line_at(level, fwidth(level));\n"
    "    float lines = max(max(minor * grid_weight, major * major_weight), contour * contour_weight);\n"
    "    ALBEDO = mix(color, grid_color.rgb, lines);\n"
    "    ROUGHNESS = ground_roughness;\n"
    "}\n";
constexpr const char* GODOT_GROUND_COVER_COLOR = "cover_color";            // ground shader uniform: colour of the soil's cover (its role)
constexpr const char* GODOT_GROUND_COVER_ALBEDO = "cover_albedo";          // ground shader uniform: albedo texture of the cover, white while none is bound
constexpr const char* GODOT_GROUND_COVER_UV = "cover_uv";                  // ground shader uniform: UV scale of the cover
constexpr const char* GODOT_GROUND_SOIL = "soil_color";                    // ground shader uniform: bare soil of levelled ground and of a slope
constexpr const char* GODOT_GROUND_BANK = "bank_color";                    // ground shader uniform: an open bank
constexpr const char* GODOT_GROUND_BED = "bed_color";                      // ground shader uniform: the wet gravel of a brook's bed
constexpr const char* GODOT_GROUND_GRID = "grid_color";                    // ground shader uniform: colour of grid and contour lines
constexpr const char* GODOT_GROUND_SLOPE_BANK = "slope_bank";              // ground shader uniform: GODOT_GROUND_SLOPE_BANK_SHARE
constexpr const char* GODOT_GROUND_SLOPE_STEEP = "slope_steep";            // ground shader uniform: GODOT_GROUND_SLOPE_STEEP_SHARE
constexpr const char* GODOT_GROUND_GRID_STEP = "grid_step";                // ground shader uniform: distance of two height samples, metres (the ground's step_m)
constexpr const char* GODOT_GROUND_GRID_MAJOR_KEY = "grid_major";          // ground shader uniform: GODOT_GROUND_GRID_MAJOR
constexpr const char* GODOT_GROUND_CONTOUR = "contour_step";               // ground shader uniform: GODOT_GROUND_CONTOUR_M
constexpr const char* GODOT_GROUND_GRID_WEIGHT = "grid_weight";            // ground shader uniform: GODOT_GROUND_GRID_SHARE
constexpr const char* GODOT_GROUND_MAJOR_WEIGHT = "major_weight";          // ground shader uniform: GODOT_GROUND_MAJOR_SHARE
constexpr const char* GODOT_GROUND_CONTOUR_WEIGHT = "contour_weight";      // ground shader uniform: GODOT_GROUND_CONTOUR_SHARE
constexpr const char* GODOT_GROUND_WET = "wet_shade";                      // ground shader uniform: GODOT_STAND_IN_WET_SHADE
constexpr const char* GODOT_GROUND_ROUGH = "ground_roughness";             // ground shader uniform: roughness of the cover
constexpr uint32_t GODOT_COLOR_GROUND_BANK = 0x7D6A52FFu;                  // an open bank: loam and gravel where the slope carries no grass
constexpr uint32_t GODOT_COLOR_GROUND_BED = 0x4B4C45FFu;                   // the bed of a brook: wet gravel
constexpr uint32_t GODOT_COLOR_GROUND_GRID = 0x10140CFFu;                  // grid and contour lines: near black, read on grass and on soil
constexpr float GODOT_GROUND_SLOPE_BANK_DEG = 18.0f;                       // slope above which the cover gives way to bare earth, degrees
constexpr float GODOT_GROUND_SLOPE_BANK_SHARE = 0.048943f;                 // 1 - cos(GODOT_GROUND_SLOPE_BANK_DEG): the shader compares 1 - normal.y with it
constexpr float GODOT_GROUND_SLOPE_STEEP_DEG = 35.0f;                      // slope above which the ground is an open bank, degrees
constexpr float GODOT_GROUND_SLOPE_STEEP_SHARE = 0.180848f;                // 1 - cos(GODOT_GROUND_SLOPE_STEEP_DEG)
constexpr float GODOT_GROUND_GRID_MAJOR = 4.0f;                            // cells between two strong grid lines: 2 m at a sample step of 0.5 m
constexpr float GODOT_GROUND_CONTOUR_M = 0.1f;                             // height between two contour lines, metres: the 0.4 m bed shows four
constexpr float GODOT_GROUND_GRID_SHARE = 0.28f;                           // share of the line colour on a cell edge
constexpr float GODOT_GROUND_MAJOR_SHARE = 0.55f;                          // share of the line colour on a strong grid line
constexpr float GODOT_GROUND_CONTOUR_SHARE = 0.45f;                        // share of the line colour on a contour line
constexpr float GODOT_GROUND_LEVEL_TOLERANCE_M = 0.01f;                    // a vertex within this of a disc's levelled height is levelled ground (mark_ground)

/**
 * The bark of the internodes (usecase:100395 age the bark, DSGN_024): one material for the tubes and
 * twigs that mixes the white bark with the stem-base bark by the share each instance carries in its
 * custom data (INSTANCE_CUSTOM.r, GODOT_INSTANCE_CUSTOM_VALUES). Its uniforms take the colour, the
 * albedo, normal and roughness textures, the roughness and the UV scale of the two bound materials
 * (GODOT_ROLE_BARK, GODOT_ROLE_BARK_BASE); a texture a material lacks samples white, a flat normal or
 * full roughness. The tiling of the section stands in the rest of the custom data (INSTANCE_CUSTOM.gba,
 * computed by ase-render-data): the texture coordinates of the unit cylinder are multiplied by the tiles of
 * the white bark around and along the tube, and those of the base bark once more by its tiles per white
 * tile. An internode without a tile record carries GODOT_TILE_NONE, the scale of the material itself.
 */
constexpr const char* GODOT_BARK_SHADER_CODE =
    "shader_type spatial;\n"
    "uniform vec4 white_color : source_color = vec4(1.0);\n"
    "uniform sampler2D white_albedo : source_color, hint_default_white, filter_linear_mipmap, repeat_enable;\n"
    "uniform sampler2D white_normal : hint_normal, filter_linear_mipmap, repeat_enable;\n"
    "uniform sampler2D white_rough : hint_default_white, filter_linear_mipmap, repeat_enable;\n"
    "uniform float white_roughness = 1.0;\n"
    "uniform vec2 white_uv = vec2(1.0);\n"
    "uniform vec4 base_color : source_color = vec4(1.0);\n"
    "uniform sampler2D base_albedo : source_color, hint_default_white, filter_linear_mipmap, repeat_enable;\n"
    "uniform sampler2D base_normal : hint_normal, filter_linear_mipmap, repeat_enable;\n"
    "uniform sampler2D base_rough : hint_default_white, filter_linear_mipmap, repeat_enable;\n"
    "uniform float base_roughness = 1.0;\n"
    "uniform vec2 base_uv = vec2(1.0);\n"
    "varying float base_share;\n"
    "varying vec3 tiles;\n"
    "void vertex() {\n"
    "    base_share = clamp(INSTANCE_CUSTOM.r, 0.0, 1.0);\n"
    "    tiles = INSTANCE_CUSTOM.gba;\n"
    "}\n"
    "void fragment() {\n"
    "    vec2 uw = UV * tiles.xy * white_uv;\n"
    "    vec2 ub = UV * tiles.xy * tiles.z * base_uv;\n"
    "    vec3 white = texture(white_albedo, uw).rgb * white_color.rgb;\n"
    "    vec3 base = texture(base_albedo, ub).rgb * base_color.rgb;\n"
    "    ALBEDO = mix(white, base, base_share);\n"
    "    NORMAL_MAP = mix(texture(white_normal, uw).rgb, texture(base_normal, ub).rgb, base_share);\n"
    "    ROUGHNESS = mix(texture(white_rough, uw).r * white_roughness, texture(base_rough, ub).r * base_roughness,\n"
    "                    base_share);\n"
    "}\n";
constexpr const char* GODOT_BARK_WHITE_COLOR = "white_color";        // bark shader uniform: albedo colour of the white bark
constexpr const char* GODOT_BARK_WHITE_ALBEDO = "white_albedo";      // bark shader uniform: albedo texture of the white bark
constexpr const char* GODOT_BARK_WHITE_NORMAL = "white_normal";      // bark shader uniform: normal texture of the white bark
constexpr const char* GODOT_BARK_WHITE_ROUGH = "white_rough";        // bark shader uniform: roughness texture of the white bark
constexpr const char* GODOT_BARK_WHITE_ROUGHNESS = "white_roughness";  // bark shader uniform: roughness of the white bark
constexpr const char* GODOT_BARK_WHITE_UV = "white_uv";              // bark shader uniform: UV scale of the white bark
constexpr const char* GODOT_BARK_BASE_COLOR = "base_color";          // bark shader uniform: albedo colour of the stem-base bark
constexpr const char* GODOT_BARK_BASE_ALBEDO = "base_albedo";        // bark shader uniform: albedo texture of the stem-base bark
constexpr const char* GODOT_BARK_BASE_NORMAL = "base_normal";        // bark shader uniform: normal texture of the stem-base bark
constexpr const char* GODOT_BARK_BASE_ROUGH = "base_rough";          // bark shader uniform: roughness texture of the stem-base bark
constexpr const char* GODOT_BARK_BASE_ROUGHNESS = "base_roughness";  // bark shader uniform: roughness of the stem-base bark
constexpr const char* GODOT_BARK_BASE_UV = "base_uv";                // bark shader uniform: UV scale of the stem-base bark

/**
 * Asset showcase (godot_asset_showcase.hpp, PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7).
 */
constexpr const char* GODOT_SHOWCASE_LINE = "asset showcase";   // first words of the showcase's log line
constexpr const char* GODOT_SHOWCASE_ROOT = "AssetShowcase";    // name of the showcase's root node
constexpr const char* GODOT_SHOWCASE_SUN = "ShowcaseSun";       // name of the light the sun of the host sets
constexpr float GODOT_SHOWCASE_SPACING = 1.5f;                  // metres between two slots of one row
constexpr float GODOT_SHOWCASE_SLOT = 1.0f;                     // metres a template is scaled to along its largest extent
constexpr float GODOT_SHOWCASE_SPHERE_RADIUS = 0.5f;            // metres: radius of the sphere a material is shown on
constexpr float GODOT_SHOWCASE_PLANE_ROW = 1.5f;                // z of the row of planes; the spheres stand at z 0
constexpr float GODOT_SHOWCASE_TEMPLATE_ROW = 3.0f;             // z of the row of organ templates
constexpr float GODOT_SHOWCASE_FOV_DEGREES = 60.0f;             // horizontal field of view the showcase camera keeps
constexpr float GODOT_SHOWCASE_HALF_FOV_TAN = 0.57735027f;      // tan of half of GODOT_SHOWCASE_FOV_DEGREES
constexpr float GODOT_SHOWCASE_CAMERA_HEIGHT_SHARE = 0.6f;      // camera height as a share of its distance: a view from above at about 31 degrees

/**
 * The ground of a patch: the code the lay request carries and the patch record returns
 * (ase-pl-flora FLORA_SOIL_*), and the word the start configuration names it by.
 */
constexpr uint8_t GODOT_SOIL_MEADOW = 0;                      // meadow soil
constexpr uint8_t GODOT_SOIL_FOREST = 1;                      // forest floor
constexpr uint8_t GODOT_SOIL_COUNT = 2;                       // grounds the view has a material for
constexpr const char* GODOT_SOIL_WORD_MEADOW = "meadow";      // word of GODOT_SOIL_MEADOW in the start configuration
constexpr const char* GODOT_SOIL_WORD_FOREST = "forest";      // word of GODOT_SOIL_FOREST in the start configuration

/**
 * The codes the patch record carries - those of ase-plant (PLANT_STAGE_*, PLANT_COND_*), forwarded
 * by the plugin unchanged. A code outside them is shown as unknown, never mapped onto a known one.
 */
constexpr uint8_t GODOT_STAGE_SEED = 0;       // a seed in the soil
constexpr uint8_t GODOT_STAGE_SPROUT = 1;     // a seedling
constexpr uint8_t GODOT_STAGE_GROW = 2;       // a growing stand
constexpr uint8_t GODOT_STAGE_MATURE = 3;     // a mature stand
constexpr uint8_t GODOT_STAGE_COUNT = 4;      // stages the view has a word and a colour for
constexpr uint8_t GODOT_COND_HEALTHY = 0;     // a stand without a condition mark
constexpr uint8_t GODOT_COND_WILTED = 1;      // a wilted stand
constexpr uint8_t GODOT_COND_DEAD = 2;        // a dead stand
constexpr uint8_t GODOT_COND_COUNT = 3;       // conditions the view has a word for
constexpr uint8_t GODOT_CODE_UNKNOWN = 255u;  // a stage or condition outside the documented codes

/**
 * The staged bundle (clients/avt-client-android/cmake/AvtRuntime.cmake, scripts/stage_native.py):
 * one index file names everything else in the directory - one line per unit, the data root and
 * every data file below it.
 */
constexpr const char* GODOT_BUNDLE_INDEX = "avt_bundle.index";   // table of contents of the bundle
constexpr const char* GODOT_BUNDLE_KEY_DATA_ROOT = "data_root";  // index key: directory of the units' data files
constexpr const char* GODOT_BUNDLE_KEY_UNIT = "unit";            // index key: unit library manifest version api sha256
constexpr const char* GODOT_BUNDLE_KEY_DATA = "data";            // index key: one data file, relative to the data root
constexpr uint32_t GODOT_BUNDLE_UNIT_FIELDS = 6u;                // fields of one unit line
constexpr const char* GODOT_BUNDLE_SEPARATOR = " = ";            // between key and value of an index line
constexpr const char* GODOT_DATA_COPY_ROOT = "user://bundle";    // where the data files are copied when the stage is no directory on disk

/**
 * Where the stage of the running build lies: res://native/<os>-<arch>/<config>/, the layout
 * scripts/stage_native.py writes. os, arch and config are Godot's own words for the running build
 * (OS::get_name lower-cased, Engine::get_architecture_name, OS::is_debug_build) - the same feature
 * words the avt.gdextension sections are keyed by, so the stage the view reads is the stage Godot
 * took the adapter from.
 */
constexpr const char* GODOT_STAGE_ROOT = "res://native";         // root of every staged platform
constexpr const char* GODOT_STAGE_JOIN = "-";                    // between os and arch in a stage directory
constexpr const char* GODOT_STAGE_DEBUG = "debug";               // stage of a debug build
constexpr const char* GODOT_STAGE_RELEASE = "release";           // stage of a release build

/**
 * The self-test table (clients/avt-client-android/config/vivarium_selftest.toml, PLAN_ASE_VIVARIUM_
 * PHASE_02_ANDROID 02.3): the keys of its TOML form, read by GodotHostResourceManager::self_test.
 * Every case checks one step within a few frames; all cases run in order on one test host.
 */
constexpr const char* GODOT_SELFTEST_KEY_TOLERANCE = "tolerance";    // largest |actual - value| of an eq or ne
constexpr const char* GODOT_SELFTEST_KEY_FRAME = "frame_seconds";    // real seconds one host frame stands for
constexpr const char* GODOT_SELFTEST_KEY_RUNS = "runs";              // runs of 00.4 the table covers
constexpr const char* GODOT_SELFTEST_KEY_CASE = "case";              // array of scenarios, in order on the one test host
constexpr const char* GODOT_SELFTEST_KEY_DO = "do";                  // actions of a scenario, in order
constexpr const char* GODOT_SELFTEST_KEY_CHECK = "check";            // run an action checks
constexpr const char* GODOT_SELFTEST_KEY_FRAMES = "frames";          // action: advance n frames
constexpr const char* GODOT_SELFTEST_KEY_REMEMBER = "remember";      // action: keep a value for a later plus
constexpr const char* GODOT_SELFTEST_KEY_IRRIGATE = "irrigate";      // action: submit one pour
constexpr const char* GODOT_SELFTEST_KEY_WAIT = "wait";              // action: advance until a value holds
constexpr const char* GODOT_SELFTEST_KEY_CATCH_UP = "catch_up";      // action: a return from the background
constexpr const char* GODOT_SELFTEST_KEY_EXPECT = "expect";          // action: compare one value
constexpr const char* GODOT_SELFTEST_KEY_RECORDS = "records";        // action: compare a record count
constexpr const char* GODOT_SELFTEST_KEY_AS = "as";                  // remember: what a later plus refers to
constexpr const char* GODOT_SELFTEST_KEY_PLACE = "place";            // a patch, by its place
constexpr const char* GODOT_SELFTEST_KEY_OBJECT = "object_id";       // irrigate: an object id named directly
constexpr const char* GODOT_SELFTEST_KEY_RECORD = "record";          // "clock" or "patch"
constexpr const char* GODOT_SELFTEST_KEY_KEY = "key";                // the value key of a record
constexpr const char* GODOT_SELFTEST_KEY_OP = "op";                  // eq ne lt le gt ge
constexpr const char* GODOT_SELFTEST_KEY_VALUE = "value";            // the value an op compares with
constexpr const char* GODOT_SELFTEST_KEY_PLUS = "plus";              // a remembered value added to value
constexpr const char* GODOT_SELFTEST_KEY_MAX_FRAMES = "max_frames";  // wait: the bound of a few frames, never a target
constexpr const char* GODOT_SELFTEST_KEY_SECONDS = "seconds";        // catch_up: real seconds handed in at once
constexpr const char* GODOT_SELFTEST_KEY_MAX_STEPS = "max_steps";    // catch_up: most ticks one call may run
constexpr const char* GODOT_SELFTEST_KEY_STEPS = "steps";            // catch_up: ticks in all
constexpr const char* GODOT_SELFTEST_KEY_AMOUNT = "amount_mm";       // irrigate: water poured, mm
constexpr const char* GODOT_SELFTEST_KEY_STATUS = "status";          // irrigate: the expected HostStatus
constexpr const char* GODOT_SELFTEST_KEY_COUNT = "count";            // records: the expected count
constexpr const char* GODOT_SELFTEST_KEY_LAY = "lay";                // action: lay one patch through the scene port
constexpr const char* GODOT_SELFTEST_KEY_SPECIES = "species";        // lay: catalog id text of the species
constexpr const char* GODOT_SELFTEST_KEY_SEED = "seed";              // lay: seed of the plant
constexpr const char* GODOT_SELFTEST_KEY_SOIL = "soil";              // lay: ground word (GODOT_SOIL_WORDS)
constexpr const char* GODOT_SELFTEST_KEY_X = "x";                    // lay: centre of the patch, world metres east
constexpr const char* GODOT_SELFTEST_KEY_Z = "z";                    // lay: centre of the patch, world metres north
constexpr const char* GODOT_SELFTEST_KEY_HALF = "half_m";            // lay: half edge of the patch square, metres
constexpr const char* GODOT_SELFTEST_RECORD_CLOCK = "clock";         // record: the clock port
constexpr const char* GODOT_SELFTEST_RECORD_PATCH = "patch";         // record: the patch port; an organ port by its GODOT_ORGAN_WORDS
constexpr const char* GODOT_SELFTEST_OP_EQ = "eq";                   // |actual - value| <= tolerance
constexpr const char* GODOT_SELFTEST_OP_NE = "ne";                   // |actual - value| > tolerance
constexpr const char* GODOT_SELFTEST_OP_LT = "lt";                   // actual < value
constexpr const char* GODOT_SELFTEST_OP_LE = "le";                   // actual <= value
constexpr const char* GODOT_SELFTEST_OP_GT = "gt";                   // actual > value
constexpr const char* GODOT_SELFTEST_OP_GE = "ge";                   // actual >= value
constexpr uint32_t GODOT_SELFTEST_DECIMALS = 4u;                     // decimals of actual and expected in a report line
constexpr uint32_t GODOT_NUMBER_BYTES = 32u;                         // text of one number in a report line, NUL included
constexpr int GODOT_LOG_LEVEL_MAX = 5;                               // highest level a host line carries (spdlog critical)

/**
 * Measurement mode (PLAN 02.3, acceptance A10): percentiles of the host time (advance + snapshot)
 * and of the whole _process interval, both in microseconds, and the PSS of the process.
 */
constexpr uint32_t GODOT_MEASURE_P50 = 50u;                          // median
constexpr uint32_t GODOT_MEASURE_P95 = 95u;                          // budget percentile of the plan
constexpr uint32_t GODOT_MEASURE_P99 = 99u;                          // tail
constexpr uint32_t GODOT_MEASURE_PERCENT = 100u;                     // whole of a percentile
constexpr double GODOT_MEASURE_FRAMES_RESERVED = 240.0;              // frames per second the buffers are reserved for
constexpr double GODOT_USEC_PER_SECOND = 1000000.0;                  // Time::get_ticks_usec per second
constexpr const char* GODOT_PSS_SOURCE = "/proc/self/smaps_rollup";  // proportional set size of this process
constexpr const char* GODOT_PSS_KEY = "Pss:";                         // its line, value in kB

/**
 * Measurement line of one cycle (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6, godot_cycle_measure.hpp):
 * the organs of the organ ports per branching order, one count per order up to this many; an organ of
 * a deeper order counts in the last one, which the line names as "<n>+".
 */
constexpr uint32_t GODOT_MEASURE_ORDERS = 16u;                       // orders counted one by one

/**
 * Boot steps of GodotHostResourceManager::boot - the name a failure is reported under.
 */
constexpr const char* GODOT_STEP_NONE = "";                // no failure recorded
constexpr const char* GODOT_STEP_CONFIG = "start_config";  // the start configuration is incomplete
constexpr const char* GODOT_STEP_CREATE = "create";        // KernelEmbeddedHost::create
constexpr const char* GODOT_STEP_BUNDLE = "bundle_index";  // reading the bundle index
constexpr const char* GODOT_STEP_MANIFEST = "manifest";    // reading a staged manifest
constexpr const char* GODOT_STEP_DATA_ROOT = "data_root";  // KernelEmbeddedHost::set_data_root
constexpr const char* GODOT_STEP_LOAD = "load_units";      // KernelEmbeddedHost::load_units
constexpr const char* GODOT_STEP_PORT = "port";            // a configured port is not offered
constexpr const char* GODOT_STEP_START = "start";          // KernelEmbeddedHost::start
constexpr const char* GODOT_STEP_LAY = "lay";              // the lay request of a configured patch was refused
constexpr const char* GODOT_STEP_GROUND = "ground";        // the lay request of the configured ground square was refused
constexpr const char* GODOT_STEP_BROOK = "brook";          // the lay request of a configured brook was refused
constexpr const char* GODOT_STEP_ADVANCE = "advance";      // KernelEmbeddedHost::advance
constexpr const char* GODOT_STEP_SNAPSHOT = "snapshot";    // reading a port snapshot
constexpr const char* GODOT_STEP_STOP = "stop";            // KernelEmbeddedHost::stop
constexpr const char* GODOT_LOG_SOURCE = "AVT";            // tag of the adapter's own log lines

/**
 * Places in the Godot project and its user directory.
 */
constexpr const char* GODOT_CONFIG_PATH = "res://config/vivarium_start.json";  // start configuration staged by avt-stage
constexpr const char* GODOT_LOG_PATH = "user://logs/ase-embedded.log";         // ASE log file, resolved to an absolute path
constexpr const char* GODOT_IDENTITY_FILE = "build.identity";                  // stage identity beside the libraries
constexpr const char* GODOT_TEXT_CONFIG_ERROR = "Erro de configuração";        // panel title while the configuration itself is unreadable
constexpr int64_t GODOT_RECT_FIELDS = 4;      // x, y, width, height of a rectangle in the configuration
constexpr int64_t GODOT_POINT_FIELDS = 2;     // x, y of a text anchor in the configuration
constexpr uint32_t GODOT_NO_OBJECT = UINT32_MAX;  // object id that means: nothing selected, nothing pending (a patch entity may be 0)
constexpr uint32_t GODOT_NO_PLACE = UINT32_MAX;   // place that means: the record names none

/**
 * The GDExtension class AseVivariumView (godot_registration.cpp): its name, the engine class its
 * objects are built from, and the engine callbacks the view takes over. Godot asks for each
 * callback by name AND by the hash of its signature; the hashes are those of the pinned
 * extension API (godot-cpp e83fd0904c = godot-4.5-stable, gdextension/extension_api.json). A
 * different hash is a different signature and is answered with "not taken over".
 */
constexpr const char* GODOT_VIEW_CLASS = "AseVivariumView";       // class the scene instantiates
constexpr const char* GODOT_VIEW_ENGINE_CLASS = "Node2D";         // engine class of the view's object
constexpr const char* GODOT_HOOK_READY = "_ready";                // Node: in the tree, children ready
constexpr uint32_t GODOT_HOOK_READY_HASH = 3218959716u;           // signature void ()
constexpr const char* GODOT_HOOK_PROCESS = "_process";            // Node: once per frame
constexpr uint32_t GODOT_HOOK_PROCESS_HASH = 373806689u;          // signature void (float delta)
constexpr const char* GODOT_HOOK_DRAW = "_draw";                  // CanvasItem: redraw requested
constexpr uint32_t GODOT_HOOK_DRAW_HASH = 3218959716u;            // signature void ()
constexpr const char* GODOT_HOOK_INPUT = "_unhandled_input";      // Node: input no control consumed
constexpr uint32_t GODOT_HOOK_INPUT_HASH = 3754044979u;           // signature void (InputEvent event)
constexpr const char* GODOT_HOOK_EXIT = "_exit_tree";             // Node: leaving the tree
constexpr uint32_t GODOT_HOOK_EXIT_HASH = 3218959716u;            // signature void ()

/**
 * Time without a pause (PLAN 01.3 point 4): every frame hands the host the real time that passed
 * since the previous one, measured on Godot's monotonic clock - a span the app spent in the
 * background arrives as one long frame and is caught up in ordinary ticks over the next frames.
 */
constexpr double GODOT_CATCH_UP_NOTE_S = 1.0;   // a frame longer than this is a return from the background and gets one line
constexpr uint32_t GODOT_SOWING_FIRST = 0u;      // the first sowing of a view: the seeds of the start configuration (godot_new_seed.hpp)
constexpr uint64_t GODOT_WATER_WAIT_STEPS = 3u;  // Dissemination runs a pour may take to show in the soil water: plugin, hydro, balance, publish
constexpr double GODOT_WATER_ARRIVED_SHARE = 0.5;  // share of the expected rise (the pour, or the room up to field capacity if smaller) the soil water must gain before the pour counts as arrived; drift between press and arrival moves it by thousandths of a millimetre

/**
 * Drawing style of AseVivariumView - RGBA, 8 bit per channel (Godot Color::hex).
 */
constexpr uint32_t GODOT_COLOR_BACKGROUND = 0x14181AFFu;    // surface behind everything
constexpr uint32_t GODOT_COLOR_HEADER = 0x1F2A2EFFu;        // band behind title and build line
constexpr uint32_t GODOT_COLOR_SOIL = 0x5B4030FFu;          // bare soil of a patch
constexpr uint32_t GODOT_COLOR_SEED = 0x9C7A44FFu;          // seed dots on bare soil
constexpr uint32_t GODOT_COLOR_SPROUT = 0x8BC34AFFu;        // cover of a seedling
constexpr uint32_t GODOT_COLOR_GROW = 0x4CAF50FFu;          // cover of a growing stand
constexpr uint32_t GODOT_COLOR_MATURE = 0x2E7D32FFu;        // cover of a mature stand
constexpr uint32_t GODOT_COLOR_WILTED = 0xA1887FFFu;        // cover of a wilted stand
constexpr uint32_t GODOT_COLOR_DEAD = 0x6E6552FFu;          // cover of a dead stand
constexpr uint32_t GODOT_COLOR_UNKNOWN = 0x9E9E9EFFu;       // cover of a code the view does not know
constexpr uint32_t GODOT_COLOR_WATER = 0x42A5F5FFu;         // filled part of the soil water bar
constexpr uint32_t GODOT_COLOR_WATER_TRACK = 0x1C2B3AFFu;   // empty part of the soil water bar
constexpr uint32_t GODOT_COLOR_SELECTION = 0xFFD54FFFu;     // frame around the selected patch
constexpr uint32_t GODOT_COLOR_TEXT = 0xECEFF1FFu;          // regular text
constexpr uint32_t GODOT_COLOR_TEXT_DIM = 0x90A4AEFFu;      // secondary text
constexpr uint32_t GODOT_COLOR_BUTTON = 0x37474FFFu;        // enabled button face
constexpr uint32_t GODOT_COLOR_BUTTON_OFF = 0x23292CFFu;    // disabled button face
constexpr uint32_t GODOT_COLOR_ERROR = 0xC62828FFu;         // error panel

constexpr int32_t GODOT_FONT_TITLE = 44;                    // title size (logical pixels)
constexpr int32_t GODOT_FONT_BODY = 28;                     // info lines and button labels
constexpr int32_t GODOT_FONT_SMALL = 20;                    // build line, patch labels, notes
constexpr int32_t GODOT_FONT_MIN = 12;                      // smallest size a line shrinks to so that it fits its width
constexpr float GODOT_FRAME_WIDTH = 6.0f;                   // selection frame line width
constexpr float GODOT_BORDER_WIDTH = 2.0f;                  // patch and button outline width
constexpr float GODOT_PATCH_INSET = 14.0f;                  // gap between patch edge and cover
constexpr float GODOT_BAR_HEIGHT = 16.0f;                   // height of the soil water bar
constexpr float GODOT_LABEL_OFFSET = 34.0f;                 // baseline of a label below a rect's top
constexpr float GODOT_HALF = 0.5f;                          // centre of a span
constexpr float GODOT_LAYOUT_SCALE_MAX = 1.0f;              // a layout is never magnified: spare space is distributed, not filled
constexpr const char* GODOT_FEATURE_MOBILE = "mobile";      // Godot feature of a handheld OS: the window is the screen, its safe area applies
constexpr float GODOT_SEED_DOT_RADIUS = 5.0f;               // radius of one seed dot on bare soil
constexpr int32_t GODOT_SEED_DOTS = 3;                      // seed dots per row and per column
constexpr int32_t GODOT_ERROR_LINES = 6;                    // most lines the error panel wraps to
constexpr int64_t GODOT_LOG_DECIMALS = 3;                   // decimals of a measured value in a log line
constexpr const char* GODOT_LOG_STEP_SCHEDULE = "Dissemination";  // schedule whose runs a log line counts as steps= (the modules' 1 Hz step)
constexpr int64_t GODOT_TEXT_DECIMALS = 1;                  // decimals of a value drawn on the surface
constexpr double GODOT_PERCENT = 100.0;                     // a share drawn as a percentage

}  // namespace ase::adp::godot

/**
 * ASE RESOURCE MANAGER IMPLEMENTATION (OOP-ECS Bridge)
 *
 * @file        godot_host_resource_manager.cpp
 * @brief       GodotHostResourceManager - External resource manager for the embedded ASE host of AseVivariumView
 * @description Flyweight Pattern implementation for unique_ptr<KernelEmbeddedHost> and the patch,
 *              clock, organ and template views read out of its port snapshots.
 *              The node stores ONLY uint32_t object ids, the manager owns everything behind them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-10-05
 * @modified    2026-11-23
 * @version     00.00.28.00028 [seed]
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
 * WHY THIS EXISTS:
 *   PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3: AseVivariumView owns exactly ONE KernelEmbeddedHost,
 *   and the resource manager carries factory, failure and teardown. The Godot node knows ids,
 *   places, rectangles and colours; everything that touches the host - the bundle, the units,
 *   the data root, the ports, the teardown - happens here, without a Godot type, so it is tested
 *   against the real bundle.
 *
 * WHAT IT NEVER DOES:
 *   It writes no simulation value back and computes nothing a module computes. The views hold
 *   what the ports reported after the last advance; stage and condition are the codes of
 *   ase-plant as the plugin forwards them.
 *
 * NO PAUSE (PLAN 01.3 point 4):
 *   advance() hands the host the real time the frame took, uncapped, runs its one tick and files
 *   the absolute real position on the real port. A return from the background is one long frame:
 *   ase-time sets its clock forward to the filed position in ONE write (TimeClkRealCalcSystem) and
 *   reports the position it laid (TIM_REAL_LAID_S on the clock record); every module computes the
 *   span up to the new time in its own pass. Nothing is replayed in ticks and nothing is dropped.
 *
 * THE SELF-TEST STARTS ONE TEST HOST (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3):
 *   self_test() creates, loads and starts one host, runs every scenario of the table on it in
 *   order - each a check of one step within a few frames - and stops it once. A start per
 *   scenario would pay the dearest step of the host again for every case and test nothing the
 *   first start did not.
 *
 * THREAD SAFETY:
 *   The log sink is the only entry from a second thread (the host's logging worker); it only
 *   appends to log_lines_ under log_mutex_ - or, on Android, hands the line to logcat, which is
 *   thread-safe itself. Everything else runs on the thread that booted the host, and the host
 *   refuses any other.
 *
 * WHERE THE BUNDLE COMES FROM (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.1):
 *   The manager never opens a stage file. The caller's VivariumStageReadFn reads the bundle index
 *   and the manifests it names - through Godot's FileAccess in the app, so a stage packed into an
 *   APK reads like one on disk. The libraries are opened under library_dir, or, with an empty
 *   library_dir, by their bare file names, which Android's linker resolves in the app's own
 *   namespace; neither res:// nor a package path ever reaches dlopen. The data files are read by
 *   the units themselves, below the data root the host names: files_dir is a real directory.
 *
 * ECS RESOURCE MANAGER IMPLEMENTATION COMPLIANCE
 *
 * [ ] NOT a Component - lives outside ECS registry
 * [ ] Accessed via registry.ctx().get<ResourceManager&>()
 * [ ] Components store ONLY uint32_t IDs
 * [ ] Thread-safe via std::mutex
 * [ ] Proper cleanup in clear_all() - closes/stops BEFORE clearing
 * [ ] No ECS anti-patterns (this is intentionally OOP bridge code)
 * [ ] Implementations in .cpp to avoid mass rebuilds
 * [ ] Header contains ONLY declarations
 * [ ] Layer dependencies checked (only depend on lower layers)
 * [ ] NO file-level static/constexpr (constants → types.hpp)
 * [ ] Filename matches convention
 * [ ] 1 File = 1 ResourceManager
 * [ ] Folder structure matches convention (src/{category}/)
 * [ ] Layer dependencies respected (no upward dependencies)
 * [ ] NO std::shared_ptr in Components - stored HERE via Flyweight Pattern
 * [ ] External resources (shared_ptr, handles) accessed via registry.ctx()
 * [ ] ResourceManager registered in on_start() via registry.ctx().emplace<>()
 * [ ] clear_all() closes resources in REVERSE dependency order
 * [ ] clear_all() called from IniSystem::on_stop() or ShutdownSystem
 */

// INCLUDES - ONLY THESE ARE ALLOWED!
// FORBIDDEN: <vector>, <map>, <unordered_map>, <optional>, <algorithm>
// ALLOWED:   <cstdint>, <cmath>, <cassert>, ase-* headers
// EXCEPTION: This file uses std::unordered_map, std::string and std::vector
//            because ResourceManager is NOT a Component - it's an OOP bridge
//            to an external host (the Godot process that embeds ASE).

// Own header FIRST
#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/adp/godot/godot_organ_records.hpp>
#include <ase/adp/godot/godot_salt_records.hpp>
#include <ase/adp/godot/godot_terrain_samples.hpp>
#include <ase/adp/godot/cap/types.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/loose/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/template/types.hpp>
#include <ase/adp/godot/types.hpp>
// The knots, feet and cut faces of the bark tubes (N0543): knots_ is a unique_ptr to this type, so the
// constructor and the destructor need it complete here; its schedule and page size come with the tube types
#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/math/math.hpp>
#include <ase/utils/strops.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace ase::adp::godot {

using ase::kernel::HostStatus;
using ase::kernel::KernelEmbeddedHost;
using ase::kernel::KernelHostInput;
using ase::kernel::KernelHostRecord;
using ase::kernel::KernelHostUnit;

namespace {

/**
 * The host's log sink. It may run on the logging worker, so it only hands the line to the
 * manager that booted the host (KernelHostLogFn contract: enqueue, never touch the scene).
 */
void queue_host_line(const char* line, uint32_t len, int level, void* user) {
    static_cast<GodotHostResourceManager*>(user)->store_log_line(line, len, level);
}

/** Append one named value; false when the input is already full. */
bool put_value(KernelHostInput& input, const char* key, double value) {
    if (input.value_count >= ase::kernel::HostValuesMax) {
        return false;
    }
    ase::utils::str_copy(input.values[input.value_count].key, ase::kernel::HostKeyBytes, key);
    input.values[input.value_count].value = value;
    input.value_count += 1u;
    return true;
}

/** A whole number below `count` as a code; GODOT_CODE_UNKNOWN for anything else. */
uint8_t code_of(double value, uint8_t count) {
    if (!whole(value) || value >= static_cast<double>(count)) {
        return GODOT_CODE_UNKNOWN;
    }
    return static_cast<uint8_t>(value);
}

/**
 * THE STAGE POSITIONS, a fixed mapping of this file (usecase:101635 - show the plant stage, D2455): the key of
 * the stage record each position of VivariumPatchView::stage is read under, in the order seed, sprout, grow,
 * mature; nullptr outside them. The position orders the tables of the view (words, colours, scales); the codes
 * behind the keys are ase-plant's, and no number of them stands in this adapter.
 */
const char* stage_key(uint8_t position) {
    const char* const keys[GODOT_STAGE_COUNT] = {GODOT_KEY_STGE_SEED, GODOT_KEY_STGE_SPRT, GODOT_KEY_STGE_GROW,
                                                 GODOT_KEY_STGE_MTUR};
    return position < GODOT_STAGE_COUNT ? keys[position] : nullptr;
}

/** Whether a stage position is read under `key` of the stage record. */
bool stage_named(uint8_t position, const char* key) {
    const char* named = stage_key(position);
    return named != nullptr && ase::utils::str_equal(named, key, ase::kernel::HostKeyBytes);
}

/**
 * THE ORGAN CLASSES, a fixed mapping of this file: the word of each class, index = GODOT_ORGAN_*.
 * The same words name the ports in the start configuration and the records in the self-test table.
 */
constexpr const char* kOrganWords[GODOT_ORGAN_COUNT] = {
    GODOT_ORGAN_WORD_SEGMENT, GODOT_ORGAN_WORD_LEAF,  GODOT_ORGAN_WORD_MALE,
    GODOT_ORGAN_WORD_FEMALE,  GODOT_ORGAN_WORD_FRUIT, GODOT_ORGAN_WORD_ROOT};

/**
 * THE DAY OF THE YEAR in the day record (N0460, flora.day.v1): ase-pl-flora files it as FLORA_HOST_KEY_YEAR_DAY
 * from CAL_YEAR_DAY, the day of the VECTUM from 1. Only this unit reads it, so the key stands here and not in
 * types.hpp, which nearly every unit of the adapter includes.
 */
constexpr const char* kDayKeyYearDay = "year_day";

/**
 * THE KEYS OF THE FIGURE PORTS (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE, ase-pl-avatar types.hpp AVATAR_HOST_KEY_*): the
 * body record carries place, course, horizontal velocity, gait, soul and camera value; the camera record eye and
 * aim; the stick, orbit and spawn requests their values. Only this unit reads and writes them, so they stand here
 * and not in types.hpp. The plugin lays the height as its third axis (z) and the second horizontal axis as y; the
 * view reads them into Godot's order (east, up, north) and computes nothing on them.
 */
constexpr const char* kFigureKeyCourse = "yaw";
constexpr const char* kFigureKeyCharacter = "character";  // spawn request: id of the character the figure plays, AVATAR_HOST_KEY_CHARACTER of ase-pl-avatar
constexpr const char* kFigureKeyVelX = "vel_x";
constexpr const char* kFigureKeyVelY = "vel_y";
constexpr const char* kFigureKeyGait = "gait";
constexpr const char* kFigureKeySoul = "soul";
constexpr const char* kFigureKeyCamera = "camera";
constexpr const char* kFigureKeyLodNear = "lod_near";
constexpr const char* kFigureKeyLodMid = "lod_mid";
constexpr const char* kFigureKeyLodFar = "lod_far";
constexpr const char* kFigureKeyEyeX = "eye_x";
constexpr const char* kFigureKeyEyeY = "eye_y";
constexpr const char* kFigureKeyEyeZ = "eye_z";
constexpr const char* kFigureKeyAimX = "aim_x";
constexpr const char* kFigureKeyAimY = "aim_y";
constexpr const char* kFigureKeyAimZ = "aim_z";
constexpr const char* kFigureKeyBound = "bound";
constexpr const char* kFigureKeyEdge = "edge";
constexpr const char* kFigureKeyForward = "forward";
constexpr const char* kFigureKeyStrafe = "strafe";
constexpr const char* kFigureKeySprint = "sprint";
constexpr const char* kFigureKeyJump = "jump";
constexpr const char* kFigureKeyYawDelta = "yaw_delta";
constexpr const char* kFigureKeyPitchDelta = "pitch_delta";
constexpr const char* kFigureKeyZoomDelta = "zoom_delta";  // turn request: change of the orbit distance, m (AVATAR_HOST_KEY_ZOOM_DELTA)
constexpr const char* kFigureKeyItem = "item";      // hold request: catalog id of the thing (AVATAR_HOST_KEY_ITEM)
constexpr const char* kFigureKeyLitres = "litres";  // drain request: litres the release lets go (AVTR_HOST_KEY_LITRES)
constexpr const char* kFigureKeyPatch = "patch";    // drain request: patch object the water falls on (AVTR_HOST_KEY_PATCH, N0517)
constexpr const char* kFigureKeyBearer = "bearer";  // gear record: the figure that carries the piece (AVATAR_HOST_KEY_BEARER)
constexpr const char* kSurvivalKeyEffectiveTemp = "effective_temp";  // survival body: MTB_EFFECTIVE_TEMP, degrees (N0530)
constexpr const char* kSurvivalKeyTempStress = "temp_stress";        // survival body: MTB_TEMP_STRESS, 0 to 1
constexpr const char* kSurvivalKeyHunger = "hunger";                 // survival body: MTB_HUNGER, 0 to 100
constexpr const char* kSurvivalKeyThirst = "thirst";                 // survival body: MTB_THIRST, 0 to 100
constexpr const char* kSurvivalKeyEnergy = "energy";                 // survival body: MTB_ENERGY, 0 to 100
constexpr const char* kSurvivalKeyFatigue = "fatigue";               // survival body: MTB_FATIGUE, 0 to 1
constexpr const char* kSurvivalKeyHealth = "health";                 // survival body: MTB_HEALTH, 0 to 100
constexpr const char* kSurvivalKeyPoisoned = "poisoned";             // survival body: TAG_MTB_POISONED, 1 or 0 (U02)
constexpr const char* kSurvivalKeyBleeding = "bleeding";             // survival body: TAG_MTB_BLEEDING, 1 or 0
constexpr const char* kSurvivalKeyEmaciated = "emaciated";           // survival body: TAG_MTB_EMACIATED, 1 or 0
constexpr const char* kSurvivalKeyWellRested = "well_rested";        // survival body: TAG_MTB_WELL_RESTED, 1 or 0
constexpr const char* kSurvivalKeyBreath = "breath";                  // survival body: GIS_LOCAL_BREATH, 0 normal or better to 1 none (N0532)
constexpr const char* kSurvivalKeyIsResting = "is_resting";          // survival rest: MTB_IS_RESTING, 1 while resting (U05)
constexpr const char* kSurvivalKeySleeping = "sleeping";              // survival rest: MTB_IS_SLEEPING, 1 while the resting figure sleeps (N0555)
constexpr const char* kSurvivalKeyRestQuality = "rest_quality";       // survival rest: MTB_REST_QUALITY, 0 to 1 (FIRECAMP T.11, N0578)
constexpr const char* kSurvivalKeyRestTarget = "rest_target";         // survival rest: MTB_CONST_REST_QUALITY_TARGET, good rest (N0578)
constexpr const char* kSurvivalKeySpoil = "spoil";                    // survival food: ENT_FOOD_ROT_CARRIED, 0 fresh to 1 rotten (U07)
constexpr const char* kSurvivalKeySoot = "soot";                      // survival smoke: GIS_LOCAL_SOOT, kg of soot in the air of the figure's cell (U06, D2500)
constexpr const char* kSurvivalKeySmell = "smell";                    // survival smoke: PER_SMELL_SMOKE, share of smoke the figure's nose caught (B3)
constexpr const char* kSurvivalKeyKind = "kind";                      // survival stock: the hash of the item_art.json key of the food (T.8)
constexpr const char* kSurvivalKeyKg = "kg";                          // survival stock: the fresh mass of the piece, kg (T.8)
// The camp ports of ase-pl-ingle (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.3, T.7; IGL_HOST_KEY_* of its types.hpp).
constexpr const char* kCampKeyEast = "x";        // camp and part request: metres east (IGL_HOST_KEY_X)
constexpr const char* kCampKeyNorth = "y";       // camp and part request: metres north, the second horizontal axis (IGL_HOST_KEY_Y)
constexpr const char* kCampKeyDrop = "drop";     // part request: release height above the stem foot, metres
constexpr const char* kCampKeyFall = "fall";     // part request: fall speed in still air, m/s
constexpr const char* kCampKeyMass = "mass";     // part request: mass of the recipe variant, kg
constexpr const char* kCampKeyMake = "make";     // part, hull and body: name id of the recipe variant
constexpr const char* kCampKeyFuel = "fuel";     // part request: present on a part that burns
constexpr const char* kCampKeyRough = "rough";   // part request: roughness of the recipe variant, metres beneath its convex hull (IGL_HOST_KEY_ROUGH)
constexpr const char* kCampKeyHullX = "hx";      // hull request: corner, metres east of the centre of mass
constexpr const char* kCampKeyHullY = "hy";      // hull request: corner, metres above the centre of mass
constexpr const char* kCampKeyHullZ = "hz";      // hull request: corner, metres north of the centre of mass
constexpr const char* kCampKeyPatch = "patch";   // body record: the patch whose frame the body lies in
constexpr const char* kCampKeyTurnX = "tx";      // body record and part request: rotation vector, east component, radians
constexpr const char* kCampKeyTurnY = "ty";      // body record and part request: rotation vector, up component, radians
constexpr const char* kCampKeyTurnZ = "tz";      // body record and part request: rotation vector, north component, radians
constexpr const char* kCampKeyBody = "body";     // refuel record: the fuel body the keeper lays wood on
constexpr const char* kCampKeyCell = "cell";     // refuel record: the fire cell of that body
constexpr double kCampFuelMark = 1.0;            // the value the key fuel carries; the plugin reads its presence
constexpr double kCampIdMax = 4294967295.0;      // the largest entity id or name id a uint32_t holds
constexpr const char* kFireKeyFlame = "flame";          // fire cell: mark HUB_FIRE_CELL_FLAM, 1 or 0 (IGL_HOST_KEY_FLAME)
constexpr const char* kFireKeyEmber = "ember";          // fire cell: mark HUB_FIRE_CELL_EMBR, 1 or 0 (IGL_HOST_KEY_EMBER)
constexpr const char* kFireKeyAsh = "ash";              // fire cell: mark HUB_FIRE_CELL_ASH, 1 or 0 (IGL_HOST_KEY_ASH)
constexpr const char* kFireKeyFuel = "fuel";            // fire cell: HUB_FIRE_CELL_FUEL, fuel left (IGL_HOST_KEY_FUEL_LEFT)
constexpr const char* kFireKeyFuelLaid = "fuel_laid";   // fire cell: HUB_FIRE_CELL_FUEL_LAID (IGL_HOST_KEY_FUEL_LAID)
constexpr const char* kFireKeyHeat = "heat";            // fire cell: HUB_FIRE_CELL_HEAT (IGL_HOST_KEY_HEAT)
constexpr const char* kFireKeyBody = "body";            // fire cell: HUB_FIRE_CELL_BODY, the fuel body (IGL_HOST_KEY_BODY)
constexpr const char* kFireKeyOxygen = "oxygen";        // fire cell: ATM_OXYGEN_FACTOR, 1 is normal (IGL_HOST_KEY_OXYGEN)
constexpr const char* kFireKeyToxicity = "toxicity";    // fire cell: ATM_TOXICITY, 0 to 1 (IGL_HOST_KEY_TOXICITY)
constexpr const char* kFireKeyCharcoal = "charcoal";    // fire cell: mark HUB_FIRE_CELL_CHAR, 1 or 0 (IGL_HOST_KEY_CHARCOAL)
constexpr const char* kFireKeyBreath = "breath_state";  // fire cell: ATM_GAS_BTH_STATE, 0 to 4 (IGL_HOST_KEY_BREATH)
constexpr const char* kFireKeyBreathNormal = "breath_normal";  // fire cell: ATM_CONST_BREATHABILITY_NORMAL, the grade of normal air
constexpr const char* kLeftKeyAshKg = "ash_kg";         // left record: HUB_ENT_ASH_REQ_ASH, kg of ash since the cell was laid (IGL_HOST_KEY_ASH_KG)
constexpr const char* kLeftKeyBurnLeft = "burn_left";   // left record: HUB_FIRE_CELL_BURN_LEFT, seconds the fuel lasts (IGL_HOST_KEY_BURN_LEFT)
constexpr const char* kWeatherKeyKind = "kind";                // weather record: ATM_WEATHER_KIND (SKY_HOST_KEY_KIND of ase-pl-sky)
constexpr const char* kWeatherKeyTemperature = "temperature";  // weather record: degrees Celsius (SKY_HOST_KEY_TEMPERATURE of ase-pl-sky)
constexpr const char* kWeatherKeyWind = "wind";                // current record: ATM_WIND_SPEED, m/s from 0 (SKY_HOST_KEY_WIND of ase-pl-sky)
constexpr const char* kWeatherKeyNight = "night";              // forecast record: 1 when the middle of the period lies under the horizon
constexpr const char* kWeatherKeyRainMm = "rain_mm";           // forecast record: the millimetres that fall in the period, from 0
constexpr const char* kWeatherCodeClear = "clear";             // code record: ATM_CONST_WEATHER_KIND_CLEAR
constexpr const char* kWeatherCodePcloud = "pcloud";           // code record: ATM_CONST_WEATHER_KIND_PCLOUD
constexpr const char* kWeatherCodeCloudy = "cloudy";           // code record: ATM_CONST_WEATHER_KIND_CLOUDY
constexpr const char* kWeatherCodeDrizzle = "drizzle";         // code record: ATM_CONST_WEATHER_KIND_DRIZZLE
constexpr const char* kWeatherCodeRain = "rain";               // code record: ATM_CONST_WEATHER_KIND_RAIN
constexpr const char* kWeatherCodeSnow = "snow";               // code record: ATM_CONST_WEATHER_KIND_SNOW
constexpr const char* kWeatherCodeFog = "fog";                 // code record: ATM_CONST_WEATHER_KIND_FOG
constexpr const char* kSapKeyRunning = "running";              // sap record: 1 while the tap in the birch runs, else 0
constexpr const char* kSapKeyHeldL = "held_l";                 // sap record: litres in the bark vessel now, from 0
constexpr const char* kSapKeyCapL = "cap_l";                   // sap record: litres the vessel holds when full, above 0
constexpr const char* kFigureKeyEyeHeight = "eye_height";  // rig report: eyes above the soles, m (AVATAR_HOST_KEY_EYE_HEIGHT)
constexpr const char* kFigureKeyHeight = "height";         // rig report: body above the soles, m (AVATAR_HOST_KEY_HEIGHT)
constexpr const char* kFigureKeyRadius = "radius";         // rig report: radius of the capsule around the whole body, m (AVATAR_HOST_KEY_RADIUS)
constexpr const char* kFigureKeyPlate = "plate";           // bind request: entity id of the area plate (AVATAR_HOST_KEY_PLATE)
constexpr const char* kFigureKeySlot = "slot";      // gear record: name_id of the socket (AVATAR_HOST_KEY_SLOT)
constexpr const char* kFigureKeyKind = "kind";      // gear record: catalog id of the thing (AVATAR_HOST_KEY_KIND)
constexpr const char* kFigureKeyWater = "water_l";  // gear record: litres a vessel holds, only on a vessel (AVATAR_HOST_KEY_WATER)
constexpr const char* kFigureKeyPour = "mm_per_l";  // gear record: column one litre lays in the bearer's hand, mm (AVATAR_HOST_KEY_POUR)
constexpr const char* kFigureKeyGaitStnd = "gait_stnd";    // code record: gait of a standing figure (AVATAR_HOST_KEY_GAIT_STND)
constexpr const char* kFigureKeyGaitWalk = "gait_walk";    // code record: gait of a walking figure (AVATAR_HOST_KEY_GAIT_WALK)
constexpr const char* kFigureKeyGaitRun = "gait_run";      // code record: gait of a running figure (AVATAR_HOST_KEY_GAIT_RUN)
constexpr const char* kFigureKeySoulAipc = "soul_aipc";    // code record: soul while the AI leads (AVATAR_HOST_KEY_SOUL_AIPC)
constexpr const char* kFigureKeySoulSspc = "soul_sspc";    // code record: soul while its player leads (AVATAR_HOST_KEY_SOUL_SSPC)
constexpr const char* kFigureKeyCamFixed = "cam_fixed";    // code record: camera in the fixed view (AVATAR_HOST_KEY_CAM_FIXED)
constexpr const char* kFigureKeyCamPlayer = "cam_player";  // code record: camera following the figure (AVATAR_HOST_KEY_CAM_PLAYER)
constexpr uint32_t kFigureCodeObject = 0u;  // object id of the one code record; the codes belong to no figure (AVATAR_HOST_CODE_OBJECT)
constexpr const char* kFigureKeyNormalX = "normal_x";  // boundary record: east share of the outward normal of the edge (AVTR_HOST_KEY_NORMAL_X)
constexpr const char* kFigureKeyNormalZ = "normal_z";  // boundary record: north share of the outward normal of the edge (AVTR_HOST_KEY_NORMAL_Z)
constexpr const char* kFigureKeyTouchX = "touch_x";    // boundary record: touched point, world metres east (AVTR_HOST_KEY_TOUCH_X)
constexpr const char* kFigureKeyTouchZ = "touch_z";    // boundary record: touched point, world metres north (AVTR_HOST_KEY_TOUCH_Z)
constexpr const char* kFigureKeyBumps = "bumps";       // boundary record: bumps of the body since it walks (AVTR_HOST_KEY_BUMPS)
constexpr double kFigureIdMax = 4294967295.0;       // the largest id a uint32_t holds

/**
 * A record value as a whole id of 32 bits; false for a value that is missing, negative, broken or too large.
 * whole() of godot_host_record.cpp compares in float and cannot tell two hash ids above 2^24 apart.
 */
bool figure_id_of(double value, uint32_t& out) {
    if (!std::isfinite(value) || value < 0.0 || value > kFigureIdMax ||
        static_cast<double>(static_cast<uint32_t>(value)) != value) {
        return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

/**
 * One gear record as a piece view (07.10). false for a record that misses one of its three values or carries one
 * that is no whole id: the gear port writes bearer, slot and kind of every piece (AvatarHostGearExptSystem). A
 * vessel adds water_l (07.14): a record without it is no vessel, one with a negative or endless value is broken.
 * A vessel also carries mm_per_l, the column one of its litres lays in its bearer's hand (N0517); missing it
 * stays 0 (the owner has not laid it yet), a negative or endless one breaks the record.
 */
bool figure_gear_of(const KernelHostRecord& record, VivariumFigureGearView& out) {
    out = VivariumFigureGearView{};
    out.object_id = record.object_id;
    if (!figure_id_of(record_value(record, kFigureKeyBearer), out.bearer) ||
        !figure_id_of(record_value(record, kFigureKeySlot), out.slot) ||
        !figure_id_of(record_value(record, kFigureKeyKind), out.kind)) {
        return false;
    }
    const double water = record_value(record, kFigureKeyWater);
    if (std::isnan(water)) {
        return true;
    }
    if (!std::isfinite(water) || water < 0.0) {
        return false;
    }
    out.vessel = true;
    out.water_l = water;
    const double pour = record_value(record, kFigureKeyPour);
    if (std::isnan(pour)) {
        return true;
    }
    if (!std::isfinite(pour) || pour < 0.0) {
        return false;
    }
    out.mm_per_l = pour;
    return true;
}

/**
 * The one code record as the code view (usecase:101611). false for a record that misses one of its seven values or
 * carries one that is not finite: the code port writes all seven every pass (AvatarHostCodeExptSystem).
 */
bool figure_code_of(const KernelHostRecord& record, VivariumFigureCodeView& out) {
    out = VivariumFigureCodeView{};
    const double values[] = {record_value(record, kFigureKeyGaitStnd), record_value(record, kFigureKeyGaitWalk),
                             record_value(record, kFigureKeyGaitRun),  record_value(record, kFigureKeySoulAipc),
                             record_value(record, kFigureKeySoulSspc), record_value(record, kFigureKeyCamFixed),
                             record_value(record, kFigureKeyCamPlayer)};
    for (const double value : values) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    out.gait_stand = values[0];
    out.gait_walk = values[1];
    out.gait_run = values[2];
    out.soul_ai = values[3];
    out.soul_player = values[4];
    out.cam_fixed = values[5];
    out.cam_player = values[6];
    out.known = true;
    return true;
}

/**
 * One boundary record as a bump view (07.13, usecase:101612). false for a record that misses one of its five values,
 * carries one that is not finite, a normal that is no unit share of an edge or a count that is no whole id: the
 * boundary port writes all five of every bumping body (AvatarHostBndyExptSystem).
 */
bool figure_bump_of(const KernelHostRecord& record, VivariumFigureBumpView& out) {
    out = VivariumFigureBumpView{};
    out.object_id = record.object_id;
    out.normal_x = record_value(record, kFigureKeyNormalX);
    out.normal_z = record_value(record, kFigureKeyNormalZ);
    out.touch_x = record_value(record, kFigureKeyTouchX);
    out.touch_z = record_value(record, kFigureKeyTouchZ);
    if (!std::isfinite(out.normal_x) || !std::isfinite(out.normal_z) || !std::isfinite(out.touch_x) ||
        !std::isfinite(out.touch_z) || ase::math::abs(out.normal_x) > 1.0 || ase::math::abs(out.normal_z) > 1.0) {
        return false;
    }
    return figure_id_of(record_value(record, kFigureKeyBumps), out.bumps);
}

/**
 * One body record as a figure view. false for a record that misses a value or carries one that is not finite:
 * the body port writes all twelve values of every figure (AvatarHostBodyExptSystem), the three detail bands last.
 */
bool figure_view_of(const KernelHostRecord& record, VivariumFigureView& out) {
    out = VivariumFigureView{};
    out.object_id = record.object_id;
    const double values[] = {record_value(record, GODOT_KEY_X),        record_value(record, GODOT_KEY_Z),
                             record_value(record, GODOT_KEY_Y),        record_value(record, kFigureKeyCourse),
                             record_value(record, kFigureKeyVelX),     record_value(record, kFigureKeyVelY),
                             record_value(record, kFigureKeyGait),     record_value(record, kFigureKeySoul),
                             record_value(record, kFigureKeyCamera),   record_value(record, kFigureKeyLodNear),
                             record_value(record, kFigureKeyLodMid),   record_value(record, kFigureKeyLodFar)};
    for (const double value : values) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    out.x = values[0];
    out.y = values[1];  // the plugin's height axis
    out.z = values[2];  // the plugin's second horizontal axis
    out.course = values[3];
    out.vel_x = values[4];
    out.vel_z = values[5];
    out.gait = values[6];
    out.soul = values[7];
    out.camera = values[8];
    out.lod_near = values[9];
    out.lod_mid = values[10];
    out.lod_far = values[11];
    return true;
}

/** One camera record as the player camera view; false for a record that misses a value or carries one not finite. */
bool figure_camera_of(const KernelHostRecord& record, VivariumFigureCameraView& out) {
    out = VivariumFigureCameraView{};
    out.figure = record.object_id;
    const double values[] = {record_value(record, kFigureKeyEyeX), record_value(record, kFigureKeyEyeZ),
                             record_value(record, kFigureKeyEyeY), record_value(record, kFigureKeyAimX),
                             record_value(record, kFigureKeyAimZ), record_value(record, kFigureKeyAimY),
                             record_value(record, kFigureKeyBound), record_value(record, kFigureKeyEdge)};
    for (const double value : values) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    // the rim state is a mark of ase-camera: 1 while it holds the camera on the rim of the loaded area, else 0
    if (values[6] != 0.0 && values[6] != 1.0) {
        return false;
    }
    out.bound = values[6] == 1.0;
    // the rim state of the followed figure is a mark of ase-physics (HUB_PHYS_STAND_BNDY): 1 in every step it holds
    // the walking body at a chunk border of the loaded ground, else 0 - RAND DES AREALS (07-G, 07.13)
    if (values[7] != 0.0 && values[7] != 1.0) {
        return false;
    }
    out.edge = values[7] == 1.0;
    out.eye_x = values[0];
    out.eye_y = values[1];  // the plugin's eye height
    out.eye_z = values[2];
    out.aim_x = values[3];
    out.aim_y = values[4];  // the plugin's aim height
    out.aim_z = values[5];
    out.known = true;
    return true;
}

/** The organ class a word names; GODOT_ORGAN_COUNT for a word that names none. */
uint32_t organ_class_of(const std::string& word) {
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        if (word == kOrganWords[c]) {
            return c;
        }
    }
    return GODOT_ORGAN_COUNT;
}

/**
 * One ground cell record as a view. false for a record that misses a key or names no whole object id:
 * the ground cell port writes the body and all four corners of every cell (ase-pl-flora,
 * FloraHostGrndCellExptSystem).
 */
bool ground_cell_view_of(const KernelHostRecord& record, VivariumGroundCellView& out) {
    out = VivariumGroundCellView{};
    out.object_id = record.object_id;
    const double ids[5] = {record_value(record, GODOT_KEY_BODY), record_value(record, GODOT_KEY_SW),
                           record_value(record, GODOT_KEY_SE), record_value(record, GODOT_KEY_NE),
                           record_value(record, GODOT_KEY_NW)};
    for (const double id : ids) {
        if (!whole(id) || id > static_cast<double>(UINT32_MAX)) {
            return false;
        }
    }
    out.body = static_cast<uint32_t>(ids[0]);
    out.sw = static_cast<uint32_t>(ids[1]);
    out.se = static_cast<uint32_t>(ids[2]);
    out.ne = static_cast<uint32_t>(ids[3]);
    out.nw = static_cast<uint32_t>(ids[4]);
    return true;
}

/**
 * One stream record as a view. false for a record that misses one of the eight keys the stream port writes for
 * every brook it names (ase-pl-water, WaterHostStrmExptSystem). The groundwater beside the bed (B0286) is read
 * from its own two keys and fails no record: the plugin files them only while ase-gis has mapped the cell at the
 * start of the bed (N0484: layer GIS_LYR_HYD_LEVEL, answered as GIS_HYD_CELL_LEVEL); before, record_value gives
 * NaN, the fields keep it, and the view's bridge logs the gap on its edge (VivariumView::follow_scene).
 */
bool stream_view_of(const KernelHostRecord& record, VivariumStreamView& out) {
    constexpr const char* KEY_GROUNDWATER_LEVEL = "groundwater_level_m";  // FLORA_HOST_KEY_GNDW_LEVEL: GIS_HYD_CELL_LEVEL, world metres above NN0
    constexpr const char* KEY_GROUNDWATER_REACH = "groundwater_reach_m";  // FLORA_HOST_KEY_GNDW_REACH: HYD_CONST_GNDW_REACH, metres
    out = VivariumStreamView{};
    out.object_id = record.object_id;
    out.x = record_value(record, GODOT_KEY_X);
    out.z = record_value(record, GODOT_KEY_Z);
    out.end_x = record_value(record, GODOT_KEY_END_X);
    out.end_z = record_value(record, GODOT_KEY_END_Z);
    out.half_m = record_value(record, GODOT_KEY_HALF);
    out.depth_m = record_value(record, GODOT_KEY_WATER_DEPTH);
    out.velocity_m_s = record_value(record, GODOT_KEY_VELOCITY);
    out.discharge_m3_s = record_value(record, GODOT_KEY_DISCHARGE);
    out.groundwater_level_m = record_value(record, KEY_GROUNDWATER_LEVEL);
    out.groundwater_reach_m = record_value(record, KEY_GROUNDWATER_REACH);
    return std::isfinite(out.x) && std::isfinite(out.z) && std::isfinite(out.end_x) && std::isfinite(out.end_z) &&
           std::isfinite(out.half_m) && std::isfinite(out.depth_m) && std::isfinite(out.velocity_m_s) &&
           std::isfinite(out.discharge_m3_s);
}

/**
 * Whether two reads of the stream port carry the same water, brook for brook in port order. The groundwater pair
 * is NaN while the port carries none, and NaN equals nothing, so two reads without it count as the same there;
 * otherwise every read would be a change and the water table would be laid again each frame.
 */
bool same_streams(const std::vector<VivariumStreamView>& read, const std::vector<VivariumStreamView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        const bool level_same = read[i].groundwater_level_m == held[i].groundwater_level_m ||
                                (std::isnan(read[i].groundwater_level_m) && std::isnan(held[i].groundwater_level_m));
        const bool reach_same = read[i].groundwater_reach_m == held[i].groundwater_reach_m ||
                                (std::isnan(read[i].groundwater_reach_m) && std::isnan(held[i].groundwater_reach_m));
        if (read[i].object_id != held[i].object_id || read[i].x != held[i].x || read[i].z != held[i].z ||
            read[i].end_x != held[i].end_x || read[i].end_z != held[i].end_z || read[i].half_m != held[i].half_m ||
            read[i].depth_m != held[i].depth_m || read[i].velocity_m_s != held[i].velocity_m_s ||
            read[i].discharge_m3_s != held[i].discharge_m3_s || !level_same || !reach_same) {
            return false;
        }
    }
    return true;
}

/**
 * Whether two values of a water record are the same read. A value the ase-gis cell never answered is missing from
 * the record and stands NaN; NaN equals nothing, so two unanswered reads count as the same, or every read of a port
 * with an unanswered value would be a change and the view would lay its water anew each frame.
 */
bool same_water_value(double read, double held) {
    return read == held || (std::isnan(read) && std::isnan(held));
}

/**
 * One current record as a view. false for a record without its place: the current port lays curr_x and curr_z with
 * every record it lays (ase-pl-water, WaterHostCurrExptSystem). Depth, speed and the two direction shares come only
 * where the ase-gis cell answered them; a missing one stays NaN and fails no record.
 */
bool current_view_of(const KernelHostRecord& record, VivariumCurrentView& out) {
    constexpr const char* KEY_X = "curr_x";              // WATR_HOST_KEY_CURR_X: east of the cell centre, metres
    constexpr const char* KEY_Z = "curr_z";              // WATR_HOST_KEY_CURR_Z: north of the cell centre, metres
    constexpr const char* KEY_DEPTH = "curr_depth_m";    // WATR_HOST_KEY_CURR_DEPTH: GIS_HYD_CELL_CURR_DEPTH, metres
    constexpr const char* KEY_SPEED = "curr_speed_mps";  // WATR_HOST_KEY_CURR_SPEED: GIS_HYD_CELL_CURR_SPEED, m/s
    constexpr const char* KEY_EAST = "curr_east";        // WATR_HOST_KEY_CURR_EAST: GIS_HYD_CELL_CURR_EAST, -1 to 1
    constexpr const char* KEY_NORTH = "curr_north";      // WATR_HOST_KEY_CURR_NORTH: GIS_HYD_CELL_CURR_NORTH, -1 to 1
    constexpr const char* KEY_CHUNK = "curr_chunk_m";    // WATR_HOST_KEY_CURR_CHUNK: TER_CONST_CHUNK_SIZE, metres (H.13)
    out = VivariumCurrentView{};
    out.object_id = record.object_id;
    out.x = record_value(record, KEY_X);
    out.z = record_value(record, KEY_Z);
    out.depth_m = record_value(record, KEY_DEPTH);
    out.speed_m_s = record_value(record, KEY_SPEED);
    out.east = record_value(record, KEY_EAST);
    out.north = record_value(record, KEY_NORTH);
    out.chunk_m = record_value(record, KEY_CHUNK);
    // what the water carries comes from the quality port and is joined afterwards; until then it is unanswered
    out.bloom_kg_m3 = std::nan("");
    out.rot_kg_m3 = std::nan("");
    out.salt_kg_m3 = std::nan("");
    out.hue_deg = std::nan("");
    return std::isfinite(out.x) && std::isfinite(out.z);
}

/**
 * What the water of one current cell carries, out of its record on the quality port (ase-pl-water,
 * WaterHostCurrQltyExptSystem, D2508), into the view of the same cell. A value the ase-gis cell never answered is
 * missing from the record and stays NaN; the view computes none of them.
 */
void quality_into(const KernelHostRecord& record, VivariumCurrentView& cell) {
    constexpr const char* KEY_BLOOM = "curr_bloom_kg_m3";  // WATR_HOST_KEY_CURR_BLOOM: GIS_HYD_CELL_CURR_BLOOM, kg/m3
    constexpr const char* KEY_ROT = "curr_rot_kg_m3";      // WATR_HOST_KEY_CURR_ROT: GIS_HYD_CELL_CURR_ROT, kg/m3
    constexpr const char* KEY_SALT = "curr_salt_kg_m3";    // WATR_HOST_KEY_CURR_SALT: GIS_HYD_CELL_CURR_SALT, kg/m3
    constexpr const char* KEY_HUE = "curr_hue_deg";        // WATR_HOST_KEY_CURR_HUE: GIS_HYD_CELL_CURR_HUE, degrees
    cell.bloom_kg_m3 = record_value(record, KEY_BLOOM);
    cell.rot_kg_m3 = record_value(record, KEY_ROT);
    cell.salt_kg_m3 = record_value(record, KEY_SALT);
    cell.hue_deg = record_value(record, KEY_HUE);
}

/** Whether two reads of the current port carry the same water, cell for cell in port order. */
bool same_currents(const std::vector<VivariumCurrentView>& read, const std::vector<VivariumCurrentView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        if (read[i].object_id != held[i].object_id || read[i].x != held[i].x || read[i].z != held[i].z ||
            !same_water_value(read[i].depth_m, held[i].depth_m) ||
            !same_water_value(read[i].speed_m_s, held[i].speed_m_s) ||
            !same_water_value(read[i].east, held[i].east) || !same_water_value(read[i].north, held[i].north) ||
            !same_water_value(read[i].chunk_m, held[i].chunk_m) ||
            !same_water_value(read[i].bloom_kg_m3, held[i].bloom_kg_m3) ||
            !same_water_value(read[i].rot_kg_m3, held[i].rot_kg_m3) ||
            !same_water_value(read[i].salt_kg_m3, held[i].salt_kg_m3) ||
            !same_water_value(read[i].hue_deg, held[i].hue_deg)) {
            return false;
        }
    }
    return true;
}

/**
 * One pit record as a view. false for a record without its place: the pit port lays pit_x and pit_z with every
 * record it lays (ase-pl-hydrogeology, HydrogeologyHostPitExptSystem). The water over the floor, over the rim and
 * the ice on it come only where the ase-gis cell answered them; a missing one stays NaN and fails no record.
 */
bool pit_view_of(const KernelHostRecord& record, VivariumPitView& out) {
    constexpr const char* KEY_X = "pit_x";              // HYG_HOST_KEY_PIT_X: east of the pit centre, metres
    constexpr const char* KEY_Z = "pit_z";              // HYG_HOST_KEY_PIT_Z: north of the pit centre, metres
    constexpr const char* KEY_DEPTH = "pit_depth_m";    // HYG_HOST_KEY_PIT_DEPTH: GIS_HYD_CELL_PIT_DEPTH, metres
    constexpr const char* KEY_SPILL = "pit_spill_m3_s";  // HYG_HOST_KEY_PIT_SPILL: GIS_HYD_CELL_PIT_SPILL, m3/s
    constexpr const char* KEY_ICE = "pit_ice_m";        // HYG_HOST_KEY_PIT_ICE: GIS_HYD_CELL_PIT_ICE, metres (H.11)
    out = VivariumPitView{};
    out.object_id = record.object_id;
    out.x = record_value(record, KEY_X);
    out.z = record_value(record, KEY_Z);
    out.depth_m = record_value(record, KEY_DEPTH);
    out.spill_m3_s = record_value(record, KEY_SPILL);
    out.ice_m = record_value(record, KEY_ICE);
    return std::isfinite(out.x) && std::isfinite(out.z);
}

/** Whether two reads of the pit port carry the same water, pit for pit in port order. */
bool same_pits(const std::vector<VivariumPitView>& read, const std::vector<VivariumPitView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        if (read[i].object_id != held[i].object_id || read[i].x != held[i].x || read[i].z != held[i].z ||
            !same_water_value(read[i].depth_m, held[i].depth_m) ||
            !same_water_value(read[i].spill_m3_s, held[i].spill_m3_s) ||
            !same_water_value(read[i].ice_m, held[i].ice_m)) {
            return false;
        }
    }
    return true;
}

/**
 * One groundwater record as a view. false for a record without its chunk: the groundwater port lays the three grid
 * coordinates with every record it lays (ase-pl-hydrogeology, HydrogeologyHostGndwExptSystem). Table, standing
 * water, stock, bank flow and spring discharge come only where the ase-gis cell answered them; a missing one stays
 * NaN.
 */
bool groundwater_view_of(const KernelHostRecord& record, VivariumGroundwaterView& out) {
    constexpr const char* KEY_CHUNK_X = "chunk_x";  // HYG_HOST_KEY_CHUNK_X: chunk grid X of the row
    constexpr const char* KEY_CHUNK_Y = "chunk_y";  // HYG_HOST_KEY_CHUNK_Y: chunk grid Y of the row, the vertical slice
    constexpr const char* KEY_CHUNK_Z = "chunk_z";  // HYG_HOST_KEY_CHUNK_Z: chunk grid Z of the row
    constexpr const char* KEY_TABLE = "table_m";    // HYG_HOST_KEY_TABLE: water table below the mean ground, metres
    constexpr const char* KEY_DEPTH = "depth_m";    // HYG_HOST_KEY_DEPTH: standing water over the lowest ground, metres
    constexpr const char* KEY_STOCK = "stock_m3";   // HYG_HOST_KEY_STOCK: groundwater stock against the rest table, m3
    constexpr const char* KEY_BANK = "bank_m3_s";   // HYG_HOST_KEY_BANK: water the stock gives the brooks, m3/s
    constexpr const char* KEY_SPRN = "sprn_m3_s";   // HYG_HOST_KEY_SPRN: the spring's part of the bank, m3/s (H.2)
    out = VivariumGroundwaterView{};
    out.object_id = record.object_id;
    out.chunk_x = record_value(record, KEY_CHUNK_X);
    out.chunk_y = record_value(record, KEY_CHUNK_Y);
    out.chunk_z = record_value(record, KEY_CHUNK_Z);
    out.table_m = record_value(record, KEY_TABLE);
    out.depth_m = record_value(record, KEY_DEPTH);
    out.stock_m3 = record_value(record, KEY_STOCK);
    out.bank_m3_s = record_value(record, KEY_BANK);
    out.sprn_m3_s = record_value(record, KEY_SPRN);
    // a chunk grid coordinate is a signed whole number (types::chunk_id_to_coords), and whole() takes no negative
    // one; the grid stays far below 2^24, where ase::math::floor in float is exact
    const auto grid = [](double value) {
        return std::isfinite(value) && ase::math::floor(static_cast<float>(value)) == static_cast<float>(value);
    };
    return grid(out.chunk_x) && grid(out.chunk_y) && grid(out.chunk_z);
}

/** Whether two reads of the groundwater port carry the same water, chunk for chunk in port order. */
bool same_groundwater(const std::vector<VivariumGroundwaterView>& read,
                      const std::vector<VivariumGroundwaterView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        if (read[i].object_id != held[i].object_id || read[i].chunk_x != held[i].chunk_x ||
            read[i].chunk_y != held[i].chunk_y || read[i].chunk_z != held[i].chunk_z ||
            !same_water_value(read[i].table_m, held[i].table_m) ||
            !same_water_value(read[i].depth_m, held[i].depth_m) ||
            !same_water_value(read[i].stock_m3, held[i].stock_m3) ||
            !same_water_value(read[i].bank_m3_s, held[i].bank_m3_s) ||
            !same_water_value(read[i].sprn_m3_s, held[i].sprn_m3_s)) {
            return false;
        }
    }
    return true;
}

/**
 * The measuring point of a changed groundwater read (H.2, H.3): how many chunks the port carries, how many of them
 * a spring runs in and with how much water, and the highest and the deepest table among them. Counted from the
 * records as they came, a value the cell never answered (NaN) counts in no sum; nothing here feeds the image.
 */
void note_groundwater(KernelEmbeddedHost& host, const std::vector<VivariumGroundwaterView>& groundwater) {
    uint32_t springs = 0u;
    uint32_t tables = 0u;
    double spring_sum = 0.0;
    double table_high = 0.0;
    double table_deep = 0.0;
    for (const VivariumGroundwaterView& chunk : groundwater) {
        if (std::isfinite(chunk.sprn_m3_s) && chunk.sprn_m3_s > 0.0) {
            springs += 1u;
            spring_sum += chunk.sprn_m3_s;
        }
        if (std::isfinite(chunk.table_m)) {
            table_high = tables == 0u || chunk.table_m < table_high ? chunk.table_m : table_high;
            table_deep = tables == 0u || chunk.table_m > table_deep ? chunk.table_m : table_deep;
            tables += 1u;
        }
    }
    std::string line = "groundwater chunks ";
    line += std::to_string(groundwater.size());
    line += " springs ";
    line += std::to_string(springs);
    line += " spring_m3_s=";
    line += std::to_string(spring_sum);
    line += " tables ";
    line += std::to_string(tables);
    line += " table_m=";
    line += std::to_string(table_high);
    line += "..";
    line += std::to_string(table_deep);
    (void)host.note(GODOT_LOG_SOURCE, line.c_str());
}

}  // namespace

/**
 * One salt record as a view (H.9, I-4). false for a record without its chunk or with a half missing: the salt port
 * lays a record only where the ase-gis cell answered the water half, the soil half and their sum
 * (ase-pl-hydrogeology, HydrogeologyHostSlntExptSystem), so a record short of one is a broken port, never a gap.
 * Declared in godot_salt_records.hpp, so the host tests lay a hand-built record out the way read_salt does.
 */
bool salt_view_of(const KernelHostRecord& record, VivariumSaltView& out) {
    constexpr const char* KEY_CHUNK_X = "chunk_x";      // HYG_HOST_KEY_CHUNK_X: chunk grid X of the row
    constexpr const char* KEY_CHUNK_Z = "chunk_z";      // HYG_HOST_KEY_CHUNK_Z: chunk grid Z of the row
    constexpr const char* KEY_HYD = "salt_hyd_kg";      // HYG_HOST_KEY_SLNT_HYD: GIS_SLNT_CHK_HYD_KG, kg
    constexpr const char* KEY_SOIL = "salt_soil_kg";    // HYG_HOST_KEY_SLNT_SOIL: GIS_SLNT_CHK_SOIL_KG, kg
    constexpr const char* KEY_SUM = "salt_kg";          // HYG_HOST_KEY_SLNT_SUM: GIS_SLNT_CHK_KG, kg
    out = VivariumSaltView{};
    out.object_id = record.object_id;
    out.chunk_x = record_value(record, KEY_CHUNK_X);
    out.chunk_z = record_value(record, KEY_CHUNK_Z);
    out.hyd_kg = record_value(record, KEY_HYD);
    out.soil_kg = record_value(record, KEY_SOIL);
    out.sum_kg = record_value(record, KEY_SUM);
    const auto grid = [](double value) {
        return std::isfinite(value) && ase::math::floor(static_cast<float>(value)) == static_cast<float>(value);
    };
    return grid(out.chunk_x) && grid(out.chunk_z) && std::isfinite(out.hyd_kg) && std::isfinite(out.soil_kg) &&
           std::isfinite(out.sum_kg);
}

namespace {

/** Whether two reads of the salt port carry the same salt, chunk for chunk in port order. */
bool same_salt(const std::vector<VivariumSaltView>& read, const std::vector<VivariumSaltView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        if (read[i].object_id != held[i].object_id || read[i].chunk_x != held[i].chunk_x ||
            read[i].chunk_z != held[i].chunk_z || read[i].hyd_kg != held[i].hyd_kg ||
            read[i].soil_kg != held[i].soil_kg || read[i].sum_kg != held[i].sum_kg) {
            return false;
        }
    }
    return true;
}

/**
 * The measuring point of a changed salt read (H.9, I-4): how many chunks the port carries and the three sums over
 * them, the water half, the soil half and the total ase-gis added. The I-4 balance of Seed 42 reads this line: the
 * total over the world stays constant while salt moves between water and soil, and the residue of total minus both
 * halves is the closing error of the addition. Nothing here feeds the image.
 */
void note_salt(KernelEmbeddedHost& host, const std::vector<VivariumSaltView>& salt) {
    double hyd_sum = 0.0;
    double soil_sum = 0.0;
    double total_sum = 0.0;
    for (const VivariumSaltView& chunk : salt) {
        hyd_sum += chunk.hyd_kg;
        soil_sum += chunk.soil_kg;
        total_sum += chunk.sum_kg;
    }
    std::string line = "salt chunks ";
    line += std::to_string(salt.size());
    line += " hyd_kg=";
    line += std::to_string(hyd_sum);
    line += " soil_kg=";
    line += std::to_string(soil_sum);
    line += " total_kg=";
    line += std::to_string(total_sum);
    line += " residue_kg=";
    line += std::to_string(total_sum - hyd_sum - soil_sum);
    (void)host.note(GODOT_LOG_SOURCE, line.c_str());
}

/**
 * Whether two reads of the terrain and ground cell ports carry the same ground, sample for sample and cell
 * for cell in port order. GODOT_TERRAIN_SCHEDULE runs whether ase-terrain changed a height or not, so a
 * read alone is no change: only a ground that differs moves terrain_generation, and the view rebuilds
 * its mesh and writes its `scene ground` line once per levelling or carved bed, never twice a second.
 */
bool same_ground(const std::vector<VivariumTerrainView>& samples, const std::vector<VivariumGroundCellView>& cells,
                 const std::vector<VivariumTerrainView>& held_samples,
                 const std::vector<VivariumGroundCellView>& held_cells) {
    if (samples.size() != held_samples.size() || cells.size() != held_cells.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < samples.size(); ++i) {
        const VivariumTerrainView& a = samples[i];
        const VivariumTerrainView& b = held_samples[i];
        if (a.object_id != b.object_id || a.body != b.body || a.x != b.x || a.z != b.z || a.height != b.height ||
            a.nx != b.nx || a.ny != b.ny || a.nz != b.nz || a.u != b.u || a.v != b.v) {
            return false;
        }
    }
    for (std::size_t i = 0u; i < cells.size(); ++i) {
        const VivariumGroundCellView& a = cells[i];
        const VivariumGroundCellView& b = held_cells[i];
        if (a.object_id != b.object_id || a.body != b.body || a.sw != b.sw || a.se != b.se || a.ne != b.ne ||
            a.nw != b.nw) {
            return false;
        }
    }
    return true;
}

/**
 * Whether two reads of the bark port carry the same tubes, vertex for vertex in port order: only a set that
 * differs moves bark_generation, so the view lays the tubes anew once per new set and not once per read.
 */
bool same_bark(const std::vector<VivariumBarkVertexView>& read, const std::vector<VivariumBarkVertexView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        const VivariumBarkVertexView& a = read[i];
        const VivariumBarkVertexView& b = held[i];
        if (a.object_id != b.object_id || a.body != b.body || a.x != b.x || a.y != b.y || a.z != b.z ||
            a.nx != b.nx || a.ny != b.ny || a.nz != b.nz || a.u != b.u || a.v != b.v || a.next != b.next ||
            a.below != b.below || a.organ != b.organ) {
            return false;
        }
    }
    return true;
}

/**
 * Whether the vertices name exactly the internodes the segment port showed: every vertex an internode of the
 * set, every internode of the set on at least one ring. ase-render-data gives every internode its tip ring, so
 * a set that names other internodes is an old set the host still holds while the new one is filed page by page.
 */
bool names_the_internodes(const std::vector<VivariumBarkVertexView>& vertices,
                          const std::vector<VivariumOrganView>& internodes) {
    std::unordered_map<uint32_t, bool> ringed;
    ringed.reserve(internodes.size());
    for (const VivariumOrganView& internode : internodes) {
        ringed.emplace(internode.object_id, false);
    }
    for (const VivariumBarkVertexView& vertex : vertices) {
        const auto found = ringed.find(vertex.organ);
        if (found == ringed.end()) {
            return false;
        }
        found->second = true;
    }
    for (const auto& entry : ringed) {
        if (!entry.second) {
            return false;
        }
    }
    return true;
}

/**
 * One template corner record as a view (usecase:101011 tessellate the organ templates). false for a record that
 * misses a key, names no known template code or no whole index, or carries a place, normal or texture
 * coordinate that is no finite number: the corner port writes all ten keys of every corner (ase-pl-flora,
 * FloraHostTmplVtxExptSystem), so a gap is a broken port - never a corner drawn at the origin.
 */
bool template_corner_view_of(const KernelHostRecord& record, VivariumTemplateCornerView& out) {
    out = VivariumTemplateCornerView{};
    out.object_id = record.object_id;
    const double code = record_value(record, GODOT_KEY_TEMPLATE);
    const double index = record_value(record, GODOT_KEY_INDEX);
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    out.nx = record_value(record, GODOT_KEY_NX);
    out.ny = record_value(record, GODOT_KEY_NY);
    out.nz = record_value(record, GODOT_KEY_NZ);
    out.u = record_value(record, GODOT_KEY_U);
    out.v = record_value(record, GODOT_KEY_V);
    if (!whole(code) || code >= static_cast<double>(GODOT_TMPL_COUNT) || !whole(index) ||
        index > static_cast<double>(UINT32_MAX) || !std::isfinite(out.x) || !std::isfinite(out.y) ||
        !std::isfinite(out.z) || !std::isfinite(out.nx) || !std::isfinite(out.ny) || !std::isfinite(out.nz) ||
        !std::isfinite(out.u) || !std::isfinite(out.v)) {
        return false;
    }
    out.code = static_cast<uint32_t>(code);
    out.index = static_cast<uint32_t>(index);
    return true;
}

/**
 * The corner a template tangent record belongs to, as one number: its template code in the high half and its
 * index in the low half, the same pair template_corner_view_of reads off the corner record.
 */
uint64_t template_corner_key(uint32_t code, uint32_t index) {
    return (static_cast<uint64_t>(code) << 32u) | static_cast<uint64_t>(index);
}

/**
 * One template tangent record (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.6) put on its corner of `corners`, found
 * through `place_of` (template_corner_key to the position in `corners`). false for a record that misses a key,
 * names no known template code or no whole index, names a corner the corner port did not carry, names a corner
 * a second time, or carries a tangent that is no finite number or a bitangent sign of 0: the tangent port
 * writes all six keys of every corner (ase-pl-flora, FloraHostTmplTngExptSystem), so a gap is a broken port -
 * never a tangent along the origin.
 */
bool template_tangent_onto(const KernelHostRecord& record, const std::unordered_map<uint64_t, std::size_t>& place_of,
                           std::vector<VivariumTemplateCornerView>& corners, std::vector<bool>& carried) {
    const double code = record_value(record, GODOT_KEY_TEMPLATE);
    const double index = record_value(record, GODOT_KEY_INDEX);
    const double tx = record_value(record, GODOT_KEY_TX);
    const double ty = record_value(record, GODOT_KEY_TY);
    const double tz = record_value(record, GODOT_KEY_TZ);
    const double tw = record_value(record, GODOT_KEY_TW);
    if (!whole(code) || code >= static_cast<double>(GODOT_TMPL_COUNT) || !whole(index) ||
        index > static_cast<double>(UINT32_MAX) || !std::isfinite(tx) || !std::isfinite(ty) || !std::isfinite(tz) ||
        !std::isfinite(tw) || tw == 0.0) {
        return false;
    }
    const auto found = place_of.find(template_corner_key(static_cast<uint32_t>(code), static_cast<uint32_t>(index)));
    if (found == place_of.end() || carried[found->second]) {
        return false;
    }
    VivariumTemplateCornerView& corner = corners[found->second];
    corner.tx = tx;
    corner.ty = ty;
    corner.tz = tz;
    corner.tw = tw;
    carried[found->second] = true;
    return true;
}

/**
 * One template triangle record as a view. false for a record that misses a key, names no known template
 * code or a corner index that is no whole number: the triangle port writes the code and all three corners of
 * every triangle (ase-pl-flora, FloraHostTmplTriExptSystem).
 */
bool template_triangle_view_of(const KernelHostRecord& record, VivariumTemplateTriangleView& out) {
    out = VivariumTemplateTriangleView{};
    out.object_id = record.object_id;
    const double code = record_value(record, GODOT_KEY_TEMPLATE);
    const double corners[3] = {record_value(record, GODOT_KEY_CORNER_A), record_value(record, GODOT_KEY_CORNER_B),
                               record_value(record, GODOT_KEY_CORNER_C)};
    if (!whole(code) || code >= static_cast<double>(GODOT_TMPL_COUNT)) {
        return false;
    }
    for (const double corner : corners) {
        if (!whole(corner) || corner > static_cast<double>(UINT32_MAX)) {
            return false;
        }
    }
    out.code = static_cast<uint32_t>(code);
    out.a = static_cast<uint32_t>(corners[0]);
    out.b = static_cast<uint32_t>(corners[1]);
    out.c = static_cast<uint32_t>(corners[2]);
    return true;
}

/**
 * One seed record as a view. false for a record that misses a key or carries a value that is no finite
 * number: the seed port writes body, x, y and z of every seed (ase-pl-flora, FloraHostSeedExptSystem), so a
 * gap is a broken port, never a seed drawn at the stem base.
 */
bool seed_view_of(const KernelHostRecord& record, VivariumSeedView& out) {
    out = VivariumSeedView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    if (!whole(body) || body > static_cast<double>(UINT32_MAX) || !std::isfinite(out.x) ||
        !std::isfinite(out.y) || !std::isfinite(out.z)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    return true;
}

/**
 * One rack record as a view. false for a record that misses a key or carries a value that is no finite
 * number: the rack port writes x, y and z of every rack (ase-pl-toolrack, ToolrackHostBodyExptSystem), so a
 * gap is a broken port, never a rack drawn at the stem foot.
 */
/**
 * True when `value` is a whole number from 0 to kCampIdMax, the form of an entity or name id on a camp port. The
 * range comes first: only then does the round trip through uint32_t decide, and it holds every id exactly, where
 * ase::math::floor works in float and loses the ids above 2^24.
 */
bool camp_id(double value) {
    return value >= 0.0 && value <= kCampIdMax && static_cast<double>(static_cast<uint32_t>(value)) == value;
}

bool rack_view_of(const KernelHostRecord& record, VivariumRackView& out) {
    out = VivariumRackView{};
    out.object_id = record.object_id;
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    return std::isfinite(out.x) && std::isfinite(out.y) && std::isfinite(out.z);
}

/** The record of one object id among the first `written` records of a snapshot; nullptr when absent. */
const KernelHostRecord* sky_record(const std::vector<KernelHostRecord>& records, uint32_t written, uint32_t id) {
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id == id) {
            return &records[i];
        }
    }
    return nullptr;
}

/**
 * The four records of the sky port and its brightest moon as one view. false for a set without one of the
 * four records, a record that misses a key, or a value that is no finite number: ase-pl-sky lays all four
 * records with all their keys in one publication, and a moon record with all of its keys for every visible
 * moon (SkyHostScneExptSystem), so a gap is a broken port - never a scene lit from the origin or in black.
 * Every moon record takes the next slot of the view; without one the view stands with no moon in the sky.
 * Every comet record takes the next comet slot the same way (usecase:101274 - track the comets), every
 * planet record the next planet slot (usecase:101273 - track the neighbour planet), every companion record
 * the next companion slot (usecase:101272 - track the companion star), every fragment record the next slot
 * of the halves of Aurora (usecase:101384 - place sun and moon spatially), every belt record the next belt
 * slot (usecase:2758 - generated asteroids).
 */
bool sky_view_of(const std::vector<KernelHostRecord>& records, uint32_t written, VivariumSkyView& out) {
    out = VivariumSkyView{};
    const KernelHostRecord* sun = sky_record(records, written, GODOT_SKY_RECORD_SUN);
    const KernelHostRecord* ambient = sky_record(records, written, GODOT_SKY_RECORD_AMBIENT);
    const KernelHostRecord* background = sky_record(records, written, GODOT_SKY_RECORD_BACKGROUND);
    const KernelHostRecord* fog = sky_record(records, written, GODOT_SKY_RECORD_FOG);
    if (sun == nullptr || ambient == nullptr || background == nullptr || fog == nullptr) {
        return false;
    }
    out.sun_x = record_value(*sun, GODOT_KEY_X);
    out.sun_y = record_value(*sun, GODOT_KEY_Y);
    out.sun_z = record_value(*sun, GODOT_KEY_Z);
    out.sun_r = record_value(*sun, GODOT_KEY_R);
    out.sun_g = record_value(*sun, GODOT_KEY_G);
    out.sun_b = record_value(*sun, GODOT_KEY_B);
    out.sun_intensity = record_value(*sun, GODOT_KEY_INTENSITY);
    out.sun_size = record_value(*sun, GODOT_KEY_SIZE);
    out.sun_disc_r = record_value(*sun, GODOT_KEY_DISC_R);
    out.sun_disc_g = record_value(*sun, GODOT_KEY_DISC_G);
    out.sun_disc_b = record_value(*sun, GODOT_KEY_DISC_B);
    out.ambient_r = record_value(*ambient, GODOT_KEY_R);
    out.ambient_g = record_value(*ambient, GODOT_KEY_G);
    out.ambient_b = record_value(*ambient, GODOT_KEY_B);
    out.ambient_intensity = record_value(*ambient, GODOT_KEY_INTENSITY);
    out.background_r = record_value(*background, GODOT_KEY_R);
    out.background_g = record_value(*background, GODOT_KEY_G);
    out.background_b = record_value(*background, GODOT_KEY_B);
    out.fog_r = record_value(*fog, GODOT_KEY_R);
    out.fog_g = record_value(*fog, GODOT_KEY_G);
    out.fog_b = record_value(*fog, GODOT_KEY_B);
    out.fog_near = record_value(*fog, GODOT_KEY_NEAR);
    out.fog_far = record_value(*fog, GODOT_KEY_FAR);
    out.fog_shimmer = record_value(*fog, GODOT_KEY_SHIMMER);
    const double values[] = {out.sun_x,        out.sun_y,        out.sun_z,         out.sun_r,
                             out.sun_g,        out.sun_b,        out.sun_intensity, out.sun_size,
                             out.sun_disc_r,   out.sun_disc_g,   out.sun_disc_b,    out.ambient_r,
                             out.ambient_g,    out.ambient_b,    out.ambient_intensity,
                             out.background_r, out.background_g, out.background_b,  out.fog_r,
                             out.fog_g,        out.fog_b,        out.fog_near,      out.fog_far,
                             out.fog_shimmer};
    for (const double value : values) {
        if (!std::isfinite(value)) {
            out = VivariumSkyView{};
            return false;
        }
    }
    // the moons: every moon record must be whole, and each takes the next slot - its own light and disc
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_MOON) {
            continue;
        }
        if (out.moons >= GODOT_SKY_MOONS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& moon = records[i];
        const double moon_values[] = {record_value(moon, GODOT_KEY_X), record_value(moon, GODOT_KEY_Y),
                                      record_value(moon, GODOT_KEY_Z), record_value(moon, GODOT_KEY_R),
                                      record_value(moon, GODOT_KEY_G), record_value(moon, GODOT_KEY_B),
                                      record_value(moon, GODOT_KEY_INTENSITY), record_value(moon, GODOT_KEY_SIZE),
                                      record_value(moon, GODOT_KEY_DISC_R),    record_value(moon, GODOT_KEY_DISC_G),
                                      record_value(moon, GODOT_KEY_DISC_B)};
        for (const double value : moon_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumMoonView& slot = out.moon[out.moons];
        slot.x = moon_values[0];
        slot.y = moon_values[1];
        slot.z = moon_values[2];
        slot.r = moon_values[3];
        slot.g = moon_values[4];
        slot.b = moon_values[5];
        slot.intensity = moon_values[6];
        slot.size = moon_values[7];
        // the disc colour ase-pl-sky files: the moonlight darkened by the shadow of a lunar eclipse (usecase:101276)
        slot.disc_r = moon_values[8];
        slot.disc_g = moon_values[9];
        slot.disc_b = moon_values[10];
        out.moons += 1u;
    }
    // the comets: every comet record must be whole, and each takes the next slot - its head and its tail
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_COMET) {
            continue;
        }
        if (out.comets >= GODOT_SKY_COMETS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& comet = records[i];
        const double comet_values[] = {record_value(comet, GODOT_KEY_X),      record_value(comet, GODOT_KEY_Y),
                                       record_value(comet, GODOT_KEY_Z),      record_value(comet, GODOT_KEY_INTENSITY),
                                       record_value(comet, GODOT_KEY_TAIL_X), record_value(comet, GODOT_KEY_TAIL_Y),
                                       record_value(comet, GODOT_KEY_TAIL_Z), record_value(comet, GODOT_KEY_LENGTH)};
        for (const double value : comet_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumCometView& slot = out.comet[out.comets];
        slot.x = comet_values[0];
        slot.y = comet_values[1];
        slot.z = comet_values[2];
        slot.intensity = comet_values[3];
        slot.tail_x = comet_values[4];
        slot.tail_y = comet_values[5];
        slot.tail_z = comet_values[6];
        slot.length = comet_values[7];
        out.comets += 1u;
    }
    // the planets: every planet record must be whole, and each takes the next slot - its disc
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_PLANET) {
            continue;
        }
        if (out.planets >= GODOT_SKY_PLANETS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& planet = records[i];
        const double planet_values[] = {record_value(planet, GODOT_KEY_X), record_value(planet, GODOT_KEY_Y),
                                        record_value(planet, GODOT_KEY_Z), record_value(planet, GODOT_KEY_SIZE),
                                        record_value(planet, GODOT_KEY_PHASE)};
        for (const double value : planet_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumPlanetView& slot = out.planet[out.planets];
        slot.x = planet_values[0];
        slot.y = planet_values[1];
        slot.z = planet_values[2];
        slot.size = planet_values[3];
        slot.phase = planet_values[4];
        out.planets += 1u;
    }
    // the companion stars: every companion record must be whole, and each takes the next slot - its disc
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_COMPANION) {
            continue;
        }
        if (out.companions >= GODOT_SKY_COMPANIONS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& companion = records[i];
        const double companion_values[] = {record_value(companion, GODOT_KEY_X),    record_value(companion, GODOT_KEY_Y),
                                           record_value(companion, GODOT_KEY_Z),    record_value(companion, GODOT_KEY_SIZE),
                                           record_value(companion, GODOT_KEY_INTENSITY)};
        for (const double value : companion_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumCompanionView& slot = out.companion[out.companions];
        slot.x = companion_values[0];
        slot.y = companion_values[1];
        slot.z = companion_values[2];
        slot.size = companion_values[3];
        slot.intensity = companion_values[4];
        out.companions += 1u;
    }
    // the moon halves: every fragment record must be whole, and each takes the next slot - its disc
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_FRAGMENT) {
            continue;
        }
        if (out.fragments >= GODOT_SKY_FRAGMENTS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& fragment = records[i];
        const double fragment_values[] = {record_value(fragment, GODOT_KEY_X),    record_value(fragment, GODOT_KEY_Y),
                                          record_value(fragment, GODOT_KEY_Z),    record_value(fragment, GODOT_KEY_SIZE),
                                          record_value(fragment, GODOT_KEY_INTENSITY)};
        for (const double value : fragment_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumFragmentView& slot = out.fragment[out.fragments];
        slot.x = fragment_values[0];
        slot.y = fragment_values[1];
        slot.z = fragment_values[2];
        slot.size = fragment_values[3];
        slot.intensity = fragment_values[4];
        out.fragments += 1u;
    }
    // the asteroid belts: every belt record must be whole, and each takes the next slot - its grains
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_BELT) {
            continue;
        }
        if (out.belts >= GODOT_SKY_BELTS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& belt = records[i];
        const double belt_values[] = {record_value(belt, GODOT_KEY_X),         record_value(belt, GODOT_KEY_Y),
                                      record_value(belt, GODOT_KEY_Z),         record_value(belt, GODOT_KEY_SPAN),
                                      record_value(belt, GODOT_KEY_INTENSITY), record_value(belt, GODOT_KEY_DENSITY)};
        for (const double value : belt_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumBeltView& slot = out.belt[out.belts];
        slot.x = belt_values[0];
        slot.y = belt_values[1];
        slot.z = belt_values[2];
        slot.span = belt_values[3];
        slot.intensity = belt_values[4];
        slot.density = belt_values[5];
        out.belts += 1u;
    }
    // the meteor showers: every shower record must be whole, and each takes the next slot - its streaks
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_SHOWER) {
            continue;
        }
        if (out.showers >= GODOT_SKY_SHOWERS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& shower = records[i];
        const double shower_values[] = {record_value(shower, GODOT_KEY_X),         record_value(shower, GODOT_KEY_Y),
                                        record_value(shower, GODOT_KEY_Z),         record_value(shower, GODOT_KEY_INTENSITY),
                                        record_value(shower, GODOT_KEY_RATE)};
        for (const double value : shower_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumShowerView& slot = out.shower[out.showers];
        slot.x = shower_values[0];
        slot.y = shower_values[1];
        slot.z = shower_values[2];
        slot.intensity = shower_values[3];
        slot.rate = shower_values[4];
        out.showers += 1u;
    }
    // the eclipse: a standing eclipse record must be whole and takes the next slot - the jump to an eclipse
    // reads its kind and the hours to its maximum (usecase:101276 - predict the eclipses)
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_ECLIPSE) {
            continue;
        }
        if (out.eclipses >= GODOT_SKY_ECLIPSES_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& eclipse = records[i];
        const double eclipse_values[] = {record_value(eclipse, GODOT_KEY_KIND), record_value(eclipse, GODOT_KEY_OBSCURATION),
                                         record_value(eclipse, GODOT_KEY_HOURS_TO_MAX), record_value(eclipse, GODOT_KEY_DURATION)};
        for (const double value : eclipse_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumEclipseView& slot = out.eclipse[out.eclipses];
        slot.kind = eclipse_values[0];
        slot.obscuration = eclipse_values[1];
        slot.hours_to_max = eclipse_values[2];
        slot.duration = eclipse_values[3];
        out.eclipses += 1u;
    }
    // the predicted eclipses: an eclipse next record must be whole and takes the next slot - the jump to an
    // eclipse seeks the clock by its game hours (usecase:101276 - predict the eclipses)
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_ECLIPSE_NEXT) {
            continue;
        }
        if (out.eclipse_nexts >= GODOT_SKY_ECLIPSE_NEXTS_MAX) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& prediction = records[i];
        const double next_values[] = {record_value(prediction, GODOT_KEY_NEXT_SOLAR),
                                      record_value(prediction, GODOT_KEY_NEXT_LUNAR),
                                      record_value(prediction, GODOT_KEY_NEXT_MOONRISE)};
        for (const double value : next_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        VivariumEclipseNextView& slot = out.eclipse_next[out.eclipse_nexts];
        slot.next_solar = next_values[0];
        slot.next_lunar = next_values[1];
        slot.next_moonrise = next_values[2];
        out.eclipse_nexts += 1u;
    }
    // the turn of the star sphere: at most one record, whole - the starfield turns by it every frame, and its
    // marks tell whether the stars port carries a new set (the stars themselves stand on the stars port)
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_TURN) {
            continue;
        }
        if (out.turn_known) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& turn = records[i];
        const double turn_values[] = {record_value(turn, GODOT_KEY_W),           record_value(turn, GODOT_KEY_X),
                                      record_value(turn, GODOT_KEY_Y),           record_value(turn, GODOT_KEY_Z),
                                      record_value(turn, GODOT_KEY_SET_SIDEREAL), record_value(turn, GODOT_KEY_SET_ZENITH),
                                      record_value(turn, GODOT_KEY_SET_MOON)};
        for (const double value : turn_values) {
            if (!std::isfinite(value)) {
                out = VivariumSkyView{};
                return false;
            }
        }
        out.turn_w = turn_values[0];
        out.turn_x = turn_values[1];
        out.turn_y = turn_values[2];
        out.turn_z = turn_values[3];
        out.set_sidereal = turn_values[4];
        out.set_zenith = turn_values[5];
        out.set_moon = turn_values[6];
        out.turn_known = true;
    }
    // the wind drift (ase-pl-sky SKY_HOST_RECORD_WIND): at most one record, whole and inside its ranges - the
    // haze and the cloud banks move along it. ase-pl-sky files it only once wind arrived; a set without it
    // leaves wind_known false here, and the read keeps the drift of the last set that carried one.
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        if (records[i].object_id != GODOT_SKY_RECORD_WIND) {
            continue;
        }
        if (out.wind_known) {
            out = VivariumSkyView{};
            return false;
        }
        const KernelHostRecord& wind = records[i];
        const double drift_x = record_value(wind, GODOT_KEY_DRIFT_X);
        const double drift_z = record_value(wind, GODOT_KEY_DRIFT_Z);
        const double share = record_value(wind, GODOT_KEY_WIND);
        const bool drift_inside = drift_x >= GODOT_SKY_DRIFT_MIN && drift_x <= GODOT_SKY_DRIFT_MAX &&
                                  drift_z >= GODOT_SKY_DRIFT_MIN && drift_z <= GODOT_SKY_DRIFT_MAX;
        const bool share_inside = share >= GODOT_SKY_WIND_MIN && share <= GODOT_SKY_WIND_MAX;
        if (!drift_inside || !share_inside) {
            out = VivariumSkyView{};
            return false;
        }
        out.drift_x = drift_x;
        out.drift_z = drift_z;
        out.wind = share;
        out.wind_known = true;
    }
    out.known = true;
    return true;
}

/**
 * The visible star set out of the records of the stars port (usecase:5606 - observe the sky): every record
 * must be a whole star record, at most GODOT_SKY_STARS_MAX of them; its place is normalised to a unit
 * direction on the celestial sphere. Moved here from sky_view_of, where the stars rode on the scene port
 * and were read with every snapshot. On any broken record out is left empty and the answer is false.
 */
bool stars_view_of(const std::vector<KernelHostRecord>& records, uint32_t written, std::vector<VivariumStarView>& out) {
    out.clear();
    for (uint32_t i = 0u; i < written && i < records.size(); ++i) {
        const KernelHostRecord& star = records[i];
        if (star.object_id != GODOT_SKY_RECORD_STAR || out.size() >= GODOT_SKY_STARS_MAX) {
            out.clear();
            return false;
        }
        const double star_values[] = {record_value(star, GODOT_KEY_X), record_value(star, GODOT_KEY_Y),
                                      record_value(star, GODOT_KEY_Z), record_value(star, GODOT_KEY_R),
                                      record_value(star, GODOT_KEY_G), record_value(star, GODOT_KEY_B),
                                      record_value(star, GODOT_KEY_POINT)};
        for (const double value : star_values) {
            if (!std::isfinite(value)) {
                out.clear();
                return false;
            }
        }
        const float square = static_cast<float>(star_values[0] * star_values[0] + star_values[1] * star_values[1] +
                                                star_values[2] * star_values[2]);
        const double length = static_cast<double>(ase::math::sqrt(square));
        if (!(length > 0.0)) {
            out.clear();
            return false;
        }
        VivariumStarView slot;
        slot.x = star_values[0] / length;
        slot.y = star_values[1] / length;
        slot.z = star_values[2] / length;
        slot.r = star_values[3];
        slot.g = star_values[4];
        slot.b = star_values[5];
        slot.point = star_values[6];
        out.push_back(slot);
    }
    return true;
}

/**
 * The ports and operations every host of the Vivarium needs: patch, forecast, stem, autumn, cap, clock, date, scene, ground, brook,
 * terrain, ground cell, the six organ ports, the five instance ports, the sky port, the stars port, the seed port, the
 * tile port, the sway port, the stream port, the level, the flatten, the lower, the raise and the elevation port and the two
 * template ports, irrigate, prune, level, flatten, lower, raise, shape and lay - every port boot()
 * asks the set for. An
 * empty one would surface later as a NotFound of the first call, or as the port step of a boot that a
 * configuration check could have refused before a host existed.
 */
bool ports_named(const VivariumStartConfig& config) {
    bool named = !config.patch_port.empty() && !config.forecast_port.empty() && !config.soil_port.empty() &&
                 !config.cond_port.empty() &&
                 !config.stage_port.empty() &&
                 !config.stem_port.empty() &&
                 !config.autumn_port.empty() &&
                 !config.cap_port.empty() &&
                 !config.clock_port.empty() &&
                 !config.date_port.empty() &&
                 !config.scene_port.empty() &&
                 !config.ground_port.empty() && !config.brook_port.empty() && !config.terrain_port.empty() &&
                 !config.ground_cell_port.empty() && !config.sky_port.empty() && !config.stars_port.empty() &&
                 !config.seed_port.empty() &&
                 !config.tile_port.empty() && !config.lineage_port.empty() &&
                 !config.sway_port.empty() && !config.bend_port.empty() &&
                 !config.tint_port.empty() && !config.atlas_port.empty() &&
                 !config.bark_port.empty() &&
                 !config.knot_port.empty() && !config.foot_port.empty() && !config.scar_port.empty() &&
                 !config.stream_port.empty() &&
                 !config.level_port.empty() &&
                 !config.flatten_port.empty() && !config.lower_port.empty() && !config.raise_port.empty() &&
                 !config.op_lower.empty() && !config.op_raise.empty() &&
                 !config.elevation_port.empty() && !config.op_shape.empty() &&
                 !config.template_corner_port.empty() && !config.template_triangle_port.empty() &&
                 !config.template_tangent_port.empty() &&
                 !config.op_irrigate.empty() &&
                 !config.op_prune.empty() && !config.op_level.empty() && !config.op_flatten.empty() &&
                 !config.op_lay.empty() && !config.real_port.empty() && !config.op_real.empty();
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        named = named && !config.organ_ports[c].empty();
    }
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        named = named && !config.instance_ports[c].empty();
    }
    return named;
}

/**
 * The patches of a start configuration the game host can lay: at least one, each with a species and
 * a known ground, every place once. ase-pl-flora places what it is asked for; that a place names one
 * patch is a property of the configuration and is checked where it is read, here.
 */
bool patches_layable(const std::vector<VivariumPatchSpec>& patches) {
    if (patches.empty()) {
        return false;
    }
    for (size_t i = 0u; i < patches.size(); ++i) {
        if (patches[i].species.empty() || patches[i].soil >= GODOT_SOIL_COUNT) {
            return false;
        }
        for (size_t j = i + 1u; j < patches.size(); ++j) {
            if (patches[i].place == patches[j].place) {
                return false;
            }
        }
    }
    return true;
}

/** The lay request of one patch: place, species id, seed, ground, where it lies in the world, the field
 * capacity of its soil, its gravity and the Startstand of its plant; ase-pl-flora places nothing without
 * all ten. */
KernelHostInput lay_input(const VivariumPatchSpec& patch) {
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_PLACE, static_cast<double>(patch.place));
    (void)put_value(input, GODOT_KEY_SPECIES,
                    static_cast<double>(KernelEmbeddedHost::name_id(patch.species.c_str())));
    (void)put_value(input, GODOT_KEY_SEED, static_cast<double>(patch.seed));
    (void)put_value(input, GODOT_KEY_SOIL, static_cast<double>(patch.soil));
    (void)put_value(input, GODOT_KEY_X, patch.x);
    (void)put_value(input, GODOT_KEY_Z, patch.z);
    (void)put_value(input, GODOT_KEY_HALF, patch.half_m);
    (void)put_value(input, GODOT_KEY_CAPACITY_MM, patch.capacity_mm);
    (void)put_value(input, GODOT_KEY_GRAVITY_REL, patch.gravity_rel);
    (void)put_value(input, GODOT_KEY_AGE_DAYS, patch.age_days);
    return input;
}

/** The lay request of the ground square: its centre, half edge and sample step. */
KernelHostInput ground_input(const VivariumGroundSpec& ground) {
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_X, ground.x);
    (void)put_value(input, GODOT_KEY_Z, ground.z);
    (void)put_value(input, GODOT_KEY_HALF, ground.half_m);
    (void)put_value(input, GODOT_KEY_STEP, ground.step_m);
    return input;
}

/** The lay request of one brook: the two ends of its bed, its half width and depth, its water. */
KernelHostInput brook_input(const VivariumBrookSpec& brook) {
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_X, brook.x);
    (void)put_value(input, GODOT_KEY_Z, brook.z);
    (void)put_value(input, GODOT_KEY_END_X, brook.end_x);
    (void)put_value(input, GODOT_KEY_END_Z, brook.end_z);
    (void)put_value(input, GODOT_KEY_HALF, brook.half_m);
    (void)put_value(input, GODOT_KEY_DEPTH, brook.depth_m);
    (void)put_value(input, GODOT_KEY_BASE, brook.base_m3_s);
    (void)put_value(input, GODOT_KEY_CATCHMENT, brook.catchment_m2);
    return input;
}

/** The words of one index value, split at blanks. */
std::vector<std::string> fields_of(const std::string& value) {
    std::vector<std::string> fields;
    std::string field;
    for (const char c : value) {
        if (c == ' ' || c == '\t') {
            if (!field.empty()) {
                fields.push_back(field);
                field.clear();
            }
        } else {
            field.push_back(c);
        }
    }
    if (!field.empty()) {
        fields.push_back(field);
    }
    return fields;
}

// THE SELF-TEST (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3)

/** One number of a report line: fixed decimals; nan and inf stay words (ase-utils). */
std::string decimal(double value) {
    char text[GODOT_NUMBER_BYTES] = {};
    ase::utils::str_append_f32(text, GODOT_NUMBER_BYTES, static_cast<float>(value),
                               GODOT_SELFTEST_DECIMALS);
    return std::string(text);
}

/** A scenario's label in the report: the runs its actions check, each once. */
std::string scenario_label(const toml::table& scenario) {
    std::vector<std::string> checks;
    const toml::array* actions = scenario[GODOT_SELFTEST_KEY_DO].as_array();
    if (actions != nullptr) {
        for (const toml::node& action : *actions) {
            const toml::table* entry = action.as_table();
            const std::string check =
                entry == nullptr ? std::string() : (*entry)[GODOT_SELFTEST_KEY_CHECK].value_or(std::string());
            bool known = check.empty();
            for (const std::string& seen : checks) {
                known = known || seen == check;
            }
            if (!known) {
                checks.push_back(check);
            }
        }
    }
    std::string label;
    for (const std::string& check : checks) {
        label += label.empty() ? check : " " + check;
    }
    return label.empty() ? std::string("?") : label;
}

/** Every record the port holds; the buffer grows once to the port's own answer. */
HostStatus snapshot_all(KernelEmbeddedHost& host, const std::string& port,
                        std::vector<KernelHostRecord>& records, uint32_t& written) {
    uint32_t required = 0u;
    written = 0u;
    if (records.empty()) {
        records.assign(ase::kernel::HostRecordsMax, KernelHostRecord{});
    }
    HostStatus status = host.snapshot(port.c_str(), records.data(), static_cast<uint32_t>(records.size()),
                                      &written, &required);
    if (status == ase::kernel::HostStatusCapacity) {
        records.assign(required, KernelHostRecord{});
        status = host.snapshot(port.c_str(), records.data(), static_cast<uint32_t>(records.size()),
                               &written, &required);
    }
    return status;
}

/**
 * A refused record names itself. Every check of a snapshot read that refuses the frame notes its port,
 * the object id of the record and the check it failed before it hands InvalidArgument back; fail()
 * alone can only say that the snapshot was refused, never which of the reads refused it.
 */
HostStatus refuse_record(KernelEmbeddedHost& host, const std::string& port, uint32_t object_id,
                         const char* check) {
    std::string line = "snapshot refused at port ";
    line += port;
    line += " object ";
    line += std::to_string(object_id);
    line += ": ";
    line += check;
    (void)host.note(GODOT_LOG_SOURCE, line.c_str());
    return ase::kernel::HostStatusInvalidArgument;
}

/** One comparison of the self-test: counted, and written as one report line. */
void report_check(const std::string& check, const std::string& subject, const std::string& actual,
                  const std::string& expected, bool ok, VivariumSelfTestResult& result,
                  std::vector<std::string>& report) {
    result.checks += 1u;
    if (!ok) {
        result.errors += 1u;
    }
    report.push_back("selftest check=" + (check.empty() ? std::string("?") : check) + " " + subject +
                     " actual=" + actual + " expected=" + expected + " result=" + (ok ? "ok" : "FAILED"));
}

/** A scenario or a table that could not run to its end: one error, the step and its cause. */
void report_broken(const std::string& label, const char* step, const std::string& cause,
                   VivariumSelfTestResult& result, std::vector<std::string>& report) {
    result.errors += 1u;
    report.push_back("selftest scenario=\"" + label + "\" step=" + step + " cause=" + cause +
                     " result=FAILED");
}

/** op of the table applied; `known` false for an op the form does not have. */
bool holds(double actual, const std::string& op, double value, double tolerance, bool& known) {
    known = true;
    if (op == GODOT_SELFTEST_OP_EQ) {
        return std::fabs(actual - value) <= tolerance;
    }
    if (op == GODOT_SELFTEST_OP_NE) {
        return std::fabs(actual - value) > tolerance;
    }
    if (op == GODOT_SELFTEST_OP_LT) {
        return actual < value;
    }
    if (op == GODOT_SELFTEST_OP_LE) {
        return actual <= value;
    }
    if (op == GODOT_SELFTEST_OP_GT) {
        return actual > value;
    }
    if (op == GODOT_SELFTEST_OP_GE) {
        return actual >= value;
    }
    known = false;
    return false;
}

/** The record of the patch on `place` in the last patch snapshot; nullptr when no record carries it. */
const KernelHostRecord* record_at(const std::vector<KernelHostRecord>& records, uint32_t written, int64_t place) {
    for (uint32_t i = 0u; i < written; ++i) {
        if (record_value(records[i], GODOT_KEY_PLACE) == static_cast<double>(place)) {
            return &records[i];
        }
    }
    return nullptr;
}

/**
 * The value an action names: a patch's (place) or the clock's (record = "clock"). NaN when the
 * snapshot fails or carries no such record or key - every comparison with NaN fails, so a value
 * that is not there is never a value that passes.
 */
double read_value(KernelEmbeddedHost& host, const VivariumStartConfig& config, const toml::table& spec,
                  VivariumSelfTestState& state) {
    const std::string key = spec[GODOT_SELFTEST_KEY_KEY].value_or(std::string());
    uint32_t written = 0u;
    if (spec[GODOT_SELFTEST_KEY_RECORD].value_or(std::string()) == GODOT_SELFTEST_RECORD_CLOCK) {
        if (snapshot_all(host, config.clock_port, state.records, written) != ase::kernel::HostStatusOk ||
            written != 1u) {
            return std::nan("");
        }
        return record_value(state.records[0], key.c_str());
    }
    // the soil water stands on the soil port (usecase:101340), one record per patch that carries its place
    const std::string& port = spec[GODOT_SELFTEST_KEY_RECORD].value_or(std::string()) == GODOT_SELFTEST_RECORD_SOIL
                                  ? config.soil_port
                                  : config.patch_port;
    if (snapshot_all(host, port, state.records, written) != ase::kernel::HostStatusOk) {
        return std::nan("");
    }
    const KernelHostRecord* record =
        record_at(state.records, written, spec[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1}));
    return record == nullptr ? std::nan("") : record_value(*record, key.c_str());
}

/** The object id of the patch on `place`; GODOT_NO_OBJECT when the snapshot carries none. */
uint32_t object_at(KernelEmbeddedHost& host, const VivariumStartConfig& config, int64_t place,
                   VivariumSelfTestState& state) {
    uint32_t written = 0u;
    if (snapshot_all(host, config.patch_port, state.records, written) != ase::kernel::HostStatusOk) {
        return GODOT_NO_OBJECT;
    }
    const KernelHostRecord* record = record_at(state.records, written, place);
    return record == nullptr ? GODOT_NO_OBJECT : record->object_id;
}

/** value of an action plus the remembered value its `plus` refers to; NaN for an unknown one. */
double target_of(const toml::table& spec, const VivariumSelfTestState& state) {
    double value = spec[GODOT_SELFTEST_KEY_VALUE].value_or(std::nan(""));
    const std::string plus = spec[GODOT_SELFTEST_KEY_PLUS].value_or(std::string());
    if (!plus.empty()) {
        const auto found = state.remembered.find(plus);
        value = found == state.remembered.end() ? std::nan("") : value + found->second;
    }
    return value;
}

HostStatus pour(KernelEmbeddedHost& host, const VivariumStartConfig& config, uint32_t object_id, double amount_mm) {
    KernelHostInput input{};
    input.object_id = object_id;
    if (!put_value(input, GODOT_KEY_AMOUNT_MM, amount_mm)) {
        return ase::kernel::HostStatusCapacity;
    }
    return host.submit(config.patch_port.c_str(), config.op_irrigate.c_str(), &input);
}

/** File the absolute real position on the real port; the plugin hands it to ase-time in its next Reception run. */
HostStatus file_real(KernelEmbeddedHost& host, const VivariumStartConfig& config, double real_s) {
    KernelHostInput input{};
    if (!put_value(input, GODOT_KEY_REAL_S, real_s)) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    return host.submit(config.real_port.c_str(), config.op_real.c_str(), &input);
}

/** The real position the clock record says ase-time laid its clock to; NaN while the record carries none. */
double laid_real(KernelEmbeddedHost& host, const VivariumStartConfig& config, VivariumSelfTestState& state) {
    uint32_t written = 0u;
    if (snapshot_all(host, config.clock_port, state.records, written) != ase::kernel::HostStatusOk ||
        written != 1u) {
        return std::nan("");
    }
    return record_value(state.records[0], GODOT_KEY_REAL_LAID);
}

/**
 * One frame of real time - exactly one ordinary tick, after which the real position moved on by
 * the frame is filed, as the game host files it. false when the host refuses the frame.
 */
bool frame(KernelEmbeddedHost& host, const VivariumStartConfig& config, VivariumSelfTestState& state) {
    uint32_t steps = 0u;
    if (host.advance(state.frame_s, &steps) != ase::kernel::HostStatusOk || steps != 1u) {
        return false;
    }
    state.real_s += state.frame_s;
    return file_real(host, config, state.real_s) == ase::kernel::HostStatusOk;
}

/** The host port of the start file's port name `name` the digest of the camp walk folds; empty for another name. */
std::string camp_walk_port(const VivariumStartConfig& config, const std::string& name) {
    return name == GODOT_SELFTEST_PORT_FIRE        ? config.survival_fire_port
           : name == GODOT_SELFTEST_PORT_CAMP_BODY ? config.camp_body_port
                                                   : std::string();
}

/** The FNV-1a digest `digest` with `bytes` bytes at `data` folded in, byte by byte. */
uint64_t fold_bytes(uint64_t digest, const void* data, size_t bytes) {
    const unsigned char* at = static_cast<const unsigned char*>(data);
    for (size_t i = 0u; i < bytes; ++i) {
        digest ^= static_cast<uint64_t>(at[i]);
        digest *= GODOT_WALK_DIGEST_PRIME;
    }
    return digest;
}

/** The state of one record the walk folds: every key up to its end with the bits of its value, in the record's
 *  order, without the keys `ids` names. The object id and those keys carry an entity id of the host registry,
 *  which follows the order the run made entities and means nothing on another host. */
std::vector<std::pair<std::string, uint64_t>> walk_state(const KernelHostRecord& record,
                                                         const std::vector<std::string>& ids) {
    std::vector<std::pair<std::string, uint64_t>> state;
    for (uint32_t i = 0u; i < record.value_count; ++i) {
        size_t length = 0u;
        while (length < ase::kernel::HostKeyBytes && record.values[i].key[length] != '\0') {
            ++length;
        }
        std::string key(record.values[i].key, length);
        if (std::find(ids.begin(), ids.end(), key) == ids.end()) {
            state.emplace_back(std::move(key), std::bit_cast<uint64_t>(record.values[i].value));
        }
    }
    return state;
}

/** The digest with the first `written` records folded in, each as its walk_state, the records in the order of
 *  those states - the fold of the Linux reader (avt_headless_camp.cpp, fold_records), so the two lines "camp walk
 *  digest" carry the same bits when the two hosts reported the same state, whatever ids their registries gave. */
uint64_t fold_snapshot(uint64_t digest, const std::vector<KernelHostRecord>& snapshot, uint32_t written,
                       const std::vector<std::string>& ids) {
    std::vector<KernelHostRecord> records(snapshot.begin(), snapshot.begin() + written);
    std::vector<std::vector<std::pair<std::string, uint64_t>>> states;
    states.reserve(records.size());
    for (const KernelHostRecord& record : records) {
        states.push_back(walk_state(record, ids));
    }
    std::sort(states.begin(), states.end());
    for (const std::vector<std::pair<std::string, uint64_t>>& state : states) {
        for (const std::pair<std::string, uint64_t>& value : state) {
            digest = fold_bytes(digest, value.first.data(), value.first.size());
            digest = fold_bytes(digest, &value.second, sizeof(value.second));
        }
    }
    return digest;
}

/** `value` in lower-case hexadecimal without leading zeros, as std::hex prints it on the Linux reader. */
std::string hex_of(uint64_t value) {
    constexpr const char* digits = "0123456789abcdef";
    std::string text;
    do {
        text.insert(text.begin(), digits[value & 0xfull]);
        value >>= 4u;
    } while (value != 0u);
    return text;
}

/**
 * One camp action of the table (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.10): camp, hull, part or digest, through
 * the same ports and with the same values as the Linux reader (clients/avt-client-android/tests/avt_headless_camp.cpp)
 * and as the game host lays its camp (lay_camp, file_hull, file_part). `handled` false when the action names none
 * of them; the answer is that of run_action.
 */
bool run_camp_action(KernelEmbeddedHost& host, const toml::table& action, const VivariumStartConfig& config,
                     const std::string& label, VivariumSelfTestState& state, VivariumSelfTestResult& result,
                     std::vector<std::string>& report, bool& handled) {
    handled = true;
    const std::string check = action[GODOT_SELFTEST_KEY_CHECK].value_or(std::string());
    if (const toml::table* camp = action[GODOT_SELFTEST_KEY_CAMP].as_table()) {
        const int64_t place = (*camp)[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1});
        const double east = (*camp)[GODOT_SELFTEST_KEY_X].value_or(std::nan(""));
        const double north = (*camp)[GODOT_SELFTEST_KEY_Z].value_or(std::nan(""));
        const std::string expected = (*camp)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        if (place < 0 || place > static_cast<int64_t>(UINT32_MAX) || !std::isfinite(east) || !std::isfinite(north) ||
            expected.empty() || config.camp_place_port.empty()) {
            report_broken(label, GODOT_SELFTEST_KEY_CAMP, "needs place, a finite x and z, status and the camp_place port",
                          result, report);
            return false;
        }
        KernelHostInput input{};
        input.object_id = static_cast<uint32_t>(place);
        (void)put_value(input, kCampKeyEast, east);
        (void)put_value(input, kCampKeyNorth, north);
        const std::string actual = GodotHostResourceManager::status_name(
            host.submit(config.camp_place_port.c_str(), config.op_lay.c_str(), &input));
        report_check(check, "place=" + std::to_string(place) + " action=camp", actual, expected, actual == expected,
                     result, report);
        return true;
    }
    if (const toml::table* hull = action[GODOT_SELFTEST_KEY_HULL].as_table()) {
        const std::string make = (*hull)[GODOT_SELFTEST_KEY_MAKE].value_or(std::string());
        const toml::array* corners = (*hull)[GODOT_SELFTEST_KEY_CORNERS].as_array();
        const std::string expected = (*hull)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        if (make.empty() || corners == nullptr || corners->empty() || expected.empty() ||
            config.camp_hull_port.empty()) {
            report_broken(label, GODOT_SELFTEST_KEY_HULL, "needs make, corners, status and the camp_hull port", result,
                          report);
            return false;
        }
        const double make_id = static_cast<double>(KernelEmbeddedHost::name_id(make.c_str()));
        const char* const axes[GODOT_SELFTEST_HULL_AXES] = {kCampKeyHullX, kCampKeyHullY, kCampKeyHullZ};
        for (size_t corner = 0u; corner < corners->size(); ++corner) {
            const toml::array* at = (*corners)[corner].as_array();
            if (at == nullptr || at->size() != GODOT_SELFTEST_HULL_AXES) {
                report_broken(label, GODOT_SELFTEST_KEY_HULL, "a corner is no [hx, hy, hz]", result, report);
                return false;
            }
            KernelHostInput input{};
            input.object_id = static_cast<uint32_t>(corner);
            (void)put_value(input, kCampKeyMake, make_id);
            for (uint32_t axis = 0u; axis < GODOT_SELFTEST_HULL_AXES; ++axis) {
                const double value = (*at)[axis].value_or(std::nan(""));
                if (!std::isfinite(value)) {
                    report_broken(label, GODOT_SELFTEST_KEY_HULL, "a corner carries no finite number", result, report);
                    return false;
                }
                (void)put_value(input, axes[axis], value);
            }
            const std::string actual = GodotHostResourceManager::status_name(
                host.submit(config.camp_hull_port.c_str(), config.op_lay.c_str(), &input));
            report_check(check, "corner=" + std::to_string(corner) + " action=hull make=" + make, actual, expected,
                         actual == expected, result, report);
        }
        return true;
    }
    if (const toml::table* part = action[GODOT_SELFTEST_KEY_PART].as_table()) {
        const std::string make = (*part)[GODOT_SELFTEST_KEY_MAKE].value_or(std::string());
        const int64_t place = (*part)[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1});
        const double east = (*part)[GODOT_SELFTEST_KEY_X].value_or(std::nan(""));
        const double north = (*part)[GODOT_SELFTEST_KEY_Z].value_or(std::nan(""));
        const double drop = (*part)[GODOT_SELFTEST_KEY_DROP].value_or(std::nan(""));
        const double fall = (*part)[GODOT_SELFTEST_KEY_FALL].value_or(std::nan(""));
        const double mass = (*part)[GODOT_SELFTEST_KEY_MASS].value_or(std::nan(""));
        const double rough = (*part)[GODOT_SELFTEST_KEY_ROUGH].value_or(std::nan(""));
        const double turn_x = (*part)[GODOT_SELFTEST_KEY_TURN_X].value_or(std::nan(""));
        const double turn_y = (*part)[GODOT_SELFTEST_KEY_TURN_Y].value_or(std::nan(""));
        const double turn_z = (*part)[GODOT_SELFTEST_KEY_TURN_Z].value_or(std::nan(""));
        const bool fuel = (*part)[GODOT_SELFTEST_KEY_FUEL].value_or(false);
        const std::string expected = (*part)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        const uint32_t patch = object_at(host, config, place, state);
        if (make.empty() || patch == GODOT_NO_OBJECT || !std::isfinite(east) || !std::isfinite(north) ||
            !std::isfinite(drop) || !std::isfinite(fall) || !(mass > 0.0) || !(rough >= 0.0) ||
            !std::isfinite(turn_x) || !std::isfinite(turn_y) || !std::isfinite(turn_z) || expected.empty() ||
            config.camp_part_port.empty()) {
            report_broken(label, GODOT_SELFTEST_KEY_PART,
                          "needs make, the place of a laid patch, finite x, z, drop, fall, tx, ty and tz, a positive "
                          "mass, a roughness of at least 0, status and the camp_part port",
                          result, report);
            return false;
        }
        KernelHostInput input{};
        input.object_id = patch;
        (void)put_value(input, kCampKeyEast, east);
        (void)put_value(input, kCampKeyNorth, north);
        (void)put_value(input, kCampKeyDrop, drop);
        (void)put_value(input, kCampKeyFall, fall);
        (void)put_value(input, kCampKeyMass, mass);
        (void)put_value(input, kCampKeyMake, static_cast<double>(KernelEmbeddedHost::name_id(make.c_str())));
        (void)put_value(input, kCampKeyRough, rough);
        (void)put_value(input, kCampKeyTurnX, turn_x);
        (void)put_value(input, kCampKeyTurnY, turn_y);
        (void)put_value(input, kCampKeyTurnZ, turn_z);
        if (fuel) {
            (void)put_value(input, kCampKeyFuel, kCampFuelMark);
        }
        const std::string actual = GodotHostResourceManager::status_name(
            host.submit(config.camp_part_port.c_str(), config.op_lay.c_str(), &input));
        report_check(check, "place=" + std::to_string(place) + " action=part make=" + make, actual, expected,
                     actual == expected, result, report);
        return true;
    }
    if (const toml::table* digest = action[GODOT_SELFTEST_KEY_DIGEST].as_table()) {
        // The walk of the camp: after every frame each record of each named port folded into one digest, the
        // line set beside the line of the Linux reader by data/probes/t10_fire_digest_compare.py.
        const toml::array* names = (*digest)[GODOT_SELFTEST_KEY_RECORDS].as_array();
        const int64_t frames = (*digest)[GODOT_SELFTEST_KEY_FRAMES].value_or(int64_t{0});
        const toml::array* id_keys = (*digest)[GODOT_SELFTEST_KEY_IDS].as_array();
        std::vector<std::string> ports;
        for (size_t i = 0u; names != nullptr && i < names->size(); ++i) {
            const std::string port = camp_walk_port(config, (*names)[i].value_or(std::string()));
            if (!port.empty()) {
                ports.push_back(port);
            }
        }
        std::vector<std::string> ids;
        for (size_t i = 0u; id_keys != nullptr && i < id_keys->size(); ++i) {
            const std::string key = (*id_keys)[i].value_or(std::string());
            if (!key.empty()) {
                ids.push_back(key);
            }
        }
        if (names == nullptr || names->empty() || ports.size() != names->size() || id_keys == nullptr ||
            ids.size() != id_keys->size() || frames <= 0) {
            report_broken(label, GODOT_SELFTEST_KEY_DIGEST,
                          "needs records naming survival_fire or camp_body ports of the start file, ids naming the "
                          "keys that carry an entity id, and frames",
                          result, report);
            return false;
        }
        uint64_t walk = GODOT_WALK_DIGEST_BASIS;
        uint64_t folded = 0u;
        for (int64_t i = 0; i < frames; ++i) {
            if (!frame(host, config, state)) {
                report_broken(label, GODOT_STEP_ADVANCE, "a frame was refused", result, report);
                return false;
            }
            for (const std::string& port : ports) {
                uint32_t written = 0u;
                const HostStatus status = snapshot_all(host, port, state.records, written);
                if (status != ase::kernel::HostStatusOk) {
                    report_broken(label, GODOT_STEP_SNAPSHOT, GodotHostResourceManager::status_name(status), result,
                                  report);
                    return false;
                }
                folded += written;
                walk = fold_snapshot(walk, state.records, written, ids);
            }
        }
        report_check(check, "action=digest frames=" + std::to_string(frames), std::to_string(folded) + " records",
                     "more than 0 records", folded > 0u, result, report);
        report.push_back("camp walk digest 0x" + hex_of(walk) + " over " + std::to_string(frames) + " frames, " +
                         std::to_string(folded) + " records");
        return true;
    }
    handled = false;
    return false;
}

/**
 * Runs one action of a started scenario. Answers false (reported) when the action cannot run -
 * then the scenario ends there; a comparison that FAILS is counted and the scenario goes on, so
 * one wrong value never hides the next.
 */
bool run_action(KernelEmbeddedHost& host, const toml::table& action, const VivariumStartConfig& config,
                const std::string& label, VivariumSelfTestState& state, VivariumSelfTestResult& result,
                std::vector<std::string>& report) {
    const std::string check = action[GODOT_SELFTEST_KEY_CHECK].value_or(std::string());
    if (!check.empty()) {
        state.checked.push_back(check);
    }

    if (const toml::node* frames = action.get(GODOT_SELFTEST_KEY_FRAMES)) {
        const int64_t count = frames->value_or(int64_t{-1});
        if (count <= 0) {
            report_broken(label, GODOT_SELFTEST_KEY_FRAMES, "needs a positive count", result, report);
            return false;
        }
        for (int64_t i = 0; i < count; ++i) {
            if (!frame(host, config, state)) {
                report_broken(label, GODOT_STEP_ADVANCE, "a frame was refused", result, report);
                return false;
            }
        }
        return true;
    }

    if (const toml::table* keep = action[GODOT_SELFTEST_KEY_REMEMBER].as_table()) {
        const std::string as = (*keep)[GODOT_SELFTEST_KEY_AS].value_or(std::string());
        const double value = read_value(host, config, *keep, state);
        if (as.empty() || !std::isfinite(value)) {
            report_broken(label, GODOT_SELFTEST_KEY_REMEMBER, "needs `as` and a value the snapshot carries", result,
                          report);
            return false;
        }
        state.remembered[as] = value;
        return true;
    }

    if (const toml::table* water = action[GODOT_SELFTEST_KEY_IRRIGATE].as_table()) {
        const toml::node* amount = water->get(GODOT_SELFTEST_KEY_AMOUNT);
        const std::string expected = (*water)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        uint32_t object_id = GODOT_NO_OBJECT;
        std::string target;
        if (const toml::node* place = water->get(GODOT_SELFTEST_KEY_PLACE)) {
            const int64_t at = place->value_or(int64_t{-1});
            object_id = object_at(host, config, at, state);
            target = "place=" + std::to_string(at);
        } else {
            const int64_t named = (*water)[GODOT_SELFTEST_KEY_OBJECT].value_or(int64_t{-1});
            object_id = named < 0 || named > static_cast<int64_t>(UINT32_MAX) ? GODOT_NO_OBJECT
                                                                              : static_cast<uint32_t>(named);
            target = "object=" + std::to_string(named);
        }
        if (amount == nullptr || !amount->is_number() || expected.empty() ||
            (object_id == GODOT_NO_OBJECT && water->get(GODOT_SELFTEST_KEY_OBJECT) == nullptr)) {
            report_broken(label, GODOT_SELFTEST_KEY_IRRIGATE, "needs a patch or object, amount_mm and status", result,
                          report);
            return false;
        }
        const double amount_mm = amount->value_or(std::nan(""));
        const std::string actual = GodotHostResourceManager::status_name(pour(host, config, object_id, amount_mm));
        report_check(check, target + " action=irrigate amount_mm=" + decimal(amount_mm), actual, expected,
                     actual == expected, result, report);
        return true;
    }

    if (const toml::table* wait = action[GODOT_SELFTEST_KEY_WAIT].as_table()) {
        const int64_t bound = (*wait)[GODOT_SELFTEST_KEY_MAX_FRAMES].value_or(int64_t{0});
        const std::string op = (*wait)[GODOT_SELFTEST_KEY_OP].value_or(std::string());
        const std::string key = (*wait)[GODOT_SELFTEST_KEY_KEY].value_or(std::string());
        const double value = target_of(*wait, state);
        bool known = true;
        bool reached = holds(read_value(host, config, *wait, state), op, value, state.tolerance, known);
        if (bound <= 0 || !known || !std::isfinite(value)) {
            report_broken(label, GODOT_SELFTEST_KEY_WAIT, "needs max_frames, a known op and a value", result, report);
            return false;
        }
        int64_t frames = 0;
        for (; frames < bound && !reached; ++frames) {
            if (!frame(host, config, state)) {
                report_broken(label, GODOT_STEP_ADVANCE, "a frame was refused", result, report);
                return false;
            }
            reached = holds(read_value(host, config, *wait, state), op, value, state.tolerance, known);
        }
        report_check(check, "wait key=" + key + " frames=" + std::to_string(frames),
                     reached ? "reached" : "not reached", op + " " + decimal(value) + " within " +
                     std::to_string(bound) + " frames", reached, result, report);
        return true;
    }

    if (const toml::table* catch_up = action[GODOT_SELFTEST_KEY_CATCH_UP].as_table()) {
        const double seconds = (*catch_up)[GODOT_SELFTEST_KEY_SECONDS].value_or(std::nan(""));
        const int64_t bound = (*catch_up)[GODOT_SELFTEST_KEY_MAX_FRAMES].value_or(int64_t{0});
        if (!(seconds > 0.0) || !std::isfinite(seconds) || bound <= 0) {
            report_broken(label, GODOT_SELFTEST_KEY_CATCH_UP, "needs seconds and max_frames", result, report);
            return false;
        }
        // One long frame, as the game host meets a return from the background: one tick, then the
        // real position moved on by the whole span is filed at once.
        uint32_t steps = 0u;
        if (host.advance(seconds, &steps) != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_ADVANCE, "the return was refused", result, report);
            return false;
        }
        state.real_s += seconds;
        const double filed = state.real_s;
        if (file_real(host, config, filed) != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_SELFTEST_KEY_CATCH_UP, "the real position was refused", result, report);
            return false;
        }
        // ase-time lays the clock forward to it in one write in its next Dissemination run and reports
        // the position it laid; the bound is a few frames to see that run, never a target. The filed
        // position and the laid one meet in the float the hub carries, and float rounding is monotone.
        const double filed_f = static_cast<double>(static_cast<float>(filed));
        bool laid = false;
        int64_t frames = 0;
        for (; frames < bound && !laid; ++frames) {
            if (!frame(host, config, state)) {
                report_broken(label, GODOT_STEP_ADVANCE, "a frame was refused", result, report);
                return false;
            }
            laid = laid_real(host, config, state) >= filed_f;
        }
        report_check(check, "catch_up seconds=" + decimal(seconds) + " frames=" + std::to_string(frames),
                     laid ? "laid" : "not laid",
                     "real_laid_s >= " + decimal(filed_f) + " within " + std::to_string(bound) + " frames", laid,
                     result, report);
        return true;
    }

    if (const toml::table* expect = action[GODOT_SELFTEST_KEY_EXPECT].as_table()) {
        const std::string op = (*expect)[GODOT_SELFTEST_KEY_OP].value_or(std::string());
        const std::string key = (*expect)[GODOT_SELFTEST_KEY_KEY].value_or(std::string());
        const double actual = read_value(host, config, *expect, state);
        const double value = target_of(*expect, state);
        bool known = true;
        const bool ok = holds(actual, op, value, state.tolerance, known);
        if (!known) {
            report_broken(label, GODOT_SELFTEST_KEY_EXPECT, "unknown op " + op, result, report);
            return false;
        }
        const std::string subject =
            (*expect)[GODOT_SELFTEST_KEY_RECORD].value_or(std::string()) == GODOT_SELFTEST_RECORD_CLOCK
                ? std::string("clock")
                : ((*expect)[GODOT_SELFTEST_KEY_RECORD].value_or(std::string()) == GODOT_SELFTEST_RECORD_SOIL
                       ? std::string("soil ")
                       : std::string()) +
                      "place=" + std::to_string((*expect)[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1}));
        report_check(check, subject + " key=" + key, decimal(actual), op + " " + decimal(value), ok, result, report);
        return true;
    }

    if (const toml::table* count = action[GODOT_SELFTEST_KEY_RECORDS].as_table()) {
        // A count of the clock, the patch or an organ port, compared with op (eq when none is named).
        // With max_frames the comparison is awaited frame by frame - the organs of a sprout reach
        // their port a few frames after the stage that lets it sprout.
        const std::string record = (*count)[GODOT_SELFTEST_KEY_RECORD].value_or(std::string());
        const int64_t expected = (*count)[GODOT_SELFTEST_KEY_COUNT].value_or(int64_t{-1});
        const std::string op = (*count)[GODOT_SELFTEST_KEY_OP].value_or(std::string(GODOT_SELFTEST_OP_EQ));
        const int64_t bound = (*count)[GODOT_SELFTEST_KEY_MAX_FRAMES].value_or(int64_t{0});
        const uint32_t organ_class = organ_class_of(record);
        const std::string& port = record == GODOT_SELFTEST_RECORD_CLOCK   ? config.clock_port
                                  : record == GODOT_SELFTEST_RECORD_SOIL   ? config.soil_port
                                  : organ_class < GODOT_ORGAN_COUNT        ? config.organ_ports[organ_class]
                                                                           : config.patch_port;
        bool known = true;
        (void)holds(0.0, op, 0.0, 0.0, known);
        if (expected < 0 || !known || bound < 0) {
            report_broken(label, GODOT_SELFTEST_KEY_RECORDS, "records needs a count, a known op and no negative bound",
                          result, report);
            return false;
        }
        uint32_t written = 0u;
        HostStatus status = snapshot_all(host, port, state.records, written);
        bool reached = status == ase::kernel::HostStatusOk &&
                       holds(static_cast<double>(written), op, static_cast<double>(expected), 0.0, known);
        int64_t frames = 0;
        for (; frames < bound && !reached && status == ase::kernel::HostStatusOk; ++frames) {
            if (!frame(host, config, state)) {
                report_broken(label, GODOT_STEP_ADVANCE, "a frame was refused", result, report);
                return false;
            }
            status = snapshot_all(host, port, state.records, written);
            reached = status == ase::kernel::HostStatusOk &&
                      holds(static_cast<double>(written), op, static_cast<double>(expected), 0.0, known);
        }
        if (status != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_SNAPSHOT, GodotHostResourceManager::status_name(status), result, report);
            return false;
        }
        report_check(check, "records=" + record + (bound > 0 ? " frames=" + std::to_string(frames) : std::string()),
                     std::to_string(written), op + " " + std::to_string(expected), reached, result, report);
        return true;
    }

    if (const toml::table* lay = action[GODOT_SELFTEST_KEY_LAY].as_table()) {
        // One patch through the scene port, as the game host lays the patches of its configuration.
        VivariumPatchSpec patch;
        const int64_t place = (*lay)[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1});
        const int64_t seed = (*lay)[GODOT_SELFTEST_KEY_SEED].value_or(int64_t{-1});
        patch.species = (*lay)[GODOT_SELFTEST_KEY_SPECIES].value_or(std::string());
        patch.soil = GodotHostResourceManager::soil_code((*lay)[GODOT_SELFTEST_KEY_SOIL].value_or(std::string()));
        patch.x = (*lay)[GODOT_SELFTEST_KEY_X].value_or(std::nan(""));
        patch.z = (*lay)[GODOT_SELFTEST_KEY_Z].value_or(std::nan(""));
        patch.half_m = (*lay)[GODOT_SELFTEST_KEY_HALF].value_or(std::nan(""));
        patch.capacity_mm = (*lay)[GODOT_SELFTEST_KEY_CAPACITY].value_or(std::nan(""));
        patch.gravity_rel = (*lay)[GODOT_SELFTEST_KEY_GRAVITY].value_or(std::nan(""));
        patch.age_days = (*lay)[GODOT_KEY_AGE_DAYS].value_or(std::nan(""));
        const std::string expected = (*lay)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        // The adapter only sees that every number is there and finite; the bounds of capacity and gravity
        // are ase-pl-flora's (FLORA_WATER_MIN_MM..MAX_MM, GRV_CONST_AREA_REL_MIN..MAX), and a case that
        // expects a refusal sends a value outside them on purpose.
        if (place < 0 || place > static_cast<int64_t>(UINT32_MAX) || seed < 0 ||
            seed > static_cast<int64_t>(UINT32_MAX) || patch.species.empty() || patch.soil >= GODOT_SOIL_COUNT ||
            !std::isfinite(patch.x) || !std::isfinite(patch.z) || !(patch.half_m > 0.0) ||
            !std::isfinite(patch.capacity_mm) || !std::isfinite(patch.gravity_rel) ||
            !std::isfinite(patch.age_days) || expected.empty()) {
            report_broken(label, GODOT_SELFTEST_KEY_LAY,
                          "needs place, species, seed, a known soil, x, z, a positive half_m, capacity_mm, "
                          "gravity_rel, age_days and status",
                          result, report);
            return false;
        }
        patch.place = static_cast<uint32_t>(place);
        patch.seed = static_cast<uint32_t>(seed);
        const KernelHostInput input = lay_input(patch);
        const std::string actual = GodotHostResourceManager::status_name(
            host.submit(config.scene_port.c_str(), config.op_lay.c_str(), &input));
        report_check(check, "place=" + std::to_string(place) + " action=lay species=" + patch.species, actual,
                     expected, actual == expected, result, report);
        return true;
    }

    if (const toml::table* view = action[GODOT_SELFTEST_KEY_VIEW].as_table()) {
        // The eye through the view port, as the scene places it at its aim site: the level of detail bands
        // the bodies around it, and an organ of a body without a band reaches no organ port (03.11 point 7).
        const double at_x = (*view)[GODOT_SELFTEST_KEY_X].value_or(std::nan(""));
        const double at_y = (*view)[GODOT_SELFTEST_KEY_Y].value_or(std::nan(""));
        const double at_z = (*view)[GODOT_SELFTEST_KEY_Z].value_or(std::nan(""));
        const double aim_x = (*view)[GODOT_SELFTEST_KEY_AIM_X].value_or(std::nan(""));
        const double aim_y = (*view)[GODOT_SELFTEST_KEY_AIM_Y].value_or(std::nan(""));
        const double aim_z = (*view)[GODOT_SELFTEST_KEY_AIM_Z].value_or(std::nan(""));
        const std::string expected = (*view)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        if (!std::isfinite(at_x) || !std::isfinite(at_y) || !std::isfinite(at_z) || !std::isfinite(aim_x) ||
            !std::isfinite(aim_y) || !std::isfinite(aim_z) || expected.empty()) {
            report_broken(label, GODOT_SELFTEST_KEY_VIEW, "needs a finite x, y, z, aim_x, aim_y, aim_z and status",
                          result, report);
            return false;
        }
        KernelHostInput input{};
        (void)put_value(input, GODOT_KEY_X, at_x);
        (void)put_value(input, GODOT_KEY_Y, at_y);
        (void)put_value(input, GODOT_KEY_Z, at_z);
        (void)put_value(input, GODOT_KEY_AIM_X, aim_x);
        (void)put_value(input, GODOT_KEY_AIM_Y, aim_y);
        (void)put_value(input, GODOT_KEY_AIM_Z, aim_z);
        const std::string actual = GodotHostResourceManager::status_name(
            host.submit(config.view_port.c_str(), config.op_view.c_str(), &input));
        report_check(check,
                     "action=view x=" + decimal(at_x) + " y=" + decimal(at_y) + " z=" + decimal(at_z) +
                         " aim_x=" + decimal(aim_x) + " aim_y=" + decimal(aim_y) + " aim_z=" + decimal(aim_z),
                     actual, expected, actual == expected, result, report);
        return true;
    }

    bool handled = false;
    const bool ran = run_camp_action(host, action, config, label, state, result, report, handled);
    if (handled) {
        return ran;
    }
    report_broken(label, GODOT_SELFTEST_KEY_DO,
                  "an action names none of frames, remember, irrigate, wait, catch_up, expect, records, lay, view, "
                  "camp, hull, part, digest",
                  result, report);
    return false;
}

}  // anonymous namespace

// LIFETIME

GodotHostResourceManager::GodotHostResourceManager()
    : failure_step_(GODOT_STEP_NONE), failure_status_(ase::kernel::HostStatusOk) {
    knots_ = std::make_unique<GodotKnotResourceManager>();  // the knot store lives as long as the host (N0543)
}

GodotHostResourceManager::~GodotHostResourceManager() {
    clear_all();
}

// THE BUNDLE INDEX

bool GodotHostResourceManager::parse_index(const std::string& text, VivariumBundleInfo& out) {
    out = VivariumBundleInfo{};
    const std::string separator(GODOT_BUNDLE_SEPARATOR);
    std::size_t start = 0u;
    while (start < text.size()) {
        const std::size_t newline = text.find('\n', start);
        const std::size_t end = newline == std::string::npos ? text.size() : newline;
        std::string line = text.substr(start, end - start);
        start = end + 1u;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::size_t at = line.find(separator);
        if (at == std::string::npos) {
            out = VivariumBundleInfo{};
            return false;
        }
        const std::string key = line.substr(0u, at);
        const std::string value = line.substr(at + separator.size());
        if (key == GODOT_BUNDLE_KEY_DATA_ROOT) {
            out.data_root = value;
        } else if (key == GODOT_BUNDLE_KEY_UNIT) {
            const std::vector<std::string> fields = fields_of(value);
            if (fields.size() != GODOT_BUNDLE_UNIT_FIELDS) {
                out = VivariumBundleInfo{};
                return false;
            }
            VivariumBundleUnit unit;
            unit.unit = fields[0];
            unit.library = fields[1];
            unit.manifest = fields[2];
            unit.version = fields[3];
            unit.api_version = fields[4];
            out.units.push_back(unit);
        } else if (key == GODOT_BUNDLE_KEY_DATA) {
            out.data.push_back(value);
        } else {
            out = VivariumBundleInfo{};
            return false;
        }
    }
    if (out.data_root.empty() || out.units.empty()) {
        out = VivariumBundleInfo{};
        return false;
    }
    return true;
}

uint8_t GodotHostResourceManager::soil_code(const std::string& word) {
    if (word == GODOT_SOIL_WORD_MEADOW) {
        return GODOT_SOIL_MEADOW;
    }
    if (word == GODOT_SOIL_WORD_FOREST) {
        return GODOT_SOIL_FOREST;
    }
    return GODOT_CODE_UNKNOWN;
}

const char* GodotHostResourceManager::organ_word(uint32_t organ_class) {
    return organ_class < GODOT_ORGAN_COUNT ? kOrganWords[organ_class] : "";
}

// FACTORY

HostStatus GodotHostResourceManager::boot(const VivariumStartConfig& config) {
    // Reiniciar runs the same teardown as _exit_tree: the old host, its views and its bundle go
    // first, so no id, input or drawing value of the previous run survives into this one.
    clear_all();
    config_ = config;
    failure_step_ = GODOT_STEP_NONE;
    failure_status_ = ase::kernel::HostStatusOk;
    elapsed_in_s_ = 0.0;
    backlog_s_ = 0.0;
    return_real_s_ = 0.0;
    tick_count_ = 0u;
    last_steps_ = 0u;

    // A configuration the host could never run is refused before a host exists: an empty name
    // would be a NotFound later, an empty real port a clock that never learns of a return.
    if (config_.read_stage == nullptr || config_.files_dir.empty() || config_.files_dir[0] != '/' ||
        !ports_named(config_) || !patches_layable(config_.patches) ||
        !std::isfinite(config_.irrigate_amount_mm) || !(config_.irrigate_amount_mm > 0.0)) {
        return fail(GODOT_STEP_CONFIG, ase::kernel::HostStatusInvalidArgument);
    }

    const char* log_file = config_.log_file.empty() ? nullptr : config_.log_file.c_str();
    HostStatus status = KernelEmbeddedHost::create(host_, &queue_host_line, this, log_file);
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_CREATE, status);
    }
    // THE START MODE BELONGS TO THE HOST'S CREATION: it is named before any unit loads, so the pass
    // that reads back the last state of the owners already finds it in its first tick.
    if (config_.start_fresh) {
        status = host_->set_start_fresh();
        if (status != ase::kernel::HostStatusOk) {
            return fail(GODOT_STEP_CREATE, status);
        }
    }

    status = read_bundle();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_BUNDLE, status);
    }
    status = read_manifests();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_MANIFEST, status);
    }
    status = load_set(*host_);
    if (status != ase::kernel::HostStatusOk) {
        return fail(failure_step_ == GODOT_STEP_NONE ? GODOT_STEP_LOAD : failure_step_, status);
    }

    bool offered = host_->has_port(config_.patch_port.c_str()) && host_->has_port(config_.forecast_port.c_str()) &&
                   host_->has_port(config_.soil_port.c_str()) &&
                   host_->has_port(config_.cond_port.c_str()) &&
                   host_->has_port(config_.stage_port.c_str()) &&
                   host_->has_port(config_.stem_port.c_str()) &&
                   host_->has_port(config_.autumn_port.c_str()) &&
                   host_->has_port(config_.cap_port.c_str()) &&
                   host_->has_port(config_.clock_port.c_str()) &&
                   host_->has_port(config_.date_port.c_str()) &&
                   host_->has_port(config_.scene_port.c_str()) && host_->has_port(config_.ground_port.c_str()) &&
                   host_->has_port(config_.brook_port.c_str()) && host_->has_port(config_.terrain_port.c_str()) &&
                   host_->has_port(config_.ground_cell_port.c_str()) && host_->has_port(config_.sky_port.c_str()) &&
                   host_->has_port(config_.stars_port.c_str()) &&
                   host_->has_port(config_.seed_port.c_str()) && host_->has_port(config_.tile_port.c_str()) &&
                   host_->has_port(config_.lineage_port.c_str()) &&
                   host_->has_port(config_.sway_port.c_str()) && host_->has_port(config_.bend_port.c_str()) &&
                   host_->has_port(config_.tint_port.c_str()) && host_->has_port(config_.atlas_port.c_str()) &&
                   host_->has_port(config_.bark_port.c_str()) &&
                   host_->has_port(config_.knot_port.c_str()) && host_->has_port(config_.foot_port.c_str()) &&
                   host_->has_port(config_.scar_port.c_str()) &&
                   host_->has_port(config_.stream_port.c_str()) &&
                   host_->has_port(config_.level_port.c_str()) && host_->has_port(config_.flatten_port.c_str()) &&
                   host_->has_port(config_.lower_port.c_str()) && host_->has_port(config_.raise_port.c_str()) &&
                   host_->has_port(config_.elevation_port.c_str()) &&
                   host_->has_port(config_.template_corner_port.c_str()) &&
                   host_->has_port(config_.template_triangle_port.c_str()) &&
                   host_->has_port(config_.template_tangent_port.c_str()) &&
                   host_->has_port(config_.real_port.c_str());
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        offered = offered && host_->has_port(config_.organ_ports[c].c_str());
    }
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        offered = offered && host_->has_port(config_.instance_ports[c].c_str());
    }
    if (!offered) {
        return fail(GODOT_STEP_PORT, ase::kernel::HostStatusNotFound);
    }

    status = host_->start();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_START, status);
    }

    // THE RELIEF OF THE GROUND FIRST: ase-pl-flora takes the shape request in Ingestion of the first
    // frame, ase-terrain reads the contract in Reception, and the first chunk is generated in Dynamics
    // with it - so the patches, the square and the brooks lie on the configured relief from the start.
    status = shape(config_.ground.height_scale_m, config_.ground.base_height_m);
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_ELEVATION, status);
    }
    // THE PATCHES OF THE START CONFIGURATION: with a host present ase-pl-flora lays none of its own
    // and takes these requests in its next Reception run; the patch port shows them from then on.
    status = lay_patches();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_LAY, status);
    }
    // THE GROUND OF THE SCENE (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8): the measured square and the
    // brooks, each on its own port; ase-terrain shapes and measures them, the terrain port shows them.
    status = lay_ground();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_GROUND, status);
    }
    status = lay_brooks();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_BROOK, status);
    }
    // THE FIGURE OF THIS SEAT (PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.8): one spawn request on the spawn port of
    // ase-pl-avatar, ase-player spawns the figure at the configured place. A host whose unit set carries no
    // ase-pl-avatar offers no figure port: the view then draws its scene without a figure and names the gap.
    if (figure_offered()) {
        status = spawn_figure(config_.figure_seat, config_.figure_x, config_.figure_z);
        if (status != ase::kernel::HostStatusOk) {
            return fail(GODOT_STEP_LAY, status);
        }
    } else {
        (void)host_->note(GODOT_LOG_SOURCE, "figure ports not offered: ase-pl-avatar is not in the unit set, no figure");
    }

    records_.assign(ase::kernel::HostRecordsMax, KernelHostRecord{});
    status = read_snapshot();
    if (status != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_SNAPSHOT, status);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::lay_ground() {
    const KernelHostInput input = ground_input(config_.ground);
    return host_->submit(config_.ground_port.c_str(), config_.op_lay.c_str(), &input);
}

HostStatus GodotHostResourceManager::lay_brooks() {
    for (const VivariumBrookSpec& brook : config_.brooks) {
        const KernelHostInput input = brook_input(brook);
        const HostStatus status = host_->submit(config_.brook_port.c_str(), config_.op_lay.c_str(), &input);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::lay_patches() {
    for (const VivariumPatchSpec& patch : config_.patches) {
        const KernelHostInput input = lay_input(patch);
        const HostStatus status = host_->submit(config_.scene_port.c_str(), config_.op_lay.c_str(), &input);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::fail(const char* step, HostStatus status) {
    failure_step_ = step;
    failure_status_ = status;
    patches_.clear();
    places_.clear();
    caps_.clear();
    clock_ = VivariumClockView{};
    weather_ = VivariumWeatherView{};
    sap_ = VivariumSapView{};
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        organs_[c].clear();
    }
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        instances_[c].clear();
    }
    organ_generation_ += 1u;
    bark_.clear();
    bark_owed_ = false;
    bark_generation_ += 1u;
    knots_->clear_all();
    terrain_.clear();
    ground_cells_.clear();
    terrain_generation_ += 1u;
    streams_.clear();
    stream_generation_ += 1u;
    currents_.clear();
    current_generation_ += 1u;
    pits_.clear();
    pit_generation_ += 1u;
    groundwater_.clear();
    groundwater_generation_ += 1u;
    salt_.clear();
    salt_generation_ += 1u;
    water_kinds_.clear();
    template_corners_.clear();
    template_triangles_.clear();
    template_generation_ += 1u;
    sky_ = VivariumSkyView{};
    seeds_.clear();
    racks_.clear();
    figures_.clear();
    figure_camera_ = VivariumFigureCameraView{};
    figure_gear_.clear();
    figure_code_ = VivariumFigureCodeView{};
    figure_bumps_.clear();
    if (!host_) {
        return status;  // no host, no logger: the node shows and prints the failure itself
    }
    // The host stays, STOPPED: its units are unloaded, but its logger still carries this line.
    (void)host_->stop();
    std::string line = "boot or run failed at ";
    line += step;
    line += ": ";
    line += status_name(status);
    line += " (";
    line += std::to_string(status);
    line += ")";
    (void)host_->note(GODOT_LOG_SOURCE, line.c_str());
    return status;
}

HostStatus GodotHostResourceManager::read_bundle() {
    std::string index;
    if (!config_.read_stage(GODOT_BUNDLE_INDEX, index, config_.read_stage_user) || index.empty()) {
        return ase::kernel::HostStatusNotFound;
    }
    if (!parse_index(index, bundle_)) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_manifests() {
    manifests_.clear();
    for (const VivariumBundleUnit& unit : bundle_.units) {
        std::string manifest;
        if (!config_.read_stage(unit.manifest.c_str(), manifest, config_.read_stage_user) || manifest.empty()) {
            manifests_.clear();
            return ase::kernel::HostStatusNotFound;
        }
        manifests_.push_back(manifest);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::load_set(KernelEmbeddedHost& host) {
    // THE DATA ROOT FIRST: every unit finds it in the App's context from its on_load on.
    const std::string data_root = config_.files_dir + "/" + bundle_.data_root;
    HostStatus status = host.set_data_root(data_root.c_str());
    if (status != ase::kernel::HostStatusOk) {
        failure_step_ = GODOT_STEP_DATA_ROOT;
        return status;
    }
    // The paths must outlive the call; KernelHostUnit only points at them.
    std::vector<std::string> paths;
    paths.reserve(bundle_.units.size());
    std::vector<KernelHostUnit> units;
    units.reserve(bundle_.units.size());
    for (size_t i = 0; i < bundle_.units.size() && i < manifests_.size(); ++i) {
        paths.push_back(library_path(bundle_.units[i].library));
        KernelHostUnit unit;
        unit.library_path = paths.back().c_str();
        unit.manifest_text = manifests_[i].c_str();
        unit.manifest_length = static_cast<uint32_t>(manifests_[i].size());
        units.push_back(unit);
    }
    status = host.load_units(units.data(), static_cast<uint32_t>(units.size()));
    if (status != ase::kernel::HostStatusOk) {
        failure_step_ = GODOT_STEP_LOAD;
    }
    return status;
}

std::string GodotHostResourceManager::library_path(const std::string& library) const {
    // No directory: the libraries of an APK lie in its native library directory, which the app's
    // linker namespace searches by file name - the bare name is the whole address there.
    return config_.library_dir.empty() ? library : config_.library_dir + "/" + library;
}

// SELF-TEST

VivariumSelfTestResult GodotHostResourceManager::self_test(const VivariumStartConfig& config,
                                                           const std::string& table,
                                                           std::vector<std::string>& report) {
    VivariumSelfTestResult result;
    // The game host boots after the self-test, never beside it: two Apps at once would be a
    // second simulation in the process (PLAN 02.3).
    if (host_) {
        report_broken("-", GODOT_STEP_CREATE, "a game host exists, the self-test runs before it", result, report);
        return result;
    }
    config_ = config;
    // The table lays its own patches (action lay): the self-test needs the ports and operations of a
    // game host, never its configured patches.
    if (config_.read_stage == nullptr || config_.files_dir.empty() || !ports_named(config_)) {
        report_broken("-", GODOT_STEP_CONFIG, "no stage reader, files directory, port or operation", result, report);
        return result;
    }
    if (read_bundle() != ase::kernel::HostStatusOk || read_manifests() != ase::kernel::HostStatusOk) {
        report_broken("-", GODOT_STEP_BUNDLE, "bundle index or a manifest not readable", result, report);
        bundle_ = VivariumBundleInfo{};
        manifests_.clear();
        return result;
    }

    toml::table root;
    try {
        root = toml::parse(table);
    } catch (const toml::parse_error& error) {
        report_broken("-", GODOT_SELFTEST_KEY_CASE,
                      "table not readable at line " + std::to_string(error.source().begin.line) + ": " +
                          std::string(error.description()),
                      result, report);
        bundle_ = VivariumBundleInfo{};
        manifests_.clear();
        return result;
    }
    const double tolerance = root[GODOT_SELFTEST_KEY_TOLERANCE].value_or(std::nan(""));
    const double frame_s = root[GODOT_SELFTEST_KEY_FRAME].value_or(std::nan(""));
    const toml::array* runs = root[GODOT_SELFTEST_KEY_RUNS].as_array();
    const toml::array* cases = root[GODOT_SELFTEST_KEY_CASE].as_array();
    if (!(tolerance >= 0.0) || !(frame_s > 0.0) || runs == nullptr || cases == nullptr || cases->empty()) {
        report_broken("-", GODOT_SELFTEST_KEY_CASE, "table needs tolerance, frame_seconds, runs and cases", result,
                      report);
        bundle_ = VivariumBundleInfo{};
        manifests_.clear();
        return result;
    }

    // ONE TEST HOST FOR THE WHOLE TABLE: created, loaded and started once, every scenario in the
    // table's order on it, stopped and destroyed after the last one - before the game host exists.
    // No log file - the game host's file must not rotate - and this manager's sink. A host that
    // cannot start is one error of its own; the runs it would have checked are reported below.
    std::vector<std::string> checked;
    std::unique_ptr<KernelEmbeddedHost> host;
    HostStatus status = KernelEmbeddedHost::create(host, &queue_host_line, this, nullptr);
    bool ready = status == ase::kernel::HostStatusOk;
    if (!ready) {
        report_broken("-", GODOT_STEP_CREATE, status_name(status), result, report);
    }
    if (ready) {
        failure_step_ = GODOT_STEP_NONE;
        status = load_set(*host);
        ready = status == ase::kernel::HostStatusOk;
        if (!ready) {
            // A failed load has torn the host down already; its lines name the cause.
            report_broken("-", failure_step_, status_name(status), result, report);
            failure_step_ = GODOT_STEP_NONE;
        }
    }
    if (ready) {
        status = host->start();
        ready = status == ase::kernel::HostStatusOk;
        if (!ready) {
            report_broken("-", GODOT_STEP_START, status_name(status), result, report);
            (void)host->stop();
        }
    }

    if (ready) {
        // The one host has one real clock: the real position runs on from scenario to scenario, as
        // it runs on from frame to frame of the game host, and never steps back.
        double real_s = 0.0;
        for (const toml::node& entry : *cases) {
            const toml::table* scenario = entry.as_table();
            if (scenario == nullptr) {
                report_broken("?", GODOT_SELFTEST_KEY_CASE, "a case is no table", result, report);
                continue;
            }
            const std::string label = scenario_label(*scenario);
            const toml::array* actions = (*scenario)[GODOT_SELFTEST_KEY_DO].as_array();
            if (actions == nullptr) {
                report_broken(label, GODOT_SELFTEST_KEY_DO, "no actions", result, report);
                continue;
            }
            VivariumSelfTestState state;
            state.tolerance = tolerance;
            state.frame_s = frame_s;
            state.real_s = real_s;
            bool complete = true;
            for (const toml::node& node : *actions) {
                const toml::table* action = node.as_table();
                if (action == nullptr) {
                    report_broken(label, GODOT_SELFTEST_KEY_DO, "an action is no table", result, report);
                    complete = false;
                    break;
                }
                if (!run_action(*host, *action, config_, label, state, result, report)) {
                    complete = false;
                    break;
                }
            }
            real_s = state.real_s;
            checked.insert(checked.end(), state.checked.begin(), state.checked.end());
            if (complete) {
                result.cases += 1u;
            }
        }
        status = host->stop();
        if (status != ase::kernel::HostStatusOk) {
            report_broken("-", GODOT_STEP_STOP, status_name(status), result, report);
        }
    }
    host.reset();

    // Every run the table claims must be checked by one of its actions - a run nobody checked is
    // an error, never a run that passed.
    for (const toml::node& node : *runs) {
        const std::string run = node.value_or(std::string());
        bool found = false;
        for (const std::string& check : checked) {
            found = found || check == run;
        }
        if (!found) {
            result.errors += 1u;
            report.push_back("selftest run=" + run + " result=FAILED cause=no action checks it");
        }
    }
    std::string set;
    for (const VivariumBundleUnit& unit : bundle_.units) {
        set += (set.empty() ? "" : ",") + unit.unit + "@" + unit.version;
    }
    report.push_back("selftest end cases=" + std::to_string(result.cases) + " checks=" +
                     std::to_string(result.checks) + " errors=" + std::to_string(result.errors) + " set=" + set);
    // The manager leaves as it came: no host, no bundle - boot() reads its own.
    bundle_ = VivariumBundleInfo{};
    manifests_.clear();
    return result;
}

// RUNNING

HostStatus GodotHostResourceManager::advance(double elapsed_s) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    // Never capped, never dropped: NaN and infinity reach the host unchanged and are refused there
    // with its own warning. The host runs its one tick of the frame; the absolute real position is
    // filed after it, and a long frame - the background time - moves it by the whole span at once.
    // ase-time sets its clock forward to it in one write, and every module computes the span up to
    // the new time in its own pass: a return is the same case as a jump.
    uint32_t steps = 0u;
    const HostStatus status = host_->advance(elapsed_s, &steps);
    if (status != ase::kernel::HostStatusOk) {
        return status;  // this frame is refused; the host said why
    }
    elapsed_in_s_ += elapsed_s;
    last_steps_ = steps;
    tick_count_ += steps;
    const HostStatus filed = file_real(*host_, config_, elapsed_in_s_);
    if (filed != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_ADVANCE, filed);
    }
    // A long frame opens a return: its seconds wait as the backlog until the clock record shows
    // ase-time laid the position this frame filed.
    if (elapsed_s > GODOT_CATCH_UP_NOTE_S) {
        return_real_s_ = elapsed_in_s_;
        backlog_s_ += elapsed_s;
    }

    const HostStatus read = read_snapshot();
    if (read != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_SNAPSHOT, read);
    }
    // The filed position and the laid one meet in the float the hub carries; float rounding is monotone.
    if (return_real_s_ > 0.0 && clock_.known &&
        clock_.real_laid_s >= static_cast<double>(static_cast<float>(return_real_s_))) {
        return_real_s_ = 0.0;
        backlog_s_ = 0.0;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_snapshot() {
    uint32_t written = 0u;
    // THE CONDITION CODES ARE ASE-PLANT'S (FLORA_HOST_PORT_COND, usecase:101340 - show the bed condition, D2454):
    // ase-pl-flora files PLT_CONST_COND_HEALTHY, PLT_CONST_COND_WLTG and PLT_CONST_COND_DEAD as one record, and
    // the view compares the condition of each patch against them, never against a number of its own. While no
    // record came (the codes not yet on the star) every condition stays unknown; two records are a broken port.
    HostStatus status = snapshot_all(*host_, config_.cond_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written > 1u) {
        return refuse_record(*host_, config_.cond_port, records_[1].object_id, "more than one condition record");
    }
    const double nan = std::nan("");
    const double cond_healthy = written == 1u ? record_value(records_[0], GODOT_KEY_COND_HEALTHY) : nan;
    const double cond_wilted = written == 1u ? record_value(records_[0], GODOT_KEY_COND_WILTED) : nan;
    const double cond_dead = written == 1u ? record_value(records_[0], GODOT_KEY_COND_DEAD) : nan;
    const bool codes = whole(cond_healthy) && whole(cond_wilted) && whole(cond_dead);

    // THE STAGE CODES ARE ASE-PLANT'S (FLORA_HOST_PORT_STAGE, usecase:101635 - show the plant stage, D2455):
    // ase-pl-flora files PLT_CONST_STAGE_SEED, _SPRT, _GROW and _MTUR as one record, and the view turns the stage
    // of each patch into its position among them (stage_key), never against a number of its own. While no record
    // came every stage stays unknown; two records are a broken port.
    status = snapshot_all(*host_, config_.stage_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written > 1u) {
        return refuse_record(*host_, config_.stage_port, records_[1].object_id, "more than one stage record");
    }
    double stage_codes[GODOT_STAGE_COUNT] = {};
    bool stage_codes_known = written == 1u;
    for (uint8_t position = 0u; position < GODOT_STAGE_COUNT; ++position) {
        stage_codes[position] = written == 1u ? record_value(records_[0], stage_key(position)) : nan;
        stage_codes_known = stage_codes_known && whole(stage_codes[position]);
    }

    status = snapshot_all(*host_, config_.patch_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    patches_.clear();
    places_.clear();
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        VivariumPatchView view;
        view.object_id = record.object_id;
        const double place = record_value(record, GODOT_KEY_PLACE);
        const double stage = record_value(record, GODOT_KEY_STAGE);
        const double condition = record_value(record, GODOT_KEY_CONDITION);
        view.coverage = record_value(record, GODOT_KEY_COVERAGE);
        // the soil water stands on the soil port, read below (usecase:101340 - show the bed condition); until
        // its record for this patch is read it is not measured
        view.soil_mm = std::nan("");
        view.soil_rel = std::nan("");
        view.capacity_mm = std::nan("");
        view.root_m = std::nan("");
        // the stage forecasts stand on the forecast port, read below; until its record for this patch
        // is read they are not measured
        view.grow_at_h = std::nan("");
        view.mtur_at_h = std::nan("");
        // the autumn colour of the individual stands on the autumn port, read below (usecase:101646 colour the
        // leaves in autumn); until its record for this patch is read it is not measured
        view.autumn = std::nan("");
        // the stem base: x and z from the lay request, y once ase-terrain levelled the disc (03.11)
        view.x = record_value(record, GODOT_KEY_X);
        view.y = record_value(record, GODOT_KEY_Y);
        view.z = record_value(record, GODOT_KEY_Z);
        // THE PORT CONTRACT IS THE PLACE. ase-pl-flora writes a key only while the input row behind
        // it stands (FloraHostPtchExptSystem: a key stands only while its row stands, never a zero
        // the modules did not publish), so before the first runs of the modules a record carries
        // its place and nothing else. A missing value is NOT MEASURED YET: a code stays unknown, a
        // number stays NaN, which share() draws as nothing. A record without a whole place is a
        // broken port, not a value to draw.
        if (!whole(place)) {
            patches_.clear();
            places_.clear();
            return refuse_record(*host_, config_.patch_port, record.object_id, "place is no whole number");
        }
        view.place = static_cast<uint32_t>(place);
        view.stage = GODOT_CODE_UNKNOWN;
        for (uint8_t position = 0u; stage_codes_known && whole(stage) && position < GODOT_STAGE_COUNT; ++position) {
            if (stage == stage_codes[position]) {
                view.stage = position;
            }
        }
        view.seed = stage_named(view.stage, GODOT_KEY_STGE_SEED);
        view.sprout = stage_named(view.stage, GODOT_KEY_STGE_SPRT);
        view.grow = stage_named(view.stage, GODOT_KEY_STGE_GROW);
        view.mature = stage_named(view.stage, GODOT_KEY_STGE_MTUR);
        view.condition = condition;
        view.wilted = codes && whole(condition) && condition == cond_wilted;
        view.dead = codes && whole(condition) && condition == cond_dead;
        view.cond_known = view.wilted || view.dead || (codes && whole(condition) && condition == cond_healthy);
        view.soil = code_of(record_value(record, GODOT_KEY_SOIL), GODOT_SOIL_COUNT);
        patches_[view.object_id] = view;
        places_[view.place] = view.object_id;
    }

    // THE STAGE FORECASTS HAVE A PORT OF THEIR OWN (FLORA_HOST_PORT_FORECAST): the patch record is full
    // (HUB_CONST_HOST_VALUES_MAX), so ase-pl-flora files grow_at_h and mtur_at_h as one record per patch
    // with a forecast, its object id the patch. A patch without such a record keeps both unmeasured; a
    // record that names no patch of the patch port is a broken port.
    status = snapshot_all(*host_, config_.forecast_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        patches_.clear();
        places_.clear();
        return status;
    }
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        const auto found = patches_.find(record.object_id);
        if (found == patches_.end()) {
            patches_.clear();
            places_.clear();
            return refuse_record(*host_, config_.forecast_port, record.object_id, "forecast of no patch");
        }
        found->second.grow_at_h = record_value(record, GODOT_KEY_GROW_AT_H);
        found->second.mtur_at_h = record_value(record, GODOT_KEY_MTUR_AT_H);
    }

    // THE SOIL WATER HAS A PORT OF ITS OWN (FLORA_HOST_PORT_SOIL, usecase:101340 - show the bed condition): the
    // patch record is full (HUB_CONST_HOST_VALUES_MAX), so ase-pl-flora files soil_mm, soil_rel, capacity_mm and
    // root_m of ase-hydrology as one record per patch whose soil row stands, its object id the patch. A patch
    // without such a record keeps all four unmeasured; a record that names no patch of the patch port is a
    // broken port.
    status = snapshot_all(*host_, config_.soil_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        patches_.clear();
        places_.clear();
        return status;
    }
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        const auto found = patches_.find(record.object_id);
        if (found == patches_.end()) {
            patches_.clear();
            places_.clear();
            return refuse_record(*host_, config_.soil_port, record.object_id, "soil of no patch");
        }
        found->second.soil_mm = record_value(record, GODOT_KEY_SOIL_MM);
        found->second.soil_rel = record_value(record, GODOT_KEY_SOIL_REL);
        found->second.capacity_mm = record_value(record, GODOT_KEY_CAPACITY_MM);
        found->second.root_m = record_value(record, GODOT_KEY_ROOT_M);
    }

    // THE STEM FOOT IS A BODY OF ASE-PHYSICS (FLORA_HOST_PORT_STEM, usecase:100708 land the dispersed seeds,
    // N0487): ase-pl-flora files one record per patch whose stem body stands, its object id the patch, base_y
    // the height of the foot over the level of the patch - a dug hole lowers it, a heap raises it. A patch
    // without such a record stands at its level (base_y 0, set by the view's default); a record that names no
    // patch of the patch port, or carries no height, is a broken port.
    status = snapshot_all(*host_, config_.stem_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        patches_.clear();
        places_.clear();
        return status;
    }
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        const auto found = patches_.find(record.object_id);
        if (found == patches_.end()) {
            patches_.clear();
            places_.clear();
            return refuse_record(*host_, config_.stem_port, record.object_id, "stem of no patch");
        }
        const double base_y = record_value(record, GODOT_KEY_BASE_Y);
        if (!std::isfinite(base_y)) {
            patches_.clear();
            places_.clear();
            return refuse_record(*host_, config_.stem_port, record.object_id, "stem without base_y");
        }
        found->second.base_y = base_y;
    }

    // THE AUTUMN COLOUR IS THE INDIVIDUAL'S OWN (FLORA_HOST_PORT_AUTUMN, usecase:101646 colour the leaves in autumn,
    // CAUSA_ASE_PLANT step 6): ase-pl-flora files one record per patch whose colour ase-phytomer measured, its
    // object id the patch, autumn the share of the way from the start of autumn to the own leaf fall the warmth of
    // its cell moved. A patch without such a record keeps it unmeasured; a record that names no patch of the patch
    // port, or carries no finite share, is a broken port.
    status = snapshot_all(*host_, config_.autumn_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        patches_.clear();
        places_.clear();
        return status;
    }
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        const auto found = patches_.find(record.object_id);
        if (found == patches_.end()) {
            patches_.clear();
            places_.clear();
            return refuse_record(*host_, config_.autumn_port, record.object_id, "autumn of no patch");
        }
        const double autumn = record_value(record, GODOT_KEY_AUTUMN);
        if (!std::isfinite(autumn)) {
            patches_.clear();
            places_.clear();
            return refuse_record(*host_, config_.autumn_port, record.object_id, "autumn without a share");
        }
        found->second.autumn = autumn;
    }

    // THE CAP IS THE INDIVIDUAL'S OWN (FLORA_HOST_PORT_CAP, usecase:101362 scale the organ templates): ase-pl-flora
    // files one record per patch whose fruit body has a cap, its object id the patch, cap_radius the radius
    // ase-phytomer passed on for its individual (PHY_CAP_RADIUS). A patch without such a record has no fruit body to
    // draw; a record that names no patch of the patch port, or carries no finite radius, is a broken port.
    caps_.clear();
    status = snapshot_all(*host_, config_.cap_port, records_, written);
    if (status != ase::kernel::HostStatusOk) {
        patches_.clear();
        places_.clear();
        return status;
    }
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = records_[i];
        if (patches_.find(record.object_id) == patches_.end()) {
            patches_.clear();
            places_.clear();
            caps_.clear();
            return refuse_record(*host_, config_.cap_port, record.object_id, "cap of no patch");
        }
        const double cap_radius = record_value(record, GODOT_KEY_CAP_RADIUS);
        if (!std::isfinite(cap_radius)) {
            patches_.clear();
            places_.clear();
            caps_.clear();
            return refuse_record(*host_, config_.cap_port, record.object_id, "cap without a radius");
        }
        caps_[record.object_id] = cap_radius;
    }

    std::vector<KernelHostRecord> clock_records(1u);
    status = snapshot_all(*host_, config_.clock_port, clock_records, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    // The clock port publishes an empty set until the clock row of the plugin stands
    // (FloraHostClkExptSystem): the clock is not known yet, and nothing is broken. Two clocks are.
    if (written == 0u) {
        clock_ = VivariumClockView{};
        return ase::kernel::HostStatusOk;
    }
    if (written != 1u) {
        return refuse_record(*host_, config_.clock_port, clock_records[0].object_id, "more than one clock");
    }
    VivariumClockView clock;
    clock.elapsed_s = record_value(clock_records[0], GODOT_KEY_ELAPSED);
    clock.day = record_value(clock_records[0], GODOT_KEY_DAY);
    clock.hour = record_value(clock_records[0], GODOT_KEY_HOUR);
    if (!std::isfinite(clock.elapsed_s) || !std::isfinite(clock.day) || !std::isfinite(clock.hour)) {
        return refuse_record(*host_, config_.clock_port, clock_records[0].object_id,
                             "elapsed, day or hour not finite");
    }
    // The length of the day and of the hour are ase-time's constants, filed by the plugin from the star;
    // the view turns the clock into game hours by them and measures nothing.
    clock.hours_per_day = record_value(clock_records[0], GODOT_KEY_HOURS_PER_DAY);
    clock.seconds_per_hour = record_value(clock_records[0], GODOT_KEY_SECONDS_PER_HOUR);
    if (!std::isfinite(clock.hours_per_day) || !std::isfinite(clock.seconds_per_hour) || clock.hours_per_day <= 0.0 ||
        clock.seconds_per_hour <= 0.0) {
        return refuse_record(*host_, config_.clock_port, clock_records[0].object_id,
                             "hours per day or seconds per hour missing or not positive");
    }
    // The cycle end of ase-phytomer's growth clock rides on the clock record once that clock is ready
    // (FloraHostClkExptSystem, PHY_CLK_LAST_S); the host only shows it, nothing waits on it.
    clock.shoot_s = record_value(clock_records[0], GODOT_KEY_SHOOT);
    clock.shoot_known = std::isfinite(clock.shoot_s);
    // The real position ase-time last laid its clock to (TIM_REAL_LAID_S, 0 = never): the end of a
    // return from the background, read here instead of being counted in ticks.
    clock.real_laid_s = record_value(clock_records[0], GODOT_KEY_REAL_LAID);
    if (!std::isfinite(clock.real_laid_s) || clock.real_laid_s < 0.0) {
        return refuse_record(*host_, config_.clock_port, clock_records[0].object_id,
                             "real position laid missing or negative");
    }
    // ase-time's verdict on the last seek or date request rides on the clock record while it stands
    // (FloraHostClkExptSystem, TIM_SEEK_*); absent before the first request was judged. The view reads
    // it and compares no clock part of its own.
    clock.seek_seq = record_value(clock_records[0], GODOT_KEY_SEEK_SEQ);
    clock.seek_order = record_value(clock_records[0], GODOT_KEY_SEEK_ORDER);
    // The target comes as day and hour of the day: an absolute game hour outgrows the exact range of a float
    // slot (VECTUM 154 lies near 18.58 million hours, beyond 2^24), so ase-time splits it (TIM_SEEK_TARGET_DAY).
    clock.seek_target_day = record_value(clock_records[0], GODOT_KEY_SEEK_TARGET_DAY);
    clock.seek_target_h = record_value(clock_records[0], GODOT_KEY_SEEK_TARGET_H);
    clock.seek_known = std::isfinite(clock.seek_seq) && std::isfinite(clock.seek_order) &&
                       std::isfinite(clock.seek_target_day) && std::isfinite(clock.seek_target_h);
    // The bounds of a seek request are ase-time's constants (TIM_CONST_SEEK_SEQ_MAX, TIM_CONST_SEEK_ORDER_NONE,
    // TIM_CONST_SEEK_ORDER_REFUSED), filed by the plugin from the star on a record of the seek port, because the
    // clock record is full (HUB_CONST_HOST_VALUES_MAX). The view bounds the request number and the verdict by
    // them and holds no number of its own; without the record no jump is filed.
    if (!config_.seek_port.empty() && host_->has_port(config_.seek_port.c_str())) {
        std::vector<KernelHostRecord> seek_records(1u);
        status = snapshot_all(*host_, config_.seek_port, seek_records, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        if (written > 1u) {
            return refuse_record(*host_, config_.seek_port, seek_records[0].object_id, "more than one seek bounds");
        }
        if (written == 1u) {
            clock.seek_seq_min = record_value(seek_records[0], GODOT_KEY_SEEK_SEQ_MIN);
            clock.seek_seq_max = record_value(seek_records[0], GODOT_KEY_SEEK_SEQ_MAX);
            clock.seek_order_none = record_value(seek_records[0], GODOT_KEY_SEEK_ORDER_NONE);
            clock.seek_order_refused = record_value(seek_records[0], GODOT_KEY_SEEK_ORDER_REFUSED);
            if (!std::isfinite(clock.seek_seq_min) || !std::isfinite(clock.seek_seq_max) ||
                !std::isfinite(clock.seek_order_none) || !std::isfinite(clock.seek_order_refused) ||
                clock.seek_seq_min <= 0.0 || clock.seek_seq_max < clock.seek_seq_min ||
                clock.seek_order_refused <= clock.seek_order_none) {
                return refuse_record(*host_, config_.seek_port, seek_records[0].object_id,
                                     "seek bounds incomplete or out of order: the largest number lies below the "
                                     "first, or the refused code does not lie above the none code");
            }
            clock.seek_bounds_known = true;
        }
    }
    if (clock.seek_known && clock.seek_bounds_known &&
        (clock.seek_order < clock.seek_order_none || clock.seek_order > clock.seek_order_refused)) {
        return refuse_record(*host_, config_.clock_port, clock_records[0].object_id,
                             "seek verdict outside ase-time's codes, none to refused");
    }
    // The day period rides on the clock record; the SURA date (ARCH_ASE_CALENDAR 2.9.1) stands on a record
    // of its own on the date port, because one record holds at most HUB_CONST_HOST_VALUES_MAX values and
    // the hour, its constants and the sun fill the clock record already (FloraHostClkExptSystem). The date
    // port publishes an empty set until ase-calendar measured CAL_YEAR (FloraHubCalSyncSystem); until then
    // the header keeps the day and hour line. A part that is missing alone is a broken record, not a date
    // still to come.
    clock.period = record_value(clock_records[0], GODOT_KEY_PERIOD);
    std::vector<KernelHostRecord> date_records(1u);
    status = snapshot_all(*host_, config_.date_port, date_records, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written > 1u) {
        return refuse_record(*host_, config_.date_port, date_records[0].object_id, "more than one date");
    }
    if (written == 1u) {
        clock.year = record_value(date_records[0], GODOT_KEY_YEAR);
        clock.epoch = record_value(date_records[0], GODOT_KEY_EPOCH);
        clock.cycle = record_value(date_records[0], GODOT_KEY_CYCLE);
        clock.month = record_value(date_records[0], GODOT_KEY_MONTH);
        clock.month_day = record_value(date_records[0], GODOT_KEY_MONTH_DAY);
        // ase-calendar's lengths of a LUNBREX and a VECTUM ride with the date: the debug date entry bounds
        // its fields by them, never by a number of the view.
        clock.days_per_month = record_value(date_records[0], GODOT_KEY_DAYS_PER_MONTH);
        clock.months_per_year = record_value(date_records[0], GODOT_KEY_MONTHS_PER_YEAR);
        // ase-calendar's codes of the four seasons ride with the date (CAL_CONST_SEASON_*): the signs compare the
        // day record's season against them, never against a number of the view.
        clock.season_glacix = record_value(date_records[0], GODOT_KEY_SEASON_GLACIX);
        clock.season_saltatrix = record_value(date_records[0], GODOT_KEY_SEASON_SALTATRIX);
        clock.season_ignix = record_value(date_records[0], GODOT_KEY_SEASON_IGNIX);
        clock.season_halitrix = record_value(date_records[0], GODOT_KEY_SEASON_HALITRIX);
        const double date_parts[] = {clock.year,          clock.epoch,           clock.cycle,
                                     clock.month,         clock.month_day,       clock.days_per_month,
                                     clock.months_per_year, clock.season_glacix,
                                     clock.season_saltatrix, clock.season_ignix, clock.season_halitrix};
        std::size_t finite_parts = 0u;
        for (const double part : date_parts) {
            finite_parts += std::isfinite(part) ? 1u : 0u;
        }
        if (finite_parts != std::size(date_parts)) {
            return refuse_record(*host_, config_.date_port, date_records[0].object_id,
                                 "SURA date incomplete: year, epoch, cycle, month, day, the calendar lengths and "
                                 "the season codes come together");
        }
        if (!std::isfinite(clock.period)) {
            return refuse_record(*host_, config_.clock_port, clock_records[0].object_id,
                                 "SURA date without its day period");
        }
    }
    clock.date_known = written == 1u;
    clock.season_codes_known = clock.date_known;  // a date record stands only complete, the four codes with it
    // N0460: the day record carries the day's sunrise, sunset and daylight, the season (CAL_SEASON,
    // CAL_DAYLIGHT_DURATION) and the shares the modules place the clock, the sun and today at (TIM_DAY_PROGRESS,
    // ase-calendar's share of the year) on a port of its own, because the clock and the date record are full
    // (HUB_CONST_HOST_VALUES_MAX). The adapter never turns an hour into a share: that is the clock logic of the
    // modules. Until ase-pl-flora offers the port or files the record the day is not known and the sheet draws no
    // light, sun or today (the units log the gap on their edge); a part that is missing alone is a broken record.
    if (!config_.day_port.empty() && host_->has_port(config_.day_port.c_str())) {
        std::vector<KernelHostRecord> day_records(1u);
        status = snapshot_all(*host_, config_.day_port, day_records, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        if (written > 1u) {
            return refuse_record(*host_, config_.day_port, day_records[0].object_id, "more than one day");
        }
        if (written == 1u) {
            const KernelHostRecord& day = day_records[0];
            clock.sunrise_h = record_value(day, GODOT_KEY_SUNRISE_H);
            clock.sunset_h = record_value(day, GODOT_KEY_SUNSET_H);
            clock.daylight_h = record_value(day, GODOT_KEY_DAYLIGHT_H);
            clock.season = record_value(day, GODOT_KEY_SEASON);
            clock.day_share = record_value(day, GODOT_KEY_DAY_SHARE);
            clock.sunrise_share = record_value(day, GODOT_KEY_SUNRISE_SHARE);
            clock.sunset_share = record_value(day, GODOT_KEY_SUNSET_SHARE);
            clock.year_share = record_value(day, GODOT_KEY_YEAR_SHARE);
            clock.year_day = record_value(day, kDayKeyYearDay);
            // B0280: ase-calendar's share of a whole LUNBREX rides with today's share; the year ring places its LUN
            // marks and season arcs by it and never divides by the LUNBREX of a year (the last one is shorter)
            clock.lun_share = record_value(day, GODOT_KEY_LUN_SHARE);
            const double day_parts[] = {clock.sunrise_h,     clock.sunset_h,     clock.daylight_h,
                                        clock.season,        clock.day_share,    clock.sunrise_share,
                                        clock.sunset_share,  clock.year_share,   clock.year_day,
                                        clock.lun_share};
            std::size_t finite_day = 0u;
            for (const double part : day_parts) {
                finite_day += std::isfinite(part) ? 1u : 0u;
            }
            if (finite_day != std::size(day_parts)) {
                return refuse_record(*host_, config_.day_port, day.object_id,
                                     "day incomplete: sunrise, sunset, daylight, the season, the shares of the "
                                     "day, the sun and the year and the day of the year come together");
            }
            clock.day_known = true;
        }
    }
    // N0460: one record per day period (TIM_DAY_PERIOD ASO .. NOX) with the hour it begins (0 for ASO, else
    // ase-time's TIM_CONST_PERIOD_BND_*) and the share of the day ase-time places that hour at. Entity per item: a
    // phase is a record, never a slot of the day record. Every period comes exactly once; a set with a period
    // missing, twice or out of range, or a record without its hour or share, is refused.
    if (!config_.phase_port.empty() && host_->has_port(config_.phase_port.c_str())) {
        std::vector<KernelHostRecord> phase_records(GODOT_DAY_PHASES + 1u);
        status = snapshot_all(*host_, config_.phase_port, phase_records, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        if (written != 0u && written != GODOT_DAY_PHASES) {
            return refuse_record(*host_, config_.phase_port, phase_records[0].object_id,
                                 "phase set incomplete: every day period comes exactly once");
        }
        if (written == GODOT_DAY_PHASES) {
            clock.phase_start_h.assign(GODOT_DAY_PHASES, std::nan(""));
            clock.phase_start_share.assign(GODOT_DAY_PHASES, std::nan(""));
            for (std::size_t r = 0u; r < written; ++r) {
                const KernelHostRecord& phase = phase_records[r];
                const double period = record_value(phase, GODOT_KEY_PERIOD);
                const double start_h = record_value(phase, GODOT_KEY_START_H);
                const double start_share = record_value(phase, GODOT_KEY_START_SHARE);
                if (!std::isfinite(period) || !std::isfinite(start_h) || !std::isfinite(start_share)) {
                    return refuse_record(*host_, config_.phase_port, phase.object_id,
                                         "phase incomplete: the period, its start hour and its share come together");
                }
                if (!whole(period) || period < static_cast<double>(GODOT_PERIOD_FIRST) ||
                    period > static_cast<double>(GODOT_PERIOD_LAST)) {
                    return refuse_record(*host_, config_.phase_port, phase.object_id, "period out of range");
                }
                const std::size_t slot = static_cast<std::size_t>(period) - static_cast<std::size_t>(GODOT_PERIOD_FIRST);
                if (std::isfinite(clock.phase_start_h[slot])) {
                    return refuse_record(*host_, config_.phase_port, phase.object_id, "period twice");
                }
                clock.phase_start_h[slot] = start_h;
                clock.phase_start_share[slot] = start_share;
            }
            clock.phases_known = true;
        }
    }
    // N0460: one record per season (CAL_CONST_SEASON_GLACIX .. HALITRIX) with its first LUN
    // (CAL_CONST_SEASON_*_START) and its hours of light (CAL_CONST_SSN_DAYLIGHT_*), ase-calendar's constants. Every
    // season comes exactly once, refused like the phase set.
    if (!config_.season_port.empty() && host_->has_port(config_.season_port.c_str())) {
        std::vector<KernelHostRecord> season_records(GODOT_YEAR_SEASONS + 1u);
        status = snapshot_all(*host_, config_.season_port, season_records, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        if (written != 0u && written != GODOT_YEAR_SEASONS) {
            return refuse_record(*host_, config_.season_port, season_records[0].object_id,
                                 "season set incomplete: every season comes exactly once");
        }
        if (written == GODOT_YEAR_SEASONS) {
            clock.season_start_lun.assign(GODOT_YEAR_SEASONS, std::nan(""));
            clock.season_light_h.assign(GODOT_YEAR_SEASONS, std::nan(""));
            for (std::size_t r = 0u; r < written; ++r) {
                const KernelHostRecord& season = season_records[r];
                const double code = record_value(season, GODOT_KEY_SEASON);
                const double start_lun = record_value(season, GODOT_KEY_START_LUN);
                const double light_h = record_value(season, GODOT_KEY_LIGHT_H);
                if (!std::isfinite(code) || !std::isfinite(start_lun) || !std::isfinite(light_h)) {
                    return refuse_record(*host_, config_.season_port, season.object_id,
                                         "season incomplete: the season, its first LUN and its light come together");
                }
                if (!whole(code) || code >= static_cast<double>(GODOT_YEAR_SEASONS)) {
                    return refuse_record(*host_, config_.season_port, season.object_id, "season out of range");
                }
                const std::size_t slot = static_cast<std::size_t>(code);
                if (std::isfinite(clock.season_start_lun[slot])) {
                    return refuse_record(*host_, config_.season_port, season.object_id, "season twice");
                }
                clock.season_start_lun[slot] = start_lun;
                clock.season_light_h[slot] = light_h;
            }
            clock.seasons_known = true;
        }
    }
    // N0459: one pace record with the factor of ase-time's base pace the clock runs at, as ase-pl-flora divides it
    // (FloraPaceCalcSystem). The adapter never divides the scale itself. A record without a positive finite factor
    // is refused; without the record the pace is not known and the hourglass keeps the factor last asked.
    if (!config_.pace_port.empty() && host_->has_port(config_.pace_port.c_str())) {
        std::vector<KernelHostRecord> pace_records(1u);
        status = snapshot_all(*host_, config_.pace_port, pace_records, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        if (written > 1u) {
            return refuse_record(*host_, config_.pace_port, pace_records[0].object_id, "more than one pace");
        }
        if (written == 1u) {
            const KernelHostRecord& pace = pace_records[0];
            const double factor = record_value(pace, GODOT_KEY_FACTOR);
            if (!std::isfinite(factor) || !(factor > GODOT_PACE_NONE)) {
                return refuse_record(*host_, config_.pace_port, pace.object_id,
                                     "pace without a positive factor of the base pace");
            }
            clock.pace_factor = factor;
            clock.pace_known = true;
        }
    }
    clock.known = true;
    clock_ = clock;
    const HostStatus organs = read_organs();
    if (organs != ase::kernel::HostStatusOk) {
        return organs;
    }
    const HostStatus bark = read_bark();
    if (bark != ase::kernel::HostStatusOk) {
        return bark;
    }
    const HostStatus knots = read_knots();
    if (knots != ase::kernel::HostStatusOk) {
        return knots;
    }
    const HostStatus terrain = read_terrain();
    if (terrain != ase::kernel::HostStatusOk) {
        return terrain;
    }
    const HostStatus streams = read_streams();
    if (streams != ase::kernel::HostStatusOk) {
        return streams;
    }
    const HostStatus currents = read_currents();
    if (currents != ase::kernel::HostStatusOk) {
        return currents;
    }
    const HostStatus pits = read_pits();
    if (pits != ase::kernel::HostStatusOk) {
        return pits;
    }
    const HostStatus groundwater = read_groundwater();
    if (groundwater != ase::kernel::HostStatusOk) {
        return groundwater;
    }
    const HostStatus salt = read_salt();
    if (salt != ase::kernel::HostStatusOk) {
        return salt;
    }
    const HostStatus water_kinds =
        water_kinds_.read(*host_, VivariumWaterKindsPorts{config_.lake_port, config_.sea_port, config_.glacier_port});
    if (water_kinds != ase::kernel::HostStatusOk) {
        return water_kinds;
    }
    const HostStatus templates = read_templates();
    if (templates != ase::kernel::HostStatusOk) {
        return templates;
    }
    const HostStatus sky = read_sky();
    if (sky != ase::kernel::HostStatusOk) {
        return sky;
    }
    const HostStatus stars = read_stars();
    if (stars != ase::kernel::HostStatusOk) {
        return stars;
    }
    const HostStatus seeds = read_seeds();
    if (seeds != ase::kernel::HostStatusOk) {
        return seeds;
    }
    const HostStatus racks = read_racks();
    if (racks != ase::kernel::HostStatusOk) {
        return racks;
    }
    const HostStatus bodies = read_survival_bodies();
    if (bodies != ase::kernel::HostStatusOk) {
        return bodies;
    }
    const HostStatus rests = read_survival_rests();
    if (rests != ase::kernel::HostStatusOk) {
        return rests;
    }
    const HostStatus foods = read_survival_foods();
    if (foods != ase::kernel::HostStatusOk) {
        return foods;
    }
    const HostStatus smokes = read_survival_smokes();
    if (smokes != ase::kernel::HostStatusOk) {
        return smokes;
    }
    const HostStatus fires = read_survival_fires();
    if (fires != ase::kernel::HostStatusOk) {
        return fires;
    }
    const HostStatus left = read_survival_left();
    if (left != ase::kernel::HostStatusOk) {
        return left;
    }
    const HostStatus weather = read_weather();
    if (weather != ase::kernel::HostStatusOk) {
        return weather;
    }
    const HostStatus sap = read_sap();
    if (sap != ase::kernel::HostStatusOk) {
        return sap;
    }
    const HostStatus loose = read_loose();
    if (loose != ase::kernel::HostStatusOk) {
        return loose;
    }
    const HostStatus camp_bodies = read_camp_bodies();
    if (camp_bodies != ase::kernel::HostStatusOk) {
        return camp_bodies;
    }
    const HostStatus camp_refuels = read_camp_refuels();
    if (camp_refuels != ase::kernel::HostStatusOk) {
        return camp_refuels;
    }
    return read_figures();
}

HostStatus GodotHostResourceManager::read_organs() {
    // THE ORGANS CHANGE ONLY IN ONE SCHEDULE. ase-phytomer and ase-radix write their organ rows in
    // GODOT_ORGAN_SCHEDULE; the plugin's sync system reads them in the Reception and the plugin lays them
    // on the ports in the Production of the frame after. So a new run owes two reads - this frame and the
    // next - and a frame without one reuses the views instead of reading thousands of records again.
    uint64_t runs = 0u;
    const HostStatus counted = host_->schedule_runs(GODOT_ORGAN_SCHEDULE, &runs);
    if (counted != ase::kernel::HostStatusOk) {
        return counted;
    }
    if (runs != organ_runs_ || organ_generation_ == 0u) {
        organ_runs_ = runs;
        organ_reads_due_ = GODOT_ORGAN_READS_PER_RUN;
    }
    if (organ_reads_due_ == 0u) {
        return ase::kernel::HostStatusOk;
    }
    organ_reads_due_ -= 1u;
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.organ_ports[c], organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        organs_[c].clear();
        organs_[c].reserve(written);
        for (uint32_t i = 0u; i < written; ++i) {
            VivariumOrganView view;
            if (!organ_view_of(organ_records_[i], view)) {
                organs_[c].clear();
                return refuse_record(*host_, config_.organ_ports[c], organ_records_[i].object_id,
                                     "organ values incomplete or out of range");
            }
            // the segment port carries the bark of every internode as its twelfth value
            if (c == GODOT_ORGAN_SEGMENT && !segment_bark_of(organ_records_[i], view)) {
                organs_[c].clear();
                return refuse_record(*host_, config_.organ_ports[c], organ_records_[i].object_id,
                                     "bark of the internode missing or out of range");
            }
            // the leaf port carries the card of every leaf as its twelfth value (TREEGEN T.6)
            if (c == GODOT_ORGAN_LEAF && !leaf_template_of(organ_records_[i], view)) {
                organs_[c].clear();
                return refuse_record(*host_, config_.organ_ports[c], organ_records_[i].object_id,
                                     "card of the leaf missing or no leaf template");
            }
            organs_[c].push_back(view);
        }
    }
    // The bark tiling of the internodes, from the same Production run: one record per internode with
    // lenticels, its object id the internode; an internode without one keeps GODOT_TILE_NONE.
    {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.tile_port, organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        std::vector<VivariumOrganView>& segments = organs_[GODOT_ORGAN_SEGMENT];
        std::unordered_map<uint32_t, std::size_t> segment_at;
        segment_at.reserve(segments.size());
        for (std::size_t s = 0u; s < segments.size(); ++s) {
            segment_at.emplace(segments[s].object_id, s);
        }
        for (uint32_t i = 0u; i < written; ++i) {
            const auto found = segment_at.find(organ_records_[i].object_id);
            if (found == segment_at.end()) {
                continue;  // the internode left the segment port in the same run; its tiling has nothing to scale
            }
            if (!segment_tile_of(organ_records_[i], segments[found->second])) {
                return refuse_record(*host_, config_.tile_port, organ_records_[i].object_id,
                                     "bark tiling missing or out of range");
            }
        }
    }
    // The lineage of every shown organ, from the same Production run (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.9, the round
    // trip S0, S1, S0): one record per internode, leaf, catkin and fruit stand, its object id the organ; an organ
    // without one keeps GODOT_LINEAGE_NONE. The object id is the entity and changes with a new resolution, the lineage
    // does not, so the host test of the round trip compares the organs by it.
    {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.lineage_port, organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        std::unordered_map<uint32_t, VivariumOrganView*> organ_at;
        for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
            for (VivariumOrganView& organ : organs_[c]) {
                organ_at.emplace(organ.object_id, &organ);
            }
        }
        for (uint32_t i = 0u; i < written; ++i) {
            const auto found = organ_at.find(organ_records_[i].object_id);
            if (found == organ_at.end()) {
                continue;  // the organ left its port in the same run; its lineage names nothing shown
            }
            if (!organ_lineage_of(organ_records_[i], *found->second)) {
                return refuse_record(*host_, config_.lineage_port, organ_records_[i].object_id,
                                     "lineage missing or no whole number in range");
            }
        }
    }
    // The sway of every organ on a branch, from the same Production run: one record per organ, its object id the
    // organ; an organ without one keeps GODOT_SWAY_NONE and stands still.
    {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.sway_port, organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        std::unordered_map<uint32_t, VivariumOrganView*> organ_at;
        for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
            for (VivariumOrganView& organ : organs_[c]) {
                organ_at.emplace(organ.object_id, &organ);
            }
        }
        for (uint32_t i = 0u; i < written; ++i) {
            const auto found = organ_at.find(organ_records_[i].object_id);
            if (found == organ_at.end()) {
                continue;  // the organ left its port in the same run; its sway has nothing to swing
            }
            if (!organ_sway_of(organ_records_[i], *found->second)) {
                return refuse_record(*host_, config_.sway_port, organ_records_[i].object_id,
                                     "sway missing or out of range");
            }
        }
    }
    // The bend of every leaf card, from the same Production run (PLAN_ASE_VIVARIUM_TREEGEN T.6): one record per bent
    // leaf, its object id the leaf; a leaf without one keeps GODOT_BEND_NONE and is drawn flat.
    {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.bend_port, organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        std::vector<VivariumOrganView>& leaves = organs_[GODOT_ORGAN_LEAF];
        std::unordered_map<uint32_t, std::size_t> leaf_at;
        leaf_at.reserve(leaves.size());
        for (std::size_t l = 0u; l < leaves.size(); ++l) {
            leaf_at.emplace(leaves[l].object_id, l);
        }
        for (uint32_t i = 0u; i < written; ++i) {
            const auto found = leaf_at.find(organ_records_[i].object_id);
            if (found == leaf_at.end()) {
                continue;  // the leaf left the leaf port in the same run; its bend has no card to lift
            }
            if (!leaf_bend_of(organ_records_[i], leaves[found->second])) {
                return refuse_record(*host_, config_.bend_port, organ_records_[i].object_id,
                                     "leaf bend missing or not finite");
            }
        }
    }
    // The colour offset of every leaf card, from the same Production run (PLAN_ASE_VIVARIUM_TREEGEN T.6, Farbe,
    // Variante, Herbst): one record per leaf with an offset, its object id the leaf; a leaf without one keeps
    // GODOT_TINT_NONE and the colour of its species.
    {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.tint_port, organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        std::vector<VivariumOrganView>& leaves = organs_[GODOT_ORGAN_LEAF];
        std::unordered_map<uint32_t, std::size_t> leaf_at;
        leaf_at.reserve(leaves.size());
        for (std::size_t l = 0u; l < leaves.size(); ++l) {
            leaf_at.emplace(leaves[l].object_id, l);
        }
        for (uint32_t i = 0u; i < written; ++i) {
            const auto found = leaf_at.find(organ_records_[i].object_id);
            if (found == leaf_at.end()) {
                continue;  // the leaf left the leaf port in the same run; its offset has no card to colour
            }
            if (!leaf_tint_of(organ_records_[i], leaves[found->second])) {
                return refuse_record(*host_, config_.tint_port, organ_records_[i].object_id,
                                     "leaf colour offset missing or not finite");
            }
        }
    }
    // The atlas cell of every leaf card, from the same Production run (PLAN_ASE_VIVARIUM_TREEGEN T.6, Farbe,
    // Variante, Herbst): one record per leaf with a cell, its object id the leaf; a leaf without one keeps
    // GODOT_ATLS_CELL_NONE of GODOT_ATLS_CELLS_NONE and shows the whole texture of its species.
    {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.atlas_port, organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        std::vector<VivariumOrganView>& leaves = organs_[GODOT_ORGAN_LEAF];
        std::unordered_map<uint32_t, std::size_t> leaf_at;
        leaf_at.reserve(leaves.size());
        for (std::size_t l = 0u; l < leaves.size(); ++l) {
            leaf_at.emplace(leaves[l].object_id, l);
        }
        for (uint32_t i = 0u; i < written; ++i) {
            const auto found = leaf_at.find(organ_records_[i].object_id);
            if (found == leaf_at.end()) {
                continue;  // the leaf left the leaf port in the same run; its cell has no card to show
            }
            if (!leaf_atlas_of(organ_records_[i], leaves[found->second])) {
                return refuse_record(*host_, config_.atlas_port, organ_records_[i].object_id,
                                     "leaf atlas cell missing or not finite");
            }
        }
    }
    // The instances of every drawn class, from the same Production run as the organs they belong to.
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        uint32_t written = 0u;
        const HostStatus status = snapshot_all(*host_, config_.instance_ports[c], organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        instances_[c].clear();
        instances_[c].reserve(written);
        for (uint32_t i = 0u; i < written; ++i) {
            VivariumInstanceView view;
            if (!instance_view_of(organ_records_[i], view)) {
                instances_[c].clear();
                return refuse_record(*host_, config_.instance_ports[c], organ_records_[i].object_id,
                                     "instance values incomplete or out of range");
            }
            instances_[c].push_back(view);
        }
    }
    organ_generation_ += 1u;
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_bark() {
    // THE BARK SET COMES PAGE BY PAGE (register B0295 point 4). ase-render-data rebuilds the tubes of a body that
    // ticked in GODOT_ORGAN_SCHEDULE; ase-pl-flora files the changed set at GODOT_BARK_VERTS_PER_PASS vertices per
    // GODOT_BARK_SCHEDULE pass, and the host keeps the old set until the pass that files the last page. A read
    // after a new run therefore may still see the old set: the port is read with every snapshot until the new
    // set is in - a read that differs from the held set, or a read that names exactly the internodes the segment
    // port showed once the passes since the run covered the organ reads and every page of that set. A run that
    // changed no tube files nothing, and its first read already ends the reads.
    uint64_t runs = 0u;
    const HostStatus counted = host_->schedule_runs(GODOT_ORGAN_SCHEDULE, &runs);
    if (counted != ase::kernel::HostStatusOk) {
        return counted;
    }
    uint64_t passes = 0u;
    const HostStatus passed = host_->schedule_runs(GODOT_BARK_SCHEDULE, &passes);
    if (passed != ase::kernel::HostStatusOk) {
        return passed;
    }
    if (runs != bark_runs_ || bark_generation_ == 0u) {
        bark_runs_ = runs;
        bark_passes_ = passes;
        bark_owed_ = true;
    }
    if (!bark_owed_) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.bark_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumBarkVertexView> vertices;
    vertices.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumBarkVertexView view;
        if (!bark_vertex_view_of(organ_records_[i], view)) {
            bark_.clear();
            return refuse_record(*host_, config_.bark_port, organ_records_[i].object_id,
                                 "bark vertex incomplete or out of range");
        }
        vertices.push_back(view);
    }
    const uint64_t pages = (static_cast<uint64_t>(written) + GODOT_BARK_VERTS_PER_PASS - 1u) / GODOT_BARK_VERTS_PER_PASS;
    const bool covered = passes - bark_passes_ > GODOT_ORGAN_READS_PER_RUN + pages;
    const bool changed = !same_bark(vertices, bark_);
    if (changed) {
        bark_ = std::move(vertices);
        bark_generation_ += 1u;
        bark_owed_ = false;
        return ase::kernel::HostStatusOk;
    }
    if (covered && names_the_internodes(bark_, organs_[GODOT_ORGAN_SEGMENT])) {
        bark_owed_ = false;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_knots() {
    // THE KNOTS COME PAGE BY PAGE, LIKE THE BARK (N0543). ase-render-data carries the knots of a body that ticked
    // in GODOT_ORGAN_SCHEDULE; ase-pl-flora files the tip knots, the foot knots and the cut faces at
    // GODOT_KNOT_ROWS_PER_PASS rows per GODOT_KNOT_SCHEDULE pass, all three ports in the same pass. Whether a read is
    // owed and when the owed reads end is the knot store's account (GodotKnotResourceManager::due, store_read); this
    // reader takes the three snapshots and refuses a set with a record it cannot read whole. The organ buffer is
    // reused: the three reads run one after the other in the same snapshot, after the organs and the bark.
    uint64_t runs = 0u;
    const HostStatus counted = host_->schedule_runs(GODOT_ORGAN_SCHEDULE, &runs);
    if (counted != ase::kernel::HostStatusOk) {
        return counted;
    }
    uint64_t passes = 0u;
    const HostStatus passed = host_->schedule_runs(GODOT_KNOT_SCHEDULE, &passes);
    if (passed != ase::kernel::HostStatusOk) {
        return passed;
    }
    if (!knots_->due(runs, passes)) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    HostStatus status = snapshot_all(*host_, config_.knot_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumKnotView> knots;
    knots.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumKnotView view;
        if (!knot_view_of(organ_records_[i], view)) {
            return refuse_record(*host_, config_.knot_port, organ_records_[i].object_id,
                                 "tip knot incomplete or out of range");
        }
        knots.push_back(view);
    }
    status = snapshot_all(*host_, config_.foot_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumFootView> feet;
    feet.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumFootView view;
        if (!foot_view_of(organ_records_[i], view)) {
            return refuse_record(*host_, config_.foot_port, organ_records_[i].object_id,
                                 "foot knot incomplete or out of range");
        }
        feet.push_back(view);
    }
    status = snapshot_all(*host_, config_.scar_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumScarView> scars;
    scars.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumScarView view;
        if (!scar_view_of(organ_records_[i], view)) {
            return refuse_record(*host_, config_.scar_port, organ_records_[i].object_id,
                                 "cut face incomplete or out of range");
        }
        scars.push_back(view);
    }
    knots_->store_read(std::move(knots), std::move(feet), std::move(scars), passes, organs_[GODOT_ORGAN_SEGMENT]);
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_terrain() {
    // THE HEIGHTS CHANGE ONLY IN ONE SCHEDULE. ase-terrain publishes changed samples in
    // GODOT_TERRAIN_SCHEDULE; the plugin's sync system reads them in the Reception and the plugin lays them
    // on the port in the Production of the frame after - the same two reads per run as the organs. The organ
    // buffer is reused: both reads run one after the other in the same snapshot.
    uint64_t runs = 0u;
    const HostStatus counted = host_->schedule_runs(GODOT_TERRAIN_SCHEDULE, &runs);
    if (counted != ase::kernel::HostStatusOk) {
        return counted;
    }
    if (runs != terrain_runs_ || terrain_generation_ == 0u) {
        terrain_runs_ = runs;
        terrain_reads_due_ = GODOT_TERRAIN_READS_PER_RUN;
    }
    if (terrain_reads_due_ == 0u) {
        return ase::kernel::HostStatusOk;
    }
    terrain_reads_due_ -= 1u;
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.terrain_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumTerrainView> samples;
    samples.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumTerrainView view;
        if (!terrain_view_of(organ_records_[i], view)) {
            terrain_.clear();
            return refuse_record(*host_, config_.terrain_port, organ_records_[i].object_id,
                                 "terrain sample incomplete or out of range");
        }
        samples.push_back(view);
    }
    // the cells of the same ground, from the same snapshot: ase-render-data lays both in one Observation
    const HostStatus cells_read = snapshot_all(*host_, config_.ground_cell_port, organ_records_, written);
    if (cells_read != ase::kernel::HostStatusOk) {
        terrain_.clear();
        return cells_read;
    }
    std::vector<VivariumGroundCellView> cells;
    cells.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumGroundCellView view;
        if (!ground_cell_view_of(organ_records_[i], view)) {
            terrain_.clear();
            ground_cells_.clear();
            return refuse_record(*host_, config_.ground_cell_port, organ_records_[i].object_id,
                                 "ground cell incomplete or out of range");
        }
        cells.push_back(view);
    }
    // a read that carries the ground the view already holds is no change (same_ground)
    const bool changed = !same_ground(samples, cells, terrain_, ground_cells_);
    terrain_ = std::move(samples);
    ground_cells_ = std::move(cells);
    if (changed) {
        terrain_generation_ += 1u;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_streams() {
    // A handful of records per snapshot, one per brook: read every time, and only water that differs moves
    // stream_generation, so the view lays the ribbon anew when ase-hydro changed it and not per frame. The
    // organ buffer is reused: the reads run one after the other in the same snapshot.
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.stream_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumStreamView> streams;
    streams.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumStreamView view;
        if (!stream_view_of(organ_records_[i], view)) {
            streams_.clear();
            return refuse_record(*host_, config_.stream_port, organ_records_[i].object_id,
                                 "brook water incomplete or out of range");
        }
        streams.push_back(view);
    }
    const bool changed = !same_streams(streams, streams_);
    streams_ = std::move(streams);
    if (changed) {
        stream_generation_ += 1u;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_currents() {
    // THE RIVER CURRENT (PLAN_ASE_VIVARIUM_PHASE_10_HYDRO H.13): ase-hydro computes it per cell, ase-gis answers it and
    // ase-pl-water lays one record per current cell on water.current.v1. The port stands only once the plugin closed
    // its first set, so it is read once it is offered, and until then the view holds no current; that the start of
    // the view never waits on it keeps a plugin without its range rows from holding the whole scene. Only water that
    // differs moves current_generation, so the view lays it anew when ase-hydro changed it and not per frame.
    if (config_.current_port.empty() || !host_->has_port(config_.current_port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.current_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumCurrentView> currents;
    currents.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumCurrentView view;
        if (!current_view_of(organ_records_[i], view)) {
            currents_.clear();
            return refuse_record(*host_, config_.current_port, organ_records_[i].object_id,
                                 "current cell without its place");
        }
        currents.push_back(view);
    }
    // WHAT THE WATER CARRIES (D2508): ase-pl-water lays the algae, rot and salt of each current cell on the quality
    // port under the same row id. Joined here by object id, in one pass over each port; a cell without a quality
    // record keeps NaN, so the lake shader draws no bloom where ase-hydro measured none.
    if (!config_.quality_port.empty() && host_->has_port(config_.quality_port.c_str())) {
        std::unordered_map<uint32_t, std::size_t> cell_at;
        cell_at.reserve(currents.size());
        for (std::size_t i = 0u; i < currents.size(); ++i) {
            cell_at.emplace(currents[i].object_id, i);
        }
        uint32_t carried = 0u;
        const HostStatus quality_status = snapshot_all(*host_, config_.quality_port, organ_records_, carried);
        if (quality_status != ase::kernel::HostStatusOk) {
            return quality_status;
        }
        for (uint32_t i = 0u; i < carried; ++i) {
            const auto found = cell_at.find(organ_records_[i].object_id);
            if (found != cell_at.end()) {
                quality_into(organ_records_[i], currents[found->second]);
            }
        }
    }
    const bool changed = !same_currents(currents, currents_);
    currents_ = std::move(currents);
    if (changed) {
        current_generation_ += 1u;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_pits() {
    // THE WATER OF THE DUG PITS (H.4): ase-hydro computes it over the pit floor, ase-gis answers it and
    // ase-pl-hydrogeology lays one record per pit on hydrogeology.pit.v1. Read once the plugin offers the port, as the
    // current; the pit surface stands depth_m over the floor the ground of the view already carries.
    if (config_.pit_port.empty() || !host_->has_port(config_.pit_port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.pit_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumPitView> pits;
    pits.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumPitView view;
        if (!pit_view_of(organ_records_[i], view)) {
            pits_.clear();
            return refuse_record(*host_, config_.pit_port, organ_records_[i].object_id, "pit without its place");
        }
        pits.push_back(view);
    }
    const bool changed = !same_pits(pits, pits_);
    pits_ = std::move(pits);
    if (changed) {
        pit_generation_ += 1u;
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_groundwater() {
    // THE GROUNDWATER OF EVERY CHUNK (H.3): ase-hydro computes table, standing water, stock and bank flow per chunk,
    // ase-gis answers them under HUB_TERN_CHK and ase-pl-hydrogeology lays one record per chunk on
    // hydrogeology.groundwater.v1. Read once the plugin offers the port, as the current.
    if (config_.groundwater_port.empty() || !host_->has_port(config_.groundwater_port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.groundwater_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumGroundwaterView> groundwater;
    groundwater.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumGroundwaterView view;
        if (!groundwater_view_of(organ_records_[i], view)) {
            groundwater_.clear();
            return refuse_record(*host_, config_.groundwater_port, organ_records_[i].object_id,
                                 "groundwater without its chunk");
        }
        groundwater.push_back(view);
    }
    const bool changed = !same_groundwater(groundwater, groundwater_);
    groundwater_ = std::move(groundwater);
    if (changed) {
        groundwater_generation_ += 1u;
        note_groundwater(*host_, groundwater_);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_salt() {
    // THE SALT OF EVERY CHUNK (H.9, I-4): ase-hydro owns the water half, ase-pedosphere the soil half, ase-gis adds
    // both under HUB_TERN_CHK and ase-pl-hydrogeology lays one record per chunk on hydrogeology.salt.v1. Read once
    // the plugin offers the port, as the groundwater.
    if (config_.salt_port.empty() || !host_->has_port(config_.salt_port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.salt_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumSaltView> salt;
    salt.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumSaltView view;
        if (!salt_view_of(organ_records_[i], view)) {
            salt_.clear();
            return refuse_record(*host_, config_.salt_port, organ_records_[i].object_id, "salt without its chunk");
        }
        salt.push_back(view);
    }
    const bool changed = !same_salt(salt, salt_);
    salt_ = std::move(salt);
    if (changed) {
        salt_generation_ += 1u;
        note_salt(*host_, salt_);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_templates() {
    // THE TEMPLATES ARE BUILT ONCE (usecase:101011 tessellate the organ templates). ase-render-data builds the fine
    // and the coarse cylinder and the unit leaf in the Initialization of its host and keeps their rows; the
    // plugin lays them on its three template ports in every Production run. So the ports are read with every
    // snapshot until each carries a set, and not again while this host runs. Until the plugin's first
    // Production run both sets are empty: the templates are not known yet, and nothing is broken.
    if (!template_corners_.empty() && !template_triangles_.empty()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    HostStatus status = snapshot_all(*host_, config_.template_corner_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumTemplateCornerView> corners;
    corners.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumTemplateCornerView view;
        if (!template_corner_view_of(organ_records_[i], view)) {
            return refuse_record(*host_, config_.template_corner_port, organ_records_[i].object_id,
                                 "template corner incomplete or out of range");
        }
        corners.push_back(view);
    }
    // THE TANGENT OF EVERY CORNER (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.6) comes on a port of its own, since the
    // corner record is full; each record names its corner by code and index and is joined to it here. The
    // plugin lays it in the same Production run as the corners, so a set whose tangents are all missing is not
    // known yet and is read again with the next snapshot; a set with some corners missing theirs is broken.
    status = snapshot_all(*host_, config_.template_tangent_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written == 0u) {
        return ase::kernel::HostStatusOk;
    }
    std::unordered_map<uint64_t, std::size_t> place_of;
    place_of.reserve(corners.size());
    for (std::size_t c = 0u; c < corners.size(); ++c) {
        place_of.emplace(template_corner_key(corners[c].code, corners[c].index), c);
    }
    std::vector<bool> carried(corners.size(), false);
    for (uint32_t i = 0u; i < written; ++i) {
        if (!template_tangent_onto(organ_records_[i], place_of, corners, carried)) {
            return refuse_record(*host_, config_.template_tangent_port, organ_records_[i].object_id,
                                 "template tangent incomplete, out of range or without its corner");
        }
    }
    for (std::size_t c = 0u; c < corners.size(); ++c) {
        if (!carried[c]) {
            return refuse_record(*host_, config_.template_tangent_port, corners[c].object_id,
                                 "template corner without its tangent");
        }
    }
    status = snapshot_all(*host_, config_.template_triangle_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<VivariumTemplateTriangleView> triangles;
    triangles.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumTemplateTriangleView view;
        if (!template_triangle_view_of(organ_records_[i], view)) {
            return refuse_record(*host_, config_.template_triangle_port, organ_records_[i].object_id,
                                 "template triangle incomplete or out of range");
        }
        triangles.push_back(view);
    }
    if (corners.empty() || triangles.empty()) {
        return ase::kernel::HostStatusOk;
    }
    template_corners_ = std::move(corners);
    template_triangles_ = std::move(triangles);
    template_generation_ += 1u;
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_sky() {
    // THE SKY IS READ EVERY SNAPSHOT. ase-pl-sky lays its four records and one per visible moon, comet,
    // planet, companion star, moon half, belt and shower in every Production run (SkyHostScneExptSystem), a
    // handful of numbers - no schedule count saves anything here. Until the plugin's light pipeline has run
    // once the port publishes an empty set: the sky is not known yet, and nothing is broken. A set below the
    // four records of one sky, or above them plus every kind at its own maximum, is.
    //
    // THE UPPER BOUND IS THE SUM OF EVERY KIND THE VIEW READS. sky_view_of takes moons, comets, planets,
    // companions, halves, belts and showers, each refused above its own maximum; a bound of the four records
    // plus the moons alone threw away a whole sky the moment two moons and one comet stood above the
    // horizon, and the scene lost its light without any kind being over its own limit.
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.sky_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written == 0u) {
        sky_ = VivariumSkyView{};
        return ase::kernel::HostStatusOk;
    }
    // THE STARS STAND ON THEIR OWN PORT. The turn record (GODOT_SKY_RECORD_TURN) is the one star record
    // left on this port; the eclipse and the prediction records count in as every other kind.
    const uint32_t sky_records_max = GODOT_SKY_RECORD_COUNT + GODOT_SKY_MOONS_MAX + GODOT_SKY_COMETS_MAX +
                                     GODOT_SKY_PLANETS_MAX + GODOT_SKY_COMPANIONS_MAX +
                                     GODOT_SKY_FRAGMENTS_MAX + GODOT_SKY_BELTS_MAX + GODOT_SKY_SHOWERS_MAX +
                                     GODOT_SKY_ECLIPSES_MAX + GODOT_SKY_ECLIPSE_NEXTS_MAX + GODOT_SKY_TURNS_MAX +
                                     GODOT_SKY_WINDS_MAX;
    if (written < GODOT_SKY_RECORD_COUNT || written > sky_records_max) {
        sky_ = VivariumSkyView{};
        const std::string count = "record count " + std::to_string(written) + " outside " +
                                  std::to_string(GODOT_SKY_RECORD_COUNT) + " to " + std::to_string(sky_records_max);
        return refuse_record(*host_, config_.sky_port, organ_records_[0].object_id, count.c_str());
    }
    VivariumSkyView sky;
    if (!sky_view_of(organ_records_, written, sky)) {
        sky_ = VivariumSkyView{};
        return refuse_record(*host_, config_.sky_port, organ_records_[0].object_id,
                             "a sky record incomplete, out of range or above its kind's maximum");
    }
    // the held star set and the marks it was read at ride over to the new view; read_stars replaces them
    // only when the turn record names a set computed at other marks
    sky.star = std::move(sky_.star);
    sky.star_sidereal = sky_.star_sidereal;
    sky.star_zenith = sky_.star_zenith;
    sky.star_moon = sky_.star_moon;
    sky.star_generation = sky_.star_generation;
    // a set without the wind record keeps the last drift: ase-pl-sky files it only once wind arrived, and the
    // haze and the clouds hold their course until the next one (D2293)
    if (!sky.wind_known && sky_.wind_known) {
        sky.wind_known = true;
        sky.drift_x = sky_.drift_x;
        sky.drift_z = sky_.drift_z;
        sky.wind = sky_.wind;
    }
    sky_ = std::move(sky);
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_stars() {
    // THE STAR SET IS READ WHEN IT CHANGED, NEVER EVERY SNAPSHOT. ase-pl-sky computes its visible set anew
    // after a sidereal step or a change of the brightness (SkyCelStarVisSystem) and files it on the stars
    // port then (SkyHostStarExptSystem); the kernel keeps the last set between those closes. The marks the
    // set was computed at ride on the turn record of the scene port, so a frame whose marks equal those of
    // the held set reads nothing - thousands of records would otherwise be copied out of the kernel each
    // frame for a set that has not moved. The motion of the sky is the turn, applied by the view.
    if (!sky_.turn_known) {
        return ase::kernel::HostStatusOk;
    }
    const bool same_marks = sky_.star_sidereal == sky_.set_sidereal && sky_.star_zenith == sky_.set_zenith &&
                            sky_.star_moon == sky_.set_moon;
    if (same_marks && sky_.star_generation != 0u) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.stars_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written > GODOT_SKY_STARS_MAX) {
        sky_.star.clear();
        const std::string count = "record count " + std::to_string(written) + " above " +
                                  std::to_string(GODOT_SKY_STARS_MAX);
        return refuse_record(*host_, config_.stars_port, organ_records_[0].object_id, count.c_str());
    }
    std::vector<VivariumStarView> stars;
    if (!stars_view_of(organ_records_, written, stars)) {
        sky_.star.clear();
        return refuse_record(*host_, config_.stars_port, organ_records_[0].object_id,
                             "a star record incomplete, not finite or of another kind");
    }
    sky_.star = std::move(stars);
    sky_.star_sidereal = sky_.set_sidereal;
    sky_.star_zenith = sky_.set_zenith;
    sky_.star_moon = sky_.set_moon;
    star_generation_ += 1u;
    sky_.star_generation = star_generation_;
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_seeds() {
    // THE SEEDS ARE READ EVERY SNAPSHOT. ase-physics moves a flying seed in every kinetic step and the plugin
    // lays the port anew in every Production run (FloraHostSeedExptSystem); no schedule count saves a read of
    // a set that changes every frame. An empty set is a scene without a shattered fruit stand, nothing broken.
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.seed_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    seeds_.clear();
    seeds_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumSeedView view;
        if (!seed_view_of(organ_records_[i], view)) {
            seeds_.clear();
            return refuse_record(*host_, config_.seed_port, organ_records_[i].object_id,
                                 "seed body incomplete or out of range");
        }
        seeds_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_racks() {
    // THE RACKS ARE READ EVERY SNAPSHOT (usecase:101625): ase-physics drops a laid rack in its kinetic steps and
    // ase-pl-toolrack lays the rack port anew in every Production run (ToolrackHostBodyExptSystem). A host
    // without the plugin offers no rack port: no rack, nothing broken.
    racks_.clear();
    if (!rack_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.toolrack_rack_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    racks_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumRackView view;
        if (!rack_view_of(organ_records_[i], view)) {
            racks_.clear();
            return refuse_record(*host_, config_.toolrack_rack_port, organ_records_[i].object_id,
                                 "rack body incomplete or out of range");
        }
        racks_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_survival_bodies() {
    // THE BODIES ARE READ EVERY SNAPSHOT (U01, U03, N0530): ase-metabolism writes the MTB_* values of a figure
    // in every metabolism tick, and the port lays them per figure. A host without the port offers none: the
    // bridge then draws the SHA defaults and logs its gap. A key the record does not carry stays NaN and its
    // row is not shown; a key that stands but is not finite is refused.
    survival_bodies_.clear();
    if (!survival_body_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.survival_body_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    survival_bodies_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        VivariumSurvivalBodyView view;
        view.object_id = record.object_id;
        view.effective_temp = record_value(record, kSurvivalKeyEffectiveTemp);
        view.temp_stress = record_value(record, kSurvivalKeyTempStress);
        view.hunger = record_value(record, kSurvivalKeyHunger);
        view.thirst = record_value(record, kSurvivalKeyThirst);
        view.energy = record_value(record, kSurvivalKeyEnergy);
        view.fatigue = record_value(record, kSurvivalKeyFatigue);
        view.health = record_value(record, kSurvivalKeyHealth);
        view.poisoned = record_value(record, kSurvivalKeyPoisoned);
        view.bleeding = record_value(record, kSurvivalKeyBleeding);
        view.emaciated = record_value(record, kSurvivalKeyEmaciated);
        view.well_rested = record_value(record, kSurvivalKeyWellRested);
        view.breath = record_value(record, kSurvivalKeyBreath);
        for (const double value : {view.effective_temp, view.temp_stress, view.hunger, view.thirst, view.energy,
                                   view.fatigue, view.health, view.poisoned, view.bleeding, view.emaciated,
                                   view.well_rested, view.breath}) {
            if (std::isinf(value)) {
                survival_bodies_.clear();
                return refuse_record(*host_, config_.survival_body_port, record.object_id,
                                     "survival body value not finite");
            }
        }
        survival_bodies_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_survival_rests() {
    // THE RESTS ARE READ EVERY SNAPSHOT (U05, N0555): ase-metabolism writes MTB_IS_RESTING and MTB_IS_SLEEPING of a
    // figure in every Regulation pass from the action ase-bdi chose, with MTB_REST_QUALITY and the target of good
    // rest beside them for the side of the sleep (T.11, N0578), and the rest port lays them per figure, apart
    // from the body port, whose record is full at the width of the host. A host without the port offers none: the
    // bridge then keeps the mockup's pose and logs its gap. A key the record does not carry stays NaN; a key that
    // stands but is not finite is refused.
    survival_rests_.clear();
    if (!survival_rest_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.survival_rest_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    survival_rests_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        VivariumSurvivalRestView view;
        view.object_id = record.object_id;
        view.is_resting = record_value(record, kSurvivalKeyIsResting);
        view.sleeping = record_value(record, kSurvivalKeySleeping);
        view.rest_quality = record_value(record, kSurvivalKeyRestQuality);
        view.rest_target = record_value(record, kSurvivalKeyRestTarget);
        for (const double value : {view.is_resting, view.sleeping, view.rest_quality, view.rest_target}) {
            if (std::isinf(value)) {
                survival_rests_.clear();
                return refuse_record(*host_, config_.survival_rest_port, record.object_id,
                                     "survival rest value not finite");
            }
        }
        survival_rests_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_survival_foods() {
    // THE SPOIL IS READ EVERY SNAPSHOT (T.8, U07, N0530): ase-entropy rots every piece of food in a bag by the
    // temperature it lies at in each Coordination pass and lays the share of the bearer as ENT_FOOD_ROT_CARRIED, and
    // ase-pl-survival lays one record per figure that carries food in Production (SurvivalHostFoodExptSystem). A host
    // without the port offers none: the bridge then draws the mockup's ring and logs its gap. A key the record does
    // not carry stays NaN; a key that stands but is not finite is refused.
    survival_foods_.clear();
    if (!survival_food_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.survival_food_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    survival_foods_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        VivariumSurvivalFoodView view;
        view.object_id = record.object_id;
        view.spoil = record_value(record, kSurvivalKeySpoil);
        if (std::isinf(view.spoil)) {
            survival_foods_.clear();
            return refuse_record(*host_, config_.survival_food_port, record.object_id, "survival food value not finite");
        }
        survival_foods_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_survival_smokes() {
    // THE SOOT IS READ EVERY SNAPSHOT (T.8, U06, D2500): ase-fire lays the soot of every fire cell on the raster cell
    // of its fuel body in each Dissemination pass (FireSmokCellCalcSystem, HUB_GIS_SMOK_CELL_REQ), ase-gis sums it per
    // cell under each figure as GIS_LOCAL_SOOT in Regulation (GisLyrNthrSmokCellSystem), and ase-pl-survival lays one
    // record per figure in Production (SurvivalHostSmokExptSystem). A host without the port offers none: the bridge
    // then leaves the plume on the owner's grade of the air and logs its gap. A key the record does not carry stays
    // NaN; a key that stands but is not finite is refused.
    survival_smokes_.clear();
    if (!survival_smoke_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.survival_smoke_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    survival_smokes_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        VivariumSurvivalSmokeView view;
        view.object_id = record.object_id;
        view.soot = record_value(record, kSurvivalKeySoot);
        view.smell = record_value(record, kSurvivalKeySmell);
        if (std::isinf(view.soot) || std::isinf(view.smell)) {
            survival_smokes_.clear();
            return refuse_record(*host_, config_.survival_smoke_port, record.object_id, "survival smoke value not finite");
        }
        survival_smokes_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_survival_fires() {
    // THE FIRE CELLS ARE READ EVERY SNAPSHOT (U04, B0657): ase-fire turns its HUB_FIRE_CELL rows in every fire tick,
    // ase-atmosphere its air, and ase-pl-ingle lays one record per cell in Production (IngleHostFireExptSystem). A
    // host without the port offers no cell: the bridge then draws the SHA defaults and logs its gap. A key the
    // record does not carry stays NaN; a key that stands but is not finite is refused.
    survival_fires_.clear();
    if (!survival_fire_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.survival_fire_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    survival_fires_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        VivariumSurvivalFireView view;
        view.object_id = record.object_id;
        view.flame = record_value(record, kFireKeyFlame);
        view.ember = record_value(record, kFireKeyEmber);
        view.ash = record_value(record, kFireKeyAsh);
        view.fuel = record_value(record, kFireKeyFuel);
        view.fuel_laid = record_value(record, kFireKeyFuelLaid);
        view.heat = record_value(record, kFireKeyHeat);
        view.body = record_value(record, kFireKeyBody);
        view.oxygen = record_value(record, kFireKeyOxygen);
        view.toxicity = record_value(record, kFireKeyToxicity);
        view.charcoal = record_value(record, kFireKeyCharcoal);
        view.breath_state = record_value(record, kFireKeyBreath);
        view.breath_normal = record_value(record, kFireKeyBreathNormal);
        // The ash and the burn time travel on ingle.left.v1 (read_survival_left, right after this pass); until a
        // left record names this cell, neither is measured.
        view.ash_kg = std::nan("");
        view.burn_left = std::nan("");
        for (const double value : {view.flame, view.charcoal, view.ember, view.ash, view.fuel, view.fuel_laid,
                                   view.heat, view.body, view.oxygen, view.toxicity, view.breath_state,
                                   view.breath_normal}) {
            if (std::isinf(value)) {
                survival_fires_.clear();
                return refuse_record(*host_, config_.survival_fire_port, record.object_id,
                                     "fire cell value not finite");
            }
        }
        survival_fires_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_survival_left() {
    // THE ASH AND THE BURN TIME ARE READ EVERY SNAPSHOT, RIGHT AFTER THE FIRE CELLS (T.8, E0257): the fire record
    // holds the twelve values a host record takes, so ase-pl-ingle lays the kg of ash (HUB_ENT_ASH_REQ_ASH) and the
    // seconds the fuel lasts (HUB_FIRE_CELL_BURN_LEFT) on a port of its own, object id = the same fire cell
    // (IngleHostLeftExptSystem). Each left record is joined onto the fire cell of its object id; a key it does not
    // carry stays NaN, a key that stands but is not finite is refused, and a left record without a fire cell of
    // the same pass belongs to no cell the card shows and is left.
    if (!survival_left_offered() || survival_fires_.empty()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.survival_left_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        const double ash_kg = record_value(record, kLeftKeyAshKg);
        const double burn_left = record_value(record, kLeftKeyBurnLeft);
        if (std::isinf(ash_kg) || std::isinf(burn_left)) {
            return refuse_record(*host_, config_.survival_left_port, record.object_id,
                                 "fire cell ash or burn time not finite");
        }
        for (VivariumSurvivalFireView& view : survival_fires_) {
            if (view.object_id == record.object_id) {
                view.ash_kg = ash_kg;
                view.burn_left = burn_left;
            }
        }
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_weather() {
    // THE WEATHER IS READ EVERY SNAPSHOT (N0449, usecase:5606 observe the sky): ase-atmosphere computes the kind
    // (ATM_WEATHER_KIND), the temperature and the wind (ATM_WIND_SPEED) and ase-pl-sky files them as the current
    // record of sky.weather.v1 (SkyHostWthrExptSystem); one forecast record per coming day period is read beside
    // it, a record no module computes yet (N0449, owed by ase-atmosphere); the record that carries a period is a
    // forecast, the one without is the current weather. The kinds are compared against ase-atmosphere's codes on
    // sky.weather.code.v1 (SkyHostWthrCodeExptSystem, ATM_CONST_WEATHER_KIND_*), as the condition and stage
    // codes of ase-plant, never against a number of this adapter. A port not offered or a record not yet filed
    // leaves the weather unknown and the HUD's bridge draws SHA's; a record that stands with a key missing, twice or
    // out of range is refused.
    weather_ = VivariumWeatherView{};
    uint32_t written = 0u;
    bool codes = false;
    if (!config_.weather_code_port.empty() && host_->has_port(config_.weather_code_port.c_str())) {
        const HostStatus status = snapshot_all(*host_, config_.weather_code_port, organ_records_, written);
        if (status != ase::kernel::HostStatusOk) {
            return status;
        }
        if (written > 1u) {
            return refuse_record(*host_, config_.weather_code_port, organ_records_[1].object_id,
                                 "more than one weather code record");
        }
        if (written == 1u) {
            const KernelHostRecord& record = organ_records_[0];
            weather_.code_clear = record_value(record, kWeatherCodeClear);
            weather_.code_pcloud = record_value(record, kWeatherCodePcloud);
            weather_.code_cloudy = record_value(record, kWeatherCodeCloudy);
            weather_.code_drizzle = record_value(record, kWeatherCodeDrizzle);
            weather_.code_rain = record_value(record, kWeatherCodeRain);
            weather_.code_snow = record_value(record, kWeatherCodeSnow);
            weather_.code_fog = record_value(record, kWeatherCodeFog);
            const double read_codes[] = {weather_.code_clear,   weather_.code_pcloud, weather_.code_cloudy,
                                         weather_.code_drizzle, weather_.code_rain,   weather_.code_snow,
                                         weather_.code_fog};
            std::size_t whole_codes = 0u;
            for (const double code : read_codes) {
                whole_codes += whole(code) ? 1u : 0u;
            }
            if (whole_codes != std::size(read_codes)) {
                return refuse_record(*host_, config_.weather_code_port, record.object_id,
                                     "weather codes incomplete: the seven kinds come together as whole codes");
            }
            codes = true;
        }
    }
    if (config_.weather_port.empty() || !host_->has_port(config_.weather_port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    const HostStatus status = snapshot_all(*host_, config_.weather_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    bool current = false;
    bool filed[GODOT_DAY_PHASES] = {};
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        const double period = record_value(record, GODOT_KEY_PERIOD);
        const double kind = record_value(record, kWeatherKeyKind);
        const double temperature = record_value(record, kWeatherKeyTemperature);
        if (std::isnan(period)) {
            const double wind = record_value(record, kWeatherKeyWind);
            if (current) {
                return refuse_record(*host_, config_.weather_port, record.object_id, "more than one current weather");
            }
            if (!whole(kind) || !std::isfinite(temperature) || !std::isfinite(wind) || wind < 0.0) {
                return refuse_record(*host_, config_.weather_port, record.object_id,
                                     "current weather incomplete: the kind, the temperature and the wind from 0 "
                                     "come together");
            }
            weather_.kind = kind;
            weather_.temperature = temperature;
            weather_.wind = wind;
            current = true;
            continue;
        }
        if (!whole(period) || period < static_cast<double>(GODOT_PERIOD_FIRST) ||
            period > static_cast<double>(GODOT_PERIOD_LAST)) {
            return refuse_record(*host_, config_.weather_port, record.object_id, "forecast period out of range");
        }
        const double night = record_value(record, kWeatherKeyNight);
        const double rain_mm = record_value(record, kWeatherKeyRainMm);
        if (!whole(kind) || !std::isfinite(temperature) || !(night == 0.0 || night == 1.0) ||
            !std::isfinite(rain_mm) || rain_mm < 0.0) {
            return refuse_record(*host_, config_.weather_port, record.object_id,
                                 "forecast incomplete: the kind, the temperature, the night as 0 or 1 and the "
                                 "rain from 0 come together");
        }
        const std::size_t slot = static_cast<std::size_t>(period) - static_cast<std::size_t>(GODOT_PERIOD_FIRST);
        if (filed[slot]) {
            return refuse_record(*host_, config_.weather_port, record.object_id, "forecast period twice");
        }
        filed[slot] = true;
        // The coming periods stand in the order the weather port files them: which period comes next is the clock of
        // ase-time, never a rotation of this adapter.
        weather_.forecast.push_back(VivariumWeatherPhaseView{period, kind, night, temperature, rain_mm});
    }
    weather_.known = codes && current;
    weather_.forecast_known = codes && weather_.forecast.size() == GODOT_DAY_PHASES;
    if (!weather_.forecast_known) {
        weather_.forecast.clear();
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_sap() {
    // THE SAP IS READ EVERY SNAPSHOT (N0457): ase-pl-flora files one record for the birch's tap on flora.sap.v1, whether
    // it runs, the litres the bark vessel holds now and the litres it holds when full; the vessel fills on the owner's
    // side, never in this adapter. A port not offered or no record filed leaves the sap unknown and the HUD's bridge
    // draws SHA's tap; a second record or a key that stands out of range is refused.
    sap_ = VivariumSapView{};
    if (config_.sap_port.empty() || !host_->has_port(config_.sap_port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.sap_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written == 0u) {
        return ase::kernel::HostStatusOk;
    }
    if (written > 1u) {
        return refuse_record(*host_, config_.sap_port, organ_records_[1].object_id, "more than one sap record");
    }
    const auto& record = organ_records_[0];
    const double running = record_value(record, kSapKeyRunning);
    const double held_l = record_value(record, kSapKeyHeldL);
    const double cap_l = record_value(record, kSapKeyCapL);
    const bool flag = running == 0.0 || running == 1.0;
    if (!flag || !std::isfinite(held_l) || held_l < 0.0 || !std::isfinite(cap_l) || !(cap_l > 0.0)) {
        return refuse_record(*host_, config_.sap_port, record.object_id,
                             "sap incomplete: running 1 or 0, held_l from 0 and cap_l above 0 come together");
    }
    sap_.known = true;
    sap_.running = running == 1.0;
    sap_.held_l = held_l;
    sap_.cap_l = cap_l;
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_loose() {
    // THE LOOSENED SOIL IS THE SOIL'S OWN, READ EVERY SNAPSHOT (N0456, m.html patchLoose): ase-terrain keeps the
    // compaction of every macro cell (TerrainStaMcroCpctComponent, ase-gis GIS_CAT_LITHO_CPCT), the spade lowers it,
    // rain settles it back, and its rows HUB_TERN_MCRO reach the host through ase-pl-flora (FloraHostCpctExptSystem):
    // one record per cell on terrain.loose.v1 with the cell's centre x and z, its edge cell_m and cpct_share 0..1.
    // The looseness the HUD draws is the complement of that share; the adapter keeps the cells as the grid they form
    // and never computes a compaction. A port not offered or no record filed leaves the raster unknown and the HUD's
    // loose bridge draws the mockup's grid. Cells of different edges, a share out of 0..1, a cell twice or a cell
    // missing from the grid the records span is a broken raster and refused.
    loose_ = VivariumLooseView{};
    if (config_.loose_port.empty() || !host_->has_port(config_.loose_port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.loose_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written == 0u) {
        return ase::kernel::HostStatusOk;
    }
    const double cell_m = record_value(organ_records_[0], GODOT_KEY_CELL_M);
    if (!std::isfinite(cell_m) || !(cell_m > 0.0)) {
        return refuse_record(*host_, config_.loose_port, organ_records_[0].object_id, "loose cell without an edge");
    }
    double west_x = record_value(organ_records_[0], GODOT_KEY_X);
    double east_x = west_x;
    double south_z = record_value(organ_records_[0], GODOT_KEY_Z);
    double north_z = south_z;
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        const double x = record_value(record, GODOT_KEY_X);
        const double z = record_value(record, GODOT_KEY_Z);
        const double share = record_value(record, GODOT_KEY_CPCT_SHARE);
        if (!std::isfinite(x) || !std::isfinite(z) || record_value(record, GODOT_KEY_CELL_M) != cell_m ||
            !std::isfinite(share) || share < 0.0 || share > 1.0) {
            return refuse_record(*host_, config_.loose_port, record.object_id,
                                 "loose cell incomplete: x, z, the raster's cell_m and cpct_share 0..1 come together");
        }
        west_x = ase::math::min(west_x, x);
        east_x = ase::math::max(east_x, x);
        south_z = ase::math::min(south_z, z);
        north_z = ase::math::max(north_z, z);
    }
    const int64_t columns = std::llround((east_x - west_x) / cell_m) + 1;
    const int64_t rows = std::llround((north_z - south_z) / cell_m) + 1;
    if (columns * rows != static_cast<int64_t>(written)) {
        return refuse_record(*host_, config_.loose_port, organ_records_[0].object_id,
                             "loose raster with gaps: every cell of its grid comes exactly once");
    }
    std::vector<double> loose(static_cast<size_t>(written), std::nan(""));
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        const int64_t column = std::llround((record_value(record, GODOT_KEY_X) - west_x) / cell_m);
        const int64_t row = std::llround((record_value(record, GODOT_KEY_Z) - south_z) / cell_m);
        const size_t cell = static_cast<size_t>(row * columns + column);
        if (!std::isnan(loose[cell])) {
            return refuse_record(*host_, config_.loose_port, record.object_id, "loose cell filed twice");
        }
        loose[cell] = 1.0 - record_value(record, GODOT_KEY_CPCT_SHARE);
    }
    loose_.known = true;
    loose_.loose = std::move(loose);
    loose_.columns = columns;
    loose_.rows = rows;
    loose_.west_x = west_x;
    loose_.south_z = south_z;
    loose_.cell_m = cell_m;
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_camp_bodies() {
    // THE CAMP BODIES ARE READ EVERY SNAPSHOT (T.3): ase-physics drops, tips and rests every part in its kinetic
    // steps, and ase-pl-ingle lays one record per part body in Production (IngleHostBodyExptSystem). A host
    // without the plugin offers no body port: no part drawn, nothing broken. A body whose place, turn, patch or
    // make is missing or not a whole id is refused: the plugin lays all of them for every body it files.
    camp_bodies_.clear();
    if (!camp_body_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.camp_body_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    camp_bodies_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        VivariumCampBodyView view;
        view.object_id = record.object_id;
        const double patch = record_value(record, kCampKeyPatch);
        const double make = record_value(record, kCampKeyMake);
        view.x = record_value(record, GODOT_KEY_X);
        view.y = record_value(record, GODOT_KEY_Y);
        view.z = record_value(record, GODOT_KEY_Z);
        view.tx = record_value(record, kCampKeyTurnX);
        view.ty = record_value(record, kCampKeyTurnY);
        view.tz = record_value(record, kCampKeyTurnZ);
        const bool finite = std::isfinite(view.x) && std::isfinite(view.y) && std::isfinite(view.z) &&
                            std::isfinite(view.tx) && std::isfinite(view.ty) && std::isfinite(view.tz);
        if (!camp_id(patch) || !camp_id(make) || !finite) {
            camp_bodies_.clear();
            return refuse_record(*host_, config_.camp_body_port, record.object_id,
                                 "camp body incomplete or out of range");
        }
        view.patch = static_cast<uint32_t>(patch);
        view.make = static_cast<uint32_t>(make);
        camp_bodies_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_camp_refuels() {
    // THE KEEPERS ARE READ EVERY SNAPSHOT (T.7): ase-bdi sets a keeper's action to laying wood, and ase-pl-ingle
    // lays one record per such keeper in Production (IngleHostFuelExptSystem): the body he tends and, while
    // ase-fire names one, its fire cell. A record without a whole body id is refused; a missing cell stays apart.
    camp_refuels_.clear();
    if (!camp_refuel_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.camp_refuel_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    camp_refuels_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        const KernelHostRecord& record = organ_records_[i];
        VivariumCampRefuelView view;
        view.keeper = record.object_id;
        const double body = record_value(record, kCampKeyBody);
        const double cell = record_value(record, kCampKeyCell);
        if (!camp_id(body)) {
            camp_refuels_.clear();
            return refuse_record(*host_, config_.camp_refuel_port, record.object_id, "refuel without a body");
        }
        view.body = static_cast<uint32_t>(body);
        if (!std::isnan(cell)) {
            if (!camp_id(cell)) {
                camp_refuels_.clear();
                return refuse_record(*host_, config_.camp_refuel_port, record.object_id, "refuel cell out of range");
            }
            view.celled = true;
            view.cell = static_cast<uint32_t>(cell);
        }
        camp_refuels_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::read_figures() {
    // THE FIGURES ARE READ EVERY SNAPSHOT (07.8): ase-player moves a figure in every frame and ase-pl-avatar lays the
    // body port anew in every Production run (AvatarHostBodyExptSystem), the camera port after it
    // (AvatarHostCamExptSystem). A host without the plugin offers neither port: no figure, nothing broken.
    figures_.clear();
    figure_camera_ = VivariumFigureCameraView{};
    figure_gear_.clear();
    figure_code_ = VivariumFigureCodeView{};
    figure_bumps_.clear();
    if (!figure_offered()) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    HostStatus status = snapshot_all(*host_, config_.figure_body_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    figures_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumFigureView view;
        if (!figure_view_of(organ_records_[i], view)) {
            figures_.clear();
            return refuse_record(*host_, config_.figure_body_port, organ_records_[i].object_id,
                                 "figure body incomplete or not finite");
        }
        figures_.push_back(view);
    }
    // one active camera at most: the camera port carries a record only while that camera follows a figure
    status = snapshot_all(*host_, config_.figure_camera_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written > 1u) {
        return refuse_record(*host_, config_.figure_camera_port, organ_records_[0].object_id,
                             "more than one active player camera");
    }
    if (written == 1u && !figure_camera_of(organ_records_[0], figure_camera_)) {
        figure_camera_ = VivariumFigureCameraView{};
        return refuse_record(*host_, config_.figure_camera_port, organ_records_[0].object_id,
                             "player camera without eye or aim");
    }
    // what the figures carry (07.10): the plugin closes the gear set every Production run after the camera
    // (AvatarHostGearExptSystem), so a figure without a record holds nothing this frame
    status = snapshot_all(*host_, config_.figure_gear_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    figure_gear_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumFigureGearView piece;
        if (!figure_gear_of(organ_records_[i], piece)) {
            figure_gear_.clear();
            return refuse_record(*host_, config_.figure_gear_port, organ_records_[i].object_id,
                                 "gear piece without bearer, slot or kind as whole id, or with a broken water_l");
        }
        figure_gear_.push_back(piece);
    }
    // the bumps against the edge of the first own chunk (07.13, usecase:101612): the plugin closes the boundary set
    // every Production run after the organism record (AvatarHostBndyExptSystem), one record per walking body that
    // bumped since it walks. The edge is the terrain's and holds every walking body, so the set is read before the
    // codes and stands with or without a figure the view draws
    status = snapshot_all(*host_, config_.figure_boundary_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    figure_bumps_.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        VivariumFigureBumpView bump;
        if (!figure_bump_of(organ_records_[i], bump)) {
            figure_bumps_.clear();
            return refuse_record(*host_, config_.figure_boundary_port, organ_records_[i].object_id,
                                 "bump without normal, touched point or count, or with a value not finite");
        }
        figure_bumps_.push_back(bump);
    }
    // what gait, soul and camera of the body record mean (usecase:101611): the plugin files one code record every
    // Production run (AvatarHostCodeExptSystem), so standing figures without it are a broken plugin, never a view
    // that guesses the codes
    status = snapshot_all(*host_, config_.figure_code_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written > 1u) {
        return refuse_record(*host_, config_.figure_code_port, organ_records_[0].object_id,
                             "more than one code record");
    }
    if (written == 0u) {
        return figures_.empty() ? ase::kernel::HostStatusOk
                                : refuse_record(*host_, config_.figure_code_port, kFigureCodeObject,
                                                "figures without their codes");
    }
    if (!figure_code_of(organ_records_[0], figure_code_)) {
        figure_code_ = VivariumFigureCodeView{};
        return refuse_record(*host_, config_.figure_code_port, organ_records_[0].object_id,
                             "code record incomplete or not finite");
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::spawn_figure(uint32_t seat, double x, double z) {
    // The request names the seat and a place: the figure is ase-player's, the place on the ground the plugin's to
    // check. The plugin's second horizontal axis is Godot's north.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = seat;
    (void)put_value(input, GODOT_KEY_X, x);
    (void)put_value(input, GODOT_KEY_Y, z);
    // The character of the seat, as the id the units key their catalogs with; ase-pl-avatar carries it on
    // the spawn row and ase-creation gives the figure the genome and the start values of that character.
    (void)put_value(input, kFigureKeyCharacter,
                    static_cast<double>(KernelEmbeddedHost::name_id(config_.figure_character.c_str())));
    return host_->submit(config_.figure_spawn_port.c_str(), config_.op_spawn.c_str(), &input);
}

HostStatus GodotHostResourceManager::steer_figure(uint32_t figure, double forward, double strafe, double sprint,
                                                  double jump) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyForward, forward);
    (void)put_value(input, kFigureKeyStrafe, strafe);
    (void)put_value(input, kFigureKeySprint, sprint);
    (void)put_value(input, kFigureKeyJump, jump);
    return host_->submit(config_.figure_stick_port.c_str(), config_.op_steer.c_str(), &input);
}

HostStatus GodotHostResourceManager::turn_figure_camera(uint32_t figure, double yaw_delta, double pitch_delta) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    // AvatarHostOrbtSystem turns a camera only on a whole request: yaw, pitch AND zoom; a drag leaves the distance.
    (void)put_value(input, kFigureKeyYawDelta, yaw_delta);
    (void)put_value(input, kFigureKeyPitchDelta, pitch_delta);
    (void)put_value(input, kFigureKeyZoomDelta, 0.0);
    return host_->submit(config_.figure_orbit_port.c_str(), config_.op_turn.c_str(), &input);
}

HostStatus GodotHostResourceManager::zoom_figure_camera(uint32_t figure, double zoom_delta) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    // The same whole turn request as the drag, with the course left alone: only the orbit distance moves.
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyYawDelta, 0.0);
    (void)put_value(input, kFigureKeyPitchDelta, 0.0);
    (void)put_value(input, kFigureKeyZoomDelta, zoom_delta);
    return host_->submit(config_.figure_orbit_port.c_str(), config_.op_turn.c_str(), &input);
}

HostStatus GodotHostResourceManager::switch_figure_camera(uint32_t figure) {
    // The switch carries no value: which camera is active is ase-camera's, the view only asks for the other one.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    return host_->submit(config_.figure_switch_port.c_str(), config_.op_switch.c_str(), &input);
}

HostStatus GodotHostResourceManager::release_figure(uint32_t figure) {
    // soul.mode on a figure the player leads (07.11): the AI leads it on; no value, the port names the direction.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    return host_->submit(config_.figure_release_port.c_str(), config_.op_release.c_str(), &input);
}

HostStatus GodotHostResourceManager::seize_figure(uint32_t figure) {
    // soul.mode on a figure the AI leads (07.11): the player takes it back; no value, the port names the direction.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    return host_->submit(config_.figure_seize_port.c_str(), config_.op_seize.c_str(), &input);
}

HostStatus GodotHostResourceManager::hold_figure(uint32_t figure, uint32_t item) {
    // The hand of Phase 06 took a thing (07.10): the item travels as its catalog id; what the thing is and
    // whether the figure may carry it are ase-pl-avatar's and ase-equipment's to check.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyItem, static_cast<double>(item));
    return host_->submit(config_.figure_hold_port.c_str(), config_.op_hold.c_str(), &input);
}

HostStatus GodotHostResourceManager::don_figure(uint32_t figure, uint32_t item) {
    // The pack on the back (N0590): one port per place, so the back has its own port and the hand keeps its own;
    // the item travels as the catalog id of its key, what it is and whether it fits the back are ase-pl-avatar's
    // and ase-equipment's to check.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyItem, static_cast<double>(item));
    return host_->submit(config_.figure_don_port.c_str(), config_.op_don.c_str(), &input);
}

HostStatus GodotHostResourceManager::stock_figure(uint32_t figure, uint32_t item, double kg) {
    // One piece of the start stock (T.8, usecase:5378): the item travels as the hash of its item_art.json key, the
    // mass in kg; whether the figure stands and the piece is whole is ase-pl-survival's and ase-equipment's to check.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (config_.survival_stock_port.empty() || !host_->has_port(config_.survival_stock_port.c_str())) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kSurvivalKeyKind, static_cast<double>(item));
    (void)put_value(input, kSurvivalKeyKg, kg);
    return host_->submit(config_.survival_stock_port.c_str(), config_.op_stock.c_str(), &input);
}

HostStatus GodotHostResourceManager::lay_camp(uint32_t place, double east, double north) {
    // The camp place (T.0): measured on the height cells of ase-terrain and laid in the start file; the plugin lays
    // one camp entity per request, and whether the place is inside the world is its own check.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!camp_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = place;
    (void)put_value(input, kCampKeyEast, east);
    (void)put_value(input, kCampKeyNorth, north);
    return host_->submit(config_.camp_place_port.c_str(), config_.op_lay.c_str(), &input);
}

HostStatus GodotHostResourceManager::file_hull(uint32_t corner, uint32_t make, double hx, double hy, double hz) {
    // One corner of the convex hull of a make (T.1): the export carries it in the node extras of the recipe
    // variant, the plugin lays it as a corner row and ase-physics builds the collision form from all of them.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!camp_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = corner;
    (void)put_value(input, kCampKeyMake, static_cast<double>(make));
    (void)put_value(input, kCampKeyHullX, hx);
    (void)put_value(input, kCampKeyHullY, hy);
    (void)put_value(input, kCampKeyHullZ, hz);
    return host_->submit(config_.camp_hull_port.c_str(), config_.op_lay.c_str(), &input);
}

HostStatus GodotHostResourceManager::file_part(uint32_t patch, double east, double north, double drop, double fall,
                                               double mass, uint32_t make, double rough, double turn_x,
                                               double turn_y, double turn_z, bool fuel) {
    // One part of the camp as a rigid body (T.3): the plugin files HUB_PHYS_REG_BODY_ADD_CAMP and ase-physics lays
    // it in its turn a few millimetres above its ground, where it settles; fuel travels only on a part that burns,
    // its presence is the mark. Eleven values, inside the twelve a record holds.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!camp_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = patch;
    (void)put_value(input, kCampKeyEast, east);
    (void)put_value(input, kCampKeyNorth, north);
    (void)put_value(input, kCampKeyDrop, drop);
    (void)put_value(input, kCampKeyFall, fall);
    (void)put_value(input, kCampKeyMass, mass);
    (void)put_value(input, kCampKeyMake, static_cast<double>(make));
    (void)put_value(input, kCampKeyRough, rough);
    (void)put_value(input, kCampKeyTurnX, turn_x);
    (void)put_value(input, kCampKeyTurnY, turn_y);
    (void)put_value(input, kCampKeyTurnZ, turn_z);
    if (fuel) {
        (void)put_value(input, kCampKeyFuel, kCampFuelMark);
    }
    return host_->submit(config_.camp_part_port.c_str(), config_.op_lay.c_str(), &input);
}

HostStatus GodotHostResourceManager::drop_figure(uint32_t figure) {
    // The hand of Phase 06 let its thing go (07.10): no value, the port names the slot it empties.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    return host_->submit(config_.figure_drop_port.c_str(), config_.op_drop.c_str(), &input);
}

HostStatus GodotHostResourceManager::fill_figure(uint32_t figure) {
    // brookTap under SS/PC (07.14): no value, the engine fills the vessel in the hand to its room.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    return host_->submit(config_.figure_fill_port.c_str(), config_.op_fill.c_str(), &input);
}

HostStatus GodotHostResourceManager::drain_figure(uint32_t figure, double litres, uint32_t patch) {
    // The release of the can (07.14): the litres and the patch they fall on travel as they are; their range is
    // ase-pl-avatar's to check, and the soil column of the patch is ase-equipment's (N0517).
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyLitres, litres);
    (void)put_value(input, kFigureKeyPatch, static_cast<double>(patch));
    return host_->submit(config_.figure_drain_port.c_str(), config_.op_drain.c_str(), &input);
}

HostStatus GodotHostResourceManager::measure_figure(uint32_t figure, double eye_height, double height, double radius) {
    // The scene measured the skeleton it built (07.6): eye height, height and body radius go to the plugin as they
    // are; their range is ase-pl-avatar's to check, the face landing of ase-camera reads the eye height from there and
    // ase-physics height and radius as the capsule the walking body collides with.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyEyeHeight, eye_height);
    (void)put_value(input, kFigureKeyHeight, height);
    (void)put_value(input, kFigureKeyRadius, radius);
    return host_->submit(config_.figure_rig_port.c_str(), config_.op_measure.c_str(), &input);
}

HostStatus GodotHostResourceManager::turn_figure(uint32_t figure, double yaw_delta) {
    // fig.turn (07.7): the same operation as the orbit, on the figure's own turn port; the plugin bounds the delta.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyYawDelta, yaw_delta);
    return host_->submit(config_.figure_turn_port.c_str(), config_.op_turn.c_str(), &input);
}

HostStatus GodotHostResourceManager::bind_figure_area(uint32_t figure, uint32_t plate) {
    // 07.7: the plate travels as f64, which holds every u32 id exactly; whether it names a living plate and the
    // figure a figure of the star is ase-pl-avatar's to check (AvatarHostAreaSystem).
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!figure_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = figure;
    (void)put_value(input, kFigureKeyPlate, static_cast<double>(plate));
    return host_->submit(config_.figure_area_port.c_str(), config_.op_bind.c_str(), &input);
}

HostStatus GodotHostResourceManager::irrigate(uint32_t object_id, double amount_mm) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    input.object_id = object_id;
    if (!put_value(input, GODOT_KEY_AMOUNT_MM, amount_mm)) {
        return ase::kernel::HostStatusCapacity;
    }
    return host_->submit(config_.patch_port.c_str(), config_.op_irrigate.c_str(), &input);
}

HostStatus GodotHostResourceManager::prune(uint32_t object_id) {
    // The cut names only the internode: which branch it sits on and what falls with it is
    // ase-phytomer's. The request goes on the port the internode is laid out on.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    input.object_id = object_id;
    return host_->submit(config_.organ_ports[GODOT_ORGAN_SEGMENT].c_str(), config_.op_prune.c_str(), &input);
}

HostStatus GodotHostResourceManager::level(uint32_t object_id) {
    // The request names only the patch: its disc is the plugin's, the height ase-terrain's.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    input.object_id = object_id;
    return host_->submit(config_.level_port.c_str(), config_.op_level.c_str(), &input);
}

HostStatus GodotHostResourceManager::lay_brook(const VivariumBrookSpec& brook) {
    // The bed the player draws goes the way the configured brooks go at the start (lay_brooks).
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    const KernelHostInput input = brook_input(brook);
    return host_->submit(config_.brook_port.c_str(), config_.op_lay.c_str(), &input);
}

HostStatus GodotHostResourceManager::flatten(double x, double z, double radius_m) {
    // The request names a place, no patch: the disc is the player's, the height ase-terrain's.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_X, x);
    (void)put_value(input, GODOT_KEY_Z, z);
    (void)put_value(input, GODOT_KEY_HALF, radius_m);
    return host_->submit(config_.flatten_port.c_str(), config_.op_flatten.c_str(), &input);
}

HostStatus GodotHostResourceManager::lay_rack(uint32_t patch, double x, double z) {
    // The request names the patch and a place in its frame: the body and its height are ase-physics', the
    // range check and the one rack per patch the plugin's (ToolrackHostPlcmSystem).
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (!rack_offered()) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    input.object_id = patch;
    (void)put_value(input, GODOT_KEY_X, x);
    (void)put_value(input, GODOT_KEY_Z, z);
    return host_->submit(config_.toolrack_scene_port.c_str(), config_.op_lay.c_str(), &input);
}

HostStatus GodotHostResourceManager::lower(double x, double z, double radius_m, double depth_m) {
    // A spade stroke names a place, no patch: ase-pl-flora files it as a dig order, ase-terrain lowers the disc.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_X, x);
    (void)put_value(input, GODOT_KEY_Z, z);
    (void)put_value(input, GODOT_KEY_HALF, radius_m);
    (void)put_value(input, GODOT_KEY_DEPTH, depth_m);
    return host_->submit(config_.lower_port.c_str(), config_.op_lower.c_str(), &input);
}

HostStatus GodotHostResourceManager::raise(double x, double z, double radius_m, double height_m) {
    // The heap stroke carries its height under the same key as the dig depth: one stroke, one amount.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_X, x);
    (void)put_value(input, GODOT_KEY_Z, z);
    (void)put_value(input, GODOT_KEY_HALF, radius_m);
    (void)put_value(input, GODOT_KEY_DEPTH, height_m);
    return host_->submit(config_.raise_port.c_str(), config_.op_raise.c_str(), &input);
}

HostStatus GodotHostResourceManager::loosen(double x, double z, double half_m, double depth_m) {
    // A hoe stroke names a place, no patch: ase-pl-flora files it as a cultivation order, ase-terrain loosens the
    // topsoil of the block (B0649). The port is asked first: a plugin set without it answers NotFound, never a
    // refused submit, and the view keeps its bridge (N0456).
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    if (config_.loosen_port.empty() || !host_->has_port(config_.loosen_port.c_str())) {
        return ase::kernel::HostStatusNotFound;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_X, x);
    (void)put_value(input, GODOT_KEY_Z, z);
    (void)put_value(input, GODOT_KEY_HALF, half_m);
    (void)put_value(input, GODOT_KEY_DEPTH, depth_m);
    return host_->submit(config_.loosen_port.c_str(), config_.op_loosen.c_str(), &input);
}

HostStatus GodotHostResourceManager::pace(double factor) {
    // The request names no object: the clock is the one clock of this host. The view sends the factor
    // only; the base pace it multiplies and the highest pace that is valid are ase-time's, read from the
    // star by ase-pl-flora (FloraHostPaceSystem).
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_FACTOR, factor);
    return host_->submit(config_.pace_port.c_str(), config_.op_pace.c_str(), &input);
}

HostStatus GodotHostResourceManager::seek(double seq, double game_hour, double ahead_h, double day_hour) {
    // The request names no object: the clock is the one clock of this host. Every form stands on the
    // request, the unused ones as GODOT_SEEK_FORM_NONE; where the target lies and whether the clock can
    // reach it is ase-time's to judge from its own clock, and its verdict comes back with `seq`.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_GAME_HOUR, game_hour);
    (void)put_value(input, GODOT_KEY_AHEAD_H, ahead_h);
    (void)put_value(input, GODOT_KEY_DAY_HOUR, day_hour);
    (void)put_value(input, GODOT_KEY_SEQ, seq);
    return host_->submit(config_.seek_port.c_str(), config_.op_seek.c_str(), &input);
}

HostStatus GodotHostResourceManager::seek_date(double seq, double year, double month, double month_day, double hour) {
    // The request names no object: the clock is the one clock of this host. Whether the date exists is
    // ase-calendar's to check with its own constants, whether it lies behind the clock ase-time's to
    // judge; the verdict comes back with `seq` on the same channel as that of a seek.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_SEQ, seq);
    (void)put_value(input, GODOT_KEY_YEAR, year);
    (void)put_value(input, GODOT_KEY_MONTH, month);
    (void)put_value(input, GODOT_KEY_MONTH_DAY, month_day);
    (void)put_value(input, GODOT_KEY_HOUR, hour);
    return host_->submit(config_.date_port.c_str(), config_.op_seek_date.c_str(), &input);
}

HostStatus GodotHostResourceManager::shape(double height_scale_m, double base_height_m) {
    // The request names no object: the relief is the one relief of this world. Which values are valid
    // is ase-pl-flora's and ase-terrain's to check.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_HEIGHT_SCALE, height_scale_m);
    (void)put_value(input, GODOT_KEY_BASE_HEIGHT, base_height_m);
    return host_->submit(config_.elevation_port.c_str(), config_.op_shape.c_str(), &input);
}

HostStatus GodotHostResourceManager::view(double x, double y, double z, double aim_x, double aim_y,
                                          double aim_z) {
    // The request names no object: the camera is the one camera of this view.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    (void)put_value(input, GODOT_KEY_X, x);
    (void)put_value(input, GODOT_KEY_Y, y);
    (void)put_value(input, GODOT_KEY_Z, z);
    (void)put_value(input, GODOT_KEY_AIM_X, aim_x);
    (void)put_value(input, GODOT_KEY_AIM_Y, aim_y);
    (void)put_value(input, GODOT_KEY_AIM_Z, aim_z);
    return host_->submit(config_.view_port.c_str(), config_.op_view.c_str(), &input);
}

HostStatus GodotHostResourceManager::note(const char* text) {
    if (!host_) {
        return ase::kernel::HostStatusInvalidState;
    }
    return host_->note(GODOT_LOG_SOURCE, text);
}

// VIEWS

const VivariumPatchView* GodotHostResourceManager::get_patch(uint32_t object_id) const {
    const auto found = patches_.find(object_id);
    return found == patches_.end() ? nullptr : &found->second;
}

const VivariumPatchView* GodotHostResourceManager::get_patch_at(uint32_t place) const {
    const auto found = places_.find(place);
    return found == places_.end() ? nullptr : get_patch(found->second);
}

const std::unordered_map<uint32_t, double>& GodotHostResourceManager::get_caps() const {
    return caps_;
}

double GodotHostResourceManager::stage_due_h(const VivariumPatchView& patch, uint8_t position) const {
    if (stage_named(position, GODOT_KEY_STGE_GROW)) {
        return patch.grow_at_h;
    }
    if (stage_named(position, GODOT_KEY_STGE_MTUR)) {
        return patch.mtur_at_h;
    }
    return GODOT_HOUR_NONE;
}

bool GodotHostResourceManager::has_patch(uint32_t object_id) const {
    return patches_.find(object_id) != patches_.end();
}

uint32_t GodotHostResourceManager::patch_count() const {
    return static_cast<uint32_t>(patches_.size());
}

const VivariumClockView& GodotHostResourceManager::get_clock() const {
    return clock_;
}

const VivariumWeatherView& GodotHostResourceManager::get_weather() const {
    return weather_;
}

const VivariumSapView& GodotHostResourceManager::get_sap() const {
    return sap_;
}

const VivariumLooseView& GodotHostResourceManager::get_loose() const {
    return loose_;
}

double GodotHostResourceManager::loose_at(double x, double z) const {
    if (!loose_.known) {
        return std::nan("");
    }
    const int64_t column = std::llround((x - loose_.west_x) / loose_.cell_m);
    const int64_t row = std::llround((z - loose_.south_z) / loose_.cell_m);
    if (column < 0 || row < 0 || column >= loose_.columns || row >= loose_.rows) {
        return std::nan("");
    }
    return loose_.loose[static_cast<size_t>(row * loose_.columns + column)];
}

const std::vector<VivariumOrganView>& GodotHostResourceManager::get_organs(uint32_t organ_class) const {
    // An unknown class answers the empty set no_organs_: never an index past the store.
    return organ_class < GODOT_ORGAN_COUNT ? organs_[organ_class] : no_organs_;
}

const std::vector<VivariumInstanceView>& GodotHostResourceManager::get_instances(uint32_t organ_class) const {
    return organ_class < GODOT_DRAWN_COUNT ? instances_[organ_class] : no_instances_;
}

uint64_t GodotHostResourceManager::organ_generation() const {
    return organ_generation_;
}

const std::vector<VivariumBarkVertexView>& GodotHostResourceManager::get_bark() const {
    return bark_;
}

uint64_t GodotHostResourceManager::bark_generation() const {
    return bark_generation_;
}

const GodotKnotResourceManager& GodotHostResourceManager::knots() const {
    return *knots_;
}

const std::vector<VivariumTerrainView>& GodotHostResourceManager::get_terrain() const {
    return terrain_;
}

const std::vector<VivariumGroundCellView>& GodotHostResourceManager::get_ground_cells() const {
    return ground_cells_;
}

uint64_t GodotHostResourceManager::terrain_generation() const {
    return terrain_generation_;
}

const std::vector<VivariumStreamView>& GodotHostResourceManager::get_streams() const {
    return streams_;
}

uint64_t GodotHostResourceManager::stream_generation() const {
    return stream_generation_;
}

const std::vector<VivariumCurrentView>& GodotHostResourceManager::get_currents() const {
    return currents_;
}

uint64_t GodotHostResourceManager::current_generation() const {
    return current_generation_;
}

const std::vector<VivariumPitView>& GodotHostResourceManager::get_pits() const {
    return pits_;
}

uint64_t GodotHostResourceManager::pit_generation() const {
    return pit_generation_;
}

const std::vector<VivariumGroundwaterView>& GodotHostResourceManager::get_groundwater() const {
    return groundwater_;
}

uint64_t GodotHostResourceManager::groundwater_generation() const {
    return groundwater_generation_;
}

const std::vector<VivariumSaltView>& GodotHostResourceManager::get_salt() const {
    return salt_;
}

uint64_t GodotHostResourceManager::salt_generation() const {
    return salt_generation_;
}

const std::vector<VivariumLakeView>& GodotHostResourceManager::get_lakes() const {
    return water_kinds_.get_lakes();
}

uint64_t GodotHostResourceManager::lake_generation() const {
    return water_kinds_.lake_generation();
}

const std::vector<VivariumSeaView>& GodotHostResourceManager::get_seas() const {
    return water_kinds_.get_seas();
}

uint64_t GodotHostResourceManager::sea_generation() const {
    return water_kinds_.sea_generation();
}

const std::vector<VivariumGlacierView>& GodotHostResourceManager::get_glaciers() const {
    return water_kinds_.get_glaciers();
}

uint64_t GodotHostResourceManager::glacier_generation() const {
    return water_kinds_.glacier_generation();
}

const std::vector<VivariumTemplateCornerView>& GodotHostResourceManager::get_template_corners() const {
    return template_corners_;
}

const std::vector<VivariumTemplateTriangleView>& GodotHostResourceManager::get_template_triangles() const {
    return template_triangles_;
}

uint64_t GodotHostResourceManager::template_generation() const {
    return template_generation_;
}

const VivariumSkyView& GodotHostResourceManager::get_sky() const {
    return sky_;
}

const std::vector<VivariumSeedView>& GodotHostResourceManager::get_seeds() const {
    return seeds_;
}

const std::vector<VivariumRackView>& GodotHostResourceManager::get_racks() const {
    return racks_;
}

bool GodotHostResourceManager::rack_offered() const {
    // both rack ports of the configuration, or none: a rack laid where it cannot be read is no rack
    return host_ && !config_.toolrack_scene_port.empty() && !config_.toolrack_rack_port.empty() &&
           host_->has_port(config_.toolrack_scene_port.c_str()) && host_->has_port(config_.toolrack_rack_port.c_str());
}

const std::vector<VivariumSurvivalBodyView>& GodotHostResourceManager::get_survival_bodies() const {
    return survival_bodies_;
}

bool GodotHostResourceManager::survival_body_offered() const {
    return host_ && !config_.survival_body_port.empty() && host_->has_port(config_.survival_body_port.c_str());
}

const std::vector<VivariumSurvivalRestView>& GodotHostResourceManager::get_survival_rests() const {
    return survival_rests_;
}

bool GodotHostResourceManager::survival_rest_offered() const {
    return host_ && !config_.survival_rest_port.empty() && host_->has_port(config_.survival_rest_port.c_str());
}

const std::vector<VivariumSurvivalFoodView>& GodotHostResourceManager::get_survival_foods() const {
    return survival_foods_;
}

bool GodotHostResourceManager::survival_food_offered() const {
    return host_ && !config_.survival_food_port.empty() && host_->has_port(config_.survival_food_port.c_str());
}

const std::vector<VivariumSurvivalSmokeView>& GodotHostResourceManager::get_survival_smokes() const {
    return survival_smokes_;
}

bool GodotHostResourceManager::survival_smoke_offered() const {
    return host_ && !config_.survival_smoke_port.empty() && host_->has_port(config_.survival_smoke_port.c_str());
}

bool GodotHostResourceManager::camp_offered() const {
    return host_ && !config_.camp_place_port.empty() && !config_.camp_hull_port.empty() &&
           !config_.camp_part_port.empty() && host_->has_port(config_.camp_place_port.c_str()) &&
           host_->has_port(config_.camp_hull_port.c_str()) && host_->has_port(config_.camp_part_port.c_str());
}

const std::vector<VivariumCampBodyView>& GodotHostResourceManager::get_camp_bodies() const {
    return camp_bodies_;
}

bool GodotHostResourceManager::camp_body_offered() const {
    return host_ && !config_.camp_body_port.empty() && host_->has_port(config_.camp_body_port.c_str());
}

const std::vector<VivariumCampRefuelView>& GodotHostResourceManager::get_camp_refuels() const {
    return camp_refuels_;
}

bool GodotHostResourceManager::camp_refuel_offered() const {
    return host_ && !config_.camp_refuel_port.empty() && host_->has_port(config_.camp_refuel_port.c_str());
}

const std::vector<VivariumSurvivalFireView>& GodotHostResourceManager::get_survival_fires() const {
    return survival_fires_;
}

bool GodotHostResourceManager::survival_fire_offered() const {
    return host_ && !config_.survival_fire_port.empty() && host_->has_port(config_.survival_fire_port.c_str());
}

bool GodotHostResourceManager::survival_left_offered() const {
    return host_ && !config_.survival_left_port.empty() && host_->has_port(config_.survival_left_port.c_str());
}

const std::vector<VivariumFigureView>& GodotHostResourceManager::get_figures() const {
    return figures_;
}

const VivariumFigureCameraView& GodotHostResourceManager::get_figure_camera() const {
    return figure_camera_;
}

const std::vector<VivariumFigureGearView>& GodotHostResourceManager::get_figure_gear() const {
    return figure_gear_;
}

const VivariumFigureCodeView& GodotHostResourceManager::get_figure_code() const {
    return figure_code_;
}

const std::vector<VivariumFigureBumpView>& GodotHostResourceManager::get_figure_bumps() const {
    return figure_bumps_;
}

bool GodotHostResourceManager::figure_offered() const {
    // every figure port of the configuration, or none: a half offered plugin is no figure
    return host_ && !config_.figure_body_port.empty() && host_->has_port(config_.figure_body_port.c_str()) &&
           host_->has_port(config_.figure_camera_port.c_str()) && host_->has_port(config_.figure_spawn_port.c_str()) &&
           host_->has_port(config_.figure_stick_port.c_str()) && host_->has_port(config_.figure_orbit_port.c_str()) &&
           host_->has_port(config_.figure_switch_port.c_str()) && host_->has_port(config_.figure_release_port.c_str()) &&
           host_->has_port(config_.figure_seize_port.c_str()) && host_->has_port(config_.figure_hold_port.c_str()) &&
           host_->has_port(config_.figure_drop_port.c_str()) && host_->has_port(config_.figure_don_port.c_str()) &&
           host_->has_port(config_.figure_fill_port.c_str()) &&
           host_->has_port(config_.figure_drain_port.c_str()) && host_->has_port(config_.figure_gear_port.c_str()) &&
           host_->has_port(config_.figure_rig_port.c_str()) && host_->has_port(config_.figure_turn_port.c_str()) &&
           host_->has_port(config_.figure_code_port.c_str()) && host_->has_port(config_.figure_area_port.c_str()) &&
           host_->has_port(config_.figure_boundary_port.c_str());
}

// LOG LINES

void GodotHostResourceManager::store_log_line(const char* line, uint32_t len, int level) {
    if (line == nullptr) {
        return;
    }
    // The node prints line by line; the trailing line break of the formatter would double it.
    while (len > 0u && (line[len - 1u] == '\n' || line[len - 1u] == '\r')) {
        len -= 1u;
    }
#if defined(__ANDROID__)
    // PLAN 02.1: the native logcat sink. Written now, from the thread that wrote the line - a line
    // queued for the next frame dies with a process that never reaches that frame. The spdlog
    // levels 0..5 (trace .. critical) map onto the contiguous logcat priorities VERBOSE .. FATAL
    // of the NDK ABI. Nothing here touches a Godot object.
    const std::string text(line, len);
    const int clamped = level < 0 ? 0 : (level > GODOT_LOG_LEVEL_MAX ? GODOT_LOG_LEVEL_MAX : level);
    __android_log_write(ANDROID_LOG_VERBOSE + clamped, GODOT_LOG_SOURCE, text.c_str());
#else
    (void)level;  // the line carries its level tag already
    const std::lock_guard<std::mutex> lock(log_mutex_);
    log_lines_.emplace_back(line, len);
#endif
}

uint32_t GodotHostResourceManager::remove_log_lines(std::vector<std::string>& out) {
    const std::lock_guard<std::mutex> lock(log_mutex_);
    const auto moved = static_cast<uint32_t>(log_lines_.size());
    for (std::string& line : log_lines_) {
        out.push_back(std::move(line));
    }
    log_lines_.clear();
    return moved;
}

// STATE AND MEASUREMENT

bool GodotHostResourceManager::running() const {
    return host_ && host_->state() == ase::kernel::HostStateRunning;
}

bool GodotHostResourceManager::has_host() const {
    return host_ != nullptr;
}

const char* GodotHostResourceManager::failure_step() const {
    return failure_step_;
}

HostStatus GodotHostResourceManager::failure_status() const {
    return failure_status_;
}

const VivariumBundleInfo& GodotHostResourceManager::get_bundle() const {
    return bundle_;
}

double GodotHostResourceManager::simulation_seconds() const {
    return elapsed_in_s_ - backlog_s_;
}

double GodotHostResourceManager::stage_seconds() const {
    // Only a running host is asked, as in schedule_runs: a stopped one would log its refusal on every frame.
    double ticked_s = 0.0;
    if (!running() || host_->ticked_seconds(&ticked_s) != ase::kernel::HostStatusOk) {
        return 0.0;
    }
    return ticked_s;
}

uint64_t GodotHostResourceManager::tick_count() const {
    return tick_count_;
}

uint32_t GodotHostResourceManager::last_steps() const {
    return last_steps_;
}

double GodotHostResourceManager::backlog_seconds() const {
    return backlog_s_;
}

uint32_t GodotHostResourceManager::system_count() const {
    return host_ ? host_->system_count() : 0u;
}

uint32_t GodotHostResourceManager::unit_count() const {
    return host_ ? host_->unit_count() : 0u;
}

bool GodotHostResourceManager::read_pss_kib(uint64_t& out) const {
    // ONE READER OF THE PSS: the cycle line of 03.6 (godot_cycle_measure.cpp) reads the same number
    // headless on Linux, where this manager does not exist. Qualified, because this member carries
    // the same name.
    return ::ase::adp::godot::read_pss_kib(out);
}

bool GodotHostResourceManager::measure_begin(CycleMeasure& measure) const {
    return host_ != nullptr && cycle_measure_begin(measure, *host_);
}

bool GodotHostResourceManager::measure_cycle(CycleMeasure& measure, std::string& line) {
    if (host_ == nullptr) {
        line = "cycle=" + std::to_string(measure.cycle + 1u) + " refused=no_host";
        return false;
    }
    return cycle_measure_line(measure, *host_, line);
}

HostStatus GodotHostResourceManager::schedule_runs(const char* schedule, uint64_t* runs) const {
    if (runs != nullptr) {
        *runs = 0u;
    }
    // Only a running host is asked: a stopped one would log its refusal, and the view measures on
    // every focus change - after a failed boot that would be one warning per window switch.
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    return host_->schedule_runs(schedule, runs);
}

const char* GodotHostResourceManager::status_name(HostStatus status) {
    if (status == ase::kernel::HostStatusOk) {
        return "Ok";
    }
    if (status == ase::kernel::HostStatusInvalidState) {
        return "InvalidState";
    }
    if (status == ase::kernel::HostStatusInvalidArgument) {
        return "InvalidArgument";
    }
    if (status == ase::kernel::HostStatusNotFound) {
        return "NotFound";
    }
    if (status == ase::kernel::HostStatusCapacity) {
        return "Capacity";
    }
    if (status == ase::kernel::HostStatusAbiMismatch) {
        return "AbiMismatch";
    }
    return "Unknown";
}

// BULK CLEANUP

void GodotHostResourceManager::clear_all() {
    // Stop BEFORE clearing: the host runs the binding teardown (App shutdown, ports released,
    // App destroyed, dlclose in reverse load order) and releases its logger last. Lines it still
    // writes stay in log_lines_ for the node to print.
    if (host_) {
        (void)host_->stop();
        host_.reset();
    }
    patches_.clear();
    places_.clear();
    caps_.clear();
    clock_ = VivariumClockView{};
    weather_ = VivariumWeatherView{};
    records_.clear();
    for (uint32_t c = 0u; c < GODOT_ORGAN_COUNT; ++c) {
        organs_[c].clear();
    }
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        instances_[c].clear();
    }
    organ_records_.clear();
    organ_runs_ = 0u;
    organ_reads_due_ = 0u;
    organ_generation_ += 1u;  // the view drops the geometry of the old host
    bark_.clear();
    bark_runs_ = 0u;
    bark_passes_ = 0u;
    bark_owed_ = false;
    bark_generation_ += 1u;   // and its bark tubes
    knots_->clear_all();      // and the knots, feet and cut faces of those tubes
    terrain_.clear();
    ground_cells_.clear();
    terrain_runs_ = 0u;
    terrain_reads_due_ = 0u;
    terrain_generation_ += 1u;  // and the ground of the old host
    streams_.clear();
    stream_generation_ += 1u;   // and the water of its brooks
    currents_.clear();
    current_generation_ += 1u;  // and its river current
    pits_.clear();
    pit_generation_ += 1u;      // and the water of its pits
    groundwater_.clear();
    groundwater_generation_ += 1u;  // and its groundwater
    salt_.clear();
    salt_generation_ += 1u;  // and its salt
    water_kinds_.clear();    // and its lakes, its sea and its glacier
    template_corners_.clear();
    template_triangles_.clear();
    template_generation_ += 1u;  // and its templates
    sky_ = VivariumSkyView{};   // and its light
    seeds_.clear();             // and its seeds
    racks_.clear();             // and its tool racks
    survival_bodies_.clear();   // and its figures' bodies
    survival_rests_.clear();    // and their rest
    survival_foods_.clear();    // and the spoil of their food
    survival_smokes_.clear();   // and the soot in the air of their cells
    survival_fires_.clear();    // and its fire cells
    camp_bodies_.clear();       // and the bodies of its camp
    camp_refuels_.clear();      // and the keepers laying wood
    figures_.clear();           // and its figures
    figure_camera_ = VivariumFigureCameraView{};  // and the camera following one
    figure_gear_.clear();       // and what the figures carry
    figure_code_ = VivariumFigureCodeView{};  // and what their codes mean
    figure_bumps_.clear();      // and the bumps against the chunk edge
    bundle_ = VivariumBundleInfo{};
    manifests_.clear();
}

}  // namespace ase::adp::godot

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
 * @modified    2026-11-14
 * @version     00.00.04.00021 [seed]
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
#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/adp/godot/godot_organ_records.hpp>
#include <ase/adp/godot/godot_terrain_samples.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/math/math.hpp>
#include <ase/utils/strops.hpp>

#include <toml++/toml.hpp>

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
constexpr const char* kFigureKeyForward = "forward";
constexpr const char* kFigureKeyStrafe = "strafe";
constexpr const char* kFigureKeySprint = "sprint";
constexpr const char* kFigureKeyJump = "jump";
constexpr const char* kFigureKeyYawDelta = "yaw_delta";
constexpr const char* kFigureKeyPitchDelta = "pitch_delta";
constexpr const char* kFigureKeyZoomDelta = "zoom_delta";  // turn request: change of the orbit distance, m (AVATAR_HOST_KEY_ZOOM_DELTA)
constexpr const char* kFigureKeyItem = "item";      // hold request: catalog id of the thing (AVATAR_HOST_KEY_ITEM)
constexpr const char* kFigureKeyBearer = "bearer";  // gear record: the figure that carries the piece (AVATAR_HOST_KEY_BEARER)
constexpr const char* kFigureKeyEyeHeight = "eye_height";  // rig report: eyes above the soles, m (AVATAR_HOST_KEY_EYE_HEIGHT)
constexpr const char* kFigureKeyHeight = "height";         // rig report: body above the soles, m (AVATAR_HOST_KEY_HEIGHT)
constexpr const char* kFigureKeySlot = "slot";      // gear record: name_id of the socket (AVATAR_HOST_KEY_SLOT)
constexpr const char* kFigureKeyKind = "kind";      // gear record: catalog id of the thing (AVATAR_HOST_KEY_KIND)
constexpr const char* kFigureKeyGaitStnd = "gait_stnd";    // code record: gait of a standing figure (AVATAR_HOST_KEY_GAIT_STND)
constexpr const char* kFigureKeyGaitWalk = "gait_walk";    // code record: gait of a walking figure (AVATAR_HOST_KEY_GAIT_WALK)
constexpr const char* kFigureKeyGaitRun = "gait_run";      // code record: gait of a running figure (AVATAR_HOST_KEY_GAIT_RUN)
constexpr const char* kFigureKeySoulAipc = "soul_aipc";    // code record: soul while the AI leads (AVATAR_HOST_KEY_SOUL_AIPC)
constexpr const char* kFigureKeySoulSspc = "soul_sspc";    // code record: soul while its player leads (AVATAR_HOST_KEY_SOUL_SSPC)
constexpr const char* kFigureKeyCamFixed = "cam_fixed";    // code record: camera in the fixed view (AVATAR_HOST_KEY_CAM_FIXED)
constexpr const char* kFigureKeyCamPlayer = "cam_player";  // code record: camera following the figure (AVATAR_HOST_KEY_CAM_PLAYER)
constexpr uint32_t kFigureCodeObject = 0u;  // object id of the one code record; the codes belong to no figure (AVATAR_HOST_CODE_OBJECT)
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
 * that is no whole id: the gear port writes bearer, slot and kind of every piece (AvatarHostGearExptSystem).
 */
bool figure_gear_of(const KernelHostRecord& record, VivariumFigureGearView& out) {
    out = VivariumFigureGearView{};
    out.object_id = record.object_id;
    return figure_id_of(record_value(record, kFigureKeyBearer), out.bearer) &&
           figure_id_of(record_value(record, kFigureKeySlot), out.slot) &&
           figure_id_of(record_value(record, kFigureKeyKind), out.kind);
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
                             record_value(record, kFigureKeyAimZ), record_value(record, kFigureKeyAimY)};
    for (const double value : values) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
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
 * every brook it names (ase-pl-flora, FloraHostStrmExptSystem). The groundwater beside the bed (B0286) is read
 * from its own two keys and fails no record: until ase-hydro publishes it (N0484) record_value gives NaN, the
 * fields keep it, and the view's bridge logs the gap on its edge (VivariumView::follow_scene).
 */
bool stream_view_of(const KernelHostRecord& record, VivariumStreamView& out) {
    constexpr const char* KEY_GROUNDWATER_LEVEL = "groundwater_level_m";  // HYD_STREAM_GNDW_LEVEL, world metres
    constexpr const char* KEY_GROUNDWATER_REACH = "groundwater_reach_m";  // HYD_STREAM_GNDW_REACH, metres
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
 * The ports and operations every host of the Vivarium needs: patch, forecast, clock, date, scene, ground, brook,
 * terrain, ground cell, the six organ ports, the five instance ports, the sky port, the stars port, the seed port, the
 * tile port, the sway port, the stream port, the level, the flatten, the lower, the raise and the elevation port and the two
 * template ports, irrigate, prune, level, flatten, lower, raise, shape and lay - every port boot()
 * asks the set for. An
 * empty one would surface later as a NotFound of the first call, or as the port step of a boot that a
 * configuration check could have refused before a host existed.
 */
bool ports_named(const VivariumStartConfig& config) {
    bool named = !config.patch_port.empty() && !config.forecast_port.empty() && !config.clock_port.empty() &&
                 !config.date_port.empty() &&
                 !config.scene_port.empty() &&
                 !config.ground_port.empty() && !config.brook_port.empty() && !config.terrain_port.empty() &&
                 !config.ground_cell_port.empty() && !config.sky_port.empty() && !config.stars_port.empty() &&
                 !config.seed_port.empty() &&
                 !config.tile_port.empty() && !config.sway_port.empty() && !config.bark_port.empty() &&
                 !config.stream_port.empty() &&
                 !config.level_port.empty() &&
                 !config.flatten_port.empty() && !config.lower_port.empty() && !config.raise_port.empty() &&
                 !config.op_lower.empty() && !config.op_raise.empty() &&
                 !config.elevation_port.empty() && !config.op_shape.empty() &&
                 !config.template_corner_port.empty() && !config.template_triangle_port.empty() &&
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

/** The lay request of one patch: place, species id, seed, ground and where it lies in the world. */
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
    if (snapshot_all(host, config.patch_port, state.records, written) != ase::kernel::HostStatusOk) {
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
                : "place=" + std::to_string((*expect)[GODOT_SELFTEST_KEY_PLACE].value_or(int64_t{-1}));
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
        const std::string expected = (*lay)[GODOT_SELFTEST_KEY_STATUS].value_or(std::string());
        if (place < 0 || place > static_cast<int64_t>(UINT32_MAX) || seed < 0 ||
            seed > static_cast<int64_t>(UINT32_MAX) || patch.species.empty() || patch.soil >= GODOT_SOIL_COUNT ||
            !std::isfinite(patch.x) || !std::isfinite(patch.z) || !(patch.half_m > 0.0) || expected.empty()) {
            report_broken(label, GODOT_SELFTEST_KEY_LAY,
                          "needs place, species, seed, a known soil, x, z, a positive half_m and status", result,
                          report);
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

    report_broken(label, GODOT_SELFTEST_KEY_DO,
                  "an action names none of frames, remember, irrigate, wait, catch_up, expect, records, lay, view",
                  result, report);
    return false;
}

}  // anonymous namespace

// LIFETIME

GodotHostResourceManager::GodotHostResourceManager()
    : failure_step_(GODOT_STEP_NONE), failure_status_(ase::kernel::HostStatusOk) {}

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
                   host_->has_port(config_.clock_port.c_str()) &&
                   host_->has_port(config_.date_port.c_str()) &&
                   host_->has_port(config_.scene_port.c_str()) && host_->has_port(config_.ground_port.c_str()) &&
                   host_->has_port(config_.brook_port.c_str()) && host_->has_port(config_.terrain_port.c_str()) &&
                   host_->has_port(config_.ground_cell_port.c_str()) && host_->has_port(config_.sky_port.c_str()) &&
                   host_->has_port(config_.stars_port.c_str()) &&
                   host_->has_port(config_.seed_port.c_str()) && host_->has_port(config_.tile_port.c_str()) &&
                   host_->has_port(config_.sway_port.c_str()) && host_->has_port(config_.bark_port.c_str()) &&
                   host_->has_port(config_.stream_port.c_str()) &&
                   host_->has_port(config_.level_port.c_str()) && host_->has_port(config_.flatten_port.c_str()) &&
                   host_->has_port(config_.lower_port.c_str()) && host_->has_port(config_.raise_port.c_str()) &&
                   host_->has_port(config_.elevation_port.c_str()) &&
                   host_->has_port(config_.template_corner_port.c_str()) &&
                   host_->has_port(config_.template_triangle_port.c_str()) &&
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
    clock_ = VivariumClockView{};
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
    terrain_.clear();
    ground_cells_.clear();
    terrain_generation_ += 1u;
    streams_.clear();
    stream_generation_ += 1u;
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
    HostStatus status = snapshot_all(*host_, config_.patch_port, records_, written);
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
        view.soil_mm = record_value(record, GODOT_KEY_SOIL_MM);
        view.soil_rel = record_value(record, GODOT_KEY_SOIL_REL);
        view.capacity_mm = record_value(record, GODOT_KEY_CAPACITY_MM);
        // the stage forecasts stand on the forecast port, read below; until its record for this patch
        // is read they are not measured
        view.grow_at_h = std::nan("");
        view.mtur_at_h = std::nan("");
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
        view.stage = code_of(stage, GODOT_STAGE_COUNT);
        view.condition = code_of(condition, GODOT_COND_COUNT);
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
        const double date_parts[] = {clock.year,      clock.epoch,          clock.cycle,          clock.month,
                                     clock.month_day, clock.days_per_month, clock.months_per_year};
        std::size_t finite_parts = 0u;
        for (const double part : date_parts) {
            finite_parts += std::isfinite(part) ? 1u : 0u;
        }
        if (finite_parts != std::size(date_parts)) {
            return refuse_record(*host_, config_.date_port, date_records[0].object_id,
                                 "SURA date incomplete: year, epoch, cycle, month, day and the calendar lengths "
                                 "come together");
        }
        if (!std::isfinite(clock.period)) {
            return refuse_record(*host_, config_.clock_port, clock_records[0].object_id,
                                 "SURA date without its day period");
        }
    }
    clock.date_known = written == 1u;
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
            const double day_parts[] = {clock.sunrise_h,     clock.sunset_h,     clock.daylight_h,
                                        clock.season,        clock.day_share,    clock.sunrise_share,
                                        clock.sunset_share,  clock.year_share,   clock.year_day};
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
    const HostStatus terrain = read_terrain();
    if (terrain != ase::kernel::HostStatusOk) {
        return terrain;
    }
    const HostStatus streams = read_streams();
    if (streams != ase::kernel::HostStatusOk) {
        return streams;
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

HostStatus GodotHostResourceManager::read_templates() {
    // THE TEMPLATES ARE BUILT ONCE (usecase:101011 tessellate the organ templates). ase-render-data builds the fine
    // and the coarse cylinder and the unit leaf in the Initialization of its host and keeps their rows; the
    // plugin lays them on its two template ports in every Production run. So both ports are read with every
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

HostStatus GodotHostResourceManager::read_figures() {
    // THE FIGURES ARE READ EVERY SNAPSHOT (07.8): ase-player moves a figure in every frame and ase-pl-avatar lays the
    // body port anew in every Production run (AvatarHostBodyExptSystem), the camera port after it
    // (AvatarHostCamExptSystem). A host without the plugin offers neither port: no figure, nothing broken.
    figures_.clear();
    figure_camera_ = VivariumFigureCameraView{};
    figure_gear_.clear();
    figure_code_ = VivariumFigureCodeView{};
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
                                 "gear piece without bearer, slot or kind as whole id");
        }
        figure_gear_.push_back(piece);
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

HostStatus GodotHostResourceManager::measure_figure(uint32_t figure, double eye_height, double height) {
    // The scene measured the skeleton it built (07.6): eye height and height go to the plugin as they are;
    // their range is ase-pl-avatar's to check, the face landing of ase-camera reads the eye height from there.
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

bool GodotHostResourceManager::has_patch(uint32_t object_id) const {
    return patches_.find(object_id) != patches_.end();
}

uint32_t GodotHostResourceManager::patch_count() const {
    return static_cast<uint32_t>(patches_.size());
}

const VivariumClockView& GodotHostResourceManager::get_clock() const {
    return clock_;
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

bool GodotHostResourceManager::figure_offered() const {
    // every figure port of the configuration, or none: a half offered plugin is no figure
    return host_ && !config_.figure_body_port.empty() && host_->has_port(config_.figure_body_port.c_str()) &&
           host_->has_port(config_.figure_camera_port.c_str()) && host_->has_port(config_.figure_spawn_port.c_str()) &&
           host_->has_port(config_.figure_stick_port.c_str()) && host_->has_port(config_.figure_orbit_port.c_str()) &&
           host_->has_port(config_.figure_switch_port.c_str()) && host_->has_port(config_.figure_release_port.c_str()) &&
           host_->has_port(config_.figure_seize_port.c_str()) && host_->has_port(config_.figure_hold_port.c_str()) &&
           host_->has_port(config_.figure_drop_port.c_str()) && host_->has_port(config_.figure_gear_port.c_str()) &&
           host_->has_port(config_.figure_rig_port.c_str()) && host_->has_port(config_.figure_turn_port.c_str()) &&
           host_->has_port(config_.figure_code_port.c_str());
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
    clock_ = VivariumClockView{};
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
    terrain_.clear();
    ground_cells_.clear();
    terrain_runs_ = 0u;
    terrain_reads_due_ = 0u;
    terrain_generation_ += 1u;  // and the ground of the old host
    streams_.clear();
    stream_generation_ += 1u;   // and the water of its brooks
    template_corners_.clear();
    template_triangles_.clear();
    template_generation_ += 1u;  // and its templates
    sky_ = VivariumSkyView{};   // and its light
    seeds_.clear();             // and its seeds
    racks_.clear();             // and its tool racks
    figures_.clear();           // and its figures
    figure_camera_ = VivariumFigureCameraView{};  // and the camera following one
    figure_gear_.clear();       // and what the figures carry
    figure_code_ = VivariumFigureCodeView{};  // and what their codes mean
    bundle_ = VivariumBundleInfo{};
    manifests_.clear();
}

}  // namespace ase::adp::godot

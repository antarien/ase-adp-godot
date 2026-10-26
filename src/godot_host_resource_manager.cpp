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
 * @actor       Entity Component System (ECS)
 * @created     2026-10-05
 * @modified    2026-10-25
 * @version     00.00.04.00007 [seed]
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
 *   advance() hands the host the real time the frame took, uncapped. The host ticks it in
 *   ordinary steps of its scheduler's frame, at most catch_up_steps of them per call, and keeps
 *   the rest in its backlog - a return from the background is caught up over a few frames, never
 *   in one giant step and never dropped.
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
#include <ase/adp/godot/types.hpp>
#include <ase/fileio/text_reader.hpp>
#include <ase/math/math.hpp>
#include <ase/utils/strops.hpp>

#include <toml++/toml.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
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

/** The value stored under `key` in a snapshot record; NaN when the record does not carry it. */
double record_value(const KernelHostRecord& record, const char* key) {
    for (uint32_t i = 0u; i < record.value_count && i < ase::kernel::HostValuesMax; ++i) {
        if (ase::utils::str_equal(record.values[i].key, key, ase::kernel::HostKeyBytes)) {
            return record.values[i].value;
        }
    }
    return std::nan("");
}

/**
 * True for a finite, whole, non-negative value. The port carries places and codes as small whole
 * numbers, which a float holds exactly - ase-math has the float floor.
 */
bool whole(double value) {
    return std::isfinite(value) && value >= 0.0 &&
           ase::math::floor(static_cast<float>(value)) == static_cast<float>(value);
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
 * The bark of a segment record into its view (usecase:100395 age the bark). false for a record without
 * GODOT_KEY_BARK or with a share outside GODOT_BARK_WHITE..GODOT_BARK_BASE: the segment port writes it for
 * every internode as its twelfth value (FloraHostSegmExptSystem), so a gap is a broken port - never an
 * internode drawn in a bark nobody computed.
 */
bool segment_bark_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double bark = record_value(record, GODOT_KEY_BARK);
    if (!std::isfinite(bark) || bark < GODOT_BARK_WHITE || bark > GODOT_BARK_BASE) {
        return false;
    }
    out.bark = bark;
    return true;
}

/**
 * The bark tiling of a tile record into the view of its internode (03.11 step 2, the material data of a
 * section). false for a record without one of the three keys or with a count that is no positive finite
 * number: the tile port writes all three for every internode it names (FloraHostTileExptSystem), so a gap is
 * a broken port - never a bark stretched by a tiling nobody computed.
 */
bool segment_tile_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double around = record_value(record, GODOT_KEY_TILE_AROUND);
    const double along = record_value(record, GODOT_KEY_TILE_ALONG);
    const double base = record_value(record, GODOT_KEY_TILE_BASE);
    if (!std::isfinite(around) || !std::isfinite(along) || !std::isfinite(base) || around <= 0.0 ||
        along <= 0.0 || base <= 0.0) {
        return false;
    }
    out.tile_around = around;
    out.tile_along = along;
    out.tile_base = base;
    return true;
}

/**
 * One organ record as a view. false for a record that misses a key or carries a value that is no
 * finite number: the organ port writes all eleven keys of every organ (ase-pl-flora, the organ export
 * systems), so a gap is a broken port - never an organ drawn at the origin.
 */
bool organ_view_of(const KernelHostRecord& record, VivariumOrganView& out) {
    out = VivariumOrganView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    out.order = record_value(record, GODOT_KEY_ORDER);
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    out.qx = record_value(record, GODOT_KEY_QX);
    out.qy = record_value(record, GODOT_KEY_QY);
    out.qz = record_value(record, GODOT_KEY_QZ);
    out.qw = record_value(record, GODOT_KEY_QW);
    out.length = record_value(record, GODOT_KEY_LENGTH);
    out.radius = record_value(record, GODOT_KEY_RADIUS);
    if (!whole(body) || body > static_cast<double>(UINT32_MAX) || !std::isfinite(out.order) ||
        !std::isfinite(out.x) || !std::isfinite(out.y) || !std::isfinite(out.z) || !std::isfinite(out.qx) ||
        !std::isfinite(out.qy) || !std::isfinite(out.qz) || !std::isfinite(out.qw) ||
        !std::isfinite(out.length) || !std::isfinite(out.radius)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    return true;
}

/**
 * THE KEYS OF AN INSTANCE RECORD, a fixed mapping of this file, in the row order of MultiMesh::set_buffer
 * and of VivariumInstanceView::rows.
 */
constexpr const char* kInstanceKeys[GODOT_INSTANCE_VALUES] = {
    GODOT_KEY_M00, GODOT_KEY_M01, GODOT_KEY_M02, GODOT_KEY_M03, GODOT_KEY_M10, GODOT_KEY_M11,
    GODOT_KEY_M12, GODOT_KEY_M13, GODOT_KEY_M20, GODOT_KEY_M21, GODOT_KEY_M22, GODOT_KEY_M23};

/**
 * One instance record as a view. false for a record that misses a key or carries a value that is no
 * finite number: the instance port writes all twelve keys of every organ (ase-pl-flora, the instance
 * export systems), so a gap is a broken port - never an instance drawn collapsed at the origin.
 */
bool instance_view_of(const KernelHostRecord& record, VivariumInstanceView& out) {
    out = VivariumInstanceView{};
    out.object_id = record.object_id;
    for (uint32_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
        const double value = record_value(record, kInstanceKeys[v]);
        if (!std::isfinite(value)) {
            return false;
        }
        out.rows[v] = static_cast<float>(value);
    }
    return true;
}

/**
 * One terrain record as a view. false for a record that misses a key or carries a value that is no
 * finite number: the terrain port writes all nine keys of every sample - place and height of
 * ase-terrain, normal and texture coordinates of ase-render-data (ase-pl-flora,
 * FloraHostTernExptSystem) - so a gap is a broken port, never ground drawn at height zero or lit
 * with a normal the view chose.
 */
bool terrain_view_of(const KernelHostRecord& record, VivariumTerrainView& out) {
    out = VivariumTerrainView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    out.x = record_value(record, GODOT_KEY_X);
    out.z = record_value(record, GODOT_KEY_Z);
    out.height = record_value(record, GODOT_KEY_HEIGHT);
    out.nx = record_value(record, GODOT_KEY_NX);
    out.ny = record_value(record, GODOT_KEY_NY);
    out.nz = record_value(record, GODOT_KEY_NZ);
    out.u = record_value(record, GODOT_KEY_U);
    out.v = record_value(record, GODOT_KEY_V);
    if (!whole(body) || body > static_cast<double>(UINT32_MAX) || !std::isfinite(out.x) ||
        !std::isfinite(out.z) || !std::isfinite(out.height) || !std::isfinite(out.nx) ||
        !std::isfinite(out.ny) || !std::isfinite(out.nz) || !std::isfinite(out.u) || !std::isfinite(out.v)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    return true;
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
 * The four records of the sky port as one view. false for a set without one of the four records, a
 * record that misses a key, or a value that is no finite number: ase-pl-sky lays all four records with
 * all their keys in one publication (SkyHostScneExptSystem), so a gap is a broken port - never a scene
 * lit from the origin or in black.
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
    const double values[] = {out.sun_x,        out.sun_y,        out.sun_z,         out.sun_r,
                             out.sun_g,        out.sun_b,        out.sun_intensity, out.ambient_r,
                             out.ambient_g,    out.ambient_b,    out.ambient_intensity,
                             out.background_r, out.background_g, out.background_b,  out.fog_r,
                             out.fog_g,        out.fog_b,        out.fog_near,      out.fog_far};
    for (const double value : values) {
        if (!std::isfinite(value)) {
            out = VivariumSkyView{};
            return false;
        }
    }
    out.known = true;
    return true;
}

/**
 * The ports and operations every host of the Vivarium needs: patch, clock, scene, ground, brook,
 * terrain, ground cell, the six organ ports, the five instance ports, the sky port, the seed port, the
 * tile port, the level and the flatten port and the two template ports, irrigate, prune, level, flatten
 * and lay - every port boot()
 * asks the set for. An
 * empty one would surface later as a NotFound of the first call, or as the port step of a boot that a
 * configuration check could have refused before a host existed.
 */
bool ports_named(const VivariumStartConfig& config) {
    bool named = !config.patch_port.empty() && !config.clock_port.empty() && !config.scene_port.empty() &&
                 !config.ground_port.empty() && !config.brook_port.empty() && !config.terrain_port.empty() &&
                 !config.ground_cell_port.empty() && !config.sky_port.empty() && !config.seed_port.empty() &&
                 !config.tile_port.empty() && !config.level_port.empty() && !config.flatten_port.empty() &&
                 !config.template_corner_port.empty() && !config.template_triangle_port.empty() &&
                 !config.op_irrigate.empty() &&
                 !config.op_prune.empty() && !config.op_level.empty() && !config.op_flatten.empty() &&
                 !config.op_lay.empty();
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

/** One frame of real time - exactly one ordinary tick. false when the host refuses the frame. */
bool frame(KernelEmbeddedHost& host, const VivariumSelfTestState& state) {
    uint32_t steps = 0u;
    double backlog = 0.0;
    return host.advance(state.frame_s, 1u, &steps, &backlog) == ase::kernel::HostStatusOk && steps == 1u;
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
            if (!frame(host, state)) {
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
            if (!frame(host, state)) {
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
        const int64_t max_steps = (*catch_up)[GODOT_SELFTEST_KEY_MAX_STEPS].value_or(int64_t{0});
        const int64_t expected = (*catch_up)[GODOT_SELFTEST_KEY_STEPS].value_or(int64_t{-1});
        if (!(seconds > 0.0) || max_steps <= 0 || max_steps > static_cast<int64_t>(UINT32_MAX) || expected < 0) {
            report_broken(label, GODOT_SELFTEST_KEY_CATCH_UP, "needs seconds, max_steps and steps", result, report);
            return false;
        }
        uint32_t steps = 0u;
        double backlog = 0.0;
        if (host.advance(seconds, static_cast<uint32_t>(max_steps), &steps, &backlog) != ase::kernel::HostStatusOk) {
            report_broken(label, GODOT_STEP_ADVANCE, "the catch-up was refused", result, report);
            return false;
        }
        int64_t total = steps;
        bool bounded = steps <= static_cast<uint32_t>(max_steps);
        // the backlog drains over later calls, each a bounded number of ordinary ticks
        for (int64_t call = 0; backlog > 0.0 && call <= expected; ++call) {
            if (host.advance(0.0, static_cast<uint32_t>(max_steps), &steps, &backlog) != ase::kernel::HostStatusOk) {
                report_broken(label, GODOT_STEP_ADVANCE, "the catch-up was refused", result, report);
                return false;
            }
            bounded = bounded && steps <= static_cast<uint32_t>(max_steps);
            total += steps;
        }
        report_check(check, "catch_up seconds=" + decimal(seconds) + " max_steps=" + std::to_string(max_steps),
                     std::to_string(total) + (bounded && backlog == 0.0 ? "" : " unbounded"),
                     std::to_string(expected), total == expected && bounded && backlog == 0.0, result, report);
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
            if (!frame(host, state)) {
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

    report_broken(label, GODOT_SELFTEST_KEY_DO,
                  "an action names none of frames, remember, irrigate, wait, catch_up, expect, records, lay",
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
    tick_count_ = 0u;
    last_steps_ = 0u;

    // A configuration the host could never run is refused before a host exists: no catch-up
    // steps would turn every frame into a refused advance, an empty name into a NotFound later.
    if (config_.read_stage == nullptr || config_.files_dir.empty() || config_.files_dir[0] != '/' ||
        !ports_named(config_) || !patches_layable(config_.patches) ||
        !std::isfinite(config_.irrigate_amount_mm) || !(config_.irrigate_amount_mm > 0.0) ||
        config_.catch_up_steps == 0u) {
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

    bool offered = host_->has_port(config_.patch_port.c_str()) && host_->has_port(config_.clock_port.c_str()) &&
                   host_->has_port(config_.scene_port.c_str()) && host_->has_port(config_.ground_port.c_str()) &&
                   host_->has_port(config_.brook_port.c_str()) && host_->has_port(config_.terrain_port.c_str()) &&
                   host_->has_port(config_.ground_cell_port.c_str()) && host_->has_port(config_.sky_port.c_str()) &&
                   host_->has_port(config_.seed_port.c_str()) && host_->has_port(config_.tile_port.c_str()) &&
                   host_->has_port(config_.level_port.c_str()) && host_->has_port(config_.flatten_port.c_str()) &&
                   host_->has_port(config_.template_corner_port.c_str()) &&
                   host_->has_port(config_.template_triangle_port.c_str());
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
    terrain_.clear();
    ground_cells_.clear();
    terrain_generation_ += 1u;
    template_corners_.clear();
    template_triangles_.clear();
    template_generation_ += 1u;
    sky_ = VivariumSkyView{};
    seeds_.clear();
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
    // with its own warning; a long frame is the background time, and the host catches it up.
    uint32_t steps = 0u;
    double backlog = 0.0;
    const HostStatus status = host_->advance(elapsed_s, config_.catch_up_steps, &steps, &backlog);
    if (status != ase::kernel::HostStatusOk) {
        return status;  // this frame is refused; the host said why
    }
    elapsed_in_s_ += elapsed_s;
    backlog_s_ = backlog;
    last_steps_ = steps;
    tick_count_ += steps;

    const HostStatus read = read_snapshot();
    if (read != ase::kernel::HostStatusOk) {
        return fail(GODOT_STEP_SNAPSHOT, read);
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
            return ase::kernel::HostStatusInvalidArgument;
        }
        view.place = static_cast<uint32_t>(place);
        view.stage = code_of(stage, GODOT_STAGE_COUNT);
        view.condition = code_of(condition, GODOT_COND_COUNT);
        view.soil = code_of(record_value(record, GODOT_KEY_SOIL), GODOT_SOIL_COUNT);
        patches_[view.object_id] = view;
        places_[view.place] = view.object_id;
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
        return ase::kernel::HostStatusInvalidArgument;
    }
    VivariumClockView clock;
    clock.elapsed_s = record_value(clock_records[0], GODOT_KEY_ELAPSED);
    clock.day = record_value(clock_records[0], GODOT_KEY_DAY);
    clock.hour = record_value(clock_records[0], GODOT_KEY_HOUR);
    if (!std::isfinite(clock.elapsed_s) || !std::isfinite(clock.day) || !std::isfinite(clock.hour)) {
        return ase::kernel::HostStatusInvalidArgument;
    }
    clock.known = true;
    clock_ = clock;
    const HostStatus organs = read_organs();
    if (organs != ase::kernel::HostStatusOk) {
        return organs;
    }
    const HostStatus terrain = read_terrain();
    if (terrain != ase::kernel::HostStatusOk) {
        return terrain;
    }
    const HostStatus templates = read_templates();
    if (templates != ase::kernel::HostStatusOk) {
        return templates;
    }
    const HostStatus sky = read_sky();
    if (sky != ase::kernel::HostStatusOk) {
        return sky;
    }
    return read_seeds();
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
                return ase::kernel::HostStatusInvalidArgument;
            }
            // the segment port carries the bark of every internode as its twelfth value
            if (c == GODOT_ORGAN_SEGMENT && !segment_bark_of(organ_records_[i], view)) {
                organs_[c].clear();
                return ase::kernel::HostStatusInvalidArgument;
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
                return ase::kernel::HostStatusInvalidArgument;
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
                return ase::kernel::HostStatusInvalidArgument;
            }
            instances_[c].push_back(view);
        }
    }
    organ_generation_ += 1u;
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
            return ase::kernel::HostStatusInvalidArgument;
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
            return ase::kernel::HostStatusInvalidArgument;
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
            return ase::kernel::HostStatusInvalidArgument;
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
            return ase::kernel::HostStatusInvalidArgument;
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
    // THE SKY IS READ EVERY SNAPSHOT. ase-pl-sky lays its four records in every Production run
    // (SkyHostScneExptSystem), a handful of numbers - no schedule count saves anything here. Until the
    // plugin's light pipeline has run once the port publishes an empty set: the sky is not known yet,
    // and nothing is broken. A set that is not the four records of one sky is.
    uint32_t written = 0u;
    const HostStatus status = snapshot_all(*host_, config_.sky_port, organ_records_, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    if (written == 0u) {
        sky_ = VivariumSkyView{};
        return ase::kernel::HostStatusOk;
    }
    if (written != GODOT_SKY_RECORD_COUNT) {
        sky_ = VivariumSkyView{};
        return ase::kernel::HostStatusInvalidArgument;
    }
    VivariumSkyView sky;
    if (!sky_view_of(organ_records_, written, sky)) {
        sky_ = VivariumSkyView{};
        return ase::kernel::HostStatusInvalidArgument;
    }
    sky_ = sky;
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
            return ase::kernel::HostStatusInvalidArgument;
        }
        seeds_.push_back(view);
    }
    return ase::kernel::HostStatusOk;
}

HostStatus GodotHostResourceManager::irrigate(uint32_t object_id) {
    if (!running()) {
        return ase::kernel::HostStatusInvalidState;
    }
    KernelHostInput input{};
    input.object_id = object_id;
    if (!put_value(input, GODOT_KEY_AMOUNT_MM, config_.irrigate_amount_mm)) {
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

const std::vector<VivariumTerrainView>& GodotHostResourceManager::get_terrain() const {
    return terrain_;
}

const std::vector<VivariumGroundCellView>& GodotHostResourceManager::get_ground_cells() const {
    return ground_cells_;
}

uint64_t GodotHostResourceManager::terrain_generation() const {
    return terrain_generation_;
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
    terrain_.clear();
    ground_cells_.clear();
    terrain_runs_ = 0u;
    terrain_reads_due_ = 0u;
    terrain_generation_ += 1u;  // and the ground of the old host
    template_corners_.clear();
    template_triangles_.clear();
    template_generation_ += 1u;  // and its templates
    sky_ = VivariumSkyView{};   // and its light
    seeds_.clear();             // and its seeds
    bundle_ = VivariumBundleInfo{};
    manifests_.clear();
}

}  // namespace ase::adp::godot

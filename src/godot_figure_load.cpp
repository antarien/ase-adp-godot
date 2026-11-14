/**
 * ASE GODOT ADAPTER - FIGURE LOAD IMPLEMENTATION
 *
 * @file        godot_figure_load.cpp
 * @brief       Parses and packs the figure file in a task of the WorkerThreadPool and hands the pack over once
 * @description See godot_figure_load.hpp. The task loads the file through godot_asset_loader.hpp (GLTFDocument,
 *              the texture packs beside the file bound into its materials) and packs the generated tree into a
 *              PackedScene; the glTF document owns every node it generates to the root, so the pack carries
 *              the whole tree. A native task stays in the pool until it is waited for, so the frame that sees
 *              it completed waits for it once, which returns at once, and only then reads the pack.
 *
 *              The file is read in blocks of kFigureLoadBlock and parsed from the bytes (parse_asset_bytes),
 *              so the start screen counts every block read and then the parse, the scene and the pack
 *              (figure_load_share). The time of each step goes into the record for the log line.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101611 - animate character motion
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_figure_load.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/gltf_document.hpp>
#include <godot_cpp/classes/gltf_state.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

/** The description the task carries in the pool; only this unit starts it, so it stands here and not in types.hpp. */
constexpr const char* kFigureLoadTask = "figure file";

/** The block the task reads the file in; each one read is a step of the start screen (4 MiB: 43 steps for 169.6 MB). */
constexpr int64_t kFigureLoadBlock = 4 * 1024 * 1024;

/** The steps after the blocks: the parse, the scene, the pack. */
constexpr int64_t kFigureLoadPhases = 3;

int64_t blocks_of(int64_t bytes) {
    return (bytes + kFigureLoadBlock - 1) / kFigureLoadBlock;
}

/** The file read block by block; the bytes read stand in the record after every block. Empty when it does not read whole. */
::godot::PackedByteArray read_blocks(FigureSceneLoad* load, const ::godot::String& path) {
    const ::godot::Ref<::godot::FileAccess> source = ::godot::FileAccess::open(path, ::godot::FileAccess::READ);
    if (source.is_null()) {
        return ::godot::PackedByteArray();
    }
    const int64_t total = static_cast<int64_t>(source->get_length());
    __atomic_store_n(&load->bytes_total, total, __ATOMIC_RELEASE);
    ::godot::PackedByteArray bytes;
    int64_t read = 0;
    while (read < total) {
        const ::godot::PackedByteArray block = source->get_buffer(ase::math::min(kFigureLoadBlock, total - read));
        if (block.is_empty()) {
            return ::godot::PackedByteArray();
        }
        bytes.append_array(block);
        read += block.size();
        __atomic_store_n(&load->bytes_read, read, __ATOMIC_RELEASE);
    }
    return bytes;
}

/**
 * The task: reads the file in blocks, parses the bytes, generates the scene, packs it and frees the tree; the
 * pack is the only thing it hands back. After every step it counts it in the record for the start screen.
 */
void load_figure_file(void* userdata) {
    FigureSceneLoad* load = static_cast<FigureSceneLoad*>(userdata);
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const int64_t begun = static_cast<int64_t>(clock->get_ticks_usec());
    const ::godot::String path = load->asset_root.path_join(::godot::String::utf8(load->file.c_str()));
    const ::godot::PackedByteArray bytes = read_blocks(load, path);
    const int64_t read_end = static_cast<int64_t>(clock->get_ticks_usec());
    load->read_usec = read_end - begun;
    if (bytes.is_empty()) {
        return;
    }
    ::godot::Ref<::godot::GLTFDocument> document;
    document.instantiate();
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    const bool parsed = parse_asset_bytes(document, state, load->asset_root, load->file, bytes);
    const int64_t parse_end = static_cast<int64_t>(clock->get_ticks_usec());
    load->parse_usec = parse_end - read_end;
    __atomic_store_n(&load->phases_done, 1, __ATOMIC_RELEASE);
    if (!parsed) {
        return;
    }
    ::godot::Node* model = document->generate_scene(state);
    const int64_t scene_end = static_cast<int64_t>(clock->get_ticks_usec());
    load->scene_usec = scene_end - parse_end;
    __atomic_store_n(&load->phases_done, 2, __ATOMIC_RELEASE);
    if (model == nullptr) {
        return;
    }
    ::godot::Ref<::godot::PackedScene> pack;
    pack.instantiate();
    if (pack->pack(model) == ::godot::OK) {
        load->model = pack;
    }
    memdelete(model);
    load->pack_usec = static_cast<int64_t>(clock->get_ticks_usec()) - scene_end;
    __atomic_store_n(&load->phases_done, kFigureLoadPhases, __ATOMIC_RELEASE);
}

}  // namespace

void start_figure_load(FigureSceneLoad& load, const ::godot::String& asset_root, const std::string& file) {
    ::godot::WorkerThreadPool* pool = ::godot::WorkerThreadPool::get_singleton();
    if (file.empty() || load.task >= 0 || pool == nullptr) {
        return;
    }
    load.asset_root = asset_root;
    load.file = file;
    load.model.unref();
    load.bytes_total = 0;
    load.bytes_read = 0;
    load.phases_done = 0;
    load.read_usec = 0;
    load.parse_usec = 0;
    load.scene_usec = 0;
    load.pack_usec = 0;
    load.task = pool->add_native_task(&load_figure_file, &load, false, kFigureLoadTask);
}

bool figure_load_running(const FigureSceneLoad& load) {
    return load.task >= 0;
}

bool take_figure_load(FigureSceneLoad& load, ::godot::Ref<::godot::PackedScene>& model) {
    ::godot::WorkerThreadPool* pool = ::godot::WorkerThreadPool::get_singleton();
    if (load.task < 0 || pool == nullptr || !pool->is_task_completed(load.task)) {
        return false;
    }
    pool->wait_for_task_completion(load.task);
    load.task = -1;
    model = load.model;
    load.model.unref();
    return true;
}

void end_figure_load(FigureSceneLoad& load) {
    ::godot::WorkerThreadPool* pool = ::godot::WorkerThreadPool::get_singleton();
    if (load.task < 0 || pool == nullptr) {
        return;
    }
    pool->wait_for_task_completion(load.task);
    load.task = -1;
    load.model.unref();
}

double figure_load_share(const FigureSceneLoad& load) {
    if (load.task < 0) {
        return 1.0;
    }
    const int64_t total = __atomic_load_n(&load.bytes_total, __ATOMIC_ACQUIRE);
    if (total <= 0) {
        return 0.0;
    }
    const int64_t read = __atomic_load_n(&load.bytes_read, __ATOMIC_ACQUIRE);
    const int64_t phases = __atomic_load_n(&load.phases_done, __ATOMIC_ACQUIRE);
    const int64_t done = blocks_of(read) + phases;
    const int64_t steps = blocks_of(total) + kFigureLoadPhases;
    return static_cast<double>(done) / static_cast<double>(steps);
}

::godot::String figure_load_times(const FigureSceneLoad& load) {
    return ::godot::String("(read ") + ::godot::String::num_int64(load.read_usec / 1000) + " ms in " +
           ::godot::String::num_int64(blocks_of(load.bytes_total)) + " blocks, parse " +
           ::godot::String::num_int64(load.parse_usec / 1000) + " ms, scene " +
           ::godot::String::num_int64(load.scene_usec / 1000) + " ms, pack " +
           ::godot::String::num_int64(load.pack_usec / 1000) + " ms)";
}

}  // namespace ase::adp::godot

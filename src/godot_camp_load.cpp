/**
 * ASE GODOT ADAPTER - CAMP LOAD IMPLEMENTATION
 *
 * @file        godot_camp_load.cpp
 * @brief       Parses one camp export in a task of the WorkerThreadPool into its makes and hands them over once
 * @description See godot_camp_load.hpp. The task reads the file whole, parses it through godot_asset_loader.hpp,
 *              reads the extras of every node from the document's JSON and takes each node's mesh from the parsed
 *              state (GLTFNode → GLTFMesh → ImporterMesh); no scene is generated, so the task holds no Node and
 *              touches no tree, also when the pool runs a second camp task on its thread while a texture load
 *              of the first one waits. A node is a make only with all four extras -
 *              make a text ending in .<variant>, mass_kg a positive number, roughness_m a number not below zero,
 *              hull at least IGL_HULL_MIN_PNT corners of three finite numbers each. A node without a make text is
 *              skipped silently, so a helper node of the recipe never becomes a body; a node WITH a make text that
 *              lacks another extra is counted on the load with the first one's missing extra, and so is a file
 *              that cannot be read or parsed, because a role left with no make is otherwise drawn by nobody and
 *              named by nothing (an export of an older recipe version, measured 2026-11-23: v1 carries no
 *              roughness_m, every role came out with makes=0 and the log said only "has no make in its export").
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    render/pipeline/stage
 * @usecase     usecase:5436 - kindle fire
 * @actor       Funken (FNKN)
 * @created     2026-11-21
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_camp_load.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>

#include <ase/kernel/kernel_embedded_host.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/gltf_document.hpp>
#include <godot_cpp/classes/gltf_mesh.hpp>
#include <godot_cpp/classes/gltf_node.hpp>
#include <godot_cpp/classes/gltf_state.hpp>
#include <godot_cpp/classes/importer_mesh.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cmath>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

/** The description the task carries in the pool; only this unit starts it, so it stands here and not in types.hpp. */
constexpr const char* kCampLoadTask = "camp file";

/** The values of one hull corner: east, up, north. */
constexpr int64_t kCampHullAxes = 3;

/** The fewest corners a convex hull with a volume has; ase-pl-ingle refuses fewer (IGL_HULL_MIN_PNT). */
constexpr int64_t kCampHullMinCorners = 4;

/** The variant of a make string: the whole number after its last dot; false without one. */
bool variant_of(const ::godot::String& make, uint32_t& variant) {
    const int64_t dot = make.rfind(".");
    if (dot < 0) {
        return false;
    }
    const ::godot::String suffix = make.substr(dot + 1);
    if (suffix.is_empty() || !suffix.is_valid_int()) {
        return false;
    }
    const int64_t value = suffix.to_int();
    if (value < 0 || value > static_cast<int64_t>(UINT32_MAX)) {
        return false;
    }
    variant = static_cast<uint32_t>(value);
    return true;
}

/** The hull of the extras as flat corners; false when it is no list of at least four corners of finite numbers. */
bool hull_of(const ::godot::Variant& value, std::vector<double>& hull) {
    if (value.get_type() != ::godot::Variant::ARRAY) {
        return false;
    }
    const ::godot::Array values = value;
    const int64_t count = values.size();
    if (count % kCampHullAxes != 0 || count < kCampHullAxes * kCampHullMinCorners) {
        return false;
    }
    hull.clear();
    hull.reserve(static_cast<size_t>(count));
    for (int64_t i = 0; i < count; ++i) {
        const ::godot::Variant::Type type = values[i].get_type();
        if (type != ::godot::Variant::FLOAT && type != ::godot::Variant::INT) {
            return false;
        }
        const double number = values[i];
        if (!std::isfinite(number)) {
            return false;
        }
        hull.push_back(number);
    }
    return true;
}

/** The make text of a node's extras; empty for a node without extras or without a make text (a helper node). */
::godot::String make_text_of(const ::godot::Dictionary& node) {
    const ::godot::Variant extras_value = node.get("extras", ::godot::Variant());
    if (extras_value.get_type() != ::godot::Variant::DICTIONARY) {
        return ::godot::String();
    }
    const ::godot::Dictionary extras = extras_value;
    const ::godot::Variant name = extras.get("make", ::godot::Variant());
    if (name.get_type() != ::godot::Variant::STRING) {
        return ::godot::String();
    }
    return name;
}

/**
 * The make one glTF node with a make text carries in its extras; returns nullptr when the node carries mass_kg,
 * roughness_m and hull of the recipe, else the extra it lacks or holds out of range, so the view names why a role
 * has no make. The roughness is required like the mass (ase-pl-ingle refuses a part without it); ase-physics
 * bounds it. A node without a make text never reaches here (make_text_of).
 */
const char* make_of(const ::godot::Dictionary& node, const ::godot::String& text, CampMake& make) {
    const ::godot::Dictionary extras = node.get("extras", ::godot::Dictionary());
    const ::godot::Variant mass = extras.get("mass_kg", ::godot::Variant());
    const ::godot::Variant rough = extras.get("roughness_m", ::godot::Variant());
    if (mass.get_type() != ::godot::Variant::FLOAT && mass.get_type() != ::godot::Variant::INT) {
        return "mass_kg";
    }
    if (rough.get_type() != ::godot::Variant::FLOAT && rough.get_type() != ::godot::Variant::INT) {
        return "roughness_m";
    }
    make.mass_kg = mass;
    make.roughness_m = rough;
    if (!(make.mass_kg > 0.0) || !std::isfinite(make.mass_kg)) {
        return "mass_kg";
    }
    if (!variant_of(text, make.variant)) {
        return "make variant";
    }
    if (!(make.roughness_m >= 0.0) || !std::isfinite(make.roughness_m)) {
        return "roughness_m";
    }
    if (!hull_of(extras.get("hull", ::godot::Variant()), make.hull)) {
        return "hull";
    }
    const ::godot::CharString utf8 = text.utf8();
    make.name = std::string(utf8.get_data());
    make.make = ase::kernel::KernelEmbeddedHost::name_id(make.name.c_str());
    return nullptr;
}

/**
 * The mesh of glTF node `index`, the node itself or the first mesh below it depth first; null without one. It is
 * read from the parsed state (GLTFNode → GLTFMesh → ImporterMesh, its surfaces with their materials), never from a
 * generated scene node: generate_scene replaces each importer mesh node by its MeshInstance3D and frees it, so the
 * node GLTFState::get_scene_node hands back afterwards is a freed object.
 */
::godot::Ref<::godot::Mesh> mesh_of(const ::godot::TypedArray<::godot::GLTFNode>& nodes,
                                    const ::godot::TypedArray<::godot::GLTFMesh>& meshes, int32_t index) {
    if (index < 0 || index >= nodes.size()) {
        return ::godot::Ref<::godot::Mesh>();
    }
    const ::godot::Ref<::godot::GLTFNode> node = nodes[index];
    if (node.is_null()) {
        return ::godot::Ref<::godot::Mesh>();
    }
    const int32_t mesh = node->get_mesh();
    if (mesh >= 0 && mesh < meshes.size()) {
        const ::godot::Ref<::godot::GLTFMesh> gltf_mesh = meshes[mesh];
        const ::godot::Ref<::godot::ImporterMesh> importer =
            gltf_mesh.is_valid() ? gltf_mesh->get_mesh() : ::godot::Ref<::godot::ImporterMesh>();
        if (importer.is_valid()) {
            return importer->get_mesh();
        }
    }
    const ::godot::PackedInt32Array children = node->get_children();
    for (int64_t i = 0; i < children.size(); ++i) {
        const ::godot::Ref<::godot::Mesh> below = mesh_of(nodes, meshes, children[i]);
        if (below.is_valid()) {
            return below;
        }
    }
    return ::godot::Ref<::godot::Mesh>();
}

/** The task: reads and parses the file, takes every make of its nodes with the mesh of its node from the state. */
void load_camp_file(void* userdata) {
    CampRoleLoad* load = static_cast<CampRoleLoad*>(userdata);
    const ::godot::String path = load->asset_root.path_join(::godot::String::utf8(load->file.c_str()));
    const ::godot::PackedByteArray bytes = ::godot::FileAccess::get_file_as_bytes(path);
    if (bytes.is_empty()) {
        load->refusal = "file " + load->file + " unreadable or empty";
        return;
    }
    ::godot::Ref<::godot::GLTFDocument> document;
    document.instantiate();
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    if (!parse_asset_bytes(document, state, load->asset_root, load->file, bytes)) {
        load->refusal = "file " + load->file + " not parsed as glTF";
        return;
    }
    // the parsed nodes and meshes stand in the state in the order of the document's nodes, so json node i is
    // GLTFNode i; no scene is generated, the task holds no Node and touches no tree
    const ::godot::TypedArray<::godot::GLTFNode> parsed_nodes = state->get_nodes();
    const ::godot::TypedArray<::godot::GLTFMesh> parsed_meshes = state->get_meshes();
    const ::godot::Dictionary json = state->get_json();
    const ::godot::Variant nodes_value = json.get("nodes", ::godot::Variant());
    if (nodes_value.get_type() == ::godot::Variant::ARRAY) {
        const ::godot::Array nodes = nodes_value;
        for (int64_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].get_type() != ::godot::Variant::DICTIONARY) {
                continue;
            }
            const ::godot::Dictionary node = nodes[i];
            const ::godot::String text = make_text_of(node);
            if (text.is_empty()) {
                continue;
            }
            CampMake make;
            const char* lacks = make_of(node, text, make);
            if (lacks != nullptr) {
                if (load->refused == 0u) {
                    const ::godot::CharString utf8 = text.utf8();
                    load->refusal = std::string(utf8.get_data()) + " lacks " + lacks;
                }
                ++load->refused;
                continue;
            }
            make.mesh = mesh_of(parsed_nodes, parsed_meshes, static_cast<int32_t>(i));
            load->makes.push_back(make);
        }
    }
}

}  // namespace

void start_camp_load(CampRoleLoad& load, const ::godot::String& asset_root) {
    ::godot::WorkerThreadPool* pool = ::godot::WorkerThreadPool::get_singleton();
    if (load.file.empty() || load.task >= 0 || pool == nullptr) {
        return;
    }
    load.asset_root = asset_root;
    load.makes.clear();
    load.refused = 0u;
    load.refusal.clear();
    load.taken = false;
    load.task = pool->add_native_task(&load_camp_file, &load, false, kCampLoadTask);
}

bool take_camp_load(CampRoleLoad& load) {
    ::godot::WorkerThreadPool* pool = ::godot::WorkerThreadPool::get_singleton();
    if (load.task < 0 || pool == nullptr || !pool->is_task_completed(load.task)) {
        return false;
    }
    pool->wait_for_task_completion(load.task);
    load.task = -1;
    load.taken = true;
    return true;
}

void end_camp_load(CampRoleLoad& load) {
    ::godot::WorkerThreadPool* pool = ::godot::WorkerThreadPool::get_singleton();
    if (load.task < 0 || pool == nullptr) {
        return;
    }
    pool->wait_for_task_completion(load.task);
    load.task = -1;
    load.makes.clear();
}

}  // namespace ase::adp::godot

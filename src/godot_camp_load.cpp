/**
 * ASE GODOT ADAPTER - CAMP LOAD IMPLEMENTATION
 *
 * @file        godot_camp_load.cpp
 * @brief       Parses one camp export in a task of the WorkerThreadPool into its makes and hands them over once
 * @description See godot_camp_load.hpp. The task reads the file whole, parses it through godot_asset_loader.hpp,
 *              reads the extras of every node from the document's JSON and generates the scene only to take each
 *              node's mesh; the generated tree is freed by the task. A node is a make only with all three extras -
 *              make a text ending in .<variant>, mass_kg a positive number, hull at least IGL_HULL_MIN_PNT corners
 *              of three finite numbers each; any other node is skipped, so a helper node of the recipe never
 *              becomes a body.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    render/pipeline/stage
 * @usecase     usecase:5436 - kindle fire
 * @actor       Funken (FNKN)
 * @created     2026-11-21
 * @modified    2026-11-21
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
#include <godot_cpp/classes/gltf_document.hpp>
#include <godot_cpp/classes/gltf_state.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
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

/** The make one glTF node carries in its extras; false for a node without make, mass_kg and hull of the recipe. */
bool make_of(const ::godot::Dictionary& node, CampMake& make) {
    const ::godot::Variant extras_value = node.get("extras", ::godot::Variant());
    if (extras_value.get_type() != ::godot::Variant::DICTIONARY) {
        return false;
    }
    const ::godot::Dictionary extras = extras_value;
    const ::godot::Variant name = extras.get("make", ::godot::Variant());
    const ::godot::Variant mass = extras.get("mass_kg", ::godot::Variant());
    if (name.get_type() != ::godot::Variant::STRING) {
        return false;
    }
    if (mass.get_type() != ::godot::Variant::FLOAT && mass.get_type() != ::godot::Variant::INT) {
        return false;
    }
    const ::godot::String text = name;
    make.mass_kg = mass;
    if (!(make.mass_kg > 0.0) || !std::isfinite(make.mass_kg) || !variant_of(text, make.variant)) {
        return false;
    }
    if (!hull_of(extras.get("hull", ::godot::Variant()), make.hull)) {
        return false;
    }
    const ::godot::CharString utf8 = text.utf8();
    make.name = std::string(utf8.get_data());
    make.make = ase::kernel::KernelEmbeddedHost::name_id(make.name.c_str());
    return true;
}

/** The mesh of the generated node `index`, the node itself or the first mesh below it; null without one. */
::godot::Ref<::godot::Mesh> mesh_of(const ::godot::Ref<::godot::GLTFState>& state, int32_t index) {
    ::godot::Node* node = state->get_scene_node(index);
    ::godot::MeshInstance3D* instance = ::godot::Object::cast_to<::godot::MeshInstance3D>(node);
    if (instance == nullptr) {
        instance = first_mesh(node);
    }
    if (instance == nullptr) {
        return ::godot::Ref<::godot::Mesh>();
    }
    return instance->get_mesh();
}

/** The task: reads and parses the file, takes every make of its nodes with the mesh of its node, frees the tree. */
void load_camp_file(void* userdata) {
    CampRoleLoad* load = static_cast<CampRoleLoad*>(userdata);
    const ::godot::String path = load->asset_root.path_join(::godot::String::utf8(load->file.c_str()));
    const ::godot::PackedByteArray bytes = ::godot::FileAccess::get_file_as_bytes(path);
    if (bytes.is_empty()) {
        return;
    }
    ::godot::Ref<::godot::GLTFDocument> document;
    document.instantiate();
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    if (!parse_asset_bytes(document, state, load->asset_root, load->file, bytes)) {
        return;
    }
    ::godot::Node* scene = document->generate_scene(state);
    if (scene == nullptr) {
        return;
    }
    const ::godot::Dictionary json = state->get_json();
    const ::godot::Variant nodes_value = json.get("nodes", ::godot::Variant());
    if (nodes_value.get_type() == ::godot::Variant::ARRAY) {
        const ::godot::Array nodes = nodes_value;
        for (int64_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].get_type() != ::godot::Variant::DICTIONARY) {
                continue;
            }
            CampMake make;
            if (!make_of(nodes[i], make)) {
                continue;
            }
            make.mesh = mesh_of(state, static_cast<int32_t>(i));
            load->makes.push_back(make);
        }
    }
    memdelete(scene);
}

}  // namespace

void start_camp_load(CampRoleLoad& load, const ::godot::String& asset_root) {
    ::godot::WorkerThreadPool* pool = ::godot::WorkerThreadPool::get_singleton();
    if (load.file.empty() || load.task >= 0 || pool == nullptr) {
        return;
    }
    load.asset_root = asset_root;
    load.makes.clear();
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

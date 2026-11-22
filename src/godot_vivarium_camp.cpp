/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW CAMP FIRE
 *
 * @file        godot_vivarium_camp.cpp
 * @brief       VivariumView::follow_camp - the camp fire laid once through ase-pl-ingle, its bodies drawn, wood laid
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.0, T.1, T.3, T.7 (usecase:5436 - kindle fire). The view computes
 *              nothing of the fire: it hands the host what the start file and the exports carry and draws what the
 *              host reports. In this order, each step once per host:
 *
 *                1. every role of camp.parts loads its export in a task of the WorkerThreadPool (godot_camp_load.hpp);
 *                2. the camp place goes out on ingle.camp.v1, measured in T.0 on the height cells of ase-terrain;
 *                3. once every load is taken, every hull corner of every make goes out on ingle.hull.v1 - ase-physics
 *                   builds the collision form from them, never the plugin and never the view;
 *                4. on a later frame every part goes out on ingle.part.v1 in the frame of the patch of camp.place;
 *                   ase-physics drops it under GRV_CONST_ACCELERATION onto the height field, tips it and lets it rest.
 *
 *              Every frame the bodies of ingle.body.v1 are drawn with the mesh of their make, placed and turned as
 *              ase-physics holds them. A keeper ase-bdi sets to laying wood (ingle.refuel.v1) gets one new log laid as
 *              a part of its own above the body he tends, once while that action stands: ase-fire never adds wood to
 *              a burning cell, the new log becomes a fuel body and a fire cell of its own where it lands.
 *
 *              A role the stage binds no file for, a part whose variant the export does not carry, a body whose make
 *              has no mesh: each is named on its own line and stays out - never drawn as a stand-in shape.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    ecs/module
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

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_camp_load.hpp>
#include <ase/adp/godot/godot_vivarium_camp.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <iterator>
#include <string>
#include <unordered_set>

namespace ase::adp::godot {

namespace {

constexpr const char* kCampLogRole = "camp.log";    // the role of the logs a keeper lays (ingle.refuel.v1, T.7)
constexpr const char* kCampRootName = "Camp";        // the node the camp bodies are drawn under
constexpr const char* kCampBodyName = "CampBody_";   // the prefix of a body node, followed by its entity id
constexpr uint32_t kCampKeeperShift = 32u;           // keeper << 32 | body: one key per keeper and the body he tends

using ase::kernel::HostStatus;

/** The make of `role` and `variant` among the taken loads; nullptr when the export carries no such node. */
const CampMake* find_make(const VivariumCampState& camp, const std::string& role, uint32_t variant) {
    const auto load = camp.loads.find(role);
    if (load == camp.loads.end()) {
        return nullptr;
    }
    for (const CampMake& make : load->second.makes) {
        if (make.variant == variant) {
            return &make;
        }
    }
    return nullptr;
}

/** The turn of a rotation vector: its direction the axis, its length the angle in radians; none at length 0. */
::godot::Basis turn_of(const VivariumCampBodyView& body) {
    const ::godot::Vector3 turn(static_cast<float>(body.tx), static_cast<float>(body.ty), static_cast<float>(body.tz));
    const float angle = turn.length();
    if (!(angle > 0.0f)) {
        return ::godot::Basis();
    }
    return ::godot::Basis(turn / angle, angle);
}

}  // namespace

void restart_camp(VivariumCampState& camp) {
    camp.camp_laid = false;
    camp.hulls_filed = false;
    camp.parts_filed = false;
    camp.refuels_laid.clear();
    camp.next_log = 0u;
    for (auto& [body, node] : camp.drawn) {
        if (node != nullptr) {
            node->queue_free();
        }
    }
    camp.drawn.clear();
}

void end_camp(VivariumCampState& camp) {
    for (auto& [role, load] : camp.loads) {
        end_camp_load(load);
    }
}

void VivariumView::follow_camp() {
    if (!configured_ || scene_ == nullptr) {
        return;
    }
    // 1. THE EXPORTS: every role of camp.parts and the logs of a refuel, each in a task of its own.
    if (!camp_.loads_started) {
        camp_.loads_started = true;
        std::unordered_set<std::string> roles{kCampLogRole};
        for (const VivariumCampPartSpec& part : start_.camp_parts) {
            roles.insert(part.role);
        }
        for (const std::string& role : roles) {
            CampRoleLoad& load = camp_.loads[role];
            load.role = role;
            const auto name = asset_roles_.find(role);
            const auto file = name != asset_roles_.end() ? assets_.files.find(name->second) : assets_.files.end();
            if (file == assets_.files.end()) {
                note("camp role " + text_of(role) + " bound to no file");
                continue;
            }
            load.file = file->second;
            start_camp_load(load, asset_root_);
        }
    }
    bool loading = false;
    for (auto& [role, load] : camp_.loads) {
        if (take_camp_load(load)) {
            for (const CampMake& make : load.makes) {
                camp_.meshes[make.make] = make.mesh;
            }
            note("camp role " + text_of(role) + " makes=" + ::godot::String::num_uint64(load.makes.size()));
        }
        loading = loading || load.task >= 0;
    }
    if (!resources_.running() || !resources_.camp_offered()) {
        return;
    }
    // 2. THE PLACE (T.0), once per host.
    if (!camp_.camp_laid) {
        camp_.camp_laid = true;
        const HostStatus laid = resources_.lay_camp(start_.camp_place, start_.camp_x, start_.camp_z);
        note("camp lay place=" + ::godot::String::num_uint64(start_.camp_place) + " x=" +
             ::godot::String::num(start_.camp_x, 3) + " z=" + ::godot::String::num(start_.camp_z, 3) +
             " result=" + GodotHostResourceManager::status_name(laid));
    }
    if (loading) {
        return;
    }
    // 3. THE HULLS (T.1): every corner of every make, before the first part of it, on a frame of their own.
    if (!camp_.hulls_filed) {
        camp_.hulls_filed = true;
        uint64_t filed = 0u;
        uint64_t refused = 0u;
        for (const auto& [role, load] : camp_.loads) {
            for (const CampMake& make : load.makes) {
                for (size_t i = 0u; i + 2u < make.hull.size(); i += 3u) {
                    const HostStatus status = resources_.file_hull(static_cast<uint32_t>(i / 3u), make.make,
                                                                   make.hull[i], make.hull[i + 1u], make.hull[i + 2u]);
                    (status == ase::kernel::HostStatusOk ? filed : refused) += 1u;
                }
            }
        }
        note("camp hulls filed=" + ::godot::String::num_uint64(filed) + " refused=" + ::godot::String::num_uint64(refused));
        return;
    }
    // 4. THE PARTS (T.3): in the frame of the patch of camp.place, each once; ase-physics lets them fall.
    const VivariumPatchView* patch = resources_.get_patch_at(start_.camp_place);
    if (!camp_.parts_filed && patch != nullptr) {
        camp_.parts_filed = true;
        for (const VivariumCampPartSpec& part : start_.camp_parts) {
            const CampMake* make = find_make(camp_, part.role, part.variant);
            if (make == nullptr) {
                note("camp part " + text_of(part.role) + "." + ::godot::String::num_uint64(part.variant) +
                     " has no make in its export");
                continue;
            }
            const HostStatus status = resources_.file_part(patch->object_id, part.east, part.north, part.drop,
                                                           part.fall, make->mass_kg, make->make, part.fuel);
            note("camp part " + text_of(make->name) + " east=" + ::godot::String::num(part.east, 3) + " north=" +
                 ::godot::String::num(part.north, 3) + " result=" + GodotHostResourceManager::status_name(status));
        }
    }
    // THE BODIES: placed and turned as ase-physics holds them, in the frame of their patch.
    if (camp_.root == nullptr) {
        camp_.root = memnew(::godot::Node3D);
        camp_.root->set_name(kCampRootName);
        scene_->add_child(camp_.root);
    }
    std::unordered_set<uint32_t> seen;
    for (const VivariumCampBodyView& body : resources_.get_camp_bodies()) {
        const VivariumPatchView* frame = resources_.get_patch(body.patch);
        const auto mesh = camp_.meshes.find(body.make);
        if (frame == nullptr || mesh == camp_.meshes.end() || mesh->second.is_null()) {
            continue;
        }
        seen.insert(body.object_id);
        ::godot::MeshInstance3D*& node = camp_.drawn[body.object_id];
        if (node == nullptr) {
            node = memnew(::godot::MeshInstance3D);
            node->set_name(::godot::String(kCampBodyName) + ::godot::String::num_uint64(body.object_id));
            node->set_mesh(mesh->second);
            camp_.root->add_child(node);
            note("camp body " + ::godot::String::num_uint64(body.object_id) + " drawn");
        }
        const ::godot::Vector3 origin(static_cast<float>(frame->x + body.x), static_cast<float>(frame->y + body.y),
                                      static_cast<float>(frame->z + body.z));
        node->set_transform(::godot::Transform3D(turn_of(body), origin));
    }
    for (auto drawn = camp_.drawn.begin(); drawn != camp_.drawn.end();) {
        if (seen.count(drawn->first) == 0u) {
            if (drawn->second != nullptr) {
                drawn->second->queue_free();
            }
            drawn = camp_.drawn.erase(drawn);
        } else {
            ++drawn;
        }
    }
    // THE WOOD (T.7): one new log above the body a keeper tends, once while his action stands.
    const auto& logs = camp_.loads[kCampLogRole].makes;
    const VivariumCampPartSpec* log_spec = nullptr;
    for (const VivariumCampPartSpec& part : start_.camp_parts) {
        if (part.role == kCampLogRole && log_spec == nullptr) {
            log_spec = &part;
        }
    }
    std::unordered_set<uint64_t> standing;
    for (const VivariumCampRefuelView& refuel : resources_.get_camp_refuels()) {
        const uint64_t key = (static_cast<uint64_t>(refuel.keeper) << kCampKeeperShift) | refuel.body;
        standing.insert(key);
        if (camp_.refuels_laid.count(key) != 0u) {
            continue;
        }
        const VivariumCampBodyView* target = nullptr;
        for (const VivariumCampBodyView& body : resources_.get_camp_bodies()) {
            if (body.object_id == refuel.body) {
                target = &body;
            }
        }
        if (target == nullptr) {
            continue;  // ingle.body.v1 shows the body on a later frame
        }
        camp_.refuels_laid.insert(key);
        if (logs.empty() || log_spec == nullptr) {
            note("camp refuel keeper=" + ::godot::String::num_uint64(refuel.keeper) + " has no log to lay");
            continue;
        }
        const CampMake& log = logs[camp_.next_log % logs.size()];
        camp_.next_log += 1u;
        const HostStatus status = resources_.file_part(target->patch, target->x, target->z, target->y + log_spec->drop,
                                                       log_spec->fall, log.mass_kg, log.make, true);
        note("camp refuel keeper=" + ::godot::String::num_uint64(refuel.keeper) + " body=" +
             ::godot::String::num_uint64(refuel.body) + " log=" + text_of(log.name) +
             " result=" + GodotHostResourceManager::status_name(status));
    }
    for (auto laid = camp_.refuels_laid.begin(); laid != camp_.refuels_laid.end();) {
        laid = standing.count(*laid) == 0u ? camp_.refuels_laid.erase(laid) : std::next(laid);
    }
}

}  // namespace ase::adp::godot

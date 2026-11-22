/**
 * ASE RESOURCE MANAGER IMPLEMENTATION (OOP-ECS Bridge)
 *
 * @file        godot_knot_resource_manager.cpp
 * @brief       GodotKnotResourceManager - when the knot and foot ports are read, and when a read is a new set
 * @description Flyweight Pattern implementation for the knot and foot views of the bark tubes, read out of the
 *              snapshots of the knot and foot ports of ase-pl-flora. See godot_knot_resource_manager.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * FLYWEIGHT PATTERN (INST_ASE_ECS_SER)
 *
 *   GodotHostResourceManager            GodotKnotResourceManager (NOT ECS!)
 *   ┌──────────────────────────┐       ┌──────────────────────────────────┐
 *   │ snapshot of knot + foot  │ ────> │ vector<VivariumKnotView>  knots_ │
 *   │ ports after every frame  │       │ vector<VivariumFootView>  feet_  │
 *   └──────────────────────────┘       └──────────────────────────────────┘
 *
 * WHY THIS EXISTS:
 *   PLAN_ASE_VIVARIUM_TREEGEN T.6: the tubes come as one knot per internode instead of one record per vertex.
 *   FloraHostCylNodeExptSystem files the marked knot rows at GODOT_KNOT_ROWS_PER_PASS per GODOT_KNOT_SCHEDULE pass
 *   and the foot rows in the pass of their knots; the host swaps a set in at the pass that files its last page. A
 *   read after a new organ run may still see the old set, so the ports are read with every snapshot until the new
 *   set is in. A run that changed no knot files nothing, and its first covered read ends the reads.
 *
 * WHAT IT NEVER DOES:
 *   It computes no knot, no frame and no radius; it takes what the ports carried and decides only whether a read is
 *   a new set.
 *
 * THREAD SAFETY:
 *   Everything runs on the thread that booted the host, which refuses any other; no second thread enters.
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

#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>
#include <ase/adp/godot/godot_host_organ_view.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ase::adp::godot {

namespace {

/** Whether two reads carry the same knots, feet and cut faces, record for record in port order. */
bool same_knots(const std::vector<VivariumKnotView>& knots, const std::vector<VivariumFootView>& feet,
                const std::vector<VivariumScarView>& scars, const std::vector<VivariumKnotView>& held_knots,
                const std::vector<VivariumFootView>& held_feet, const std::vector<VivariumScarView>& held_scars) {
    if (knots.size() != held_knots.size() || feet.size() != held_feet.size() || scars.size() != held_scars.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < scars.size(); ++i) {
        if (scars[i].object_id != held_scars[i].object_id || scars[i].body != held_scars[i].body) {
            return false;
        }
    }
    for (std::size_t i = 0u; i < knots.size(); ++i) {
        const VivariumKnotView& a = knots[i];
        const VivariumKnotView& b = held_knots[i];
        if (a.object_id != b.object_id || a.body != b.body || a.x != b.x || a.y != b.y || a.z != b.z ||
            a.radius != b.radius || a.qx != b.qx || a.qy != b.qy || a.qz != b.qz || a.qw != b.qw || a.v != b.v ||
            a.below != b.below) {
            return false;
        }
    }
    for (std::size_t i = 0u; i < feet.size(); ++i) {
        const VivariumFootView& a = feet[i];
        const VivariumFootView& b = held_feet[i];
        if (a.object_id != b.object_id || a.body != b.body || a.x != b.x || a.y != b.y || a.z != b.z ||
            a.radius != b.radius || a.qx != b.qx || a.qy != b.qy || a.qz != b.qz || a.qw != b.qw || a.v != b.v) {
            return false;
        }
    }
    return true;
}

/**
 * Whether the knots name exactly the internodes the segment port showed: every knot, every foot and every cut face
 * an internode of the set, every internode of the set with its knot. ase-render-data gives every internode its tip
 * knot, so a set that names other internodes is an old set the host still holds while the new one is filed page by
 * page.
 */
bool names_the_internodes(const std::vector<VivariumKnotView>& knots, const std::vector<VivariumFootView>& feet,
                          const std::vector<VivariumScarView>& scars,
                          const std::vector<VivariumOrganView>& internodes) {
    std::unordered_map<uint32_t, bool> knotted;
    knotted.reserve(internodes.size());
    for (const VivariumOrganView& internode : internodes) {
        knotted.emplace(internode.object_id, false);
    }
    for (const VivariumKnotView& knot : knots) {
        const auto found = knotted.find(knot.object_id);
        if (found == knotted.end()) {
            return false;
        }
        found->second = true;
    }
    for (const VivariumFootView& foot : feet) {
        if (knotted.find(foot.object_id) == knotted.end()) {
            return false;
        }
    }
    for (const VivariumScarView& scar : scars) {
        if (knotted.find(scar.object_id) == knotted.end()) {
            return false;
        }
    }
    for (const auto& entry : knotted) {
        if (!entry.second) {
            return false;
        }
    }
    return true;
}

}  // namespace

GodotKnotResourceManager::GodotKnotResourceManager() = default;

GodotKnotResourceManager::~GodotKnotResourceManager() = default;

bool GodotKnotResourceManager::due(uint64_t organ_runs, uint64_t knot_passes) {
    if (organ_runs != runs_ || generation_ == 0u) {
        runs_ = organ_runs;
        passes_ = knot_passes;
        owed_ = true;
    }
    return owed_;
}

void GodotKnotResourceManager::store_read(std::vector<VivariumKnotView>&& knots, std::vector<VivariumFootView>&& feet,
                                          std::vector<VivariumScarView>&& scars, uint64_t knot_passes,
                                          const std::vector<VivariumOrganView>& internodes) {
    const uint64_t pages =
        (static_cast<uint64_t>(knots.size()) + GODOT_KNOT_ROWS_PER_PASS - 1u) / GODOT_KNOT_ROWS_PER_PASS;
    const bool covered = knot_passes - passes_ > GODOT_ORGAN_READS_PER_RUN + pages;
    if (!same_knots(knots, feet, scars, knots_, feet_, scars_)) {
        knots_ = std::move(knots);
        feet_ = std::move(feet);
        scars_ = std::move(scars);
        generation_ += 1u;
        owed_ = false;
        return;
    }
    if (covered && names_the_internodes(knots_, feet_, scars_, internodes)) {
        owed_ = false;
    }
}

const std::vector<VivariumKnotView>& GodotKnotResourceManager::get_knots() const {
    return knots_;
}

const std::vector<VivariumFootView>& GodotKnotResourceManager::get_feet() const {
    return feet_;
}

const std::vector<VivariumScarView>& GodotKnotResourceManager::get_scars() const {
    return scars_;
}

uint64_t GodotKnotResourceManager::get_generation() const {
    return generation_;
}

void GodotKnotResourceManager::clear_all() {
    knots_.clear();
    feet_.clear();
    scars_.clear();
    runs_ = 0u;
    passes_ = 0u;
    owed_ = false;
    generation_ += 1u;  // the view drops the tubes of the dropped set
}

}  // namespace ase::adp::godot

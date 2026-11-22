#pragma once

/**
 * ASE RESOURCE MANAGER (NOT A COMPONENT!)
 *
 * @file        godot_knot_resource_manager.hpp
 * @brief       GodotKnotResourceManager - holds the knots and feet of the bark tubes as the knot and foot ports showed them
 * @description PLAN_ASE_VIVARIUM_TREEGEN T.6 (usecase:101613 show the plant bark tubes): ase-render-data carries one
 *              knot per internode - centre, radius, frame, v - and ase-pl-flora files them page by page on the knot
 *              port (flora.mesh.knot.v1), the knot at the foot of every axis on the foot port (flora.mesh.foot.v1).
 *              No vertex crosses a port. This manager holds the last complete read of both, decides when the ports
 *              are read again and when a read is a new set, and counts the sets it took, so the view lays the tubes
 *              anew once per new set.
 *
 *              WHY A MANAGER OF ITS OWN: every unit that names GodotHostResourceManager compiles its class header
 *              (data/probes/b0680_class_reach.py counts them); the class holds this one behind a pointer, so the
 *              knot views and their rules change here without recompiling those readers (the header ratchet of
 *              2026-11-21 holds that header).
 *
 *              NO GODOT TYPE: plain numbers in, plain views out, so the host tests check it without a Godot
 *              process.
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
 *                                                      │ get_knots / get_feet
 *                                                      ▼
 *                                       tubes_by_body (godot_plant_tubes.hpp)
 *
 * USAGE:
 *   // GodotHostResourceManager::read_knots, after the organ read of a frame
 *   if (knots.due(organ_runs, knot_passes)) {
 *       knots.store_read(std::move(read_knots), std::move(read_feet), knot_passes, internodes);
 *   }
 *   // the plant scene
 *   tubes_by_body(knots.get_knots(), knots.get_feet(), internodes);
 *
 * THIS IS NOT A COMPONENT!
 * This class holds the views of the last read of two host ports outside every registry; the ports are read again
 * whenever an organ run owes a new set, and the view holds no reference into the App.
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

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** One organ record of the segment port; declared in godot_host_organ_view.hpp, which only the .cpp includes. */
struct VivariumOrganView;

/**
 * The tip knot of one internode as the last read of the knot port showed it. Its ring is the upper ring of the tube
 * piece of the internode and the lower ring of its continuation child; the radius is the tip radius ase-render-data
 * gave the cone, which the child starts with.
 */
struct VivariumKnotView {
    uint32_t object_id = 0;   // the internode, as the segment port names it
    uint32_t body = 0;        // object id of the patch the plant stands on
    double   x = 0.0;         // centre of the knot, metres east of the stem base
    double   y = 0.0;         // centre of the knot, metres above the ground at the stem base
    double   z = 0.0;         // centre of the knot, metres north of the stem base
    double   radius = 0.0;    // radius of the ring at the knot, metres
    double   qx = 0.0;        // knot frame, vector part, east share: local y is the tangent, local x the carried across
    double   qy = 0.0;        // knot frame, vector part, upward share
    double   qz = 0.0;        // knot frame, vector part, north share
    double   qw = 1.0;        // knot frame, scalar part
    double   v = 0.0;         // along the axis from its foot, metres
    uint32_t below = 0;       // the internode whose knot is the lower ring of this piece; the internode itself at an axis foot
    uint32_t rings = 0;       // intermediate rings between the lower ring and this knot at detail level 0 (usecase:101643 - ring the tubes by curvature), 0 on a straight piece
};

/** The knot at the foot of an axis as the last read of the foot port showed it: the lower ring of its first internode. */
struct VivariumFootView {
    uint32_t object_id = 0;   // the internode that starts the axis
    uint32_t body = 0;        // object id of the patch the plant stands on
    double   x = 0.0;         // centre of the foot knot, metres east of the stem base
    double   y = 0.0;         // centre of the foot knot, metres above the ground at the stem base
    double   z = 0.0;         // centre of the foot knot, metres north of the stem base
    double   radius = 0.0;    // radius of the foot ring, metres: the own radius of the internode
    double   qx = 0.0;        // foot frame, vector part, east share
    double   qy = 0.0;        // foot frame, vector part, upward share
    double   qz = 0.0;        // foot frame, vector part, north share
    double   qw = 1.0;        // foot frame, scalar part
    double   v = 0.0;         // along the axis, metres: 0 at the foot
    uint32_t sides = 0;       // corners K of every ring of the axis this foot starts (usecase:101644 - count ring corners by size)
    double   noise = 0.0;     // in [0, 1], the key of o_axis, the place of the axis in the bark noise field (plant_bark.gdshader)
};

/**
 * The cut face of a stub as the last read of the scar port showed it: the internode whose tip knot a keeper's cut
 * left open (HUB_PLNT_CYL_KNOT_SCAR). The view closes that tip ring with a disk of wood; the knot itself stands on
 * the knot port, so this record names only the internode and its body.
 */
struct VivariumScarView {
    uint32_t object_id = 0;   // the internode of the stub, as the segment and the knot port name it
    uint32_t body = 0;        // object id of the patch the plant stands on
};

/**
 * This class lives OUTSIDE every registry. Its owner is the GodotHostResourceManager of one view; it is fed with the
 * views of each read and hands the last complete set out.
 */
class GodotKnotResourceManager {
public:
    GodotKnotResourceManager();
    ~GodotKnotResourceManager();

    GodotKnotResourceManager(const GodotKnotResourceManager&) = delete;
    GodotKnotResourceManager& operator=(const GodotKnotResourceManager&) = delete;
    GodotKnotResourceManager(GodotKnotResourceManager&&) = delete;
    GodotKnotResourceManager& operator=(GodotKnotResourceManager&&) = delete;

    /**
     * Whether the knot and foot ports are read in this snapshot. ase-render-data carries the knots of a body that
     * ticked in GODOT_ORGAN_SCHEDULE, and ase-pl-flora files the changed set at GODOT_KNOT_ROWS_PER_PASS rows per
     * GODOT_KNOT_SCHEDULE pass; the host keeps the old set until the pass that files the last page. So a new organ
     * run (or no set taken yet) owes a read, and the read stays owed until store_read took the new set.
     */
    [[nodiscard]] bool due(uint64_t organ_runs, uint64_t knot_passes);

    /**
     * Takes one read of the knot, foot and scar ports. A read that differs from the held set is the new set: it
     * replaces the held one and moves the generation. A read equal to the held set ends the owed reads once the
     * passes since the run covered GODOT_ORGAN_READS_PER_RUN organ reads and every page of the set and the knots
     * name exactly the internodes the segment port showed - a set naming others is the old set still held while the
     * new one is filed. ase-pl-flora files all three ports in the same pass and folds the cut faces into the same
     * checksum, so a knot that becomes a cut face is a new set of all three.
     */
    void store_read(std::vector<VivariumKnotView>&& knots, std::vector<VivariumFootView>&& feet,
                    std::vector<VivariumScarView>&& scars, uint64_t knot_passes,
                    const std::vector<VivariumOrganView>& internodes);

    /** The tip knots of every internode of the last set, in port order. */
    [[nodiscard]] const std::vector<VivariumKnotView>& get_knots() const;
    /** The foot knots of every axis of the last set, in port order. */
    [[nodiscard]] const std::vector<VivariumFootView>& get_feet() const;
    /** The cut faces of every stub of the last set, in port order. */
    [[nodiscard]] const std::vector<VivariumScarView>& get_scars() const;
    /** Grows by one every time a new set was taken or the set was dropped; the view lays the tubes anew then. */
    [[nodiscard]] uint64_t get_generation() const;

    /** BULK CLEANUP - drop both sets and every count of the reads; the generation grows, so the view drops its tubes. */
    void clear_all();

private:
    std::vector<VivariumKnotView> knots_;     // the tip knots of the last set
    std::vector<VivariumFootView> feet_;      // the foot knots of the last set
    std::vector<VivariumScarView> scars_;     // the cut faces of the last set
    uint64_t                     generation_ = 0;
    uint64_t                      runs_ = 0;    // GODOT_ORGAN_SCHEDULE runs at the last new run the reads followed
    uint64_t                      passes_ = 0;  // GODOT_KNOT_SCHEDULE passes at that run
    bool                          owed_ = false;  // a run's new set is not taken yet
};

}  // namespace ase::adp::godot

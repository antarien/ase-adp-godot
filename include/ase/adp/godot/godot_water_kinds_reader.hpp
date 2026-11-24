#pragma once

/**
 * ASE GODOT ADAPTER - THE READER OF THE LAKE, SEA AND GLACIER PORTS
 *
 * @file        godot_water_kinds_reader.hpp
 * @brief       Reads the three water kind ports of ase-pl-water into their views, with change detection
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO, section "Geometrie-Schicht: Wasser im Bild": the lake (water.lake.v1),
 *              the sea (water.sea.v1) and the glacier (water.glacier.v1) are three ports of ase-pl-water, one record
 *              per body or chunk. The readers lay a record out into its view the way read_currents does for the river:
 *              a value the record does not carry stays NaN, nothing is computed, and a port the config does not name
 *              or the plugin does not offer yet reads as empty. Each port keeps its own generation, which grows only
 *              when the read differs from the held one, so the view lays the water anew when ase-hydro changed it
 *              and not per frame.
 *
 *              The record functions and the comparisons carry no Godot type, so the host tests lay a hand-built
 *              record out the way the reader does.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_glacier_view.hpp>
#include <ase/adp/godot/godot_host_lake_view.hpp>
#include <ase/adp/godot/godot_host_sea_view.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/kernel/kernel_types.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The host port names of the three kinds; an empty name is a port the view does not read. */
struct VivariumWaterKindsPorts {
    std::string lake;     // water.lake.v1: one record per lake or pond body
    std::string sea;      // water.sea.v1: one record per salt chunk
    std::string glacier;  // water.glacier.v1: one record per chunk with ice
};

/** One lake record as a view; every key it lacks stays NaN. */
VivariumLakeView lake_view_of(const ase::kernel::KernelHostRecord& record);
/** One sea record as a view; every key it lacks stays NaN. */
VivariumSeaView sea_view_of(const ase::kernel::KernelHostRecord& record);
/** One glacier record as a view; every key it lacks stays NaN. */
VivariumGlacierView glacier_view_of(const ase::kernel::KernelHostRecord& record);

/** Whether the lake carries every value its surface is drawn from: the box on all three axes and its depth. */
bool lake_drawable(const VivariumLakeView& lake);
/** Whether the chunk carries every value its sea is drawn from: the chunk, its edge and its lowest ground. */
bool sea_drawable(const VivariumSeaView& sea);
/** Whether the chunk carries every value its ice is drawn from: the chunk, its edge, the ice and the melt. */
bool glacier_drawable(const VivariumGlacierView& glacier);

/** Whether two reads of the lake port carry the same bodies, in port order. */
bool same_lakes(const std::vector<VivariumLakeView>& read, const std::vector<VivariumLakeView>& held);
/** Whether two reads of the sea port carry the same chunks, in port order. */
bool same_seas(const std::vector<VivariumSeaView>& read, const std::vector<VivariumSeaView>& held);
/** Whether two reads of the glacier port carry the same chunks, in port order. */
bool same_glaciers(const std::vector<VivariumGlacierView>& read, const std::vector<VivariumGlacierView>& held);

/**
 * The held reads of the three ports with their generations. The resource manager holds one and calls read with
 * every snapshot.
 */
class GodotWaterKindsReader {
public:
    /** Reads the three ports once the plugin offers them; the first status that is not Ok is handed back. */
    ase::kernel::HostStatus read(ase::kernel::KernelEmbeddedHost& host, const VivariumWaterKindsPorts& ports);
    /** Empties the three reads and moves their generations, as a host restart does. */
    void clear();

    [[nodiscard]] const std::vector<VivariumLakeView>& get_lakes() const;
    [[nodiscard]] const std::vector<VivariumSeaView>& get_seas() const;
    [[nodiscard]] const std::vector<VivariumGlacierView>& get_glaciers() const;
    /** Grows by one every time a lake differs from the last read. */
    [[nodiscard]] uint64_t lake_generation() const;
    /** Grows by one every time a salt chunk differs from the last read. */
    [[nodiscard]] uint64_t sea_generation() const;
    /** Grows by one every time a chunk of ice differs from the last read. */
    [[nodiscard]] uint64_t glacier_generation() const;

private:
    std::vector<ase::kernel::KernelHostRecord> records_;  // the buffer a snapshot fills, grown once to the port's answer
    std::vector<VivariumLakeView>              lakes_;    // the lake and pond bodies, last read
    std::vector<VivariumSeaView>               seas_;     // the salt chunks, last read
    std::vector<VivariumGlacierView>           glaciers_; // the chunks with ice, last read
    uint64_t                                   lake_generation_ = 0;
    uint64_t                                   sea_generation_ = 0;
    uint64_t                                   glacier_generation_ = 0;
};

}  // namespace ase::adp::godot

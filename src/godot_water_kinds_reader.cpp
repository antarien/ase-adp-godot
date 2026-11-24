/**
 * ASE GODOT ADAPTER - THE READER OF THE LAKE, SEA AND GLACIER PORTS
 *
 * @file        godot_water_kinds_reader.cpp
 * @brief       Reads the three water kind ports of ase-pl-water into their views, with change detection
 * @description See godot_water_kinds_reader.hpp.
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

#include <ase/adp/godot/godot_water_kinds_reader.hpp>
#include <ase/adp/godot/godot_host_record.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

using ase::kernel::HostStatus;
using ase::kernel::KernelEmbeddedHost;
using ase::kernel::KernelHostRecord;

namespace {

/**
 * Whether two values of a water record are the same read. A value the record never carried stands NaN; NaN equals
 * nothing, so two unanswered reads count as the same, or every read of a port with an unanswered value would be a
 * change and the view would lay its water anew each frame.
 */
bool same_value(double read, double held) {
    return read == held || (std::isnan(read) && std::isnan(held));
}

/** Every record the port holds; the buffer grows once to the port's own answer. */
HostStatus snapshot_port(KernelEmbeddedHost& host, const std::string& port, std::vector<KernelHostRecord>& records,
                         uint32_t& written) {
    uint32_t required = 0u;
    written = 0u;
    if (records.empty()) {
        records.assign(ase::kernel::HostRecordsMax, KernelHostRecord{});
    }
    HostStatus status = host.snapshot(port.c_str(), records.data(), static_cast<uint32_t>(records.size()), &written,
                                      &required);
    if (status == ase::kernel::HostStatusCapacity) {
        records.assign(required, KernelHostRecord{});
        status = host.snapshot(port.c_str(), records.data(), static_cast<uint32_t>(records.size()), &written,
                               &required);
    }
    return status;
}

/**
 * One port into its held read: a port the config does not name or the plugin does not offer yet leaves the held read
 * as it is and reads as Ok; a changed read replaces the held one and moves its generation.
 */
template <typename View, typename ViewOf, typename SameAs>
HostStatus read_port(KernelEmbeddedHost& host, const std::string& port, std::vector<KernelHostRecord>& records,
                     std::vector<View>& held, uint64_t& generation, ViewOf view_of, SameAs same_as) {
    if (port.empty() || !host.has_port(port.c_str())) {
        return ase::kernel::HostStatusOk;
    }
    uint32_t written = 0u;
    const HostStatus status = snapshot_port(host, port, records, written);
    if (status != ase::kernel::HostStatusOk) {
        return status;
    }
    std::vector<View> read;
    read.reserve(written);
    for (uint32_t i = 0u; i < written; ++i) {
        read.push_back(view_of(records[i]));
    }
    const bool changed = !same_as(read, held);
    held = std::move(read);
    if (changed) {
        generation += 1u;
    }
    return ase::kernel::HostStatusOk;
}

/** Whether `value` is a finite number above zero. */
bool positive(double value) {
    return std::isfinite(value) && value > 0.0;
}

}  // namespace

VivariumLakeView lake_view_of(const KernelHostRecord& record) {
    constexpr const char* KEY_CX = "lake_cx";        // SPT_AABB_CX: centre of the box, east
    constexpr const char* KEY_CY = "lake_cy";        // SPT_AABB_CY: centre of the box, north
    constexpr const char* KEY_CZ = "lake_cz";        // SPT_AABB_CZ: centre of the box, height
    constexpr const char* KEY_HX = "lake_hx";        // SPT_AABB_HX: half extent east
    constexpr const char* KEY_HY = "lake_hy";        // SPT_AABB_HY: half extent north
    constexpr const char* KEY_HZ = "lake_hz";        // SPT_AABB_HZ: half the span between floor and surface
    constexpr const char* KEY_DEPTH = "lake_depth_m";  // GIS_HYD_DEP: depth of the water, metres
    constexpr const char* KEY_KIND = "lake_kind";    // the code of the body, lake or pond
    VivariumLakeView out;
    out.object_id = record.object_id;
    out.cx = record_value(record, KEY_CX);
    out.cy = record_value(record, KEY_CY);
    out.cz = record_value(record, KEY_CZ);
    out.hx = record_value(record, KEY_HX);
    out.hy = record_value(record, KEY_HY);
    out.hz = record_value(record, KEY_HZ);
    out.depth_m = record_value(record, KEY_DEPTH);
    out.kind = record_value(record, KEY_KIND);
    return out;
}

VivariumSeaView sea_view_of(const KernelHostRecord& record) {
    constexpr const char* KEY_CHUNK_X = "sea_chunk_x";  // chunk grid coordinate east
    constexpr const char* KEY_CHUNK_Z = "sea_chunk_z";  // chunk grid coordinate north
    constexpr const char* KEY_CHUNK_M = "sea_chunk_m";  // edge of the chunk, metres
    constexpr const char* KEY_LOW = "sea_low_m";        // lowest ground of the chunk, metres above NN0
    VivariumSeaView out;
    out.object_id = record.object_id;
    out.chunk_x = record_value(record, KEY_CHUNK_X);
    out.chunk_z = record_value(record, KEY_CHUNK_Z);
    out.chunk_m = record_value(record, KEY_CHUNK_M);
    out.low_m = record_value(record, KEY_LOW);
    return out;
}

VivariumGlacierView glacier_view_of(const KernelHostRecord& record) {
    constexpr const char* KEY_CHUNK_X = "glcr_chunk_x";   // chunk grid coordinate east
    constexpr const char* KEY_CHUNK_Z = "glcr_chunk_z";   // chunk grid coordinate north
    constexpr const char* KEY_CHUNK_M = "glcr_chunk_m";   // edge of the chunk, metres
    constexpr const char* KEY_ICE = "glcr_ice_mm";        // ice as millimetres of water column
    constexpr const char* KEY_MELT = "glcr_melt_mm_h";    // melt as millimetres of water column per hour
    VivariumGlacierView out;
    out.object_id = record.object_id;
    out.chunk_x = record_value(record, KEY_CHUNK_X);
    out.chunk_z = record_value(record, KEY_CHUNK_Z);
    out.chunk_m = record_value(record, KEY_CHUNK_M);
    out.ice_mm = record_value(record, KEY_ICE);
    out.melt_mm_h = record_value(record, KEY_MELT);
    return out;
}

bool lake_drawable(const VivariumLakeView& lake) {
    return std::isfinite(lake.cx) && std::isfinite(lake.cy) && std::isfinite(lake.cz) && positive(lake.hx) &&
           positive(lake.hy) && positive(lake.hz) && positive(lake.depth_m);
}

bool sea_drawable(const VivariumSeaView& sea) {
    return std::isfinite(sea.chunk_x) && std::isfinite(sea.chunk_z) && positive(sea.chunk_m) &&
           std::isfinite(sea.low_m);
}

bool glacier_drawable(const VivariumGlacierView& glacier) {
    return std::isfinite(glacier.chunk_x) && std::isfinite(glacier.chunk_z) && positive(glacier.chunk_m) &&
           positive(glacier.ice_mm) && std::isfinite(glacier.melt_mm_h);
}

bool same_lakes(const std::vector<VivariumLakeView>& read, const std::vector<VivariumLakeView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        if (read[i].object_id != held[i].object_id || !same_value(read[i].cx, held[i].cx) ||
            !same_value(read[i].cy, held[i].cy) || !same_value(read[i].cz, held[i].cz) ||
            !same_value(read[i].hx, held[i].hx) || !same_value(read[i].hy, held[i].hy) ||
            !same_value(read[i].hz, held[i].hz) || !same_value(read[i].depth_m, held[i].depth_m) ||
            !same_value(read[i].kind, held[i].kind)) {
            return false;
        }
    }
    return true;
}

bool same_seas(const std::vector<VivariumSeaView>& read, const std::vector<VivariumSeaView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        if (read[i].object_id != held[i].object_id || !same_value(read[i].chunk_x, held[i].chunk_x) ||
            !same_value(read[i].chunk_z, held[i].chunk_z) || !same_value(read[i].chunk_m, held[i].chunk_m) ||
            !same_value(read[i].low_m, held[i].low_m)) {
            return false;
        }
    }
    return true;
}

bool same_glaciers(const std::vector<VivariumGlacierView>& read, const std::vector<VivariumGlacierView>& held) {
    if (read.size() != held.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < read.size(); ++i) {
        if (read[i].object_id != held[i].object_id || !same_value(read[i].chunk_x, held[i].chunk_x) ||
            !same_value(read[i].chunk_z, held[i].chunk_z) || !same_value(read[i].chunk_m, held[i].chunk_m) ||
            !same_value(read[i].ice_mm, held[i].ice_mm) || !same_value(read[i].melt_mm_h, held[i].melt_mm_h)) {
            return false;
        }
    }
    return true;
}

HostStatus GodotWaterKindsReader::read(KernelEmbeddedHost& host, const VivariumWaterKindsPorts& ports) {
    // THE THREE KINDS OF STANDING AND FROZEN WATER (PLAN_ASE_VIVARIUM_PHASE_10_HYDRO, Geometrie-Schicht): each port
    // is read once its plugin offers it, as the river current; until then the view holds none of it and draws no
    // water of that kind.
    const HostStatus lake = read_port(host, ports.lake, records_, lakes_, lake_generation_, lake_view_of, same_lakes);
    if (lake != ase::kernel::HostStatusOk) {
        return lake;
    }
    const HostStatus sea = read_port(host, ports.sea, records_, seas_, sea_generation_, sea_view_of, same_seas);
    if (sea != ase::kernel::HostStatusOk) {
        return sea;
    }
    return read_port(host, ports.glacier, records_, glaciers_, glacier_generation_, glacier_view_of, same_glaciers);
}

void GodotWaterKindsReader::clear() {
    lakes_.clear();
    lake_generation_ += 1u;
    seas_.clear();
    sea_generation_ += 1u;
    glaciers_.clear();
    glacier_generation_ += 1u;
}

const std::vector<VivariumLakeView>& GodotWaterKindsReader::get_lakes() const {
    return lakes_;
}

const std::vector<VivariumSeaView>& GodotWaterKindsReader::get_seas() const {
    return seas_;
}

const std::vector<VivariumGlacierView>& GodotWaterKindsReader::get_glaciers() const {
    return glaciers_;
}

uint64_t GodotWaterKindsReader::lake_generation() const {
    return lake_generation_;
}

uint64_t GodotWaterKindsReader::sea_generation() const {
    return sea_generation_;
}

uint64_t GodotWaterKindsReader::glacier_generation() const {
    return glacier_generation_;
}

}  // namespace ase::adp::godot

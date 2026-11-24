/**
 * ASE ADAPTER TESTS - THE LAKE, SEA AND GLACIER WATER WITHOUT GODOT
 *
 * @file        test_godot_plant_water_kinds.cpp
 * @design      DSGN_066
 * @brief       Pins the records of the three water kind ports, their change detection and the surfaces they lay
 * @description PLAN_ASE_VIVARIUM_PHASE_10_HYDRO "Geometrie-Schicht: Wasser im Bild": a lake record becomes a quad at the
 *              top face of its box (centre z plus half z, the engine's z is the height), a salt chunk a plane at NN0
 *              over its chunk of the grid, a glacier chunk the ground triangles of its chunk lifted by the ice. A
 *              record that lacks a key stays NaN in the view and lays nothing, a value that never changed is no
 *              change of the read. The positive controls of each case lay the whole record first.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_water_kinds_geom.hpp>
#include <ase/adp/godot/godot_water_kinds_reader.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/utils/strops.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr float kLakeDepthFull = 10.0f;  // scales the tests lay the shares over
constexpr float kSeaDepthFull = 100.0f;
constexpr float kIceFull = 5.0f;
constexpr float kMeltFull = 50.0f;

WaterKindsScale scale() {
    WaterKindsScale s;
    s.lake_depth_m = kLakeDepthFull;
    s.sea_depth_m = kSeaDepthFull;
    s.ice_m = kIceFull;
    s.melt_mm_h = kMeltFull;
    return s;
}

/** A record of object `id` with the given key and value pairs, as the plugin lays it on its port. */
ase::kernel::KernelHostRecord record_of(uint32_t id, const std::vector<std::pair<const char*, double>>& pairs) {
    ase::kernel::KernelHostRecord record;
    record.object_id = id;
    for (const auto& pair : pairs) {
        ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
        ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, pair.first);
        slot.value = pair.second;
    }
    return record;
}

ase::kernel::KernelHostRecord whole_lake() {
    return record_of(11u, {{"lake_cx", 100.0}, {"lake_cy", 50.0}, {"lake_cz", 4.0}, {"lake_hx", 8.0},
                           {"lake_hy", 6.0}, {"lake_hz", 2.0}, {"lake_depth_m", 4.0}, {"lake_kind", 1.0}});
}

ase::kernel::KernelHostRecord whole_sea() {
    return record_of(12u, {{"sea_chunk_x", -2.0}, {"sea_chunk_z", 3.0}, {"sea_chunk_m", 32.0}, {"sea_low_m", -25.0}});
}

ase::kernel::KernelHostRecord whole_glacier() {
    return record_of(13u, {{"glcr_chunk_x", 1.0}, {"glcr_chunk_z", 0.0}, {"glcr_chunk_m", 32.0},
                           {"glcr_ice_mm", 2000.0}, {"glcr_melt_mm_h", 25.0}});
}

/** A flat ground of two triangles per cell over the square [west, west + edge] x [south, south + edge] at height `y`. */
PlantMeshArrays flat_ground(float west, float south, float edge, float y) {
    PlantMeshArrays out;
    const float x[4] = {west, west + edge, west + edge, west};
    const float z[4] = {south, south, south + edge, south + edge};
    for (int c = 0; c < 4; ++c) {
        out.positions.insert(out.positions.end(), {x[c], y, z[c]});
        out.normals.insert(out.normals.end(), {0.0f, 1.0f, 0.0f});
        out.uvs.insert(out.uvs.end(), {x[c], z[c]});
        out.colors.insert(out.colors.end(), {1.0f, 1.0f, 1.0f, 1.0f});
    }
    out.indices.insert(out.indices.end(), {0, 1, 2, 0, 2, 3});
    return out;
}

}  // namespace

TEST_CASE("water kinds records: a whole record reads, a missing key stays NaN and lays nothing") {
    const VivariumLakeView lake = lake_view_of(whole_lake());
    REQUIRE(lake_drawable(lake));  // positive control: the whole record is drawable
    CHECK(lake.object_id == 11u);
    CHECK(lake.cz == doctest::Approx(4.0));
    CHECK(lake.hz == doctest::Approx(2.0));
    ase::kernel::KernelHostRecord short_lake = whole_lake();
    short_lake.values[5u].key[0u] = '\0';  // lake_hz gone
    const VivariumLakeView headless = lake_view_of(short_lake);
    CHECK(std::isnan(headless.hz));
    CHECK_FALSE(lake_drawable(headless));
    CHECK(sea_drawable(sea_view_of(whole_sea())));
    CHECK(glacier_drawable(glacier_view_of(whole_glacier())));
    CHECK_FALSE(sea_drawable(sea_view_of(record_of(1u, {{"sea_chunk_x", 0.0}, {"sea_chunk_z", 0.0}}))));
    CHECK_FALSE(glacier_drawable(glacier_view_of(record_of(1u, {{"glcr_chunk_x", 0.0}, {"glcr_chunk_z", 0.0},
                                                               {"glcr_chunk_m", 32.0}, {"glcr_ice_mm", 0.0},
                                                               {"glcr_melt_mm_h", 0.0}}))));  // no ice is no glacier
}

TEST_CASE("water kinds records: two reads with unanswered values are the same read, a changed value is not") {
    std::vector<VivariumSeaView> held{sea_view_of(record_of(12u, {{"sea_chunk_x", 0.0}}))};
    std::vector<VivariumSeaView> read{sea_view_of(record_of(12u, {{"sea_chunk_x", 0.0}}))};
    CHECK(same_seas(read, held));  // positive control: NaN against NaN is no change
    read[0].chunk_x = 1.0;
    CHECK_FALSE(same_seas(read, held));
    std::vector<VivariumLakeView> lakes{lake_view_of(whole_lake())};
    std::vector<VivariumLakeView> lakes_read{lake_view_of(whole_lake())};
    CHECK(same_lakes(lakes_read, lakes));
    lakes_read[0].depth_m = 5.0;
    CHECK_FALSE(same_lakes(lakes_read, lakes));
    lakes_read.clear();
    CHECK_FALSE(same_lakes(lakes_read, lakes));
    std::vector<VivariumGlacierView> ice{glacier_view_of(whole_glacier())};
    std::vector<VivariumGlacierView> ice_read{glacier_view_of(whole_glacier())};
    CHECK(same_glaciers(ice_read, ice));
    ice_read[0].melt_mm_h = 26.0;
    CHECK_FALSE(same_glaciers(ice_read, ice));
}

TEST_CASE("water kinds geometry: the lake lies at the top face of its box, centre z plus half z") {
    PlantMeshArrays out;
    REQUIRE(water_kinds_lake_surface(lake_view_of(whole_lake()), scale(), out));
    REQUIRE(out.positions.size() == 12u);
    for (std::size_t v = 0u; v < 4u; ++v) {
        CHECK(out.positions[v * 3u + 1u] == doctest::Approx(6.0f));  // 4 + 2
        CHECK(out.colors[v * 4u] == doctest::Approx(0.4f));          // 4 m of 10 m
    }
    CHECK(out.positions[0] == doctest::Approx(92.0f));   // east: 100 - 8
    CHECK(out.positions[2] == doctest::Approx(44.0f));   // north: 50 - 6
    CHECK(out.positions[6] == doctest::Approx(108.0f));  // 100 + 8
    CHECK(out.positions[8] == doctest::Approx(56.0f));   // 50 + 6
    CHECK(out.indices.size() == 6u);
    PlantMeshArrays none;
    ase::kernel::KernelHostRecord short_lake = whole_lake();
    short_lake.values[2u].key[0u] = '\0';  // lake_cz gone: no surface, never a lake at NN0
    CHECK_FALSE(water_kinds_lake_surface(lake_view_of(short_lake), scale(), none));
    CHECK(none.positions.empty());
}

TEST_CASE("water kinds geometry: the sea plane lies at the datum over its chunk of the grid") {
    PlantMeshArrays out;
    REQUIRE(water_kinds_sea_plane(sea_view_of(whole_sea()), 0.0f, scale(), out));
    REQUIRE(out.positions.size() == 12u);
    CHECK(out.positions[0] == doctest::Approx(-64.0f));  // chunk -2 of 32 m
    CHECK(out.positions[2] == doctest::Approx(96.0f));   // chunk 3 of 32 m
    CHECK(out.positions[6] == doctest::Approx(-32.0f));
    for (std::size_t v = 0u; v < 4u; ++v) {
        CHECK(out.positions[v * 3u + 1u] == doctest::Approx(0.0f));
        CHECK(out.colors[v * 4u] == doctest::Approx(0.25f));  // 25 m under NN0 of 100 m
    }
    PlantMeshArrays none;
    CHECK_FALSE(water_kinds_sea_plane(sea_view_of(record_of(1u, {{"sea_chunk_x", 0.0}, {"sea_chunk_z", 0.0},
                                                                 {"sea_chunk_m", 32.0}})), 0.0f, scale(), none));
    CHECK(none.positions.empty());  // no lowest ground: no sea
}

TEST_CASE("water kinds geometry: the ice lifts the ground triangles of its chunk by the thickness") {
    const PlantMeshArrays ground = flat_ground(32.0f, 0.0f, 32.0f, 10.0f);  // chunk (1, 0)
    PlantMeshArrays out;
    REQUIRE(water_kinds_glacier_cap(glacier_view_of(whole_glacier()), ground, scale(), out) == 2u);
    REQUIRE(out.positions.size() == 18u);
    for (std::size_t v = 0u; v < 6u; ++v) {
        CHECK(out.positions[v * 3u + 1u] == doctest::Approx(12.0f));  // 10 m ground + 2000 mm of water column
        CHECK(out.colors[v * 4u] == doctest::Approx(0.4f));           // 2 m of 5 m
        CHECK(out.colors[v * 4u + 1u] == doctest::Approx(0.5f));      // 25 of 50 mm/h
    }
    PlantMeshArrays elsewhere;
    const PlantMeshArrays other = flat_ground(0.0f, 0.0f, 32.0f, 10.0f);  // chunk (0, 0): no ice there
    CHECK(water_kinds_glacier_cap(glacier_view_of(whole_glacier()), other, scale(), elsewhere) == 0u);
    CHECK(elsewhere.positions.empty());
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - GODOT HOST LEVEL OF A PLANT BODY
 *
 * @file        test_godot_host_lod.cpp
 * @design      DSGN_018
 * @brief       Pins the graded level the level port hands to the view for the plant bodies of the start patches
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen (usecase:101657 grade plants by picture height):
 *              ase-render-data grades every plant body by the height of its picture (RND_LOD_LEVEL, RND_LOD_FADE),
 *              ase-pl-flora files one record per graded patch on flora.lod.v1, and GodotHostResourceManager keeps it
 *              per patch (get_lods). The case boots the shared start configuration and waits until the port carries
 *              a record: every record names a patch of the patch port, its level is one of the six codes, its fade
 *              lies between none and full, and its view, tile and canopy codes are 3, 4 and 5.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside the others of the bundle.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101657 - grade plants by picture height
 * @actor       Picture Height Grader (PICTHGTGRA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/lod/godot_host_lod_view.hpp>

#include <cstdint>
#include <unordered_map>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::environment;

namespace {

constexpr uint32_t kBoundFrames = 256u;  // frames the case waits for the first level record at most
constexpr double kLevelFull = 0.0;       // RND_CONST_LOD_LEVEL_FULL, the finest code
constexpr double kLevelView = 3.0;       // RND_CONST_LOD_LEVEL_VIEW
constexpr double kLevelTile = 4.0;       // RND_CONST_LOD_LEVEL_TILE
constexpr double kLevelCnpy = 5.0;       // RND_CONST_LOD_LEVEL_CNPY, the coarsest code
constexpr double kFadeNone = 0.0;        // RND_CONST_LOD_FADE_NONE
constexpr double kFadeFull = 1.0;        // RND_CONST_LOD_FADE_FULL

}  // namespace

TEST_CASE("usecase:101657: the level port carries the graded level of the plant bodies of the start patches") {
    // the shared start configuration as it stands
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    advance_frames(resources, 8u);
    for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_lods().empty(); ++frame) {
        advance_frames(resources, 1u);
    }
    const std::unordered_map<uint32_t, VivariumLodView>& lods = resources.get_lods();
    REQUIRE_FALSE(lods.empty());
    for (const auto& graded : lods) {
        CHECK(resources.get_patch(graded.first) != nullptr);
        CHECK(graded.second.level >= kLevelFull);
        CHECK(graded.second.level <= kLevelCnpy);
        CHECK(graded.second.fade >= kFadeNone);
        CHECK(graded.second.fade <= kFadeFull);
        CHECK(graded.second.view == doctest::Approx(kLevelView));
        CHECK(graded.second.tile == doctest::Approx(kLevelTile));
        CHECK(graded.second.cnpy == doctest::Approx(kLevelCnpy));
    }
}

}  // namespace ase::adp::godot

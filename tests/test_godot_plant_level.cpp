/**
 * ASE ADAPTER TESTS - SHARE OF BODY AND CARD BY THE GRADED LEVEL WITHOUT GODOT
 *
 * @file        test_godot_plant_level.cpp
 * @design      DSGN_018
 * @brief       Pins how much of a plant level_share gives its drawn body and its view card for every graded level
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen (usecase:101657 grade plants by picture height):
 *              ase-render-data grades a plant body to one of the codes FULL 0, CORS 1, CRWN 2, VIEW 3, TILE 4 and
 *              CNPY 5 (RND_CONST_LOD_LEVEL_*) and hands a share of it to the next coarser level in the lowest tenth
 *              of each band (RND_LOD_FADE). The cases pin: no record draws the body whole, a level at or above the
 *              view code stands the card in whole, the level just below hands its fade over to the card, and the
 *              finer levels draw the body whole whatever their fade. Body and card always add up to one.
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

#include <doctest/doctest.h>

#include <ase/adp/godot/lod/godot_host_lod_view.hpp>

namespace ase::adp::godot {

namespace {

constexpr double kFull = 0.0;   // RND_CONST_LOD_LEVEL_FULL
constexpr double kCrwn = 2.0;   // RND_CONST_LOD_LEVEL_CRWN, the level just below the view card
constexpr double kView = 3.0;   // RND_CONST_LOD_LEVEL_VIEW
constexpr double kTile = 4.0;   // RND_CONST_LOD_LEVEL_TILE
constexpr double kCnpy = 5.0;   // RND_CONST_LOD_LEVEL_CNPY
constexpr double kHalf = 0.5;   // a fade halfway through the dither band

/** The record of one graded body with the codes ase-render-data files beside its level. */
VivariumLodView graded(double level, double fade) {
    VivariumLodView lod;
    lod.level = level;
    lod.fade = fade;
    lod.view = kView;
    lod.tile = kTile;
    lod.cnpy = kCnpy;
    return lod;
}

}  // namespace

TEST_CASE("usecase:101657: a body without a level record is not graded and draws whole") {
    const PlantLevelShare share = level_share(nullptr);
    CHECK(share.body == doctest::Approx(1.0f));
    CHECK(share.card == doctest::Approx(0.0f));
}

TEST_CASE("usecase:101657: the view level and every coarser one stand the card in whole") {
    const double coarse[] = {kView, kTile, kCnpy};
    for (const double level : coarse) {
        const VivariumLodView lod = graded(level, kHalf);
        const PlantLevelShare share = level_share(&lod);
        CHECK(share.body == doctest::Approx(0.0f));
        CHECK(share.card == doctest::Approx(1.0f));
    }
}

TEST_CASE("usecase:101657: the level below the view hands its fade over to the card") {
    const double fades[] = {0.0, 0.25, kHalf, 1.0};
    for (const double fade : fades) {
        const VivariumLodView lod = graded(kCrwn, fade);
        const PlantLevelShare share = level_share(&lod);
        CHECK(share.card == doctest::Approx(static_cast<float>(fade)));
        CHECK(share.body + share.card == doctest::Approx(1.0f));
    }
}

TEST_CASE("usecase:101657: the finer levels draw the body whole whatever their fade") {
    const double fine[] = {kFull, kFull + 1.0};
    for (const double level : fine) {
        const VivariumLodView lod = graded(level, kHalf);
        const PlantLevelShare share = level_share(&lod);
        CHECK(share.body == doctest::Approx(1.0f));
        CHECK(share.card == doctest::Approx(0.0f));
    }
}

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - SHARE OF THE FOUR FORMS OF A PLANT BY THE GRADED LEVEL WITHOUT GODOT
 *
 * @file        test_godot_plant_level.cpp
 * @design      DSGN_018
 * @brief       Pins how much of a plant level_share gives its drawn body, its view card, its billboard and the canopy for every graded level
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen (usecase:101657 grade plants by picture height):
 *              ase-render-data grades a plant body to one of the codes FULL 0, CORS 1, CRWN 2, VIEW 3, TILE 4 and
 *              CNPY 5 (RND_CONST_LOD_LEVEL_*) and hands a share of it to the next coarser level in the lowest tenth
 *              of each band (RND_LOD_FADE). The cases pin: no record draws the body whole; the finer levels draw the
 *              body whole whatever their fade; the level just below the view hands its fade from the body to the
 *              card; the view level hands it from the card to the billboard; the tile level hands it from the
 *              billboard to the canopy; the canopy level is canopy whole. The four shares always add up to one.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101657 - grade plants by picture height
 * @actor       Picture Height Grader (PICTHGTGRA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.02.00002 [core]
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

/** The sum of the four shares: one for every level and fade. */
float total(const PlantLevelShare& share) {
    return share.body + share.card + share.tile + share.canopy;
}

}  // namespace

TEST_CASE("usecase:101657: a body without a level record is not graded and draws whole") {
    const PlantLevelShare share = level_share(nullptr);
    CHECK(share.body == doctest::Approx(1.0f));
    CHECK(share.card == doctest::Approx(0.0f));
    CHECK(share.tile == doctest::Approx(0.0f));
    CHECK(share.canopy == doctest::Approx(0.0f));
}

TEST_CASE("usecase:101657: the finer levels draw the body whole whatever their fade") {
    const double fine[] = {kFull, kFull + 1.0};
    for (const double level : fine) {
        const VivariumLodView lod = graded(level, kHalf);
        const PlantLevelShare share = level_share(&lod);
        CHECK(share.body == doctest::Approx(1.0f));
        CHECK(share.card == doctest::Approx(0.0f));
        CHECK(share.tile == doctest::Approx(0.0f));
        CHECK(share.canopy == doctest::Approx(0.0f));
    }
}

TEST_CASE("usecase:101657: the level below the view hands its fade from the body to the card") {
    const double fades[] = {0.0, 0.25, kHalf, 1.0};
    for (const double fade : fades) {
        const VivariumLodView lod = graded(kCrwn, fade);
        const PlantLevelShare share = level_share(&lod);
        CHECK(share.body == doctest::Approx(1.0f - static_cast<float>(fade)));
        CHECK(share.card == doctest::Approx(static_cast<float>(fade)));
        CHECK(share.tile == doctest::Approx(0.0f));
        CHECK(share.canopy == doctest::Approx(0.0f));
        CHECK(total(share) == doctest::Approx(1.0f));
    }
}

TEST_CASE("usecase:101657: the view level hands its fade from the card to the billboard") {
    const double fades[] = {0.0, 0.25, kHalf, 1.0};
    for (const double fade : fades) {
        const VivariumLodView lod = graded(kView, fade);
        const PlantLevelShare share = level_share(&lod);
        CHECK(share.body == doctest::Approx(0.0f));
        CHECK(share.card == doctest::Approx(1.0f - static_cast<float>(fade)));
        CHECK(share.tile == doctest::Approx(static_cast<float>(fade)));
        CHECK(share.canopy == doctest::Approx(0.0f));
        CHECK(total(share) == doctest::Approx(1.0f));
    }
}

TEST_CASE("usecase:101657: the tile level hands its fade from the billboard to the canopy") {
    const double fades[] = {0.0, 0.25, kHalf, 1.0};
    for (const double fade : fades) {
        const VivariumLodView lod = graded(kTile, fade);
        const PlantLevelShare share = level_share(&lod);
        CHECK(share.body == doctest::Approx(0.0f));
        CHECK(share.card == doctest::Approx(0.0f));
        CHECK(share.tile == doctest::Approx(1.0f - static_cast<float>(fade)));
        CHECK(share.canopy == doctest::Approx(static_cast<float>(fade)));
        CHECK(total(share) == doctest::Approx(1.0f));
    }
}

TEST_CASE("usecase:101657: the canopy level is canopy whole, no form of the individual is drawn") {
    const VivariumLodView lod = graded(kCnpy, kHalf);
    const PlantLevelShare share = level_share(&lod);
    CHECK(share.body == doctest::Approx(0.0f));
    CHECK(share.card == doctest::Approx(0.0f));
    CHECK(share.tile == doctest::Approx(0.0f));
    CHECK(share.canopy == doctest::Approx(1.0f));
}

TEST_CASE("usecase:100387: a patch without a canopy record is covered by nothing") {
    CHECK(canopy_cover(nullptr) == doctest::Approx(0.0f));
}

TEST_CASE("usecase:100387: the canopy covers by Beer-Lambert, 1 - exp(-ext * lai)") {
    VivariumCnpyView cnpy;
    cnpy.ext = 0.5;   // PLT_CONST_CNPY_LIGHT_EXT of hub_constants.json
    cnpy.lai = 4.0;   // a closed birch stand
    cnpy.height = 18.5;
    // 1 - exp(-2) = 0.8646647
    CHECK(canopy_cover(&cnpy) == doctest::Approx(0.8646647f).epsilon(1e-5));
    cnpy.lai = 0.0;   // a cell without leaves covers nothing
    CHECK(canopy_cover(&cnpy) == doctest::Approx(0.0f));
}

TEST_CASE("usecase:100387: a denser canopy covers more and never more than the whole ground") {
    VivariumCnpyView thin;
    thin.ext = 0.5;
    thin.lai = 1.0;
    VivariumCnpyView dense = thin;
    dense.lai = 8.0;
    CHECK(canopy_cover(&thin) < canopy_cover(&dense));
    CHECK(canopy_cover(&dense) <= 1.0f);
    VivariumCnpyView negative = thin;
    negative.lai = -1.0;  // a product below zero would cover less than nothing; the cover stays at zero
    CHECK(canopy_cover(&negative) == doctest::Approx(0.0f));
}

TEST_CASE("usecase:101657: the billboard slots fill a page of 128 x 64 tiles row by row before the next page") {
    struct Case {
        uint32_t index;
        uint32_t page;
        uint32_t column;
        uint32_t row;
    };
    const Case cases[] = {
        {0u, 0u, 0u, 0u},       // the first slot at the top left of the first page
        {127u, 0u, 127u, 0u},   // the last tile of the first row
        {128u, 0u, 0u, 1u},     // the next row begins at the left edge
        {8191u, 0u, 127u, 63u}, // the last tile of the first page
        {8192u, 1u, 0u, 0u},    // the second page begins
        {10000u, 1u, 16u, 14u}, // the last of 10 001 birches: 1808 slots into the second page, row 14, column 16
    };
    for (const Case& c : cases) {
        const AtlasSlot slot = billboard_slot(c.index);
        CHECK(slot.page == c.page);
        CHECK(slot.column == c.column);
        CHECK(slot.row == c.row);
    }
}

TEST_CASE("usecase:101657: 10000 billboards stand on two atlas pages, never one target per plant") {
    // V9 of PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN: 10 000 birches on the Pixel 8; the slots 0..9999 reach two pages only
    CHECK(billboard_slot(0u).page == 0u);
    CHECK(billboard_slot(9999u).page == 1u);
}

TEST_CASE("usecase:101657: the view card slots fill a page of 64 x 32 tiles of 32 x 64 pixels row by row") {
    struct Case {
        uint32_t index;
        uint32_t page;
        uint32_t column;
        uint32_t row;
    };
    const Case cases[] = {
        {0u, 0u, 0u, 0u},       // the first slot at the top left of the first page
        {63u, 0u, 63u, 0u},     // the last tile of the first row: 64 tiles of 32 pixels fill 2048
        {64u, 0u, 0u, 1u},      // the next row begins at the left edge
        {2047u, 0u, 63u, 31u},  // the last tile of the first page: 32 rows of 64 pixels fill 2048
        {2048u, 1u, 0u, 0u},    // the second page begins
        {9999u, 4u, 15u, 28u},  // the last of 10 000 birches: page 4, 1807 slots into it, row 28, column 15
    };
    for (const Case& c : cases) {
        const AtlasSlot slot = view_slot(c.index);
        CHECK(slot.page == c.page);
        CHECK(slot.column == c.column);
        CHECK(slot.row == c.row);
    }
    // the same slot stands on a billboard page and on a view page, each by its own tile size
    CHECK(view_slot(128u).row == 2u);
    CHECK(billboard_slot(128u).row == 1u);
}

TEST_CASE("usecase:101657: the view tile is drawn again when the bearing to the observer turns past a step") {
    // straight in front (+z) is step 0, a tenth of a step to the side stays in it
    CHECK(view_step(0.0f, 400.0f) == 0);
    CHECK(view_step(400.0f * 0.0174f * 0.1f, 400.0f) == 0);
    // the plan's walk: at 400 m the bearing turns 10 degrees when the observer walks 70 m across, one step further
    CHECK(view_step(70.0f, 400.0f) == 0);
    CHECK(view_step(71.0f, 400.0f) == 1);
    // to the other side the step below zero begins at once, so both sides have steps of the same width
    CHECK(view_step(-1.0f, 400.0f) == -1);
    // a quarter turn to +x is nine steps of 10 degrees
    CHECK(view_step(400.0f, 0.0f) == 9);
    // the step does not depend on the distance, only on the bearing
    CHECK(view_step(71.0f, 400.0f) == view_step(710.0f, 4000.0f));
}

}  // namespace ase::adp::godot

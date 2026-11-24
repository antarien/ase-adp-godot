/**
 * ASE GODOT ADAPTER - SHARE OF THE FOUR FORMS OF A PLANT BY THE GRADED LEVEL
 *
 * @file        godot_plant_level.cpp
 * @brief       level_share: how much of a plant its drawn body, its view card, its billboard and the canopy show, by the graded level
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen (usecase:101657 grade plants by picture height):
 *              the level of a plant body comes from ase-render-data through flora.lod.v1 (godot_host_lod_view.hpp).
 *              Below the view level the body is drawn (LOD 0 to 2), at the view level the view card stands in
 *              (LOD 3), at the tile level the billboard tile drawn from the card (LOD 4), and from the canopy level on
 *              the individual is drawn as part of the canopy of the whole (LOD 5). A level whose next coarser level
 *              draws another form hands its fade over to that form, the dither band of ase-render-data; every other
 *              level draws its own form whole, and an ungraded body draws the body whole. No number of the view's own
 *              decides: the level, the fade and the three codes are the record's.
 *
 *              NO GODOT TYPE: the host tests check it without a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
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

#include <ase/adp/godot/lod/godot_host_lod_view.hpp>
#include <ase/adp/godot/lod/types.hpp>

namespace ase::adp::godot {

namespace {

// the share slot of the form the level `level` draws: the body below the view code, the card below the tile code,
// the billboard below the canopy code, the canopy from it on
float* form_of(PlantLevelShare& share, double level, const VivariumLodView& lod) {
    if (level < lod.view) {
        return &share.body;
    }
    if (level < lod.tile) {
        return &share.card;
    }
    if (level < lod.cnpy) {
        return &share.tile;
    }
    return &share.canopy;
}

}  // namespace

PlantLevelShare level_share(const VivariumLodView* lod) {
    PlantLevelShare share;
    if (lod == nullptr) {
        return share;
    }
    share.body = 0.0f;
    float* own = form_of(share, lod->level, *lod);
    float* coarser = form_of(share, lod->level + GODOT_LOD_CODE_STEP, *lod);
    *own = 1.0f;
    if (coarser != own) {
        // the fade stands between RND_CONST_LOD_FADE_NONE and RND_CONST_LOD_FADE_FULL, clamped by the selector of
        // ase-render-data (fade_toward_coarser), and the reader refused every record without a finite one
        const float handed = static_cast<float>(lod->fade);
        *coarser = handed;
        *own = *own - handed;
    }
    return share;
}

}  // namespace ase::adp::godot

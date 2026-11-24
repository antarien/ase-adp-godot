/**
 * ASE GODOT ADAPTER - SHARE OF BODY AND CARD BY THE GRADED LEVEL
 *
 * @file        godot_plant_level.cpp
 * @brief       level_share: how much of a plant its drawn body and how much its view card show, by the graded level
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen (usecase:101657 grade plants by picture height):
 *              the level of a plant body comes from ase-render-data through flora.lod.v1 (godot_host_lod_view.hpp).
 *              At and above the view level the card stands in; the level just below it hands its fade over to the
 *              card, the dither band of ase-render-data; every finer level and an ungraded body draw the body whole.
 *              No number of the view's own decides: the level, the fade and the view code are the record's.
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
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/lod/godot_host_lod_view.hpp>
#include <ase/adp/godot/lod/types.hpp>

namespace ase::adp::godot {

PlantLevelShare level_share(const VivariumLodView* lod) {
    PlantLevelShare share;
    if (lod == nullptr) {
        return share;
    }
    if (lod->level >= lod->view) {
        share.body = 0.0f;
        share.card = 1.0f;
        return share;
    }
    if (lod->level + GODOT_LOD_CODE_STEP >= lod->view) {
        // the fade stands between RND_CONST_LOD_FADE_NONE and RND_CONST_LOD_FADE_FULL, clamped by the selector of
        // ase-render-data (fade_toward_coarser), and the reader refused every record without a finite one
        const float handed = static_cast<float>(lod->fade);
        share.card = handed;
        share.body = share.body - handed;
    }
    return share;
}

}  // namespace ase::adp::godot

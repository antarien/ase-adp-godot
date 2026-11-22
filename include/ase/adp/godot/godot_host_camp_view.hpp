#pragma once

/**
 * ASE GODOT ADAPTER - HOST CAMP VIEW
 *
 * @file        godot_host_camp_view.hpp
 * @brief       Every camp part body and every keeper laying wood as the last snapshot of ase-pl-ingle showed them
 * @description The copies GodotHostResourceManager keeps of the camp fire (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.3
 *              and T.7, usecase:5436 - kindle fire): one record per part body ase-physics holds on ingle.body.v1,
 *              and one record per keeper whose action ase-bdi sets to laying wood on ingle.refuel.v1. Only the
 *              units that draw the camp and lay its wood read them (godot_vivarium_camp.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    ecs/module
 * @usecase     usecase:5436 - kindle fire
 * @actor       Funken (FNKN)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * One camp part body as the last snapshot of ingle.body.v1 showed it - a copy. ase-physics drops the part onto
 * the height field, tips it and lets it rest; IngleHostBodyExptSystem lays its place patch-local (origin at the
 * stem foot, x east, y up, z north) and its turn as a rotation vector from the frame its hull was exported in.
 * The view draws the make there and computes none of it.
 */
struct VivariumCampBodyView {
    uint32_t object_id = 0;  // the body, the entity ase-physics keeps
    uint32_t patch = 0;      // the patch whose frame the body lies in (HUB_PHYS_BODY_OWNER)
    uint32_t make = 0;       // name id of the recipe variant the body is made from
    double   x = 0.0;        // metres east of the stem foot
    double   y = 0.0;        // metres above the stem foot
    double   z = 0.0;        // metres north of the stem foot
    double   tx = 0.0;       // rotation vector, east component, radians
    double   ty = 0.0;       // rotation vector, up component, radians
    double   tz = 0.0;       // rotation vector, north component, radians
};

/**
 * One keeper laying wood this pass as the last snapshot of ingle.refuel.v1 showed him - a copy. ase-fire never
 * adds wood to a burning cell; the view lays a new log as a part of its own on the body he tends.
 */
struct VivariumCampRefuelView {
    uint32_t keeper = 0;     // the keeper, object id of the record
    uint32_t body = 0;       // the fuel body his action targets (BDI_ACTION_TARGET)
    bool     celled = false; // ase-fire names a fire cell under that body
    uint32_t cell = 0;       // that fire cell, valid while celled
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - HOST ARC VIEW
 *
 * @file        godot_host_arc_view.hpp
 * @brief       The sky arc of the embedded host as the last snapshot showed it: shares of now, occurrences, codes
 * @description The copy GodotHostResourceManager keeps of flora.arc.v1 (VivariumArcView, one record, read every
 *              frame), of flora.almanac.v1 (VivariumAlmanacView, one record per occurrence, read again only when the
 *              number of the set of the arc differs from the held one, so anchor and occurrences never come from two
 *              states) and of flora.event_code.v1 (VivariumEventCodeView, the codes of the kinds and rings, two
 *              records merged by key). Every number is the owner's: ase-calendar places the anchor and the shares, the
 *              adapter holds no number of its own and compares only against these codes. No Godot type, so the host
 *              tests of the arc run without a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <vector>

namespace ase::adp::godot {

/** flora.arc.v1: the shares of now and the next event, as the last frame showed them. */
struct VivariumArcView {
    bool   known = false;      // false until a snapshot carried the arc record
    double day_now = 0.0;      // share of now against the anchor of the day ring, in half windows (CAL_ARC_DAY_NOW)
    double lun_now = 0.0;      // share of now against the anchor of the month ring (CAL_ARC_LUN_NOW)
    double year_now = 0.0;     // share of now against the anchor of the year ring (CAL_ARC_YEAR_NOW)
    double seq = 0.0;          // number of the set the anchors and the occurrences belong to (CAL_ARC_SEQ)
    double next_kind = 0.0;    // code of the kind of the next event, the code evnt_none while none (CAL_ARC_NEXT_KIND)
    double next_body = 0.0;    // body of the next event (CAL_ARC_NEXT_BODY)
    double next_in_h = 0.0;    // game hours until it, below 0 while none (CAL_ARC_NEXT_IN_H)
    double next_in_d = 0.0;    // whole game days until it, below 0 while none (CAL_ARC_NEXT_IN_D)
};

/** One occurrence of flora.almanac.v1, its place on a ring as the owner placed it. */
struct VivariumOccurrenceView {
    double kind = 0.0;   // code of its kind (VivariumEventCodeView)
    double ring = 0.0;   // code of its ring (VivariumEventCodeView)
    double body = 0.0;   // which one of its kind: the moon, the conjunction, the season, the period, the phase
    double share = 0.0;  // its place on the ring as a share of the window against the anchor of the set
    double span = 0.0;   // its length as a share of the window, 0 for a point
};

/** flora.almanac.v1: the occurrences of one set. */
struct VivariumAlmanacView {
    bool   known = false;  // false until a complete set of the arc's current number was read
    double seq = 0.0;      // the number of the set held
    std::vector<VivariumOccurrenceView> occurrences;  // every occurrence of the set, in the order of the port
};

/** flora.event_code.v1: the codes the calendar numbers its kinds and rings with. */
struct VivariumEventCodeView {
    bool   known = false;       // false until both records carried every code
    double none = 0.0;          // evnt_none: the code of "no event"
    double kind_count = 0.0;    // evnt_kind_count: how many kinds the register holds
    double hunt = 0.0;          // evnt_hunt
    double season = 0.0;        // evnt_season
    double vectum = 0.0;        // evnt_vectum
    double stage = 0.0;         // evnt_stage
    double period = 0.0;        // evnt_period
    double dawn = 0.0;          // evnt_dawn
    double dusk = 0.0;          // evnt_dusk
    double moonrise = 0.0;      // evnt_moonrise
    double fullmoon = 0.0;      // evnt_fullmoon
    double eclipse = 0.0;       // evnt_eclipse
    double conj = 0.0;          // evnt_conj
    double conj_great = 0.0;    // evnt_conj_great
    double antares = 0.0;       // evnt_antares
    double arcturus = 0.0;      // evnt_arcturus
    double tumbrae = 0.0;       // evnt_tumbrae
    double lunbrex = 0.0;       // evnt_lunbrex
    double ring_day = 0.0;      // ring_day
    double ring_lun = 0.0;      // ring_lun
    double ring_year = 0.0;     // ring_year
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - THE PORTS THE WATER WAS LAID FROM
 *
 * @file        godot_vivarium_water_marks.hpp
 * @brief       The generations of the three ports the water of the scene was last laid from
 * @description The water of the care view is laid by fill_plant_water (godot_plant_brook.hpp) out of three ports:
 *              the brooks ase-hydro measures (water.stream.v1), the pits it fills (hydrogeology.pit.v1, D2506) and
 *              the current cells with what their water carries (water.current.v1 joined with water.quality.v1, the
 *              hue ase-signature files per cell). Each port moves its generation in the resource manager only when
 *              a record changed; the view lays the water anew when one of the three moved or the ground under it
 *              did, never once per frame (godot_vivarium_scene.cpp), and the start screen counts the brooks as built
 *              once the stream generation is the one laid (godot_vivarium_clock.cpp).
 *
 *              It stands apart from the view because it is one responsibility with its own life: written in the one
 *              pass that lays the water, read by that pass and by the count of the built scene parts. The view
 *              holds one of it.
 *
 *              NO GODOT TYPE: plain values only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.23.00023 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/** The generation of each water port the scene's water was last laid from; 0 before the first lay. */
struct VivariumWaterMarks {
    uint64_t streams = 0;   // stream generation the brook water was last laid from (usecase:100399 carry brook water)
    uint64_t pits = 0;      // pit generation the water of the pits was last laid from (D2506, hydrogeology.pit.v1)
    uint64_t currents = 0;  // current generation the hue of the brook water was last laid from (water.quality.v1, curr_hue_deg)
};

}  // namespace ase::adp::godot

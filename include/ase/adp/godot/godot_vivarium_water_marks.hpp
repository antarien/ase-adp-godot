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
 * @modified    2026-11-24
 * @version     00.00.31.00031 [core]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * The generation of each water port the scene's water was last laid from; 0 before the first lay. The last three
 * belong to the standing and frozen waters of H.13 (fill_plant_water_kinds, godot_plant_water_kinds.hpp), laid in a
 * pass of their own beside the brook water.
 */
struct VivariumWaterMarks {
    uint64_t streams = 0;   // stream generation the brook water was last laid from (usecase:100399 carry brook water)
    uint64_t pits = 0;      // pit generation the water of the pits was last laid from (D2506, hydrogeology.pit.v1)
    uint64_t currents = 0;  // current generation the hue of the brook water was last laid from (water.quality.v1, curr_hue_deg)
    uint64_t lakes = 0;     // lake generation the lake and pond surfaces were last laid from (H.13, water.lake.v1)
    uint64_t seas = 0;      // sea generation the sea plane was last laid from (H.13, water.sea.v1)
    uint64_t glaciers = 0;  // glacier generation the ice caps were last laid from (H.13, water.glacier.v1)
};

}  // namespace ase::adp::godot

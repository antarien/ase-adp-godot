#pragma once

/**
 * ASE GODOT ADAPTER - LAYOUT OF THE SALT RECORDS
 *
 * @file        godot_salt_records.hpp
 * @brief       One record of the salt port as the view of one chunk's dissolved salt
 * @description The salt port hands the app every chunk ase-gis answered (H.9, I-4): the chunk, the salt the
 *              water carries, the salt the soil holds and the sum ase-gis added (ase-pl-hydrogeology,
 *              HydrogeologyHostSlntExptSystem on hydrogeology.salt.v1). The reader lays one record out into the
 *              view the debug line and the I-4 balance read, and refuses a record without its chunk, with a
 *              chunk off the grid or with a value that is no finite number: the port lays a record only where
 *              all three answered, so a short record is a broken port, never a gap the view fills.
 *
 *              NO GODOT TYPE: a record of the kernel in, a view out, so the host tests check it without a
 *              Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.28.00028 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_groundwater_view.hpp>
#include <ase/kernel/kernel_host_port.hpp>

namespace ase::adp::godot {

/** One salt record as a view; false for a record without its chunk, a chunk off the grid or a value that is no finite number. */
bool salt_view_of(const ase::kernel::KernelHostRecord& record, VivariumSaltView& out);

}  // namespace ase::adp::godot

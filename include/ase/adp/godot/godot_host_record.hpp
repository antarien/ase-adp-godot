#pragma once

/**
 * ASE GODOT ADAPTER - ONE VALUE OF A HOST RECORD
 *
 * @file        godot_host_record.hpp
 * @brief       The value a port record carries under a key, and the test of a whole number in it
 * @description Every port of the embedded host hands the app records of named values (KernelHostRecord).
 *              The readers of the organ, terrain, patch, stream, template, sky and seed ports all ask one
 *              record the same two questions: which value stands under this key, and is it a whole number
 *              the port uses for a place, a body or a code. Both answers stand here once, so every unit
 *              that lays out records reads them the same way.
 *
 *              NO GODOT TYPE: a record of the kernel in, a number out, so the host tests check it without
 *              a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101344 - carry host records to app
 * @actor       Host Record Transport Control (HOSTXFER)
 * @created     2026-11-05
 * @modified    2026-11-05
 * @version     00.00.01.00000
 */

#include <ase/kernel/kernel_host_port.hpp>

namespace ase::adp::godot {

/** The value stored under `key` in a port record; NaN when the record does not carry it. */
double record_value(const ase::kernel::KernelHostRecord& record, const char* key);

/**
 * True for a finite, whole, non-negative value. The port carries places, bodies and codes as small whole
 * numbers, which a float holds exactly - ase-math has the float floor.
 */
bool whole(double value);

}  // namespace ase::adp::godot

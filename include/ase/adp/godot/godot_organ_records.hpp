#pragma once

/**
 * ASE GODOT ADAPTER - LAYOUT OF THE ORGAN RECORDS
 *
 * @file        godot_organ_records.hpp
 * @brief       The records of the organ, bark, tile, sway and instance ports as views of one organ
 * @description The flora plugin hands every organ of a plant to the app over five ports: the organ itself
 *              (body, order, place, turn, length, radius), the bark share of an internode, the bark tiling
 *              of an internode, the sway of an organ and the twelve rows of its instance transform. Each
 *              reader below lays one record out into the view the node draws, and refuses a record that
 *              misses a key or carries a value that is no finite number: the ports write every key for every
 *              organ they name, so a gap is a broken port, never an organ drawn from a number nobody
 *              computed.
 *
 *              NO GODOT TYPE: a record of the kernel in, a view out, so the host tests check every reader
 *              without a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    render/pipeline/stage
 * @usecase     usecase:100974 - lay out organ records
 * @actor       Vivarium Steward (VIVARIUSTE)
 * @created     2026-11-05
 * @modified    2026-11-05
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/kernel/kernel_host_port.hpp>

namespace ase::adp::godot {

/** One organ record as a view; false for a record that misses a key or carries a value that is no finite number. */
bool organ_view_of(const ase::kernel::KernelHostRecord& record, VivariumOrganView& out);

/** The bark share of a segment record into its view; false without one in GODOT_BARK_WHITE..GODOT_BARK_BASE. */
bool segment_bark_of(const ase::kernel::KernelHostRecord& record, VivariumOrganView& out);

/** The bark tiling of a tile record into the view of its internode; false without three positive finite counts. */
bool segment_tile_of(const ase::kernel::KernelHostRecord& record, VivariumOrganView& out);

/** The sway of a sway record into the view of its organ; false without seven finite values and a frequency >= 0. */
bool organ_sway_of(const ase::kernel::KernelHostRecord& record, VivariumOrganView& out);

/** One instance record as a view; false for a record that misses one of the twelve rows. */
bool instance_view_of(const ase::kernel::KernelHostRecord& record, VivariumInstanceView& out);

}  // namespace ase::adp::godot

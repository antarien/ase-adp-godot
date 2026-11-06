#pragma once

/**
 * ASE GODOT ADAPTER - LAYOUT OF THE TERRAIN SAMPLES
 *
 * @file        godot_terrain_samples.hpp
 * @brief       One record of the terrain port as the view of one ground sample
 * @description The terrain port hands the app every sample of the patch ground: its body, its place, the
 *              height ase-terrain computed and the normal and texture coordinates ase-render-data computed
 *              (ase-pl-flora, FloraHostTernExptSystem). The reader lays one record out into the view the
 *              node meshes the ground from, and refuses a record that misses one of the nine keys or carries
 *              a value that is no finite number - never ground drawn at height zero or lit with a normal the
 *              view chose.
 *
 *              NO GODOT TYPE: a record of the kernel in, a view out, so the host tests check it without a
 *              Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    render/pipeline/stage
 * @usecase     usecase:100971 - lay out terrain samples
 * @actor       Vivarium Steward (VIVARIUSTE)
 * @created     2026-11-05
 * @modified    2026-11-05
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/kernel/kernel_host_port.hpp>

namespace ase::adp::godot {

/** One terrain record as a view; false for a record that misses a key or carries a value that is no finite number. */
bool terrain_view_of(const ase::kernel::KernelHostRecord& record, VivariumTerrainView& out);

}  // namespace ase::adp::godot

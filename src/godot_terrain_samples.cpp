/**
 * ASE GODOT ADAPTER - LAYOUT OF THE TERRAIN SAMPLES
 *
 * @file        godot_terrain_samples.cpp
 * @brief       One record of the terrain port as the view of one ground sample
 * @description The terrain port writes all nine keys of every sample - place and height of ase-terrain,
 *              normal and texture coordinates of ase-render-data (ase-pl-flora, FloraHostTernExptSystem) - so
 *              a gap is a broken port, never ground drawn at height zero or lit with a normal the view chose.
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

// Own header FIRST
#include <ase/adp/godot/godot_terrain_samples.hpp>
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

bool terrain_view_of(const ase::kernel::KernelHostRecord& record, VivariumTerrainView& out) {
    out = VivariumTerrainView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    out.x = record_value(record, GODOT_KEY_X);
    out.z = record_value(record, GODOT_KEY_Z);
    out.height = record_value(record, GODOT_KEY_HEIGHT);
    out.nx = record_value(record, GODOT_KEY_NX);
    out.ny = record_value(record, GODOT_KEY_NY);
    out.nz = record_value(record, GODOT_KEY_NZ);
    out.u = record_value(record, GODOT_KEY_U);
    out.v = record_value(record, GODOT_KEY_V);
    if (!whole(body) || body > static_cast<double>(UINT32_MAX) || !std::isfinite(out.x) ||
        !std::isfinite(out.z) || !std::isfinite(out.height) || !std::isfinite(out.nx) ||
        !std::isfinite(out.ny) || !std::isfinite(out.nz) || !std::isfinite(out.u) || !std::isfinite(out.v)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    return true;
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - LAYOUT OF THE ORGAN RECORDS
 *
 * @file        godot_organ_records.cpp
 * @brief       The records of the organ, bark, tile, sway and instance ports as views of one organ
 * @description Each reader lays one port record out into the view of one organ and refuses a record with a
 *              gap: the flora ports write every key for every organ they name, so a missing or non-finite
 *              value is a broken port, never an organ drawn at the origin, in a bark nobody computed or
 *              swung by a number nobody computed.
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

// Own header FIRST
#include <ase/adp/godot/godot_organ_records.hpp>
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

using ase::kernel::KernelHostRecord;

/**
 * The organ port writes all eleven keys of every organ (ase-pl-flora, the organ export systems), so a gap is
 * a broken port - never an organ drawn at the origin.
 */
bool organ_view_of(const KernelHostRecord& record, VivariumOrganView& out) {
    out = VivariumOrganView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    out.order = record_value(record, GODOT_KEY_ORDER);
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    out.qx = record_value(record, GODOT_KEY_QX);
    out.qy = record_value(record, GODOT_KEY_QY);
    out.qz = record_value(record, GODOT_KEY_QZ);
    out.qw = record_value(record, GODOT_KEY_QW);
    out.length = record_value(record, GODOT_KEY_LENGTH);
    out.radius = record_value(record, GODOT_KEY_RADIUS);
    if (!whole(body) || body > static_cast<double>(UINT32_MAX) || !std::isfinite(out.order) ||
        !std::isfinite(out.x) || !std::isfinite(out.y) || !std::isfinite(out.z) || !std::isfinite(out.qx) ||
        !std::isfinite(out.qy) || !std::isfinite(out.qz) || !std::isfinite(out.qw) ||
        !std::isfinite(out.length) || !std::isfinite(out.radius)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    return true;
}

/**
 * usecase:100395 age the bark. The segment port writes GODOT_KEY_BARK for every internode as its twelfth value
 * (FloraHostSegmExptSystem), so a gap is a broken port - never an internode drawn in a bark nobody computed.
 */
bool segment_bark_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double bark = record_value(record, GODOT_KEY_BARK);
    if (!std::isfinite(bark) || bark < GODOT_BARK_WHITE || bark > GODOT_BARK_BASE) {
        return false;
    }
    out.bark = bark;
    return true;
}

/**
 * 03.11 step 2, the material data of a section. The tile port writes all three keys for every internode it
 * names (FloraHostTileExptSystem), so a gap is a broken port - never a bark stretched by a tiling nobody
 * computed.
 */
bool segment_tile_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double around = record_value(record, GODOT_KEY_TILE_AROUND);
    const double along = record_value(record, GODOT_KEY_TILE_ALONG);
    const double base = record_value(record, GODOT_KEY_TILE_BASE);
    if (!std::isfinite(around) || !std::isfinite(along) || !std::isfinite(base) || around <= 0.0 ||
        along <= 0.0 || base <= 0.0) {
        return false;
    }
    out.tile_around = around;
    out.tile_along = along;
    out.tile_base = base;
    return true;
}

/**
 * usecase:101335 animate the branch sway. The sway port writes all seven keys for every organ it names
 * (FloraHostSwayExptSystem), so a gap is a broken port - never a branch swung by a number nobody computed.
 */
bool organ_sway_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double freq = record_value(record, GODOT_KEY_SWAY_FREQ);
    const double mean_x = record_value(record, GODOT_KEY_SWAY_MEAN_X);
    const double mean_z = record_value(record, GODOT_KEY_SWAY_MEAN_Z);
    const double gust_x = record_value(record, GODOT_KEY_SWAY_GUST_X);
    const double gust_z = record_value(record, GODOT_KEY_SWAY_GUST_Z);
    const double shape_base = record_value(record, GODOT_KEY_SWAY_SHAPE_BASE);
    const double shape_top = record_value(record, GODOT_KEY_SWAY_SHAPE_TOP);
    if (!std::isfinite(freq) || !std::isfinite(mean_x) || !std::isfinite(mean_z) || !std::isfinite(gust_x) ||
        !std::isfinite(gust_z) || !std::isfinite(shape_base) || !std::isfinite(shape_top) || freq < GODOT_SWAY_NONE) {
        return false;
    }
    out.sway_freq = freq;
    out.sway_mean_x = mean_x;
    out.sway_mean_z = mean_z;
    out.sway_gust_x = gust_x;
    out.sway_gust_z = gust_z;
    out.sway_base = shape_base;
    out.sway_top = shape_top;
    return true;
}

/**
 * The instance port writes all twelve keys of every organ (ase-pl-flora, the instance export systems), so a
 * gap is a broken port - never an instance drawn collapsed at the origin. The keys stand in the row order of
 * MultiMesh::set_buffer and of VivariumInstanceView::rows.
 */
bool instance_view_of(const KernelHostRecord& record, VivariumInstanceView& out) {
    const char* const keys[GODOT_INSTANCE_VALUES] = {
        GODOT_KEY_M00, GODOT_KEY_M01, GODOT_KEY_M02, GODOT_KEY_M03, GODOT_KEY_M10, GODOT_KEY_M11,
        GODOT_KEY_M12, GODOT_KEY_M13, GODOT_KEY_M20, GODOT_KEY_M21, GODOT_KEY_M22, GODOT_KEY_M23};
    out = VivariumInstanceView{};
    out.object_id = record.object_id;
    for (uint32_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
        const double value = record_value(record, keys[v]);
        if (!std::isfinite(value)) {
            return false;
        }
        out.rows[v] = static_cast<float>(value);
    }
    return true;
}

}  // namespace ase::adp::godot

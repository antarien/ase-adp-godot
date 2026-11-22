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
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

// Own header FIRST
#include <ase/adp/godot/godot_organ_records.hpp>
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/leaf/types.hpp>
#include <ase/adp/godot/tube/types.hpp>

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
 * The bend port writes both keys of every bent leaf (ase-pl-flora, FloraHostBendExptSystem), each inside the
 * bounds of the species catalogue that FloraHubBendSyncSystem checked against PLT_CONST_LEAF_BEND_MIN/MAX. A gap
 * or a value that is no finite number is a broken port; the view holds no bound of ase-plant of its own.
 */
bool leaf_bend_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double across = record_value(record, GODOT_KEY_BEND_ACROSS);
    const double along = record_value(record, GODOT_KEY_BEND_ALONG);
    if (!std::isfinite(across) || !std::isfinite(along)) {
        return false;
    }
    out.bend_across = across;
    out.bend_along = along;
    return true;
}

/**
 * The tint port writes its one key for every leaf with a colour offset (ase-pl-flora, FloraHostTintExptSystem), so a
 * value that is no finite number is a broken port - never a card drawn black or white.
 */
bool leaf_tint_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double offset = record_value(record, GODOT_KEY_TINT_OFFSET);
    if (!std::isfinite(offset)) {
        return false;
    }
    out.tint_offset = offset;
    return true;
}

/**
 * The atlas port writes both keys of every leaf with an atlas cell (ase-pl-flora, FloraHostAtlsExptSystem), the cell
 * below its cells and the cells inside FLRA_PART_ATLS_CELLS_MIN/MAX as FloraHubAtlsSyncSystem checked them. A gap or
 * a value that is no finite number is a broken port - never a card sampled outside its texture.
 */
bool leaf_atlas_of(const KernelHostRecord& record, VivariumOrganView& out) {
    const double cell = record_value(record, GODOT_KEY_ATLS_CELL);
    const double cells = record_value(record, GODOT_KEY_ATLS_CELLS);
    if (!std::isfinite(cell) || !std::isfinite(cells)) {
        return false;
    }
    out.atlas_cell = cell;
    out.atlas_cells = cells;
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

/**
 * usecase:101613 show the plant bark tubes (register B0295 point 4). The bark port writes all twelve keys of every
 * vertex (FloraHostCylExptSystem), so a gap is a broken port - never a tube closed over a vertex nobody laid. The
 * three links are object ids: whole numbers up to UINT32_MAX, where UINT32_MAX is InvalidEntityId.
 */
bool bark_vertex_view_of(const KernelHostRecord& record, VivariumBarkVertexView& out) {
    out = VivariumBarkVertexView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    const double next = record_value(record, GODOT_KEY_NEXT);
    const double below = record_value(record, GODOT_KEY_BELOW);
    const double organ = record_value(record, GODOT_KEY_ORGAN);
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    out.nx = record_value(record, GODOT_KEY_NX);
    out.ny = record_value(record, GODOT_KEY_NY);
    out.nz = record_value(record, GODOT_KEY_NZ);
    out.u = record_value(record, GODOT_KEY_U);
    out.v = record_value(record, GODOT_KEY_V);
    const double limit = static_cast<double>(UINT32_MAX);
    if (!whole(body) || body > limit || !whole(next) || next > limit || !whole(below) || below > limit ||
        !whole(organ) || organ > limit || !std::isfinite(out.x) || !std::isfinite(out.y) || !std::isfinite(out.z) ||
        !std::isfinite(out.nx) || !std::isfinite(out.ny) || !std::isfinite(out.nz) || !std::isfinite(out.u) ||
        !std::isfinite(out.v)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    out.next = static_cast<uint32_t>(next);
    out.below = static_cast<uint32_t>(below);
    out.organ = static_cast<uint32_t>(organ);
    return true;
}

/**
 * usecase:101613 show the plant bark tubes (PLAN_ASE_VIVARIUM_TREEGEN T.6). The knot port writes all twelve keys of
 * every internode (FloraHostCylNodeExptSystem), so a gap is a broken port - never a ring laid around a knot nobody
 * carried. below is an object id, a whole number up to UINT32_MAX; the radius is never negative. rings is the count
 * of intermediate rings ase-render-data gave the piece by the turn of its tangent (usecase:101643 - ring the tubes
 * by curvature), a whole number up to GODOT_TUBE_RING_MAX.
 */
bool knot_view_of(const KernelHostRecord& record, VivariumKnotView& out) {
    out = VivariumKnotView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    const double below = record_value(record, GODOT_KEY_BELOW);
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    out.radius = record_value(record, GODOT_KEY_RADIUS);
    out.qx = record_value(record, GODOT_KEY_QX);
    out.qy = record_value(record, GODOT_KEY_QY);
    out.qz = record_value(record, GODOT_KEY_QZ);
    out.qw = record_value(record, GODOT_KEY_QW);
    out.v = record_value(record, GODOT_KEY_V);
    const double rings = record_value(record, GODOT_KEY_RINGS);
    const double limit = static_cast<double>(UINT32_MAX);
    if (!whole(body) || body > limit || !whole(below) || below > limit || !whole(rings) ||
        rings > static_cast<double>(GODOT_TUBE_RING_MAX) || !std::isfinite(out.x) ||
        !std::isfinite(out.y) || !std::isfinite(out.z) || !std::isfinite(out.radius) || out.radius < 0.0 ||
        !std::isfinite(out.qx) || !std::isfinite(out.qy) || !std::isfinite(out.qz) || !std::isfinite(out.qw) ||
        !std::isfinite(out.v)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    out.below = static_cast<uint32_t>(below);
    out.rings = static_cast<uint32_t>(rings);
    return true;
}

/**
 * usecase:101613 show the plant bark tubes (PLAN_ASE_VIVARIUM_TREEGEN T.6). The foot port writes all twelve keys of
 * every internode that starts an axis (FloraHostCylNodeExptSystem, in the pass of its knot), so a gap is a broken
 * port - never an axis opened on a ring nobody carried. sides is the corner count K ase-render-data laid every ring
 * of the axis with (usecase:101644 - count ring corners by size), a whole number from GODOT_TUBE_SIDE_MIN to
 * GODOT_TUBE_SIDE_MAX; noise is the key of the place of the axis in the bark noise field, from GODOT_TUBE_NOISE_LEAST
 * to GODOT_TUBE_NOISE_MOST ("Rindenrauschen").
 */
bool foot_view_of(const KernelHostRecord& record, VivariumFootView& out) {
    out = VivariumFootView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    out.x = record_value(record, GODOT_KEY_X);
    out.y = record_value(record, GODOT_KEY_Y);
    out.z = record_value(record, GODOT_KEY_Z);
    out.radius = record_value(record, GODOT_KEY_RADIUS);
    out.qx = record_value(record, GODOT_KEY_QX);
    out.qy = record_value(record, GODOT_KEY_QY);
    out.qz = record_value(record, GODOT_KEY_QZ);
    out.qw = record_value(record, GODOT_KEY_QW);
    out.v = record_value(record, GODOT_KEY_V);
    const double sides = record_value(record, GODOT_KEY_SIDES);
    if (!whole(sides) || sides < static_cast<double>(GODOT_TUBE_SIDE_MIN) ||
        sides > static_cast<double>(GODOT_TUBE_SIDE_MAX)) {
        return false;
    }
    out.sides = static_cast<uint32_t>(sides);
    out.noise = record_value(record, GODOT_KEY_NOISE);
    if (!std::isfinite(out.noise) || out.noise < GODOT_TUBE_NOISE_LEAST || out.noise > GODOT_TUBE_NOISE_MOST) {
        return false;
    }
    if (!whole(body) || body > static_cast<double>(UINT32_MAX) || !std::isfinite(out.x) || !std::isfinite(out.y) ||
        !std::isfinite(out.z) || !std::isfinite(out.radius) || out.radius < 0.0 || !std::isfinite(out.qx) ||
        !std::isfinite(out.qy) || !std::isfinite(out.qz) || !std::isfinite(out.qw) || !std::isfinite(out.v)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    return true;
}

/**
 * usecase:101613 show the plant bark tubes (PLAN_ASE_VIVARIUM_TREEGEN T.6, cut face). The scar port writes the
 * body of every stub knot (FloraHostCylNodeExptSystem step 5b), its object the internode, so a missing body is a
 * broken port - never a disk laid on a tip nobody cut.
 */
bool scar_view_of(const KernelHostRecord& record, VivariumScarView& out) {
    out = VivariumScarView{};
    out.object_id = record.object_id;
    const double body = record_value(record, GODOT_KEY_BODY);
    if (!whole(body) || body > static_cast<double>(UINT32_MAX)) {
        return false;
    }
    out.body = static_cast<uint32_t>(body);
    return true;
}

}  // namespace ase::adp::godot

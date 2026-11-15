/**
 * ASE GODOT ADAPTER - GROUND AROUND THE PLANT
 *
 * @file        godot_plant_ground.cpp
 * @brief       The ground mesh from the terrain and ground cell ports, the marks its shader paints by, and its patch
 * @description See godot_plant_ground.hpp. The bytes are written as Godot writes an uncompressed surface
 *              (RenderingServer::mesh_create_surface_data_from_arrays): floats as their four bytes, the normal
 *              through the octahedral code of Vector3::octahedron_encode cut to uint16, the colour cut to uint8.
 *              Godot 4.5 gives a surface with normals and no tangents a tangent of its own, derived from the
 *              normal and coded through Vector3::octahedron_tangent_encode right behind it (normal stride 8);
 *              the patch writes that tangent too, so a moved normal never keeps the tangent of its old place.
 *              ground_code_holds holds that code against the bytes Godot made before any patch is written.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-10-26
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

/** `value` as its four bytes at `at`, the lowest first, as a float or uint32 stands in Godot's buffers. */
void put_u32(uint8_t* at, uint32_t value) {
    at[0] = static_cast<uint8_t>(value & 0xffu);
    at[1] = static_cast<uint8_t>((value >> 8u) & 0xffu);
    at[2] = static_cast<uint8_t>((value >> 16u) & 0xffu);
    at[3] = static_cast<uint8_t>((value >> 24u) & 0xffu);
}

/** `value` as its two bytes at `at`, the lowest first. */
void put_u16(uint8_t* at, uint16_t value) {
    at[0] = static_cast<uint8_t>(value & 0xffu);
    at[1] = static_cast<uint8_t>((value >> 8u) & 0xffu);
}

/** One share of the octahedral code as Godot stores it: CLAMP(share * 65535, 0, 65535), cut to uint16. */
uint16_t octahedral_share(float share) {
    const float scaled = share * 65535.0f;
    return static_cast<uint16_t>(scaled < 0.0f ? 0.0f : (scaled > 65535.0f ? 65535.0f : scaled));
}

/** One colour share as Godot stores it: CLAMP(int(share * 255.0), 0, 255). */
uint8_t color_share(float share) {
    const int scaled = static_cast<int>(static_cast<double>(share) * 255.0);
    return static_cast<uint8_t>(scaled < 0 ? 0 : (scaled > 255 ? 255 : scaled));
}

/**
 * Vector3::octahedron_encode of (x, y, z): divided by the sum of the shares, the lower half folded over, both
 * shares moved from [-1, 1] to [0, 1] into `first` and `second`.
 */
void octahedral_code(float x, float y, float z, float& first, float& second) {
    const float sum = std::fabs(x) + std::fabs(y) + std::fabs(z);
    const float nx = x / sum;
    const float ny = y / sum;
    const float nz = z / sum;
    const float ox = nz >= 0.0f ? nx : (1.0f - std::fabs(ny)) * (nx >= 0.0f ? 1.0f : -1.0f);
    const float oy = nz >= 0.0f ? ny : (1.0f - std::fabs(nx)) * (ny >= 0.0f ? 1.0f : -1.0f);
    first = ox * 0.5f + 0.5f;
    second = oy * 0.5f + 0.5f;
}

/**
 * The tangent Godot gives a normal (nx, ny, nz) that came without one, in its code at `tangent` (4 bytes): the
 * cross product of (nz, -nx, ny) with the normalised normal, normalised, through Vector3::octahedron_tangent_encode
 * with sign 1 - the second share raised to GODOT_GROUND_TANGENT_BIAS at least and moved into the upper half.
 */
void code_tangent(float nx, float ny, float nz, uint8_t* tangent) {
    const float length = ase::math::sqrt(nx * nx + ny * ny + nz * nz);
    const float bx = length > 0.0f ? nx / length : 0.0f;
    const float by = length > 0.0f ? ny / length : 0.0f;
    const float bz = length > 0.0f ? nz / length : 0.0f;
    // (nz, -nx, ny) crossed with (bx, by, bz)
    const float tx = -nx * bz - ny * by;
    const float ty = ny * bx - nz * bz;
    const float tz = nz * by + nx * bx;
    const float across = ase::math::sqrt(tx * tx + ty * ty + tz * tz);
    float first = 0.0f;
    float second = 0.0f;
    octahedral_code(across > 0.0f ? tx / across : 0.0f, across > 0.0f ? ty / across : 0.0f,
                    across > 0.0f ? tz / across : 0.0f, first, second);
    second = second > GODOT_GROUND_TANGENT_BIAS ? second : GODOT_GROUND_TANGENT_BIAS;
    put_u16(tangent, octahedral_share(first));
    put_u16(tangent + 2, octahedral_share(second * 0.5f + 0.5f));
}

/**
 * Vertex `vertex` of `arrays` in Godot's code: its place at `place` (12 bytes), its normal at `normal` (4 bytes,
 * Vector3::octahedron_encode), the tangent Godot derives from it at `tangent` (4 bytes, code_tangent; nullptr
 * where the surface carries none), its colour at `color` (4 bytes), its texture coordinates at `uv` (8 bytes).
 */
void code_vertex(const PlantMeshArrays& arrays, uint32_t vertex, uint8_t* place, uint8_t* normal, uint8_t* tangent,
                 uint8_t* color, uint8_t* uv) {
    const std::size_t v = vertex;
    put_u32(place, std::bit_cast<uint32_t>(arrays.positions[v * 3u]));
    put_u32(place + 4, std::bit_cast<uint32_t>(arrays.positions[v * 3u + 1u]));
    put_u32(place + 8, std::bit_cast<uint32_t>(arrays.positions[v * 3u + 2u]));
    const float nx = arrays.normals[v * 3u];
    const float ny = arrays.normals[v * 3u + 1u];
    const float nz = arrays.normals[v * 3u + 2u];
    float first = 0.0f;
    float second = 0.0f;
    octahedral_code(nx, ny, nz, first, second);
    put_u16(normal, octahedral_share(first));
    put_u16(normal + 2, octahedral_share(second));
    if (tangent != nullptr) {
        code_tangent(nx, ny, nz, tangent);
    }
    for (std::size_t share = 0u; share < 4u; ++share) {
        color[share] = color_share(arrays.colors[v * 4u + share]);
    }
    put_u32(uv, std::bit_cast<uint32_t>(arrays.uvs[v * 2u]));
    put_u32(uv + 4, std::bit_cast<uint32_t>(arrays.uvs[v * 2u + 1u]));
}

/** The mark of the ground at (x, y, z) (mark_ground): `red` 1 on levelled ground, `green` 1 in a brook's bed. */
void mark_of(const std::vector<PlantGroundDisc>& discs, const std::vector<PlantGroundBed>& beds, float x, float y,
             float z, float& red, float& green) {
    // levelled: inside the disc and at its height - a disc ase-terrain has not levelled yet stays grass
    bool levelled = false;
    for (const PlantGroundDisc& disc : discs) {
        const float dx = x - disc.x;
        const float dz = z - disc.z;
        levelled = levelled || (dx * dx + dz * dz <= disc.radius * disc.radius &&
                                std::fabs(y - disc.height) <= GODOT_GROUND_LEVEL_TOLERANCE_M);
    }
    // bed: no farther from the line between the two ends than half the width
    bool bed = false;
    for (const PlantGroundBed& brook : beds) {
        const float ax = brook.end_x - brook.x;
        const float az = brook.end_z - brook.z;
        const float length2 = ax * ax + az * az;
        const float along = length2 > 0.0f ? ((x - brook.x) * ax + (z - brook.z) * az) / length2 : 0.0f;
        const float t = along < 0.0f ? 0.0f : (along > 1.0f ? 1.0f : along);
        const float dx = x - (brook.x + t * ax);
        const float dz = z - (brook.z + t * az);
        bed = bed || dx * dx + dz * dz <= brook.half_m * brook.half_m;
    }
    red = levelled ? 1.0f : 0.0f;
    green = bed ? 1.0f : 0.0f;
}

/** Widens [first, last] by `vertex`; `any` says whether the range held a vertex before. */
void widen(uint32_t vertex, bool& any, uint32_t& first, uint32_t& last) {
    first = any && first < vertex ? first : vertex;
    last = any && last > vertex ? last : vertex;
    any = true;
}

}  // namespace

uint32_t build_ground(const std::vector<VivariumTerrainView>& samples,
                      const std::vector<VivariumGroundCellView>& cells, PlantMeshArrays& out,
                      std::vector<PlantGroundSlot>& slots) {
    // one vertex per sample, in the order of the port; its slot by object id
    std::unordered_map<uint32_t, int32_t> slot;
    slot.reserve(samples.size());
    slots.reserve(slots.size() + samples.size());
    for (const VivariumTerrainView& sample : samples) {
        const auto vertex = static_cast<int32_t>(out.positions.size() / 3u);
        if (!slot.emplace(sample.object_id, vertex).second) {
            slots.push_back(PlantGroundSlot{sample.object_id, -1});
            continue;
        }
        slots.push_back(PlantGroundSlot{sample.object_id, vertex});
        append3(out.positions, static_cast<float>(sample.x), static_cast<float>(sample.height),
                static_cast<float>(sample.z));
        append3(out.normals, static_cast<float>(sample.nx), static_cast<float>(sample.ny), static_cast<float>(sample.nz));
        out.uvs.push_back(static_cast<float>(sample.u));
        out.uvs.push_back(static_cast<float>(sample.v));
    }
    // per cell the two triangles in the order the cell names its corners (ase-render-data)
    uint32_t drawn = 0u;
    for (const VivariumGroundCellView& cell : cells) {
        const auto sw = slot.find(cell.sw);
        const auto se = slot.find(cell.se);
        const auto ne = slot.find(cell.ne);
        const auto nw = slot.find(cell.nw);
        if (sw == slot.end() || se == slot.end() || ne == slot.end() || nw == slot.end()) {
            continue;
        }
        out.indices.insert(out.indices.end(),
                           {sw->second, se->second, ne->second, sw->second, ne->second, nw->second});
        drawn += 1u;
    }
    return drawn;
}

uint32_t mark_ground(const std::vector<PlantGroundDisc>& discs, const std::vector<PlantGroundBed>& beds,
                     PlantMeshArrays& out) {
    const std::size_t vertices = out.positions.size() / 3u;
    out.colors.assign(vertices * 4u, 0.0f);
    uint32_t marked = 0u;
    for (std::size_t v = 0u; v < vertices; ++v) {
        float red = 0.0f;
        float green = 0.0f;
        mark_of(discs, beds, out.positions[v * 3u], out.positions[v * 3u + 1u], out.positions[v * 3u + 2u], red,
                green);
        out.colors[v * 4u] = red;
        out.colors[v * 4u + 1u] = green;
        out.colors[v * 4u + 3u] = 1.0f;
        marked += (red > 0.0f || green > 0.0f) ? 1u : 0u;
    }
    return marked;
}

bool encode_ground(PlantGroundSurface& surface, uint32_t vertex) {
    const PlantGroundLayout& layout = surface.layout;
    const PlantMeshArrays& arrays = surface.arrays;
    const std::size_t v = vertex;
    const std::size_t place = layout.position_offset + v * layout.position_stride;
    const std::size_t normal = layout.normal_offset + v * layout.normal_stride;
    const std::size_t color = layout.color_offset + v * layout.attribute_stride;
    const std::size_t uv = layout.uv_offset + v * layout.attribute_stride;
    if (layout.normal_stride < 4u || place + 12u > surface.vertex_bytes.size() ||
        normal + layout.normal_stride > surface.vertex_bytes.size() || color + 4u > surface.attribute_bytes.size() ||
        uv + 8u > surface.attribute_bytes.size() || (v + 1u) * 3u > arrays.positions.size() ||
        (v + 1u) * 3u > arrays.normals.size() || (v + 1u) * 4u > arrays.colors.size() ||
        (v + 1u) * 2u > arrays.uvs.size()) {
        return false;
    }
    // a stride of 8 carries the tangent Godot derived from the normal right behind it
    uint8_t* tangent = layout.normal_stride >= 8u ? surface.vertex_bytes.data() + normal + 4u : nullptr;
    code_vertex(arrays, vertex, surface.vertex_bytes.data() + place, surface.vertex_bytes.data() + normal, tangent,
                surface.attribute_bytes.data() + color, surface.attribute_bytes.data() + uv);
    return true;
}

bool ground_code_holds(const PlantGroundSurface& surface) {
    // the normal alone (stride 4) or the normal with the tangent Godot derived from it (stride 8): both are coded
    if ((surface.layout.normal_stride != 4u && surface.layout.normal_stride != 8u) ||
        surface.arrays.positions.empty()) {
        return false;
    }
    PlantGroundSurface probe;
    probe.layout = surface.layout;
    probe.arrays = surface.arrays;
    probe.vertex_bytes.assign(surface.vertex_bytes.size(), 0u);
    probe.attribute_bytes.assign(surface.attribute_bytes.size(), 0u);
    if (!encode_ground(probe, 0u)) {
        return false;
    }
    const PlantGroundLayout& layout = surface.layout;
    const auto same = [](const std::vector<uint8_t>& made, const std::vector<uint8_t>& coded, std::size_t at,
                         std::size_t length) {
        for (std::size_t i = at; i < at + length; ++i) {
            if (made[i] != coded[i]) {
                return false;
            }
        }
        return true;
    };
    return same(surface.vertex_bytes, probe.vertex_bytes, layout.position_offset, 12u) &&
           same(surface.vertex_bytes, probe.vertex_bytes, layout.normal_offset, layout.normal_stride) &&
           same(surface.attribute_bytes, probe.attribute_bytes, layout.color_offset, 4u) &&
           same(surface.attribute_bytes, probe.attribute_bytes, layout.uv_offset, 8u);
}

bool patch_ground(const std::vector<VivariumTerrainView>& samples, const std::vector<VivariumGroundCellView>& cells,
                  const std::vector<PlantGroundDisc>& discs, const std::vector<PlantGroundBed>& beds,
                  PlantGroundSurface& surface) {
    surface.positions = PlantGroundSpan{};
    surface.normals = PlantGroundSpan{};
    surface.attributes = PlantGroundSpan{};
    surface.grown = false;
    if (!surface.patchable || samples.size() != surface.slots.size() || cells.size() != surface.cells.size()) {
        return false;
    }
    for (std::size_t i = 0u; i < cells.size(); ++i) {
        const VivariumGroundCellView& now = cells[i];
        const VivariumGroundCellView& built = surface.cells[i];
        if (now.object_id != built.object_id || now.body != built.body || now.sw != built.sw || now.se != built.se ||
            now.ne != built.ne || now.nw != built.nw) {
            return false;
        }
    }
    for (std::size_t i = 0u; i < samples.size(); ++i) {
        if (samples[i].object_id != surface.slots[i].object_id) {
            return false;
        }
    }
    PlantMeshArrays& arrays = surface.arrays;
    bool moved_any = false;
    uint32_t moved_first = 0u;
    uint32_t moved_last = 0u;
    bool marked_any = false;
    uint32_t marked_first = 0u;
    uint32_t marked_last = 0u;
    // the samples that moved: place, normal and texture coordinates as the port carries them now
    for (std::size_t i = 0u; i < samples.size(); ++i) {
        if (surface.slots[i].vertex < 0) {
            continue;
        }
        const auto vertex = static_cast<uint32_t>(surface.slots[i].vertex);
        const VivariumTerrainView& sample = samples[i];
        float* place = arrays.positions.data() + static_cast<std::size_t>(vertex) * 3u;
        float* normal = arrays.normals.data() + static_cast<std::size_t>(vertex) * 3u;
        float* uv = arrays.uvs.data() + static_cast<std::size_t>(vertex) * 2u;
        const float now[8] = {static_cast<float>(sample.x),  static_cast<float>(sample.height),
                              static_cast<float>(sample.z),  static_cast<float>(sample.nx),
                              static_cast<float>(sample.ny), static_cast<float>(sample.nz),
                              static_cast<float>(sample.u),  static_cast<float>(sample.v)};
        const bool moved = place[0] != now[0] || place[1] != now[1] || place[2] != now[2] || normal[0] != now[3] ||
                           normal[1] != now[4] || normal[2] != now[5];
        const bool shifted = uv[0] != now[6] || uv[1] != now[7];
        if (!moved && !shifted) {
            continue;
        }
        for (std::size_t share = 0u; share < 3u; ++share) {
            place[share] = now[share];
            normal[share] = now[3u + share];
        }
        uv[0] = now[6];
        uv[1] = now[7];
        if (!encode_ground(surface, vertex)) {
            return false;
        }
        if (moved) {
            widen(vertex, moved_any, moved_first, moved_last);
            surface.grown = surface.grown || now[1] < surface.low_m || now[1] > surface.high_m;
            surface.low_m = now[1] < surface.low_m ? now[1] : surface.low_m;
            surface.high_m = now[1] > surface.high_m ? now[1] : surface.high_m;
        }
        if (shifted) {
            widen(vertex, marked_any, marked_first, marked_last);
        }
    }
    // the vertices whose mark changed: a dig levels or unlevels ground, a flat or a bed comes or goes
    const auto vertices = static_cast<uint32_t>(arrays.positions.size() / 3u);
    for (uint32_t v = 0u; v < vertices; ++v) {
        float red = 0.0f;
        float green = 0.0f;
        const std::size_t at = static_cast<std::size_t>(v) * 3u;
        mark_of(discs, beds, arrays.positions[at], arrays.positions[at + 1u], arrays.positions[at + 2u], red, green);
        float* color = arrays.colors.data() + static_cast<std::size_t>(v) * 4u;
        if (color[0] == red && color[1] == green) {
            continue;
        }
        color[0] = red;
        color[1] = green;
        if (!encode_ground(surface, v)) {
            return false;
        }
        widen(v, marked_any, marked_first, marked_last);
    }
    const PlantGroundLayout& layout = surface.layout;
    if (moved_any) {
        surface.positions = PlantGroundSpan{layout.position_offset + moved_first * layout.position_stride,
                                            layout.position_offset + moved_last * layout.position_stride + 12u};
        // the normal and, at stride 8, the tangent behind it: the whole slot of the last moved vertex
        surface.normals = PlantGroundSpan{layout.normal_offset + moved_first * layout.normal_stride,
                                          layout.normal_offset + (moved_last + 1u) * layout.normal_stride};
    }
    if (marked_any || moved_any) {
        // a moved vertex carries its colour and coordinates unchanged; the span covers both ranges
        const uint32_t first = marked_any && (!moved_any || marked_first < moved_first) ? marked_first : moved_first;
        const uint32_t last = marked_any && (!moved_any || marked_last > moved_last) ? marked_last : moved_last;
        surface.attributes = marked_any ? PlantGroundSpan{first * layout.attribute_stride,
                                                          (last + 1u) * layout.attribute_stride}
                                        : PlantGroundSpan{};
    }
    return true;
}

}  // namespace ase::adp::godot

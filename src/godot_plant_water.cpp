/**
 * ASE GODOT ADAPTER - THE WATER OF THE BROOKS AT THE LEVEL ASE-HYDRO RECKONS
 *
 * @file        godot_plant_water.cpp
 * @brief       The water plane in the bed of every brook at its bed plus the ase-hydro depth, and the line it runs along
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3. See godot_plant_water.hpp.
 *
 *              THE WATER STANDS WHERE ASE-HYDRO PUTS IT (build_water_table, B0281): ase-hydro reckons the
 *              depth of every brook (HydroHubStrmPubSystem), the stream port carries it, and the plane stands
 *              in the bed of each brook at its bed plus that depth. The bed of a station is the lowest ground
 *              vertex of its section within the reach of the bed (stream_levels); from the lower end upstream
 *              the level never sinks under the level below it, the rule of the ribbon, nor under the
 *              groundwater table the brook runs in (table_of). Beside a bed the plane stands at the
 *              groundwater level ase-hydro gives that brook, within the reach the stream port carries
 *              (groundwater_level_m, groundwater_reach_m; beside_bed, B0286), so a pit dug below it fills to
 *              it. Everywhere else, and beside a brook whose record carries no groundwater yet (N0484), the
 *              table stands at NN0 (GODOT_WATER_TABLE_M), the datum ase-terrain generates its ground around:
 *              B0281 had taken that table out, and the groundwater was seen nowhere, neither in the brook nor
 *              in any hollow (operator 2026-11-13, a regression). Where the ground stands
 *              above the plane it hides it, so the shore is where ground and plane cross. The plane shares the
 *              corners of the ground, one vertex per ground vertex a wet cell names, the depth of the water
 *              above that vertex in its colour; it reads the ground mesh as build_ground laid it and
 *              patch_ground moved it, never the ports by object id (P0690). A vertex within the bed of a
 *              stream record carries that stream's velocity and course (flow_at); the shader moves the
 *              ripples along it. The care view draws this plane and nothing else as its water.
 *
 *              THE LINE OF THE WATER (build_water_line, B0285) is read from the same levels: per station the
 *              centre of the ground vertices under the level, each weighted by the water over it. The stream
 *              port carries only the filed start and end of a bed, and a carved bed winds off the line between
 *              them; the HUD's brook sign lies along this line, so it stands on the water the plane draws and
 *              not beside it, where the straight line between the ends runs over the bank. Only the water the
 *              caller's eye sees counts: under a flat camera the near bank hides the near half of the bed, and
 *              the centre of all of it projected onto the bank below the visible strip.
 *
 *              THE LINE OF THE BED (build_bed_line, B0285) stands for the brook where the lip hides all its
 *              water: a bed carved 0.6 m deep holding 0.17 m of water shows the eye 1.9 m high none of it (run
 *              20261113-d2309-tri). Per station the seen ground vertices under the section's highest one, each
 *              weighted by its depth under it, so the walls and floor the eye sees carry the point.
 *
 *              The ribbon (build_water), a course followed over the terrain samples above the table, stands in
 *              godot_plant_water_ribbon.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-01
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

float channel_of(float value, float full) {
    const float share = value / full;
    return share < 0.0f ? 0.0f : (share > 1.0f ? 1.0f : share);
}

/**
 * The flow at (x, z) as the vertex colour of the table carries it: `red` the velocity share of the fastest
 * stream whose bed holds the place, `course_x` and `course_z` its unit course mapped from [-1, 1] into [0, 1].
 * The bed of a stream reaches half its width plus the swing of a winding carved bed (GODOT_WATER_SWING_SHARE of
 * the bed width) from the line between its ends. A place in no bed keeps red 0 and the course north.
 */
void flow_at(float x, float z, const std::vector<VivariumStreamView>& streams, float& red, float& course_x,
             float& course_z) {
    red = 0.0f;
    course_x = 0.5f;
    course_z = 1.0f;
    for (const VivariumStreamView& stream : streams) {
        const auto ax = static_cast<float>(stream.end_x - stream.x);
        const auto az = static_cast<float>(stream.end_z - stream.z);
        const float length2 = ax * ax + az * az;
        const auto half_m = static_cast<float>(stream.half_m);
        const float share = channel_of(static_cast<float>(stream.velocity_m_s), GODOT_WATER_VELOCITY_FULL_M_S);
        if (!(length2 > 0.0f) || !(half_m > 0.0f) || !(share > red)) {
            continue;
        }
        const float along =
            ((x - static_cast<float>(stream.x)) * ax + (z - static_cast<float>(stream.z)) * az) / length2;
        const float t = along < 0.0f ? 0.0f : (along > 1.0f ? 1.0f : along);
        const float dx = x - (static_cast<float>(stream.x) + t * ax);
        const float dz = z - (static_cast<float>(stream.z) + t * az);
        const float reach = half_m + (half_m + half_m) * GODOT_WATER_SWING_SHARE;
        if (dx * dx + dz * dz > reach * reach) {
            continue;
        }
        const float length = ase::math::sqrt(length2);
        red = share;
        course_x = 0.5f + 0.5f * ax / length;
        course_z = 0.5f + 0.5f * az / length;
    }
}

/**
 * The station of `stream` whose section holds (x, z): the place along the line between its ends, within its ends,
 * no farther from that line than half the bed width plus the swing of a winding bed (GODOT_WATER_SWING_SHARE of
 * the bed width), rounded to the nearest of the GODOT_WATER_STATIONS sections. false outside the bed, and for a
 * bed without length or width.
 */
bool station_of(const VivariumStreamView& stream, float x, float z, std::size_t& station) {
    const auto ax = static_cast<float>(stream.end_x - stream.x);
    const auto az = static_cast<float>(stream.end_z - stream.z);
    const float length2 = ax * ax + az * az;
    const auto half_m = static_cast<float>(stream.half_m);
    if (!(length2 > 0.0f) || !(half_m > 0.0f)) {
        return false;
    }
    const float along = ((x - static_cast<float>(stream.x)) * ax + (z - static_cast<float>(stream.z)) * az) / length2;
    if (along < 0.0f || along > 1.0f) {
        return false;
    }
    const float dx = x - (static_cast<float>(stream.x) + along * ax);
    const float dz = z - (static_cast<float>(stream.z) + along * az);
    const float reach = half_m + (half_m + half_m) * GODOT_WATER_SWING_SHARE;
    if (dx * dx + dz * dz > reach * reach) {
        return false;
    }
    station = static_cast<std::size_t>(ase::math::round(along * static_cast<float>(GODOT_WATER_STATIONS)));
    return true;
}

/**
 * Whether (x, z) lies within the groundwater ase-hydro reckons beside `stream`: no farther from the line between
 * its ends, held within its ends, than the reach the stream port carries (groundwater_reach_m). false for a stream
 * whose record carries no finite level or no positive reach: then ase-hydro gives no water beside that brook.
 */
bool beside_bed(const VivariumStreamView& stream, float x, float z) {
    const auto reach = static_cast<float>(stream.groundwater_reach_m);
    if (!std::isfinite(stream.groundwater_level_m) || !std::isfinite(stream.groundwater_reach_m) || !(reach > 0.0f)) {
        return false;
    }
    const auto ax = static_cast<float>(stream.end_x - stream.x);
    const auto az = static_cast<float>(stream.end_z - stream.z);
    const float length2 = ax * ax + az * az;
    const float along =
        length2 > 0.0f ? ((x - static_cast<float>(stream.x)) * ax + (z - static_cast<float>(stream.z)) * az) / length2
                       : 0.0f;
    const float t = along < 0.0f ? 0.0f : (along > 1.0f ? 1.0f : along);
    const float dx = x - (static_cast<float>(stream.x) + t * ax);
    const float dz = z - (static_cast<float>(stream.z) + t * az);
    return dx * dx + dz * dz <= reach * reach;
}

/**
 * The groundwater table under `stream`, world metres: the level its record carries, or NN0
 * (GODOT_WATER_TABLE_M, the datum ase-terrain generates its ground around) while the stream port carries none
 * (N0484). The water of a bed never stands below it: the brook is fed by the groundwater it runs in, and the
 * table at NN0 stood in the brook and in every hollow below it until B0281 took it out (operator 2026-11-13:
 * the groundwater NN0 is seen nowhere any more, neither in the brook nor anywhere, a regression).
 */
float table_of(const VivariumStreamView& stream) {
    return std::isfinite(stream.groundwater_level_m) ? static_cast<float>(stream.groundwater_level_m)
                                                     : GODOT_WATER_TABLE_M;
}

/**
 * The water level of `stream` at its GODOT_WATER_STATIONS + 1 stations, written from `first` on into `levels`
 * and `reached`: the bed of a station is the lowest vertex of `ground` its section holds (station_of), the level
 * that bed plus the depth ase-hydro reckons for the stream. The water runs toward the lower of the two reached
 * ends; from that end upstream the level is the bed plus the depth or the level below it, whichever is higher,
 * so a bump of the bed dams the water behind it, the rule of build_water. A station no vertex falls into keeps
 * reached 0. A dry stream reaches no station: it holds no water.
 */
void stream_levels(const VivariumStreamView& stream, const PlantMeshArrays& ground, std::size_t first,
                   std::vector<float>& levels, std::vector<uint8_t>& reached) {
    const std::size_t stations = static_cast<std::size_t>(GODOT_WATER_STATIONS) + 1u;
    const auto depth_m = static_cast<float>(stream.depth_m);
    if (!(depth_m > 0.0f)) {
        return;
    }
    const std::size_t vertices = ground.positions.size() / 3u;
    for (std::size_t v = 0u; v < vertices; ++v) {
        std::size_t station = 0u;
        if (!station_of(stream, ground.positions[v * 3u], ground.positions[v * 3u + 2u], station)) {
            continue;
        }
        const float height = ground.positions[v * 3u + 1u];
        if (reached[first + station] == 0u || height < levels[first + station]) {
            levels[first + station] = height;
            reached[first + station] = 1u;
        }
    }
    std::size_t head = stations;
    std::size_t tail = stations;
    for (std::size_t station = 0u; station < stations; ++station) {
        if (reached[first + station] != 0u) {
            head = head == stations ? station : head;
            tail = station;
        }
    }
    const bool falls_to_end = head == stations || levels[first + tail] <= levels[first + head];
    float below = 0.0f;
    bool wet = false;
    for (std::size_t step = 0u; step < stations; ++step) {
        const std::size_t station = falls_to_end ? stations - 1u - step : step;
        if (reached[first + station] == 0u) {
            wet = false;
            continue;
        }
        const float level = levels[first + station] + depth_m;
        below = wet && below > level ? below : level;
        // the brook stands at least at the groundwater table it runs in (table_of)
        const float table = table_of(stream);
        levels[first + station] = below > table ? below : table;
        wet = true;
    }
}

}  // namespace

uint32_t build_water_table(const std::vector<VivariumStreamView>& streams, const PlantMeshArrays& ground,
                           PlantMeshArrays& out) {
    // the level ase-hydro gives every station of every stream, stream after stream
    const std::size_t stations = static_cast<std::size_t>(GODOT_WATER_STATIONS) + 1u;
    std::vector<float> levels(streams.size() * stations, 0.0f);
    std::vector<uint8_t> reached(streams.size() * stations, 0u);
    for (std::size_t stream = 0u; stream < streams.size(); ++stream) {
        stream_levels(streams[stream], ground, stream * stations, levels, reached);
    }
    // the level over every ground vertex: the highest of the streams whose bed holds it; flooded and in_bed 0 in
    // no bed. in_bed stays apart from flooded: the table below floods every vertex, the bank rule of the cells
    // reads which corners a bed holds
    const std::size_t ground_vertices = ground.positions.size() / 3u;
    std::vector<float> level_of(ground_vertices, 0.0f);
    std::vector<uint8_t> flooded(ground_vertices, 0u);
    std::vector<uint8_t> in_bed(ground_vertices, 0u);
    for (std::size_t v = 0u; v < ground_vertices; ++v) {
        for (std::size_t stream = 0u; stream < streams.size(); ++stream) {
            std::size_t station = 0u;
            if (!station_of(streams[stream], ground.positions[v * 3u], ground.positions[v * 3u + 2u], station) ||
                reached[stream * stations + station] == 0u) {
                continue;
            }
            const float level = levels[stream * stations + station];
            if (flooded[v] == 0u || level > level_of[v]) {
                level_of[v] = level;
                flooded[v] = 1u;
                in_bed[v] = 1u;
            }
        }
    }
    // the groundwater beside every bed: a vertex in no bed, within the reach ase-hydro gives a stream, takes the
    // highest groundwater level of those streams, so a pit dug below that level fills to it (usecase:101339,
    // B0286); the ground around the pit stands above the plane and hides it. Everywhere else, and beside a brook
    // whose record carries no groundwater (N0484), the table stands at NN0 (GODOT_WATER_TABLE_M), so every
    // hollow below NN0 holds water as before B0281
    for (std::size_t v = 0u; v < ground_vertices; ++v) {
        if (flooded[v] != 0u) {
            continue;
        }
        level_of[v] = GODOT_WATER_TABLE_M;
        flooded[v] = 1u;
        bool carried = false;
        for (const VivariumStreamView& stream : streams) {
            if (!beside_bed(stream, ground.positions[v * 3u], ground.positions[v * 3u + 2u])) {
                continue;
            }
            const auto level = static_cast<float>(stream.groundwater_level_m);
            if (!carried || level > level_of[v]) {
                level_of[v] = level;
                carried = true;
            }
        }
    }
    // one plane vertex per ground vertex a wet cell names, laid when the first cell names it; -1 not yet laid
    std::vector<int32_t> vertex_of(ground_vertices, -1);
    uint32_t drawn = 0u;
    for (std::size_t first = 0u; first + 6u <= ground.indices.size(); first += 6u) {
        // the cell's corners sw, se, ne, nw out of its triangles (sw, se, ne) and (sw, ne, nw)
        const int32_t corners[4] = {ground.indices[first], ground.indices[first + 1u], ground.indices[first + 2u],
                                    ground.indices[first + 5u]};
        bool known = true;
        bool wet = false;
        float cell_level = 0.0f;
        bool cell_bed = false;
        for (const int32_t corner : corners) {
            if (corner < 0 || static_cast<std::size_t>(corner) >= ground_vertices) {
                known = false;
                break;
            }
            const auto at = static_cast<std::size_t>(corner);
            if (flooded[at] == 0u) {
                continue;
            }
            wet = wet || ground.positions[at * 3u + 1u] < level_of[at];
            if (in_bed[at] != 0u) {
                cell_level = cell_bed && cell_level < level_of[at] ? cell_level : level_of[at];
                cell_bed = true;
            }
        }
        if (!known || !wet) {
            continue;  // an index past the ground, or a cell in no bed or wholly above its water: no water over it
        }
        int32_t vertex[4] = {0, 0, 0, 0};
        for (std::size_t corner = 0u; corner < 4u; ++corner) {
            const auto at = static_cast<std::size_t>(corners[corner]);
            if (vertex_of[at] >= 0) {
                vertex[corner] = vertex_of[at];
                continue;
            }
            const float x = ground.positions[at * 3u];
            const float height = ground.positions[at * 3u + 1u];
            const float z = ground.positions[at * 3u + 2u];
            // a corner on the bank, outside every bed, carries the lowest water of the cell's bed corners, so no
            // bank is flooded by the level of the station upstream and the brook does not sink to the table across
            // its own cell; where its groundwater stands higher it carries that. Its ground above hides it
            const bool bank = in_bed[at] == 0u && cell_bed && cell_level > level_of[at];
            const float level = bank ? cell_level : level_of[at];
            const float green = channel_of(level - height, GODOT_WATER_DEPTH_FULL_M);
            float red = 0.0f;
            float course_x = 0.0f;
            float course_z = 0.0f;
            flow_at(x, z, streams, red, course_x, course_z);
            vertex[corner] = static_cast<int32_t>(out.positions.size() / 3u);
            vertex_of[at] = vertex[corner];
            append3(out.positions, x, level, z);
            append3(out.normals, 0.0f, 1.0f, 0.0f);
            out.uvs.push_back(x);
            out.uvs.push_back(z);
            out.colors.insert(out.colors.end(), {red, green, course_x, course_z});
        }
        // the two triangles in the order the cell names its corners, the front face looking up as the ground's
        out.indices.insert(out.indices.end(), {vertex[0], vertex[1], vertex[2], vertex[0], vertex[2], vertex[3]});
        drawn += 1u;
    }
    return drawn;
}

uint32_t build_water_line(const VivariumStreamView& stream, const PlantMeshArrays& ground,
                          bool (*seen)(const void* context, float x, float y, float z), const void* context,
                          std::vector<float>& out) {
    // the levels of the table at the stream's stations, the rule build_water_table lays its water by
    const std::size_t stations = static_cast<std::size_t>(GODOT_WATER_STATIONS) + 1u;
    std::vector<float> levels(stations, 0.0f);
    std::vector<uint8_t> reached(stations, 0u);
    stream_levels(stream, ground, 0u, levels, reached);
    // per station the vertices under its level, each weighted by the water over it
    std::vector<float> weights(stations, 0.0f);
    std::vector<float> sum_x(stations, 0.0f);
    std::vector<float> sum_z(stations, 0.0f);
    const std::size_t vertices = ground.positions.size() / 3u;
    for (std::size_t v = 0u; v < vertices; ++v) {
        const float x = ground.positions[v * 3u];
        const float z = ground.positions[v * 3u + 2u];
        std::size_t station = 0u;
        if (!station_of(stream, x, z, station) || reached[station] == 0u) {
            continue;
        }
        const float water = levels[station] - ground.positions[v * 3u + 1u];
        if (!(water > 0.0f)) {
            continue;
        }
        // the water the bank hides from the eye lays no part of the line
        if (seen != nullptr && !seen(context, x, levels[station], z)) {
            continue;
        }
        weights[station] += water;
        sum_x[station] += water * x;
        sum_z[station] += water * z;
    }
    uint32_t laid = 0u;
    for (std::size_t station = 0u; station < stations; ++station) {
        if (!(weights[station] > 0.0f)) {
            continue;  // no vertex of the section under its level: no water stands there
        }
        append3(out, sum_x[station] / weights[station], levels[station], sum_z[station] / weights[station]);
        laid += 1u;
    }
    return laid;
}

uint32_t build_bed_line(const VivariumStreamView& stream, const PlantMeshArrays& ground,
                        bool (*seen)(const void* context, float x, float y, float z), const void* context,
                        std::vector<float>& out) {
    // the highest vertex of every section: the bank the bed is carved into
    const std::size_t stations = static_cast<std::size_t>(GODOT_WATER_STATIONS) + 1u;
    std::vector<float> banks(stations, 0.0f);
    std::vector<uint8_t> reached(stations, 0u);
    const std::size_t vertices = ground.positions.size() / 3u;
    for (std::size_t v = 0u; v < vertices; ++v) {
        std::size_t station = 0u;
        if (!station_of(stream, ground.positions[v * 3u], ground.positions[v * 3u + 2u], station)) {
            continue;
        }
        const float height = ground.positions[v * 3u + 1u];
        if (reached[station] == 0u || height > banks[station]) {
            banks[station] = height;
            reached[station] = 1u;
        }
    }
    // per station the seen vertices under the bank, each weighted by how deep the bed lies there
    std::vector<float> weights(stations, 0.0f);
    std::vector<float> sum_x(stations, 0.0f);
    std::vector<float> sum_y(stations, 0.0f);
    std::vector<float> sum_z(stations, 0.0f);
    for (std::size_t v = 0u; v < vertices; ++v) {
        const float x = ground.positions[v * 3u];
        const float y = ground.positions[v * 3u + 1u];
        const float z = ground.positions[v * 3u + 2u];
        std::size_t station = 0u;
        if (!station_of(stream, x, z, station) || reached[station] == 0u) {
            continue;
        }
        const float deep = banks[station] - y;
        if (!(deep > 0.0f)) {
            continue;
        }
        // the bed the bank hides from the eye lays no part of the line
        if (seen != nullptr && !seen(context, x, y, z)) {
            continue;
        }
        weights[station] += deep;
        sum_x[station] += deep * x;
        sum_y[station] += deep * y;
        sum_z[station] += deep * z;
    }
    uint32_t laid = 0u;
    for (std::size_t station = 0u; station < stations; ++station) {
        if (!(weights[station] > 0.0f)) {
            continue;  // no seen vertex of the section under its bank: no bed the eye sees there
        }
        append3(out, sum_x[station] / weights[station], sum_y[station] / weights[station],
                sum_z[station] / weights[station]);
        laid += 1u;
    }
    return laid;
}

}  // namespace ase::adp::godot

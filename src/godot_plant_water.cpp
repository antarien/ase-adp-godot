/**
 * ASE GODOT ADAPTER - THE WATER OF THE BROOKS AT THE LEVEL ASE-HYDRO RECKONS
 *
 * @file        godot_plant_water.cpp
 * @brief       The water plane in the bed of every brook at its bed plus the ase-hydro depth, and the ribbon along a course
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3. See godot_plant_water.hpp.
 *
 *              THE WATER STANDS WHERE ASE-HYDRO PUTS IT (build_water_table, B0281): ase-hydro reckons the
 *              depth of every brook (HydroHubStrmPubSystem), the stream port carries it, and the plane stands
 *              in the bed of each brook at its bed plus that depth. The bed of a station is the lowest ground
 *              vertex of its section within the reach of the bed (stream_levels); from the lower end upstream
 *              the level never sinks under the level below it, the rule of the ribbon. A place in no bed holds
 *              no water: the view reckons no water of its own, no table at a datum. Where the ground stands
 *              above the plane it hides it, so the shore is where ground and plane cross. The plane shares the
 *              corners of the ground, one vertex per ground vertex a wet cell names, the depth of the water
 *              above that vertex in its colour; it reads the ground mesh as build_ground laid it and
 *              patch_ground moved it, never the ports by object id (P0690). A vertex within the bed of a
 *              stream record carries that stream's velocity and course (flow_at); the shader moves the
 *              ripples along it. The care view draws this plane and nothing else as its water.
 *
 *              THE RIBBON (build_water) is the water of a river running along a course ABOVE the table, a brook
 *              falling from a mountain: the care view lays none now, the vivarium has no such river.
 *
 *              THE RIBBON FOLLOWS THE DEEPEST LINE OF THE BED: a carved bed winds (TerrainMutCarvSystem), the
 *              stream port carries only its filed start and end. Station by station the ribbon reads the cross
 *              section of the ground port around the line it followed so far, takes its lowest sample as the
 *              bed and moves its middle to the centre of the wetted cross section, the samples below the bed
 *              plus the depth, each weighted by the water standing over it. The sides stand normal to the
 *              course the middles draw, not to the filed line.
 *
 *              THE SURFACE STANDS ON THE GROUND OF THE TERRAIN PORT: the same samples carry the ground mesh and
 *              the bed of every station, so ground and water share one height datum. The water runs toward the
 *              lower of the two reached ends of the bed, whichever end was filed first. From that end upstream
 *              the surface is the bed of the station plus the depth, or the surface below it where that stands
 *              higher: a bump of the bed dams the water behind it and never stands over it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-01
 * @modified    2026-11-13
 * @version     00.00.06.00000
 */

#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

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
 * The height of the bed at a station: the lowest sample within the half width, so a bank beside a narrow bed
 * is not taken for it, else the nearest sample. false while no sample stands within `reach_m` of the station:
 * the ground port does not reach it.
 */
bool bed_height_at(float x, float z, float half_m, float reach_m, const std::vector<VivariumTerrainView>& samples,
                   float& height) {
    float nearest = -1.0f;
    float nearest_height = 0.0f;
    bool in_bed = false;
    float lowest = 0.0f;
    for (const VivariumTerrainView& sample : samples) {
        const float dx = static_cast<float>(sample.x) - x;
        const float dz = static_cast<float>(sample.z) - z;
        const float distance2 = dx * dx + dz * dz;
        const auto sample_height = static_cast<float>(sample.height);
        if (nearest < 0.0f || distance2 < nearest) {
            nearest = distance2;
            nearest_height = sample_height;
        }
        if (distance2 <= half_m * half_m && (!in_bed || sample_height < lowest)) {
            in_bed = true;
            lowest = sample_height;
        }
    }
    if (nearest < 0.0f || nearest > reach_m * reach_m) {
        return false;
    }
    height = in_bed ? lowest : nearest_height;
    return true;
}

/**
 * The cross section of the bed at the station (x, z): the samples less than half a sample step along the filed
 * course (unit vector ux, uz) from the station and less than `window_m` across it (unit vector nx, nz) from the
 * middle `lateral` the ribbon followed so far. `floor` becomes the lowest of them; `lateral` moves to the centre
 * of the wetted section, the samples below floor + depth_m weighted by the water over them, and stays within
 * `limit_m` of the filed course. false while the section holds no sample.
 */
bool follow_section(float x, float z, float ux, float uz, float nx, float nz, float step_m, float window_m,
                    float depth_m, float limit_m, const std::vector<VivariumTerrainView>& samples, float& lateral,
                    float& floor) {
    const float half_step = step_m * 0.5f;
    bool found = false;
    for (const VivariumTerrainView& sample : samples) {
        const float dx = static_cast<float>(sample.x) - x;
        const float dz = static_cast<float>(sample.z) - z;
        const float along = dx * ux + dz * uz;
        const float across = dx * nx + dz * nz;
        if (ase::math::abs(along) > half_step || ase::math::abs(across - lateral) > window_m) {
            continue;
        }
        const auto sample_height = static_cast<float>(sample.height);
        if (!found || sample_height < floor) {
            floor = sample_height;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    const float level = floor + depth_m;
    float weight_sum = 0.0f;
    float weighted = 0.0f;
    for (const VivariumTerrainView& sample : samples) {
        const float dx = static_cast<float>(sample.x) - x;
        const float dz = static_cast<float>(sample.z) - z;
        const float along = dx * ux + dz * uz;
        const float across = dx * nx + dz * nz;
        const float water = level - static_cast<float>(sample.height);
        if (ase::math::abs(along) > half_step || ase::math::abs(across - lateral) > window_m || !(water > 0.0f)) {
            continue;
        }
        weight_sum += water;
        weighted += water * across;
    }
    if (weight_sum > 0.0f) {
        lateral = ase::math::clamp(weighted / weight_sum, -limit_m, limit_m);
    }
    return true;
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
        levels[first + station] = below;
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
    // the level over every ground vertex: the highest of the streams whose bed holds it; flooded 0 in no bed
    const std::size_t ground_vertices = ground.positions.size() / 3u;
    std::vector<float> level_of(ground_vertices, 0.0f);
    std::vector<uint8_t> flooded(ground_vertices, 0u);
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
        bool cell_flooded = false;
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
            cell_level = cell_flooded && cell_level < level_of[at] ? cell_level : level_of[at];
            cell_flooded = true;
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
            // bank is flooded by the level of the station upstream; its ground above that water hides it
            const float level = flooded[at] != 0u ? level_of[at] : cell_level;
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

uint32_t build_water(const std::vector<VivariumStreamView>& streams, const std::vector<VivariumTerrainView>& samples,
                     float step_m, PlantMeshArrays& out) {
    uint32_t drawn = 0u;
    const std::size_t stations = static_cast<std::size_t>(GODOT_WATER_STATIONS) + 1u;
    std::vector<float> middle_x(stations, 0.0f);
    std::vector<float> middle_z(stations, 0.0f);
    std::vector<float> beds(stations, 0.0f);
    std::vector<float> surfaces(stations, 0.0f);
    std::vector<uint8_t> reached(stations, 0u);
    for (const VivariumStreamView& stream : streams) {
        const auto ax = static_cast<float>(stream.end_x - stream.x);
        const auto az = static_cast<float>(stream.end_z - stream.z);
        const float length_m = ase::math::sqrt(ax * ax + az * az);
        const auto half_m = static_cast<float>(stream.half_m);
        const auto depth_m = static_cast<float>(stream.depth_m);
        if (!(length_m > 0.0f) || !(half_m > 0.0f) || !(depth_m > 0.0f)) {
            continue;  // no bed or a dry one: nothing stands in it
        }
        const float ux = ax / length_m;
        const float uz = az / length_m;
        // across the bed, so that (start - side, start + side, end + side) turns up as the ground cells do
        const float nx = uz;
        const float nz = -ux;
        const float spacing_m = length_m / static_cast<float>(GODOT_WATER_STATIONS);
        const float window_m = half_m + spacing_m + step_m;
        const float limit_m = (half_m + half_m) * GODOT_WATER_SWING_SHARE;
        const float red = channel_of(static_cast<float>(stream.velocity_m_s), GODOT_WATER_VELOCITY_FULL_M_S);
        const float green = channel_of(depth_m, GODOT_WATER_DEPTH_FULL_M);

        // the middles of the ribbon: the filed course, moved across onto the deepest line of the bed
        float lateral = 0.0f;
        for (std::size_t station = 0u; station < stations; ++station) {
            const float along = static_cast<float>(station) / static_cast<float>(GODOT_WATER_STATIONS);
            const float x = static_cast<float>(stream.x) + along * ax;
            const float z = static_cast<float>(stream.z) + along * az;
            float bed = 0.0f;
            reached[station] =
                bed_height_at(x + nx * lateral, z + nz * lateral, half_m, step_m + half_m, samples, bed) ? 1u : 0u;
            if (reached[station] == 0u) {
                continue;
            }
            float floor = bed;
            if (follow_section(x, z, ux, uz, nx, nz, step_m, window_m, depth_m, limit_m, samples, lateral, floor)) {
                bed = floor;
            }
            middle_x[station] = x + nx * lateral;
            middle_z[station] = z + nz * lateral;
            beds[station] = bed;
        }

        // the direction of fall: toward the lower of the two reached ends of the bed
        std::size_t head = stations;
        std::size_t tail = stations;
        for (std::size_t station = 0u; station < stations; ++station) {
            if (reached[station] != 0u) {
                head = head == stations ? station : head;
                tail = station;
            }
        }
        const bool falls_to_end = head == stations || beds[tail] <= beds[head];
        // water never runs uphill: from the mouth upstream the surface stands at its own bed plus the depth or at
        // the surface below it, whichever is higher - it never sinks under the ground of its station
        float below = 0.0f;
        bool wet = false;
        for (std::size_t step = 0u; step < stations; ++step) {
            const std::size_t station = falls_to_end ? stations - 1u - step : step;
            if (reached[station] == 0u) {
                wet = false;
                continue;
            }
            const float level = beds[station] + depth_m;
            below = wet && below > level ? below : level;
            surfaces[station] = below;
            wet = true;
        }

        bool flowing = false;
        bool ribbon = false;
        for (std::size_t station = 0u; station < stations; ++station) {
            if (reached[station] == 0u) {
                flowing = false;
                continue;
            }
            // the course at the station: from the middle before it to the middle after it, where they are reached
            const std::size_t back = station > 0u && reached[station - 1u] != 0u ? station - 1u : station;
            const std::size_t ahead = station + 1u < stations && reached[station + 1u] != 0u ? station + 1u : station;
            float tx = middle_x[ahead] - middle_x[back];
            float tz = middle_z[ahead] - middle_z[back];
            float course = ase::math::sqrt(tx * tx + tz * tz);
            if (!(course > 0.0f)) {
                tx = ux;
                tz = uz;
                course = 1.0f;
            }
            const float side_x = tz / course * half_m;
            const float side_z = -tx / course * half_m;
            const float surface = surfaces[station];
            const float along = static_cast<float>(station) / static_cast<float>(GODOT_WATER_STATIONS);
            const auto first = static_cast<int32_t>(out.positions.size() / 3u);
            for (const float sign : {-1.0f, 1.0f}) {
                append3(out.positions, middle_x[station] + sign * side_x, surface, middle_z[station] + sign * side_z);
                append3(out.normals, 0.0f, 1.0f, 0.0f);
                out.uvs.push_back(sign < 0.0f ? 0.0f : 1.0f);
                out.uvs.push_back(along * length_m);
                out.colors.insert(out.colors.end(), {red, green, 0.5f, 1.0f});  // the ribbon's UV runs along its course
            }
            if (flowing) {
                const int32_t sw = first - 2;
                const int32_t se = first - 1;
                const int32_t ne = first + 1;
                const int32_t nw = first;
                out.indices.insert(out.indices.end(), {sw, se, ne, sw, ne, nw});
                ribbon = true;
            }
            flowing = true;
        }
        drawn += ribbon ? 1u : 0u;
    }
    return drawn;
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - THE WATER TABLE AT NN0 AND THE WATER ALONG A COURSE
 *
 * @file        godot_plant_water.cpp
 * @brief       The one water plane at NN0 over every ground cell below it, and the ribbon of a river along a course
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3. See godot_plant_water.hpp.
 *
 *              THE WATER STANDS AT NN0 (build_water_table): the terrain is generated around NN0
 *              (GEN_DEFAULT_BASE_HEIGHT of ase-terrain), and every ground cell with a corner below it carries one
 *              quad of the plane at GODOT_WATER_TABLE_M. A dug hole, a carved bed and a hollow of the generated
 *              ground fill alike; where the ground stands above the plane it hides it, so the shore is where
 *              ground and plane cross. The plane shares the corners of the ground, one vertex per terrain sample
 *              a wet cell names, the depth of the table above that sample in its colour. A vertex within the bed
 *              of a stream record carries that stream's velocity and course (flow_at); the shader moves the
 *              ripples along it, and outside every bed the plane stands still. The care view draws this plane
 *              and nothing else as its water.
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
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-01
 * @modified    2026-11-05
 * @version     00.00.04.00000
 */

#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
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

}  // namespace

uint32_t build_water_table(const std::vector<VivariumStreamView>& streams,
                           const std::vector<VivariumTerrainView>& samples,
                           const std::vector<VivariumGroundCellView>& cells, PlantMeshArrays& out) {
    // the samples by object id, as the cells name their corners
    std::unordered_map<uint32_t, std::size_t> sample_of;
    sample_of.reserve(samples.size());
    for (std::size_t index = 0u; index < samples.size(); ++index) {
        sample_of.emplace(samples[index].object_id, index);
    }
    // one plane vertex per sample a wet cell names, laid when the first cell names it
    std::unordered_map<uint32_t, int32_t> vertex_of;
    uint32_t drawn = 0u;
    for (const VivariumGroundCellView& cell : cells) {
        const uint32_t corners[4] = {cell.sw, cell.se, cell.ne, cell.nw};
        std::size_t at[4] = {0u, 0u, 0u, 0u};
        bool known = true;
        bool wet = false;
        for (std::size_t corner = 0u; corner < 4u; ++corner) {
            const auto found = sample_of.find(corners[corner]);
            if (found == sample_of.end()) {
                known = false;
                break;
            }
            at[corner] = found->second;
            wet = wet || static_cast<float>(samples[found->second].height) < GODOT_WATER_TABLE_M;
        }
        if (!known || !wet) {
            continue;  // a cell the port does not close, or one wholly above the table: no water over it
        }
        int32_t vertex[4] = {0, 0, 0, 0};
        for (std::size_t corner = 0u; corner < 4u; ++corner) {
            const VivariumTerrainView& sample = samples[at[corner]];
            const auto laid = vertex_of.find(sample.object_id);
            if (laid != vertex_of.end()) {
                vertex[corner] = laid->second;
                continue;
            }
            const auto x = static_cast<float>(sample.x);
            const auto z = static_cast<float>(sample.z);
            const float green =
                channel_of(GODOT_WATER_TABLE_M - static_cast<float>(sample.height), GODOT_WATER_DEPTH_FULL_M);
            float red = 0.0f;
            float course_x = 0.0f;
            float course_z = 0.0f;
            flow_at(x, z, streams, red, course_x, course_z);
            vertex[corner] = static_cast<int32_t>(out.positions.size() / 3u);
            vertex_of.emplace(sample.object_id, vertex[corner]);
            append3(out.positions, x, GODOT_WATER_TABLE_M, z);
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

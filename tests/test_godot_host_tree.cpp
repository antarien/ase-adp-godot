/**
 * ASE ADAPTER TESTS - GODOT HOST GROWN TREE
 *
 * @file        test_godot_host_tree.cpp
 * @design      DSGN_066
 * @brief       Pins the bark tube of a birch grown to MADURA as the bark port hands it to the view
 * @description Register B0295 point 4: one continuous tube per axis. A seed is poured, ase-time sets the clock
 *              of the host to the game hour ase-plant forecasts for MADURA, and the bark port is read as the
 *              view reads it (tubes_by_body takes the same records). The case walks the stem from ring to ring
 *              along the links of the port: every tip ring stands one internode length above the ring below it
 *              along that internode's own axis, every internode of the stem carries the one wave of its axis,
 *              and the share of the tip displacement grows from ring to ring up the stem as the deflection
 *              line of a beam does. A share that falls between two neighbouring rings swings them apart.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside the others of the bundle.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.01.00001
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::environment;
using test::patch_at;
using test::vivarium_config;

namespace {

/** The centre of one ring and the organ it closes. */
struct RingCentre {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    uint32_t count = 0u;
    uint32_t below_organ = GODOT_LINK_NONE;  // the organ of the ring below, GODOT_LINK_NONE at a foot ring
};

}  // namespace

TEST_CASE("B0295 point 4: the stem of a birch at MADURA is one tube, its rings meet along the axis and swing as one beam") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    // The seed of place 0 sprouts after a pour, and ase-plant forecasts the hour of MADURA on the patch record.
    advance_frames(resources, 8u);
    const uint32_t cared = patch_at(resources, 0u).object_id;
    // The camera stands before the patch: only a body inside a detail band around the camera shows its organs
    // (ase-pl-flora FloraHubPartSyncSystem reads RND_LOD_BAND of ase-render-data). Without a camera the shoot
    // grows and never reaches the organ and bark ports.
    const VivariumPatchView& seen = patch_at(resources, 0u);
    REQUIRE(resources.view(seen.x, seen.y + 3.0, seen.z + 6.0, seen.x, seen.y, seen.z) == ase::kernel::HostStatusOk);
    REQUIRE(resources.irrigate(cared, vivarium_config(env.bundle).irrigate_amount_mm) == ase::kernel::HostStatusOk);
    constexpr uint32_t kBoundFrames = 256u;
    for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_organs(GODOT_ORGAN_SEGMENT).empty(); ++frame) {
        advance_frames(resources, 1u);
    }
    REQUIRE(!resources.get_organs(GODOT_ORGAN_SEGMENT).empty());
    for (uint32_t frame = 0u; frame < kBoundFrames && !(patch_at(resources, 0u).mtur_at_h > 0.0); ++frame) {
        advance_frames(resources, 1u);
    }
    const double mature_h = patch_at(resources, 0u).mtur_at_h;
    REQUIRE(mature_h > 0.0);

    // The jump: ase-time sets the clock to that hour in one write, every module computes the span up to it.
    REQUIRE(resources.seek(1.0, mature_h, GODOT_SEEK_FORM_NONE, GODOT_SEEK_FORM_NONE) == ase::kernel::HostStatusOk);
    uint32_t frames = 0u;
    for (; frames < kBoundFrames; ++frames) {
        advance_frames(resources, 1u);
        const VivariumClockView& clock = resources.get_clock();
        if (clock.seconds_per_hour > 0.0 && clock.elapsed_s / clock.seconds_per_hour >= mature_h) {
            break;
        }
    }
    // the bark port pages its vertices: it stands when its size held over a whole Dissemination run and more
    constexpr uint32_t kStillFrames = 16u;
    std::size_t bark_size = 0u;
    uint32_t still = 0u;
    for (uint32_t frame = 0u; frame < kBoundFrames && still < kStillFrames; ++frame) {
        advance_frames(resources, 1u);
        const std::size_t size = resources.get_bark().size();
        still = size == bark_size && size > 0u ? still + 1u : 0u;
        bark_size = size;
    }
    CAPTURE(mature_h);
    CAPTURE(frames);
    REQUIRE(still >= kStillFrames);

    const std::vector<VivariumOrganView>& internodes = resources.get_organs(GODOT_ORGAN_SEGMENT);
    const std::vector<VivariumBarkVertexView>& bark = resources.get_bark();
    std::unordered_map<uint32_t, const VivariumOrganView*> organ_of;
    for (const VivariumOrganView& organ : internodes) {
        organ_of.emplace(organ.object_id, &organ);
    }
    std::unordered_map<uint32_t, const VivariumBarkVertexView*> vertex_of;
    for (const VivariumBarkVertexView& vertex : bark) {
        vertex_of.emplace(vertex.object_id, &vertex);
    }

    // the ring of every internode of the stem: its tip ring, and the foot ring of the first one
    std::unordered_map<uint32_t, RingCentre> tip_of;
    RingCentre foot;
    double top_y = 0.0;
    for (const VivariumBarkVertexView& vertex : bark) {
        if (vertex.body != cared) {
            continue;
        }
        top_y = ase::math::max(top_y, vertex.y);
        const auto organ = organ_of.find(vertex.organ);
        if (organ == organ_of.end() || organ->second->order != 0u) {
            continue;
        }
        RingCentre& ring = vertex.below == GODOT_LINK_NONE ? foot : tip_of[vertex.organ];
        ring.x += vertex.x;
        ring.y += vertex.y;
        ring.z += vertex.z;
        ++ring.count;
        if (vertex.below != GODOT_LINK_NONE) {
            const auto below = vertex_of.find(vertex.below);
            if (below != vertex_of.end() && below->second->organ != vertex.organ) {
                ring.below_organ = below->second->organ;
            }
        }
    }
    REQUIRE(foot.count > 0u);
    REQUIRE(!tip_of.empty());
    for (auto& entry : tip_of) {
        entry.second.x /= entry.second.count;
        entry.second.y /= entry.second.count;
        entry.second.z /= entry.second.count;
    }
    foot.x /= foot.count;
    foot.y /= foot.count;
    foot.z /= foot.count;

    // the stem from its first internode up: the internode whose ring below is the foot ring has no stem parent
    std::unordered_map<uint32_t, uint32_t> child_of;
    uint32_t first = GODOT_LINK_NONE;
    for (const auto& entry : tip_of) {
        if (entry.second.below_organ == GODOT_LINK_NONE) {
            first = entry.first;
        } else {
            child_of.emplace(entry.second.below_organ, entry.first);
        }
    }
    REQUIRE(first != GODOT_LINK_NONE);

    double widest = 0.0;
    double worst_offset = 0.0;
    double worst_kink_deg = 0.0;
    double fallen_share = 0.0;
    uint32_t links = 0u;
    uint32_t wave_breaks = 0u;
    const VivariumOrganView* below_organ = nullptr;
    double below_x = foot.x;
    double below_y = foot.y;
    double below_z = foot.z;
    double below_share = organ_of.at(first)->sway_base;
    double below_ax = 0.0;
    double below_ay = 0.0;
    double below_az = 0.0;
    for (uint32_t at = first; at != GODOT_LINK_NONE;) {
        const VivariumOrganView& organ = *organ_of.at(at);
        const RingCentre& ring = tip_of.at(at);
        widest = ase::math::max(widest, organ.radius);
        // the axis of the internode: the template's y turned by its rotation
        const double ax = 2.0 * (organ.qx * organ.qy - organ.qw * organ.qz);
        const double ay = 1.0 - 2.0 * (organ.qx * organ.qx + organ.qz * organ.qz);
        const double az = 2.0 * (organ.qy * organ.qz + organ.qw * organ.qx);
        const double dx = ring.x - below_x;
        const double dy = ring.y - below_y;
        const double dz = ring.z - below_z;
        const double along = dx * ax + dy * ay + dz * az;
        const double px = dx - along * ax;
        const double py = dy - along * ay;
        const double pz = dz - along * az;
        worst_offset = ase::math::max(
            worst_offset, static_cast<double>(ase::math::sqrt(static_cast<float>(px * px + py * py + pz * pz))));
        if (below_organ != nullptr) {
            const double turn = ase::math::clamp(ax * below_ax + ay * below_ay + az * below_az, -1.0, 1.0);
            worst_kink_deg = ase::math::max(
                worst_kink_deg, static_cast<double>(ase::math::acos(static_cast<float>(turn))) * 180.0 / ase::math::PI);
            if (organ.sway_freq != below_organ->sway_freq || organ.sway_mean_x != below_organ->sway_mean_x ||
                organ.sway_mean_z != below_organ->sway_mean_z) {
                ++wave_breaks;
            }
        }
        fallen_share = ase::math::max(fallen_share, below_share - organ.sway_top);
        below_share = organ.sway_top;
        below_x = ring.x;
        below_y = ring.y;
        below_z = ring.z;
        below_ax = ax;
        below_ay = ay;
        below_az = az;
        below_organ = &organ;
        ++links;
        const auto child = child_of.find(at);
        at = child == child_of.end() ? GODOT_LINK_NONE : child->second;
    }
    const double height = top_y - patch_at(resources, 0u).y;
    MESSAGE("stem internodes=" << links << " of " << tip_of.size() << " height_m=" << height
                               << " widest_radius_m=" << widest << " worst_offset_m=" << worst_offset
                               << " worst_kink_deg=" << worst_kink_deg << " wave_breaks=" << wave_breaks
                               << " fallen_share=" << fallen_share << " bark_vertices=" << bark.size());
    CHECK(links == tip_of.size());     // one chain: every stem ring hangs on the one below it
    CHECK(worst_offset < 1.0e-3);      // each tip ring stands along its own internode above the ring below
    CHECK(wave_breaks == 0u);          // the stem swings as one beam: one wave on every internode of the axis
    CHECK(fallen_share <= 1.0e-6);     // up the stem the share of the tip displacement never falls
}

}  // namespace ase::adp::godot

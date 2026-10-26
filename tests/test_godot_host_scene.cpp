/**
 * ASE ADAPTER TESTS - GODOT HOST SCENE PORTS
 *
 * @file        test_godot_host_scene.cpp
 * @design      DSGN_021
 * @brief       Pins what the scene ports of one game host carry to the view: organ templates, sky, ground, stem bases
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11, against the real bundle with the configuration and the
 *              game host of test_godot_host_support.hpp: the template ports carry the three organ templates
 *              of ase-render-data once (usecase:101011 tessellate the organ templates), the sky port lights the
 *              scene, the terrain and ground cell ports lay the ground, and the patch records carry the
 *              stem bases ase-terrain levelled. The checks pin properties of the ports, never a number the
 *              view would choose: a unit normal, a closed set of indices, a front face facing outward.
 *
 *              ONE GAME HOST PER RUN, beside the one of test_godot_host.cpp in its own executable: these
 *              cases only read, and the host goes with the process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-24
 * @version     00.00.01.00002
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::environment;
using test::kPatchCentres;
using test::patch_at;

namespace {

/** The key of one template corner: its code above its index. */
uint64_t corner_key(uint32_t code, uint32_t index) {
    return (static_cast<uint64_t>(code) << 32u) | index;
}

}  // namespace

TEST_CASE("usecase:101011 tessellate the organ templates: the template ports carry the three templates once, each a closed set facing outward") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    // ase-render-data builds the templates in the Initialization of its host and publishes their rows in
    // the Observation; the plugin reads them in the Reception and lays them in the Production after.
    constexpr uint32_t kBoundFrames = 32u;
    for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_template_triangles().empty(); ++frame) {
        advance_frames(resources, 1u);
    }
    REQUIRE(!resources.get_template_corners().empty());
    REQUIRE(!resources.get_template_triangles().empty());
    const uint64_t generation = resources.template_generation();

    // every code has corners and triangles; every corner index stands once per code, with a unit normal
    std::unordered_map<uint64_t, const VivariumTemplateCornerView*> corner_of;
    uint32_t corners[GODOT_TMPL_COUNT] = {};
    uint32_t triangles[GODOT_TMPL_COUNT] = {};
    for (const VivariumTemplateCornerView& corner : resources.get_template_corners()) {
        REQUIRE(corner.code < GODOT_TMPL_COUNT);
        CHECK(corner_of.emplace(corner_key(corner.code, corner.index), &corner).second);
        corners[corner.code] += 1u;
        const float length = ase::math::sqrt(static_cast<float>(corner.nx * corner.nx + corner.ny * corner.ny +
                                                                corner.nz * corner.nz));
        CHECK(length == doctest::Approx(1.0).epsilon(1.0e-4));
    }
    // every triangle names three corners of its own code, clockwise seen from outside - Godot's front face:
    // the cross product of its first two edges points against the normal of its first corner
    for (const VivariumTemplateTriangleView& tri : resources.get_template_triangles()) {
        REQUIRE(tri.code < GODOT_TMPL_COUNT);
        triangles[tri.code] += 1u;
        const auto a = corner_of.find(corner_key(tri.code, tri.a));
        const auto b = corner_of.find(corner_key(tri.code, tri.b));
        const auto c = corner_of.find(corner_key(tri.code, tri.c));
        REQUIRE(a != corner_of.end());
        REQUIRE(b != corner_of.end());
        REQUIRE(c != corner_of.end());
        const double e1x = b->second->x - a->second->x;
        const double e1y = b->second->y - a->second->y;
        const double e1z = b->second->z - a->second->z;
        const double e2x = c->second->x - a->second->x;
        const double e2y = c->second->y - a->second->y;
        const double e2z = c->second->z - a->second->z;
        const double turn = (e1y * e2z - e1z * e2y) * a->second->nx + (e1z * e2x - e1x * e2z) * a->second->ny +
                            (e1x * e2y - e1y * e2x) * a->second->nz;
        CHECK(turn < 0.0);
    }
    for (uint32_t code = 0u; code < GODOT_TMPL_COUNT; ++code) {
        CHECK(corners[code] > 0u);
        CHECK(triangles[code] > 0u);
    }
    // the fine cylinder has more corners than the coarse one: that is the line the order budget moves
    CHECK(corners[GODOT_TMPL_FINE] > corners[GODOT_TMPL_CORS]);

    // the templates are read once and kept: more frames leave the set and its generation as they are
    advance_frames(resources, 8u);
    CHECK(resources.template_generation() == generation);
    CHECK(!resources.get_template_triangles().empty());
}

TEST_CASE("03.11: the sky port lights the scene - a unit sun direction, colours as shares, a fog that begins before it is full") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    // ase-pl-sky lays its four records in every Production run once its light pipeline has written
    // the manager's state (Modulation, 2 Hz) - a few frames of a quarter second after the start.
    constexpr uint32_t kBoundFrames = 32u;
    for (uint32_t frame = 0u; frame < kBoundFrames && !resources.get_sky().known; ++frame) {
        advance_frames(resources, 1u);
    }
    const VivariumSkyView& sky = resources.get_sky();
    REQUIRE(sky.known);
    const float length = ase::math::sqrt(static_cast<float>(sky.sun_x * sky.sun_x + sky.sun_y * sky.sun_y +
                                                            sky.sun_z * sky.sun_z));
    CHECK(length == doctest::Approx(1.0).epsilon(1.0e-4));
    for (const double share : {sky.sun_r, sky.sun_g, sky.sun_b, sky.ambient_r, sky.ambient_g, sky.ambient_b,
                               sky.background_r, sky.background_g, sky.background_b, sky.fog_r, sky.fog_g,
                               sky.fog_b}) {
        CHECK(share >= 0.0);
        CHECK(share <= 1.0);
    }
    CHECK(sky.sun_intensity >= 0.0);
    CHECK(sky.ambient_intensity >= 0.0);
    CHECK(sky.fog_near >= 0.0);
    CHECK(sky.fog_far >= sky.fog_near);
    MESSAGE("sun towards " << sky.sun_x << ", " << sky.sun_y << ", " << sky.sun_z << " at " << sky.sun_intensity);
}

TEST_CASE("03.11: the ground reaches the view - unit normals on every sample, cells whose corners stand on the terrain port") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    // ase-terrain measures the square the game host laid once its chunk stands; ase-render-data adds the
    // vertex and the cell in the same Cyclic tick, and the plugin lays both on the ports in the frame after.
    constexpr uint32_t kBoundFrames = 64u;
    for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_ground_cells().empty(); ++frame) {
        advance_frames(resources, 1u);
    }
    REQUIRE(!resources.get_terrain().empty());
    REQUIRE(!resources.get_ground_cells().empty());
    for (const VivariumTerrainView& sample : resources.get_terrain()) {
        const float length = ase::math::sqrt(static_cast<float>(sample.nx * sample.nx + sample.ny * sample.ny +
                                                                sample.nz * sample.nz));
        CHECK(length == doctest::Approx(1.0).epsilon(1.0e-4));
        CHECK(sample.ny > 0.0);            // the ground's normal points up out of it
        CHECK(sample.u == doctest::Approx(sample.x));
        CHECK(sample.v == doctest::Approx(sample.z));
    }
    // every corner names a sample of the same snapshot: the view finds each vertex it indexes
    for (const VivariumGroundCellView& cell : resources.get_ground_cells()) {
        for (const uint32_t corner : {cell.sw, cell.se, cell.ne, cell.nw}) {
            bool found = false;
            for (const VivariumTerrainView& sample : resources.get_terrain()) {
                found = found || sample.object_id == corner;
            }
            CHECK(found);
        }
        CHECK(cell.sw == cell.object_id);  // the cell stands on its south-west sample
    }
}

TEST_CASE("03.11: the stem base reaches the view on the patch record - its centre and the height ase-terrain levelled it to") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    // ase-terrain applies the flatten once the chunks under the disc are generated and tells the patch
    // its level (TRN_FLAT_HGT); ase-pl-flora reads it into its inputs in the next frame and lays it on
    // the record as y.
    constexpr uint32_t kBoundFrames = 64u;
    const auto levelled = [&resources]() {
        for (uint32_t place = 0u; place < 4u; ++place) {
            const VivariumPatchView* patch = resources.get_patch_at(place);
            if (patch == nullptr || !std::isfinite(patch->y)) {
                return false;
            }
        }
        return true;
    };
    for (uint32_t frame = 0u; frame < kBoundFrames && !levelled(); ++frame) {
        advance_frames(resources, 1u);
    }
    for (uint32_t place = 0u; place < 4u; ++place) {
        const VivariumPatchView& patch = patch_at(resources, place);
        // the centre goes back as the host laid it, the height is ase-terrain's
        CHECK(patch.x == doctest::Approx(kPatchCentres[place][0]));
        CHECK(patch.z == doctest::Approx(kPatchCentres[place][1]));
        REQUIRE(std::isfinite(patch.y));
        MESSAGE("place " << place << ": stem base at (" << patch.x << ", " << patch.y << ", " << patch.z << ")");
    }
}

}  // namespace ase::adp::godot

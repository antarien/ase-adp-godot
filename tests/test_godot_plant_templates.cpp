/**
 * ASE ADAPTER TESTS - ORGAN TEMPLATES WITHOUT GODOT
 *
 * @file        test_godot_plant_templates.cpp
 * @design      DSGN_018
 * @brief       Pins that a template is built from the corner and triangle records of its code, as carried
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2: build_template takes the records of the template
 *              ports - one vertex per corner of the code, one triangle per triangle record in the order it
 *              names its corners - and computes nothing (godot_plant_geometry.hpp).
 *
 *              THE WINDING IS THE PORT'S: ase-render-data lays every template triangle clockwise seen from
 *              outside, Godot's front face, and pins that in its own test (test_render_data.cpp). The view
 *              keeps the order a triangle record names - the case below checks exactly that.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101011 - tessellate the organ templates
 * @actor       Organ Template Tessellator (TMPLTESS)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_templates.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/** A corner record of template `code` as the corner port lays it: values no other corner of the test carries. */
VivariumTemplateCornerView corner(uint32_t code, uint32_t index) {
    VivariumTemplateCornerView view;
    view.object_id = code * 100u + index;
    view.code = code;
    view.index = index;
    view.x = 1.0 + index;
    view.y = 2.0 + index;
    view.z = 3.0 + index;
    view.nx = 0.1 * index;  // a normal the view never computes: it only carries it
    view.ny = 0.2 * index;
    view.nz = 0.3 * index;
    view.u = 0.01 * index;
    view.v = 0.02 * index;
    return view;
}

/** A triangle record of template `code` over three corner indices, in the order the record names them. */
VivariumTemplateTriangleView triangle(uint32_t code, uint32_t a, uint32_t b, uint32_t c) {
    VivariumTemplateTriangleView view;
    view.code = code;
    view.a = a;
    view.b = b;
    view.c = c;
    return view;
}

}  // namespace

TEST_CASE("usecase:101011 tessellate the organ templates: a template is built from the records of its code, as the ports carried them") {
    // the fine cylinder's first quad and one corner of the coarse one, laid out of index order
    const std::vector<VivariumTemplateCornerView> corners = {corner(GODOT_TMPL_FINE, 2u), corner(GODOT_TMPL_FINE, 0u),
                                                             corner(GODOT_TMPL_CORS, 0u), corner(GODOT_TMPL_FINE, 1u),
                                                             corner(GODOT_TMPL_FINE, 3u)};
    const std::vector<VivariumTemplateTriangleView> triangles = {
        triangle(GODOT_TMPL_FINE, 0u, 2u, 3u), triangle(GODOT_TMPL_CORS, 0u, 0u, 0u),
        triangle(GODOT_TMPL_FINE, 0u, 3u, 1u)};
    PlantMeshArrays fine;
    CHECK(build_template(corners, triangles, GODOT_TMPL_FINE, fine) == 2u);
    // one vertex per corner of the code, in port order - the coarse corner is not among them
    REQUIRE(fine.positions.size() == 4u * 3u);
    REQUIRE(fine.normals.size() == 4u * 3u);
    REQUIRE(fine.uvs.size() == 4u * 2u);
    const uint32_t port_order[4] = {2u, 0u, 1u, 3u};
    for (std::size_t v = 0u; v < 4u; ++v) {
        const VivariumTemplateCornerView carried = corner(GODOT_TMPL_FINE, port_order[v]);
        CHECK(fine.positions[v * 3u] == doctest::Approx(carried.x));
        CHECK(fine.positions[v * 3u + 1u] == doctest::Approx(carried.y));
        CHECK(fine.positions[v * 3u + 2u] == doctest::Approx(carried.z));
        CHECK(fine.normals[v * 3u] == doctest::Approx(carried.nx));
        CHECK(fine.normals[v * 3u + 1u] == doctest::Approx(carried.ny));
        CHECK(fine.normals[v * 3u + 2u] == doctest::Approx(carried.nz));
        CHECK(fine.uvs[v * 2u] == doctest::Approx(carried.u));
        CHECK(fine.uvs[v * 2u + 1u] == doctest::Approx(carried.v));
    }
    // every triangle over the vertices its corner indices name, in the order the record names them
    REQUIRE(fine.indices.size() == 2u * 3u);
    CHECK(fine.indices[0] == 1);  // corner 0 is the second vertex
    CHECK(fine.indices[1] == 0);  // corner 2 the first
    CHECK(fine.indices[2] == 3);  // corner 3 the fourth
    CHECK(fine.indices[3] == 1);
    CHECK(fine.indices[4] == 3);
    CHECK(fine.indices[5] == 2);  // corner 1 the third

    // the coarse template takes only its own records; a degenerate record still names known corners
    PlantMeshArrays coarse;
    CHECK(build_template(corners, triangles, GODOT_TMPL_CORS, coarse) == 1u);
    CHECK(coarse.positions.size() == 3u);
    // a triangle whose corner the port did not carry is not drawn; an index named twice is drawn once
    std::vector<VivariumTemplateCornerView> holed = corners;
    holed.erase(holed.begin());  // fine corner 2, a corner of the first triangle only
    holed.push_back(corner(GODOT_TMPL_FINE, 0u));
    PlantMeshArrays partial;
    CHECK(build_template(holed, triangles, GODOT_TMPL_FINE, partial) == 1u);
    CHECK(partial.positions.size() == 3u * 3u);
    // a code the ports carry no records for, no corners, no triangles: nothing at all
    PlantMeshArrays leaf;
    CHECK(build_template(corners, triangles, GODOT_TMPL_LEAF, leaf) == 0u);
    CHECK(leaf.positions.empty());
    PlantMeshArrays none;
    CHECK(build_template({}, triangles, GODOT_TMPL_FINE, none) == 0u);
    CHECK(none.indices.empty());
    PlantMeshArrays bare;
    CHECK(build_template(corners, {}, GODOT_TMPL_FINE, bare) == 0u);
    CHECK(bare.indices.empty());
}

}  // namespace ase::adp::godot

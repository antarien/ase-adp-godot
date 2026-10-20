/**
 * ASE GODOT ADAPTER - ASSET SHOWCASE PLAN TESTS
 *
 * @file        test_godot_asset_showcase.cpp
 * @brief       The asset showcase plan against a test list and a test binding, without a Godot process
 * @description The names split into materials and templates in list order, each once; a name the
 *              binding holds no file for counts as missing; each row stands centred on the origin, its
 *              slots GODOT_SHOWCASE_SPACING apart; the log line names all four counts; a template is
 *              scaled to one slot and one without extent keeps its size; the camera stands behind and
 *              above the rows and sees the widest one whole.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:100347 - show approved assets
 * @actor       Asset Showcase (ASSETSHOW)
 * @created     2026-10-09
 * @modified    2026-10-10
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_asset_showcase.hpp>
#include <ase/adp/godot/types.hpp>

#include <doctest/doctest.h>

#include <string>
#include <vector>

using ase::adp::godot::AssetShowcasePlan;
using ase::adp::godot::VivariumAssetBinding;

namespace {

VivariumAssetBinding test_binding() {
    VivariumAssetBinding binding;
    binding.files["birch_bark_white"] = "birch_bark_white.glb";
    binding.files["ground_meadow"] = "ground_meadow.glb";
    binding.files["birch_leaf"] = "birch_leaf.glb";
    binding.missing = {"ground_forest_floor", "brook_stone"};
    return binding;
}

const std::vector<std::string> kNames = {"birch_bark_white", "ground_meadow", "ground_forest_floor",
                                         "birch_leaf", "brook_stone", "birch_leaf"};
const std::vector<std::string> kMaterials = {"birch_bark_white", "ground_meadow", "ground_forest_floor"};

}  // namespace

TEST_CASE("showcase plan: materials and templates in list order, each once, the missing ones counted") {
    const AssetShowcasePlan plan = ase::adp::godot::plan_showcase(kNames, kMaterials, test_binding());
    CHECK(plan.materials == std::vector<std::string>{"birch_bark_white", "ground_meadow", "ground_forest_floor"});
    CHECK(plan.templates == std::vector<std::string>{"birch_leaf", "brook_stone"});
    CHECK(plan.missing == 2u);
}

TEST_CASE("showcase plan: every row stands centred on the origin, its slots one spacing apart") {
    const AssetShowcasePlan plan = ase::adp::godot::plan_showcase(kNames, kMaterials, test_binding());
    REQUIRE(plan.material_x.size() == 3u);
    CHECK(plan.material_x[0] == doctest::Approx(-ase::adp::godot::GODOT_SHOWCASE_SPACING));
    CHECK(plan.material_x[1] == doctest::Approx(0.0f));
    CHECK(plan.material_x[2] == doctest::Approx(ase::adp::godot::GODOT_SHOWCASE_SPACING));
    REQUIRE(plan.template_x.size() == 2u);
    CHECK(plan.template_x[0] == doctest::Approx(-0.5f * ase::adp::godot::GODOT_SHOWCASE_SPACING));
    CHECK(plan.template_x[1] == doctest::Approx(0.5f * ase::adp::godot::GODOT_SHOWCASE_SPACING));
}

TEST_CASE("showcase line: the number of names, materials, templates and missing ones") {
    const AssetShowcasePlan plan = ase::adp::godot::plan_showcase(kNames, kMaterials, test_binding());
    CHECK(ase::adp::godot::showcase_line(plan) == "asset showcase names=5 materials=3 templates=2 missing=2");
}

TEST_CASE("template scale: a model is brought to one slot, one without extent keeps its size") {
    CHECK(ase::adp::godot::template_scale(0.05f) == doctest::Approx(ase::adp::godot::GODOT_SHOWCASE_SLOT / 0.05f));
    CHECK(ase::adp::godot::template_scale(0.0f) == doctest::Approx(1.0f));
}

TEST_CASE("showcase camera: behind and above the rows, the widest row inside the field of view") {
    const AssetShowcasePlan plan = ase::adp::godot::plan_showcase(kNames, kMaterials, test_binding());
    float position[3];
    float target[3];
    ase::adp::godot::showcase_camera(plan, position, target);
    CHECK(target[2] == doctest::Approx(0.5f * ase::adp::godot::GODOT_SHOWCASE_TEMPLATE_ROW));
    CHECK(position[2] < 0.0f);
    CHECK(position[1] > 0.0f);
    const float distance = target[2] - position[2];
    const float half_row = plan.material_x.back() + ase::adp::godot::GODOT_SHOWCASE_SLOT;
    CHECK(half_row / distance <= ase::adp::godot::GODOT_SHOWCASE_HALF_FOV_TAN);
}

/**
 * ASE ADAPTER TESTS - ASSET BINDING WITHOUT GODOT
 *
 * @file        test_godot_asset_binding.cpp
 * @design      DSGN_066
 * @brief       Pins that a name binds only to a manifest file whose checksum matches, every other is missing
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2, the materials and organ templates the care view draws the
 *              plant with: bind_assets binds the asset names of the play stage against a test manifest - a bound
 *              name, a name without an asset, a file whose checksum does not match, a name the manifest carries
 *              twice - and asset_color gives a missing name the stand-in colour of the list, a name the list
 *              gives no colour the signal colour (godot_asset_binding.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_asset_binding.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

// The test's VivariumHashFn: the checksum of a file is what the map says, "" for a file it lacks.
std::string hash_from_map(const std::string& file, void* user) {
    const auto& sums = *static_cast<const std::unordered_map<std::string, std::string>*>(user);
    const auto found = sums.find(file);
    return found == sums.end() ? std::string() : found->second;
}

const std::string kSumA(64u, 'a');
const std::string kSumB(64u, 'b');
constexpr uint32_t kStandInLeaf = 0x5E8F32FFu;    // stand-in colour of the leaf in the test list
constexpr uint32_t kStandInBark = 0xE6E1D3FFu;    // stand-in colour of the white bark in the test list
constexpr uint32_t kStandInMeadow = 0x587A34FFu;  // stand-in colour of meadow soil in the test list

}  // namespace

TEST_CASE("asset binding: a name binds only to a manifest file whose checksum matches, every other is missing") {
    const std::vector<VivariumAssetEntry> manifest = {
        {"birch_leaf", "birch/leaf.glb", kSumA},
        {"birch_bark_white", "birch/bark_white.tres", kSumB},  // its file does not match
        {"ground_meadow", "ground/meadow_1.tres", kSumA},
        {"ground_meadow", "ground/meadow_2.tres", kSumA},      // named twice: ambiguous, missing
    };
    std::unordered_map<std::string, std::string> sums = {
        {"birch/leaf.glb", std::string(64u, 'A')},             // the same checksum in capitals
        {"birch/bark_white.tres", kSumA},
        {"ground/meadow_1.tres", kSumA},
        {"ground/meadow_2.tres", kSumA}};
    const std::vector<std::string> names = {"birch_leaf", "birch_bark_white", "ground_meadow", "birch_bud",
                                            "birch_leaf"};
    // the stand-in colours of the stage: every listed name but birch_bud, and one name the stage does not list
    const std::unordered_map<std::string, uint32_t> stand_ins = {
        {"birch_leaf", kStandInLeaf}, {"birch_bark_white", kStandInBark}, {"ground_meadow", kStandInMeadow},
        {"brook_stone", kStandInBark}};
    const VivariumAssetBinding binding = bind_assets(names, manifest, stand_ins, &hash_from_map, &sums);
    REQUIRE(binding.files.size() == 1u);
    CHECK(binding.files.at("birch_leaf") == "birch/leaf.glb");
    REQUIRE(binding.missing.size() == 3u);
    CHECK(binding.missing[0] == "birch_bark_white");
    CHECK(binding.missing[1] == "ground_meadow");
    CHECK(binding.missing[2] == "birch_bud");
    CHECK(asset_missing_line("birch_bud") == "asset missing birch_bud");

    // 03.2 point 4 (operator order of 2026-10-23): the bound name is drawn as the view draws it, a missing
    // one in the stand-in colour of the list; a name the list gives no colour - an empty one included -
    // takes the signal colour
    CHECK(asset_color(binding, "birch_leaf", GODOT_COLOR_SOIL) == GODOT_COLOR_SOIL);
    CHECK(asset_color(binding, "ground_meadow", GODOT_COLOR_SOIL) == kStandInMeadow);
    CHECK(asset_color(binding, "birch_bark_white", GODOT_COLOR_SOIL) == kStandInBark);
    CHECK(asset_color(binding, "birch_bud", GODOT_COLOR_GROW) == GODOT_COLOR_ASSET_MISSING);
    CHECK(asset_color(binding, std::string(), GODOT_COLOR_SOIL) == GODOT_COLOR_ASSET_MISSING);
    CHECK(GODOT_COLOR_ASSET_MISSING != GODOT_COLOR_SOIL);
    // the binding keeps the colours of the listed names only
    CHECK(binding.stand_ins.size() == 3u);
    CHECK(binding.stand_ins.count("brook_stone") == 0u);

    // no asset set delivered yet (Phase 04): every name is missing, none is bound silently, each listed
    // name stands in its colour
    const VivariumAssetBinding nothing = bind_assets(names, {}, stand_ins, &hash_from_map, &sums);
    CHECK(nothing.files.empty());
    CHECK(nothing.missing.size() == 4u);
    CHECK(asset_color(nothing, "birch_leaf", GODOT_COLOR_SOIL) == kStandInLeaf);
}

}  // namespace ase::adp::godot

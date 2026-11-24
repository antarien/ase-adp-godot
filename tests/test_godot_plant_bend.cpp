/**
 * ASE ADAPTER TESTS - BEND OF THE LEAF CARDS WITHOUT GODOT
 *
 * @file        test_godot_plant_bend.cpp
 * @design      DSGN_024
 * @brief       Pins the bend of every leaf card from its bend record into the custom data of its instance
 * @description PLAN_ASE_VIVARIUM_TREEGEN T.6, Gitter mit Biegung: leaf_bend_of reads the two values of a bend
 *              record (flora.bend.v1) into the view of its leaf, and rows_by_body puts them beside each instance
 *              record as its custom data, r across and g along, which the vertex shader lifts the leaf lattice by
 *              (plant_organ.gdshader). A leaf without a record keeps GODOT_BEND_NONE and is drawn flat. Farbe,
 *              Variante, Herbst: leaf_tint_of reads the colour offset of a tint record (flora.tint.v1) into the view,
 *              and rows_by_body puts it in b of the custom data, which the fragment lays on the albedo as 1 + offset;
 *              a leaf without a tint record keeps GODOT_TINT_NONE and the colour of its species. leaf_atlas_of
 *              reads the cell and the cells of an atlas record (flora.atlas.v1) into the view, and rows_by_body
 *              packs them into a as cell * GODOT_ATLS_PACK + (cells - 1), which the fragment reads back to sample
 *              the texture at (u + cell) / cells; a leaf without an atlas record carries 0. The bark
 *              share and the tiling of an internode stand on CUSTOM0 of its tube (test_godot_plant_tubes.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:6540 - generate plant organs
 * @actor       Organ Arrangement Generator (ORGAARRGEN)
 * @created     2026-11-21
 * @modified    2026-11-24
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_plant_records.hpp"

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_organ_records.hpp>
#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/leaf/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/adp/godot/template/types.hpp>
#include <ase/kernel/kernel_host_port.hpp>
#include <ase/utils/strops.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

using test_records::check_carries;
using test_records::instance;
using test_records::kPatch;
using test_records::organ;

namespace {

constexpr uint32_t kLeaf = 21u;      // the bent leaf
constexpr uint32_t kFlat = 22u;      // a leaf the bend port names no record for
constexpr double kAcross = 0.429;    // the bend across of the birch leaf card (plant_species.json)
constexpr double kAlong = 0.042;     // the bend along of the birch leaf card (plant_species.json)

/** The bend record of one leaf as FloraHostBendExptSystem files it on flora.bend.v1, across then along. */
ase::kernel::KernelHostRecord bend_record(double across, double along) {
    ase::kernel::KernelHostRecord record;
    record.object_id = kLeaf;
    const auto put = [&record](const char* key, double value) {
        ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
        ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, key);
        slot.value = value;
    };
    put(GODOT_KEY_BEND_ACROSS, across);
    put(GODOT_KEY_BEND_ALONG, along);
    return record;
}

/** The tint record of one leaf as FloraHostTintExptSystem files it on flora.tint.v1: its colour offset. */
ase::kernel::KernelHostRecord tint_record(double offset) {
    ase::kernel::KernelHostRecord record;
    record.object_id = kLeaf;
    ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
    ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, GODOT_KEY_TINT_OFFSET);
    slot.value = offset;
    return record;
}

/** The atlas record of one leaf as FloraHostAtlsExptSystem files it on flora.atlas.v1, the cell then the cells. */
ase::kernel::KernelHostRecord atlas_record(double cell, double cells) {
    ase::kernel::KernelHostRecord record;
    record.object_id = kLeaf;
    const auto put = [&record](const char* key, double value) {
        ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
        ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, key);
        slot.value = value;
    };
    put(GODOT_KEY_ATLS_CELL, cell);
    put(GODOT_KEY_ATLS_CELLS, cells);
    return record;
}

/** The leaf record as FloraHostLeafExptSystem files it, reduced to its twelfth value: the card of the leaf. */
ase::kernel::KernelHostRecord card_record(double card) {
    ase::kernel::KernelHostRecord record;
    record.object_id = kLeaf;
    ase::kernel::KernelHostValue& slot = record.values[record.value_count++];
    ase::utils::str_copy(slot.key, ase::kernel::HostKeyBytes, GODOT_KEY_TEMPLATE);
    slot.value = card;
    return record;
}

/** Channel a of an organ without an atlas record: cell 0 of cells 1, packed as plant_instances packs it. */
constexpr float kAtlasNone = static_cast<float>(GODOT_ATLS_CELL_NONE * GODOT_ATLS_PACK +
                                                (GODOT_ATLS_CELLS_NONE - GODOT_ATLS_CELLS_NONE));

}  // namespace

TEST_CASE("usecase:6540 generate plant organs: a bend record lays its two values into the view of its leaf, bit for bit") {
    VivariumOrganView leaf = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf.bend_across == GODOT_BEND_NONE);
    REQUIRE(leaf.bend_along == GODOT_BEND_NONE);
    REQUIRE(leaf_bend_of(bend_record(kAcross, kAlong), leaf));
    CHECK(leaf.bend_across == kAcross);
    CHECK(leaf.bend_along == kAlong);
}

TEST_CASE("usecase:6540 generate plant organs: a bend record with a value that is no finite number is a broken port") {
    VivariumOrganView leaf = organ(kLeaf, kPatch, 1.0);
    CHECK_FALSE(leaf_bend_of(bend_record(std::nan(""), kAlong), leaf));
    CHECK_FALSE(leaf_bend_of(bend_record(kAcross, INFINITY), leaf));
    // the view keeps the flat card it had
    CHECK(leaf.bend_across == GODOT_BEND_NONE);
    CHECK(leaf.bend_along == GODOT_BEND_NONE);
}

TEST_CASE("TREEGEN T.6: the twelfth value of a leaf record lays the fine or the coarse card into the view of its leaf") {
    VivariumOrganView near_leaf = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf_template_of(card_record(static_cast<double>(GODOT_TMPL_LEAF)), near_leaf));
    CHECK(near_leaf.card == GODOT_TMPL_LEAF);
    VivariumOrganView far_leaf = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf_template_of(card_record(static_cast<double>(GODOT_TMPL_LEAF_CORS)), far_leaf));
    CHECK(far_leaf.card == GODOT_TMPL_LEAF_CORS);
}

TEST_CASE("TREEGEN T.6: a leaf record whose card is no leaf card, no number or missing is a broken port") {
    VivariumOrganView leaf = organ(kLeaf, kPatch, 1.0);
    leaf.card = GODOT_TMPL_LEAF_CORS;
    // the bark tube, the fungus card and a code beyond the table never lay a leaf
    CHECK_FALSE(leaf_template_of(card_record(static_cast<double>(GODOT_TMPL_FINE)), leaf));
    CHECK_FALSE(leaf_template_of(card_record(static_cast<double>(GODOT_TMPL_CORS)), leaf));
    CHECK_FALSE(leaf_template_of(card_record(static_cast<double>(GODOT_TMPL_FNGS)), leaf));
    CHECK_FALSE(leaf_template_of(card_record(static_cast<double>(GODOT_TMPL_COUNT)), leaf));
    CHECK_FALSE(leaf_template_of(card_record(2.5), leaf));
    CHECK_FALSE(leaf_template_of(card_record(std::nan("")), leaf));
    CHECK_FALSE(leaf_template_of(bend_record(kAcross, kAlong), leaf));  // a record without the template key
    // the view keeps the card it had
    CHECK(leaf.card == GODOT_TMPL_LEAF_CORS);
}

TEST_CASE("usecase:6540 generate plant organs: every leaf instance carries the bend of its card as custom data, slot for slot") {
    VivariumOrganView bent = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf_bend_of(bend_record(kAcross, kAlong), bent));
    const VivariumOrganView flat = organ(kFlat, kPatch, 1.0);
    const auto split = rows_by_body({instance(kLeaf), instance(kFlat)}, {bent, flat}, GODOT_ORDER_EVERY, GODOT_TMPL_LEAF);
    const PlantBodyRows& plant = split.at(kPatch);

    // one custom slot per record, beside it in the same order: across, along, the colour offset, the atlas cell
    REQUIRE(plant.above_custom.size() == 2u * GODOT_INSTANCE_CUSTOM_VALUES);
    CHECK(plant.above_custom[0] == static_cast<float>(kAcross));
    CHECK(plant.above_custom[1] == static_cast<float>(kAlong));
    CHECK(plant.above_custom[2] == static_cast<float>(GODOT_TINT_NONE));
    CHECK(plant.above_custom[3] == kAtlasNone);
    // the flat card carries no bend, no offset and the whole texture: every slot 0, as before the atlas port
    for (uint32_t v = 0u; v < GODOT_INSTANCE_CUSTOM_VALUES; ++v) {
        CHECK(plant.above_custom[GODOT_INSTANCE_CUSTOM_VALUES + v] == static_cast<float>(GODOT_BEND_NONE));
    }
    // the records themselves stay as the port carried them
    check_carries(plant.above, 0u, instance(kLeaf));
    check_carries(plant.above, 1u, instance(kFlat));
}

TEST_CASE("usecase:6540 generate plant organs: a tint record lays its colour offset into the view of its leaf, bit for bit") {
    constexpr double kOffset = -0.137;  // a colour offset inside its range, as ase-render-data draws one
    VivariumOrganView leaf = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf.tint_offset == GODOT_TINT_NONE);
    REQUIRE(leaf_tint_of(tint_record(kOffset), leaf));
    CHECK(leaf.tint_offset == kOffset);
    // a value that is no finite number is a broken port, and the view keeps the offset it had
    CHECK_FALSE(leaf_tint_of(tint_record(std::nan("")), leaf));
    CHECK_FALSE(leaf_tint_of(tint_record(INFINITY), leaf));
    CHECK(leaf.tint_offset == kOffset);
    // a record without the key is no tint record
    ase::kernel::KernelHostRecord bare;
    bare.object_id = kLeaf;
    VivariumOrganView untinted = organ(kLeaf, kPatch, 1.0);
    CHECK_FALSE(leaf_tint_of(bare, untinted));
    CHECK(untinted.tint_offset == GODOT_TINT_NONE);
}

TEST_CASE("usecase:6540 generate plant organs: every leaf instance carries its colour offset in the third custom slot") {
    constexpr double kOffset = 0.21;  // a colour offset inside its range, as ase-render-data draws one
    VivariumOrganView tinted = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf_tint_of(tint_record(kOffset), tinted));
    const VivariumOrganView plain = organ(kFlat, kPatch, 1.0);
    const auto split = rows_by_body({instance(kLeaf), instance(kFlat)}, {tinted, plain}, GODOT_ORDER_EVERY, GODOT_TMPL_LEAF);
    const PlantBodyRows& plant = split.at(kPatch);

    REQUIRE(plant.above_custom.size() == 2u * GODOT_INSTANCE_CUSTOM_VALUES);
    // the tinted card: no bend, its offset in b, the whole texture in a
    CHECK(plant.above_custom[0] == static_cast<float>(GODOT_BEND_NONE));
    CHECK(plant.above_custom[1] == static_cast<float>(GODOT_BEND_NONE));
    CHECK(plant.above_custom[2] == static_cast<float>(kOffset));
    CHECK(plant.above_custom[3] == kAtlasNone);
    // the card without a tint record keeps the colour of its species: the factor 1 + 0 in the shader
    CHECK(plant.above_custom[GODOT_INSTANCE_CUSTOM_VALUES + 2u] == static_cast<float>(GODOT_TINT_NONE));
}

TEST_CASE("usecase:6540 generate plant organs: an atlas record lays its cell and cells into the view of its leaf, bit for bit") {
    constexpr double kCell = 2.0;   // an atlas cell below its cells, as ase-render-data draws one
    constexpr double kCells = 3.0;  // the cells of the leaf texture, the floor of an atlas count of 3.7
    VivariumOrganView leaf = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf.atlas_cell == GODOT_ATLS_CELL_NONE);
    REQUIRE(leaf.atlas_cells == GODOT_ATLS_CELLS_NONE);
    REQUIRE(leaf_atlas_of(atlas_record(kCell, kCells), leaf));
    CHECK(leaf.atlas_cell == kCell);
    CHECK(leaf.atlas_cells == kCells);
    // a value that is no finite number is a broken port, and the view keeps the cell it had
    CHECK_FALSE(leaf_atlas_of(atlas_record(std::nan(""), kCells), leaf));
    CHECK_FALSE(leaf_atlas_of(atlas_record(kCell, INFINITY), leaf));
    CHECK(leaf.atlas_cell == kCell);
    CHECK(leaf.atlas_cells == kCells);
    // a record without the keys is no atlas record
    ase::kernel::KernelHostRecord bare;
    bare.object_id = kLeaf;
    VivariumOrganView whole = organ(kLeaf, kPatch, 1.0);
    CHECK_FALSE(leaf_atlas_of(bare, whole));
    CHECK(whole.atlas_cell == GODOT_ATLS_CELL_NONE);
    CHECK(whole.atlas_cells == GODOT_ATLS_CELLS_NONE);
}

TEST_CASE("usecase:6540 generate plant organs: every leaf instance carries its atlas cell packed in the fourth custom slot") {
    constexpr double kCell = 2.0;
    constexpr double kCells = 3.0;
    VivariumOrganView varied = organ(kLeaf, kPatch, 1.0);
    REQUIRE(leaf_atlas_of(atlas_record(kCell, kCells), varied));
    const VivariumOrganView whole = organ(kFlat, kPatch, 1.0);
    const auto split = rows_by_body({instance(kLeaf), instance(kFlat)}, {varied, whole}, GODOT_ORDER_EVERY, GODOT_TMPL_LEAF);
    const PlantBodyRows& plant = split.at(kPatch);

    REQUIRE(plant.above_custom.size() == 2u * GODOT_INSTANCE_CUSTOM_VALUES);
    // cell 2 of cells 3: 2 * 16 + (3 - 1)
    CHECK(plant.above_custom[3] == static_cast<float>(kCell * GODOT_ATLS_PACK + (kCells - GODOT_ATLS_CELLS_NONE)));
    // the card without an atlas record shows the whole texture: 0, the value a carried before the atlas port
    CHECK(kAtlasNone == 0.0f);
    CHECK(plant.above_custom[GODOT_INSTANCE_CUSTOM_VALUES + 3u] == kAtlasNone);
}

TEST_CASE("usecase:6540 generate plant organs: every cell of every atlas width survives the packing as the shader reads it back") {
    // plant_organ.gdshader: packed = round(a); cell = floor(packed / 16); cells = packed - cell * 16 + 1
    constexpr double kCellsMax = 8.0;  // PLT_CONST_LEAF_ATLS_CNT_MAX, the widest atlas a species carries
    for (double cells = GODOT_ATLS_CELLS_NONE; cells <= kCellsMax; cells += 1.0) {
        for (double cell = GODOT_ATLS_CELL_NONE; cell < cells; cell += 1.0) {
            VivariumOrganView leaf = organ(kLeaf, kPatch, 1.0);
            REQUIRE(leaf_atlas_of(atlas_record(cell, cells), leaf));
            const auto split = rows_by_body({instance(kLeaf)}, {leaf}, GODOT_ORDER_EVERY, GODOT_TMPL_LEAF);
            const float packed = std::round(split.at(kPatch).above_custom[3]);
            const float read_cell = std::floor(packed / static_cast<float>(GODOT_ATLS_PACK));
            const float read_cells = packed - read_cell * static_cast<float>(GODOT_ATLS_PACK) + 1.0f;
            CHECK(read_cell == static_cast<float>(cell));
            CHECK(read_cells == static_cast<float>(cells));
        }
    }
}

}  // namespace ase::adp::godot

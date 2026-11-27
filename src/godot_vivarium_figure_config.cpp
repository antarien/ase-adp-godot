/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FIGURE CONFIGURATION
 *
 * @file        godot_vivarium_figure_config.cpp
 * @brief       VivariumView::load_figure_config - the ports, operations and spawn place of the figure of the seat
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.8: the start configuration names the ports of ase-pl-avatar,
 *              the operations the view submits through them and the seat and place the figure is spawned at;
 *              GodotHostResourceManager::boot asks for them once the host stands. Every key is required: a missing
 *              or malformed one fails the configuration by its name (fail_config), and a unit set without the plugin
 *              boots and draws no figure, because the manager names the ports it misses.
 *
 *              It stands apart from godot_vivarium_figure.cpp because its reason to change is the start
 *              configuration: a new port of ase-pl-avatar, a new operation or a new spawn field, read once at the
 *              start. follow_figure runs every frame and changes with what a frame shows or sends; this file never
 *              runs after the start.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-17
 * @modified    2026-11-17
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr double kFigureSeatMax = 4294967295.0;  // the largest seat a uint32_t holds

}  // namespace

bool VivariumView::load_figure_config(const ::godot::Dictionary& root) {
    // THE FIGURE OF THE SEAT (07.8): the ports of ase-pl-avatar, the operations through them and where the
    // figure of this view's seat is spawned. Every key is required; the manager names the ports it misses
    // once the host stands, so a unit set without the plugin boots and draws no figure.
    ::godot::String text;
    ::godot::Dictionary ports;
    if (!read_section(root, "ports", ports)) {
        return fail_config("ports");
    }
    if (!read_text(ports, "figure_body", text)) {
        return fail_config("ports.figure_body");
    }
    start_.figure_body_port = utf8_of(text);
    // The body of each figure for the rail CORPO (usecase:101440, U01, U03, N0530): the key is required, the port
    // optional; a host without it draws the SHA defaults of the bridge (VivariumView::bridge_body).
    if (!read_text(ports, "survival_body", text)) {
        return fail_config("ports.survival_body");
    }
    start_.survival_body_port = utf8_of(text);
    // The rest and sleep of each figure for the rest card (usecase:101440, U05, N0555): the key is required, the
    // port optional; a host without it keeps the mockup's pose of the bridge (VivariumView::bridge_rest).
    if (!read_text(ports, "survival_rest", text)) {
        return fail_config("ports.survival_rest");
    }
    start_.survival_rest_port = utf8_of(text);
    // The learning entries and the time slices of each figure for the RSM table of the debug layer and the activity
    // bars (usecase:101305, MIG_ASE_SKILL_RSM B3): both keys required, both ports optional; a host without them
    // shows a table without a column and the view logs the gap.
    if (!read_text(ports, "skill", text)) {
        return fail_config("ports.skill");
    }
    start_.skill_port = utf8_of(text);
    if (!read_text(ports, "skill_qnt", text)) {
        return fail_config("ports.skill_qnt");
    }
    start_.skill_qnt_port = utf8_of(text);
    if (!read_text(ports, "skill_bar", text)) {
        return fail_config("ports.skill_bar");
    }
    start_.skill_bar_port = utf8_of(text);
    if (!read_text(ports, "skill_prg", text)) {
        return fail_config("ports.skill_prg");
    }
    start_.skill_prg_port = utf8_of(text);
    // Every fire cell for the card FOGUEIRA (usecase:101440, U04, B0657): the key is required, the port optional; a
    // host without it draws the SHA defaults of the bridge (VivariumView::bridge_fire).
    if (!read_text(ports, "survival_fire", text)) {
        return fail_config("ports.survival_fire");
    }
    start_.survival_fire_port = utf8_of(text);
    // The ash and the burn time of every fire cell for the same card (T.8, E0257): a port of its own, because the
    // fire record is full at the width of the host; the key is required, the port optional, and a host without it
    // leaves the ash heap and the burn time of the card on the SHA defaults of the bridge (VivariumView::bridge_fire).
    if (!read_text(ports, "survival_left", text)) {
        return fail_config("ports.survival_left");
    }
    start_.survival_left_port = utf8_of(text);
    // The spoil of the food each figure carries for the ring ESTRAGO (usecase:101440, U07, N0530) and the port the
    // start stock of its bag goes in through (T.8, usecase:5378): both keys required, both ports optional; a host
    // without the first draws the mockup's ring of the bridge (VivariumView::bridge_food), a host without the
    // second lays no stock and notes it once.
    if (!read_text(ports, "survival_food", text)) {
        return fail_config("ports.survival_food");
    }
    start_.survival_food_port = utf8_of(text);
    // The soot in the air of the cell each figure stands in for the smoke plume of the air mark (usecase:101440, U06,
    // D2500): ase-fire lays the soot of every fire cell on the raster cell of its fuel body, ase-gis sums it per cell
    // as GIS_LOCAL_SOOT and SurvivalHostSmokExptSystem files it. The key is required, the port optional; a host
    // without it leaves the plume on the owner's grade of the air alone (VivariumView::bridge_air).
    if (!read_text(ports, "survival_smoke", text)) {
        return fail_config("ports.survival_smoke");
    }
    start_.survival_smoke_port = utf8_of(text);
    if (!read_text(ports, "survival_stock", text)) {
        return fail_config("ports.survival_stock");
    }
    start_.survival_stock_port = utf8_of(text);
    if (!read_text(ports, "figure_camera", text)) {
        return fail_config("ports.figure_camera");
    }
    start_.figure_camera_port = utf8_of(text);
    if (!read_text(ports, "figure_spawn", text)) {
        return fail_config("ports.figure_spawn");
    }
    start_.figure_spawn_port = utf8_of(text);
    if (!read_text(ports, "figure_stick", text)) {
        return fail_config("ports.figure_stick");
    }
    start_.figure_stick_port = utf8_of(text);
    if (!read_text(ports, "figure_orbit", text)) {
        return fail_config("ports.figure_orbit");
    }
    start_.figure_orbit_port = utf8_of(text);
    if (!read_text(ports, "figure_switch", text)) {
        return fail_config("ports.figure_switch");
    }
    start_.figure_switch_port = utf8_of(text);
    if (!read_text(ports, "figure_release", text)) {
        return fail_config("ports.figure_release");
    }
    start_.figure_release_port = utf8_of(text);
    if (!read_text(ports, "figure_seize", text)) {
        return fail_config("ports.figure_seize");
    }
    start_.figure_seize_port = utf8_of(text);
    if (!read_text(ports, "figure_hold", text)) {
        return fail_config("ports.figure_hold");
    }
    start_.figure_hold_port = utf8_of(text);
    if (!read_text(ports, "figure_drop", text)) {
        return fail_config("ports.figure_drop");
    }
    start_.figure_drop_port = utf8_of(text);
    if (!read_text(ports, "figure_don", text)) {
        return fail_config("ports.figure_don");
    }
    start_.figure_don_port = utf8_of(text);
    if (!read_text(ports, "figure_fill", text)) {
        return fail_config("ports.figure_fill");
    }
    start_.figure_fill_port = utf8_of(text);
    if (!read_text(ports, "figure_drain", text)) {
        return fail_config("ports.figure_drain");
    }
    start_.figure_drain_port = utf8_of(text);
    if (!read_text(ports, "figure_gear", text)) {
        return fail_config("ports.figure_gear");
    }
    start_.figure_gear_port = utf8_of(text);
    if (!read_text(ports, "figure_code", text)) {
        return fail_config("ports.figure_code");
    }
    start_.figure_code_port = utf8_of(text);
    if (!read_text(ports, "figure_rig", text)) {
        return fail_config("ports.figure_rig");
    }
    start_.figure_rig_port = utf8_of(text);
    if (!read_text(ports, "figure_turn", text)) {
        return fail_config("ports.figure_turn");
    }
    start_.figure_turn_port = utf8_of(text);
    if (!read_text(ports, "figure_area", text)) {
        return fail_config("ports.figure_area");
    }
    start_.figure_area_port = utf8_of(text);
    // 07.13, usecase:101612 keep figure in loaded area: the bumps against the chunk edge the glass pane flashes at
    if (!read_text(ports, "figure_boundary", text)) {
        return fail_config("ports.figure_boundary");
    }
    start_.figure_boundary_port = utf8_of(text);

    ::godot::Dictionary operations;
    if (!read_section(root, "operations", operations)) {
        return fail_config("operations");
    }
    if (!read_text(operations, "spawn", text)) {
        return fail_config("operations.spawn");
    }
    start_.op_spawn = utf8_of(text);
    if (!read_text(operations, "steer", text)) {
        return fail_config("operations.steer");
    }
    start_.op_steer = utf8_of(text);
    if (!read_text(operations, "turn", text)) {
        return fail_config("operations.turn");
    }
    start_.op_turn = utf8_of(text);
    if (!read_text(operations, "switch", text)) {
        return fail_config("operations.switch");
    }
    start_.op_switch = utf8_of(text);
    if (!read_text(operations, "release", text)) {
        return fail_config("operations.release");
    }
    start_.op_release = utf8_of(text);
    if (!read_text(operations, "seize", text)) {
        return fail_config("operations.seize");
    }
    start_.op_seize = utf8_of(text);
    if (!read_text(operations, "hold", text)) {
        return fail_config("operations.hold");
    }
    start_.op_hold = utf8_of(text);
    if (!read_text(operations, "drop", text)) {
        return fail_config("operations.drop");
    }
    start_.op_drop = utf8_of(text);
    if (!read_text(operations, "don", text)) {
        return fail_config("operations.don");
    }
    start_.op_don = utf8_of(text);
    if (!read_text(operations, "fill", text)) {
        return fail_config("operations.fill");
    }
    start_.op_fill = utf8_of(text);
    if (!read_text(operations, "drain", text)) {
        return fail_config("operations.drain");
    }
    start_.op_drain = utf8_of(text);
    if (!read_text(operations, "stock", text)) {
        return fail_config("operations.stock");
    }
    start_.op_stock = utf8_of(text);
    if (!read_text(operations, "measure", text)) {
        return fail_config("operations.measure");
    }
    start_.op_measure = utf8_of(text);
    if (!read_text(operations, "bind", text)) {
        return fail_config("operations.bind");
    }
    start_.op_bind = utf8_of(text);

    ::godot::Dictionary figure;
    if (!read_section(root, "figure", figure)) {
        return fail_config("figure");
    }
    double seat = 0.0;
    if (!read_number(figure, "seat", seat) || !(seat >= 0.0) || seat > kFigureSeatMax ||
        static_cast<double>(static_cast<uint32_t>(seat)) != seat) {
        return fail_config("figure.seat");
    }
    start_.figure_seat = static_cast<uint32_t>(seat);
    if (!read_number(figure, "x", start_.figure_x) || !std::isfinite(start_.figure_x)) {
        return fail_config("figure.x");
    }
    if (!read_number(figure, "z", start_.figure_z) || !std::isfinite(start_.figure_z)) {
        return fail_config("figure.z");
    }
    // The plate of the figure is a relation the product names (07.7, B0447): the place of one patch of "patches",
    // never the patch the figure happens to stand on. load_patches ran before, so the place is held against them.
    double place = 0.0;
    if (!read_number(figure, "place", place) || !(place >= 0.0) || place > kFigureSeatMax ||
        static_cast<double>(static_cast<uint32_t>(place)) != place) {
        return fail_config("figure.place");
    }
    start_.figure_place = static_cast<uint32_t>(place);
    if (std::none_of(start_.patches.begin(), start_.patches.end(),
                     [&](const VivariumPatchSpec& patch) { return patch.place == start_.figure_place; })) {
        return fail_config("figure.place");
    }
    // The character the figure plays is the product's to name; the engine carries no name of its own.
    if (!read_text(figure, "character", text)) {
        return fail_config("figure.character");
    }
    start_.figure_character = utf8_of(text);
    // The start stock of the bag (T.8, usecase:5378): what the figure carries when it first stands, each piece
    // with its item key and its fresh mass. The list may be empty; a piece without a key or a mass above zero is a
    // broken product and refuses the configuration.
    const ::godot::Variant stock_value = figure.get("stock", ::godot::Variant());
    if (stock_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("figure.stock");
    }
    const ::godot::Array stock = stock_value;
    start_.stock.clear();
    for (int64_t i = 0; i < stock.size(); ++i) {
        if (stock[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("figure.stock[]");
        }
        const ::godot::Dictionary entry = stock[i];
        VivariumStockSpec piece;
        if (!read_text(entry, "item", text) || text.is_empty()) {
            return fail_config("figure.stock[].item");
        }
        piece.item = utf8_of(text);
        if (!read_number(entry, "kg", piece.kg) || !std::isfinite(piece.kg) || !(piece.kg > 0.0)) {
            return fail_config("figure.stock[].kg");
        }
        start_.stock.push_back(piece);
    }
    // The pack on the back (N0590, usecase:101332 - wear equipment in slots): the item key of the piece the figure
    // wears from its first stand, the same key the hand of Phase 06 sends and vivarium_assets.json binds as
    // hand.<key>. An empty key means a figure without a pack and is a valid product; a missing entry is not.
    if (!read_text(figure, "don", text)) {
        return fail_config("figure.don");
    }
    start_.don = utf8_of(text);
    return true;
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD DEBUG LAYER
 *
 * @file        godot_hud_debug_layer.cpp
 * @brief       The debug console of the care view: the handle at the foot and the drawer with three tab layers
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_debug_layer.hpp. This unit binds, paints
 *              and answers a tap; where each chip and date field stands is DebugLayer::lay
 *              (godot_hud_debug_layer_lay.cpp), so draw and hit read the one placing.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_046
 * @category    ecs/module
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
 * @created     2026-11-10
 * @modified    2026-11-25
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_debug_layer.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/debug/types.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool DebugLayer::bind(const UiTheme& theme, std::string& failed) {
    double handle[4] = {};
    double fold_hit[3] = {};
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_debug_x", &x_},
             {"hud_debug_y", &y_},                     {"hud_debug_line_step", &line_step_},
             {"hud_debug_line_size", &line_size_},     {"hud_debug_handle_x", &handle[0]},
             {"hud_debug_handle_y", &handle[1]},       {"hud_debug_handle_w", &handle[2]},
             {"hud_debug_handle_h", &handle[3]},       {"hud_debug_handle_radius", &handle_radius_},
             {"hud_debug_handle_text_x", &handle_text_x_}, {"hud_debug_handle_text_y", &handle_text_y_},
             {"hud_debug_drawer_y", &drawer_y_},       {"hud_debug_drawer_h", &drawer_h_},
             {"hud_debug_drawer_line", &drawer_line_}, {"hud_debug_pad", &pad_},
             {"hud_debug_title_dy", &title_dy_},       {"hud_debug_title_size", &title_size_},
             {"hud_debug_fold_x", &fold_x_},           {"hud_debug_fold_size", &fold_size_},
             {"hud_debug_fold_hit_x", &fold_hit[0]},   {"hud_debug_fold_hit_w", &fold_hit[1]},
             {"hud_debug_fold_hit_h", &fold_hit[2]},   {"hud_debug_tab_dy", &tab_dy_},
             {"hud_debug_tab_step", &tab_step_},       {"hud_debug_tab_w", &tab_w_},
             {"hud_debug_tab_h", &tab_h_},             {"hud_debug_body_dy", &body_dy_},
             {"hud_debug_chip_h", &chip_h_},           {"hud_debug_row_step", &row_step_},
             {"hud_debug_note_dy", &note_dy_},         {"hud_debug_note_jump_dy", &note_jump_dy_},
             {"hud_debug_note_size", &note_size_},     {"hud_debug_pace_step", &pace_step_},
             {"hud_debug_pace_w", &pace_w_},           {"hud_debug_stage_step", &stage_step_},
             {"hud_debug_stage_w", &stage_w_},         {"hud_debug_stage_row", &stage_row_},
             {"hud_debug_hour_step", &hour_step_},     {"hud_debug_hour_w", &hour_w_},
             {"hud_debug_hour_row", &hour_row_},       {"hud_debug_hour_per_row", &hour_per_row_},
             {"hud_debug_hour_block", &hour_block_},   {"hud_debug_jump_step", &jump_step_},
             {"hud_debug_jump_w", &jump_w_},           {"hud_debug_date_step", &date_step_},
             {"hud_debug_registro_dy", &registro_dy_}, {"hud_debug_registro_row", &registro_row_},
             {"hud_debug_registro_per_row", &registro_per_row_},
             {"hud_debug_date_label_dy", &date_label_dy_}, {"hud_debug_date_label_size", &date_label_size_},
             {"hud_debug_date_chip_dy", &date_chip_dy_}, {"hud_debug_date_chip_w", &date_chip_w_},
             {"hud_debug_date_field_dx", &date_field_dx_}, {"hud_debug_date_field_w", &date_field_w_},
             {"hud_debug_date_value_dx", &date_value_dx_}, {"hud_debug_date_value_dy", &date_value_dy_},
             {"hud_debug_date_value_size", &date_value_size_}, {"hud_debug_date_plus_dx", &date_plus_dx_},
             {"hud_debug_date_block", &date_block_},   {"hud_debug_vec_step", &vec_step_},
             {"hud_debug_vec_w", &vec_w_},             {"hud_debug_go_w", &go_w_},
             {"hud_debug_go_h", &go_h_},               {"hud_debug_go_step", &go_step_},
             {"hud_debug_go_block", &go_block_},       {"hud_debug_key_step", &key_step_},
             {"hud_debug_key_w", &key_w_},             {"hud_debug_key_block", &key_block_},
             {"hud_debug_relief_step", &relief_step_},
             {"hud_debug_relief_w", &relief_w_},       {"hud_debug_chip_radius", &chip_radius_},
             {"hud_debug_chip_line", &chip_line_},     {"hud_debug_chip_label_dy", &chip_label_dy_},
             {"hud_debug_chip_label_size", &chip_label_size_},
             {"hud_debug_view_x", &view_x_},           {"hud_debug_view_dy", &view_y_},
             {"hud_debug_view_step", &view_step_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    view_y_ += drawer_y_;  // the view chips stand in the title row of the drawer, never over the scene
    if (!(hour_per_row_ >= 1.0)) {
        failed = "hud_debug_hour_per_row";
        return false;  // a row of no hour jump would lay every hour jump on one place
    }
    if (!(registro_per_row_ >= 1.0)) {
        failed = "hud_debug_registro_per_row";
        return false;  // a row of no event jump would lay every event jump on one place
    }
    handle_ = ::godot::Rect2(static_cast<float>(handle[0]), static_cast<float>(handle[1]),
                             static_cast<float>(handle[2]), static_cast<float>(handle[3]));
    fold_hit_ = ::godot::Rect2(static_cast<float>(fold_hit[0]), static_cast<float>(drawer_y_),
                               static_cast<float>(fold_hit[1]), static_cast<float>(fold_hit[2]));

    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"DEBUG_INK", &ink_},
                                                                       {"DEBUG_INK_ON", &ink_on_},
                                                                       {"DEBUG_LINE", &edge_},
                                                                       {"DEBUG_GROUND", &ground_},
                                                                       {"DEBUG_CHIP_OFF", &chip_off_},
                                                                       {"DEBUG_DRAWER", &drawer_},
                                                                       {"DEBUG_FIELD", &field_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }

    std::vector<HudRun> open;
    std::vector<HudRun> close;
    std::vector<HudRun> title;
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {"debug_build", &build_words_}, {"debug_time", &time_words_},
             {"debug_patch", &patch_words_}, {"debug_salt", &salt_words_},
             {"debug_open", &open},          {"debug_close", &close},
             {"debug_title", &title}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    if (!plain_word(theme, "debug_date_less", less_, failed) || !plain_word(theme, "debug_date_more", more_, failed)) {
        return false;
    }
    tabs_.assign(GODOT_HUD_DEBUG_TABS, std::string());
    notes_.assign(GODOT_HUD_DEBUG_TABS, std::string());
    for (const std::pair<uint32_t, const char*>& tab : std::initializer_list<std::pair<uint32_t, const char*>>{
             {GODOT_HUD_DEBUG_TAB_TEMPO, "tempo"},
             {GODOT_HUD_DEBUG_TAB_SALTO, "salto"},
             {GODOT_HUD_DEBUG_TAB_DATA, "data"},
             {GODOT_HUD_DEBUG_TAB_RELEVO, "relevo"}}) {
        if (!plain_word(theme, std::string("debug_tabs.") + tab.second, tabs_[tab.first], failed) ||
            !plain_word(theme, std::string("debug_notes.") + tab.second, notes_[tab.first], failed)) {
            return false;
        }
    }
    views_.assign(GODOT_HUD_DEBUG_VIEWS, std::string());
    if (!plain_word(theme, "debug_views.system", views_[GODOT_HUD_DEBUG_VIEW_SYSTEM], failed) ||
        !plain_word(theme, "debug_views.rsm", views_[GODOT_HUD_DEBUG_VIEW_RSM], failed) ||
        !plain_word(theme, "debug_views.status", views_[GODOT_HUD_DEBUG_VIEW_STATUS], failed) ||
        !rsm_table_.bind(theme, failed)) {
        return false;
    }
    theme_ = &theme;
    view_labels_.clear();
    chip_labels_.clear();
    date_labels_.clear();
    date_values_.clear();
    return build_line_.bind_data(theme, failed) && time_line_.bind_data(theme, failed) &&
           patch_line_.bind_data(theme, failed) && salt_line_.bind_data(theme, failed) && open_.bind_data(theme, failed) && close_.bind_data(theme, failed) &&
           title_.bind_data(theme, failed) && note_.bind_data(theme, failed) && open_.set(open, failed) &&
           close_.set(close, failed) && title_.set(title, failed);
}

bool DebugLayer::grown(std::vector<UiTextBlock>& blocks, size_t count, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "debug layer: a text block was asked for before bind() read the theme";
        return false;
    }
    // A longer body binds its new blocks once; the blocks it had keep their shaped text.
    for (size_t index = blocks.size(); index < count; ++index) {
        blocks.emplace_back();
        if (!blocks.back().bind_data(*theme_, failed)) {
            blocks.pop_back();
            return false;
        }
    }
    return true;
}

bool DebugLayer::draw_lines(UiPen& canvas, const ::godot::Transform2D& frame, const DebugLayerFrame& now,
                            const UiTextPlace& text, std::string& failed) {
    UiTextPlace line = text;
    std::vector<HudRun> build = build_words_;
    if (!fill_words(build,
                    {HudPortValue{"adapter", now.adapter}, HudPortValue{"client", now.client},
                     HudPortValue{"units", hud_whole(static_cast<int64_t>(now.units))}},
                    failed) ||
        !build_line_.set(build, failed)) {
        return false;
    }
    line.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(y_));
    build_line_.draw(canvas, frame, line);

    // The clock line, from the bridged clock record.
    std::vector<HudRun> time = time_words_;
    if (!fill_words(time,
                    {HudPortValue{"day", hud_rounded(now.day, 0)},
                     HudPortValue{"hour", hud_rounded(now.hour, static_cast<int32_t>(GODOT_TEXT_DECIMALS))},
                     HudPortValue{"sim", hud_rounded(now.simulation_s, static_cast<int32_t>(GODOT_TEXT_DECIMALS))},
                     HudPortValue{"steps", hud_whole(static_cast<int64_t>(now.steps))}},
                    failed) ||
        !time_line_.set(time, failed)) {
        return false;
    }
    line.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(y_ + line_step_));
    time_line_.draw(canvas, frame, line);

    // The patch line only while a patch is chosen and read.
    double next_y = y_ + 2.0 * line_step_;
    if (now.patch) {
        std::vector<HudRun> patch = patch_words_;
        if (!fill_words(patch,
                        {HudPortValue{"cover", hud_rounded(now.cover * GODOT_PERCENT, static_cast<int32_t>(GODOT_TEXT_DECIMALS))},
                         HudPortValue{"mm", hud_rounded(now.soil_mm, static_cast<int32_t>(GODOT_TEXT_DECIMALS))},
                         HudPortValue{"rel", hud_rounded(now.soil_rel * GODOT_PERCENT, static_cast<int32_t>(GODOT_TEXT_DECIMALS))}},
                        failed) ||
            !patch_line_.set(patch, failed)) {
            return false;
        }
        line.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(next_y));
        patch_line_.draw(canvas, frame, line);
        next_y += line_step_;
    }

    // The salt line (H.9, I-4) only while the salt port laid records: the world's salt in water, in soil and their
    // sum as ase-gis added it per chunk, read from the port every frame, so a move between water and soil shows as
    // it happens while the sum stays.
    if (now.salt_chunks == 0u) {
        return true;
    }
    std::vector<HudRun> salt = salt_words_;
    if (!fill_words(salt,
                    {HudPortValue{"sum", hud_rounded(now.salt_kg, static_cast<int32_t>(GODOT_TEXT_DECIMALS))},
                     HudPortValue{"hyd", hud_rounded(now.salt_hyd_kg, static_cast<int32_t>(GODOT_TEXT_DECIMALS))},
                     HudPortValue{"soil", hud_rounded(now.salt_soil_kg, static_cast<int32_t>(GODOT_TEXT_DECIMALS))},
                     HudPortValue{"chunks", hud_whole(static_cast<int64_t>(now.salt_chunks))}},
                    failed) ||
        !salt_line_.set(salt, failed)) {
        return false;
    }
    line.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(next_y));
    salt_line_.draw(canvas, frame, line);
    return true;
}

bool DebugLayer::chip(UiPen& canvas, const ::godot::Transform2D& frame, const DebugLayerChip& chip,
                      const DebugLayerFrame& now, UiTextBlock& label, std::string& failed) {
    UiSvgOutline outline;
    svg_append_rounded_rect(outline, chip.rect, static_cast<float>(chip_radius_));
    canvas.draw_set_transform_matrix(frame);
    svg_fill(canvas, outline, chip.on ? ground_ : chip_off_);
    svg_stroke(canvas, outline, edge_, chip_line_);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    if (!label.set(chip.label, failed)) {
        return false;
    }
    UiTextPlace text;
    text.at = ::godot::Vector2(chip.rect.get_center().x, static_cast<float>(chip.rect.get_center().y + chip_label_dy_));
    text.size = chip_label_size_;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.color = chip.on ? ink_on_ : ink_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    label.draw(canvas, frame, text);
    return true;
}

void DebugLayer::view_chips(const DebugLayerFrame& now, std::vector<DebugLayerChip>& out) const {
    // The three tab layers of the console (usecase:101305, MIG_ASE_SKILL_RSM B4): chips in the title row of the open
    // drawer, the size of a drawer tab, the form of a chip; a tap on one shows its layer and the drawer stays open.
    // The closed console shows only its handle at the foot, nothing over the scene.
    if (!now.open) {
        return;
    }
    for (uint32_t view = 0; view < GODOT_HUD_DEBUG_VIEWS; ++view) {
        DebugLayerChip chip;
        chip.rect = ::godot::Rect2(static_cast<float>(view_x_ + view * view_step_), static_cast<float>(view_y_),
                                   static_cast<float>(tab_w_), static_cast<float>(tab_h_));
        chip.label = views_[view];
        chip.on = now.view == view;
        chip.act.view = view;
        out.push_back(chip);
    }
}

bool DebugLayer::draw_view(UiPen& canvas, const ::godot::Transform2D& frame, const DebugLayerFrame& now,
                           const UiTextPlace& text, std::string& failed) {
    std::vector<DebugLayerChip> chips;
    view_chips(now, chips);
    if (!grown(view_labels_, chips.size(), failed)) {
        return false;
    }
    for (size_t place = 0; place < chips.size(); ++place) {
        if (!chip(canvas, frame, chips[place], now, view_labels_[place], failed)) {
            return false;
        }
    }
    if (now.view == GODOT_HUD_DEBUG_VIEW_RSM) {
        return rsm_table_.draw(canvas, frame, now.rsm, now.elapsed, now.still, failed);
    }
    if (now.view == GODOT_HUD_DEBUG_VIEW_STATUS) {
        return draw_lines(canvas, frame, now, text, failed);
    }
    return true;  // SYSTEM: draw() paints the tabs and chips lay() placed
}

bool DebugLayer::draw(UiPen& canvas, const ::godot::Transform2D& frame, const VivariumDebug& debug,
                      const DebugLayerFrame& now, std::string& failed) {
    UiTextPlace text;
    text.size = line_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = ink_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;

    // Closed: the handle at the foot, and nothing else of the console.
    if (!now.open) {
        UiSvgOutline outline;
        svg_append_rounded_rect(outline, handle_, static_cast<float>(handle_radius_));
        canvas.draw_set_transform_matrix(frame);
        svg_fill(canvas, outline, ground_);
        svg_stroke(canvas, outline, edge_, chip_line_);
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        UiTextPlace handle = text;
        handle.at = ::godot::Vector2(static_cast<float>(handle_text_x_), static_cast<float>(handle_text_y_));
        handle.size = chip_label_size_;
        handle.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        open_.draw(canvas, frame, handle);
        return true;
    }

    // Open: the body of the SYSTEM layer only while that layer shows; the RSM and the STATUS layer have their own.
    DebugLayerBody body;
    if (now.view == GODOT_HUD_DEBUG_VIEW_SYSTEM && !lay(debug, now, body, failed)) {
        return false;
    }

    // Open: the drawer under its line, the title and the fold.
    ::godot::Rect2 drawer;
    if (!drawer_place(now, drawer)) {
        failed = "debug layer: the HUD set no surface for the open drawer (DebugLayerFrame::reach)";
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    canvas.draw_rect(drawer, drawer_, true);
    canvas.draw_line(::godot::Vector2(drawer.position.x, static_cast<float>(drawer_y_)),
                     ::godot::Vector2(drawer.get_end().x, static_cast<float>(drawer_y_)), edge_,
                     static_cast<float>(drawer_line_));
    for (const DebugLayerField& field : body.fields) {
        canvas.draw_rect(field.cell, field.part == now.typing ? ground_ : field_, true);  // the picked cell
        canvas.draw_rect(field.cell, edge_, false, static_cast<float>(chip_line_));
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    UiTextPlace title = text;
    title.at = ::godot::Vector2(static_cast<float>(pad_), static_cast<float>(drawer_y_ + title_dy_));
    title.size = title_size_;
    title_.draw(canvas, frame, title);
    UiTextPlace fold = text;
    fold.at = ::godot::Vector2(static_cast<float>(fold_x_), static_cast<float>(drawer_y_ + title_dy_));
    fold.size = fold_size_;
    fold.anchor = GODOT_TEXT_ANCHOR_END;
    close_.draw(canvas, frame, fold);

    // The view chips in the title row and the body of the RSM or the STATUS layer.
    if (!draw_view(canvas, frame, now, text, failed)) {
        return false;
    }
    if (now.view != GODOT_HUD_DEBUG_VIEW_SYSTEM) {
        return true;
    }

    // SYSTEM: the tabs and the chips of the open tab, then the labels and values of the date fields.
    if (!grown(chip_labels_, body.chips.size(), failed) || !grown(date_labels_, body.fields.size(), failed) ||
        !grown(date_values_, body.fields.size(), failed)) {
        return false;
    }
    for (size_t place = 0; place < body.chips.size(); ++place) {
        if (!chip(canvas, frame, body.chips[place], now, chip_labels_[place], failed)) {
            return false;
        }
    }
    for (size_t place = 0; place < body.fields.size(); ++place) {
        const DebugLayerField& field = body.fields[place];
        UiTextPlace label = text;
        label.at = field.label_at;
        label.size = date_label_size_;
        if (!date_labels_[place].set(field.label, failed)) {
            return false;
        }
        date_labels_[place].draw(canvas, frame, label);
        UiTextPlace value = text;
        value.at = field.value_at;
        value.size = date_value_size_;
        value.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        value.color = field.part == now.typing ? ink_on_ : ink_;
        if (!date_values_[place].set(hud_rounded(now.date[field.part], 0), failed)) {
            return false;
        }
        date_values_[place].draw(canvas, frame, value);
    }

    // The note of the tab.
    if (!note_.set(notes_[now.tab], failed)) {
        return false;
    }
    UiTextPlace note = text;
    note.at = ::godot::Vector2(static_cast<float>(pad_), static_cast<float>(body.note_y));
    note.size = note_size_;
    note_.draw(canvas, frame, note);
    return true;
}

bool DebugLayer::hit(const ::godot::Vector2& point, const VivariumDebug& debug, const DebugLayerFrame& now,
                     DebugLayerHit& out) const {
    out = DebugLayerHit{};
    std::vector<DebugLayerChip> found;
    std::string failed;
    if (!places(debug, now, found, failed)) {
        return false;  // draw reports the same refusal with its reason every frame
    }
    for (const DebugLayerChip& chip : found) {
        if (chip.rect.has_point(point)) {
            out = chip.act;
            return true;
        }
    }
    return false;
}

bool DebugLayer::drawer_place(const DebugLayerFrame& now, ::godot::Rect2& out) const {
    // The drawer stands under its line on the middle band and reaches across the surface and down to the end of the
    // hand band, as the mockup's drawer reaches the frame's foot (PLAN_ASE_VIVARIUM_PHASE_06_HUD, zones on a tall
    // surface).
    const ::godot::Rect2& surface = now.reach.surface;
    out = ::godot::Rect2(surface.position.x, static_cast<float>(drawer_y_), surface.size.x,
                         static_cast<float>(drawer_h_ + now.reach.hand));
    return now.open && surface.has_area();
}

bool DebugLayer::places(const VivariumDebug& debug, const DebugLayerFrame& now, std::vector<DebugLayerChip>& out,
                        std::string& failed) const {
    out.clear();
    view_chips(now, out);  // the three view chips answer a tap while the drawer is open
    if (!now.open) {
        DebugLayerChip handle;
        handle.rect = handle_;
        handle.act.handle = true;
        out.push_back(handle);
        return true;
    }
    DebugLayerChip fold;
    fold.rect = fold_hit_;
    fold.act.fold = true;
    out.push_back(fold);
    if (now.view != GODOT_HUD_DEBUG_VIEW_SYSTEM) {
        return true;  // the RSM and the STATUS layer only show, their body answers no tap
    }
    DebugLayerBody body;
    if (!lay(debug, now, body, failed)) {
        return false;
    }
    out.insert(out.end(), body.chips.begin(), body.chips.end());
    // A field cell answers a tap too: it picks its part for the keys of the typed entry. draw paints the cell
    // from body.fields, never as a chip.
    for (const DebugLayerField& field : body.fields) {
        DebugLayerChip cell;
        cell.rect = field.cell;
        cell.label = field.label;
        cell.act.field = field.part;
        out.push_back(cell);
    }
    return true;
}

}  // namespace ase::adp::godot

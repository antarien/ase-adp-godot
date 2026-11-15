/**
 * ASE GODOT ADAPTER - HUD DEBUG LAYER LAYOUT
 *
 * @file        godot_hud_debug_layer_lay.cpp
 * @brief       The placing of the debug drawer: its four tabs and the chips and date fields of each tab
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, building block DebugLayer, see
 *              godot_hud_debug_layer.hpp. DebugLayer::lay places, draw (godot_hud_debug_layer.cpp) paints what it
 *              placed and hit answers what a tap on it asks, so a chip is drawn and hit at the one place.
 *
 *              SALTO SORTS BY WHAT A JUMP NAMES, not by its order in debug.jumps: a stage jump carries a stage,
 *              an hour jump an hour, and every other one (years, an eclipse, a moonrise) neither; the three rows
 *              of the mockup are these three kinds.
 *              DATA TAKES THE CHIPS BESIDE A FIELD FROM debug.date.steps: the smallest step back and the smallest
 *              step ahead of its part, labelled texts.hud.debug_date_less and _more. A field with no step one way
 *              is refused with its label, never drawn with a chip that moves nothing. The row below holds every
 *              step of the first field's part, in its order; below go and now the keys of the typed entry
 *              (debug.date.keys) and clear (register N0475).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_046
 * @category    ecs/module
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_debug_layer.hpp>

#include <godot_cpp/variant/string.hpp>

namespace ase::adp::godot {

namespace {

std::string text_of(const ::godot::String& text) {
    return std::string(text.utf8().get_data());
}

::godot::Rect2 box(double x, double y, double w, double h) {
    return ::godot::Rect2(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                          static_cast<float>(h));
}

}  // namespace

bool DebugLayer::lay(const VivariumDebug& debug, const DebugLayerFrame& now, DebugLayerBody& out,
                     std::string& failed) const {
    out = DebugLayerBody{};
    for (uint32_t tab = 0; tab < GODOT_HUD_DEBUG_TABS; ++tab) {
        DebugLayerChip chip;
        chip.rect = box(pad_ + tab * tab_step_, drawer_y_ + tab_dy_, tab_w_, tab_h_);
        chip.label = tabs_[tab];
        chip.on = now.tab == tab;
        chip.act.tab = tab;
        out.chips.push_back(chip);
    }
    const double y = drawer_y_ + body_dy_;
    if (now.tab == GODOT_HUD_DEBUG_TAB_TEMPO) {
        return lay_tempo(debug, now, y, out);
    }
    if (now.tab == GODOT_HUD_DEBUG_TAB_SALTO) {
        return lay_salto(debug, now, y, out);
    }
    if (now.tab == GODOT_HUD_DEBUG_TAB_DATA) {
        return lay_data(debug, y, out, failed);
    }
    if (now.tab == GODOT_HUD_DEBUG_TAB_RELEVO) {
        return lay_relevo(debug, now, y, out);
    }
    failed = "DebugLayerFrame.tab";
    return false;
}

bool DebugLayer::lay_tempo(const VivariumDebug& debug, const DebugLayerFrame& now, double y, DebugLayerBody& out) const {
    for (uint32_t i = 0; i < debug.buttons.size(); ++i) {
        const VivariumDebugButton& button = debug.buttons[i];
        DebugLayerChip chip;
        chip.rect = box(pad_ + i * pace_step_, y, pace_w_, chip_h_);
        chip.label = text_of(button.label);
        chip.on = now.factor == button.factor;
        chip.act.button = i;
        out.chips.push_back(chip);
    }
    out.note_y = y + row_step_ + note_dy_;
    return true;
}

bool DebugLayer::lay_salto(const VivariumDebug& debug, const DebugLayerFrame& now, double y,
                           DebugLayerBody& out) const {
    uint32_t stages = 0;
    uint32_t hours = 0;
    uint32_t others = 0;
    const double hour_y = y + stage_row_;
    const double other_y = hour_y + hour_block_;
    const uint32_t per_row = static_cast<uint32_t>(hour_per_row_);
    for (uint32_t i = 0; i < debug.jumps.size(); ++i) {
        const VivariumDebugJump& jump = debug.jumps[i];
        DebugLayerChip chip;
        chip.label = text_of(jump.label);
        chip.act.jump = i;
        if (jump.stage != GODOT_STAGE_NONE) {
            chip.rect = box(pad_ + stages * stage_step_, y, stage_w_, chip_h_);
            chip.on = now.stage == jump.stage;
            ++stages;
        } else if (jump.hour != GODOT_HOUR_NONE) {
            chip.rect = box(pad_ + (hours % per_row) * hour_step_, hour_y + static_cast<double>(hours / per_row) * hour_row_, hour_w_,
                            chip_h_);
            ++hours;
        } else {
            chip.rect = box(pad_ + others * jump_step_, other_y, jump_w_, chip_h_);
            ++others;
        }
        out.chips.push_back(chip);
    }
    out.note_y = other_y + row_step_ + note_jump_dy_;
    return true;
}

bool DebugLayer::lay_data(const VivariumDebug& debug, double y, DebugLayerBody& out, std::string& failed) const {
    const VivariumDebugDate& date = debug.date;
    if (date.fields.empty()) {
        failed = "debug.date.fields";
        return false;
    }
    for (uint32_t f = 0; f < date.fields.size(); ++f) {
        const VivariumDebugDateField& field = date.fields[f];
        if (field.part >= GODOT_DATE_PARTS) {
            failed = std::string("debug.date.fields part of ") + text_of(field.label);
            return false;
        }
        const double x = pad_ + f * date_step_;
        uint32_t less = GODOT_HUD_DEBUG_NONE;
        uint32_t more = GODOT_HUD_DEBUG_NONE;
        for (uint32_t s = 0; s < date.steps.size(); ++s) {
            const VivariumDebugDateStep& step = date.steps[s];
            if (step.part != field.part) {
                continue;
            }
            if (step.delta < 0.0 && (less == GODOT_HUD_DEBUG_NONE || step.delta > date.steps[less].delta)) {
                less = s;
            }
            if (step.delta > 0.0 && (more == GODOT_HUD_DEBUG_NONE || step.delta < date.steps[more].delta)) {
                more = s;
            }
        }
        if (less == GODOT_HUD_DEBUG_NONE || more == GODOT_HUD_DEBUG_NONE) {
            failed = std::string("debug.date.steps of ") + text_of(field.label);
            return false;
        }
        DebugLayerChip minus;
        minus.rect = box(x, y + date_chip_dy_, date_chip_w_, chip_h_);
        minus.label = less_;
        minus.act.step = less;
        out.chips.push_back(minus);
        DebugLayerChip plus;
        plus.rect = box(x + date_plus_dx_, y + date_chip_dy_, date_chip_w_, chip_h_);
        plus.label = more_;
        plus.act.step = more;
        out.chips.push_back(plus);
        DebugLayerField cell;
        cell.cell = box(x + date_field_dx_, y + date_chip_dy_, date_field_w_, chip_h_);
        cell.label = text_of(field.label);
        cell.label_at = ::godot::Vector2(static_cast<float>(x), static_cast<float>(y + date_label_dy_));
        cell.value_at = ::godot::Vector2(static_cast<float>(x + date_value_dx_), static_cast<float>(y + date_value_dy_));
        cell.part = field.part;
        out.fields.push_back(cell);
    }
    const double row_y = y + date_block_;
    uint32_t row = 0;
    for (uint32_t s = 0; s < date.steps.size(); ++s) {
        if (date.steps[s].part != date.fields.front().part) {
            continue;
        }
        DebugLayerChip chip;
        chip.rect = box(pad_ + row * vec_step_, row_y, vec_w_, chip_h_);
        chip.label = text_of(date.steps[s].label);
        chip.act.step = s;
        out.chips.push_back(chip);
        ++row;
    }
    const double go_y = row_y + row_step_;
    DebugLayerChip go;
    go.rect = box(pad_, go_y, go_w_, go_h_);
    go.label = text_of(date.go_label);
    go.act.go = true;
    out.chips.push_back(go);
    DebugLayerChip back;
    back.rect = box(pad_ + go_step_, go_y, go_w_, go_h_);
    back.label = text_of(date.now_label);
    back.act.now = true;
    out.chips.push_back(back);
    // The keys of the typed entry in their order, clear after them; a tap on a field cell picks its part.
    const double key_y = go_y + go_block_;
    for (uint32_t k = 0; k < date.keys.size(); ++k) {
        DebugLayerChip key;
        key.rect = box(pad_ + k * key_step_, key_y, key_w_, chip_h_);
        key.label = text_of(date.keys[k].label);
        key.act.key = k;
        out.chips.push_back(key);
    }
    DebugLayerChip clear;
    clear.rect = box(pad_ + static_cast<double>(date.keys.size()) * key_step_, key_y, key_w_, chip_h_);
    clear.label = text_of(date.clear_label);
    clear.act.clear = true;
    out.chips.push_back(clear);
    out.note_y = key_y + key_block_;
    return true;
}

bool DebugLayer::lay_relevo(const VivariumDebug& debug, const DebugLayerFrame& now, double y, DebugLayerBody& out) const {
    for (uint32_t i = 0; i < debug.reliefs.size(); ++i) {
        const VivariumDebugRelief& relief = debug.reliefs[i];
        DebugLayerChip chip;
        chip.rect = box(pad_ + i * relief_step_, y, relief_w_, chip_h_);
        chip.label = text_of(relief.label);
        chip.on = now.height_scale_m == relief.height_scale_m;
        chip.act.relief = i;
        out.chips.push_back(chip);
    }
    out.note_y = y + row_step_ + note_jump_dy_;
    return true;
}

}  // namespace ase::adp::godot

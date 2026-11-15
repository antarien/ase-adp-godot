/**
 * ASE GODOT ADAPTER - HUD ACTION FAN
 *
 * @file        godot_hud_action_fan.cpp
 * @brief       The fan of tools left of the focus ring: one hexagon per tool whose condition holds now
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_action_fan.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_action_fan.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool ActionFan::bind_tool(const UiTheme& theme, const HudToolSpec& spec, FanTool& out, std::string& failed) const {
    if (!plain_word(theme, "tools." + spec.name, out.label, failed)) {
        return false;
    }
    // The chip's tone is the tool's domain (the mockup's DOM[TOOLS[k].dom]), a token of SHA for every tool.
    const std::string token = "hud_fan_" + spec.name + "_tone";
    ::godot::String tone;
    if (!theme.token_text(token, tone)) {
        failed = token;
        return false;
    }
    const std::string code = tone.utf8().get_data();
    if (!theme.tone(code, out.tone)) {
        failed = "DOMAIN_" + code;
        return false;
    }
    out.thing = !spec.has.empty();
    if (out.thing) {
        return out.drawing.bind(theme, failed) && out.drawing.set(spec.has, failed);
    }
    // A tool without an item: its glyph is a word of SHA.
    return word_icon(theme, "fan_glyph." + spec.name, out.icon, failed);
}

bool ActionFan::bind(const UiTheme& theme, std::string& failed) {
    drawn_.clear();
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_fan_radius", &radius_}, {"hud_fan_delay", &delay_}, {"hud_fan_stagger", &stagger_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }

    // One place per entry of hud.focus.fan.offsets, each placed by its SHA tokens.
    const HudContextSpec& spec = theme.spec();
    offsets_.clear();
    chips_.clear();
    for (uint32_t place = 0; place < spec.fan_places; ++place) {
        const std::string stem = "hud_fan_" + std::to_string(place + 1u);
        double dx = 0.0;
        double dy = 0.0;
        if (!theme.measure(stem + "_dx", dx)) {
            failed = stem + "_dx";
            return false;
        }
        if (!theme.measure(stem + "_dy", dy)) {
            failed = stem + "_dy";
            return false;
        }
        offsets_.emplace_back(static_cast<float>(dx), static_cast<float>(dy));
        UiHexChip chip;
        if (!chip.bind(theme, failed)) {
            return false;
        }
        chips_.push_back(chip);
    }

    // One entry per tool of the rules, in their order, so HudFanTool::tool indexes both.
    tools_.clear();
    for (const HudToolSpec& tool : spec.tools) {
        FanTool made;
        if (!bind_tool(theme, tool, made, failed)) {
            return false;
        }
        tools_.push_back(made);
    }

    if (spec.levels.empty()) {
        failed = "hud.perception.levels";
        return false;
    }
    level_words_ = spec.levels[0];
    return true;
}

bool ActionFan::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ActionFanFrame& now,
                     std::string& failed) {
    drawn_.clear();
    const bool named = now.level == level_words_;
    for (const HudFanTool& shown : now.tools) {
        if (shown.tool >= tools_.size() || shown.place >= offsets_.size()) {
            failed = "ActionFan: tool " + std::to_string(shown.tool) + " at place " + std::to_string(shown.place) +
                     " outside hud.tools or hud.focus.fan.offsets";
            return false;
        }
        FanTool& tool = tools_[shown.tool];
        UiHexChip& chip = chips_[shown.place];
        const std::string label = named ? tool.label : std::string();
        if (!(tool.thing ? chip.set_thing(label, failed) : chip.set_glyph(tool.icon, label, failed))) {
            return false;
        }

        // HXI(p[0], p[1], 15, item, tone, act, {fade, d: .3 + i * .07, lab}) or HX(..., glyph, ...).
        UiHexChipPlace place;
        place.centre = now.centre + offsets_[shown.place];
        place.radius = radius_;
        place.tone = tool.tone;
        place.on = false;
        place.fade = true;
        place.delay = delay_ + static_cast<double>(shown.place) * stagger_;
        place.elapsed = now.elapsed;
        place.still = now.still;
        chip.draw(canvas, frame, place);
        if (tool.thing) {
            const UiHexChipPicture picture = chip.picture(place);
            UiItemSpritePlace thing;
            thing.centre = picture.centre;
            thing.box = picture.box;
            thing.opacity = picture.opacity;
            tool.drawing.draw(canvas, frame, thing);
        }
        drawn_.push_back(FanDrawn{shown.tool, place});
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

bool ActionFan::hit(const ::godot::Vector2& point, ActionFanHit& out) const {
    out = ActionFanHit{};
    for (const FanDrawn& chip : drawn_) {
        if (chip.tool < tools_.size() && chip.place.radius > 0.0 &&
            chips_.front().hit(chip.place, point)) {
            out.tool = chip.tool;
            out.take = tools_[chip.tool].thing;
            out.rack = !tools_[chip.tool].thing;
            return true;
        }
    }
    return false;
}

void ActionFan::places(std::vector<ActionFanPlace>& out) const {
    out.clear();
    for (const FanDrawn& chip : drawn_) {
        if (chip.tool >= tools_.size() || chip.place.radius <= 0.0) {
            continue;  // hit() answers no chip without a tool or a size
        }
        const float radius = static_cast<float>(chip.place.radius);
        ActionFanPlace place;
        place.rect = ::godot::Rect2(chip.place.centre - ::godot::Vector2(radius, radius),
                                    ::godot::Vector2(radius + radius, radius + radius));
        place.act.tool = chip.tool;
        place.act.take = tools_[chip.tool].thing;
        place.act.rack = !tools_[chip.tool].thing;
        out.push_back(place);
    }
}

}  // namespace ase::adp::godot

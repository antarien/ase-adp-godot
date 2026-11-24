/**
 * ASE GODOT ADAPTER - HUD TOOL RACK
 *
 * @file        godot_hud_tool_rack.cpp
 * @brief       The tool place: the forked stakes in the world and, in the mode rack, the list of its long tools
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_tool_rack.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_tool_rack.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <initializer_list>

namespace ase::adp::godot {

namespace {

::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

/** A line of text at the baseline point (x, y) fading in after `delay`, or at once when `fade` is false. */
UiTextPlace line_at(double x, double y, double size, const ::godot::Color& color, bool fade, double delay,
                    const ToolRackFrame& now) {
    UiTextPlace place;
    place.at = at(x, y);
    place.size = size;
    place.anchor = GODOT_TEXT_ANCHOR_START;
    place.color = color;
    place.opacity = 1.0;
    place.fade = fade;
    place.delay = delay;
    place.elapsed = now.elapsed;
    place.still = now.still;
    return place;
}

}  // namespace

bool ToolRack::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    chips_.clear();
    sprites_.clear();
    places_.clear();
    listed_ = false;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_world_metre_scale", &metre_scale_},
             {"hud_world_rack_shadow_dx", &shadow_dx_},
             {"hud_world_rack_shadow_dy", &shadow_dy_},
             {"hud_world_rack_shadow_rx", &shadow_rx_},
             {"hud_world_rack_shadow_ry", &shadow_ry_},
             {"hud_world_rack_stake_half", &stake_half_},
             {"hud_world_rack_stake_h", &stake_h_},
             {"hud_world_rack_stake_line", &stake_line_},
             {"hud_world_rack_bar_half", &bar_half_},
             {"hud_world_rack_bar_rise", &bar_rise_},
             {"hud_world_rack_bar_line", &bar_line_},
             {"hud_rack_dim_y", &dim_y_},
             {"hud_rack_dim_h", &dim_h_},
             {"hud_rack_post_radius", &post_radius_},
             {"hud_rack_post_lift", &post_lift_},
             {"hud_rack_spine_x", &spine_x_},
             {"hud_rack_elbow_dy", &elbow_dy_},
             {"hud_rack_spine_top", &spine_top_},
             {"hud_rack_leader_delay", &leader_delay_},
             {"hud_rack_title_x", &title_x_},
             {"hud_rack_title_y", &title_y_},
             {"hud_rack_title_size", &title_size_},
             {"hud_rack_title_delay", &title_delay_},
             {"hud_rack_subtitle_y", &sub_y_},
             {"hud_rack_subtitle_size", &sub_size_},
             {"hud_rack_subtitle_delay", &sub_delay_},
             {"hud_rack_empty_y", &empty_y_},
             {"hud_rack_empty_size", &empty_size_},
             {"hud_rack_row_y", &row_y_},
             {"hud_rack_row_step", &row_step_},
             {"hud_rack_row_stagger", &row_stagger_},
             {"hud_rack_cell_radius", &cell_radius_},
             {"hud_rack_cell_box", &cell_box_},
             {"hud_rack_cell_delay", &cell_delay_},
             {"hud_rack_text_x", &text_x_},
             {"hud_rack_name_dy", &name_dy_},
             {"hud_rack_name_size", &name_size_},
             {"hud_rack_name_delay", &name_delay_},
             {"hud_rack_verbs_dy", &verbs_dy_},
             {"hud_rack_verbs_size", &verbs_size_},
             {"hud_rack_verbs_delay", &verbs_delay_},
             {"hud_rack_facts_dy", &facts_dy_},
             {"hud_rack_facts_size", &facts_size_},
             {"hud_rack_facts_delay", &facts_delay_},
             {"hud_rack_hit_x", &hit_x_},
             {"hud_rack_hit_dy", &hit_dy_},
             {"hud_rack_hit_w", &hit_w_},
             {"hud_rack_hit_h", &hit_h_},
             {"hud_load_decimals", &decimals_},
             {"hud_load_fine_below", &fine_below_},
             {"hud_load_fine_decimals", &fine_decimals_},
             {"hud_drop_rack_radius", &drop_radius_},
             {"hud_drop_rack_lift", &drop_lift_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"MATERIAL_WOOD_DARK", &wood_dark_},
                                                                       {"WORLD_RACK_BAR", &bar_},
                                                                       {"WORLD_RACK_SHADOW", &shadow_},
                                                                       {"OVERLAY_SCRIM_RACK", &scrim_},
                                                                       {"DOMAIN_GROUND", &ground_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"rack_title", &title_},
                                                                            {"rack_sub", &sub_},
                                                                            {"rack_empty", &empty_},
                                                                            {"rack_verbs_join", &join_},
                                                                            {"item_size", &size_},
                                                                            {"item_asset", &asset_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    if (!title_text_.bind_title(theme, failed) || !label_text_.bind_label(theme, failed) ||
        !data_text_.bind_data(theme, failed) || !line_.bind(theme, failed)) {
        return false;
    }
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_facts_ = levels[1];
    level_asset_ = levels[2];
    return true;
}

const UiItemSprite* ToolRack::sprite(const std::string& art, std::string& failed) {
    for (const std::pair<std::string, UiItemSprite>& entry : sprites_) {
        if (entry.first == art) {
            return &entry.second;
        }
    }
    UiItemSprite drawing;
    if (theme_ == nullptr || !drawing.bind(*theme_, failed) || !drawing.set(art, failed)) {
        return nullptr;
    }
    sprites_.emplace_back(art, drawing);
    return &sprites_.back().second;
}

UiHexChip* ToolRack::chip(std::size_t row, std::string& failed) {
    while (chips_.size() <= row) {
        UiHexChip made;
        if (theme_ == nullptr || !made.bind(*theme_, failed) || !made.set_thing(std::string(), failed)) {
            return nullptr;
        }
        chips_.push_back(made);
    }
    return &chips_[row];
}

bool ToolRack::say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
                   const std::vector<HudRun>& word, const std::vector<HudPortValue>& values,
                   const UiTextPlace& place, std::string& failed) {
    std::vector<HudRun> runs = word;
    if (!fill_words(runs, values, failed) || !block.set(runs, failed)) {
        return false;
    }
    block.draw(canvas, frame, place);
    canvas.draw_set_transform_matrix(frame);
    return true;
}

bool ToolRack::verbs_of(const RackTool& tool, std::vector<HudRun>& out, std::string& failed) const {
    out.clear();
    for (std::size_t index = 0; index < tool.verbs.size(); ++index) {
        std::vector<HudRun> verb;
        const std::string key = "verbs." + tool.verbs[index];
        if (!theme_->words(key, verb)) {
            failed = "texts.hud." + key;
            return false;
        }
        if (index > 0) {
            out.insert(out.end(), join_.begin(), join_.end());
        }
        out.insert(out.end(), verb.begin(), verb.end());
    }
    return true;
}

std::string ToolRack::number(double value, bool fine) const {
    // kgf: below hud_load_fine_below to hud_load_fine_decimals places, else hud_load_decimals; D.len as written.
    if (!fine) {
        return hud_shortest(value);
    }
    const int decimals = static_cast<int>(value < fine_below_ ? fine_decimals_ : decimals_);
    return hud_decimals(value, decimals);
}

bool ToolRack::metre(const UiAnchor& anchor, const ::godot::Vector3& place, ::godot::Vector2& at_place,
                     double& length) const {
    ::godot::Vector2 across;
    if (!anchor.place(place, at_place) || !anchor.place(place + ::godot::Vector3(1.0f, 0.0f, 0.0f), across)) {
        return false;
    }
    // pxm(z): the mockup's metre is this share of the scene's at the same place.
    length = static_cast<double>(at_place.distance_to(across)) * metre_scale_;
    return true;
}

bool ToolRack::draw_world(UiPen& canvas, const ::godot::Transform2D& frame, const ToolRackWorld& now,
                          std::string& failed) {
    if (now.anchor == nullptr) {
        failed = "ToolRack drawn without the projection of the frame";
        return false;
    }
    ::godot::Vector2 foot;
    double k = 0.0;
    if (!metre(*now.anchor, now.place, foot, k)) {
        return true;  // the place lies behind the camera
    }
    if (now.bodied) {
        // the rack body of ase-pl-toolrack stands as its file in the 3D scene (lay_tools), and the sun of the scene
        // casts its shadow on the ground (light_plant_scene); a painted shadow beside it would be a second one
        // that neither grows nor turns with the light (PLAN Phase 06, Tasche, Guertel und Werkzeugplatz, point 5)
        return true;
    }
    canvas.draw_set_transform_matrix(
        frame * ::godot::Transform2D(0.0f, ::godot::Size2(static_cast<float>(k * shadow_rx_),
                                                          static_cast<float>(k * shadow_ry_)),
                                     0.0f, foot + at(shadow_dx_, shadow_dy_)));
    canvas.draw_circle(::godot::Vector2(), 1.0f, shadow_);
    canvas.draw_set_transform_matrix(frame);
    for (const double side : {-stake_half_, stake_half_}) {
        const ::godot::Vector2 low = foot + at(k * side, 0.0);
        const ::godot::Vector2 high = foot + at(k * side, -k * stake_h_);
        canvas.draw_line(low, high, wood_dark_, static_cast<float>(stake_line_), true);
        canvas.draw_circle(low, static_cast<float>(stake_line_ / 2.0), wood_dark_);   // lineCap round
        canvas.draw_circle(high, static_cast<float>(stake_line_ / 2.0), wood_dark_);
    }
    const ::godot::Vector2 left = foot + at(-k * bar_half_, -k * bar_rise_);
    const ::godot::Vector2 right = foot + at(k * bar_half_, -k * bar_rise_);
    canvas.draw_line(left, right, bar_, static_cast<float>(bar_line_), true);
    canvas.draw_circle(left, static_cast<float>(bar_line_ / 2.0), bar_);
    canvas.draw_circle(right, static_cast<float>(bar_line_ / 2.0), bar_);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

bool ToolRack::draw_list(UiPen& canvas, const ::godot::Transform2D& frame, const ToolRackFrame& now,
                         std::string& failed) {
    places_.clear();
    listed_ = false;
    if (now.anchor == nullptr) {
        failed = "ToolRack listed without the projection of the frame";
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    scrim_box_ = hud_cover_dim(now.cover, dim_y_, dim_h_);
    canvas.draw_rect(scrim_box_, scrim_, true);
    listed_ = true;

    // The post above the place and the leader over the knee up the spine.
    ::godot::Vector2 foot;
    double k = 0.0;
    if (metre(*now.anchor, now.place, foot, k)) {
        const ::godot::Vector2 post = foot + at(0.0, -k * post_lift_);
        canvas.draw_circle(post, static_cast<float>(post_radius_), ground_);
        ::godot::PackedVector2Array leader;
        leader.push_back(post);
        leader.push_back(at(spine_x_, post.y + elbow_dy_));
        leader.push_back(at(spine_x_, spine_top_));
        line_.draw(canvas, leader, ground_, line_.width(), line_.drawn(now.elapsed, leader_delay_, now.still));
    }

    std::vector<HudPortValue> area;
    area.push_back(HudPortValue{"place", now.area});
    if (!say(canvas, frame, title_text_, title_, {},
             line_at(title_x_, title_y_, title_size_, ground_, true, title_delay_, now), failed) ||
        !say(canvas, frame, label_text_, sub_, area,
             line_at(title_x_, sub_y_, sub_size_, soft_, true, sub_delay_, now), failed)) {
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        return false;
    }

    // The rows along the place: list.sort((a, b) => a.gu - b.gu).
    std::vector<const RackTool*> rows;
    if (now.tools != nullptr) {
        for (const RackTool& tool : *now.tools) {
            std::size_t slot = rows.size();
            rows.push_back(&tool);
            while (slot > 0 && rows[slot - 1]->along > tool.along) {
                rows[slot] = rows[slot - 1];
                --slot;
            }
            rows[slot] = &tool;
        }
    }
    if (rows.empty()) {
        const bool said = say(canvas, frame, label_text_, empty_, {},
                              line_at(title_x_, empty_y_, empty_size_, faint_, false, 0.0, now), failed);
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        return said;
    }
    if (rows.size() > 1) {
        ::godot::PackedVector2Array spine;
        spine.push_back(at(spine_x_, row_y_));
        spine.push_back(at(spine_x_, row_y_ + static_cast<double>(rows.size() - 1) * row_step_));
        line_.draw(canvas, spine, faint_, line_.width(), line_.drawn(now.elapsed, 0.0, now.still));
    }

    for (std::size_t index = 0; index < rows.size(); ++index) {
        const RackTool& tool = *rows[index];
        const double y = row_y_ + static_cast<double>(index) * row_step_;
        const double stagger = static_cast<double>(index) * row_stagger_;
        UiHexChip* cell = chip(index, failed);
        const UiItemSprite* drawing = sprite(tool.art, failed);
        if (cell == nullptr || drawing == nullptr) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        UiHexChipPlace place;
        place.centre = at(spine_x_, y);
        place.radius = cell_radius_;
        place.tone = ground_;
        place.on = false;
        place.fade = true;
        place.delay = cell_delay_ + stagger;
        place.elapsed = now.elapsed;
        place.still = now.still;
        cell->draw(canvas, frame, place);
        const UiHexChipPicture picture = cell->picture(place);
        UiItemSpritePlace thing;
        thing.centre = picture.centre;
        thing.box = picture.box;
        thing.opacity = picture.opacity;
        drawing->draw(canvas, frame, thing);
        canvas.draw_set_transform_matrix(frame);

        std::vector<HudRun> name;
        const std::string name_key = std::string(GODOT_HUD_WORD_ITEM_HEAD) + tool.id + GODOT_HUD_WORD_ITEM_TAIL;
        if (!theme_->words(name_key, name)) {
            failed = "texts.hud." + name_key;
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        std::vector<HudRun> verbs;
        if (!verbs_of(tool, verbs, failed) ||
            !say(canvas, frame, title_text_, name, {},
                 line_at(text_x_, y + name_dy_, name_size_, title_text_.ink(), true, name_delay_ + stagger, now),
                 failed) ||
            !say(canvas, frame, label_text_, verbs, {},
                 line_at(text_x_, y + verbs_dy_, verbs_size_, ground_, true, verbs_delay_ + stagger, now), failed)) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        if (now.level >= level_facts_) {
            std::vector<HudRun> facts = size_;
            if (now.level >= level_asset_) {
                facts.insert(facts.end(), asset_.begin(), asset_.end());
            }
            std::vector<HudPortValue> values;
            values.push_back(HudPortValue{"cm", number(tool.length_cm, false)});
            values.push_back(HudPortValue{"kg", number(tool.kg, true)});
            values.push_back(HudPortValue{"asset", tool.asset});
            if (!say(canvas, frame, data_text_, facts, values,
                     line_at(text_x_, y + facts_dy_, facts_size_, faint_, true, facts_delay_ + stagger, now),
                     failed)) {
                canvas.draw_set_transform_matrix(::godot::Transform2D());
                return false;
            }
        }

        const double box = ::ase::math::max(drawing->view_width(), drawing->view_height());
        RackToolPlace drawn;
        drawn.key = tool.key;
        drawn.id = tool.id;
        drawn.k = box > 0.0 ? cell_box_ / box : 0.0;
        drawn.box = ::godot::Rect2(static_cast<float>(spine_x_ - cell_box_ / 2.0),
                                   static_cast<float>(y - cell_box_ / 2.0), static_cast<float>(cell_box_),
                                   static_cast<float>(cell_box_));
        drawn.hit = ::godot::Rect2(static_cast<float>(hit_x_), static_cast<float>(y + hit_dy_),
                                   static_cast<float>(hit_w_), static_cast<float>(hit_h_));
        places_.push_back(drawn);
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

bool ToolRack::over(const ToolRackWorld& now, const ::godot::Vector2& point, bool ground_hit,
                    const ::godot::Vector3& ground) const {
    if (now.anchor == nullptr) {
        return false;
    }
    if (now.bodied) {
        // 8.6: the body in the scene takes the drag where the ray through the finger meets it (rack_struck); the
        // ground takes it within the drop radius in scene metres around the place. metre() gives the frame length
        // of one mockup metre, the scene's metre times hud_world_metre_scale, so the scene's metre is that over
        // the scale and the radius in scene metres is the frame radius over the scene's metre.
        if (now.struck) {
            return true;
        }
        ::godot::Vector2 foot;
        double length = 0.0;
        if (!ground_hit || !metre(*now.anchor, now.place, foot, length) || length <= 0.0) {
            return false;
        }
        const double radius = drop_radius_ * metre_scale_ / length;
        const ::godot::Vector2 apart(ground.x - now.place.x, ground.z - now.place.z);
        return static_cast<double>(apart.length()) < radius;
    }
    ::godot::Vector2 foot;
    if (!now.anchor->place(now.place, foot)) {
        return false;
    }
    return static_cast<double>(point.distance_to(foot - at(0.0, drop_lift_))) < drop_radius_;
}

std::string ToolRack::hit(const ::godot::Vector2& point, std::string& key) const {
    key.clear();
    if (!listed_) {
        return std::string();
    }
    for (const RackToolPlace& row : places_) {
        if (row.hit.has_point(point) || row.box.has_point(point)) {
            key = row.key;
            return GODOT_HUD_ACT_ITEM;
        }
    }
    if (scrim_box_.has_point(point)) {
        return GODOT_HUD_ACT_REST;
    }
    return std::string();
}

}  // namespace ase::adp::godot

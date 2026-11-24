/**
 * ASE GODOT ADAPTER - HUD BAG CONTENTS
 *
 * @file        godot_hud_bag_contents.cpp
 * @brief       The things lying in one open container, drawn where they were let go, the last moved on top
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_bag_contents.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bag_contents.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

namespace {

/** Reads the token `name` into `out`; false with `name` in `failed`. */
bool read_token(const UiTheme& theme, const char* name, double& out, std::string& failed) {
    if (!theme.measure(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

/** `color` with its alpha multiplied by `opacity`. */
::godot::Color faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

/** `rect` grown by `pad` on every side. */
::godot::Rect2 padded(const ::godot::Rect2& rect, double pad) {
    const float grow = static_cast<float>(pad);
    return ::godot::Rect2(rect.position - ::godot::Vector2(grow, grow),
                          rect.size + ::godot::Vector2(grow + grow, grow + grow));
}

}  // namespace

bool BagContents::bind(const UiTheme& theme, std::string& failed) {
    double select_fill_alpha = 0.0;
    double shadow_alpha = 0.0;
    if (!read_token(theme, "hud_item_select_pad", select_pad_, failed) ||
        !read_token(theme, "hud_item_select_radius", select_radius_, failed) ||
        !read_token(theme, "hud_item_select_fill_alpha", select_fill_alpha, failed) ||
        !read_token(theme, "hud_item_select_line", select_line_, failed) ||
        !read_token(theme, "hud_item_select_dash", select_dash_, failed) ||
        !read_token(theme, "hud_item_select_gap", select_gap_, failed) ||
        !read_token(theme, "hud_item_shadow_dy", shadow_dy_, failed) ||
        !read_token(theme, "hud_item_shadow_blur", shadow_blur_, failed) ||
        !read_token(theme, "hud_item_shadow_alpha", shadow_alpha, failed) ||
        !read_token(theme, "hud_item_count_dx", count_dx_, failed) ||
        !read_token(theme, "hud_item_count_dy", count_dy_, failed) ||
        !read_token(theme, "hud_item_count_size", count_size_, failed) ||
        !read_token(theme, "hud_item_hit_pad", hit_pad_, failed) ||
        !read_token(theme, "hud_item_dragged_alpha", dragged_alpha_, failed)) {
        return false;
    }
    if (!theme.color("RENDER_WHITE", select_fill_)) {
        failed = "RENDER_WHITE";
        return false;
    }
    if (!theme.color("OVERLAY_SHADOW", shadow_)) {
        failed = "OVERLAY_SHADOW";
        return false;
    }
    select_fill_.a = static_cast<float>(select_fill_alpha);
    shadow_.a = static_cast<float>(shadow_alpha);
    if (!theme.words("stack_count", count_word_)) {
        failed = "texts.hud.stack_count";
        return false;
    }
    if (!count_.bind_data(theme, failed)) {
        return false;
    }
    theme_ = &theme;
    sprites_.clear();
    return true;
}

const UiItemSprite* BagContents::sprite(const std::string& art, std::string& failed) {
    for (const std::pair<std::string, UiItemSprite>& entry : sprites_) {
        if (entry.first == art) {
            return &entry.second;
        }
    }
    UiItemSprite drawing;
    if (!drawing.bind(*theme_, failed) || !drawing.set(art, failed)) {
        return nullptr;
    }
    sprites_.emplace_back(art, drawing);
    return &sprites_.back().second;
}

bool BagContents::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ContainerDef& container,
                       const std::vector<BagThing>& things, const BagContentsView& view,
                       std::vector<BagThingPlace>& places, std::string& failed) {
    places.clear();
    if (theme_ == nullptr) {
        failed = "hud.bag_contents.unbound";
        return false;
    }
    // inside(c).sort((a, b) => a.zo - b.zo): the order rising, the last moved drawn last and so on top.
    std::vector<size_t> drawn;
    for (size_t index = 0; index < things.size(); ++index) {
        size_t at = drawn.size();
        while (at > 0 && things[drawn[at - 1]].order > things[index].order) {
            --at;
        }
        drawn.insert(drawn.begin() + static_cast<std::ptrdiff_t>(at), index);
    }
    for (const size_t index : drawn) {
        const BagThing& thing = things[index];
        const UiItemSprite* drawing = sprite(thing.art, failed);
        if (drawing == nullptr) {
            return false;
        }
        // ik: the drawn box over the drawing's longer side.
        const double k = container.drawn_box(thing.length_cm) /
                         ase::math::max(drawing->view_width(), drawing->view_height());
        const ::godot::Vector2 corner = view.corner + thing.at;
        const ::godot::Rect2 box(corner, ::godot::Vector2(static_cast<float>(drawing->view_width() * k),
                                                          static_cast<float>(drawing->view_height() * k)));
        places.push_back(BagThingPlace{thing.key, box, k});
        const double opacity = !view.dragged.empty() && view.dragged == thing.key ? dragged_alpha_ : 1.0;

        canvas.draw_set_transform_matrix(frame);
        if (!view.selected.empty() && view.selected == thing.key) {
            UiSvgOutline frame_rect;
            svg_append_rounded_rect(frame_rect, padded(box, select_pad_), static_cast<float>(select_radius_));
            svg_fill(canvas, frame_rect, faded(select_fill_, opacity));
            svg_stroke_dashed(canvas, frame_rect, faded(view.ink, opacity), select_line_, select_dash_, select_gap_);
        }

        UiItemSpritePlace place;
        place.level = thing.level;
        place.opacity = opacity;
        place.silhouette = true;
        place.shade = shadow_;
        place.spread = shadow_blur_;
        drawing->draw_scaled(canvas, frame, corner + ::godot::Vector2(0.0f, static_cast<float>(shadow_dy_)), k,
                             place);
        place.silhouette = false;
        drawing->draw_scaled(canvas, frame, corner, k, place);

        if (thing.stack && thing.count > 1) {
            std::vector<HudRun> runs = count_word_;
            if (!fill_words(runs, {HudPortValue{"n", hud_whole(thing.count)}}, failed) ||
                !count_.set(runs, failed)) {
                return false;
            }
            UiTextPlace text;
            text.at = box.position + box.size +
                      ::godot::Vector2(static_cast<float>(count_dx_), static_cast<float>(count_dy_));
            text.size = count_size_;
            text.anchor = GODOT_TEXT_ANCHOR_END;
            text.color = view.ink;
            text.opacity = opacity;
            text.still = true;
            text.bare = true;  // <text class="d"> past T, no sh (m.html 756): an outline smudged it on the linen
            count_.draw(canvas, frame, text);
        }
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

std::string BagContents::hit(const std::vector<BagThingPlace>& places, const ::godot::Vector2& point) const {
    // The places stand in drawing order; the topmost is the last.
    for (size_t index = places.size(); index > 0; --index) {
        if (padded(places[index - 1].box, hit_pad_).has_point(point)) {
            return places[index - 1].key;
        }
    }
    return std::string();
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD BAG WINDOW
 *
 * @file        godot_hud_bag_window.cpp
 * @brief       The leather pack and the linen pouch opened: a window each, things where they were let go
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_bag_window.hpp. The measures and colours
 *              are read by read_bag_window_look (godot_hud_bag_window_look.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bag_window.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/hud/sprite/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/adp/godot/hud/touch/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <utility>

namespace ase::adp::godot {

namespace {

/** Reads the word `key`; false with its name in `failed`. */
bool read_word(const UiTheme& theme, const char* key, std::vector<HudRun>& out, std::string& failed) {
    if (!theme.words(key, out)) {
        failed = std::string("texts.hud.") + key;
        return false;
    }
    return true;
}

/** Two GODOT_HUD_* names are the same; a nullptr is none. */
bool same(const char* a, const char* b) {
    return a != nullptr && b != nullptr && std::string(a) == b;
}

::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

/** A line of text at the baseline point (x, y), shown at once. */
UiTextPlace line_at(double x, double y, double size, double anchor, const ::godot::Color& color) {
    UiTextPlace place;
    place.at = at(x, y);
    place.size = size;
    place.anchor = anchor;
    place.color = color;
    place.opacity = 1.0;
    place.still = true;
    return place;
}

/** The mockup's bagPath: a body with corner `c` at the top, `low` reached over `reach` at the foot, closed. */
UiSvgOutline bag_path(double x, double y, double w, double h, double c, double low, double reach) {
    UiSvgOutline o;
    svg_append(o, at(x + c, y));
    svg_append(o, at(x + w - c, y));
    svg_append_quadratic(o, at(x + w - c, y), at(x + w, y), at(x + w, y + c));
    svg_append(o, at(x + w, y + h - low));
    svg_append_quadratic(o, at(x + w, y + h - low), at(x + w, y + h), at(x + w - reach, y + h));
    svg_append(o, at(x + reach, y + h));
    svg_append_quadratic(o, at(x + reach, y + h), at(x, y + h), at(x, y + h - low));
    svg_append(o, at(x, y + c));
    svg_append_quadratic(o, at(x, y + c), at(x, y), at(x + c, y));
    o.closed = true;
    return o;
}

/** An open quadratic stroke from `from` over `control` to `to`. */
UiSvgOutline curve(const ::godot::Vector2& from, const ::godot::Vector2& control, const ::godot::Vector2& to) {
    UiSvgOutline o;
    svg_append(o, from);
    svg_append_quadratic(o, from, control, to);
    return o;
}

/** `point` lies inside the closed `outline`. */
bool inside(const UiSvgOutline& outline, const ::godot::Vector2& point) {
    return outline.points.size() >= GODOT_ITEM_POLYGON_MIN &&
           ::godot::Geometry2D::get_singleton()->is_point_in_polygon(point, outline.points);
}

/** The weight and the room of `things` together (the mockup's load). */
std::pair<double, double> load_of(const std::vector<BagThing>& things) {
    std::pair<double, double> sum(0.0, 0.0);
    for (const BagThing& thing : things) {
        sum.first += thing.kg;
        sum.second += thing.litres;
    }
    return sum;
}

}  // namespace

bool BagWindow::bind(const UiTheme& theme, std::string& failed) {
    if (!read_bag_window_look(theme, look_, failed)) {
        return false;
    }
    std::vector<HudRun> close_word;
    if (!read_word(theme, "items.ranzen.name", pack_title_, failed) ||
        !read_word(theme, "items.bolsa.name", pouch_title_, failed) || !read_word(theme, "load", load_, failed) ||
        !read_word(theme, "load_pouch", load_pouch_, failed) || !read_word(theme, "mouth", mouth_, failed) ||
        !read_word(theme, "window_close", close_word, failed)) {
        return false;
    }
    if (close_word.empty() || close_word[0].icon.empty()) {
        failed = "texts.hud.window_close";
        return false;
    }
    if (!close_.bind(theme, close_word[0].icon, failed)) {
        return false;
    }
    // The thresholds of perception are the spec's levels, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_load_ = levels[1];
    level_mouth_ = levels[2];
    if (!title_.bind_title(theme, failed) || !data_.bind_data(theme, failed) ||
        !pack_def_.bind(theme, "bag", failed) || !pouch_def_.bind(theme, "pouch", failed) ||
        !pack_contents_.bind(theme, failed) || !pouch_contents_.bind(theme, failed)) {
        return false;
    }
    pack_at_ = at(look_.pack.x, look_.pack.y);
    pouch_at_ = at(look_.pouch.x, look_.pouch.y);
    pack_open_ = false;
    pouch_open_ = false;
    held_ = nullptr;
    return true;
}

void BagWindow::toggle_pack() {
    pack_open_ = !pack_open_;
    if (!pack_open_) {
        pouch_open_ = false;
    }
}

void BagWindow::toggle_pouch() {
    pouch_open_ = !pouch_open_;
}

void BagWindow::close_pouch() {
    pouch_open_ = false;
}

void BagWindow::grab(const char* part, const ::godot::Vector2& point) {
    if (same(part, GODOT_HUD_ACT_PACK_GRIP)) {
        held_ = GODOT_HUD_CONTAINER_PACK;
        held_from_ = point - pack_at_;
    } else if (same(part, GODOT_HUD_ACT_POUCH_GRIP)) {
        held_ = GODOT_HUD_CONTAINER_POUCH;
        held_from_ = point - pouch_at_;
    }
}

bool BagWindow::move(const ::godot::Vector2& point) {
    if (held_ == nullptr) {
        return false;
    }
    const bool pack = same(held_, GODOT_HUD_CONTAINER_PACK);
    const double w = pack ? look_.pack.w : look_.pouch.w;
    const double h = pack ? look_.pack.h : look_.pouch.h;
    const double top = pack ? look_.pack.min_y : look_.pouch.min_y;
    const ::godot::Vector2 to = point - held_from_;
    const ::godot::Vector2 kept = at(
        ase::math::clamp(static_cast<double>(to.x), look_.margin_x, look_.frame_w - look_.margin_x - w),
        ase::math::clamp(static_cast<double>(to.y), top, look_.max_bottom - h));
    (pack ? pack_at_ : pouch_at_) = kept;
    return true;
}

bool BagWindow::release() {
    const bool held = held_ != nullptr;
    held_ = nullptr;
    return held;
}

std::string BagWindow::number(double value, bool fine) const {
    // f1: Math.round(v * 10) / 10 to one decimal; kgf: below hud_load_fine_below to two.
    const int decimals = static_cast<int>(fine && value < look_.fine_below ? look_.fine_decimals : look_.decimals);
    return hud_decimals(value, decimals);
}

bool BagWindow::say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
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

void BagWindow::take(const BagWindowFrame& now) {
    pack_def_.carry(now.pack);
    pouch_def_.carry(now.pouch);
}

bool BagWindow::draw(UiPen& canvas, const ::godot::Transform2D& frame, const BagWindowFrame& now,
                     std::string& failed) {
    pack_drawn_ = false;
    pouch_drawn_ = false;
    pack_places_.clear();
    pouch_places_.clear();
    if (!now.pouch_carried) {
        pouch_open_ = false;
    }
    bool drawn = true;
    if (pack_open_) {
        drawn = draw_pack(canvas, frame, now, failed);
    }
    if (drawn && pouch_open_) {
        drawn = draw_pouch(canvas, frame, now, failed);
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return drawn;
}

bool BagWindow::draw_pack(UiPen& canvas, const ::godot::Transform2D& frame, const BagWindowFrame& now,
                          std::string& failed) {
    const BagWindowPackSize& s = look_.pack;
    const BagWindowInks& ink = look_.inks;
    const double x = pack_at_.x;
    const double y = pack_at_.y;
    const double mid = x + s.w * GODOT_HALF;
    const bool hot = same(now.hot, GODOT_HUD_CONTAINER_PACK);
    canvas.draw_set_transform_matrix(frame);

    UiSvgOutline flap = curve(at(x + s.flap_inset, y + s.flap_dy), at(mid, y - s.flap_rise),
                              at(x + s.w - s.flap_inset, y + s.flap_dy));
    flap.closed = true;
    svg_fill(canvas, flap, ink.flap);
    svg_stroke(canvas, flap, ink.edge, s.flap_line);
    svg_stroke_dashed(canvas,
                      curve(at(x + s.seam_inset, y + s.seam_dy), at(mid, y - s.seam_rise),
                            at(x + s.w - s.seam_inset, y + s.seam_dy)),
                      ink.seam, s.seam_line, s.stitch_dash, s.stitch_gap);
    if (!say(canvas, frame, title_, pack_title_, {},
             line_at(mid, y + s.title_dy, s.title_size, GODOT_TEXT_ANCHOR_MIDDLE, ink.title), failed)) {
        return false;
    }

    const UiSvgOutline body = bag_path(x, y, s.w, s.h, s.corner, s.corner_low, s.corner_low_reach);
    svg_fill(canvas, body, ink.body);
    svg_stroke(canvas, body, hot ? (now.refused ? ink.alert : ink.life) : ink.edge, hot ? s.hot_line : s.line);
    const UiSvgOutline field = bag_path(x + s.field_dx, y + s.field_dy, s.w - s.field_shrink_w, s.h - s.field_shrink_h,
                                        s.corner, s.corner_low, s.corner_low_reach);
    svg_fill(canvas, field, ink.inner);
    svg_stroke_dashed(canvas, field, ink.inner_seam, s.field_line, s.stitch_dash, s.stitch_gap);
    close_.draw(canvas, frame, at(x + s.w + s.close_dx, y + s.close_dy), s.close_size, ink.title, 1.0);
    canvas.draw_set_transform_matrix(frame);
    pack_flap_.assign(1, flap);
    pack_body_.assign(1, body);
    pack_drawn_ = true;

    const double foot = y + s.h + s.load_dy;
    const std::vector<BagThing> none;
    const std::vector<BagThing>& things = now.pack_things != nullptr ? *now.pack_things : none;
    BagContentsView view;
    view.corner = at(x + s.content_dx, y + s.content_dy);
    view.ink = ink.title;
    view.selected = now.selected;
    view.dragged = now.dragged;
    if (!pack_contents_.draw(canvas, frame, pack_def_, things, view, pack_places_, failed)) {
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    const ContainerDefCarry& cap = pack_def_.physical();
    if (now.level >= level_load_) {
        const std::pair<double, double> sum = load_of(things);
        if (!say(canvas, frame, data_, load_,
                 {HudPortValue{"kg", number(sum.first, false)}, HudPortValue{"cap_kg", hud_shortest(cap.carry_kg)},
                  HudPortValue{"l", number(sum.second, false)}, HudPortValue{"cap_l", hud_shortest(cap.room_l)}},
                 line_at(x + s.w + s.load_dx, foot, s.load_size, GODOT_TEXT_ANCHOR_END, ink.load), failed)) {
            return false;
        }
    }
    if (now.level >= level_mouth_) {
        return say(canvas, frame, data_, mouth_,
                   {HudPortValue{"cm", hud_shortest(cap.opening_cm)}, HudPortValue{"asset", cap.asset}},
                   line_at(x + s.mouth_dx, foot, s.mouth_size, GODOT_TEXT_ANCHOR_START, ink.spec), failed);
    }
    return true;
}

bool BagWindow::draw_pouch(UiPen& canvas, const ::godot::Transform2D& frame, const BagWindowFrame& now,
                           std::string& failed) {
    const BagWindowPouchSize& s = look_.pouch;
    const BagWindowInks& ink = look_.inks;
    const double x = pouch_at_.x;
    const double y = pouch_at_.y;
    const double mid = x + s.w * GODOT_HALF;
    const double half = y + s.h * GODOT_HALF;
    const bool hot = same(now.hot, GODOT_HUD_CONTAINER_POUCH);
    canvas.draw_set_transform_matrix(frame);

    // From the neck down the left side, over the sagging foot, up the right side, back to the neck.
    UiSvgOutline body;
    const ::godot::Vector2 neck_left = at(x + s.neck_inset, y + s.neck_dy);
    const ::godot::Vector2 foot_left = at(x + s.foot_inset, y + s.h - s.foot_rise);
    const ::godot::Vector2 foot_right = at(x + s.w - s.foot_inset, y + s.h - s.foot_rise);
    const ::godot::Vector2 neck_right = at(x + s.w - s.neck_inset, y + s.neck_dy);
    svg_append(body, neck_left);
    svg_append_quadratic(body, neck_left, at(x - s.bulge, half), foot_left);
    svg_append_quadratic(body, foot_left, at(mid, y + s.h + s.foot_sag), foot_right);
    svg_append_quadratic(body, foot_right, at(x + s.w + s.bulge, half), neck_right);
    body.closed = true;
    svg_fill(canvas, body, ink.pouch_body);
    svg_stroke(canvas, body, hot ? (now.refused ? ink.alert : ink.life) : ink.pouch_edge, hot ? s.hot_line : s.line);
    svg_stroke(canvas,
               curve(at(x + s.fold_dx, y + s.fold_dy), at(x + s.fold_bend_dx, half),
                     at(x + s.fold_end_dx, y + s.h - s.fold_end_rise)),
               ink.pouch_fold, s.fold_line);
    svg_stroke(canvas, curve(neck_left, at(mid, y + s.string_dip), neck_right), ink.pouch_string, s.string_line);
    // The two knotted ends leave the middle of the string: the left one bends left, the right one right.
    for (const double side : {-1.0, 1.0}) {
        const ::godot::Vector2 from = at(mid + side * s.knot_gap, y + s.knot_dy);
        svg_stroke(canvas,
                   curve(from, from + at(side * s.knot_bend_x, -s.knot_bend_y),
                         from + at(side * s.knot_reach_x, -s.knot_reach_y)),
                   ink.pouch_string, s.knot_line);
    }
    if (!say(canvas, frame, title_, pouch_title_, {},
             line_at(x + s.title_dx, y + s.title_dy, s.title_size, GODOT_TEXT_ANCHOR_START, ink.pouch_title),
             failed)) {
        return false;
    }
    close_.draw(canvas, frame, at(x + s.w + s.close_dx, y), s.close_size, ink.pouch_title, 1.0);
    canvas.draw_set_transform_matrix(frame);
    pouch_body_.assign(1, body);
    pouch_drawn_ = true;

    const double foot = y + s.h + s.load_dy;
    const std::vector<BagThing> none;
    const std::vector<BagThing>& things = now.pouch_things != nullptr ? *now.pouch_things : none;
    BagContentsView view;
    view.corner = at(x + s.content_dx, y + s.content_dy);
    view.ink = ink.pouch_ink;
    view.selected = now.selected;
    view.dragged = now.dragged;
    if (!pouch_contents_.draw(canvas, frame, pouch_def_, things, view, pouch_places_, failed)) {
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    if (now.level < level_load_) {
        return true;
    }
    const std::pair<double, double> sum = load_of(things);
    return say(canvas, frame, data_, load_pouch_,
               {HudPortValue{"kg", number(sum.first, true)}, HudPortValue{"l", number(sum.second, false)},
                HudPortValue{"cap_l", hud_shortest(pouch_def_.physical().room_l)}},
               line_at(mid, foot, s.load_size, GODOT_TEXT_ANCHOR_MIDDLE, ink.pouch_load), failed);
}

void BagWindow::close_places(::godot::Rect2& pack, bool& pack_drawn, ::godot::Rect2& pouch, bool& pouch_drawn) const {
    const BagWindowPackSize& p = look_.pack;
    const BagWindowPouchSize& s = look_.pouch;
    pack = ::godot::Rect2(pack_at_ + at(p.w + p.close_hit_dx, p.close_hit_dy), at(p.close_hit_w, p.close_hit_h));
    pouch = ::godot::Rect2(pouch_at_ + at(s.w + s.close_hit_dx, s.close_hit_dy), at(s.close_hit_w, s.close_hit_h));
    pack_drawn = pack_drawn_;
    pouch_drawn = pouch_drawn_;
}

void BagWindow::grip_places(::godot::Rect2& pack, bool& pack_drawn, ::godot::Rect2& pouch, bool& pouch_drawn) const {
    const BagWindowPouchSize& s = look_.pouch;
    pouch = ::godot::Rect2(pouch_at_ + at(s.grip_dx, s.grip_dy), at(s.w - s.grip_shrink_w, s.grip_h));
    pouch_drawn = pouch_drawn_;
    pack = ::godot::Rect2();
    pack_drawn = pack_drawn_ && !pack_flap_.empty() && !pack_body_.empty() && !pack_flap_[0].points.is_empty() &&
                 !pack_body_[0].points.is_empty();
    if (!pack_drawn) {
        return;
    }
    // The flap's box, cut at the top of the body: hit() asks the body before the flap, so only the strip above it
    // answers the grip.
    ::godot::Rect2 flap(pack_flap_[0].points[0], ::godot::Vector2());
    for (const ::godot::Vector2& point : pack_flap_[0].points) {
        flap = flap.expand(point);
    }
    float body_top = pack_body_[0].points[0].y;
    for (const ::godot::Vector2& point : pack_body_[0].points) {
        body_top = ase::math::min(body_top, point.y);
    }
    const float bottom = ase::math::min(flap.get_end().y, body_top);
    pack = ::godot::Rect2(flap.position, ::godot::Vector2(flap.size.x, ase::math::max(0.0f, bottom - flap.position.y)));
    pack_drawn = pack.size.y > 0.0f;
}

BagWindowHit BagWindow::hit(const ::godot::Vector2& point) const {
    ::godot::Rect2 pack_close;
    ::godot::Rect2 pouch_close;
    bool pack_shown = false;
    bool pouch_shown = false;
    close_places(pack_close, pack_shown, pouch_close, pouch_shown);
    if (pouch_shown) {
        const BagWindowPouchSize& s = look_.pouch;
        const std::string key = pouch_contents_.hit(pouch_places_, point);
        if (!key.empty()) {
            return BagWindowHit{GODOT_HUD_ACT_ITEM, key};
        }
        if (pouch_close.has_point(point)) {
            return BagWindowHit{GODOT_HUD_ACT_POUCH_CLOSE, std::string()};
        }
        if (::godot::Rect2(pouch_at_ + at(s.grip_dx, s.grip_dy), at(s.w - s.grip_shrink_w, s.grip_h))
                .has_point(point)) {
            return BagWindowHit{GODOT_HUD_ACT_POUCH_GRIP, std::string()};
        }
        if (!pouch_body_.empty() && inside(pouch_body_[0], point)) {
            return BagWindowHit{GODOT_HUD_ACT_WINDOW_BODY, std::string()};
        }
    }
    if (pack_shown) {
        const std::string key = pack_contents_.hit(pack_places_, point);
        if (!key.empty()) {
            return BagWindowHit{GODOT_HUD_ACT_ITEM, key};
        }
        if (pack_close.has_point(point)) {
            return BagWindowHit{GODOT_HUD_ACT_PACK_CLOSE, std::string()};
        }
        if (!pack_body_.empty() && inside(pack_body_[0], point)) {
            return BagWindowHit{GODOT_HUD_ACT_WINDOW_BODY, std::string()};
        }
        if (!pack_flap_.empty() && inside(pack_flap_[0], point)) {
            return BagWindowHit{GODOT_HUD_ACT_PACK_GRIP, std::string()};
        }
    }
    return BagWindowHit{};
}

BagWindowDrop BagWindow::over(const ::godot::Vector2& point) const {
    const BagWindowPackSize& p = look_.pack;
    const BagWindowPouchSize& q = look_.pouch;
    const double px = point.x;
    const double py = point.y;
    if (pouch_drawn_ && px > pouch_at_.x && px < pouch_at_.x + q.w && py > pouch_at_.y - q.drop_rise &&
        py < pouch_at_.y + q.h + q.drop_sink) {
        return BagWindowDrop{GODOT_HUD_CONTAINER_POUCH, pouch_at_ + at(q.content_dx, q.content_dy), &pouch_def_};
    }
    if (pack_drawn_ && px > pack_at_.x && px < pack_at_.x + p.w && py > pack_at_.y - p.drop_rise &&
        py < pack_at_.y + p.h) {
        return BagWindowDrop{GODOT_HUD_CONTAINER_PACK, pack_at_ + at(p.content_dx, p.content_dy), &pack_def_};
    }
    return BagWindowDrop{};
}

}  // namespace ase::adp::godot

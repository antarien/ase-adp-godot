/**
 * ASE GODOT ADAPTER - HUD ARC SIGN
 *
 * @file        godot_hud_arc_sign.cpp
 * @brief       ArcSign: one occurrence on a ring, a sign on a dark disc, a node or a dot, see godot_hud_arc_sign.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, mockup ringLay REG, glyphAt and tan.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_sign.hpp>
#include <ase/adp/godot/godot_hud_arc_pure.hpp>
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** The text token `token` as UTF-8; false when the theme carries none. */
bool text_token(const UiTheme& theme, const std::string& token, std::string& out) {
    ::godot::String text;
    if (!theme.token_text(token, text)) {
        return false;
    }
    out = utf8_of(text);
    return true;
}

/** The kinds of the register in the order of the names, the lunar eclipse beside the solar one. */
constexpr const char* kKindNames[] = {GODOT_ARC_KIND_PERIOD,   GODOT_ARC_KIND_DAWN,         GODOT_ARC_KIND_DUSK,
                                      GODOT_ARC_KIND_MOONRISE, GODOT_ARC_KIND_HUNT,         GODOT_ARC_KIND_FULLMOON,
                                      GODOT_ARC_KIND_CONJ,     GODOT_ARC_KIND_ECLIPSE,      GODOT_ARC_KIND_ECLIPSE_LUNAR,
                                      GODOT_ARC_KIND_STAGE,    GODOT_ARC_KIND_SEASON,       GODOT_ARC_KIND_VECTUM,
                                      GODOT_ARC_KIND_ARCTURUS, GODOT_ARC_KIND_ANTARES,      GODOT_ARC_KIND_CONJ_GREAT,
                                      GODOT_ARC_KIND_TUMBRAE,  GODOT_ARC_KIND_LUNBREX};

/** The seasons in the order of the codes the calendar numbers them with, as the band and the year ring read them. */
constexpr const char* kSeasonColors[] = {"SEASON_GLACIX", "SEASON_SALTATRIX", "SEASON_IGNIX", "SEASON_HALITRIX"};

/** A place in logical pixels as the flank line writes it. */
std::string pixels(const ::godot::Transform2D& frame, const ::godot::Vector2& at) {
    const ::godot::Vector2 logical = frame.xform(at);
    return hud_decimals(static_cast<double>(logical.x), GODOT_ARC_PIXEL_DECIMALS) + " y=" +
           hud_decimals(static_cast<double>(logical.y), GODOT_ARC_PIXEL_DECIMALS);
}

}  // namespace

bool ArcSign::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_arc_mark_node", &mark_node_},
             {"hud_arc_mark_dot", &mark_dot_},
             {"hud_arc_mark_glyph", &mark_glyph_},
             {"hud_arc_mark_turn", &mark_turn_},
             {"hud_arc_mark_start", &mark_start_},
             {"hud_arc_eclipse_lunar_body", &lunar_body_},
             {"hud_arc_node_day_radius", &node_day_r_},
             {"hud_arc_node_radius", &node_r_},
             {"hud_arc_node_fill_alpha", &node_fill_alpha_},
             {"hud_arc_node_line", &node_line_},
             {"hud_arc_node_label_outset", &node_outset_},
             {"hud_arc_node_label_size", &node_size_},
             {"hud_arc_period_label_inset", &period_inset_},
             {"hud_arc_period_label_size", &period_size_},
             {"hud_arc_dot_radius", &dot_r_},
             {"hud_arc_day_turn_radius", &turn_r_},
             {"hud_arc_day_turn_label_inset", &turn_inset_},
             {"hud_arc_day_turn_label_deg", &turn_deg_},
             {"hud_arc_day_turn_label_size", &turn_size_},
             {"hud_arc_lun_start_tick_half", &start_half_},
             {"hud_arc_lun_start_tick_line", &start_line_},
             {"hud_arc_lun_label_inset", &start_inset_},
             {"hud_arc_lun_label_size", &start_size_},
             {"hud_arc_sign_day_size", &sign_day_size_},
             {"hud_arc_sign_size", &sign_size_},
             {"hud_arc_sign_disc_share", &disc_share_},
             {"hud_arc_sign_disc_line", &disc_line_},
             {"hud_arc_sign_day_label_outset", &day_outset_},
             {"hud_arc_sign_label_outset", &outset_},
             {"hud_arc_sign_day_label_size", &day_label_},
             {"hud_arc_sign_label_size", &label_},
             {"hud_arc_sign_label_alpha", &label_alpha_},
             {"hud_arc_label_central", &central_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK", &ink_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_CHIP_OFF", &chip_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    seasons_.clear();
    for (const char* name : kSeasonColors) {
        ::godot::Color color;
        if (!theme.color(name, color)) {
            failed = name;
            return false;
        }
        seasons_.push_back(color);
    }
    if (!text_token(theme, "hud_arc_domain_ink", domain_ink_)) {
        failed = "hud_arc_domain_ink";
        return false;
    }
    if (!text_token(theme, "hud_arc_domain_season", domain_season_)) {
        failed = "hud_arc_domain_season";
        return false;
    }
    if (!text_token(theme, "hud_arc_domain_soft", domain_soft_)) {
        failed = "hud_arc_domain_soft";
        return false;
    }
    looks_.clear();
    for (const char* name : kKindNames) {
        Look one;
        one.name = name;
        const std::string prefix = std::string("hud_arc_kind_") + name;
        if (!theme.measure(prefix + "_mark", one.mark)) {
            failed = prefix + "_mark";
            return false;
        }
        if (!text_token(theme, prefix + "_domain", one.domain) || !text_token(theme, prefix + "_word", one.word)) {
            failed = prefix + "_domain, _word";
            return false;
        }
        if (one.mark == mark_glyph_ && !bind_word_icon(theme, std::string("arc.glyph.") + name, one.glyph, failed)) {
            return false;
        }
        looks_.push_back(one);
    }
    return true;
}

const ArcSign::Look* ArcSign::look(const std::string& name) const {
    for (const Look& each : looks_) {
        if (each.name == name) {
            return &each;
        }
    }
    return nullptr;
}

std::string ArcSign::name_with(const std::string& kind, double body) const {
    return kind == GODOT_ARC_KIND_ECLIPSE && body == lunar_body_ ? std::string(GODOT_ARC_KIND_ECLIPSE_LUNAR) : kind;
}

std::string ArcSign::name_of(const ArcSignFrame& sign) const {
    return name_with(sign.kind, sign.body);
}

::godot::Vector2 ArcSign::below(const ::godot::Vector2& middle, double size) const {
    return middle + ::godot::Vector2(0.0f, static_cast<float>(size * central_));
}

bool ArcSign::tone_of(const std::string& kind, double body, ::godot::Color& out, std::string& failed) const {
    const Look* found = look(name_with(kind, body));
    if (found == nullptr) {
        failed = "hud_arc_kind_" + kind;
        return false;
    }
    // A body the theme sets apart carries a domain of its own, the kind's domain token with the body appended
    // (LUNIX SANGUEX and TENEBRAX MAGNORUM in alert); every other body keeps the domain of its kind.
    std::string domain = found->domain;
    std::string own;
    if (whole(body) && text_token(*theme_, "hud_arc_kind_" + found->name + "_domain_" +
                                               hud_whole(static_cast<int64_t>(body)), own)) {
        domain = own;
    }
    if (domain == domain_season_) {
        const size_t season = static_cast<size_t>(body);
        if (!whole(body) || season >= seasons_.size()) {
            failed = "arc: the body " + hud_shortest(body) + " of a season names none";
            return false;
        }
        out = seasons_[season];
        return true;
    }
    if (domain == domain_ink_) {
        out = ink_;
        return true;
    }
    if (domain == domain_soft_) {
        out = soft_;
        return true;
    }
    if (!theme_->tone(domain, out)) {
        failed = "DOMAIN_" + domain;
        return false;
    }
    return true;
}

bool ArcSign::word_of(const std::string& kind, double body, std::vector<HudRun>& out, std::string& failed) const {
    const Look* found = look(name_with(kind, body));
    if (found == nullptr) {
        failed = "hud_arc_kind_" + kind;
        return false;
    }
    const std::string number = hud_whole(static_cast<int64_t>(body));
    if (!theme_->words(found->word + "." + number, out) && !theme_->words(found->word, out)) {
        failed = "texts.hud." + found->word;
        return false;
    }
    return fill_words(out, {HudPortValue{"n", number}}, failed);
}

bool ArcSign::lay(const ArcSignFrame& sign, uint32_t ring, ArcLaidSign& out, std::string& failed) const {
    out = ArcLaidSign{};
    const std::string name = name_of(sign);
    const Look* found = look(name);
    if (found == nullptr) {
        failed = "hud_arc_kind_" + sign.kind;
        return false;
    }
    out.kind = name;
    out.body = sign.body;
    out.share = sign.share;
    out.span = sign.span;
    out.ring = ring;
    out.node = found->mark == mark_node_;
    out.dot = found->mark == mark_dot_;
    out.glyph = found->glyph;
    out.turn = found->mark == mark_turn_;
    out.start = found->mark == mark_start_;
    if (!tone_of(sign.kind, sign.body, out.tone, failed)) {
        return false;
    }
    out.has_word = !out.dot;
    if (!out.has_word) {
        return true;
    }
    std::vector<HudRun> runs;
    if (!word_of(sign.kind, sign.body, runs, failed)) {
        return false;
    }
    const bool bound = out.turn ? out.word.bind_title(*theme_, failed) : out.word.bind_label(*theme_, failed);
    return bound && out.word.set(runs, failed);
}

void ArcSign::draw_mark(UiPen& canvas, const ::godot::Transform2D& frame, const ArcSignPlace& place,
                        const ArcLaidSign& sign, const ::godot::Vector2& at, double opacity) const {
    const bool day = place.ring == GODOT_ARC_SLOT_DAY;
    if (sign.dot) {
        canvas.draw_circle(at, static_cast<float>(dot_r_), UiArcFade::faded(sign.tone, opacity), true, -1.0f, true);
        return;
    }
    if (sign.turn) {
        canvas.draw_circle(at, static_cast<float>(turn_r_), UiArcFade::faded(sign.tone, opacity), true, -1.0f, true);
        return;
    }
    if (sign.start) {
        const double degrees = arc_disc_angle(sign.share, place.now, place.half_deg);
        canvas.draw_line(UiRing::point(place.centre, place.radius - start_half_, arc_ring_angle(degrees)),
                         UiRing::point(place.centre, place.radius + start_half_, arc_ring_angle(degrees)),
                         UiArcFade::faded(sign.tone, opacity), static_cast<float>(start_line_), true);
        return;
    }
    if (sign.node) {
        const float radius = static_cast<float>(day ? node_day_r_ : node_r_);
        canvas.draw_circle(at, radius, UiArcFade::faded(chip_, opacity * node_fill_alpha_), true, -1.0f, true);
        canvas.draw_circle(at, radius, UiArcFade::faded(sign.tone, opacity), false, static_cast<float>(node_line_), true);
        return;
    }
    const double size = day ? sign_day_size_ : sign_size_;
    const float disc = static_cast<float>(size * disc_share_);
    canvas.draw_circle(at, disc, UiArcFade::faded(chip_, opacity * node_fill_alpha_), true, -1.0f, true);
    canvas.draw_circle(at, disc, UiArcFade::faded(sign.tone, opacity), false, static_cast<float>(disc_line_), true);
    sign.glyph.draw(canvas, frame, at, size, sign.tone, opacity);
}

void ArcSign::draw_word(UiPen& canvas, const ::godot::Transform2D& frame, const ArcSignPlace& place,
                        ArcLaidSign& sign, double degrees, double opacity) const {
    const bool day = place.ring == GODOT_ARC_SLOT_DAY;
    double radius = place.radius + (day ? day_outset_ : outset_);
    double size = day ? day_label_ : label_;
    double turned = degrees;
    ::godot::Color color = sign.tone;
    double shown = opacity * label_alpha_;
    if (sign.node) {
        size = day ? period_size_ : node_size_;
        radius = day ? place.radius - period_inset_ : place.radius + node_outset_;
        // The word of a period stands at the middle of its span, inside the ring; the others stand at their node.
        turned = day ? arc_disc_angle(sign.share + sign.span * static_cast<double>(GODOT_HALF), place.now,
                                      place.half_deg)
                     : degrees;
        color = day ? soft_ : sign.tone;
        shown = opacity;
    }
    // The turn of a day and the start of a month: the word tangent inside the ring, in the tone of the kind; the
    // turn's word stands hud_arc_day_turn_label_deg after its dot, the month's at its tick.
    if (sign.turn || sign.start) {
        size = sign.turn ? turn_size_ : start_size_;
        radius = place.radius - (sign.turn ? turn_inset_ : start_inset_);
        color = sign.tone;
        shown = opacity;
    }
    if (sign.turn) {
        turned = degrees + turn_deg_;
    }
    const ::godot::Vector2 middle = UiRing::point(place.centre, radius, arc_ring_angle(turned));
    UiTextPlace text;
    text.at = below(middle, size);
    text.pivot = middle;
    text.rotate_deg = -turned;
    text.size = size;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.color = color;
    text.opacity = shown;
    text.elapsed = place.elapsed;
    text.still = place.still;
    sign.word.draw(canvas, frame, text);
}

void ArcSign::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ArcSignPlace& place, ArcLaidSign& sign,
                   std::vector<std::string>& flank) const {
    const double degrees = arc_disc_angle(sign.share, place.now, place.half_deg);
    const ::godot::Vector2 at = UiRing::point(place.centre, place.radius, arc_ring_angle(degrees));
    if (!sign.announced) {
        sign.announced = true;
        flank.push_back(std::string(GODOT_ARC_LINE_SIGN) + " kind=" + sign.kind + " body=" + hud_shortest(sign.body) +
                        " ring=" + std::to_string(place.ring) + " x=" + pixels(frame, at));
    }
    const double opacity = place.fade->opacity(static_cast<double>(at.x));
    if (!(opacity > 0.0)) {
        return;
    }
    draw_mark(canvas, frame, place, sign, at, opacity);
    if (sign.has_word) {
        draw_word(canvas, frame, place, sign, degrees, opacity);
    }
}

}  // namespace ase::adp::godot

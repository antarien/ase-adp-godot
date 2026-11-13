/**
 * ASE GODOT ADAPTER - HUD TRAIL VIEW
 *
 * @file        godot_hud_trail_view.cpp
 * @brief       The trail on arrival: how long the keeper was away and the last events, down to the play chip
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_trail_view.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0450 (flora.trail.v1: the past events with date, domain, words and case), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.01.00003
 */

#include <ase/adp/godot/godot_hud_trail_view.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool TrailView::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    drawn_.clear();
    play_drawn_ = false;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_arrive_dim_y", &dim_y_},
             {"hud_arrive_dim_h", &dim_h_},
             {"hud_arrive_x", &x_},
             {"hud_arrive_title_y", &title_y_},
             {"hud_arrive_title_size", &title_size_},
             {"hud_arrive_span_y", &span_y_},
             {"hud_arrive_span_size", &span_size_},
             {"hud_arrive_span_delay", &span_delay_},
             {"hud_arrive_spine_top", &spine_top_},
             {"hud_arrive_spine_tail", &spine_tail_},
             {"hud_arrive_events", &events_},
             {"hud_arrive_row_y", &row_y_},
             {"hud_arrive_row_step", &row_step_},
             {"hud_arrive_row_delay", &row_delay_},
             {"hud_arrive_row_stagger", &row_stagger_},
             {"hud_arrive_pulse_radius", &pulse_radius_},
             {"hud_arrive_dot_radius", &dot_radius_},
             {"hud_arrive_tick_from", &tick_from_},
             {"hud_arrive_tick_to", &tick_to_},
             {"hud_arrive_text_x", &text_x_},
             {"hud_arrive_date_dy", &date_dy_},
             {"hud_arrive_date_size", &date_size_},
             {"hud_arrive_event_dy", &event_dy_},
             {"hud_arrive_event_size", &event_size_},
             {"hud_arrive_detail_dy", &detail_dy_},
             {"hud_arrive_detail_size", &detail_size_},
             {"hud_arrive_case_dy", &case_dy_},
             {"hud_arrive_case_size", &case_size_},
             {"hud_arrive_hit_x", &hit_x_},
             {"hud_arrive_hit_dy", &hit_dy_},
             {"hud_arrive_hit_w", &hit_w_},
             {"hud_arrive_hit_h", &hit_h_},
             {"hud_arrive_play_x", &play_x_},
             {"hud_arrive_play_y", &play_y_},
             {"hud_arrive_play_radius", &play_radius_},
             {"hud_arrive_play_delay", &play_delay_},
             {"hud_arrive_play_label_y", &play_label_y_},
             {"hud_arrive_play_label_size", &play_label_size_},
             {"hud_arrive_play_label_delay", &play_label_delay_},
             {"hud_line", &pulse_line_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_SCRIM_TRAIL", &scrim_},
                                                                         {"OVERLAY_INK", &ink_},
                                                                         {"OVERLAY_INK_SOFT", &soft_},
                                                                         {"OVERLAY_INK_FAINT", &faint_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }

    // The threshold of the case line is the spec's level, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_case_ = levels[2];

    rows_.clear();
    rows_.resize(static_cast<size_t>(events_));
    for (Row& row : rows_) {
        if (!row.date.bind_data(theme, failed) || !row.title.bind_title(theme, failed) ||
            !row.line.bind_label(theme, failed) || !row.cased.bind_data(theme, failed)) {
            return false;
        }
    }

    std::string glyph;
    if (!word_icon(theme, "arrive_play_glyph", glyph, failed) || !play_.bind(theme, failed) ||
        !play_.set_glyph(glyph, std::string(), failed)) {
        return false;
    }
    play_place_.centre = ::godot::Vector2(static_cast<float>(play_x_), static_cast<float>(play_y_));
    play_place_.radius = play_radius_;
    play_place_.tone = ink_;
    play_place_.fade = true;
    play_place_.delay = play_delay_;

    return title_.bind_title(theme, failed) && span_.bind_data(theme, failed) &&
           play_label_.bind_label(theme, failed) && set_word(title_, "arrive_title", {}, failed) &&
           set_word(play_label_, "arrive_play", {}, failed) && spine_.bind(theme, failed) &&
           fade_.bind(theme, "hud_anim_fade", failed) && ring_.bind(theme, failed);
}

bool TrailView::set_word(UiTextBlock& block, const std::string& key, const std::vector<HudPortValue>& values,
                         std::string& failed) const {
    std::vector<HudRun> runs;
    if (!theme_->words(key, runs)) {
        failed = "texts.hud." + key;
        return false;
    }
    return fill_words(runs, values, failed) && block.set(runs, failed);
}

void TrailView::put(UiTextBlock& block, UiPen& canvas, const ::godot::Transform2D& frame,
                    const TrailViewFrame& now, double x, double y, double size, const ::godot::Color& color,
                    double anchor, double delay) const {
    UiTextPlace place;
    place.at = ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
    place.size = size;
    place.anchor = anchor;
    place.color = color;
    place.opacity = 1.0;
    place.fade = true;
    place.delay = delay;
    place.elapsed = now.elapsed;
    place.still = now.still;
    block.draw(canvas, frame, place);
}

bool TrailView::draw_row(UiPen& canvas, const ::godot::Transform2D& frame, const TrailViewFrame& now,
                         const TrailEvent& event, size_t i, bool newest, double y, std::string& failed) {
    ::godot::Color tone;
    if (!theme_->tone(event.tone, tone)) {
        failed = "DOMAIN_" + event.tone;
        return false;
    }
    Row& row = rows_[i];
    if (!set_word(row.date, "arrive_date",
                  {HudPortValue{"lun", hud_whole(event.lun)}, HudPortValue{"tum", hud_whole(event.tum)}},
                  failed) ||
        !set_word(row.title, "trail." + event.kind + ".title", event.values, failed) ||
        !set_word(row.line, "trail." + event.kind + ".line", event.values, failed) ||
        !row.cased.set(event.case_id, failed)) {
        return false;
    }

    const double delay = row_delay_ + static_cast<double>(i) * row_stagger_;
    const double shown = fade_.progress(now.elapsed, delay, now.still);
    const ::godot::Vector2 dot(static_cast<float>(x_), static_cast<float>(y));
    ::godot::Color tick = tone;
    tick.a *= static_cast<float>(shown);
    canvas.draw_set_transform_matrix(frame);
    if (newest) {
        ring_.pulse(canvas, dot, pulse_radius_, tone, pulse_line_, now.elapsed, now.still);
    }
    ring_.disc(canvas, dot, dot_radius_, tick);
    canvas.draw_line(::godot::Vector2(static_cast<float>(tick_from_), static_cast<float>(y)),
                     ::godot::Vector2(static_cast<float>(tick_to_), static_cast<float>(y)), tick,
                     static_cast<float>(spine_.width()));
    canvas.draw_set_transform_matrix(::godot::Transform2D());

    put(row.date, canvas, frame, now, text_x_, y + date_dy_, date_size_, soft_, GODOT_TEXT_ANCHOR_START, delay);
    put(row.title, canvas, frame, now, text_x_, y + event_dy_, event_size_, event.alert ? tone : ink_,
        GODOT_TEXT_ANCHOR_START, delay);
    put(row.line, canvas, frame, now, text_x_, y + detail_dy_, detail_size_, soft_, GODOT_TEXT_ANCHOR_START, delay);
    if (now.level >= level_case_) {
        put(row.cased, canvas, frame, now, text_x_, y + case_dy_, case_size_, faint_, GODOT_TEXT_ANCHOR_START,
            delay);
    }
    drawn_.push_back(Drawn{::godot::Rect2(static_cast<float>(hit_x_), static_cast<float>(y + hit_dy_),
                                          static_cast<float>(hit_w_), static_cast<float>(hit_h_)),
                           event.sign});
    return true;
}

bool TrailView::draw(UiPen& canvas, const ::godot::Transform2D& frame, const TrailViewFrame& now,
                     std::string& failed) {
    drawn_.clear();
    play_drawn_ = false;
    if (theme_ == nullptr) {
        failed = "TrailView drawn before it was bound";
        return false;
    }

    canvas.draw_set_transform_matrix(frame);
    canvas.draw_rect(hud_cover_dim(now.cover, dim_y_, dim_h_), scrim_, true);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    put(title_, canvas, frame, now, x_, title_y_, title_size_, ink_, GODOT_TEXT_ANCHOR_START, 0.0);

    // The span and the rows stand in full from the frame; who filled it, flora.trail.v1 or the bridge, the view
    // never asks (VivariumView::fill_hud logs N0450 on its edge).
    if (!set_word(span_, "arrive_span",
                  {HudPortValue{"days", hud_whole(now.days)}, HudPortValue{"lun", hud_whole(now.lun)},
                   HudPortValue{"from", hud_whole(now.from)}, HudPortValue{"to", hud_whole(now.to)}},
                  failed)) {
        return false;
    }
    put(span_, canvas, frame, now, x_, span_y_, span_size_, soft_, GODOT_TEXT_ANCHOR_START, span_delay_);

    const size_t shown = now.events.size() < rows_.size() ? now.events.size() : rows_.size();
    const size_t first = now.events.size() - shown;
    if (shown > 0) {
        // arrive(): the spine runs from its top over (n - 1) steps plus the tail, n the length of the whole trail.
        const double last =
            spine_top_ + static_cast<double>(now.events.size() - 1) * row_step_ + spine_tail_;
        ::godot::PackedVector2Array spine;
        spine.push_back(::godot::Vector2(static_cast<float>(x_), static_cast<float>(spine_top_)));
        spine.push_back(::godot::Vector2(static_cast<float>(x_), static_cast<float>(last)));
        canvas.draw_set_transform_matrix(frame);
        spine_.draw(canvas, spine, faint_, spine_.width(), spine_.drawn(now.elapsed, 0.0, now.still));
        canvas.draw_set_transform_matrix(::godot::Transform2D());
    }
    for (size_t i = 0; i < shown; ++i) {
        const double y = row_y_ + static_cast<double>(i) * row_step_;
        if (!draw_row(canvas, frame, now, now.events[first + i], i, i + 1 == shown, y, failed)) {
            return false;
        }
    }

    play_place_.elapsed = now.elapsed;
    play_place_.still = now.still;
    play_.draw(canvas, frame, play_place_);
    put(play_label_, canvas, frame, now, play_x_, play_label_y_, play_label_size_, ink_, GODOT_TEXT_ANCHOR_MIDDLE,
        play_label_delay_);
    play_drawn_ = true;
    return true;
}

bool TrailView::hit(const ::godot::Vector2& point, uint32_t& sign) const {
    if (play_drawn_ && play_.hit(play_place_, point)) {
        sign = UINT32_MAX;
        return true;
    }
    for (const Drawn& row : drawn_) {
        if (row.box.has_point(point)) {
            sign = row.sign;
            return true;
        }
    }
    return false;
}

void TrailView::play_place(::godot::Rect2& place, bool& hidden) const {
    hidden = !play_drawn_;
    const float radius = static_cast<float>(play_place_.radius);
    place = ::godot::Rect2(play_place_.centre.x - radius, play_place_.centre.y - radius, 2.0f * radius,
                           2.0f * radius);
}

bool TrailView::row_place(size_t index, ::godot::Rect2& box, uint32_t& sign) const {
    if (index >= drawn_.size()) {
        return false;
    }
    box = drawn_[index].box;
    sign = drawn_[index].sign;
    return true;
}

}  // namespace ase::adp::godot

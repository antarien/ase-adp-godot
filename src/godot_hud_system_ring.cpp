/**
 * ASE GODOT ADAPTER - HUD SYSTEM RING
 *
 * @file        godot_hud_system_ring.cpp
 * @brief       The system ring of the mode sys: hold it to reset the patch, with its progress arc, sentence and close
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_system_ring.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    ecs/module
 * @usecase     usecase:101019 - start the client window
 * @actor       Vivarium Steward (VIVARIUSTE)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_system_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/types.hpp>

#include <initializer_list>
#include <utility>
#include <vector>

namespace ase::adp::godot {

bool SystemRing::bind(const UiTheme& theme, std::string& failed) {
    double hit_x = 0.0;
    double hit_y = 0.0;
    double hit_w = 0.0;
    double hit_h = 0.0;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_sys_dim_y", &dim_y_},
             {"hud_sys_dim_h", &dim_h_},
             {"hud_sys_x", &x_},
             {"hud_sys_y", &y_},
             {"hud_sys_radius", &radius_},
             {"hud_sys_arc_width", &arc_width_},
             {"hud_sys_glyph_size", &glyph_size_},
             {"hud_sys_hit_radius", &hit_radius_},
             {"hud_sys_title_y", &title_y_},
             {"hud_sys_title_size", &title_size_},
             {"hud_sys_sentence_y", &sentence_y_},
             {"hud_sys_sentence_size", &sentence_size_},
             {"hud_sys_hint_y", &hint_y_},
             {"hud_sys_hint_size", &hint_size_},
             {"hud_line", &line_},
             {"hud_gesture_system_hold_ms", &hold_ms_},
             {"hud_sheet_close_x", &close_x_},
             {"hud_sheet_close_y", &close_y_},
             {"hud_sheet_close_size", &close_size_},
             {"hud_sheet_close_hit_x", &hit_x},
             {"hud_sheet_close_hit_y", &hit_y},
             {"hud_sheet_close_hit_w", &hit_w},
             {"hud_sheet_close_hit_h", &hit_h}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(hold_ms_ > 0.0)) {
        failed = "hud_gesture_system_hold_ms";
        return false;  // a hold of no length would reset on the first touch
    }
    close_hit_ = ::godot::Rect2(static_cast<float>(hit_x), static_cast<float>(hit_y), static_cast<float>(hit_w),
                                static_cast<float>(hit_h));
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_SCRIM_SYSTEM", &scrim_},
                                                                       {"DOMAIN_ALERT_HOLD", &hold_},
                                                                       {"DOMAIN_ALERT", &alert_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    std::vector<HudRun> title;
    std::vector<HudRun> sentence;
    std::vector<HudRun> hint;
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {"reset_title", &title}, {"reset_line", &sentence}, {"reset_hold", &hint}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    return bind_word_icon(theme, "reset_glyph", glyph_, failed) &&
           bind_word_icon(theme, "window_close", close_, failed) && title_.bind_title(theme, failed) &&
           sentence_.bind_label(theme, failed) && hint_.bind_label(theme, failed) && title_.set(title, failed) &&
           sentence_.set(sentence, failed) && hint_.set(hint, failed) && ring_.bind(theme, failed);
}

double SystemRing::share(double held) const {
    const double share = held * GODOT_MS_PER_SECOND / hold_ms_;
    if (share > 1.0) {
        return 1.0;
    }
    return share > 0.0 ? share : 0.0;
}

bool SystemRing::full(double held) const {
    return held * GODOT_MS_PER_SECOND >= hold_ms_;
}

bool SystemRing::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SystemRingFrame& now,
                      std::string& failed) {
    (void)failed;
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));

    // The dim, the disc with its ring, and the arc of the held share.
    canvas.draw_set_transform_matrix(frame);
    canvas.draw_rect(hud_cover_dim(now.cover, dim_y_, dim_h_), scrim_, true);
    ring_.disc(canvas, centre, radius_, hold_);
    ring_.circle(canvas, centre, radius_, faint_, line_, 1.0);
    UiArcStyle style;
    style.opacity = 1.0;
    style.drawn = 1.0;
    ring_.arc(canvas, centre, radius_, 0.0, GODOT_RING_TURN_DEG * share(now.held), alert_, arc_width_, style);
    canvas.draw_set_transform_matrix(::godot::Transform2D());

    // G(X, Y, HISTORY, glyph_size, alert), then the three lines, middle-anchored.
    glyph_.draw(canvas, frame, centre, glyph_size_, alert_, 1.0);
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(title_y_));
    text.size = title_size_;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.color = alert_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    title_.draw(canvas, frame, text);
    UiTextPlace line = text;
    line.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(sentence_y_));
    line.size = sentence_size_;
    line.color = soft_;
    sentence_.draw(canvas, frame, line);
    UiTextPlace hint = text;
    hint.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(hint_y_));
    hint.size = hint_size_;
    hint.color = faint_;
    hint_.draw(canvas, frame, hint);

    // The close, where the calendar sheet has it.
    close_.draw(canvas, frame, ::godot::Vector2(static_cast<float>(close_x_), static_cast<float>(close_y_)),
                close_size_, soft_, 1.0);
    return true;
}

bool SystemRing::hit(const ::godot::Vector2& point, SystemRingHit& out) const {
    out = SystemRingHit{};
    if (close_hit_.has_point(point)) {
        out.rest = true;
        return true;
    }
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));
    if (static_cast<double>(point.distance_to(centre)) <= hit_radius_) {
        out.hold = true;
        return true;
    }
    return false;
}

::godot::Rect2 SystemRing::hold_place() const {
    return ::godot::Rect2(static_cast<float>(x_ - hit_radius_), static_cast<float>(y_ - hit_radius_),
                          static_cast<float>(2.0 * hit_radius_), static_cast<float>(2.0 * hit_radius_));
}

::godot::Rect2 SystemRing::close_place() const {
    return close_hit_;
}

}  // namespace ase::adp::godot

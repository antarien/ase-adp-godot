#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI RING
 *
 * @file        godot_hud_ui_ring.hpp
 * @brief       Ring, arc from a start to an end angle, arc in steps, and pulse: the round marks of the HUD
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiRing. The mockup draws every round
 *              mark with four forms, and this unit is the one drawer of them:
 *
 *              THE RING: a circle of the focus, the soil ring, the bag, the system ring, the year and the day
 *              ring - a disc in a veil colour under an outline in a line colour (circle and disc).
 *
 *              THE ARC: ARC(cx, cy, r, a0, a1, col, w, o) over arcP - an angle in degrees from the top,
 *              clockwise; a full turn is cut just short of 360 so the path stays one arc, a sweep under a tenth
 *              of a degree draws nothing (types.hpp GODOT_RING_*). o.draw lets the arc grow from its start
 *              (.draw, the tokens hud_anim_draw_ms and hud_anim_draw_ease), o.op is its opacity, o.cap a round
 *              end, o.d its delay. The soil water and growth half rings, the season arcs of the year ring,
 *              the light arc of the day ring and the system ring's hold are this form.
 *
 *              THE STEPS: the three arcs of pouring (i * step + from to i * step + to, one colour each from
 *              the caller: poured, filled, empty) - the same arc, repeated at a step.
 *
 *              THE PULSE: .pulse, a ring that grows from its radius to hud_anim_pulse_scale times it while its
 *              opacity falls from hud_anim_pulse_alpha to 0, endlessly over hud_anim_pulse_ms with
 *              hud_anim_pulse_ease; the stroke grows with it (the mockup scales the element). Still, it stands
 *              at its radius at hud_anim_still_pulse_alpha.
 *
 *              Geometry, colour and line width come from the calling group, read from SHA through UiTheme;
 *              this unit holds only the four motion tokens and computes no place.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** How an arc is drawn: its opacity, its round end, and how far it has grown (0..1). */
struct UiArcStyle {
    double opacity = 0.0;  // o.op of the mockup, multiplied into the colour's alpha
    bool round_cap = false;  // o.cap: stroke-linecap round
    double drawn = 0.0;    // the share of the sweep drawn from the start (1 for an arc without o.draw)
};

/** The drawer of the HUD's rings, arcs, stepped arcs and pulses. */
class UiRing {
public:
    /** Binds the draw and pulse motion and the pulse's scale and alphas; false with the missing token in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** A filled disc of radius `radius` in `fill`. */
    void disc(UiPen& canvas, const ::godot::Vector2& centre, double radius,
              const ::godot::Color& fill) const;

    /** The full ring of radius `radius` as a line of `width` in `color` at `opacity`. */
    void circle(UiPen& canvas, const ::godot::Vector2& centre, double radius,
                const ::godot::Color& color, double width, double opacity) const;

    /** The arc from `from_deg` to `to_deg` (degrees from the top, clockwise) in `color`, `width`, `style`. */
    void arc(UiPen& canvas, const ::godot::Vector2& centre, double radius, double from_deg,
             double to_deg, const ::godot::Color& color, double width, const UiArcStyle& style) const;

    /** The share an arc with o.draw has grown `elapsed` seconds after appearing, starting after `delay`. */
    double drawn(double elapsed, double delay, bool still) const;

    /** The point `radius` from `centre` at `degrees` from the top, clockwise: where a ring's mark or word stands. */
    static ::godot::Vector2 point(const ::godot::Vector2& centre, double radius, double degrees);

    /**
     * One arc per colour of `colors`: arc i from i * step_deg + from_deg to i * step_deg + to_deg, all in
     * `width` at `opacity`.
     */
    void steps(UiPen& canvas, const ::godot::Vector2& centre, double radius, double from_deg,
               double to_deg, double step_deg, const std::vector<::godot::Color>& colors, double width,
               double opacity) const;

    /** The pulse of a ring of `radius` in `color` and `width`, `elapsed` seconds after it appeared. */
    void pulse(UiPen& canvas, const ::godot::Vector2& centre, double radius,
               const ::godot::Color& color, double width, double elapsed, bool still) const;

private:
    UiMotion draw_;             // hud_anim_draw: the growth of an arc
    UiMotion pulse_;            // hud_anim_pulse: one turn of the pulse
    double pulse_scale_ = 0.0;  // hud_anim_pulse_scale: the radius factor at the end of a turn
    double pulse_alpha_ = 0.0;  // hud_anim_pulse_alpha: the opacity at the start of a turn
    double still_alpha_ = 0.0;  // hud_anim_still_pulse_alpha: the opacity of a still pulse
};

}  // namespace ase::adp::godot

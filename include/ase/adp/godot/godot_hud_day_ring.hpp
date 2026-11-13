#pragma once

/**
 * ASE GODOT ADAPTER - HUD DAY RING
 *
 * @file        godot_hud_day_ring.hpp
 * @brief       The day ring of the calendar sheet: the light of the day, the six phases, the sun or moon, the date
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block DayRing (the inner ring of the mockup's
 *              skySheet()). It stands on the sheet's centre (hud_year_x, hud_year_y), the centre the year ring
 *              turns about, with radius hud_day_radius; an angle is degrees from the top, clockwise, and the
 *              day runs once round the ring.
 *
 *              THE RING. A circle of hud_day_radius in OVERLAY_INK_FAINT at hud_line_hair.
 *
 *              THE LIGHT. An arc on the ring from the sunrise to the sunset of the day, in DOMAIN_CUT at
 *              hud_day_light_width and hud_day_light_alpha, growing from its start (UiRing's draw) after
 *              hud_day_light_delay.
 *
 *              THE PHASES. One tick per phase at its start, hud_day_tick_half inside and outside the ring, in
 *              OVERLAY_INK_SOFT at hud_line_hair; its word texts.hud.phases.<name> in class t at the middle of
 *              the phase, hud_day_phase_inset inside the ring and hud_day_phase_dy lower, size
 *              hud_day_phase_size, centred, in DOMAIN_TIME for the current phase and OVERLAY_INK_SOFT for every
 *              other. The last phase ends where the first begins, a full turn on.
 *
 *              THE SUN. At the day's place, hud_day_sun_gap outside the ring, ICON_MOON in DOMAIN_SKY while the
 *              sun is under the horizon, ICON_SUN in DOMAIN_CUT while it is up, size hud_day_sun_size.
 *
 *              THE DATE. In the centre texts.hud.day_of with the TUM in class tb, size hud_day_tum_size,
 *              hud_day_tum_dy off the centre, in the ink of the class; texts.hud.date_of with LUN and VEC in
 *              class d, size hud_day_lun_size, hud_day_lun_dy off the centre, in OVERLAY_INK_SOFT;
 *              texts.hud.hour_of with the hour to hud_day_hour_decimals decimals and the word of the current
 *              phase in class d, size hud_day_hour_size, hud_day_hour_dy off the centre, in OVERLAY_INK_FAINT.
 *              All three centred. Without a measured date the ring draws nothing.
 *
 *              THE PLACE OF AN HOUR. The unit never turns an hour into a share of the day: that is the clock
 *              logic of ase-time, and the adapter translates. The day's place, the start of each phase, the
 *              sunrise and the sunset stand where the module puts them, as their share of the day (0 to 1),
 *              and the ring turns the share into an angle.
 *
 *              BRIDGE GAP N0460. The share of the day of the clock (TIM_DAY_PROGRESS) and the shares of the
 *              sunrise and the sunset come from the day record flora.day.v1 (day_share, sunrise_share,
 *              sunset_share), the share each phase begins at from the six records of flora.phase.v1 (period,
 *              start_share). GodotHostResourceManager::read_snapshot reads them into VivariumClockView, the
 *              bridge fills day_share, phase_shares, sunrise_share and sunset_share, and the night is the sky
 *              port's sun_y as on the band. The ring, the light, the phases, the sun and the date are always drawn
 *              in full from that frame: while a port files no record the bridge writes the mockup's values from
 *              hud_bridge_clock_* of SHA and logs the gap once on its edge (godot_vivarium_hud_bridge.cpp, bridge_clock);
 *              the ring never asks whether a port delivered.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0460 (flora.day.v1: shares of the day, sunrise, sunset; flora.phase.v1: phase shares), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the day ring shows this frame. */
struct DayRingFrame {
    uint32_t vec = 0;                 // the year (VEC, CAL_YEAR)
    uint32_t lun = 0;                  // the month (LUN, CAL_MONTH)
    uint32_t tum = 0;                  // the day of the month (TUM, CAL_DAY)
    double hour = 0.0;                 // the clock's hour (HOR, TIM_CURRENT_HOUR)
    int32_t period = 0;                // the current phase, GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST (TIM_DAY_PERIOD)

    double day_share = 0.0;            // the clock's share of the day, as ase-time places it (0 to 1)
    std::vector<double> phase_shares;  // the share of the day each phase begins at, in the order of GODOT_PERIOD_*
    double sunrise_share = 0.0;        // the share of the day the sun rises at
    double sunset_share = 0.0;         // the share of the day the sun sets at
    bool night = false;                // the sun is under the horizon

    double elapsed = 0.0;              // seconds since the sheet opened
    bool still = false;                // every motion held
};

/** The day ring. */
class DayRing {
public:
    /** Binds measures, colours, words and glyphs; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the ring of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const DayRingFrame& now,
              std::string& failed);

private:
    /** The light, the phase ticks and words and the sun from the frame; a missing record is logged as N0460. */
    bool draw_day(UiPen& canvas, const ::godot::Transform2D& frame, const DayRingFrame& now,
                  std::string& failed);

    /** The TUM, the LUN with the VEC and the hour with the phase in the centre. */
    bool draw_date(UiPen& canvas, const ::godot::Transform2D& frame, const DayRingFrame& now,
                   std::string& failed);

    /** The hour to hud_day_hour_decimals decimals. */
    std::string decimal(double value) const;

    double x_ = 0.0;              // hud_year_x
    double y_ = 0.0;              // hud_year_y
    double radius_ = 0.0;         // hud_day_radius
    double light_width_ = 0.0;    // hud_day_light_width
    double light_alpha_ = 0.0;    // hud_day_light_alpha
    double light_delay_ = 0.0;    // hud_day_light_delay
    double tick_half_ = 0.0;      // hud_day_tick_half
    double phase_inset_ = 0.0;    // hud_day_phase_inset
    double phase_dy_ = 0.0;       // hud_day_phase_dy
    double phase_size_ = 0.0;     // hud_day_phase_size
    double sun_gap_ = 0.0;        // hud_day_sun_gap
    double sun_size_ = 0.0;       // hud_day_sun_size
    double tum_dy_ = 0.0;         // hud_day_tum_dy
    double tum_size_ = 0.0;       // hud_day_tum_size
    double lun_dy_ = 0.0;         // hud_day_lun_dy
    double lun_size_ = 0.0;       // hud_day_lun_size
    double hour_dy_ = 0.0;        // hud_day_hour_dy
    double hour_size_ = 0.0;      // hud_day_hour_size
    double hour_decimals_ = 0.0;  // hud_day_hour_decimals
    double line_hair_ = 0.0;      // hud_line_hair

    ::godot::Color soft_, faint_, time_, cut_, sky_;

    std::vector<HudRun> day_of_;          // texts.hud.day_of
    std::vector<HudRun> date_of_;         // texts.hud.date_of
    std::vector<HudRun> hour_of_;         // texts.hud.hour_of
    std::vector<std::vector<HudRun>> phase_words_;  // texts.hud.phases.*, in the order of GODOT_PERIOD_*

    UiRing ring_;           // the circle and the light arc
    UiIcon sun_;            // ICON_SUN
    UiIcon moon_;           // ICON_MOON
    UiTextBlock phase_;     // class t
    UiTextBlock tum_;       // class tb
    UiTextBlock lun_;       // class d
    UiTextBlock hour_;      // class d

};

}  // namespace ase::adp::godot

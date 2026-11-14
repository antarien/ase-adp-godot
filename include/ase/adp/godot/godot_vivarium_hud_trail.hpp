#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM HUD TRAIL BRIDGE
 *
 * @file        godot_vivarium_hud_trail.hpp
 * @brief       The trail of past events the bridge writes into the HUD's frame while flora.trail.v1 files none
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.4 and the bridge section, operator decision 2026-11-10: the HUD
 *              draws every element from its frame and never asks whether a port delivered. The trail on arrival
 *              (TrailView, the mockup's arrive()) and the dots of the year ring (YearRing, the mockup's
 *              skySheet()) stand on the records of flora.trail.v1 (N0450); while the port files none, they
 *              stand on the mockup's S.log, every value a hud_bridge_trail_* token of SHA (theme.css
 *              --ase-hud-bridge-trail-*). VivariumView::fill_hud logs the gap once on its edge.
 *
 *              THE SPAN. hud_bridge_trail_days, _lun, _from, _to: the days away, the LUNBREX and the first and
 *              last TUMBREX of the mockup's arrive().
 *
 *              THE EVENTS. hud_bridge_trail_count events, oldest first; event n carries hud_bridge_trail_<n>_lun,
 *              _tum, _year_share, _kind (the words texts.hud.trail.<kind>), _tone (DOMAIN_<TONE>), _alert (1
 *              where the mockup's domain is alert), _sign (a key of hud.signs, or rest), _case (the mockup's uc)
 *              and _values port values; value k is _value_<k>_name with either _value_<k>_word (a key of
 *              texts.hud, written in the language of the view) or _value_<k>_number written with
 *              _value_<k>_decimals. load() resolves every word, tone and sign once, so a token SHA does not
 *              state stops the view.
 *
 *              THE RING. An event's dot stands at its share of the year as the port files it
 *              (hud_bridge_trail_<n>_year_share, 0..1): ase-calendar places the day in the year, the adapter
 *              never reckons a date.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0450 (flora.trail.v1: the past events with date, domain, words and case), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_trail_view.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_year_ring.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The trail of the mockup's S.log while flora.trail.v1 files no event (N0450). */
class VivariumTrailBridge {
public:
    /** Reads the hud_bridge_trail_* values of SHA and resolves their words, tones and signs; false with the
     *  failing token in `failed`. */
    bool load(const UiTheme& theme, std::string& failed);

    /** Writes the events and the span into `trail`. */
    void trail(TrailViewFrame& trail) const;

    /** Writes the events' dots into `year` at the share of the year the port files for each. */
    void ring(YearRingFrame& year) const;

private:
    /** Reads event `index` (counted from 0) into `out` and its share of the year into `share`; false with the
     *  failing token in `failed`. */
    bool load_event(const UiTheme& theme, uint32_t index, TrailEvent& out, double& share,
                    std::string& failed) const;

    /** Reads value `value` of the event under `prefix` into `out`; false with the failing token in `failed`. */
    bool load_value(const UiTheme& theme, const std::string& prefix, uint32_t value, HudPortValue& out,
                    std::string& failed) const;

    std::vector<TrailEvent> events_;  // hud_bridge_trail_<n>_*: the mockup's S.log, oldest first
    std::vector<double> shares_;      // hud_bridge_trail_<n>_year_share: each event's share of the year
    uint32_t days_ = 0;               // hud_bridge_trail_days: the 9 DIAS of arrive()
    uint32_t lun_ = 0;                // hud_bridge_trail_lun: its LUN 4
    uint32_t from_ = 0;               // hud_bridge_trail_from: its TUM 3
    uint32_t to_ = 0;                 // hud_bridge_trail_to: its TUM 12
};

}  // namespace ase::adp::godot

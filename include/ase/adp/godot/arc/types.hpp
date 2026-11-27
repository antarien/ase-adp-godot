#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the sky arc: the keys of its three ports and the names its units share
 * @description All compile-time constants of the arc sub-type; NO runtime values - those belong in the views.
 *              The documented contract of flora.arc.v1, flora.almanac.v1 and flora.event_code.v1
 *              (PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT, section "Die Brücke", package EXT.5), repeated here because the
 *              adapter reads the ports and never links the plugin: flora.arc.v1 carries one record (object id 0)
 *              every frame (the shares of "now" against the anchor of each ring in half windows, the number of the
 *              set, the next event with its deadline); flora.almanac.v1 one record per occurrence, its object id the
 *              occurrence entity; flora.event_code.v1 the codes the calendar numbers its kinds and rings with, on TWO
 *              records (a host record holds at most HUB_CONST_HOST_VALUES_MAX values) which the reader merges by key,
 *              so the adapter holds no number of its own. Below the keys: the names of the kinds of the register, which
 *              are the keys of the SHA tokens hud_arc_kind_<kind>_*, the three slots of the rings in drawing order (the
 *              adapter's own count of its three discs, never the calendar's code of a ring, which arc_ring_index maps
 *              onto a slot), the register entry of the gap and the words of the debug flank lines.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
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
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstddef>
#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: evnt = event, lun = LUNBREX (month), occ = occurrence, conj = conjunction, SURA = the date format.
 */

// flora.arc.v1, one record, object id 0, read every frame (exempt from hud_refresh_ms).
constexpr const char* GODOT_KEY_ARC_DAY_NOW = "day_now";      // arc record: share of now against the day ring's anchor, in half windows
constexpr const char* GODOT_KEY_ARC_LUN_NOW = "lun_now";      // arc record: share of now against the month ring's anchor
constexpr const char* GODOT_KEY_ARC_YEAR_NOW = "year_now";    // arc record: share of now against the year ring's anchor
constexpr const char* GODOT_KEY_ARC_NEXT_KIND = "next_kind";  // arc record: code of the kind of the next event, the code evnt_none while none
constexpr const char* GODOT_KEY_ARC_NEXT_BODY = "next_body";  // arc record: body of the next event (the moon, the conjunction, the season, ...)
constexpr const char* GODOT_KEY_ARC_NEXT_IN_H = "next_in_h";  // arc record: game hours until the next event, below 0 while none
constexpr const char* GODOT_KEY_ARC_NEXT_IN_D = "next_in_d";  // arc record: whole game days until the next event, below 0 while none

// flora.almanac.v1, one record per occurrence; the number of its set is the key "seq" (GODOT_KEY_SEQ).
constexpr const char* GODOT_KEY_OCC_KIND = "kind";            // occurrence record: code of its kind (flora.event_code.v1 evnt_*)
constexpr const char* GODOT_KEY_OCC_RING = "ring";            // occurrence record: code of its ring (flora.event_code.v1 ring_*)
constexpr const char* GODOT_KEY_OCC_SHARE = "share";          // occurrence record: its place on the ring, share of the window, -1 left edge, 0 anchor, +1 right edge
constexpr const char* GODOT_KEY_OCC_SPAN = "span";            // occurrence record: its length as a share of the window (light, season, period), 0 for a point

// flora.event_code.v1, two records merged by key: the codes of the kinds and of the rings.
constexpr const char* GODOT_KEY_CODE_NONE = "evnt_none";            // event codes: the code of "no event"
constexpr const char* GODOT_KEY_CODE_KIND_COUNT = "evnt_kind_count";  // event codes: how many kinds the register holds
constexpr const char* GODOT_KEY_CODE_HUNT = "evnt_hunt";            // event codes: the kind hunt (VENATRIX)
constexpr const char* GODOT_KEY_CODE_SEASON = "evnt_season";        // event codes: the kind season
constexpr const char* GODOT_KEY_CODE_VECTUM = "evnt_vectum";        // event codes: the kind vectum (the new year)
constexpr const char* GODOT_KEY_CODE_STAGE = "evnt_stage";          // event codes: the kind stage (the next stage of the plant)
constexpr const char* GODOT_KEY_CODE_PERIOD = "evnt_period";        // event codes: the kind period (a day period begins)
constexpr const char* GODOT_KEY_CODE_DAWN = "evnt_dawn";            // event codes: the kind dawn (sunrise)
constexpr const char* GODOT_KEY_CODE_DUSK = "evnt_dusk";            // event codes: the kind dusk (sunset)
constexpr const char* GODOT_KEY_CODE_MOONRISE = "evnt_moonrise";    // event codes: the kind moonrise
constexpr const char* GODOT_KEY_CODE_FULLMOON = "evnt_fullmoon";    // event codes: the kind fullmoon
constexpr const char* GODOT_KEY_CODE_ECLIPSE = "evnt_eclipse";      // event codes: the kind eclipse (solar or lunar, told by the body)
constexpr const char* GODOT_KEY_CODE_CONJ = "evnt_conj";            // event codes: the kind conj (a conjunction)
constexpr const char* GODOT_KEY_CODE_CONJ_GREAT = "evnt_conj_great";  // event codes: the kind conj_great (a great conjunction)
constexpr const char* GODOT_KEY_CODE_ANTARES = "evnt_antares";      // event codes: the kind antares (a phase of Antares)
constexpr const char* GODOT_KEY_CODE_ARCTURUS = "evnt_arcturus";    // event codes: the kind arcturus (a phase of Arcturus)
constexpr const char* GODOT_KEY_CODE_TUMBRAE = "evnt_tumbrae";      // event codes: the kind tumbrae (a TUMBRAE begins, the turn of the day)
constexpr const char* GODOT_KEY_CODE_LUNBREX = "evnt_lunbrex";      // event codes: the kind lunbrex (a LUNBREX begins)
constexpr const char* GODOT_KEY_CODE_RING_DAY = "ring_day";         // event codes: the day ring (TUMBRAE, unit HORAX)
constexpr const char* GODOT_KEY_CODE_RING_LUN = "ring_lun";         // event codes: the month ring (LUNBREX, unit TUMBRAE)
constexpr const char* GODOT_KEY_CODE_RING_YEAR = "ring_year";       // event codes: the year ring (VECTUM, unit TUMBRAE)
constexpr std::size_t GODOT_EVENT_CODE_KEYS = 21u;                  // keys of flora.event_code.v1 over both records: every one must stand

// The kinds of the register by name: the key of every SHA token hud_arc_kind_<kind>_* and of the words arc.kind.<kind>.
constexpr const char* GODOT_ARC_KIND_PERIOD = "period";             // kind name: a day period begins
constexpr const char* GODOT_ARC_KIND_DAWN = "dawn";                 // kind name: sunrise
constexpr const char* GODOT_ARC_KIND_DUSK = "dusk";                 // kind name: sunset
constexpr const char* GODOT_ARC_KIND_MOONRISE = "moonrise";         // kind name: moonrise
constexpr const char* GODOT_ARC_KIND_HUNT = "hunt";                 // kind name: the hunt VENATRIX
constexpr const char* GODOT_ARC_KIND_FULLMOON = "fullmoon";         // kind name: a full moon
constexpr const char* GODOT_ARC_KIND_CONJ = "conj";                 // kind name: a conjunction
constexpr const char* GODOT_ARC_KIND_ECLIPSE = "eclipse";           // kind name: an eclipse of the sun
constexpr const char* GODOT_ARC_KIND_ECLIPSE_LUNAR = "eclipse_lunar";  // kind name: an eclipse of the moon, the body of an eclipse decides (hud_arc_eclipse_lunar_body)
constexpr const char* GODOT_ARC_KIND_STAGE = "stage";               // kind name: the next stage of the plant
constexpr const char* GODOT_ARC_KIND_SEASON = "season";             // kind name: a season begins
constexpr const char* GODOT_ARC_KIND_VECTUM = "vectum";             // kind name: a new VECTUM
constexpr const char* GODOT_ARC_KIND_ARCTURUS = "arcturus";         // kind name: a phase of Arcturus
constexpr const char* GODOT_ARC_KIND_ANTARES = "antares";           // kind name: a phase of Antares
constexpr const char* GODOT_ARC_KIND_CONJ_GREAT = "conj_great";     // kind name: a great conjunction
constexpr const char* GODOT_ARC_KIND_TUMBRAE = "tumbrae";           // kind name: a TUMBRAE begins, "TUM n" at the turn of the day
constexpr const char* GODOT_ARC_KIND_LUNBREX = "lunbrex";           // kind name: a LUNBREX begins, "LUN n" at its first TUMBRAE
constexpr const char* GODOT_ARC_EVENT_NEXT = "next";                 // event word of a jump (debug.jumps[].event): the next event of any kind, next_kind of flora.arc.v1

// The three slots of the discs in drawing order: the adapter's own count, never the calendar's code of a ring.
constexpr uint32_t GODOT_ARC_SLOT_DAY = 0u;                         // slot of the disc of the day ring TUMBRAE
constexpr uint32_t GODOT_ARC_SLOT_LUN = 1u;                         // slot of the disc of the month ring LUNBREX
constexpr uint32_t GODOT_ARC_SLOT_YEAR = 2u;                        // slot of the disc of the year ring VECTUM
constexpr uint32_t GODOT_ARC_RINGS = 3u;                            // rings of the arc

constexpr int64_t GODOT_ARC_POINTS_MIN = 2;                         // fewest segments of an arc: the shortest span is still a line
constexpr uint32_t GODOT_ARC_FADE_STOPS = 6u;                      // stops of the fade across the frame's width (hud_arc_fade_0 .. hud_arc_fade_5)

// The token prefixes of the three rings (hud_arc_<ring>_r, _half_deg, _step_share).
constexpr const char* GODOT_ARC_RING_WORD_DAY = "hud_arc_day_";     // token prefix of the day ring
constexpr const char* GODOT_ARC_RING_WORD_LUN = "hud_arc_lun_";     // token prefix of the month ring
constexpr const char* GODOT_ARC_RING_WORD_YEAR = "hud_arc_year_";   // token prefix of the year ring

// The gap of the ports (register carrier N0448): logged once on its edge, the arc stands whole from SHA meanwhile.
constexpr const char* GODOT_HUD_GAP_ARC = "N0448";                  // flora.arc.v1, flora.almanac.v1, flora.event_code.v1: the sky arc
constexpr const char* GODOT_ARC_GAP_ARC = "flora.arc.v1";           // gap field: the port of the shares of now
constexpr const char* GODOT_ARC_GAP_ALMANAC = "flora.almanac.v1";   // gap field: the port of the occurrences
constexpr const char* GODOT_ARC_GAP_CODES = "flora.event_code.v1";  // gap field: the port of the codes
constexpr const char* GODOT_ARC_GAP_UNKNOWN = "flora.almanac.v1.kind";  // gap field: an occurrence whose kind or ring no code names

// The debug flank lines of the arc (PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT, "Sonde und Geräte").
constexpr const char* GODOT_ARC_LINE_NOW = "debug arc now";         // flank line: the three shares of now and the frame time, every frame
constexpr const char* GODOT_ARC_LINE_SIGN = "debug arc sign";       // flank line: a drawn sign in logical pixels, on a new set
constexpr const char* GODOT_ARC_LINE_NEXT = "debug arc next";       // flank line: the reading under the needle, on every change
constexpr const char* GODOT_ARC_LINE_DATE = "debug arc date";       // flank line: the SURA date under the needle (period, year, epoch, cycle, month, day, hour), on every change (E08, E10)
constexpr const char* GODOT_ARC_LINE_SURFACE = "debug surface";     // flank line: safe area and camera cut-out in logical pixels, at start and on every change, device only (E02)
constexpr const char* GODOT_ARC_PLACE_ARC = "arc";                  // debug place part: the hit area of the arc
constexpr const char* GODOT_ARC_PLACE_NEEDLE = "arc_needle";        // debug place part: the needle
constexpr int32_t GODOT_ARC_NOW_DECIMALS = 7;                       // decimals of a share of now in the flank line: two frames never read alike
constexpr int32_t GODOT_ARC_PIXEL_DECIMALS = 1;                     // decimals of a place in logical pixels in the flank line
constexpr const char* GODOT_ARC_SURA_GAP = " ";                     // between the period name and the SURA number of the readout
constexpr const char* GODOT_ARC_SURA_PART_GAP = ".";                // between two parts of the SURA number of the readout

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the constants of the Godot adapter
 * @description Every value the adapter reads that does NOT change at runtime and that is NOT
 *              configuration: the value keys and codes of the host ports of ase-pl-flora, the
 *              table of contents of the staged bundle, the form of the self-test table, the names
 *              of the boot steps a failure is reported under, the GDExtension class binding of
 *              AseVivariumView (class names, the engine callbacks it takes over and their
 *              signature hashes), and its drawing style (colours, font sizes, line widths).
 *
 *              WHAT IS NOT HERE: positions, rectangles, texts, the pour amount and the port and
 *              operation names. They are the client's configuration and come from
 *              res://config/vivarium_start.json (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.4).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-05
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * WHY THE PORT KEYS ARE WRITTEN A SECOND TIME
 *
 * ase-pl-flora carries the same names (FLORA_HOST_KEY_* in its types.hpp). The adapter sees only
 * the kernel facade - no component, no hub, no unit header (ARCH_ASE_KERNEL_EMBEDDED.md, the section
 * on layers and boundaries) - so it holds the DOCUMENTED contract of the ports
 * (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 point 1), exactly as
 * clients/avt-client-android/tests/avt_headless.cpp does. A renamed key breaks the host test of
 * this adapter, which runs against the real bundle.
 *
 * ABBREVIATIONS USED IN THIS ADAPTER:
 *   adp   adapter   third-party isolation module
 *   avt   Antares Vivarium, the client this adapter draws for
 *   tmpl  template  an organ template of ase-render-data
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

#include <cstdint>

namespace ase::adp::godot {

/**
 * Host ports of ase-pl-flora - value keys (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 point 1). The
 * patch port carries one record per patch, its object id is the patch; the clock port one record.
 */
constexpr const char* GODOT_KEY_PLACE = "place";              // patch record: where the view draws the patch
constexpr const char* GODOT_KEY_COVERAGE = "coverage";        // patch record: covered share of the patch, 0..1
constexpr const char* GODOT_KEY_STAGE = "stage";              // patch record: growth stage code (PLT_STAGE)
constexpr const char* GODOT_KEY_CONDITION = "condition";      // patch record: condition code (PLT_CONDITION)
// The condition codes of ase-plant, one record on flora.cond.v1 (usecase:101340 - show the bed condition, D2454):
// read_snapshot compares every patch condition against them and keeps no number of ase-plant itself.
constexpr const char* GODOT_KEY_COND_HEALTHY = "cond_healthy";  // condition record: the healthy code (PLT_CONST_COND_HEALTHY)
constexpr const char* GODOT_KEY_COND_WILTED = "cond_wilted";    // condition record: the wilted code (PLT_CONST_COND_WLTG)
constexpr const char* GODOT_KEY_COND_DEAD = "cond_dead";        // condition record: the dead code (PLT_CONST_COND_DEAD)
// The stage codes of ase-plant, one record on flora.stage.v1 (usecase:101635 - show the plant stage, D2455):
// read_snapshot turns every patch stage into its position among them and keeps no number of ase-plant itself.
constexpr const char* GODOT_KEY_STGE_SEED = "stage_seed";       // stage record: the seed code (PLT_CONST_STAGE_SEED)
constexpr const char* GODOT_KEY_STGE_SPRT = "stage_sprout";     // stage record: the sprout code (PLT_CONST_STAGE_SPRT)
constexpr const char* GODOT_KEY_STGE_GROW = "stage_grow";       // stage record: the grow code (PLT_CONST_STAGE_GROW)
constexpr const char* GODOT_KEY_STGE_MTUR = "stage_mature";     // stage record: the mature code (PLT_CONST_STAGE_MTUR)
constexpr const char* GODOT_KEY_SOIL_MM = "soil_mm";          // patch record: soil water, mm (HYD_SOIL_WATER)
constexpr const char* GODOT_KEY_SOIL_REL = "soil_rel";        // patch record: soil water over field capacity, 0..1
constexpr const char* GODOT_KEY_CAPACITY_MM = "capacity_mm";  // patch record and patch lay request: field capacity of the soil, mm
constexpr const char* GODOT_KEY_GRAVITY_REL = "gravity_rel";  // patch lay request: gravity of the patch relative to g; ase-pl-flora judges the bounds
constexpr const char* GODOT_KEY_GROW_AT_H = "grow_at_h";      // patch record: game hour ase-plant forecasts CRESCENDO at (PLT_GROW_AT_H)
constexpr const char* GODOT_KEY_MTUR_AT_H = "mtur_at_h";      // patch record: game hour ase-plant forecasts MADURA at (PLT_MTUR_AT_H)
constexpr const char* GODOT_KEY_BASE_Y = "base_y";            // stem record: the stem foot ase-physics holds, metres above the level of its patch
constexpr const char* GODOT_KEY_AMOUNT_MM = "amount_mm";      // irrigate request: water poured on the patch, mm
constexpr const char* GODOT_KEY_FACTOR = "factor";            // pace request: the multiple of ase-time's base pace the host's clock runs at, 0 = its default
constexpr const char* GODOT_KEY_GAME_HOUR = "game_hour";      // seek request: the absolute game hour (day x hours per day + hour) the host's clock moves to
constexpr const char* GODOT_KEY_AHEAD_H = "ahead_h";          // seek request: game hours ahead of the host's clock, ase-time lays the target from its own clock
constexpr const char* GODOT_KEY_DAY_HOUR = "day_hour";        // seek request: hour of the game day, ase-time lays its next occurrence from its own clock
constexpr const char* GODOT_KEY_SEQ = "seq";                  // seek and date request: number of the request, given by the host, only grows (TIM_CONST_SEEK_SEQ_MIN on)
constexpr const char* GODOT_KEY_SEEK_SEQ = "seek_seq";        // clock record: number of the request ase-time last judged (TIM_SEEK_SEQ), absent while no request was judged
constexpr const char* GODOT_KEY_SEEK_ORDER = "seek_order";    // clock record: ase-time's verdict on that request (TIM_SEEK_ORDER, GODOT_SEEK_ORDER_*)
constexpr const char* GODOT_KEY_SEEK_TARGET_DAY = "seek_target_day";  // clock record: whole game day of the target ase-time judged the request against, -1 = none or refused (TIM_SEEK_TARGET_DAY)
constexpr const char* GODOT_KEY_SEEK_TARGET_H = "seek_target_h";  // clock record: hour of that day, below TIM_CONST_HOURS_PER_DAY, -1 = none or refused (TIM_SEEK_TARGET_H); the absolute target is day · hours_per_day + hour
constexpr const char* GODOT_KEY_SEEK_SEQ_MIN = "seek_seq_min";  // seek bounds record: lowest request number, the first the host gives a jump (TIM_CONST_SEEK_SEQ_MIN)
constexpr const char* GODOT_KEY_SEEK_SEQ_MAX = "seek_seq_max";  // seek bounds record: largest request number a float slot holds exactly (TIM_CONST_SEEK_SEQ_MAX)
constexpr const char* GODOT_KEY_SEEK_ORDER_NONE = "seek_order_none";  // seek bounds record: verdict code of no request judged yet (TIM_CONST_SEEK_ORDER_NONE)
constexpr const char* GODOT_KEY_SEEK_ORDER_REFUSED = "seek_order_refused";  // seek bounds record: verdict code of a refused request, the largest code (TIM_CONST_SEEK_ORDER_REFUSED)
constexpr double GODOT_SEEK_FORM_NONE = -1.0;                 // a seek form the request does not use (FLORA_SEEK_FORM_NONE)
constexpr int GODOT_SEEK_ORDER_AHEAD = 1;                     // verdict: the clock was set to the target in one write (TIME_SEEK_ORDER_AHEAD)
constexpr int GODOT_SEEK_ORDER_AT = 2;                        // verdict: the clock already stood at the target (TIME_SEEK_ORDER_AT)
constexpr int GODOT_SEEK_ORDER_BEHIND = 3;                    // verdict: the target lies behind the clock, which only runs forward (TIME_SEEK_ORDER_BEHIND)
constexpr const char* GODOT_KEY_REAL_S = "real_s";            // real request: the absolute real seconds since the host started, filed every frame (TIM_REQ_TIME_REAL_S)
constexpr const char* GODOT_KEY_REAL_LAID = "real_laid_s";    // clock record: real position ase-time last set its clock forward to in one write, 0 = never (TIM_REAL_LAID_S)
constexpr const char* GODOT_KEY_ELAPSED = "elapsed_s";        // clock record: game time, seconds (TIM_ELAPSED_GAME)
constexpr const char* GODOT_KEY_DAY = "day";                  // clock record: game day (TIM_DAY_COUNT)
constexpr const char* GODOT_KEY_HOUR = "hour";                // clock record: hour of the game day (TIM_CURRENT_HOUR)
constexpr const char* GODOT_KEY_PERIOD = "period";            // clock record: code of the day period (TIM_DAY_PERIOD, TIM_CONST_DAY_PERIOD_ASO..NOX)
constexpr const char* GODOT_KEY_SHOOT = "shoot_s";            // clock record: game seconds up to which ase-phytomer's growth clock ran its cycles (PHY_CLK_LAST_S), absent until that clock is ready
constexpr const char* GODOT_KEY_HOURS_PER_DAY = "hours_per_day";       // clock record: hours of a game day, ase-time's TIM_CONST_HOURS_PER_DAY
constexpr const char* GODOT_KEY_SECONDS_PER_HOUR = "seconds_per_hour"; // clock record: game seconds of a game hour, ase-time's TIM_CONST_SECONDS_PER_HOUR
constexpr const char* GODOT_KEY_YEAR = "year";                // date record: SURA VECTUM, the year of the age (CAL_YEAR)
constexpr const char* GODOT_KEY_EPOCH = "epoch";              // date record: SURA AETRIX, the epoch (CAL_EPOCH)
constexpr const char* GODOT_KEY_CYCLE = "cycle";              // date record: SURA CYCLUM, the cycle (CAL_CYCLE)
constexpr const char* GODOT_KEY_MONTH = "month";              // date record: SURA LUNBREX, the month, from 1 (CAL_MONTH)
constexpr const char* GODOT_KEY_MONTH_DAY = "month_day";      // date record: SURA TUMBRAE, the day of the month, from 1 (CAL_DAY)
constexpr const char* GODOT_KEY_DAYS_PER_MONTH = "days_per_month";    // date record: TUMBRAE of a LUNBREX, ase-calendar's CAL_CONST_DAYS_PER_MONTH
constexpr const char* GODOT_KEY_MONTHS_PER_YEAR = "months_per_year";  // date record: LUNBREX of a VECTUM, ase-calendar's CAL_CONST_MONTHS_PER_YEAR
constexpr const char* GODOT_KEY_SEASON_GLACIX = "season_glacix";        // date record: code of GLACIX, winter (CAL_CONST_SEASON_GLACIX)
constexpr const char* GODOT_KEY_SEASON_SALTATRIX = "season_saltatrix";  // date record: code of SALTATRIX, spring (CAL_CONST_SEASON_SALTATRIX)
constexpr const char* GODOT_KEY_SEASON_IGNIX = "season_ignix";          // date record: code of IGNIX, summer (CAL_CONST_SEASON_IGNIX)
constexpr const char* GODOT_KEY_SEASON_HALITRIX = "season_halitrix";    // date record: code of HALITRIX, autumn (CAL_CONST_SEASON_HALITRIX)
constexpr const char* GODOT_KEY_SUNRISE_H = "sunrise_h";      // day record (N0460): the hour the sun rises this day (ase-calendar, ase-ephemeris)
constexpr const char* GODOT_KEY_SUNSET_H = "sunset_h";        // day record (N0460): the hour the sun sets this day (ase-calendar, ase-ephemeris)
constexpr const char* GODOT_KEY_DAYLIGHT_H = "daylight_h";    // day record (N0460): the hours of daylight this day (CAL_DAYLIGHT_DURATION)
constexpr const char* GODOT_KEY_SEASON = "season";            // day and season record (N0460): the season, CAL_CONST_SEASON_GLACIX .. HALITRIX (CAL_SEASON)
constexpr const char* GODOT_KEY_DAY_SHARE = "day_share";      // day record (N0460): the clock's share of the day, 0 to 1 (TIM_DAY_PROGRESS)
constexpr const char* GODOT_KEY_SUNRISE_SHARE = "sunrise_share";  // day record (N0460): the share of the day the sun rises at, 0 to 1
constexpr const char* GODOT_KEY_SUNSET_SHARE = "sunset_share";    // day record (N0460): the share of the day the sun sets at, 0 to 1
constexpr const char* GODOT_KEY_YEAR_SHARE = "year_share";    // day record (N0460): today's share of the year, 0 to 1 (ase-calendar)
constexpr const char* GODOT_KEY_LUN_SHARE = "lun_share";      // day record (B0280): the share of the year a whole LUNBREX fills, ase-calendar's CAL_LUN_SHARE; the last LUNBREX is shorter, so LUN m starts at (m - 1) times it
constexpr const char* GODOT_KEY_START_H = "start_h";          // phase record (N0460): the hour the phase begins, 0 for ASO, else TIM_CONST_PERIOD_BND_*
constexpr const char* GODOT_KEY_START_SHARE = "start_share";  // phase record (N0460): the share of the day the phase begins at, 0 to 1
constexpr const char* GODOT_KEY_START_LUN = "start_lun";      // season record (N0460): the first LUN of the season (CAL_CONST_SEASON_*_START)
constexpr const char* GODOT_KEY_LIGHT_H = "light_h";          // season record (N0460): the hours of light of the season (CAL_CONST_SSN_DAYLIGHT_*)
constexpr uint32_t GODOT_YEAR_SEASONS = 4u;                  // records the season port carries, one per season CAL_CONST_SEASON_GLACIX .. HALITRIX

/**
 * The SURA date line in the header (ARCH_ASE_CALENDAR 2.9.1): "<PERIOD> VECTUM.AETRIX.CYCLUM.LUNBREX.
 * TUMBRAE.HORAX". The period names are the short names the engine header prints for TIM_DAY_PERIOD
 * (clients/ase-client-web/src/shared/hooks/useHeaderData.ts, DAY_PERIODS); the codes are those of
 * ase-time (hub constants TIM_CONST_DAY_PERIOD_ASO = 0 to TIM_CONST_DAY_PERIOD_NOX = 5).
 */
constexpr const char* GODOT_PERIOD_ASO = "ASO";               // day period code 0, night and hunt
constexpr const char* GODOT_PERIOD_VEL = "VEL";               // day period code 1, dawn transition
constexpr const char* GODOT_PERIOD_RAL = "RAL";               // day period code 2, day and activity
constexpr const char* GODOT_PERIOD_CRE = "CRE";               // day period code 3, dusk transition
constexpr const char* GODOT_PERIOD_PEX = "PEX";               // day period code 4, evening and rest
constexpr const char* GODOT_PERIOD_NOX = "NOX";               // day period code 5, midnight transition
constexpr int GODOT_PERIOD_FIRST = 0;                         // code of ASO, the first period (TIM_CONST_DAY_PERIOD_ASO)
constexpr int GODOT_PERIOD_LAST = 5;                          // code of NOX, the last period (TIM_CONST_DAY_PERIOD_NOX)
constexpr uint32_t GODOT_DAY_PHASES = static_cast<uint32_t>(GODOT_PERIOD_LAST - GODOT_PERIOD_FIRST + 1);  // records the phase port carries, one per period
constexpr const char* GODOT_SURA_PERIOD_GAP = " ";            // between the period name and the SURA number
constexpr const char* GODOT_SURA_PART_GAP = ".";              // between two parts of the SURA number
constexpr const char* GODOT_KEY_SOIL = "soil";                // patch record and lay request: ground code (GODOT_SOIL_*)
constexpr const char* GODOT_KEY_SPECIES = "species";          // lay request: catalog id of the species (KernelEmbeddedHost::name_id)
constexpr const char* GODOT_KEY_SEED = "seed";                // lay request: seed of the plant
constexpr const char* GODOT_KEY_BODY = "body";                // organ record: object id of the patch the plant stands on
constexpr const char* GODOT_KEY_ORDER = "order";              // organ record: branching order of its axis (stem 0)
constexpr const char* GODOT_KEY_X = "x";                      // organ record: metres east of the stem base; patch record: the stem base, world metres east; corner record: place on the unit
constexpr const char* GODOT_KEY_Y = "y";                      // organ record: metres above the ground at the stem base; patch record: the stem base, world metres up (TRN_FLAT_HGT); corner record: place along the unit
constexpr const char* GODOT_KEY_Z = "z";                      // organ record: metres north of the stem base; patch record: the stem base, world metres north; corner record: place on the unit
constexpr const char* GODOT_KEY_AIM_X = "aim_x";              // view request: the point the camera looks at, world metres east (FLORA_HOST_KEY_AIM_X)
constexpr const char* GODOT_KEY_AIM_Y = "aim_y";              // view request: the point the camera looks at, world metres up (FLORA_HOST_KEY_AIM_Y)
constexpr const char* GODOT_KEY_AIM_Z = "aim_z";              // view request: the point the camera looks at, world metres north (FLORA_HOST_KEY_AIM_Z)
// An organ record carries the organ's frame in place of a direction: the unit quaternion ase-render-data
// builds from the direction and roll of ase-phytomer or ase-radix (hub_plnt_part_quat_comp.hpp). It turns
// the plant frame (y up) into the organ's: local y along the organ, local x along its roll axis.
constexpr const char* GODOT_KEY_QX = "qx";                    // organ record: frame, vector part, east share
constexpr const char* GODOT_KEY_QY = "qy";                    // organ record: frame, vector part, upward share
constexpr const char* GODOT_KEY_QZ = "qz";                    // organ record: frame, vector part, north share
constexpr const char* GODOT_KEY_QW = "qw";                    // organ record: frame, scalar part
constexpr const char* GODOT_KEY_LENGTH = "length";            // organ record: length, metres (leaf: edge of the equal square); comet record: angular length of the tail, radians
constexpr const char* GODOT_KEY_RADIUS = "radius";            // organ record: radius, metres (internode and root segment, else 0)
constexpr const char* GODOT_KEY_BARK = "bark";                // segment record only: share of the stem-base bark at the middle of the internode, 0 white to 1 base bark (usecase:100395)
constexpr double GODOT_BARK_WHITE = 0.0;                      // share of the stem-base bark of the white bark, the lower end of its range
constexpr double GODOT_BARK_BASE = 1.0;                       // share of the stem-base bark of the base bark whole, the upper end of its range
// The bark tiling of an internode (03.11 step 2, the material data of a section; usecase:100395), one record per
// internode with lenticels on the tile port, its object id the internode; ase-render-data computed all three.
constexpr const char* GODOT_KEY_TILE_AROUND = "around";       // tile record: tiles of the white bark around the tube
constexpr const char* GODOT_KEY_TILE_ALONG = "along";         // tile record: tiles of the white bark along the tube
constexpr const char* GODOT_KEY_TILE_BASE = "base";           // tile record: tiles of the stem-base bark per tile of the white bark
constexpr double GODOT_TILE_NONE = 1.0;                       // tiling of an internode without a tile record: the scale of the material itself
// The sway of an organ on a branch (usecase:101335 animate the branch sway, DSGN_024 sway group 21/PMG/TP/WS), one
// record per organ on the sway port, its object id the organ; ase-phytomer computed all seven, the view none.
constexpr const char* GODOT_KEY_SWAY_FREQ = "freq";           // sway record: natural frequency of the branch, Hz
constexpr const char* GODOT_KEY_SWAY_MEAN_X = "mean_x";       // sway record: east mean displacement of the tip of the branch, metres
constexpr const char* GODOT_KEY_SWAY_MEAN_Z = "mean_z";       // sway record: north mean displacement of the tip of the branch, metres
constexpr const char* GODOT_KEY_SWAY_GUST_X = "gust_x";       // sway record: east gust amplitude at the tip of the branch, metres
constexpr const char* GODOT_KEY_SWAY_GUST_Z = "gust_z";       // sway record: north gust amplitude at the tip of the branch, metres
constexpr const char* GODOT_KEY_SWAY_SHAPE_BASE = "shape_base";  // sway record: share of the tip displacement at the base of the organ
constexpr const char* GODOT_KEY_SWAY_SHAPE_TOP = "shape_top";    // sway record: share of the tip displacement at the top of the organ
constexpr double GODOT_SWAY_NONE = 0.0;                       // every sway value of an organ without a sway record: it stands still
constexpr uint32_t GODOT_SWAY_TEXELS = 2u;                    // texels of one instance in the sway data texture
constexpr uint32_t GODOT_SWAY_TEXEL_VALUES = 4u;              // values of one texel of the sway data texture (RGBA)
constexpr uint32_t GODOT_SWAY_VALUES = GODOT_SWAY_TEXELS * GODOT_SWAY_TEXEL_VALUES;  // values of one instance: the seven of its record and one spare
constexpr uint32_t GODOT_SWAY_ROW_INSTANCES = 1024u;          // instances in one row of the sway data texture: the row count stays far below every texture limit
// The bark tube of every axis (register B0295 point 4, usecase:101613 show the plant bark tubes): one record per
// vertex on the bark port, its object id the vertex row; ase-render-data built the tube with frames carried by
// parallel transport, ase-pl-flora lays it out page by page (FLORA_HOST_PORT_BARK, FLORA_HOST_KEY_NEXT/BELOW/ORGAN,
// repeated here because the adapter reads the port and never links the plugin). A record carries body, x, y, z
// (plant-local), nx, ny, nz, u (0..1 around the ring) and v (metres along the axis from its foot ring), and the
// three links below; the view closes the quad strip between two rings by object id and builds no geometry.
constexpr const char* GODOT_KEY_NEXT = "next";                // bark record: object id of the next vertex on the same knot ring, InvalidEntityId after the closing one
constexpr const char* GODOT_KEY_BELOW = "below";              // bark record: object id of the vertex at the same place on the ring below, InvalidEntityId on a foot ring
constexpr const char* GODOT_KEY_ORGAN = "organ";              // bark record: object id of the internode whose knot ring the vertex belongs to (a segment record)
constexpr uint32_t GODOT_BARK_VERTS_PER_PASS = 4096u;         // bark vertices ase-pl-flora files per pass (FLORA_HOST_BARK_VERTS_PER_PASS): a new set takes one pass per page before the host swaps it in
constexpr const char* GODOT_BARK_SCHEDULE = "Production";     // schedule in which ase-pl-flora files the bark pages, one pass per tick
constexpr uint32_t GODOT_BARK_CUSTOM_VALUES = 4u;            // values of one custom array of a bark vertex (RGBA float): CUSTOM0 = bark share and the three tilings, CUSTOM1 = sway slot, sway share, ring radius and order
constexpr uint32_t GODOT_LINK_NONE = UINT32_MAX;              // bark record: a link that names no vertex (InvalidEntityId of the engine)

/**
 * The organ ports of ase-pl-flora (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.3): one port per organ class,
 * one record per organ, its object id the organ. The class is the index of every per-class store of
 * the adapter; its word names the port in the start configuration ("ports") and the record in the
 * self-test table - the same word in both places.
 */
constexpr uint32_t GODOT_ORGAN_SEGMENT = 0u;                  // internodes of every shoot
constexpr uint32_t GODOT_ORGAN_LEAF = 1u;                     // leaves
constexpr uint32_t GODOT_ORGAN_MALE = 2u;                     // male catkins
constexpr uint32_t GODOT_ORGAN_FEMALE = 3u;                   // female catkins
constexpr uint32_t GODOT_ORGAN_FRUIT = 4u;                    // fruit stands
constexpr uint32_t GODOT_ORGAN_ROOT = 5u;                     // root segments of every root
constexpr uint32_t GODOT_ORGAN_COUNT = 6u;                    // organ classes
constexpr const char* GODOT_ORGAN_WORD_SEGMENT = "segment";   // word of GODOT_ORGAN_SEGMENT
constexpr const char* GODOT_ORGAN_WORD_LEAF = "leaf";         // word of GODOT_ORGAN_LEAF
constexpr const char* GODOT_ORGAN_WORD_MALE = "male";         // word of GODOT_ORGAN_MALE
constexpr const char* GODOT_ORGAN_WORD_FEMALE = "female";     // word of GODOT_ORGAN_FEMALE
constexpr const char* GODOT_ORGAN_WORD_FRUIT = "fruit";       // word of GODOT_ORGAN_FRUIT
constexpr const char* GODOT_ORGAN_WORD_ROOT = "root";         // word of GODOT_ORGAN_ROOT
constexpr const char* GODOT_ORGAN_SCHEDULE = "Dissemination"; // schedule in which ase-phytomer and ase-radix change the organ rows
constexpr uint32_t GODOT_ORGAN_READS_PER_RUN = 2u;            // organ reads after a new run: that frame, and the next, in which the plugin's sync system (Reception) and export (Production) carry the run's rows to the ports

/**
 * The instance ports of ase-pl-flora (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2, build form of the
 * instance matrix): one port per DRAWN organ class - every class before the root, which lies in the
 * ground and is not drawn (03.1 point 4) - one record per organ, its object id the organ, the twelve
 * values of its plant-local 3x4 in the row order of MultiMesh::set_buffer. ase-render-data computes them;
 * the view copies them. The start configuration names the port under "<word>_instance".
 */
constexpr uint32_t GODOT_DRAWN_COUNT = 5u;                    // organ classes the view draws: GODOT_ORGAN_SEGMENT up to GODOT_ORGAN_FRUIT
constexpr const char* GODOT_INSTANCE_SUFFIX = "_instance";    // start configuration: ports.<word>_instance names the instance port of a class
constexpr uint32_t GODOT_INSTANCE_VALUES = 12u;               // values of one instance record: three rows of basis and origin
constexpr uint32_t GODOT_INSTANCE_CUSTOM_VALUES = 4u;         // custom data of one instance on the MultiMesh buffer after its twelve values: r = share of the stem-base bark, g b = tiles of the white bark around and along, a = tiles of the base bark per white tile
constexpr const char* GODOT_KEY_M00 = "m00";                  // instance record, row 0 (east): the template's x axis
constexpr const char* GODOT_KEY_M01 = "m01";                  // instance record, row 0 (east): the template's y axis
constexpr const char* GODOT_KEY_M02 = "m02";                  // instance record, row 0 (east): the template's z axis
constexpr const char* GODOT_KEY_M03 = "m03";                  // instance record, row 0 (east): the origin, metres
constexpr const char* GODOT_KEY_M10 = "m10";                  // instance record, row 1 (up): the template's x axis
constexpr const char* GODOT_KEY_M11 = "m11";                  // instance record, row 1 (up): the template's y axis
constexpr const char* GODOT_KEY_M12 = "m12";                  // instance record, row 1 (up): the template's z axis
constexpr const char* GODOT_KEY_M13 = "m13";                  // instance record, row 1 (up): the origin, metres
constexpr const char* GODOT_KEY_M20 = "m20";                  // instance record, row 2 (north): the template's x axis
constexpr const char* GODOT_KEY_M21 = "m21";                  // instance record, row 2 (north): the template's y axis
constexpr const char* GODOT_KEY_M22 = "m22";                  // instance record, row 2 (north): the template's z axis
constexpr const char* GODOT_KEY_M23 = "m23";                  // instance record, row 2 (north): the origin, metres

/**
 * The ground of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8): where a patch lies, the measured
 * square of terrain and the brook are data of the start configuration; ase-pl-flora takes them on its
 * scene, ground and brook ports and shows the measured heights on its terrain port, one record per
 * sample, its object id the sample. World positions are metres, x east, z north - the same keys as
 * the organ records, here in the world instead of plant-local. The adapter computes none of it.
 */
constexpr const char* GODOT_KEY_HALF = "half_m";              // lay requests: half edge of a patch or ground square, half width of a bed; patch record: half edge
constexpr const char* GODOT_KEY_STEP = "step_m";              // ground lay request: distance of two terrain samples, metres
constexpr const char* GODOT_KEY_HEIGHT_SCALE = "height_scale_m";  // shape request: height amplitude of the relief, metres (TER_GEN_HGT_SCL)
constexpr const char* GODOT_KEY_BASE_HEIGHT = "base_height_m";    // shape request: ground level of the relief above NN0, metres (TER_GEN_BAS_HGT)
constexpr const char* GODOT_KEY_END_X = "end_x";              // brook lay request: east end of the bed, world metres
constexpr const char* GODOT_KEY_END_Z = "end_z";              // brook lay request: north end of the bed, world metres
constexpr const char* GODOT_KEY_DEPTH = "depth_m";            // brook lay request: depth of the bed below the ground; spade stroke: depth dug or height heaped, metres
constexpr const char* GODOT_KEY_BASE = "base_m3_s";           // brook lay request: base flow of the brook without rain, m3/s (ase-hydro)
constexpr const char* GODOT_KEY_CATCHMENT = "catchment_m2";   // brook lay request: area whose rain runs into the brook, m2 (ase-hydro)
constexpr const char* GODOT_KEY_WATER_DEPTH = "water_depth_m";  // stream record: water depth in the bed, metres (HYD_STREAM_DEPTH)
constexpr const char* GODOT_KEY_VELOCITY = "velocity_m_s";      // stream record: mean flow velocity, metres per second (HYD_STREAM_VELOCITY)
constexpr const char* GODOT_KEY_DISCHARGE = "discharge_m3_s";   // stream record: discharge, cubic metres per second (HYD_STREAM_DISCHARGE)
constexpr const char* GODOT_KEY_HEIGHT = "height";            // terrain record: ground height at the sample, metres
constexpr const char* GODOT_KEY_NX = "nx";                    // terrain record: unit normal of the ground, east share (ase-render-data); corner record: its normal, x share
constexpr const char* GODOT_KEY_NY = "ny";                    // terrain record: unit normal of the ground, upward share; corner record: its normal, y share
constexpr const char* GODOT_KEY_NZ = "nz";                    // terrain record: unit normal of the ground, north share; corner record: its normal, z share
constexpr const char* GODOT_KEY_U = "u";                      // terrain record: texture coordinate, world metres east; corner record: around or across, 0..1
constexpr const char* GODOT_KEY_V = "v";                      // terrain record: texture coordinate, world metres north; corner record: along, 0..1
constexpr const char* GODOT_KEY_SW = "sw";                    // ground cell record: object id of the south-west corner sample
constexpr const char* GODOT_KEY_SE = "se";                    // ground cell record: object id of the south-east corner sample
constexpr const char* GODOT_KEY_NE = "ne";                    // ground cell record: object id of the north-east corner sample
constexpr const char* GODOT_KEY_NW = "nw";                    // ground cell record: object id of the north-west corner sample
constexpr const char* GODOT_TERRAIN_SCHEDULE = "Dissemination"; // schedule in which ase-terrain publishes changed sample heights
constexpr uint32_t GODOT_TERRAIN_READS_PER_RUN = 2u;          // terrain reads after a new run: that frame and the next, as for the organs

/**
 * The organ templates (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2, usecase:101011 tessellate the organ templates):
 * ase-render-data builds the fine and the coarse cylinder and the unit leaf on the unit, ase-pl-flora lays
 * their corners on one port and their triangles on another, each record with the code of its template. The
 * codes and keys are the plugin's (ase-pl-flora types.hpp, FLORA_HOST_TMPL_*, FLORA_HOST_KEY_*), repeated
 * here because the adapter reads the port and never links the plugin. A corner record carries the code,
 * its index and x, y, z, nx, ny, nz, u, v; a triangle record the code and three corner indices, clockwise
 * seen from outside. The adapter builds no ring and decides no winding.
 */
constexpr const char* GODOT_KEY_TEMPLATE = "template";        // template records: the code of the template, GODOT_TMPL_*
constexpr const char* GODOT_KEY_INDEX = "index";              // corner record: its place in the template, named by the triangles
constexpr const char* GODOT_KEY_CORNER_A = "a";               // triangle record: index of its first corner
constexpr const char* GODOT_KEY_CORNER_B = "b";               // triangle record: index of its second corner
constexpr const char* GODOT_KEY_CORNER_C = "c";               // triangle record: index of its third corner
constexpr uint32_t GODOT_TMPL_FINE = 0u;                      // template code: the fine cylinder, the internodes up to the shown orders
constexpr uint32_t GODOT_TMPL_CORS = 1u;                      // template code: the coarse cylinder, the internodes above them
constexpr uint32_t GODOT_TMPL_LEAF = 2u;                      // template code: the unit leaf, the stand-in of a leaf or a seed whose template file is missing
constexpr uint32_t GODOT_TMPL_COUNT = 3u;                     // template codes the ports carry

/**
 * The sky and the light of the scene (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11: sun, moon and the day
 * cycle): ase-pl-sky lays four records on its scene port every frame and one more per visible moon
 * (SkyHostScneExptSystem), one per object id below. The sun and the moon record carry the unit
 * direction TOWARDS their body in the world frame of the organ records (x east, y up, z north) - a
 * directional light shines along its negative; colours are shares 0..1. The ids and keys are the
 * plugin's (ase-pl-sky types.hpp, SKY_HOST_*), repeated here because the adapter reads the port and
 * never links the plugin. The adapter computes none of the light.
 */
constexpr uint32_t GODOT_SKY_RECORD_SUN = 1u;                 // sky record object id: the sun - direction towards it, colour, strength
constexpr uint32_t GODOT_SKY_RECORD_AMBIENT = 2u;             // sky record object id: the ambient light - colour, strength
constexpr uint32_t GODOT_SKY_RECORD_BACKGROUND = 3u;          // sky record object id: the background of the sky - colour
constexpr uint32_t GODOT_SKY_RECORD_FOG = 4u;                 // sky record object id: the fog - colour, near and far distance
constexpr uint32_t GODOT_SKY_RECORD_MOON = 5u;                // sky record object id: a visible moon - direction towards it, colour, strength of its light
constexpr uint32_t GODOT_SKY_RECORD_COUNT = 4u;               // records of one sky publication without its moons: sun, ambient, background, fog
constexpr uint32_t GODOT_SKY_MOONS_MAX = 3u;                  // moons one publication may carry: Scorpii, Amilox and Aurora (CAUSA_ASE_SKY, CEL_MOON_*_CONTRIB)
constexpr uint32_t GODOT_SKY_RECORD_COMET = 6u;               // sky record object id: the visible comet - direction towards it, brightness, tail direction and length
constexpr uint32_t GODOT_SKY_COMETS_MAX = 1u;                 // comets one publication may carry: ase-ephemeris hands one comet as EPH_COM_* rows
constexpr const char* GODOT_KEY_TAIL_X = "tail_x";            // comet record: east share of the unit direction the tail runs on the sky
constexpr const char* GODOT_KEY_TAIL_Y = "tail_y";            // comet record: upward share of the unit direction the tail runs on the sky
constexpr const char* GODOT_KEY_TAIL_Z = "tail_z";            // comet record: north share of the unit direction the tail runs on the sky
constexpr uint32_t GODOT_SKY_RECORD_PLANET = 7u;              // sky record object id: Luca above the horizon - direction towards it, angular size, illuminated share
constexpr uint32_t GODOT_SKY_PLANETS_MAX = 1u;                // planets one publication may carry: Luca, the neighbour planet (PLAN_ASE_SKY_EXT)
constexpr const char* GODOT_KEY_PHASE = "phase";              // planet record: illuminated share of the disc, 0 new to 1 full
constexpr uint32_t GODOT_SKY_RECORD_COMPANION = 8u;           // sky record object id: Arcturus above the horizon - direction towards it, angular size, nebula glow
constexpr uint32_t GODOT_SKY_COMPANIONS_MAX = 1u;             // companion stars one publication may carry: Arcturus (CAUSA_ASE_EPHEMERIS)
constexpr uint32_t GODOT_SKY_RECORD_FRAGMENT = 9u;            // sky record object id: a half of the split moon Aurora above the horizon - direction, angular size, brightness
constexpr uint32_t GODOT_SKY_FRAGMENTS_MAX = 2u;              // moon halves one publication may carry: Matrum and Fractum Aurix (SCISSURA AURIX)
constexpr uint32_t GODOT_SKY_RECORD_BELT = 10u;               // sky record object id: the asteroid belt arc - direction to its centre, span, glitter, density
constexpr uint32_t GODOT_SKY_BELTS_MAX = 1u;                  // belts one publication may carry: Cingulum Aurix (CAUSA_ASE_EPHEMERIS)
constexpr const char* GODOT_KEY_SPAN = "span";                // belt record: angle the arc spans along the orbit, radians, centred on the record direction
constexpr const char* GODOT_KEY_DENSITY = "density";          // belt record: apparent share of the belt particles, 0..1
constexpr uint32_t GODOT_SKY_RECORD_SHOWER = 11u;             // sky record object id: the meteor shower while it throws - direction to its radiant, intensity, rate
constexpr uint32_t GODOT_SKY_SHOWERS_MAX = 1u;                // showers one publication may carry: ase-ephemeris hands one shower as EPH_MTR_* rows
constexpr const char* GODOT_KEY_RATE = "rate";                // shower record: meteors per hour out of the radiant
constexpr uint32_t GODOT_SKY_RECORD_STAR = 12u;               // star record object id on the stars port: a visible star - its place on the celestial sphere (pole +y), colour, rendered size in pixels
constexpr uint32_t GODOT_SKY_STARS_MAX = 16384u;              // stars one set of the stars port may carry: the visible part of the 8921 field stars of ase-pl-sky (seed 42) plus the catalogue, with room
constexpr uint32_t GODOT_SKY_RECORD_TURN = 15u;               // sky record object id on the scene port: the turn of the star sphere (quaternion w, x, y, z) and the marks its visible set was computed at
constexpr uint32_t GODOT_SKY_TURNS_MAX = 1u;                  // turn records one scene publication may carry: ase-pl-sky files one per manager
constexpr const char* GODOT_KEY_W = "w";                      // turn record: scalar part of the quaternion that turns the celestial sphere into the world
constexpr const char* GODOT_KEY_SET_SIDEREAL = "set_sidereal"; // turn record: sidereal angle the visible star set was computed at, radians - a new value means a new set on the stars port
constexpr const char* GODOT_KEY_SET_ZENITH = "set_zenith";    // turn record: zenith brightness the visible star set was computed at
constexpr const char* GODOT_KEY_SET_MOON = "set_moon";        // turn record: moon share of the brightness the visible star set was computed at
constexpr const char* GODOT_KEY_POINT = "point";              // star record: rendered size of the star point, pixels (ase-pl-sky STAR_SIZE_*)
constexpr uint32_t GODOT_SKY_RECORD_WIND = 16u;               // sky record object id on the scene port: the wind drift haze and clouds move along, filed only once wind arrived (ase-pl-sky SKY_HOST_RECORD_WIND, usecase:101530)
constexpr uint32_t GODOT_SKY_WINDS_MAX = 1u;                  // wind records one scene publication may carry: ase-pl-sky files one per manager
constexpr const char* GODOT_KEY_DRIFT_X = "drift_x";          // wind record: east share of the drift on the world grid, the wind share folded in (SkyStatAtspWndComponent.drift_x)
constexpr const char* GODOT_KEY_DRIFT_Z = "drift_z";          // wind record: north share of the drift on the world grid, the wind share folded in (SkyStatAtspWndComponent.drift_z)
constexpr const char* GODOT_KEY_WIND = "wind";                // wind record: share of the wind in the drift, 0 calm to 1 full (SkyStatAtspWndComponent.wind_share)
constexpr double GODOT_SKY_DRIFT_MIN = -1.0;                  // wind record: lowest east or north share of the drift
constexpr double GODOT_SKY_DRIFT_MAX = 1.0;                   // wind record: highest east or north share of the drift
constexpr double GODOT_SKY_WIND_MIN = 0.0;                    // wind record: calm
constexpr double GODOT_SKY_WIND_MAX = 1.0;                    // wind record: full wind (SKY_WTHR_WIND_FULL and above)
constexpr uint32_t GODOT_SKY_RECORD_ECLIPSE = 13u;            // sky record object id: the standing eclipse - kind, obscuration, hours to its maximum, duration (usecase:101276)
constexpr uint32_t GODOT_SKY_ECLIPSES_MAX = 1u;               // eclipses one publication may carry: ase-ephemeris hands one eclipse as EPH_ECL_* rows
constexpr const char* GODOT_KEY_KIND = "kind";                // eclipse record: code of the kind of eclipse (EPH_ECL_TYPE, ase-ephemeris ECLIPSE_*)
constexpr const char* GODOT_KEY_OBSCURATION = "obscuration";  // eclipse record: covered share of the eclipsed disc, 0..1
constexpr const char* GODOT_KEY_HOURS_TO_MAX = "hours_to_max"; // eclipse record: game hours until the maximum of the eclipse
constexpr uint32_t GODOT_SKY_RECORD_ECLIPSE_NEXT = 14u;       // sky record object id: the predicted eclipses - game hours to the next solar and the next lunar one (usecase:101276)
constexpr uint32_t GODOT_SKY_ECLIPSE_NEXTS_MAX = 1u;          // prediction records one publication may carry: ase-ephemeris hands one pair as EPH_ECL_NEXT_* rows
constexpr const char* GODOT_KEY_NEXT_SOLAR = "next_solar";    // eclipse next record: game hours until the next solar eclipse (EPH_ECL_NEXT_SOLAR)
constexpr const char* GODOT_KEY_NEXT_LUNAR = "next_lunar";    // eclipse next record: game hours until the next lunar eclipse (EPH_ECL_NEXT_LUNAR)
constexpr const char* GODOT_KEY_NEXT_MOONRISE = "next_moonrise";  // eclipse next record: game hours until Scorpii next rises (EPH_MOO_NEXT_RISE)
constexpr const char* GODOT_KEY_DURATION = "duration";        // eclipse record: game hours the eclipse lasts
constexpr const char* GODOT_KEY_R = "r";                      // sky records: red share of the colour, 0..1
constexpr const char* GODOT_KEY_G = "g";                      // sky records: green share of the colour, 0..1
constexpr const char* GODOT_KEY_B = "b";                      // sky records: blue share of the colour, 0..1
constexpr const char* GODOT_KEY_DISC_R = "disc_r";            // sun record: red share of the disc colour, the sunlight darkened by an eclipse, 0..1
constexpr const char* GODOT_KEY_DISC_G = "disc_g";            // sun record: green share of the disc colour, 0..1
constexpr const char* GODOT_KEY_DISC_B = "disc_b";            // sun record: blue share of the disc colour, 0..1
constexpr const char* GODOT_KEY_INTENSITY = "intensity";      // sun and ambient record: strength of the light
constexpr const char* GODOT_KEY_NEAR = "near";                // fog record: distance where the fog begins, metres
constexpr const char* GODOT_KEY_FAR = "far";                  // fog record: distance where the fog is full, metres
constexpr const char* GODOT_KEY_SHIMMER = "shimmer";          // fog record: shimmer of hot air, 0 none to 1 full (ATM_VIS_HEAT_HAZE)
constexpr const char* GODOT_KEY_SIZE = "size";                // sun and moon record: angular diameter of the disc, radians (ase-ephemeris)

/**
 * Plant geometry (godot_plant_geometry.hpp): the template, ground and instance records go unchanged into the
 * arrays the instances are drawn with - no organ and no template is computed here.
 */
constexpr float GODOT_GEOMETRY_EPSILON = 1.0e-6f;   // a length or extent shorter than this counts as none
constexpr float GODOT_GEOMETRY_UPRIGHT = 0.99f;     // |up share| above which a direction counts as upright: its frame is taken against east, not against up
constexpr double GODOT_ORDER_EVERY = -1.0;          // split order below the stem's (0): every organ of a class lies above it

/**
 * The 3D scene of the care view (godot_plant_scene.hpp, PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1 point 2):
 * the names of its nodes and the keys of the "view" section of the start configuration - the near
 * view and the visible orders by frame time (03.6, V8) are numbers of that section. Every plant stands as
 * one node per body (GODOT_PLANT_BODY_PREFIX and the patch's object id) under GODOT_PLANT_BODIES, its six
 * instance nodes under it.
 */
constexpr const char* GODOT_PLANT_SCENE_ROOT = "PlantScene";      // name of the scene's root node
constexpr const char* GODOT_PLANT_CAMERA = "PlantCamera";         // the camera of the near view
constexpr const char* GODOT_PLANT_SUN = "PlantSun";               // the directional light the sun of the sky port sets
constexpr const char* GODOT_PLANT_MOON = "PlantMoon";             // prefix of the directional light of a moon slot: this and the slot number
constexpr const char* GODOT_PLANT_SUN_DISC = "PlantSunDisc";      // the sun as a disc in the sky, along its direction
constexpr const char* GODOT_PLANT_MOON_DISC = "PlantMoonDisc";    // prefix of the disc of a moon slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_COMET_HEAD = "PlantCometHead";  // prefix of the head of a comet slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_COMET_TAIL = "PlantCometTail";  // prefix of the tail of a comet slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_CLOUDS = "PlantClouds";         // node name of the multimesh of the cloud banks in the sky (CloudBank, N04)
constexpr float GODOT_SKY_CLOUD_DISTANCE = 900.0f;  // metres from the camera to a cloud bank: nearer than the discs, so a bank passes over sun and moon as the mockup draws it after them
constexpr const char* GODOT_PLANT_PLANET_DISC = "PlantPlanetDisc";  // prefix of the disc of a planet slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_COMPANION_DISC = "PlantCompanionDisc";  // prefix of the disc of a companion star slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_FRAGMENT_DISC = "PlantFragmentDisc";    // prefix of the disc of a moon half slot in the sky: this and the slot number
constexpr const char* GODOT_PLANT_BELT_GRAIN = "PlantBeltGrain";          // prefix of a particle of the asteroid belt arc: this and the particle number
constexpr const char* GODOT_PLANT_ENVIRONMENT = "PlantEnvironment";  // sky colour, ambient light and fog of the sky port
constexpr const char* GODOT_PLANT_HAZE = "PlantHaze";             // the shimmer of hot air in front of the camera (usecase:101515 - veil sight with weather)
constexpr const char* GODOT_PLANT_GROUND = "PlantGround";         // the ground from the terrain samples
constexpr const char* GODOT_PLANT_WATER = "PlantWater";           // the water of the brooks in their beds (usecase:101339 - show the ground around plant)
constexpr const char* GODOT_PLANT_BODIES = "PlantBodies";         // the node the plants stand under, one child per body
constexpr const char* GODOT_PLANT_BODY_PREFIX = "PlantBody";      // name of a plant's node: this and the object id of its patch
constexpr const char* GODOT_PLANT_BARK = "PlantBark";             // the continuous bark tube of every axis of the plant, the node of its two meshes below (usecase:101613)
constexpr const char* GODOT_PLANT_TUBES = "PlantTubes";           // the bark tube of the internodes up to the shown orders, every corner of the knot ring
constexpr const char* GODOT_PLANT_TWIGS = "PlantTwigs";           // the bark tube of the internodes above them, on evenly spaced corners of the knot ring
constexpr uint32_t GODOT_TWIG_LEAST_CORNERS = 3u;                 // corners of a knot ring a twig is drawn with at least: the fewest that close a surface, RENDER_DATA_TMPL_CYL_SIDES_CORS of ase-render-data
constexpr const char* GODOT_PLANT_LEAVES = "PlantLeaves";         // the leaves as instances
constexpr const char* GODOT_PLANT_MALE = "PlantMale";             // the male catkins as instances
constexpr const char* GODOT_PLANT_FEMALE = "PlantFemale";         // the female catkins as instances
constexpr const char* GODOT_PLANT_FRUIT = "PlantFruit";           // the fruit stands as instances
constexpr const char* GODOT_PLANT_SEEDS = "PlantSeeds";           // the seed bodies of ase-physics as instances, in world places (seed port)
constexpr const char* GODOT_VIEW_KEY = "view";                    // start configuration: the section of the 3D scene
constexpr const char* GODOT_VIEW_KEY_DISTANCE = "camera_distance_m";  // view: metres from the focused stem base to the camera, along the ground
constexpr const char* GODOT_VIEW_KEY_HEIGHT = "camera_height_m";  // view: metres of the camera above the focused stem base
constexpr const char* GODOT_VIEW_KEY_LOOK = "camera_look_m";      // view: metres above the stem base the camera looks at
constexpr const char* GODOT_VIEW_KEY_FOV = "camera_fov_deg";      // view: vertical field of view, degrees
constexpr const char* GODOT_VIEW_KEY_HEADING = "camera_heading_deg";  // view: compass direction the camera looks in, degrees from north towards east
constexpr const char* GODOT_VIEW_KEY_BUDGET = "frame_budget_ms";  // view: frame time the scene may take (OrderBudget)
constexpr const char* GODOT_VIEW_KEY_HEADROOM = "headroom";       // view: share of the budget below which a frame is calm
constexpr const char* GODOT_VIEW_KEY_CEILING = "order_ceiling";   // view: most orders ever drawn on the fine cylinder
constexpr const char* GODOT_VIEW_KEY_START = "order_start";       // view: orders drawn on the fine cylinder at the start
constexpr const char* GODOT_VIEW_KEY_CALM = "calm_frames";        // view: calm frames in a row that give one order more
constexpr const char* GODOT_VIEW_KEY_OVER = "over_frames";        // view: over frames in a row that take one order back
constexpr const char* GODOT_VIEW_KEY_PICK = "pick_px";            // view: logical pixels around a tap within which the middle of an internode is hit (cutting mode, 03.10 step 9)
constexpr const char* GODOT_VIEW_KEY_FLATTEN = "flatten_m";       // view: radius of the disc one tap in the flatten mode levels, metres (03.8)
constexpr const char* GODOT_VIEW_KEY_LEAST_RING = "least_ring_px"; // view: screen pixels a bark ring narrower than this is held at (plant_bark.gdshader, register B0482)
/**
 * The exposure of the scene camera (register D1721): the eye adapts to the night, a fixed camera does not. The
 * sky port hands the light as ase-pl-sky computes it - at night the sun light 0.05 and the ambient
 * 0.02/0.02/0.06 at intensity 0.15 (ATM_CONST_NIGHT_AMBIENT_*), by day at noon the sun 0.95 and the ambient
 * 0.7 (run 20261025-phase03-v46) - and the view keeps those numbers. It lifts the camera's exposure by the
 * square root of the ratio of GODOT_EXPOSURE_DAY_LIGHT to the light of the scene (scene_exposure), the partial
 * adaptation of an eye, never below 1 and at most
 * GODOT_EXPOSURE_LIFT_MAX, as an eye adapts to starlight: the night stays darker than the day, and the ground
 * and the birch stay to be seen. The multiplier is the camera's, so both renderers, mobile and Compatibility,
 * take it.
 */
constexpr float GODOT_EXPOSURE_DAY_LIGHT = 1.0f;   // light of the scene that needs no lift: the sun at noon plus its ambient share
constexpr float GODOT_EXPOSURE_LIFT_MAX = 32.0f;   // largest lift of the exposure: the night stays at least this much darker than the day's light
/**
 * Sun and moon as discs in the sky (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11: sun, moon and the day cycle).
 * Each disc stands along the direction of its record, at GODOT_SKY_DISC_DISTANCE from the camera, so it
 * keeps its place in the sky however the camera moves - a body at infinity. It glows in the colour of its
 * record, unshaded and outside the fog, and casts no shadow. The sun disc stands while the sun is above
 * the horizon plane; a moon disc while the port carries a record for its slot, which ase-pl-sky files only
 * for a visible moon. The radii are the look of the view, not a sky quantity: at the distance they
 * give the sun about 2.9 and the moon about 2.3 degrees.
 */
constexpr float GODOT_SKY_DISC_DISTANCE = 1000.0f;  // metres from the camera to the centre of a disc
constexpr float GODOT_SUN_DISC_RADIUS = 25.0f;      // metres: radius of the sun disc at GODOT_SKY_DISC_DISTANCE
constexpr float GODOT_MOON_DISC_RADIUS = 20.0f;     // metres: radius of the moon disc at GODOT_SKY_DISC_DISTANCE
constexpr int32_t GODOT_SKY_DISC_SEGMENTS = 32;     // radial segments of a disc's sphere
constexpr int32_t GODOT_SKY_DISC_RINGS = 16;        // rings of a disc's sphere
/**
 * The comet in the sky (usecase:101274 - track the comets). Its head is a disc like a moon's, along the
 * direction of its record; its tail a cone that starts at the head and runs along the tail direction of the
 * record, as long as the arc the record gives (2·d·sin(length/2) at the disc distance d). ase-ephemeris
 * holds no size of the head, so its radius is the look of the view, as the disc radii above are; head and
 * tail glow in the comet's brightness.
 */
constexpr float GODOT_COMET_HEAD_RADIUS = 6.0f;     // metres: radius of the comet head at GODOT_SKY_DISC_DISTANCE (about 0.7 degrees)
constexpr int32_t GODOT_COMET_TAIL_SEGMENTS = 16;   // radial segments of the tail cone
/**
 * The asteroid belt in the sky (usecase:2758 - generated asteroids): its arc is a row of glittering grains
 * along the span of its record, turned about the upward axis from the record direction. The density of the
 * record is the share of the grains that show; the number and the radius of the grains are the look of the
 * view, as the disc radii are.
 */
constexpr uint32_t GODOT_BELT_GRAINS_MAX = 24u;     // grains of the belt arc at full density
constexpr float GODOT_BELT_GRAIN_RADIUS = 1.5f;     // metres: radius of one grain at GODOT_SKY_DISC_DISTANCE
/**
 * The meteor shower in the sky (usecase:2760 - show meteorite in the nightsky): its streaks stand on a ring
 * around the radiant of its record, each a short trail of glowing points running away from the radiant. The
 * intensity of the record is the share of the streaks that show (ase-ephemeris sets the rate to intensity
 * times the peak rate, so the share follows the rate); the number, the spread and the look of the streaks
 * are the look of the view, as the grains of the belt are.
 */
constexpr uint32_t GODOT_SHOWER_STREAKS_MAX = 12u;    // streaks around the radiant at full intensity
constexpr uint32_t GODOT_SHOWER_STREAK_POINTS = 4u;   // glowing points of one streak, from its head outwards
constexpr float GODOT_SHOWER_STREAK_SPREAD = 0.30f;   // radians: distance of a streak head from the radiant
constexpr float GODOT_SHOWER_STREAK_STEP = 0.04f;     // radians: distance between two points of one streak
constexpr float GODOT_SHOWER_POINT_RADIUS = 1.0f;     // metres: radius of one streak point at GODOT_SKY_DISC_DISTANCE
constexpr const char* GODOT_PLANT_SHOWER_POINT = "PlantShowerPoint";  // prefix of a streak point in the sky: this and the point number
/**
 * The stars in the sky (usecase:5606 - observe the sky): one instance of a multimesh per star record,
 * along its direction at GODOT_SKY_DISC_DISTANCE, glowing in the record colour, unshaded and outside the
 * fog. The record gives the size of the star in pixels; the view turns it into metres at the disc distance
 * with its own field of view and viewport height, which is view geometry, not a sky quantity.
 */
constexpr const char* GODOT_PLANT_STARS = "PlantStars";  // node name of the starfield multimesh in the sky
constexpr const char* GODOT_STAR_META_GENERATION = "ase_star_generation";  // meta of the starfield node: star_generation of the set its instances were laid from
constexpr const char* GODOT_STAR_META_PIXEL = "ase_star_pixel";  // meta of the starfield node: metres per pixel at the disc distance the instances were sized with
constexpr float GODOT_STAR_POINT_MIN = 1.0f;             // pixels: smallest size a star point is drawn with, so no star falls between two pixels
constexpr int32_t GODOT_STAR_SEGMENTS = 6;               // radial segments of a star point's sphere
constexpr int32_t GODOT_STAR_RINGS = 3;                  // rings of a star point's sphere
constexpr float GODOT_ZOOM_START = 1.0f;   // zoom of the near view at the start: the camera as the view section places it
constexpr float GODOT_ZOOM_STEP = 0.8f;    // one wheel turn or pinch towards the birch: distance, height and look point times this
constexpr float GODOT_ZOOM_MIN = 0.08f;    // closest zoom: 0.8 m from a seedling at the configured 10 m
constexpr float GODOT_ZOOM_MAX = 1.6f;     // farthest zoom: the whole ground square in sight
constexpr float GODOT_TUBE_MID = 0.5f;                            // middle of the unit tube template (y from 0 to 1): where a tap picks an internode
constexpr double GODOT_MS_PER_SECOND = 1000.0;                    // the order budget takes frame times in milliseconds
constexpr const char* GODOT_ROLE_BARK = "bark_white";             // material role of the white bark of the tubes and twigs
constexpr const char* GODOT_ROLE_BARK_BASE = "bark_base";         // material role of the stem-base bark, mixed in by the share each internode carries
constexpr const char* GODOT_ROLE_LEAF = "leaf";                   // template role of a leaf
constexpr const char* GODOT_ROLE_MALE = "male";                   // template role of a male catkin
constexpr const char* GODOT_ROLE_FEMALE = "female";               // template role of a female catkin
constexpr const char* GODOT_ROLE_FRUIT = "fruit";                 // template role of a fruit stand
constexpr const char* GODOT_ROLE_NUTLET = "nutlet";               // template role of a winged nutlet, built in metres (ase-assgen birch_nutlet)
/**
 * The stand-in of a seed while the nutlet template is missing: the unit leaf at the width of a birch nutlet with
 * both wings, ϑ OTL/NUT/WIDTH of DSGN_024 (ONWInit 0.004 m; BSBI Plant Crib: "total width of nutlet with wings
 * 3-5 mm"). A bound template carries its own size and is drawn at scale one.
 */
constexpr float GODOT_NUTLET_STAND_IN_M = 0.004f;                 // m - width the stand-in leaf of a seed is drawn at

/**
 * Asset binding (godot_asset_binding.hpp, PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2).
 */
constexpr uint32_t GODOT_SHA256_HEX_DIGITS = 64u;                            // hex digits of one SHA-256 checksum
constexpr const char* GODOT_ASSET_MISSING = "asset missing ";               // boot line of a name without its asset, the name follows
constexpr const char* GODOT_ASSETS_PATH = "res://config/vivarium_assets.json";  // asset list of the play stages, staged beside the start configuration
constexpr const char* GODOT_ASSETS_KEY_MANIFEST = "manifest";               // asset list: res:// path of the manifest of the asset set
constexpr const char* GODOT_ASSETS_KEY_ROOT = "asset_root";                 // asset list: res:// directory the manifest's files lie in
constexpr const char* GODOT_ASSETS_KEY_STAGES = "stages";                   // asset list: one entry per play stage
constexpr const char* GODOT_ASSETS_KEY_STAGE = "stage";                     // stage entry: the play stage it lists
constexpr const char* GODOT_ASSETS_KEY_MATERIALS = "materials";             // stage entry: role → material of the list
constexpr const char* GODOT_ASSETS_KEY_TEMPLATES = "templates";             // stage entry: role → organ template of the list
constexpr const char* GODOT_ASSETS_KEY_ENTRIES = "assets";                  // manifest: its entries (ase-web-curator scripts/export-avt-assets.cjs)
constexpr const char* GODOT_ASSETS_KEY_ASSET = "name";                      // manifest entry: which asset of the list it delivers
constexpr const char* GODOT_ASSETS_KEY_FILE = "file";                       // manifest entry: its file below the asset root
constexpr const char* GODOT_ASSETS_KEY_SHA256 = "sha256";                   // manifest entry: SHA-256 of that file
constexpr const char* GODOT_ASSETS_COMMENT = "_";                           // a key of the list starting with it is a comment, never a role
constexpr const char* GODOT_ROLE_GROUND_MEADOW = "ground_meadow";           // material role of meadow soil (GODOT_SOIL_MEADOW)
constexpr const char* GODOT_ROLE_GROUND_FOREST = "ground_forest";           // material role of forest floor (GODOT_SOIL_FOREST)
constexpr uint32_t GODOT_COLOR_ASSET_MISSING = 0xFF00FFFFu;                 // signal colour of a name the asset list gives no stand-in colour
constexpr const char* GODOT_GLTF_KEY_IMAGES = "images";                     // glTF document: its image list, in the order GLTFState::get_images holds them
constexpr const char* GODOT_GLTF_KEY_IMAGE = "name";                        // glTF image: which map of ase-assgen it is (assgen_kit.save_map)
constexpr const char* GODOT_IMAGE_TRANSLUCENCY = "_translucency";           // glTF image ending of the light a thin organ lets through (leaf.py, fruit.py, grass.py), bound as backlight

constexpr uint32_t GODOT_COLOR_STAND_IN_BARK = 0xE6E1D3FFu;               // stand-in of GODOT_ROLE_BARK: the white bark of the birch
constexpr uint32_t GODOT_COLOR_STAND_IN_BARK_BASE = 0x3E3630FFu;          // stand-in of GODOT_ROLE_BARK_BASE: the dark cracked bark of the stem base
constexpr uint32_t GODOT_COLOR_STAND_IN_MEADOW = 0x587A34FFu;             // stand-in of GODOT_ROLE_GROUND_MEADOW: grass over earth
constexpr uint32_t GODOT_COLOR_STAND_IN_FOREST = 0x5B4632FFu;             // stand-in of GODOT_ROLE_GROUND_FOREST: leaf litter over earth
constexpr uint32_t GODOT_COLOR_STAND_IN_LEAF = 0x5E8F32FFu;               // stand-in of GODOT_ROLE_LEAF: a fresh birch leaf
constexpr uint32_t GODOT_COLOR_STAND_IN_MALE = 0xA38A4CFFu;               // stand-in of GODOT_ROLE_MALE: a male catkin
constexpr uint32_t GODOT_COLOR_STAND_IN_FEMALE = 0x7D8C3FFFu;             // stand-in of GODOT_ROLE_FEMALE: a female catkin
constexpr uint32_t GODOT_COLOR_STAND_IN_FRUIT = 0x8A6A3EFFu;              // stand-in of GODOT_ROLE_FRUIT: a ripe fruit stand
constexpr uint32_t GODOT_COLOR_STAND_IN_NUTLET = 0x9C7A44FFu;             // stand-in of GODOT_ROLE_NUTLET: a winged nutlet

/**
 * The water of the care view (usecase:101339 - show the ground around plant). The water stands at NN0 and nowhere
 * else: one plane at GODOT_WATER_TABLE_M over every ground cell below it (build_water_table,
 * godot_plant_water.hpp), so a dug hole, a carved bed and a hollow of the ground below NN0 hold water. The vertex
 * colour carries the water in the range a colour holds: red the velocity of the stream whose bed holds the vertex
 * over GODOT_WATER_VELOCITY_FULL_M_S, green the depth of the table over GODOT_WATER_DEPTH_FULL_M, blue and alpha
 * the course of that stream. The velocity moves the ripples along the course, the depth turns the shallow colour
 * to the deep one; the view computes no water. The ribbon of a river along a course above the table
 * (build_water, GODOT_WATER_STATIONS sections) is drawn by no view now.
 * The two colours are those of brook water, not of the sea: murky, dark, and slightly green from the
 * algae and the suspended silt a lowland brook carries; the shallow colour shows the bed through it.
 */
constexpr float GODOT_WATER_TABLE_M = 0.0f;              // NN0, the water table: the datum ase-terrain generates its ground around (GEN_DEFAULT_BASE_HEIGHT)
constexpr uint32_t GODOT_WATER_STATIONS = 32u;           // sections of the ribbon along one bed
constexpr float GODOT_WATER_VELOCITY_FULL_M_S = 2.0f;    // velocity the red channel reads 1 at, a fast mountain brook
constexpr float GODOT_WATER_DEPTH_FULL_M = 0.5f;         // depth the green channel reads 1 at, the deep colour
constexpr float GODOT_WATER_SWING_SHARE = 1.0f;          // farthest the ribbon follows the deepest line of a winding bed off its filed course, as a share of the bed width

constexpr float GODOT_GROUND_LEVEL_TOLERANCE_M = 0.01f;                    // a vertex within this of a disc's levelled height is levelled ground (mark_ground)
constexpr float GODOT_GROUND_TANGENT_BIAS = 1.0f / 32767.0f;               // least second share of a coded tangent, Godot's bias of Vector3::octahedron_tangent_encode

/**
 * Asset showcase (godot_asset_showcase.hpp, PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7).
 */
constexpr const char* GODOT_SHOWCASE_LINE = "asset showcase";   // first words of the showcase's log line
constexpr const char* GODOT_SHOWCASE_ROOT = "AssetShowcase";    // name of the showcase's root node
constexpr const char* GODOT_SHOWCASE_SUN = "ShowcaseSun";       // name of the light the sun of the host sets
constexpr float GODOT_SHOWCASE_SPACING = 1.5f;                  // metres between two slots of one row
constexpr float GODOT_SHOWCASE_SLOT = 1.0f;                     // metres a template is scaled to along its largest extent
constexpr float GODOT_SHOWCASE_SPHERE_RADIUS = 0.5f;            // metres: radius of the sphere a material is shown on
constexpr float GODOT_SHOWCASE_PLANE_ROW = 1.5f;                // z of the row of planes; the spheres stand at z 0
constexpr float GODOT_SHOWCASE_TEMPLATE_ROW = 3.0f;             // z of the row of organ templates
constexpr float GODOT_SHOWCASE_FOV_DEGREES = 60.0f;             // horizontal field of view the showcase camera keeps
constexpr float GODOT_SHOWCASE_HALF_FOV_TAN = 0.57735027f;      // tan of half of GODOT_SHOWCASE_FOV_DEGREES
constexpr float GODOT_SHOWCASE_CAMERA_HEIGHT_SHARE = 0.6f;      // camera height as a share of its distance: a view from above at about 31 degrees

/**
 * The ground of a patch: the code the lay request carries and the patch record returns
 * (ase-pl-flora FLORA_SOIL_*), and the word the start configuration names it by.
 */
constexpr uint8_t GODOT_SOIL_MEADOW = 0;                      // meadow soil
constexpr uint8_t GODOT_SOIL_FOREST = 1;                      // forest floor
constexpr uint8_t GODOT_SOIL_COUNT = 2;                       // grounds the view has a material for
constexpr const char* GODOT_SOIL_WORD_MEADOW = "meadow";      // word of GODOT_SOIL_MEADOW in the start configuration
constexpr const char* GODOT_SOIL_WORD_FOREST = "forest";      // word of GODOT_SOIL_FOREST in the start configuration

/**
 * The stage of a patch (usecase:101635 - show the plant stage, D2455). The patch record carries the stage
 * code of ase-plant unchanged; the stage record on flora.stage.v1 carries the four codes themselves
 * (GODOT_KEY_STGE_*). read_snapshot turns the stage of every patch into its position among those four keys,
 * in the order the keys are read - seed, sprout, grow, mature - and sets the patch's stage marks by
 * equality with them, as for the condition. The view holds no stage code of its own; a stage that equals
 * none of the four codes is shown as unknown, never mapped onto a known one.
 */
constexpr uint8_t GODOT_STAGE_COUNT = 4;      // stages the view has a word and a colour for: the codes the stage record carries
constexpr uint8_t GODOT_STAGE_NONE = UINT8_MAX;  // debug jump: no stage to jump to, the jump names an hour
constexpr double GODOT_HOUR_NONE = -1.0;      // debug jump: no hour of the game day to jump to, the jump names a stage
constexpr double GODOT_YEARS_NONE = 0.0;      // debug date jump: no VECTUM to jump ahead, the jump names a stage or an hour
constexpr uint8_t GODOT_ECLIPSE_NONE = 0u;    // debug eclipse jump: no kind to land on - ase-ephemeris ECLIPSE_NONE, the jump names another target
constexpr uint8_t GODOT_ECLIPSE_KIND_MAX = 4u;  // highest kind code of ase-ephemeris (ECLIPSE_LUNAR_TOTAL, types.hpp of ase-ephemeris)
constexpr uint8_t GODOT_ECLIPSE_SOLAR_MAX = 2u; // highest solar kind code of ase-ephemeris (ECLIPSE_SOLAR_TOTAL); kinds above it are lunar
constexpr uint8_t GODOT_DATE_PART_YEAR = 0u;    // debug date entry: the VECTUM
constexpr uint8_t GODOT_DATE_PART_MONTH = 1u;   // debug date entry: the LUNBREX of the VECTUM
constexpr uint8_t GODOT_DATE_PART_DAY = 2u;     // debug date entry: the TUMBRAE of the LUNBREX
constexpr uint8_t GODOT_DATE_PART_HOUR = 3u;    // debug date entry: the whole HORAX of the TUMBRAE
constexpr uint8_t GODOT_DATE_PARTS = 4u;        // parts of a SURA date the entry holds
constexpr uint8_t GODOT_DATE_PART_NONE = UINT8_MAX;  // debug date entry: a field or step that names no part
constexpr const char* GODOT_DATE_KEY_YEAR = "year";        // debug.date part word of the VECTUM
constexpr const char* GODOT_DATE_KEY_MONTH = "month";      // debug.date part word of the LUNBREX
constexpr const char* GODOT_DATE_KEY_DAY = "month_day";    // debug.date part word of the TUMBRAE
constexpr const char* GODOT_DATE_KEY_HOUR = "hour";        // debug.date part word of the HORAX
constexpr double GODOT_DATE_YEAR_FIRST = 0.0;   // first VECTUM of a run as the clock record counts it (ase-calendar CAL_DATE_SEEK_YEAR_FIRST)
constexpr double GODOT_DATE_FIRST = 1.0;        // first LUNBREX and first TUMBRAE as the clock record counts them (CAL_DATE_SEEK_DATE_FIRST)
constexpr double GODOT_DATE_HOUR_FIRST = 0.0;   // first HORAX of a TUMBRAE (CAL_DATE_SEEK_HOUR_FIRST)
constexpr double GODOT_PACE_NONE = 0.0;       // pace request: the host's clock back on its default pace
constexpr uint8_t GODOT_CODE_UNKNOWN = 255u;  // a stage outside the documented codes

/**
 * The staged bundle (clients/avt-client-android/cmake/AvtRuntime.cmake, scripts/stage_native.py):
 * one index file names everything else in the directory - one line per unit, the data root and
 * every data file below it.
 */
constexpr const char* GODOT_BUNDLE_INDEX = "avt_bundle.index";   // table of contents of the bundle
constexpr const char* GODOT_BUNDLE_KEY_DATA_ROOT = "data_root";  // index key: directory of the units' data files
constexpr const char* GODOT_BUNDLE_KEY_UNIT = "unit";            // index key: unit library manifest version api sha256
constexpr const char* GODOT_BUNDLE_KEY_DATA = "data";            // index key: one data file, relative to the data root
constexpr uint32_t GODOT_BUNDLE_UNIT_FIELDS = 6u;                // fields of one unit line
constexpr const char* GODOT_BUNDLE_SEPARATOR = " = ";            // between key and value of an index line
constexpr const char* GODOT_DATA_COPY_ROOT = "user://bundle";    // where the data files are copied when the stage is no directory on disk

/**
 * Where the stage of the running build lies: res://native/<os>-<arch>/<config>/, the layout
 * scripts/stage_native.py writes. os, arch and config are Godot's own words for the running build
 * (OS::get_name lower-cased, Engine::get_architecture_name, OS::is_debug_build) - the same feature
 * words the avt.gdextension sections are keyed by, so the stage the view reads is the stage Godot
 * took the adapter from.
 */
constexpr const char* GODOT_STAGE_ROOT = "res://native";         // root of every staged platform
constexpr const char* GODOT_STAGE_JOIN = "-";                    // between os and arch in a stage directory
constexpr const char* GODOT_STAGE_DEBUG = "debug";               // stage of a debug build
constexpr const char* GODOT_STAGE_RELEASE = "release";           // stage of a release build

/**
 * The self-test table (clients/avt-client-android/config/vivarium_selftest.toml, PLAN_ASE_VIVARIUM_
 * PHASE_02_ANDROID 02.3): the keys of its TOML form, read by GodotHostResourceManager::self_test.
 * Every case checks one step within a few frames; all cases run in order on one test host.
 */
constexpr const char* GODOT_SELFTEST_KEY_TOLERANCE = "tolerance";    // largest |actual - value| of an eq or ne
constexpr const char* GODOT_SELFTEST_KEY_FRAME = "frame_seconds";    // real seconds one host frame stands for
constexpr const char* GODOT_SELFTEST_KEY_RUNS = "runs";              // runs of 00.4 the table covers
constexpr const char* GODOT_SELFTEST_KEY_CASE = "case";              // array of scenarios, in order on the one test host
constexpr const char* GODOT_SELFTEST_KEY_DO = "do";                  // actions of a scenario, in order
constexpr const char* GODOT_SELFTEST_KEY_CHECK = "check";            // run an action checks
constexpr const char* GODOT_SELFTEST_KEY_FRAMES = "frames";          // action: advance n frames
constexpr const char* GODOT_SELFTEST_KEY_REMEMBER = "remember";      // action: keep a value for a later plus
constexpr const char* GODOT_SELFTEST_KEY_IRRIGATE = "irrigate";      // action: submit one pour
constexpr const char* GODOT_SELFTEST_KEY_WAIT = "wait";              // action: advance until a value holds
constexpr const char* GODOT_SELFTEST_KEY_CATCH_UP = "catch_up";      // action: a return from the background
constexpr const char* GODOT_SELFTEST_KEY_EXPECT = "expect";          // action: compare one value
constexpr const char* GODOT_SELFTEST_KEY_RECORDS = "records";        // action: compare a record count
constexpr const char* GODOT_SELFTEST_KEY_AS = "as";                  // remember: what a later plus refers to
constexpr const char* GODOT_SELFTEST_KEY_PLACE = "place";            // a patch, by its place
constexpr const char* GODOT_SELFTEST_KEY_OBJECT = "object_id";       // irrigate: an object id named directly
constexpr const char* GODOT_SELFTEST_KEY_RECORD = "record";          // "clock" or "patch"
constexpr const char* GODOT_SELFTEST_KEY_KEY = "key";                // the value key of a record
constexpr const char* GODOT_SELFTEST_KEY_OP = "op";                  // eq ne lt le gt ge
constexpr const char* GODOT_SELFTEST_KEY_VALUE = "value";            // the value an op compares with
constexpr const char* GODOT_SELFTEST_KEY_PLUS = "plus";              // a remembered value added to value
constexpr const char* GODOT_SELFTEST_KEY_MAX_FRAMES = "max_frames";  // wait: the bound of a few frames, never a target
constexpr const char* GODOT_SELFTEST_KEY_SECONDS = "seconds";        // catch_up: real seconds the one long frame files at once
constexpr const char* GODOT_SELFTEST_KEY_AMOUNT = "amount_mm";       // irrigate: water poured, mm
constexpr const char* GODOT_SELFTEST_KEY_STATUS = "status";          // irrigate: the expected HostStatus
constexpr const char* GODOT_SELFTEST_KEY_COUNT = "count";            // records: the expected count
constexpr const char* GODOT_SELFTEST_KEY_LAY = "lay";                // action: lay one patch through the scene port
constexpr const char* GODOT_SELFTEST_KEY_SPECIES = "species";        // lay: catalog id text of the species
constexpr const char* GODOT_SELFTEST_KEY_SEED = "seed";              // lay: seed of the plant
constexpr const char* GODOT_SELFTEST_KEY_SOIL = "soil";              // lay: ground word (GODOT_SOIL_WORDS)
constexpr const char* GODOT_SELFTEST_KEY_X = "x";                    // lay: centre of the patch, world metres east
constexpr const char* GODOT_SELFTEST_KEY_Z = "z";                    // lay: centre of the patch, world metres north
constexpr const char* GODOT_SELFTEST_KEY_HALF = "half_m";            // lay: half edge of the patch square, metres
constexpr const char* GODOT_SELFTEST_KEY_CAPACITY = "capacity_mm";   // lay: field capacity of the patch soil, mm
constexpr const char* GODOT_SELFTEST_KEY_GRAVITY = "gravity_rel";    // lay: gravity of the patch relative to g
constexpr const char* GODOT_SELFTEST_KEY_VIEW = "view";              // action: place the eye of the view through the view port
constexpr const char* GODOT_SELFTEST_KEY_Y = "y";                    // view: the eye, world metres up (x east, z north as for lay)
constexpr const char* GODOT_SELFTEST_KEY_AIM_X = "aim_x";            // view: the point the eye looks at, world metres east
constexpr const char* GODOT_SELFTEST_KEY_AIM_Y = "aim_y";            // view: the point the eye looks at, world metres up
constexpr const char* GODOT_SELFTEST_KEY_AIM_Z = "aim_z";            // view: the point the eye looks at, world metres north
constexpr const char* GODOT_SELFTEST_RECORD_CLOCK = "clock";         // record: the clock port
constexpr const char* GODOT_SELFTEST_RECORD_PATCH = "patch";         // record: the patch port; an organ port by its GODOT_ORGAN_WORDS
constexpr const char* GODOT_SELFTEST_OP_EQ = "eq";                   // |actual - value| <= tolerance
constexpr const char* GODOT_SELFTEST_OP_NE = "ne";                   // |actual - value| > tolerance
constexpr const char* GODOT_SELFTEST_OP_LT = "lt";                   // actual < value
constexpr const char* GODOT_SELFTEST_OP_LE = "le";                   // actual <= value
constexpr const char* GODOT_SELFTEST_OP_GT = "gt";                   // actual > value
constexpr const char* GODOT_SELFTEST_OP_GE = "ge";                   // actual >= value
constexpr uint32_t GODOT_SELFTEST_DECIMALS = 4u;                     // decimals of actual and expected in a report line
constexpr uint32_t GODOT_NUMBER_BYTES = 32u;                         // text of one number in a report line, NUL included
constexpr int GODOT_LOG_LEVEL_MAX = 5;                               // highest level a host line carries (spdlog critical)

/**
 * Measurement mode (PLAN 02.3, acceptance A10): percentiles of the host time (advance + snapshot)
 * and of the whole _process interval, both in microseconds, and the PSS of the process.
 */
constexpr uint32_t GODOT_MEASURE_P50 = 50u;                          // median
constexpr uint32_t GODOT_MEASURE_P95 = 95u;                          // budget percentile of the plan
constexpr uint32_t GODOT_MEASURE_P99 = 99u;                          // tail
constexpr uint32_t GODOT_MEASURE_PERCENT = 100u;                     // whole of a percentile
constexpr double GODOT_MEASURE_FRAMES_RESERVED = 240.0;              // frames per second the buffers are reserved for
constexpr double GODOT_USEC_PER_SECOND = 1000000.0;                  // Time::get_ticks_usec per second
constexpr const char* GODOT_PSS_SOURCE = "/proc/self/smaps_rollup";  // proportional set size of this process
constexpr const char* GODOT_PSS_KEY = "Pss:";                         // its line, value in kB

/**
 * Measurement line of one cycle (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6, godot_cycle_measure.hpp):
 * the organs of the organ ports per branching order, one count per order up to this many; an organ of
 * a deeper order counts in the last one, which the line names as "<n>+".
 */
constexpr uint32_t GODOT_MEASURE_ORDERS = 16u;                       // orders counted one by one

/**
 * The frames after one ground act (lower, raise, flatten): the view watches the _process intervals of this
 * many frames after the act and writes one line with the longest of them against the budget of a frame that
 * carries a changed ground (P0689: the terrain travels as a delta, no frame of a stab takes longer).
 */
constexpr uint32_t GODOT_GROUND_WATCH_FRAMES = 30u;                  // frames watched after one ground act
constexpr uint64_t GODOT_GROUND_WATCH_BUDGET_USEC = 100000u;         // longest interval one of them may take

/**
 * Boot steps of GodotHostResourceManager::boot - the name a failure is reported under.
 */
constexpr const char* GODOT_STEP_NONE = "";                // no failure recorded
constexpr const char* GODOT_STEP_CONFIG = "start_config";  // the start configuration is incomplete
constexpr const char* GODOT_STEP_CREATE = "create";        // KernelEmbeddedHost::create
constexpr const char* GODOT_STEP_BUNDLE = "bundle_index";  // reading the bundle index
constexpr const char* GODOT_STEP_MANIFEST = "manifest";    // reading a staged manifest
constexpr const char* GODOT_STEP_DATA_ROOT = "data_root";  // KernelEmbeddedHost::set_data_root
constexpr const char* GODOT_STEP_LOAD = "load_units";      // KernelEmbeddedHost::load_units
constexpr const char* GODOT_STEP_PORT = "port";            // a configured port is not offered
constexpr const char* GODOT_STEP_START = "start";          // KernelEmbeddedHost::start
constexpr const char* GODOT_STEP_ELEVATION = "elevation";  // the shape request of the configured relief was refused
constexpr const char* GODOT_STEP_LAY = "lay";              // the lay request of a configured patch was refused
constexpr const char* GODOT_STEP_GROUND = "ground";        // the lay request of the configured ground square was refused
constexpr const char* GODOT_STEP_BROOK = "brook";          // the lay request of a configured brook was refused
constexpr const char* GODOT_STEP_ADVANCE = "advance";      // KernelEmbeddedHost::advance
constexpr const char* GODOT_STEP_SNAPSHOT = "snapshot";    // reading a port snapshot
constexpr const char* GODOT_STEP_STOP = "stop";            // KernelEmbeddedHost::stop
constexpr const char* GODOT_LOG_SOURCE = "AVT";            // tag of the adapter's own log lines

/**
 * Places in the Godot project and its user directory.
 */
constexpr const char* GODOT_CONFIG_PATH = "res://config/vivarium_start.json";  // start configuration staged by avt-stage
constexpr const char* GODOT_LOG_PATH = "user://logs/ase-embedded.log";         // ASE log file, resolved to an absolute path
constexpr const char* GODOT_IDENTITY_FILE = "build.identity";                  // stage identity beside the libraries
constexpr const char* GODOT_TEXT_CONFIG_ERROR = "Erro de configuração";        // panel title while the configuration itself is unreadable
constexpr int64_t GODOT_RECT_FIELDS = 4;      // x, y, width, height of a rectangle in the configuration
constexpr int64_t GODOT_POINT_FIELDS = 2;     // x, y of a text anchor in the configuration
constexpr uint32_t GODOT_NO_OBJECT = UINT32_MAX;  // object id that means: nothing selected, nothing pending (a patch entity may be 0)
constexpr uint32_t GODOT_NO_PLACE = UINT32_MAX;   // place that means: the record names none

/**
 * The GDExtension class AseVivariumView (godot_registration.cpp): its name, the engine class its
 * objects are built from, and the engine callbacks the view takes over. Godot asks for each
 * callback by name AND by the hash of its signature; the hashes are those of the pinned
 * extension API (godot-cpp e83fd0904c = godot-4.5-stable, gdextension/extension_api.json). A
 * different hash is a different signature and is answered with "not taken over".
 */
constexpr const char* GODOT_VIEW_CLASS = "AseVivariumView";       // class the scene instantiates
constexpr const char* GODOT_VIEW_ENGINE_CLASS = "Node2D";         // engine class of the view's object
constexpr const char* GODOT_HOOK_READY = "_ready";                // Node: in the tree, children ready
constexpr uint32_t GODOT_HOOK_READY_HASH = 3218959716u;           // signature void ()
constexpr const char* GODOT_HOOK_PROCESS = "_process";            // Node: once per frame
constexpr uint32_t GODOT_HOOK_PROCESS_HASH = 373806689u;          // signature void (float delta)
constexpr const char* GODOT_HOOK_DRAW = "_draw";                  // CanvasItem: redraw requested
constexpr uint32_t GODOT_HOOK_DRAW_HASH = 3218959716u;            // signature void ()
constexpr const char* GODOT_HOOK_INPUT = "_unhandled_input";      // Node: input no control consumed
constexpr uint32_t GODOT_HOOK_INPUT_HASH = 3754044979u;           // signature void (InputEvent event)
constexpr const char* GODOT_HOOK_EXIT = "_exit_tree";             // Node: leaving the tree
constexpr uint32_t GODOT_HOOK_EXIT_HASH = 3218959716u;            // signature void ()

/**
 * Time without a pause (PLAN 01.3 point 4): every frame hands the host the real time that passed
 * since the previous one, measured on Godot's monotonic clock, and files the absolute real
 * position on the real port. A span the app spent in the background arrives as one long frame:
 * ase-time sets its clock forward over it in one write and reports the position it laid
 * (TIM_REAL_LAID_S); nothing is replayed in ticks.
 */
constexpr double GODOT_CATCH_UP_NOTE_S = 1.0;   // a frame longer than this is a return from the background and gets one line
constexpr uint32_t GODOT_SOWING_FIRST = 0u;      // the first sowing of a view: the seeds of the start configuration (godot_new_seed.hpp)
constexpr uint64_t GODOT_WATER_WAIT_STEPS = 3u;  // Dissemination runs a pour may take to show in the soil water: plugin, hydro, balance, publish
constexpr double GODOT_WATER_ARRIVED_SHARE = 0.5;  // share of the expected rise (the pour, or the room up to field capacity if smaller) the soil water must gain before the pour counts as arrived; drift between press and arrival moves it by thousandths of a millimetre

/**
 * The start screen of AseVivariumView: it covers the initialisation and lifts when the first frame shows everything.
 */
constexpr uint32_t GODOT_INIT_RENDER_FRAMES = 10u;          // frames in a row the complete scene renders under the start screen without a new pipeline compilation before it lifts
constexpr uint32_t GODOT_INIT_SCENE_PARTS = 5u;             // parts the scene must stand with: terrain, streams, templates, organs, camera focus
constexpr double GODOT_INIT_STEPS = 6.0;                    // equal steps of the progress bar: patches, brooks, scene parts, sky and clock, figure file, quiet frames
constexpr float GODOT_PROGRESS_WIDTH_SHARE = 0.6f;          // width of the progress bar as share of the place area
constexpr float GODOT_PROGRESS_HEIGHT = 14.0f;              // height of the progress bar (logical pixels)
constexpr float GODOT_PROGRESS_GAP = 36.0f;                 // gap between the loading line and the progress bar, and the bar and its percent line
constexpr uint32_t GODOT_SPINNER_DOTS = 8u;                 // dots of the spinner above the loading line
constexpr float GODOT_SPINNER_RADIUS = 34.0f;               // radius of the spinner circle (logical pixels)
constexpr float GODOT_SPINNER_DOT = 6.0f;                   // radius of one spinner dot (logical pixels)
constexpr float GODOT_SPINNER_GAP = 70.0f;                  // gap between the spinner centre and the loading line
constexpr uint64_t GODOT_SPINNER_PERIOD_MS = 1200u;         // milliseconds of one spinner turn
constexpr float GODOT_SPINNER_DIM = 0.2f;                   // opacity of the dot farthest behind the leading one

/**
 * Drawing style of AseVivariumView - RGBA, 8 bit per channel (Godot Color::hex).
 */
constexpr uint32_t GODOT_COLOR_BACKGROUND = 0x14181AFFu;    // surface behind everything
constexpr uint32_t GODOT_COLOR_PROGRESS = 0x8BC34AFFu;      // filled part of the start screen progress bar
constexpr uint32_t GODOT_COLOR_PROGRESS_TRACK = 0x37474FFFu;  // empty part of the start screen progress bar
constexpr uint32_t GODOT_COLOR_HEADER = 0x1F2A2EFFu;        // band behind title and build line
constexpr uint32_t GODOT_COLOR_SOIL = 0x5B4030FFu;          // bare soil of a patch
constexpr uint32_t GODOT_COLOR_SEED = 0x9C7A44FFu;          // seed dots on bare soil
constexpr uint32_t GODOT_COLOR_SPROUT = 0x8BC34AFFu;        // cover of a seedling
constexpr uint32_t GODOT_COLOR_GROW = 0x4CAF50FFu;          // cover of a growing stand
constexpr uint32_t GODOT_COLOR_MATURE = 0x2E7D32FFu;        // cover of a mature stand
constexpr uint32_t GODOT_COLOR_WILTED = 0xA1887FFFu;        // cover of a wilted stand
constexpr uint32_t GODOT_COLOR_DEAD = 0x6E6552FFu;          // cover of a dead stand
constexpr uint32_t GODOT_COLOR_UNKNOWN = 0x9E9E9EFFu;       // cover of a code the view does not know
constexpr uint32_t GODOT_COLOR_WATER = 0x42A5F5FFu;         // filled part of the soil water bar
constexpr uint32_t GODOT_COLOR_WATER_TRACK = 0x1C2B3AFFu;   // empty part of the soil water bar
constexpr uint32_t GODOT_COLOR_SELECTION = 0xFFD54FFFu;     // frame around the selected patch
constexpr uint32_t GODOT_COLOR_TEXT = 0xECEFF1FFu;          // regular text
constexpr uint32_t GODOT_COLOR_TEXT_DIM = 0x90A4AEFFu;      // secondary text
constexpr uint32_t GODOT_COLOR_BUTTON = 0x37474FFFu;        // enabled button face
constexpr uint32_t GODOT_COLOR_BUTTON_OFF = 0x23292CFFu;    // disabled button face
constexpr uint32_t GODOT_COLOR_ERROR = 0xC62828FFu;         // error panel

constexpr int32_t GODOT_FONT_TITLE = 44;                    // title size (logical pixels)
constexpr int32_t GODOT_FONT_BODY = 28;                     // info lines and button labels
constexpr int32_t GODOT_FONT_SMALL = 20;                    // build line, patch labels, notes
constexpr int32_t GODOT_FONT_MIN = 12;                      // smallest size a line shrinks to so that it fits its width
constexpr float GODOT_FRAME_WIDTH = 6.0f;                   // selection frame line width
constexpr float GODOT_BORDER_WIDTH = 2.0f;                  // patch and button outline width
constexpr float GODOT_BORDER_WIDTH_ACTIVE = 4.0f;           // outline of the debug button whose time lapse is running
constexpr float GODOT_PATCH_INSET = 14.0f;                  // gap between patch edge and cover
constexpr float GODOT_BAR_HEIGHT = 16.0f;                   // height of the soil water bar
constexpr float GODOT_LABEL_OFFSET = 34.0f;                 // baseline of a label below a rect's top
constexpr float GODOT_HALF = 0.5f;                          // centre of a span
constexpr float GODOT_LAYOUT_SCALE_MAX = 1.0f;              // a layout is never magnified: spare space is distributed, not filled
constexpr const char* GODOT_FEATURE_MOBILE = "mobile";      // Godot feature of a handheld OS: the window is the screen, its safe area applies
constexpr const char* GODOT_DISPLAY_HEADLESS = "headless";  // name of the display server that opens no window (usecase:101019 start the client window)
constexpr uint32_t GODOT_WINDOW_NUMBER_CHARS = 32u;         // text room of one measured value in the window line
constexpr uint32_t GODOT_WINDOW_DECIMALS = 2u;              // decimals of a surface size or screen scale in the window line
constexpr float GODOT_SEED_DOT_RADIUS = 5.0f;               // radius of one seed dot on bare soil
constexpr int32_t GODOT_SEED_DOTS = 3;                      // seed dots per row and per column
constexpr int32_t GODOT_ERROR_LINES = 6;                    // most lines the error panel wraps to
constexpr int64_t GODOT_LOG_DECIMALS = 3;                   // decimals of a measured value in a log line
constexpr const char* GODOT_LOG_STEP_SCHEDULE = "Dissemination";  // schedule whose runs a log line counts as steps= (the modules' 1 Hz step)
constexpr const char* GODOT_DAY_TURN_SCHEDULE = "GameDawn";        // game-time schedule ase-calendar requests once per crossed TUMBRAE; its runs close a measured cycle
constexpr int64_t GODOT_TEXT_DECIMALS = 1;                  // decimals of a value drawn on the surface
constexpr double GODOT_PERCENT = 100.0;                     // a share drawn as a percentage

// The motion of the HUD (usecase:101440, 06.3 UiMotion): a design token names a CSS easing, these are the
// control points CSS Easing Functions Level 1 defines for its keywords, and the solver of cubic-bezier().
constexpr const char* GODOT_EASE_LINEAR = "linear";                 // CSS keyword: progress equals time
constexpr const char* GODOT_EASE = "ease";                          // CSS keyword ease
constexpr const char* GODOT_EASE_IN = "ease-in";                    // CSS keyword ease-in
constexpr const char* GODOT_EASE_OUT = "ease-out";                  // CSS keyword ease-out
constexpr const char* GODOT_EASE_IN_OUT = "ease-in-out";            // CSS keyword ease-in-out
constexpr const char* GODOT_EASE_BEZIER = "cubic-bezier(";          // head of a CSS cubic-bezier(x1, y1, x2, y2)
constexpr const char* GODOT_EASE_BEZIER_END = ")";                  // tail of that function
constexpr const char* GODOT_EASE_BEZIER_SEPARATOR = ",";            // separator of its four numbers
constexpr int64_t GODOT_EASE_BEZIER_POINTS = 4;                     // x1, y1, x2, y2: the numbers a cubic-bezier() carries
constexpr double GODOT_EASE_DEFAULT_X1 = 0.25;                      // ease = cubic-bezier(0.25, 0.1, 0.25, 1): first control x
constexpr double GODOT_EASE_DEFAULT_Y1 = 0.1;                       // ease: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_DEFAULT_X2 = 0.25;                      // ease: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_DEFAULT_Y2 = 1.0;                       // ease: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_X1 = 0.42;                           // ease-in = cubic-bezier(0.42, 0, 1, 1): first control x
constexpr double GODOT_EASE_IN_Y1 = 0.0;                            // ease-in: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_X2 = 1.0;                            // ease-in: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_Y2 = 1.0;                            // ease-in: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_X1 = 0.0;                           // ease-out = cubic-bezier(0, 0, 0.58, 1): first control x
constexpr double GODOT_EASE_OUT_Y1 = 0.0;                           // ease-out: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_X2 = 0.58;                          // ease-out: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_Y2 = 1.0;                           // ease-out: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_X1 = 0.42;                       // ease-in-out = cubic-bezier(0.42, 0, 0.58, 1): first control x
constexpr double GODOT_EASE_IN_OUT_Y1 = 0.0;                        // ease-in-out: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_X2 = 0.58;                       // ease-in-out: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_Y2 = 1.0;                        // ease-in-out: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_LINEAR_X1 = 0.0;                        // linear as a bezier on the diagonal: first control x
constexpr double GODOT_EASE_LINEAR_Y1 = 0.0;                        // linear on the diagonal: first control y
constexpr double GODOT_EASE_LINEAR_X2 = 1.0;                        // linear on the diagonal: second control x
constexpr double GODOT_EASE_LINEAR_Y2 = 1.0;                        // linear on the diagonal: second control y
constexpr int32_t GODOT_EASE_SOLVE_STEPS = 24;                      // bisection steps that find the curve parameter of a time (2^-24 of the span)
constexpr double GODOT_EASE_SOLVE_EPSILON = 1.0e-6;                 // time distance (share of the duration) at which the parameter counts as found

// The ring of the HUD (usecase:101440, 06.3 UiRing): the mockup's arcP measures an angle in degrees from the
// top clockwise, closes a full turn just short of 360 so that the path stays an arc, and drops a sweep below
// a tenth of a degree.
constexpr double GODOT_RING_TOP_DEG = 90.0;                         // arcP: (a - 90) degrees turns the mockup's top to Godot's angle 0 at +x
constexpr double GODOT_RING_FULL_DEG = 359.9;                       // arcP: degrees a full-turn sweep is cut to, so the path stays one arc
constexpr double GODOT_RING_MIN_DEG = 0.1;                          // ARC: degrees below which a sweep draws no path
constexpr double GODOT_RING_TURN_DEG = 360.0;                       // degrees of one turn
constexpr double GODOT_RING_POINTS_PER_TURN = 96.0;                 // polyline points draw_arc gets for a full circle; a shorter arc gets its share
constexpr int32_t GODOT_RING_POINTS_MIN = 3;                        // fewest polyline points of the shortest arc
constexpr double GODOT_TEXT_ANCHOR_START = 0.0;                     // text-anchor start: the whole width right of x
constexpr double GODOT_TEXT_ANCHOR_MIDDLE = 0.5;                    // text-anchor middle: half the width left of x
constexpr double GODOT_TEXT_ANCHOR_END = 1.0;                       // text-anchor end: the whole width left of x
constexpr int64_t GODOT_TEXT_RASTER_MIN = 1;                        // smallest whole raster size a HUD line is shaped at
constexpr int32_t GODOT_HEX_CORNERS = 6;                            // corners of a HUD hexagon chip, the first at 0 degrees

// The drawing of a carried thing (usecase:101440, 06.3 UiItemSprite): item_art.json writes each path as SVG
// path data; the reader knows the commands generate-item-art.cjs lets through and flattens every curve.
constexpr const char* GODOT_ITEM_PATH_COMMANDS = "MLHVCQAZmlhvcqaz";  // the path commands of item_art.json (generate-item-art.cjs PATH_COMMANDS)
constexpr const char* GODOT_ITEM_PATH_SEPARATORS = " ,\t\r\n";       // characters between two numbers of SVG path data
constexpr int32_t GODOT_ITEM_CURVE_POINTS = 16;                       // points one Bezier piece or one arc is flattened to
constexpr int32_t GODOT_ITEM_POLYGON_MIN = 3;                         // fewest corners a filled outline needs
constexpr double GODOT_ITEM_POINT_EPSILON = 1.0e-4;                   // view-box distance at which two outline points count as one

// The backend gaps of the care view (PLAN_ASE_VIVARIUM_PHASE_06_HUD bridge section, carrier N0448): while a port
// does not deliver a value, the bridge writes the mockup's value from SHA into the frame and the picture stays
// whole; VivariumView::open_gap logs the gap once on its edge with its register entry, never in the picture.
// The register is the one list; these name its entries.
constexpr const char* GODOT_HUD_GAP_CARRY = "N0451";                  // flora.carry.v1: things, place, containers, load (ase-inventory)
constexpr const char* GODOT_HUD_GAP_STOW = "N0452";                   // flora.stow.v1: moving a thing with its refusal code
constexpr const char* GODOT_HUD_GAP_WEATHER = "N0449";                // flora.weather.v1: the weather kind, wind and what falls (ase-weather)
constexpr const char* GODOT_HUD_GAP_PATCH = "N0454";                  // flora.patch.v1: soil water and the preview after watering
constexpr const char* GODOT_HUD_GAP_LOOSE = "N0456";                  // the loosened share of the soil after the spade (loosen the soil)
constexpr const char* GODOT_HUD_GAP_SAP = "N0457";                    // birch sap: a running tap and the fill of its vessel
constexpr const char* GODOT_HUD_GAP_PACE = "N0459";                   // flora.pace.v1: the factor ase-time runs this host's clock at
constexpr const char* GODOT_HUD_GAP_ROOT = "N0463";                   // the roots of the patch on the port: depth and spread under the trunk
constexpr const char* GODOT_HUD_GAP_CLOCK = "N0460";                  // flora.clock.v1: season, light, phase starts, shares of the day and the year (ase-calendar, ase-time)
constexpr const char* GODOT_HUD_GAP_TRAIL = "N0450";                  // flora.trail.v1: the past events with their share of the year and domain
constexpr const char* GODOT_HUD_GAP_SIGN = "N0461";                   // the state fields of each sign case on the patch set (wilt, dead, flush, shed, rest, catkin, sap, brook)
constexpr const char* GODOT_HUD_GAP_PRUNE = "N0462";                  // flora.scene.v1: the internodes that may be cut

// The debug layer (usecase:101120, 06.4 DebugLayer, the mockup's debugLayer()): its four tabs in the order of
// texts.hud.debug_tabs, and the index no chip has.
constexpr uint32_t GODOT_HUD_DEBUG_TAB_TEMPO = 0u;                    // the time lapse: debug.buttons, flora.pace.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_SALTO = 1u;                    // the jumps: debug.jumps, flora.seek.v1 and flora.date.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_DATA = 2u;                     // the free date entry: debug.date, flora.date.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_RELEVO = 3u;                   // the relief: debug.reliefs, flora.elevation.v1
constexpr uint32_t GODOT_HUD_DEBUG_TABS = 4u;                         // tabs of the drawer
constexpr uint32_t GODOT_HUD_DEBUG_NONE = UINT32_MAX;                 // a hit on no chip of that kind

// The hand slot (usecase:100367, 06.4 HandSlot, the mockup's hand()): the modes whose lines it speaks, named as
// hud.modes, the ops of a terrain verb (the token hud_verb_<v>_op), the hold word of a tool at work
// (hud_item_<id>_hold) and the acts of the hand, the stow button and a verb chip.
constexpr const char* GODOT_HUD_MODE_REST = "rest";                   // nothing chosen: the empty hand may say where to start
constexpr const char* GODOT_HUD_MODE_WATER = "water";                 // the can: pour, its arcs, the soil before and after
constexpr const char* GODOT_HUD_MODE_PRUNE = "prune";                 // the pruning knife: the shoots left to cut
constexpr const char* GODOT_HUD_MODE_SOW = "sow";                     // the planter: the seeds left in the pouch
constexpr const char* GODOT_HUD_MODE_TERRAIN = "terrain";             // a ground tool: its verbs above the hand
constexpr const char* GODOT_HUD_MODE_SAP = "sap";                     // the auger: the spouts left for a tap
constexpr const char* GODOT_HUD_MODE_PLUG = "plug";                   // the peg: the sap the vessel holds
constexpr const char* GODOT_HUD_VERB_OP_BED = "bed";                  // lays a brook bed between two points
constexpr const char* GODOT_HUD_VERB_OP_LEVEL = "level";              // levels the whole area
constexpr const char* GODOT_HUD_VERB_OP_MEASURE = "measure";          // measures one point
constexpr const char* GODOT_HUD_VERB_TOKEN = "hud_verb_";            // theme: the head of every verb token hud_verb_<v>_*
constexpr const char* GODOT_HUD_VERB_OP_TAIL = "_op";                 // theme: hud_verb_<v>_op, the terrain op of the verb
constexpr const char* GODOT_HUD_VERB_TONE_TAIL = "_tone";             // theme: hud_verb_<v>_tone, the domain its hexagon draws in
constexpr const char* GODOT_HUD_VERB_BRUSH_TAIL = "_brush_m";         // theme: hud_verb_<v>_brush_m, the radius a brush verb works under, metres
constexpr int64_t GODOT_HUD_GROUND_NO_SAMPLE = -1;                    // UiGround: a lattice point the terrain port carried no sample for
constexpr const char* GODOT_HUD_ITEM_HOLD_NONE = "none";              // a tool at work: the hand offers its use
constexpr const char* GODOT_HUD_ACT_POUR = "pour";                    // the can in the hand in the mode water: pours while held
constexpr const char* GODOT_HUD_ACT_STOW = "drop";                    // the stow button: the thing goes back where it came from
constexpr const char* GODOT_HUD_ACT_VERB = "terr";                    // a verb chip; its verb goes with it

// The anchors a sign stands at (usecase:100370, 06.4 SignMarker, the token hud_sign_<id>_at) and the bar value
// of a sign without a bar (hud_sign_<id>_bar).
constexpr const char* GODOT_HUD_SIGN_AT_CROWN = "crown";              // the mockup's crownPt(x, y) over the trunk
constexpr const char* GODOT_HUD_SIGN_AT_TRUNK = "trunk";              // an offset from the foot of the trunk
constexpr const char* GODOT_HUD_SIGN_AT_BROOK = "brook";              // the share x along the brook, y above it
constexpr const char* GODOT_HUD_SIGN_AT_SCREEN = "screen";            // a point of the mockup's screen
constexpr const char* GODOT_HUD_SIGN_BAR_NONE = "none";               // the sign has no bar

// The weather kinds flora.weather.v1 names, as the mockup's S.weather; the veil (WeatherVeil) draws none of the
// first three, the sky band (SkyBand) gives every kind its day and night glyph.
constexpr const char* GODOT_HUD_WEATHER_CLEAR = "clear";              // an open sky
constexpr const char* GODOT_HUD_WEATHER_PCLOUD = "pcloud";            // a sky partly clouded
constexpr const char* GODOT_HUD_WEATHER_CLOUDY = "cloudy";            // a sky clouded over
constexpr const char* GODOT_HUD_WEATHER_FOG = "fog";                  // bands of mist over the horizon
constexpr const char* GODOT_HUD_WEATHER_RAIN = "rain";                // streaks leaning with the wind
constexpr const char* GODOT_HUD_WEATHER_DRIZZLE = "drizzle";          // short fine dashes
constexpr const char* GODOT_HUD_WEATHER_SNOW = "snow";                // swaying flakes

// The parts of the care view a touch can land on, named as the mockup's data-act (usecase:101440, 06.7 BagWindow).
constexpr const char* GODOT_HUD_ACT_PACK_GRIP = "win:ranzen";         // the leather pack's flap: drags the window
constexpr const char* GODOT_HUD_ACT_POUCH_GRIP = "win:bolsa";         // the linen pouch's neck: drags the window
constexpr const char* GODOT_HUD_ACT_WINDOW_BODY = "winbody";          // a window's body: takes the touch, does nothing
constexpr const char* GODOT_HUD_ACT_PACK_CLOSE = "bag";               // the pack's cross (and the bag icon): opens or closes it
constexpr const char* GODOT_HUD_ACT_POUCH_CLOSE = "pclose";           // the pouch's cross: closes it
constexpr const char* GODOT_HUD_ACT_ITEM = "item";                    // a thing; its key goes with it
constexpr const char* GODOT_HUD_CONTAINER_PACK = "ranzen";            // the leather pack as a place things lie in
constexpr const char* GODOT_HUD_CONTAINER_POUCH = "bolsa";            // the linen pouch as a place things lie in
constexpr const char* GODOT_HUD_CONTAINER_BELT_FIRST = "belt0";       // the left holster on the belt
constexpr const char* GODOT_HUD_CONTAINER_BELT_SECOND = "belt1";      // the right holster on the belt
constexpr const char* GODOT_HUD_ACT_NOOP = "noop";                    // an empty holster: takes the touch, does nothing
constexpr const char* GODOT_HUD_ACT_REST = "rest";                    // the scrim of the tool rack's list: back to the mode rest
constexpr const char* GODOT_HUD_ACT_SKY = "sky";                      // the sky band: opens the calendar sheet
constexpr const char* GODOT_HUD_ACT_DEBUG = "dbg";                    // the sky band's debug switch: shows or hides the debug layer
constexpr const char* GODOT_HUD_ITEM_ART_PACK = "ranzen";             // item_art.json: the drawing of the leather pack (BagIcon)

// Drag and drop (usecase:101440, 06.7 DragLayer, the mockup's dropTarget and dropItem): the places a thing can go
// besides a container (tg.c), the place words of texts.hud (tg.lab), the words and colours of the toast after a
// release, and what a release did.
constexpr const char* GODOT_HUD_PLACE_HAND = "hand";                  // the hand slot: the thing is equipped
constexpr const char* GODOT_HUD_PLACE_GROUND = "ground";              // the ground of the world, the tool place included
constexpr const char* GODOT_HUD_WHERE_HAND = "where.hand";            // texts.hud: in the hand
constexpr const char* GODOT_HUD_WHERE_BELT = "where.belt";            // texts.hud: on the belt, either holster
constexpr const char* GODOT_HUD_WHERE_PACK = "where.ranzen";          // texts.hud: in the leather pack
constexpr const char* GODOT_HUD_WHERE_POUCH = "where.bolsa";          // texts.hud: in the linen pouch
constexpr const char* GODOT_HUD_WHERE_RACK = "where.rack";            // texts.hud: at the tool place
constexpr const char* GODOT_HUD_WHERE_GROUND = "where.ground";        // texts.hud: on the ground
constexpr const char* GODOT_HUD_WHERE_TRUNK = "where.trunk";          // texts.hud: on the trunk, the vessel under the tap
constexpr const char* GODOT_HUD_WORD_DROP_AT = "drop_at";             // texts.hud: {item} · {where} after an accepted drop
constexpr const char* GODOT_HUD_WORD_FIT = "fit_";                    // texts.hud: the head of a refusal word fit_<c>
constexpr const char* GODOT_HUD_WORD_ITEM_HEAD = "items.";            // texts.hud: the head of an item's word items.<id>.
constexpr const char* GODOT_HUD_WORD_ITEM_TAIL = ".name";             // texts.hud: the tail of an item's word .<field>
constexpr const char* GODOT_HUD_WORD_ITEM_CASE = ".case";             // texts.hud: the tail of an item's case items.<id>.case
constexpr const char* GODOT_HUD_COLOUR_DROP = "DOMAIN_GROUND";        // the toast of an accepted drop (toast(..., 'ground'))
constexpr const char* GODOT_HUD_COLOUR_REFUSED = "DOMAIN_ALERT";      // the toast of a refused drop (toast(tg.bad, 'alert'))
constexpr const char* GODOT_HUD_DRAG_TAP = "tap";                     // the finger never moved past the slop: a tap on the thing
constexpr const char* GODOT_HUD_DRAG_SEND = "send";                   // the module allowed the place: the move goes to flora.stow.v1
constexpr const char* GODOT_HUD_DRAG_REFUSED = "refused";             // the module refused the place: its word was toasted
constexpr const char* GODOT_HUD_DRAG_HELD = "held";                   // no place under the finger or no verdict: the thing stays

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the value keys and codes of the host ports of ase-pl-flora the view reads
 * @description All compile-time constants of the port sub-type; NO runtime values - those belong in the views.
 *              The patch, clock, date, day, phase and season records, the SURA date line, the lay, seek and view
 *              requests, the organ records with their frame, the bark tiling, the sway and the bark tube, the
 *              organ and instance ports, the ground, terrain and stream records and the organ templates: the
 *              documented contract of the ports (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 point 1), repeated here
 *              because the adapter reads the ports and never links the plugin.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These values are read by the port
 *              readers of the host and by the views that lay out their records.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
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

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: tmpl = template, an organ template of ase-render-data.
 */

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
constexpr const char* GODOT_KEY_ROOT_M = "root_m";            // patch record: reach of the root of the patch's body, metres (RDX_SPREAD), the root zone
constexpr const char* GODOT_KEY_GRAVITY_REL = "gravity_rel";  // patch lay request: gravity of the patch relative to g; ase-pl-flora judges the bounds
constexpr const char* GODOT_KEY_GROW_AT_H = "grow_at_h";      // patch record: game hour ase-plant forecasts CRESCENDO at (PLT_GROW_AT_H)
constexpr const char* GODOT_KEY_MTUR_AT_H = "mtur_at_h";      // patch record: game hour ase-plant forecasts MADURA at (PLT_MTUR_AT_H)
constexpr const char* GODOT_KEY_BASE_Y = "base_y";            // stem record: the stem foot ase-physics holds, metres above the level of its patch
constexpr const char* GODOT_KEY_CELL_M = "cell_m";            // loose record: the edge of the cell, metres, the same for every cell of the raster (N0456)
constexpr const char* GODOT_KEY_AUTUMN = "autumn";             // autumn record: the autumn colour of the individual of the patch (PHY_LEAF_AUTUMN, usecase:101646 colour the leaves in autumn), share 0 green at the start of autumn to 1 at its own leaf fall
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

}  // namespace ase::adp::godot

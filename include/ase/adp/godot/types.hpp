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

}  // namespace ase::adp::godot

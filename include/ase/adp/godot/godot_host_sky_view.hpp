#pragma once

/**
 * ASE GODOT ADAPTER - HOST SKY VIEW
 *
 * @file        godot_host_sky_view.hpp
 * @brief       The sky and the light of the embedded host as the last snapshot of the sky, stars and scene ports showed them
 * @description The copy GodotHostResourceManager keeps of every record of the sky port of ase-pl-sky: sun, moons,
 *              comets, planet, companion star, the halves of Aurora, belt, shower, stars, eclipses, the turn of
 *              the star sphere and the wind drift. One purpose of the dissolved godot_host_resource_manager.hpp
 *              (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that light, fog or draw the sky read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/types.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/**
 * The sky and the light of the scene as the last snapshot of the sky port showed them - a copy
 * (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3). ase-pl-sky computes all of it; the view sets its
 * directional light, its environment and its fog from these numbers and computes none of its own.
 * The sun and moon directions point TOWARDS their body in the world frame of the organ records (x east,
 * y up, z north); a directional light shines along its negative. Colours are shares 0..1. Every moon
 * record of the snapshot stands in one slot of `moon`, in the order the port carried them; `moons` says
 * how many - 0 while no moon stands in the sky.
 */
struct VivariumMoonView {
    double x = 0.0;           // unit direction towards the moon, east share
    double y = 0.0;           // unit direction towards the moon, upward share
    double z = 0.0;           // unit direction towards the moon, north share
    double r = 0.0;           // moonlight colour, red share
    double g = 0.0;           // moonlight colour, green share
    double b = 0.0;           // moonlight colour, blue share
    double intensity = 0.0;   // strength of its light as ase-pl-sky files it: CEL_MOON_CONTRIB for Amilox, 0 for Scorpii
    double size = 0.0;        // angular diameter of its disc, radians, as ase-ephemeris holds it
    double disc_r = 0.0;      // disc colour, red share: the moonlight darkened by the planet's shadow (usecase:101276)
    double disc_g = 0.0;      // disc colour, green share: the moonlight darkened by the planet's shadow
    double disc_b = 0.0;      // disc colour, blue share: the moonlight darkened by the planet's shadow
};

/**
 * A visible comet as the comet record of the sky port carries it (usecase:101274 - track the comets):
 * the unit direction towards it and the unit direction its tail runs on the sky, both in the world
 * frame, its brightness and the angular length of its tail. ase-pl-sky turns the tail away from the sun
 * and files the record only while the comet is visible and above the horizon.
 */
struct VivariumCometView {
    double x = 0.0;           // unit direction towards the comet, east share
    double y = 0.0;           // unit direction towards the comet, upward share
    double z = 0.0;           // unit direction towards the comet, north share
    double intensity = 0.0;   // brightness of the comet, 0..1 (EPH_COM_BRIGHTNESS)
    double tail_x = 0.0;      // unit direction the tail runs on the sky, east share
    double tail_y = 0.0;      // unit direction the tail runs on the sky, upward share
    double tail_z = 0.0;      // unit direction the tail runs on the sky, north share
    double length = 0.0;      // angular length of the tail on the sky, radians
};

/**
 * Luca, the neighbour planet, as the planet record of the sky port carries it (usecase:101273 - track the
 * neighbour planet): the unit direction towards it in the world frame, its angular diameter and its
 * illuminated share. ase-pl-sky files the record only while the planet stands above the horizon.
 */
struct VivariumPlanetView {
    double x = 0.0;           // unit direction towards the planet, east share
    double y = 0.0;           // unit direction towards the planet, upward share
    double z = 0.0;           // unit direction towards the planet, north share
    double size = 0.0;        // angular diameter of its disc, radians, as ase-ephemeris holds it
    double phase = 0.0;       // illuminated share of the disc, 0 new to 1 full
};

/**
 * Arcturus, the companion star, as the companion record of the sky port carries it (usecase:101272 - track
 * the companion star): the unit direction towards it in the world frame, its angular diameter and the
 * strength of its nebula glow. ase-pl-sky files the record only while it stands above the horizon.
 */
struct VivariumCompanionView {
    double x = 0.0;           // unit direction towards the companion star, east share
    double y = 0.0;           // unit direction towards the companion star, upward share
    double z = 0.0;           // unit direction towards the companion star, north share
    double size = 0.0;        // angular diameter of its disc, radians, as ase-ephemeris holds it
    double intensity = 0.0;   // strength of its nebula glow, 0..1
};

/**
 * A half of the split moon Aurora as a fragment record of the sky port carries it (usecase:101384 - place sun
 * and moon spatially): the unit direction towards it in the world frame, its angular diameter and its
 * brightness. ase-pl-sky files both halves while Aurora stands above the horizon.
 */
struct VivariumFragmentView {
    double x = 0.0;           // unit direction towards the moon half, east share
    double y = 0.0;           // unit direction towards the moon half, upward share
    double z = 0.0;           // unit direction towards the moon half, north share
    double size = 0.0;        // angular diameter of its disc, radians
    double intensity = 0.0;   // brightness of the half, 0..1
};

/**
 * The asteroid belt as the belt record of the sky port carries it (usecase:2758 - generated asteroids): the
 * unit direction towards the centre of its arc, the span of the arc along the orbit, the glitter of its grains
 * and their density. ase-pl-sky files the record only while the belt shines above the horizon.
 */
struct VivariumBeltView {
    double x = 0.0;           // unit direction towards the centre of the arc, east share
    double y = 0.0;           // unit direction towards the centre of the arc, upward share
    double z = 0.0;           // unit direction towards the centre of the arc, north share
    double span = 0.0;        // angle the arc spans along the orbit, radians
    double intensity = 0.0;   // glitter of the grains, 0..1
    double density = 0.0;     // share of the grains that show, 0..1
};

/**
 * The meteor shower as the shower record of the sky port carries it (usecase:2760 - show meteorite in the
 * nightsky): the unit direction towards its radiant, its intensity and the meteors it throws per hour.
 * ase-pl-sky files the record only while the shower throws and its radiant is above the horizon.
 */
struct VivariumShowerView {
    double x = 0.0;           // unit direction towards the radiant, east share
    double y = 0.0;           // unit direction towards the radiant, upward share
    double z = 0.0;           // unit direction towards the radiant, north share
    double intensity = 0.0;   // strength of the shower, 0..1
    double rate = 0.0;        // meteors per hour
};

/**
 * A visible star as the star record of the stars port carries it (usecase:5606 - observe the sky): its
 * place on the celestial sphere, its colour and the size it is drawn with. ase-pl-sky files the set when
 * it computes it anew (a sidereal step or a change of the brightness); the place does not move with the
 * clock, the turn record of the scene port turns the whole sphere into the world. The place is held as a
 * unit direction, normalised when the record is read.
 */
struct VivariumStarView {
    double x = 0.0;       // unit direction on the celestial sphere, first equatorial axis
    double y = 0.0;       // unit direction on the celestial sphere, towards the celestial pole
    double z = 0.0;       // unit direction on the celestial sphere, second equatorial axis
    double r = 0.0;       // star colour, red share 0..1
    double g = 0.0;       // star colour, green share 0..1
    double b = 0.0;       // star colour, blue share 0..1
    double point = 0.0;   // rendered size of the star point, pixels
};

/**
 * The standing eclipse as the eclipse record of the sky port carries it (usecase:101276 - predict the
 * eclipses): its kind, the covered share of the eclipsed disc, the game hours to its maximum and how long
 * it lasts. ase-pl-sky files the record only while ase-ephemeris reports an eclipse active and of a kind.
 */
struct VivariumEclipseView {
    double kind = 0.0;           // code of the kind of eclipse, ase-ephemeris ECLIPSE_* (EPH_ECL_TYPE)
    double obscuration = 0.0;    // covered share of the eclipsed disc, 0..1
    double hours_to_max = 0.0;   // game hours until the maximum
    double duration = 0.0;       // game hours the eclipse lasts
};

/**
 * The predicted eclipses as the eclipse next record of the sky port carries them (usecase:101276 - predict
 * the eclipses): the game hours from the clock of the snapshot to the next solar and to the next lunar
 * eclipse, as ase-ephemeris (EphemerisEclNextSystem) computes them from the orbits of ase-celestial, and
 * to the next rise of Scorpii (EphemerisMoonRiseCalcSystem; usecase:101384 - place sun and moon spatially).
 */
struct VivariumEclipseNextView {
    double next_solar = 0.0;     // game hours until the next solar eclipse (EPH_ECL_NEXT_SOLAR)
    double next_lunar = 0.0;     // game hours until the next lunar eclipse (EPH_ECL_NEXT_LUNAR)
    double next_moonrise = 0.0;  // game hours until Scorpii next rises (EPH_MOO_NEXT_RISE)
};

struct VivariumSkyView {
    bool   known = false;          // false until a snapshot carried all four records of the sky port
    uint32_t eclipses = 0;         // eclipse records of the snapshot, at most GODOT_SKY_ECLIPSES_MAX
    VivariumEclipseView eclipse[GODOT_SKY_ECLIPSES_MAX] = {};  // the standing eclipse
    uint32_t eclipse_nexts = 0;    // eclipse next records of the snapshot, at most GODOT_SKY_ECLIPSE_NEXTS_MAX
    VivariumEclipseNextView eclipse_next[GODOT_SKY_ECLIPSE_NEXTS_MAX] = {};  // the predicted eclipses
    bool   turn_known = false;     // true while the scene port carried the turn record of the star sphere
    double turn_w = 1.0;           // quaternion that turns the celestial sphere into the world, scalar part
    double turn_x = 0.0;           // quaternion that turns the celestial sphere into the world, x part
    double turn_y = 0.0;           // quaternion that turns the celestial sphere into the world, y part
    double turn_z = 0.0;           // quaternion that turns the celestial sphere into the world, z part
    double set_sidereal = 0.0;     // turn record: sidereal angle the plugin computed its visible set at, radians
    double set_zenith = 0.0;       // turn record: zenith brightness the plugin computed its visible set at
    double set_moon = 0.0;         // turn record: moon share of the brightness the plugin computed its visible set at
    double star_sidereal = 0.0;    // the same marks as they stood when the held star set was read
    double star_zenith = 0.0;      // the same marks as they stood when the held star set was read
    double star_moon = 0.0;        // the same marks as they stood when the held star set was read
    uint64_t star_generation = 0;  // grows by one every time the star set was read anew; the starfield is laid anew then
    std::vector<VivariumStarView> star;  // the visible stars of the stars port, at most GODOT_SKY_STARS_MAX, in its order
    uint32_t showers = 0;          // shower records of the snapshot, at most GODOT_SKY_SHOWERS_MAX
    VivariumShowerView shower[GODOT_SKY_SHOWERS_MAX] = {};  // the meteor showers throwing above the horizon
    uint32_t belts = 0;            // belt records of the snapshot, at most GODOT_SKY_BELTS_MAX
    VivariumBeltView belt[GODOT_SKY_BELTS_MAX] = {};  // the asteroid belts shining above the horizon
    uint32_t fragments = 0;        // moon half records of the snapshot, at most GODOT_SKY_FRAGMENTS_MAX
    VivariumFragmentView fragment[GODOT_SKY_FRAGMENTS_MAX] = {};  // the halves of Aurora, in the order of the port
    uint32_t companions = 0;       // companion star records of the snapshot, at most GODOT_SKY_COMPANIONS_MAX
    VivariumCompanionView companion[GODOT_SKY_COMPANIONS_MAX] = {};  // the companion stars above the horizon
    uint32_t planets = 0;          // planet records of the snapshot, at most GODOT_SKY_PLANETS_MAX
    VivariumPlanetView planet[GODOT_SKY_PLANETS_MAX] = {};  // the planets above the horizon, in the order of the port
    uint32_t moons = 0;            // moon records of the snapshot, at most GODOT_SKY_MOONS_MAX
    VivariumMoonView moon[GODOT_SKY_MOONS_MAX] = {};  // the visible moons, in the order of the port
    uint32_t comets = 0;           // comet records of the snapshot, at most GODOT_SKY_COMETS_MAX
    VivariumCometView comet[GODOT_SKY_COMETS_MAX] = {};  // the visible comets, in the order of the port
    double sun_x = 0.0;            // unit direction towards the sun, east share
    double sun_y = 0.0;            // unit direction towards the sun, upward share (the sine of its elevation)
    double sun_z = 0.0;            // unit direction towards the sun, north share
    double sun_r = 0.0;            // sunlight colour, red share
    double sun_g = 0.0;            // sunlight colour, green share
    double sun_b = 0.0;            // sunlight colour, blue share
    double sun_intensity = 0.0;    // strength of the sunlight
    double sun_size = 0.0;         // angular diameter of the sun disc, radians (EPH_SUN_APPARENT_SIZE)
    double sun_disc_r = 0.0;       // colour of the sun disc, red share - the sunlight darkened by an eclipse (usecase:101276)
    double sun_disc_g = 0.0;       // colour of the sun disc, green share
    double sun_disc_b = 0.0;       // colour of the sun disc, blue share
    double ambient_r = 0.0;        // ambient light colour, red share
    double ambient_g = 0.0;        // ambient light colour, green share
    double ambient_b = 0.0;        // ambient light colour, blue share
    double ambient_intensity = 0.0; // strength of the ambient light
    double background_r = 0.0;     // colour of the sky behind the scene, red share
    double background_g = 0.0;     // colour of the sky behind the scene, green share
    double background_b = 0.0;     // colour of the sky behind the scene, blue share
    double fog_r = 0.0;            // fog colour, red share
    double fog_g = 0.0;            // fog colour, green share
    double fog_b = 0.0;            // fog colour, blue share
    double fog_near = 0.0;         // distance where the fog begins, metres
    double fog_far = 0.0;          // distance where the fog is full, metres
    double fog_shimmer = 0.0;      // shimmer of hot air, 0 none to 1 full (ATM_VIS_HEAT_HAZE; usecase:101515)
    bool   wind_known = false;     // true once the scene port carried the wind record; a snapshot without it keeps the last drift
    double drift_x = 0.0;          // wind record: east share of the drift haze and clouds move along, -1..1, the wind share folded in
    double drift_z = 0.0;          // wind record: north share of the drift, -1..1, the wind share folded in
    double wind = 0.0;             // wind record: share of the wind in the drift, 0 calm to 1 full
};

}  // namespace ase::adp::godot

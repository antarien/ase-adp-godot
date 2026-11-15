/**
 * ASE ADAPTER TESTS - EXPOSURE OF THE SCENE CAMERA WITHOUT GODOT
 *
 * @file        test_godot_scene_exposure.cpp
 * @design      DSGN_066
 * @brief       Pins the exposure the camera takes for the light of the sky port
 * @description Register D1721: by day the camera keeps the scene as lit, at night it lifts its exposure in part,
 *              as an eye does, and the night stays darker than the day; without light it takes the largest lift
 *              and never divides by zero (godot_scene_exposure.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101415 - hand sky light to app
 * @actor       cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-10-26
 * @modified    2026-11-08
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_scene_exposure.hpp>
#include <ase/adp/godot/types.hpp>

namespace ase::adp::godot {

TEST_CASE("D1721: the camera lifts its exposure at night as an eye does, by day it keeps the scene as lit") {
    // noon of run v46: sun 0.95, ambient 0.7 - no lift
    VivariumSkyView day;
    day.known = true;
    day.sun_intensity = 0.95;
    day.ambient_intensity = 0.7;
    day.ambient_r = 0.6;
    day.ambient_g = 0.7;
    day.ambient_b = 0.8;
    CHECK(scene_exposure(day) == doctest::Approx(1.0f));
    // night: sun light 0.05, ambient 0.02/0.02/0.06 at 0.15 (ATM_CONST_NIGHT_AMBIENT_*) - lifted, but darker than the day
    VivariumSkyView night;
    night.known = true;
    night.sun_intensity = 0.05;
    night.ambient_intensity = 0.15;
    night.ambient_r = 0.02;
    night.ambient_g = 0.02;
    night.ambient_b = 0.06;
    const float lift = scene_exposure(night);
    const double light = 0.05 + 0.15 * (0.02 + 0.02 + 0.06) / 3.0;
    CHECK(lift * lift == doctest::Approx(1.0 / light).epsilon(0.001));
    CHECK(lift > 1.0f);
    CHECK(lift < GODOT_EXPOSURE_LIFT_MAX);
    // the lifted night stays darker than the day
    CHECK(lift * light < 1.0);
    // no light, or a sky the port did not carry yet: the largest lift, never a division by zero
    VivariumSkyView dark = night;
    dark.sun_intensity = 0.0;
    dark.ambient_intensity = 0.0;
    CHECK(scene_exposure(dark) == doctest::Approx(GODOT_EXPOSURE_LIFT_MAX));
    CHECK(scene_exposure(VivariumSkyView{}) == doctest::Approx(GODOT_EXPOSURE_LIFT_MAX));
}

TEST_CASE("C0455: a moon in the sky is light of the scene, a moon record never carried is none") {
    VivariumSkyView night;
    night.known = true;
    night.sun_intensity = 0.05;
    night.ambient_intensity = 0.15;
    night.ambient_r = 0.02;
    night.ambient_g = 0.02;
    night.ambient_b = 0.06;
    const float moonless = scene_exposure(night);
    // Amilox high up: CEL_MOON_CONTRIB 0.08, Scorpii beside it lights nothing - less lift than the moonless night
    VivariumSkyView moonlit = night;
    moonlit.moons = 2u;
    moonlit.moon[0].intensity = 0.0;
    moonlit.moon[1].intensity = 0.08;
    const float lift = scene_exposure(moonlit);
    const double light = 0.05 + 0.08 + 0.15 * (0.02 + 0.02 + 0.06) / 3.0;
    CHECK(lift * lift == doctest::Approx(1.0 / light).epsilon(0.001));
    CHECK(lift < moonless);
    // a strength in a slot the port did not fill counts for nothing
    VivariumSkyView stale = night;
    stale.moon[0].intensity = 0.08;
    CHECK(scene_exposure(stale) == doctest::Approx(moonless));
}

TEST_CASE("D2258: at dusk the sky behind the scene is light the eye sees, the lift never blows it white") {
    // PEX 24.85 h of probe 20261108-e1e4-fix: sun below the horizon, ambient 0.087, sky 0.178/0.098/0.086
    VivariumSkyView dusk;
    dusk.known = true;
    dusk.sun_intensity = 0.0;
    dusk.ambient_intensity = 0.087;
    dusk.ambient_r = 0.1;
    dusk.ambient_g = 0.1;
    dusk.ambient_b = 0.1;
    dusk.background_r = 0.178;
    dusk.background_g = 0.098;
    dusk.background_b = 0.086;
    const float lift = scene_exposure(dusk);
    CHECK(lift * lift == doctest::Approx(1.0 / 0.178).epsilon(0.001));
    CHECK(lift > 1.0f);
    // the brightest channel of the lifted sky stays below white
    CHECK(lift * 0.178 < 1.0);
    // a black sky leaves the lift to the light of the scene alone
    VivariumSkyView black = dusk;
    black.background_r = 0.0;
    black.background_g = 0.0;
    black.background_b = 0.0;
    const double light = 0.087 * 0.1;
    const float scene_only = scene_exposure(black);
    CHECK(scene_only * scene_only == doctest::Approx(1.0 / light).epsilon(0.001));
    CHECK(scene_only > lift);
}

}  // namespace ase::adp::godot

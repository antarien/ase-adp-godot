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
 * @modified    2026-10-26
 * @version     00.00.01.00000
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

}  // namespace ase::adp::godot

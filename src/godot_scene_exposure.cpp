/**
 * ASE GODOT ADAPTER - EXPOSURE OF THE SCENE CAMERA
 *
 * @file        godot_scene_exposure.cpp
 * @brief       The exposure multiplier of the scene camera for the light the sky port carries
 * @description See godot_scene_exposure.hpp (register D1721).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101415 - hand sky light to app
 * @actor       cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-10-26
 * @modified    2026-11-08
 * @version     00.00.03.00000
 */

#include <ase/adp/godot/godot_scene_exposure.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

namespace ase::adp::godot {

float scene_exposure(const VivariumSkyView& sky) {
    const double ambient = sky.ambient_intensity * (sky.ambient_r + sky.ambient_g + sky.ambient_b) / 3.0;
    // every moon the port carries lights the scene beside the sun
    double moon = 0.0;
    for (uint32_t m = 0u; m < sky.moons && m < GODOT_SKY_MOONS_MAX; ++m) {
        moon += sky.moon[m].intensity;
    }
    // the sky behind the scene is light the eye sees too: at dusk it is the brightest thing in view (D2258)
    const double sky_rg = sky.background_r > sky.background_g ? sky.background_r : sky.background_g;
    const double sky_peak = sky_rg > sky.background_b ? sky_rg : sky.background_b;
    const double lit = sky.sun_intensity + moon + ambient;
    const double light = lit > sky_peak ? lit : sky_peak;
    if (!sky.known || !(light > 0.0)) {
        return GODOT_EXPOSURE_LIFT_MAX;
    }
    // the eye adapts only in part: the lift is the square root of the ratio, so the night stays darker
    const float lift = math::sqrt(static_cast<float>(static_cast<double>(GODOT_EXPOSURE_DAY_LIGHT) / light));
    if (lift < 1.0f) {
        return 1.0f;
    }
    return lift > GODOT_EXPOSURE_LIFT_MAX ? GODOT_EXPOSURE_LIFT_MAX : lift;
}

}  // namespace ase::adp::godot

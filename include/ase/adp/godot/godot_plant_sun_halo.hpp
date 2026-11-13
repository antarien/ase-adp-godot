#pragma once

/**
 * ASE GODOT ADAPTER - THE HALO AROUND THE SUN IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_sun_halo.hpp
 * @brief       The halo the mockup draws around the sun, stood in the sky of the care view's 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N03. The mockup's drawSky lays a radial gradient
 *              around the sun before its disc (m.html 417 to 418): from SKY_SUN_HALO at sky_clear_sun_halo of its
 *              opacity at sky_sun_halo_inner mockup units out to SKY_SUN_HALO_EDGE at sky_sun_halo_outer, under
 *              an overcast sky at sky_overcast_sun_halo. SkyOvercast (godot_hud_sky_overcast.hpp) hands these
 *              numbers as the shade; this unit stands the halo in the sky of the 3D scene: a square facing the
 *              camera, centred on the sun a little behind its disc, whose colour runs outward like the canvas'
 *              gradient. Its radii are mockup units, so its size follows the HUD frame's pixels per unit and the
 *              camera's view, the way the cloud banks are sized (godot_plant_clouds.hpp). The disc covers it
 *              where both stand; the cloud banks stand nearer and cover both. Nothing of the sky is computed
 *              here: the direction and the size of the sun come with the sky port's record.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.02.00000
 */

#include <ase/adp/godot/godot_plant_environment.hpp>  // SkyOvercastShade, the halo's numbers

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

// The halo stands a little behind the sun disc along the sun direction, so the disc covers it where both stand
// and it is drawn before a disc that is itself see-through (behind the clouds). It stands here and not in
// types.hpp, which nearly every unit of the adapter includes.
constexpr const char* GODOT_PLANT_SUN_HALO = "PlantSunHalo";        // the halo around the sun, a square facing the camera
constexpr float GODOT_SUN_HALO_BEHIND = 2.0f;                       // sun disc radii the halo stands behind the disc
constexpr const char* GODOT_SUN_HALO_SHADER_FILE = "res://shaders/plant_sun_halo.gdshader";  // the look of the halo, data: an edit compiles no unit
constexpr const char* GODOT_SUN_HALO_COLOR = "halo_color";          // uniform: the colour at the inner edge
constexpr const char* GODOT_SUN_HALO_EDGE = "halo_edge";            // uniform: the colour at the outer edge
constexpr const char* GODOT_SUN_HALO_STRENGTH = "halo_strength";    // uniform: factor on the inner colour's alpha
constexpr const char* GODOT_SUN_HALO_INNER = "halo_inner";          // uniform: inner radius as a share of the outer one

/**
 * Adds the halo (GODOT_PLANT_SUN_HALO) under the scene root: a square of one metre in the material of
 * GODOT_SUN_HALO_SHADER_FILE that casts no shadow, hidden until light_plant_sun_halo stands it.
 * build_plant_sky calls it once.
 */
void build_plant_sun_halo(::godot::Node3D* scene);

/**
 * Stands the halo from `centre`, the middle of the sky dome the sun disc stands on (light_plant_sky), along
 * `toward` (world frame), GODOT_SUN_HALO_BEHIND sun disc radii behind the disc of the angular size
 * `angular_size` (radians) that stands at GODOT_SKY_DISC_DISTANCE, facing the camera. Its square is twice the
 * shade's sun_halo_outer mockup units wide: sun_halo_outer times the shade's frame_pixels is its radius on the
 * screen, and a pixel at the halo's depth along the view spans 2·depth·tan(fov/2) / viewport height metres. Its
 * colours, its opacity and its inner radius come from the shade. Hidden while the sun is not `shown`, the shade
 * draws no halo or carries no frame pixels yet (before the HUD stands), or the sun stands behind the camera.
 * light_plant_sky calls it every frame.
 */
void light_plant_sun_halo(::godot::Node3D* scene, bool shown, const ::godot::Vector3& toward, float angular_size,
                          const SkyOvercastShade& overcast, const ::godot::Vector3& centre);

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - THE DISCS OF THE SKY DOME OF THE CARE VIEW
 *
 * @file        godot_plant_sky_disc.hpp
 * @brief       The middle of the sky dome and the discs of the sky stood on it: sun, moons, planets and every glow
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11: every body of the sky stands as a disc - a sphere in an
 *              unshaded material outside the fog that casts no shadow - GODOT_SKY_DISC_DISTANCE from the middle
 *              of the sky dome along the direction of its record, so it keeps its place in the sky however the
 *              camera moves. The starfield (godot_plant_stars.hpp) and the sun's halo (godot_plant_sun_halo.hpp)
 *              stand on the same middle.
 *
 *              THE MIDDLE OF THE SKY DOME LIES WHERE ITS HORIZON MEETS THE VISIBLE GROUND (B0262). ase-pl-sky
 *              shows a body above its horizon plane (the stars: SkyCelStarVisSystem, toward.y > 0). A dome
 *              centred on the camera's eye puts that horizon at eye height; the ground's visible far edge lies
 *              lower in the picture, and the sky between the two stood empty above the ridges. sky_dome_centre
 *              lowers the middle so the dome's horizon ring lies on the ray from the camera through that edge -
 *              the ray the ridges stand on (godot_plant_horizon_ridge.hpp) - and the sky shows from where the
 *              terrain ends in the picture; what stands behind the ridges they cover. The places of the bodies
 *              on the celestial sphere stay the records'; nothing of the sky is computed here.
 *
 *              A moon disc carries the mockup's crescent (GODOT_MOON_SHADER_FILE): a shadow disc that lets the
 *              sky through, after sky_moon_shadow_* of the shade, while the moon record carries no phase (bridge
 *              gap). Every body that shines by its own glow carries that glow as opacity, never as a darker grey
 *              (glow_through), so no faint planet stands as a black disc on the grey of the clouds (row N02).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101384 - place sun and moon spatially
 * @actor       cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_environment.hpp>  // SkyOvercastShade, the crescent of a moon disc

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

// The look of a moon disc (shaders/plant_moon.gdshader, loaded by godot_plant_shader.hpp) and its uniforms: the
// disc's colour and size and the shadow disc of the crescent. The look is data: an edit of the file compiles no unit.
constexpr const char* GODOT_MOON_SHADER_FILE = "res://shaders/plant_moon.gdshader";  // the look of a moon disc
constexpr const char* GODOT_MOON_COLOR = "moon_color";              // uniform: the disc colour, its alpha the disc's opacity
constexpr const char* GODOT_MOON_RADIUS = "disc_radius";            // uniform: radius of the disc sphere, metres
constexpr const char* GODOT_MOON_SHADOW_SHIFT = "shadow_shift";     // uniform: middle of the shadow disc right of and above the moon's, moon radii
constexpr const char* GODOT_MOON_SHADOW_RADIUS = "shadow_radius";   // uniform: radius of the shadow disc, moon radii
constexpr const char* GODOT_MOON_SHADOW_COVER = "shadow_cover";     // uniform: share of the moon the shadow covers with the sky

/**
 * The middle of the sky dome: the camera `eye`, lowered by GODOT_SKY_DISC_DISTANCE · (eye height − edge height) /
 * the distance of `edge` along the ground ahead of the camera, so the dome's horizon ring - the directions of
 * height 0 at GODOT_SKY_DISC_DISTANCE - lies on the ray from the camera through the ground's visible far edge
 * `edge` (plant_ground_far_edge). Without an edge, or with the edge not ahead of the camera along the ground, the
 * middle is the camera. light_plant_sky reckons it once a frame.
 */
::godot::Vector3 sky_dome_centre(const ::godot::Camera3D* eye, const ::godot::Vector3* edge);

/**
 * The colour of a body that shines only by its own glow - a planet, a companion star, a half of Aurora, a comet,
 * a grain of the belt, a point of a meteor streak: white, its glow and the share `through` the clouds let pass
 * (SkyOvercastShade::stars, 1 under an open sky) as its opacity. A FAINT BODY FADES, IT NEVER DARKENS: the glow
 * of the sky lies in front of every body, so none ever stands darker than the sky behind it. On the black night
 * sky the white at that opacity is the grey of the glow; on the grey of an overcast sky or the blue of the day
 * it lets the sky show through instead of standing as a black disc on it (row N02).
 */
::godot::Color glow_through(float glow, float through);

/** An unshaded material outside the fog: what glows in the sky, lit by nothing in the scene. */
::godot::Ref<::godot::StandardMaterial3D> sky_glow();

/**
 * The material of a moon disc (GODOT_MOON_SHADER_FILE): unshaded, outside the fog, blended by its alpha, with
 * the shadow disc of the crescent the shade lays on it (row N02, m.html drawSky 416). Every disc has its own
 * material, so each carries its own colour and shadow; the shader they share is loaded once.
 */
::godot::Ref<::godot::ShaderMaterial> moon_glow();

/**
 * A disc of the sky under `scene`: a sphere of `radius` metres in the material `look` that casts no shadow,
 * hidden until light_plant_sky stands it along the direction of its record.
 */
void add_sky_disc(::godot::Node3D* scene, const ::godot::String& name, float radius,
                  const ::godot::Ref<::godot::Material>& look);

/**
 * Places the disc `name` along `toward` (world frame) at GODOT_SKY_DISC_DISTANCE from `centre`, the middle of the
 * sky dome (sky_dome_centre), sized to the angular diameter `angular_size` (radians) ase-ephemeris holds for the
 * body: a sphere that spans it at that distance has the radius GODOT_SKY_DISC_DISTANCE * tan(angular_size / 2).
 * The size comes with the record of ase-pl-sky. Not `shown`, the disc is hidden. Returns the sphere of a shown
 * disc, its radius in `radius`; null when it is hidden or there is no disc or no sphere.
 */
::godot::Ref<::godot::SphereMesh> place_sky_disc(::godot::Node3D* scene, const ::godot::String& name, bool shown,
                                                 const ::godot::Vector3& toward, float angular_size, float& radius,
                                                 const ::godot::Vector3& centre);

/**
 * Stands the disc `name` as place_sky_disc does, in the colour `rgb` (its alpha the share of the disc the sky
 * lets through).
 */
void stand_sky_disc(::godot::Node3D* scene, const ::godot::String& name, bool shown, const ::godot::Vector3& toward,
                    const ::godot::Color& rgb, float angular_size, const ::godot::Vector3& centre);

/**
 * Stands the moon disc `name` as place_sky_disc does, in the colour `rgb` (its alpha the disc's opacity), with
 * the shadow disc of the crescent after `overcast` (moon_shadow_*, GODOT_MOON_SHADER_FILE). BRIDGE GAP: the
 * moon record carries no phase (VivariumMoonView), so the shadow stands where the mockup draws it.
 */
void stand_moon_disc(::godot::Node3D* scene, const ::godot::String& name, bool shown, const ::godot::Vector3& toward,
                     const ::godot::Color& rgb, float angular_size, const SkyOvercastShade& overcast,
                     const ::godot::Vector3& centre);

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - THE DISCS OF THE SKY DOME OF THE CARE VIEW
 *
 * @file        godot_plant_sky_disc.cpp
 * @brief       The middle of the sky dome and the discs of the sky stood on it: sun, moons, planets and every glow
 * @description See godot_plant_sky_disc.hpp. The middle of the dome is reckoned from the camera and the ground's
 *              visible far edge alone; a disc stands GODOT_SKY_DISC_DISTANCE from it along the direction of its
 *              record and is sized to the angular diameter of that record.
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

#include <ase/adp/godot/godot_plant_sky_disc.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

::godot::Vector3 sky_dome_centre(const ::godot::Camera3D* eye, const ::godot::Vector3* edge) {
    const ::godot::Transform3D place = eye->get_transform();
    // the camera looks along its -Z; the edge's distance is measured along that view flattened onto the ground,
    // the way the ridges measure it (light_plant_horizon_ridge)
    const ::godot::Vector3 forward = -place.basis.orthonormalized().get_column(2);
    const ::godot::Vector3 ahead(forward.x, 0.0f, forward.z);
    const float ahead_length = ahead.length();
    if (edge == nullptr || ahead_length <= 0.0f) {
        return place.origin;
    }
    const float edge_ahead = (*edge - place.origin).dot(ahead) / ahead_length;
    if (edge_ahead <= 0.0f) {
        return place.origin;
    }
    // at the disc distance along the ground the ray through the edge stands this far below the eye; the ring of
    // height 0 around the lowered middle lies on it
    const float drop = GODOT_SKY_DISC_DISTANCE * (place.origin.y - edge->y) / edge_ahead;
    return place.origin - ::godot::Vector3(0.0f, drop, 0.0f);
}

::godot::Color glow_through(float glow, float through) {
    return ::godot::Color(1.0f, 1.0f, 1.0f, glow * through);
}

::godot::Ref<::godot::StandardMaterial3D> sky_glow() {
    ::godot::Ref<::godot::StandardMaterial3D> glow;
    glow.instantiate();
    glow->set_shading_mode(::godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
    glow->set_flag(::godot::BaseMaterial3D::FLAG_DISABLE_FOG, true);
    return glow;
}

::godot::Ref<::godot::ShaderMaterial> moon_glow() {
    ::godot::Ref<::godot::ShaderMaterial> glow;
    glow.instantiate();
    glow->set_shader(load_plant_shader(GODOT_MOON_SHADER_FILE));
    return glow;
}

void add_sky_disc(::godot::Node3D* scene, const ::godot::String& name, float radius,
                  const ::godot::Ref<::godot::Material>& look) {
    ::godot::MeshInstance3D* disc = memnew(::godot::MeshInstance3D);
    disc->set_name(name);
    ::godot::Ref<::godot::SphereMesh> sphere;
    sphere.instantiate();
    sphere->set_radius(radius);
    sphere->set_height(radius * 2.0f);
    sphere->set_radial_segments(GODOT_SKY_DISC_SEGMENTS);
    sphere->set_rings(GODOT_SKY_DISC_RINGS);
    sphere->set_material(look);
    disc->set_mesh(sphere);
    disc->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    disc->set_visible(false);
    scene->add_child(disc);
}

::godot::Ref<::godot::SphereMesh> place_sky_disc(::godot::Node3D* scene, const ::godot::String& name, bool shown,
                                                 const ::godot::Vector3& toward, float angular_size, float& radius,
                                                 const ::godot::Vector3& centre) {
    ::godot::MeshInstance3D* disc =
        ::godot::Object::cast_to<::godot::MeshInstance3D>(scene->get_node_or_null(::godot::NodePath(name)));
    if (disc == nullptr) {
        return ::godot::Ref<::godot::SphereMesh>();
    }
    disc->set_visible(shown);
    if (!shown) {
        return ::godot::Ref<::godot::SphereMesh>();
    }
    disc->set_position(centre + toward.normalized() * GODOT_SKY_DISC_DISTANCE);
    const ::godot::Ref<::godot::SphereMesh> sphere = disc->get_mesh();
    if (sphere.is_null()) {
        return sphere;
    }
    radius = GODOT_SKY_DISC_DISTANCE * ase::math::tan(angular_size * 0.5f);
    sphere->set_radius(radius);
    sphere->set_height(radius * 2.0f);
    return sphere;
}

void stand_sky_disc(::godot::Node3D* scene, const ::godot::String& name, bool shown, const ::godot::Vector3& toward,
                    const ::godot::Color& rgb, float angular_size, const ::godot::Vector3& centre) {
    float radius = 0.0f;
    const ::godot::Ref<::godot::SphereMesh> sphere =
        place_sky_disc(scene, name, shown, toward, angular_size, radius, centre);
    if (sphere.is_null()) {
        return;
    }
    const ::godot::Ref<::godot::StandardMaterial3D> glow = sphere->get_material();
    if (glow.is_valid()) {
        // a disc the sky lets only part of through blends by its alpha, so the grey behind it shows through
        // (row N02: the moons behind the clouds); a whole disc stays on the opaque path
        glow->set_transparency(rgb.a < 1.0f ? ::godot::BaseMaterial3D::TRANSPARENCY_ALPHA
                                            : ::godot::BaseMaterial3D::TRANSPARENCY_DISABLED);
        glow->set_albedo(rgb);
    }
}

void stand_moon_disc(::godot::Node3D* scene, const ::godot::String& name, bool shown, const ::godot::Vector3& toward,
                     const ::godot::Color& rgb, float angular_size, const SkyOvercastShade& overcast,
                     const ::godot::Vector3& centre) {
    float radius = 0.0f;
    const ::godot::Ref<::godot::SphereMesh> sphere =
        place_sky_disc(scene, name, shown, toward, angular_size, radius, centre);
    if (sphere.is_null()) {
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> glow = sphere->get_material();
    if (glow.is_null()) {
        return;
    }
    glow->set_shader_parameter(GODOT_MOON_COLOR, rgb);
    glow->set_shader_parameter(GODOT_MOON_RADIUS, radius);
    glow->set_shader_parameter(GODOT_MOON_SHADOW_SHIFT, ::godot::Vector2(overcast.moon_shadow_x, overcast.moon_shadow_y));
    glow->set_shader_parameter(GODOT_MOON_SHADOW_RADIUS, overcast.moon_shadow_radius);
    glow->set_shader_parameter(GODOT_MOON_SHADOW_COVER, overcast.moon_shadow_cover);
}

}  // namespace ase::adp::godot

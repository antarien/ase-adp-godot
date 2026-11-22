/**
 * ASE GODOT ADAPTER - THE STARFIELD IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_stars.cpp
 * @brief       The starfield of the care view's 3D scene: the visible stars of ase-pl-sky, turned with the sky
 * @description See godot_plant_stars.hpp. The field is one multimesh centred on the middle of the sky dome
 *              (sky_dome_centre, godot_plant_sky_disc.hpp): the camera, lowered so the dome's horizon ring lies on
 *              the ray to the ground's visible far edge. Each instance stands at the place of its star on the
 *              celestial sphere times GODOT_SKY_DISC_DISTANCE, so a star keeps its place in the sky however
 *              the camera moves, and the node is turned by the quaternion of the turn record every frame; the
 *              instances are laid only when the star set or the pixel size or the overcast opacity changed.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-08
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/godot_plant_stars.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

void build_plant_stars(::godot::Node3D* scene) {
    // an unshaded material outside the fog, lit by nothing in the scene, its colour and its alpha from each
    // instance, blended by that alpha: behind the clouds a star fades into the grey, on a clear night it
    // flickers (row N02, GODOT_STAR_SHADER_FILE)
    ::godot::Ref<::godot::ShaderMaterial> glow;
    glow.instantiate();
    glow->set_shader(load_plant_shader(GODOT_STAR_SHADER_FILE));
    ::godot::Ref<::godot::SphereMesh> point;
    point.instantiate();
    point->set_radius(0.5f);
    point->set_height(1.0f);
    point->set_radial_segments(GODOT_STAR_SEGMENTS);
    point->set_rings(GODOT_STAR_RINGS);
    point->set_material(glow);
    ::godot::Ref<::godot::MultiMesh> field;
    field.instantiate();
    field->set_transform_format(::godot::MultiMesh::TRANSFORM_3D);
    field->set_use_colors(true);
    field->set_use_custom_data(true);
    field->set_mesh(point);
    field->set_instance_count(static_cast<int32_t>(GODOT_SKY_STARS_MAX));
    field->set_visible_instance_count(0);
    ::godot::MultiMeshInstance3D* stars = memnew(::godot::MultiMeshInstance3D);
    stars->set_name(::godot::String(GODOT_PLANT_STARS));
    stars->set_multimesh(field);
    stars->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    scene->add_child(stars);
}

void light_plant_stars(::godot::Node3D* scene, const VivariumSkyView& sky, const SkyOvercastShade& overcast,
                       const ::godot::Vector3& centre) {
    ::godot::MultiMeshInstance3D* stars =
        ::godot::Object::cast_to<::godot::MultiMeshInstance3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_STARS)));
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (stars == nullptr || eye == nullptr) {
        return;
    }
    const ::godot::Ref<::godot::MultiMesh> field = stars->get_multimesh();
    if (field.is_null()) {
        return;
    }
    // the field stands on the middle of the sky dome every frame, with or without a turn record: the frame line
    // (godot_vivarium_scene.cpp, scene_counts) and the moonrise (godot_vivarium_moonrise.cpp) read that middle
    // from this node, so they count what the picture draws
    stars->set_position(centre);
    const ::godot::StringName generation_key(GODOT_STAR_META_GENERATION);
    const ::godot::StringName pixel_key(GODOT_STAR_META_PIXEL);
    const ::godot::StringName opacity_key(GODOT_STAR_META_OPACITY);
    // without the turn record the sphere has no place in the world: no star shows, and the next set that
    // comes with a turn is laid anew
    if (!sky.turn_known) {
        field->set_visible_instance_count(0);
        stars->set_meta(generation_key, static_cast<int64_t>(0));
        return;
    }
    // THE MOTION OF THE SKY IS THE TURN OF THE NODE. ase-pl-sky writes the quaternion w, x, y, z of
    // R_y(bearing) · P · R_x(latitude) · R_y(-sidereal) on the turn record (SkyCelStarWrldSystem); Godot's
    // Quaternion takes x, y, z, w and turns a vector by the same Hamilton product q · v · q*.
    const ::godot::Quaternion turn(static_cast<float>(sky.turn_x), static_cast<float>(sky.turn_y),
                                   static_cast<float>(sky.turn_z), static_cast<float>(sky.turn_w));
    stars->set_transform(::godot::Transform3D(::godot::Basis(turn.normalized()), centre));
    // the flicker of a clear night runs on the card: the shade's numbers go to the material every frame, the
    // instances keep their size and place as they were laid (row N02, m.html 415)
    const ::godot::Ref<::godot::SphereMesh> point = field->get_mesh();
    const ::godot::Ref<::godot::ShaderMaterial> glow =
        point.is_valid() ? ::godot::Ref<::godot::ShaderMaterial>(point->get_material())
                         : ::godot::Ref<::godot::ShaderMaterial>();
    if (glow.is_valid()) {
        glow->set_shader_parameter(GODOT_STAR_TWINKLE, overcast.twinkle);
        glow->set_shader_parameter(GODOT_STAR_TWINKLE_BASE, overcast.twinkle_base);
        glow->set_shader_parameter(GODOT_STAR_TWINKLE_SWING, overcast.twinkle_swing);
        glow->set_shader_parameter(GODOT_STAR_TWINKLE_RATE, overcast.twinkle_rate);
        glow->set_shader_parameter(GODOT_STAR_TWINKLE_RATE_SPAN, overcast.twinkle_rate_span);
        glow->set_shader_parameter(GODOT_STAR_TWINKLE_PHASE_STEP, overcast.twinkle_phase_step);
        glow->set_shader_parameter(GODOT_STAR_TWINKLE_STEADY, overcast.twinkle_steady);
    }
    float viewport_height = 0.0f;
    const ::godot::Viewport* view = eye->get_viewport();
    if (view != nullptr) {
        viewport_height = view->get_visible_rect().size.y;
    }
    // A pixel at the disc distance spans 2·d·tan(fov/2) / viewport height metres - the camera's vertical
    // field of view and the height of the viewport it draws into; that is view geometry, the size of the
    // star is the record's.
    const float half_fov = static_cast<float>(eye->get_fov()) * 0.5f * ase::math::PI / 180.0f;
    const float metres_per_pixel = viewport_height > 0.0f
        ? 2.0f * GODOT_SKY_DISC_DISTANCE * ase::math::tan(half_fov) / viewport_height
        : 0.0f;
    // THE INSTANCES ARE LAID ONLY WHEN THE SET, THE PIXEL SIZE OR THE OPACITY CHANGED. A new set comes after a
    // sidereal step or a change of the brightness, a new opacity with a change of the weather; between those
    // the turn alone moves the sky, one transform a frame instead of one per star.
    const int64_t laid_generation = stars->has_meta(generation_key)
        ? static_cast<int64_t>(stars->get_meta(generation_key))
        : static_cast<int64_t>(0);
    const double laid_pixel = stars->has_meta(pixel_key) ? static_cast<double>(stars->get_meta(pixel_key)) : -1.0;
    const double laid_opacity = stars->has_meta(opacity_key) ? static_cast<double>(stars->get_meta(opacity_key)) : -1.0;
    if (laid_generation == static_cast<int64_t>(sky.star_generation) && laid_pixel == static_cast<double>(metres_per_pixel) &&
        laid_opacity == static_cast<double>(overcast.stars)) {
        return;
    }
    // A star point is a sphere of GODOT_STAR_SEGMENTS sides and GODOT_STAR_RINGS rings, and a pixel is
    // drawn only where the polygon covers its centre. A polygon of GODOT_STAR_POINT_MIN pixels across
    // misses every centre when it falls between them, so the faint stars (1 px from ase-pl-sky) vanished.
    // The smallest size that covers a centre wherever it falls is the diagonal of that pixel over the
    // ratio of the polygon's inner circle to its outer one, taken at the coarser of its two steps.
    const float side_half_step = ase::math::PI / static_cast<float>(GODOT_STAR_SEGMENTS);
    const float ring_half_step = ase::math::PI / static_cast<float>(2 * (GODOT_STAR_RINGS + 1));
    const float half_step = side_half_step > ring_half_step ? side_half_step : ring_half_step;
    const float cover_min = GODOT_STAR_POINT_MIN * ase::math::sqrt(2.0f) / ase::math::cos(half_step);
    const uint32_t held = static_cast<uint32_t>(sky.star.size());
    const uint32_t shown = held < GODOT_SKY_STARS_MAX ? held : GODOT_SKY_STARS_MAX;
    for (uint32_t i = 0u; i < shown; ++i) {
        const VivariumStarView& star = sky.star[i];
        const ::godot::Vector3 place =
            ::godot::Vector3(static_cast<float>(star.x), static_cast<float>(star.y), static_cast<float>(star.z)).normalized();
        const float pixels = static_cast<float>(star.point) > cover_min ? static_cast<float>(star.point) : cover_min;
        const float width = pixels * metres_per_pixel;
        field->set_instance_transform(static_cast<int32_t>(i),
                                      ::godot::Transform3D(::godot::Basis().scaled(::godot::Vector3(width, width, width)),
                                                           place * GODOT_SKY_DISC_DISTANCE));
        // the star's colour as the port carried it, at the opacity the sky lets through (1 under an open sky)
        field->set_instance_color(static_cast<int32_t>(i), ::godot::Color(static_cast<float>(star.r), static_cast<float>(star.g),
                                                                          static_cast<float>(star.b), overcast.stars));
        // what the flicker reads of the star: its size in pixels as the port carried it, its place in the set
        field->set_instance_custom_data(static_cast<int32_t>(i),
                                        ::godot::Color(static_cast<float>(star.point), static_cast<float>(i), 0.0f, 0.0f));
    }
    field->set_visible_instance_count(static_cast<int32_t>(shown));
    stars->set_meta(generation_key, static_cast<int64_t>(sky.star_generation));
    stars->set_meta(pixel_key, static_cast<double>(metres_per_pixel));
    stars->set_meta(opacity_key, static_cast<double>(overcast.stars));
}

}  // namespace ase::adp::godot

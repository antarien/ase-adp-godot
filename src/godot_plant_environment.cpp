/**
 * ASE GODOT ADAPTER - THE ENVIRONMENT OF THE CARE VIEW
 *
 * @file        godot_plant_environment.cpp
 * @brief       Sky, ambient light, fog and exposure of the care view's 3D scene
 * @description See godot_plant_environment.hpp. The sky's radiance lights nothing: the ambient light is the
 *              sky port's colour and reflections are off, as they were while the sky was a background
 *              colour, so the radiance map Godot keeps for every Sky stays at its smallest size.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_environment.hpp>
#include <ase/adp/godot/godot_plant_haze.hpp>
#include <ase/adp/godot/godot_plant_look_shader.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/godot_scene_exposure.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/camera_attributes.hpp>
#include <godot_cpp/classes/camera_attributes_practical.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

void build_plant_environment(::godot::Node3D* scene) {
    ::godot::WorldEnvironment* world = memnew(::godot::WorldEnvironment);
    world->set_name(GODOT_PLANT_ENVIRONMENT);
    scene->add_child(world);

    ::godot::Ref<::godot::ShaderMaterial> sky_look;
    sky_look.instantiate();
    sky_look->set_shader(load_plant_shader(GODOT_SKY_BACKGROUND_SHADER_FILE));
    ::godot::Ref<::godot::Sky> dome;
    dome.instantiate();
    dome->set_material(sky_look);
    dome->set_radiance_size(::godot::Sky::RADIANCE_SIZE_32);

    ::godot::Ref<::godot::Environment> environment;
    environment.instantiate();
    environment->set_sky(dome);
    environment->set_background(::godot::Environment::BG_SKY);
    environment->set_ambient_source(::godot::Environment::AMBIENT_SOURCE_COLOR);
    environment->set_reflection_source(::godot::Environment::REFLECTION_SOURCE_DISABLED);
    world->set_environment(environment);
    // the camera adapts to the light as an eye does (register D1721): light_plant_environment sets its
    // multiplier every frame from the sky port (scene_exposure); the sky's numbers stay as they are
    ::godot::Ref<::godot::CameraAttributesPractical> exposure;
    exposure.instantiate();
    exposure->set_auto_exposure_enabled(false);
    world->set_camera_attributes(exposure);
}

void light_plant_environment(::godot::Node3D* scene, const VivariumSkyView& sky, const SkyOvercastShade& overcast) {
    if (!sky.known) {
        return;
    }
    ::godot::WorldEnvironment* world = ::godot::Object::cast_to<::godot::WorldEnvironment>(
        scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_ENVIRONMENT)));
    if (world == nullptr || world->get_environment().is_null()) {
        return;
    }
    const ::godot::Ref<::godot::Environment> environment = world->get_environment();
    // THE LIFT REACHES EVERY LIGHT OF THE SCENE, NOT ONLY THE ONES GODOT LIFTS (register B0137, usecase:101415):
    // the exposure multiplier enters the sun and the sky before the frame is drawn
    // (exposure_normalization_factor), but an ambient of AMBIENT_SOURCE_COLOR and the fog's light reach the
    // scene as colour × energy alone (render_scene_data_rd.cpp, Godot 4.3). At night the sun is down and the
    // ground has only the ambient: 0.02/0.02/0.06 at 0.15 stayed unlifted at about 0.001 while the sky beside
    // it was lifted, and the Mobile frame (A2B10G10R10_UNORM, luminance multiplier 2) rounds that to 0 - the
    // night ground of run 024637 stood at (0,0,0) under a sky of (6,6,28). The same lift goes to both here.
    const float lift = scene_exposure(sky);
    const ::godot::Ref<::godot::Sky> dome = environment->get_sky();
    const ::godot::Ref<::godot::ShaderMaterial> sky_look =
        dome.is_valid() ? ::godot::Ref<::godot::ShaderMaterial>(dome->get_material())
                        : ::godot::Ref<::godot::ShaderMaterial>();
    if (sky_look.is_valid()) {
        // the sky's own colours once the HUD stands (SkyOvercast: m.html skyCols by the sun's height, veiled by the
        // fog's reach at the horizon); before that the sky port's records as they come
        sky_look->set_shader_parameter(GODOT_SKY_BACKGROUND,
                                       overcast.sky_own ? overcast.sky_zenith
                                                        : ::godot::Color(static_cast<float>(sky.background_r),
                                                                         static_cast<float>(sky.background_g),
                                                                         static_cast<float>(sky.background_b)));
        sky_look->set_shader_parameter(GODOT_SKY_HORIZON,
                                       overcast.sky_own ? overcast.sky_horizon
                                                        : ::godot::Color(static_cast<float>(sky.fog_r),
                                                                         static_cast<float>(sky.fog_g),
                                                                         static_cast<float>(sky.fog_b)));
        // where the gradient reaches the sky high up: the upward share of the camera's ray through the middle
        // of the frame's top edge (register B0187) - view geometry, projected by the camera itself
        float frame_top = GODOT_SKY_ZENITH_SHARE;
        const ::godot::Camera3D* eye =
            ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
        const ::godot::Viewport* view = eye != nullptr ? eye->get_viewport() : nullptr;
        if (view != nullptr) {
            const ::godot::Vector2 frame = view->get_visible_rect().size;
            const float top_share =
                static_cast<float>(eye->project_ray_normal(::godot::Vector2(frame.x * 0.5f, 0.0f)).y);
            if (top_share > 0.0f) {
                frame_top = top_share;
            }
        }
        sky_look->set_shader_parameter(GODOT_SKY_FRAME_TOP, frame_top);
        // an overcast sky turns both ends toward its greys (row N01); an open sky carries the share 0
        sky_look->set_shader_parameter(GODOT_SKY_OVERCAST_ZENITH, overcast.zenith);
        sky_look->set_shader_parameter(GODOT_SKY_OVERCAST_HORIZON, overcast.horizon);
        sky_look->set_shader_parameter(GODOT_SKY_OVERCAST_MIX, overcast.mix);
    }
    environment->set_ambient_light_color(::godot::Color(static_cast<float>(sky.ambient_r),
                                                        static_cast<float>(sky.ambient_g),
                                                        static_cast<float>(sky.ambient_b)));
    // the land under an overcast sky stands dimmer (m.html light(): times sky_overcast_light), the sun the same in
    // light_plant_scene; the lift stays the one of the sky port's light, so the dimming reaches the picture
    environment->set_ambient_light_energy(static_cast<float>(sky.ambient_intensity) * lift * overcast.light);
    environment->set_fog_light_energy(lift);
    environment->set_fog_enabled(true);
    environment->set_fog_mode(::godot::Environment::FOG_MODE_DEPTH);
    environment->set_fog_light_color(::godot::Color(static_cast<float>(sky.fog_r), static_cast<float>(sky.fog_g),
                                                    static_cast<float>(sky.fog_b)));
    environment->set_fog_depth_begin(static_cast<float>(sky.fog_near));
    environment->set_fog_depth_end(static_cast<float>(sky.fog_far));
    environment->set_fog_sky_affect(GODOT_FOG_SKY_AFFECT);
    // the shimmer of hot air ase-atmosphere computes, carried in the fog record (usecase:101515)
    veil_plant_scene(scene, sky);
    const ::godot::Ref<::godot::CameraAttributes> exposure = world->get_camera_attributes();
    if (exposure.is_valid()) {
        exposure->set_exposure_multiplier(lift);
    }
    // the ground's dither is half a frame step after the lift, so the shader divides its emission by the same
    // lift (GODOT_GROUND_SHADER_FILE, register B0207)
    ::godot::RenderingServer::get_singleton()->global_shader_parameter_set(GODOT_SHADER_GLOBAL_LIFT, lift);
}

float plant_fog_reach(const ::godot::Node3D* scene, const VivariumSkyView& sky, const ::godot::Vector3& at) {
    if (scene == nullptr || !sky.known) {
        return 0.0f;
    }
    const ::godot::Camera3D* eye =
        ::godot::Object::cast_to<::godot::Camera3D>(scene->get_node_or_null(::godot::NodePath(GODOT_PLANT_CAMERA)));
    if (eye == nullptr) {
        return 0.0f;
    }
    // the depth fog runs linear from fog_near to fog_far over the distance from the eye (FOG_MODE_DEPTH above)
    const float reach = static_cast<float>(sky.fog_far - sky.fog_near);
    if (!(reach > 0.0f)) {
        return 0.0f;
    }
    const float distance = eye->get_transform().origin.distance_to(at);
    return ase::math::clamp((distance - static_cast<float>(sky.fog_near)) / reach, 0.0f, 1.0f);
}

}  // namespace ase::adp::godot

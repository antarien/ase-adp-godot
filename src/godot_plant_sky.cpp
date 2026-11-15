/**
 * ASE GODOT ADAPTER - SUN, MOONS AND COMET IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_sky.cpp
 * @brief       The moon lights, the discs of sun and moons and the comet in the care view's 3D scene
 * @description See godot_plant_sky.hpp. Every body of the sky stands as a disc GODOT_SKY_DISC_DISTANCE from the
 *              middle of the sky dome along the direction of its record (godot_plant_sky_disc.hpp); the middle
 *              is reckoned once a frame from the camera and the ground's visible far edge (sky_dome_centre), so
 *              the dome's horizon lies where the terrain ends in the picture (B0262). Every moon slot has a
 *              directional light of its own beside the sun's, without shadows: by night the sun light
 *              stands below the horizon, and the moonlight ase-celestial reckons is what lights the birch.
 *              Every comet slot has a head, a disc like a moon's, and a tail, a cone from the head along
 *              the tail direction of its record (usecase:101274 - track the comets). Every planet slot
 *              has a disc at the angular size of its record, glowing by its illuminated share
 *              (usecase:101273 - track the neighbour planet). Every companion star slot has a disc at the
 *              angular size of its record, glowing by its nebula (usecase:101272 - track the companion star).
 *              Every moon half slot has a disc for a half of the split moon Aurora. The asteroid belt is a
 *              row of grains along the span of its record, as many as its density (usecase:2758 - generated
 *              asteroids). The meteor shower is a ring of streaks of glowing points around its radiant. The
 *              starfield stands in godot_plant_stars.cpp (build_plant_stars, light_plant_stars): the star set
 *              of the stars port laid once per set, the whole sphere turned by the turn record every frame
 *              (usecase:5606 - observe the sky). Under an overcast sky the moon discs and the stars fade to the
 *              shares the shade of SkyOvercast lets through (PLAN_ASE_VIVARIUM_PHASE_06_HUD row N02). The sun
 *              carries the mockup's halo (godot_plant_sun_halo.cpp, row N03), behind its disc.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101384 - place sun and moon spatially
 * @actor       cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-10-26
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_sky.hpp>
#include <ase/adp/godot/godot_plant_sky_disc.hpp>
#include <ase/adp/godot/godot_plant_clouds.hpp>
#include <ase/adp/godot/godot_plant_stars.hpp>
#include <ase/adp/godot/godot_plant_sun_halo.hpp>
#include <ase/adp/godot/godot_plant_horizon_ridge.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** The node of `scene` with that name, cast to its class; nullptr when the scene has none. */
template <typename T>
T* sky_part(::godot::Node3D* scene, const ::godot::String& name) {
    return ::godot::Object::cast_to<T>(scene->get_node_or_null(::godot::NodePath(name)));
}

/** The name of a moon or comet slot's node: `prefix` and the slot number. */
::godot::String slot_name(const char* prefix, uint32_t slot) {
    return ::godot::String(prefix) + ::godot::String::num_uint64(slot);
}

/**
 * The tail of a comet slot under `scene`: a cone, as wide as the comet head where it starts and closed at its
 * far end, in an unshaded material outside the fog that casts no shadow; hidden until light_plant_sky stands
 * it along the tail direction of the comet record.
 */
void add_comet_tail(::godot::Node3D* scene, const ::godot::String& name) {
    ::godot::MeshInstance3D* tail = memnew(::godot::MeshInstance3D);
    tail->set_name(name);
    ::godot::Ref<::godot::CylinderMesh> cone;
    cone.instantiate();
    cone->set_bottom_radius(GODOT_COMET_HEAD_RADIUS);
    cone->set_top_radius(0.0f);
    cone->set_height(GODOT_COMET_HEAD_RADIUS);
    cone->set_radial_segments(GODOT_COMET_TAIL_SEGMENTS);
    cone->set_material(sky_glow());
    tail->set_mesh(cone);
    tail->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    tail->set_visible(false);
    scene->add_child(tail);
}

/**
 * Sets the light of one moon slot: dark and hidden without a moon in it, else shining along the negative of
 * the moon's direction in its colour at its strength.
 */
void light_moon_slot(::godot::Node3D* scene, uint32_t slot, bool shown, const VivariumMoonView& moon) {
    ::godot::DirectionalLight3D* light = sky_part<::godot::DirectionalLight3D>(scene, slot_name(GODOT_PLANT_MOON, slot));
    if (light == nullptr) {
        return;
    }
    light->set_visible(shown);
    if (!shown) {
        light->set_param(::godot::Light3D::PARAM_ENERGY, 0.0f);
        return;
    }
    // the light shines along its -Z: placed on the side of the moon and turned to the origin
    const ::godot::Vector3 toward =
        ::godot::Vector3(static_cast<float>(moon.x), static_cast<float>(moon.y), static_cast<float>(moon.z)).normalized();
    const bool upright = toward.y > GODOT_GEOMETRY_UPRIGHT || toward.y < -GODOT_GEOMETRY_UPRIGHT;
    light->look_at_from_position(toward, ::godot::Vector3(0.0f, 0.0f, 0.0f),
                                 upright ? ::godot::Vector3(1.0f, 0.0f, 0.0f) : ::godot::Vector3(0.0f, 1.0f, 0.0f));
    light->set_color(::godot::Color(static_cast<float>(moon.r), static_cast<float>(moon.g), static_cast<float>(moon.b)));
    light->set_param(::godot::Light3D::PARAM_ENERGY, static_cast<float>(moon.intensity));
}

/**
 * Stands one comet slot: hidden without a comet in it, else its head along the comet's direction at
 * GODOT_SKY_DISC_DISTANCE from `centre`, the middle of the sky dome (sky_dome_centre), and its tail as a cone
 * from the head along the tail direction of the record. The arc `length` of the record spans the chord
 * 2·d·sin(length/2) at the disc distance d; the cone is as long as that chord, its axis the tail direction.
 * Head and tail glow in the comet's brightness, faded by the share `through` the clouds let pass
 * (glow_through). A tail without direction - the comet on the line through the sun - stays hidden beside a
 * shown head.
 */
void stand_comet_slot(::godot::Node3D* scene, uint32_t slot, bool shown, const VivariumCometView& comet, float through,
                      const ::godot::Vector3& centre) {
    ::godot::MeshInstance3D* head = sky_part<::godot::MeshInstance3D>(scene, slot_name(GODOT_PLANT_COMET_HEAD, slot));
    ::godot::MeshInstance3D* tail = sky_part<::godot::MeshInstance3D>(scene, slot_name(GODOT_PLANT_COMET_TAIL, slot));
    if (head == nullptr || tail == nullptr) {
        return;
    }
    head->set_visible(shown);
    if (!shown) {
        tail->set_visible(false);
        return;
    }
    const ::godot::Color glow_rgb = glow_through(static_cast<float>(comet.intensity), through);
    const ::godot::BaseMaterial3D::Transparency blend =
        glow_rgb.a < 1.0f ? ::godot::BaseMaterial3D::TRANSPARENCY_ALPHA : ::godot::BaseMaterial3D::TRANSPARENCY_DISABLED;

    // the head: a disc of the view's comet radius along the comet's direction
    const ::godot::Vector3 toward =
        ::godot::Vector3(static_cast<float>(comet.x), static_cast<float>(comet.y), static_cast<float>(comet.z)).normalized();
    const ::godot::Vector3 head_at = centre + toward * GODOT_SKY_DISC_DISTANCE;
    head->set_position(head_at);
    const ::godot::Ref<::godot::SphereMesh> sphere = head->get_mesh();
    if (sphere.is_valid()) {
        const ::godot::Ref<::godot::StandardMaterial3D> head_glow = sphere->get_material();
        if (head_glow.is_valid()) {
            head_glow->set_transparency(blend);
            head_glow->set_albedo(glow_rgb);
        }
    }

    // the tail: a cone from the head along the tail direction, as long as the chord of its arc
    const ::godot::Vector3 along = ::godot::Vector3(static_cast<float>(comet.tail_x), static_cast<float>(comet.tail_y),
                                                    static_cast<float>(comet.tail_z));
    const float chord = 2.0f * GODOT_SKY_DISC_DISTANCE * ase::math::sin(static_cast<float>(comet.length) * 0.5f);
    const bool has_tail = along.length() > 0.0f && chord > 0.0f;
    tail->set_visible(has_tail);
    if (!has_tail) {
        return;
    }
    const ::godot::Ref<::godot::CylinderMesh> cone = tail->get_mesh();
    if (cone.is_null()) {
        return;
    }
    cone->set_height(chord);
    const ::godot::Ref<::godot::StandardMaterial3D> tail_glow = cone->get_material();
    if (tail_glow.is_valid()) {
        tail_glow->set_transparency(blend);
        tail_glow->set_albedo(glow_rgb);
    }
    // the cone's axis is its local +Y, closed at the top: +Y turned onto the tail direction, the wide end at the
    // head. Basis(x, y, z) takes the three axes as its columns, so local +Y lands on axis_y; z = x × y keeps it
    // right-handed
    const ::godot::Vector3 axis_y = along.normalized();
    const ::godot::Vector3 axis_x = toward.cross(axis_y).normalized();
    const ::godot::Vector3 axis_z = axis_x.cross(axis_y).normalized();
    tail->set_transform(::godot::Transform3D(::godot::Basis(axis_x, axis_y, axis_z), head_at + axis_y * (chord * 0.5f)));
}

}  // namespace

void build_plant_sky(::godot::Node3D* scene) {
    add_sky_disc(scene, ::godot::String(GODOT_PLANT_SUN_DISC), GODOT_SUN_DISC_RADIUS, sky_glow());
    // the halo around the sun (row N03); hidden until light_plant_sky stands it with the HUD's frame
    build_plant_sun_halo(scene);
    // the two ridges of hills on the horizon (row N05); hidden until the view stands them with the HUD's frame
    build_plant_horizon_ridge(scene);
    // every moon slot lights the night from its own side; dark until the sky port carries its record. Its disc
    // carries the crescent of the mockup (moon_glow, row N02)
    for (uint32_t slot = 0u; slot < GODOT_SKY_MOONS_MAX; ++slot) {
        ::godot::DirectionalLight3D* light = memnew(::godot::DirectionalLight3D);
        light->set_name(slot_name(GODOT_PLANT_MOON, slot));
        light->set_param(::godot::Light3D::PARAM_ENERGY, 0.0f);
        light->set_visible(false);
        scene->add_child(light);
        add_sky_disc(scene, slot_name(GODOT_PLANT_MOON_DISC, slot), GODOT_MOON_DISC_RADIUS, moon_glow());
    }
    // every comet slot has a head and a tail; hidden until the sky port carries its record
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMETS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_COMET_HEAD, slot), GODOT_COMET_HEAD_RADIUS, sky_glow());
        add_comet_tail(scene, slot_name(GODOT_PLANT_COMET_TAIL, slot));
    }
    // every planet slot has a disc; hidden until the sky port carries its record, sized by it then
    for (uint32_t slot = 0u; slot < GODOT_SKY_PLANETS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_PLANET_DISC, slot), GODOT_MOON_DISC_RADIUS, sky_glow());
    }
    // every companion star slot has a disc; hidden until the sky port carries its record, sized by it then
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMPANIONS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_COMPANION_DISC, slot), GODOT_MOON_DISC_RADIUS, sky_glow());
    }
    // every moon half slot has a disc; hidden until the sky port carries its record, sized by it then
    for (uint32_t slot = 0u; slot < GODOT_SKY_FRAGMENTS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_FRAGMENT_DISC, slot), GODOT_MOON_DISC_RADIUS, sky_glow());
    }
    // the grains of the belt arc; hidden until the sky port carries a belt record
    for (uint32_t grain = 0u; grain < GODOT_BELT_GRAINS_MAX; ++grain) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_BELT_GRAIN, grain), GODOT_BELT_GRAIN_RADIUS, sky_glow());
    }
    // the points of the meteor shower's streaks; hidden until the sky port carries a shower record
    for (uint32_t point = 0u; point < GODOT_SHOWER_STREAKS_MAX * GODOT_SHOWER_STREAK_POINTS; ++point) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_SHOWER_POINT, point), GODOT_SHOWER_POINT_RADIUS, sky_glow());
    }
    // the stars (godot_plant_stars.cpp); no instance shows until the stars port carries a set
    build_plant_stars(scene);
    // the cloud banks of an overcast sky (godot_plant_clouds.cpp, N04); no instance shows until the HUD lays banks
    build_plant_clouds(scene);
}

void light_plant_sky(::godot::Node3D* scene, const VivariumSkyView& sky, const SkyOvercastShade& overcast,
                     const ::godot::Vector3* edge) {
    if (!sky.known) {
        return;
    }
    // THE MIDDLE OF THE SKY DOME, once a frame (B0262): every disc, the halo, the comets and the starfield stand
    // on it, so the dome's horizon lies on the ray from the camera through the ground's visible far edge
    const ::godot::Camera3D* eye = sky_part<::godot::Camera3D>(scene, ::godot::String(GODOT_PLANT_CAMERA));
    if (eye == nullptr) {
        return;
    }
    const ::godot::Vector3 centre = sky_dome_centre(eye, edge);

    // the sun disc: along the sun direction while the sun stands above the horizon plane, in the colour
    // ase-pl-sky gives its disc - the sunlight darkened by the obscuration of an eclipse (usecase:101276).
    // Behind the clouds of an overcast sky it takes the shade's sun_veil, the mockup's sun seen through them
    // (row N03, m.html drawSky 419); the eclipse's darkening is then the clouds' as the mockup draws it
    const ::godot::Vector3 toward_sun(static_cast<float>(sky.sun_x), static_cast<float>(sky.sun_y),
                                      static_cast<float>(sky.sun_z));
    const ::godot::Color sun_disc(static_cast<float>(sky.sun_disc_r), static_cast<float>(sky.sun_disc_g),
                                  static_cast<float>(sky.sun_disc_b));
    stand_sky_disc(scene, ::godot::String(GODOT_PLANT_SUN_DISC), sky.sun_y > 0.0, toward_sun,
                   sun_disc.lerp(overcast.sun_veil, overcast.veil), static_cast<float>(sky.sun_size), centre);
    // the halo around the sun, behind its disc (row N03)
    light_plant_sun_halo(scene, sky.sun_y > 0.0, toward_sun, static_cast<float>(sky.sun_size), overcast, centre);

    // the moons: every slot the port filled shows its disc and its light, the others stand dark; the disc
    // takes the colour ase-pl-sky gives it - the moonlight darkened by the shadow of a lunar eclipse
    // (usecase:101276), the light keeps the moonlight colour; the disc stands at the mockup's opacity and
    // behind the clouds fades to the share the overcast shade lets through (row N02, 1 under an open sky),
    // the crescent's shadow disc over it
    for (uint32_t slot = 0u; slot < GODOT_SKY_MOONS_MAX; ++slot) {
        const bool shown = slot < sky.moons;
        const VivariumMoonView& moon = sky.moon[slot];
        stand_moon_disc(scene, slot_name(GODOT_PLANT_MOON_DISC, slot), shown,
                        ::godot::Vector3(static_cast<float>(moon.x), static_cast<float>(moon.y), static_cast<float>(moon.z)),
                        ::godot::Color(static_cast<float>(moon.disc_r), static_cast<float>(moon.disc_g),
                                       static_cast<float>(moon.disc_b), overcast.moon_opacity * overcast.moon),
                        static_cast<float>(moon.size), overcast, centre);
        light_moon_slot(scene, slot, shown, moon);
    }

    // the comets: every slot the port filled shows its head and its tail, the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMETS_MAX; ++slot) {
        stand_comet_slot(scene, slot, slot < sky.comets, sky.comet[slot], overcast.stars, centre);
    }

    // the planets: every slot the port filled shows its disc at its angular size, lit by its illuminated
    // share - the planet shines by the sunlight it returns, so its glow is that share; the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_PLANETS_MAX; ++slot) {
        const VivariumPlanetView& planet = sky.planet[slot];
        const float lit = static_cast<float>(planet.phase);
        stand_sky_disc(scene, slot_name(GODOT_PLANT_PLANET_DISC, slot), slot < sky.planets,
                       ::godot::Vector3(static_cast<float>(planet.x), static_cast<float>(planet.y),
                                        static_cast<float>(planet.z)),
                       glow_through(lit, overcast.stars), static_cast<float>(planet.size), centre);
    }

    // the companion stars: every slot the port filled shows its disc at its angular size, glowing by the
    // strength of its nebula as ase-ephemeris reckons it; the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMPANIONS_MAX; ++slot) {
        const VivariumCompanionView& companion = sky.companion[slot];
        const float glow = static_cast<float>(companion.intensity);
        stand_sky_disc(scene, slot_name(GODOT_PLANT_COMPANION_DISC, slot), slot < sky.companions,
                       ::godot::Vector3(static_cast<float>(companion.x), static_cast<float>(companion.y),
                                        static_cast<float>(companion.z)),
                       glow_through(glow, overcast.stars), static_cast<float>(companion.size), centre);
    }

    // the halves of Aurora: every slot the port filled shows its disc at its angular size, glowing by its
    // brightness; the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_FRAGMENTS_MAX; ++slot) {
        const VivariumFragmentView& fragment = sky.fragment[slot];
        const float shine = static_cast<float>(fragment.intensity);
        stand_sky_disc(scene, slot_name(GODOT_PLANT_FRAGMENT_DISC, slot), slot < sky.fragments,
                       ::godot::Vector3(static_cast<float>(fragment.x), static_cast<float>(fragment.y),
                                        static_cast<float>(fragment.z)),
                       glow_through(shine, overcast.stars), static_cast<float>(fragment.size), centre);
    }

    // the asteroid belt: its density says how many grains show, spread evenly over the span of the arc, each the
    // record direction turned about the upward axis; without a belt record every grain stays hidden
    const bool belt_shown = sky.belts > 0u;
    const VivariumBeltView& belt = sky.belt[0];
    const uint32_t grains_shown =
        belt_shown ? static_cast<uint32_t>(ase::math::round(static_cast<float>(belt.density) * static_cast<float>(GODOT_BELT_GRAINS_MAX))) : 0u;
    const ::godot::Vector3 belt_centre(static_cast<float>(belt.x), static_cast<float>(belt.y), static_cast<float>(belt.z));
    const float glitter = static_cast<float>(belt.intensity);
    for (uint32_t grain = 0u; grain < GODOT_BELT_GRAINS_MAX; ++grain) {
        const bool grain_shown = grain < grains_shown;
        const float along = grains_shown > 1u
            ? (static_cast<float>(grain) / static_cast<float>(grains_shown - 1u) - 0.5f) * static_cast<float>(belt.span)
            : 0.0f;
        const ::godot::Vector3 toward = belt_centre.rotated(::godot::Vector3(0.0f, 1.0f, 0.0f), along);
        stand_sky_disc(scene, slot_name(GODOT_PLANT_BELT_GRAIN, grain), grain_shown, toward,
                       glow_through(glitter, overcast.stars),
                       2.0f * ase::math::atan2(GODOT_BELT_GRAIN_RADIUS, GODOT_SKY_DISC_DISTANCE), centre);
    }

    // the meteor shower (usecase:2760 - show meteorite in the nightsky): its intensity says how many streaks
    // show, spread evenly on a ring around the radiant; each streak is a trail of points running away from the
    // radiant and fading outwards. Without a shower record every point stays hidden
    const bool shower_shown = sky.showers > 0u;
    const VivariumShowerView& shower = sky.shower[0];
    const uint32_t streaks_shown = shower_shown
        ? static_cast<uint32_t>(ase::math::round(static_cast<float>(shower.intensity) * static_cast<float>(GODOT_SHOWER_STREAKS_MAX)))
        : 0u;
    const ::godot::Vector3 radiant(static_cast<float>(shower.x), static_cast<float>(shower.y), static_cast<float>(shower.z));
    ::godot::Vector3 across = radiant.cross(::godot::Vector3(0.0f, 1.0f, 0.0f));
    if (across.length() <= 0.0f) {
        across = ::godot::Vector3(1.0f, 0.0f, 0.0f);
    }
    across = across.normalized();
    const ::godot::Vector3 axis = radiant.length() > 0.0f ? radiant.normalized() : ::godot::Vector3(0.0f, 1.0f, 0.0f);
    const float shine = static_cast<float>(shower.intensity);
    for (uint32_t streak = 0u; streak < GODOT_SHOWER_STREAKS_MAX; ++streak) {
        const bool streak_shown = streak < streaks_shown;
        const float around = static_cast<float>(streak) / static_cast<float>(GODOT_SHOWER_STREAKS_MAX) * 2.0f * ase::math::PI;
        for (uint32_t point = 0u; point < GODOT_SHOWER_STREAK_POINTS; ++point) {
            const float away = GODOT_SHOWER_STREAK_SPREAD + static_cast<float>(point) * GODOT_SHOWER_STREAK_STEP;
            const ::godot::Vector3 toward = axis.rotated(across, away).rotated(axis, around);
            const float fade = shine * (1.0f - static_cast<float>(point) / static_cast<float>(GODOT_SHOWER_STREAK_POINTS));
            stand_sky_disc(scene, slot_name(GODOT_PLANT_SHOWER_POINT, streak * GODOT_SHOWER_STREAK_POINTS + point),
                           streak_shown, toward, glow_through(fade, overcast.stars),
                           2.0f * ase::math::atan2(GODOT_SHOWER_POINT_RADIUS, GODOT_SKY_DISC_DISTANCE), centre);
        }
    }

    // the stars (usecase:5606 - observe the sky): the held set, turned with the sky, on the same middle
    // (godot_plant_stars.cpp)
    light_plant_stars(scene, sky, overcast, centre);
}

}  // namespace ase::adp::godot

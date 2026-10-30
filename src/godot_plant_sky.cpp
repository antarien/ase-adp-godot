/**
 * ASE GODOT ADAPTER - SUN, MOONS AND COMET IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_sky.cpp
 * @brief       The moon lights, the discs of sun and moons and the comet in the care view's 3D scene
 * @description See godot_plant_sky.hpp. A disc is a sphere in an unshaded material outside the fog that casts
 *              no shadow; it stands GODOT_SKY_DISC_DISTANCE from the camera along the direction of its
 *              record, so it keeps its place in the sky however the camera moves. Every moon slot has a
 *              directional light of its own beside the sun's, without shadows: by night the sun light
 *              stands below the horizon, and the moonlight ase-celestial reckons is what lights the birch.
 *              Every comet slot has a head, a disc like a moon's, and a tail, a cone from the head along
 *              the tail direction of its record (usecase:101274 - track the comets). Every planet slot
 *              has a disc at the angular size of its record, glowing by its illuminated share
 *              (usecase:101273 - track the neighbour planet). Every companion star slot has a disc at the
 *              angular size of its record, glowing by its nebula (usecase:101272 - track the companion star).
 *              Every moon half slot has a disc for a half of the split moon Aurora. The asteroid belt is a
 *              row of grains along the span of its record, as many as its density (usecase:2758 - generated
 *              asteroids).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101384 - place sun and moon spatially
 * @actor       cycle/DAY/NIGHT (semi/nbp) Day/Night Cycle
 * @created     2026-10-26
 * @modified    2026-10-28
 * @version     00.00.03.00000
 */

#include <ase/adp/godot/godot_plant_sky.hpp>
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

/** An unshaded material outside the fog: what glows in the sky, lit by nothing in the scene. */
::godot::Ref<::godot::StandardMaterial3D> sky_glow() {
    ::godot::Ref<::godot::StandardMaterial3D> glow;
    glow.instantiate();
    glow->set_shading_mode(::godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
    glow->set_flag(::godot::BaseMaterial3D::FLAG_DISABLE_FOG, true);
    return glow;
}

/**
 * A disc of the sky under `scene`: a sphere of `radius` metres in an unshaded material outside the fog that
 * casts no shadow, hidden until light_plant_sky stands it along the direction of its record.
 */
void add_sky_disc(::godot::Node3D* scene, const ::godot::String& name, float radius) {
    ::godot::MeshInstance3D* disc = memnew(::godot::MeshInstance3D);
    disc->set_name(name);
    ::godot::Ref<::godot::SphereMesh> sphere;
    sphere.instantiate();
    sphere->set_radius(radius);
    sphere->set_height(radius * 2.0f);
    sphere->set_radial_segments(GODOT_SKY_DISC_SEGMENTS);
    sphere->set_rings(GODOT_SKY_DISC_RINGS);
    sphere->set_material(sky_glow());
    disc->set_mesh(sphere);
    disc->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    disc->set_visible(false);
    scene->add_child(disc);
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
 * Stands the disc `name` along `toward` (world frame) at GODOT_SKY_DISC_DISTANCE from the camera, in the
 * colour `rgb`, sized to the angular diameter `angular_size` (radians) ase-ephemeris holds for the body:
 * a sphere that spans it at that distance has the radius GODOT_SKY_DISC_DISTANCE * tan(angular_size / 2).
 * The adapter computes no sky of its own; the size comes with the record of ase-pl-sky. Not `shown`, the
 * disc is hidden. No disc or no camera, nothing.
 */
void stand_sky_disc(::godot::Node3D* scene, const ::godot::String& name, bool shown, const ::godot::Vector3& toward,
                    const ::godot::Color& rgb, float angular_size) {
    ::godot::MeshInstance3D* disc = sky_part<::godot::MeshInstance3D>(scene, name);
    const ::godot::Camera3D* eye = sky_part<::godot::Camera3D>(scene, ::godot::String(GODOT_PLANT_CAMERA));
    if (disc == nullptr || eye == nullptr) {
        return;
    }
    disc->set_visible(shown);
    if (!shown) {
        return;
    }
    disc->set_position(eye->get_position() + toward.normalized() * GODOT_SKY_DISC_DISTANCE);
    const ::godot::Ref<::godot::SphereMesh> sphere = disc->get_mesh();
    if (sphere.is_null()) {
        return;
    }
    const float radius = GODOT_SKY_DISC_DISTANCE * ase::math::tan(angular_size * 0.5f);
    sphere->set_radius(radius);
    sphere->set_height(radius * 2.0f);
    const ::godot::Ref<::godot::StandardMaterial3D> glow = sphere->get_material();
    if (glow.is_valid()) {
        glow->set_albedo(rgb);
    }
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
 * GODOT_SKY_DISC_DISTANCE from the camera and its tail as a cone from the head along the tail direction of the
 * record. The arc `length` of the record spans the chord 2·d·sin(length/2) at the disc distance d; the cone
 * is as long as that chord, its axis the tail direction. Head and tail glow in the comet's brightness. A tail
 * without direction - the comet on the line through the sun - stays hidden beside a shown head.
 */
void stand_comet_slot(::godot::Node3D* scene, uint32_t slot, bool shown, const VivariumCometView& comet) {
    ::godot::MeshInstance3D* head = sky_part<::godot::MeshInstance3D>(scene, slot_name(GODOT_PLANT_COMET_HEAD, slot));
    ::godot::MeshInstance3D* tail = sky_part<::godot::MeshInstance3D>(scene, slot_name(GODOT_PLANT_COMET_TAIL, slot));
    const ::godot::Camera3D* eye = sky_part<::godot::Camera3D>(scene, ::godot::String(GODOT_PLANT_CAMERA));
    if (head == nullptr || tail == nullptr || eye == nullptr) {
        return;
    }
    head->set_visible(shown);
    if (!shown) {
        tail->set_visible(false);
        return;
    }
    const float glow_share = static_cast<float>(comet.intensity);
    const ::godot::Color glow_rgb(glow_share, glow_share, glow_share);

    // the head: a disc of the view's comet radius along the comet's direction
    const ::godot::Vector3 toward =
        ::godot::Vector3(static_cast<float>(comet.x), static_cast<float>(comet.y), static_cast<float>(comet.z)).normalized();
    const ::godot::Vector3 head_at = eye->get_position() + toward * GODOT_SKY_DISC_DISTANCE;
    head->set_position(head_at);
    const ::godot::Ref<::godot::SphereMesh> sphere = head->get_mesh();
    if (sphere.is_valid()) {
        const ::godot::Ref<::godot::StandardMaterial3D> head_glow = sphere->get_material();
        if (head_glow.is_valid()) {
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
    add_sky_disc(scene, ::godot::String(GODOT_PLANT_SUN_DISC), GODOT_SUN_DISC_RADIUS);
    // every moon slot lights the night from its own side; dark until the sky port carries its record
    for (uint32_t slot = 0u; slot < GODOT_SKY_MOONS_MAX; ++slot) {
        ::godot::DirectionalLight3D* light = memnew(::godot::DirectionalLight3D);
        light->set_name(slot_name(GODOT_PLANT_MOON, slot));
        light->set_param(::godot::Light3D::PARAM_ENERGY, 0.0f);
        light->set_visible(false);
        scene->add_child(light);
        add_sky_disc(scene, slot_name(GODOT_PLANT_MOON_DISC, slot), GODOT_MOON_DISC_RADIUS);
    }
    // every comet slot has a head and a tail; hidden until the sky port carries its record
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMETS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_COMET_HEAD, slot), GODOT_COMET_HEAD_RADIUS);
        add_comet_tail(scene, slot_name(GODOT_PLANT_COMET_TAIL, slot));
    }
    // every planet slot has a disc; hidden until the sky port carries its record, sized by it then
    for (uint32_t slot = 0u; slot < GODOT_SKY_PLANETS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_PLANET_DISC, slot), GODOT_MOON_DISC_RADIUS);
    }
    // every companion star slot has a disc; hidden until the sky port carries its record, sized by it then
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMPANIONS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_COMPANION_DISC, slot), GODOT_MOON_DISC_RADIUS);
    }
    // every moon half slot has a disc; hidden until the sky port carries its record, sized by it then
    for (uint32_t slot = 0u; slot < GODOT_SKY_FRAGMENTS_MAX; ++slot) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_FRAGMENT_DISC, slot), GODOT_MOON_DISC_RADIUS);
    }
    // the grains of the belt arc; hidden until the sky port carries a belt record
    for (uint32_t grain = 0u; grain < GODOT_BELT_GRAINS_MAX; ++grain) {
        add_sky_disc(scene, slot_name(GODOT_PLANT_BELT_GRAIN, grain), GODOT_BELT_GRAIN_RADIUS);
    }
}

void light_plant_sky(::godot::Node3D* scene, const VivariumSkyView& sky) {
    if (!sky.known) {
        return;
    }
    // the sun disc: along the sun direction while the sun stands above the horizon plane, in the colour
    // ase-pl-sky gives its disc - the sunlight darkened by the obscuration of an eclipse (usecase:101276)
    const ::godot::Vector3 toward_sun(static_cast<float>(sky.sun_x), static_cast<float>(sky.sun_y),
                                      static_cast<float>(sky.sun_z));
    stand_sky_disc(scene, ::godot::String(GODOT_PLANT_SUN_DISC), sky.sun_y > 0.0, toward_sun,
                   ::godot::Color(static_cast<float>(sky.sun_disc_r), static_cast<float>(sky.sun_disc_g),
                                  static_cast<float>(sky.sun_disc_b)),
                   static_cast<float>(sky.sun_size));

    // the moons: every slot the port filled shows its disc and its light, the others stand dark
    for (uint32_t slot = 0u; slot < GODOT_SKY_MOONS_MAX; ++slot) {
        const bool shown = slot < sky.moons;
        const VivariumMoonView& moon = sky.moon[slot];
        stand_sky_disc(scene, slot_name(GODOT_PLANT_MOON_DISC, slot), shown,
                       ::godot::Vector3(static_cast<float>(moon.x), static_cast<float>(moon.y), static_cast<float>(moon.z)),
                       ::godot::Color(static_cast<float>(moon.r), static_cast<float>(moon.g), static_cast<float>(moon.b)),
                       static_cast<float>(moon.size));
        light_moon_slot(scene, slot, shown, moon);
    }

    // the comets: every slot the port filled shows its head and its tail, the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMETS_MAX; ++slot) {
        stand_comet_slot(scene, slot, slot < sky.comets, sky.comet[slot]);
    }

    // the planets: every slot the port filled shows its disc at its angular size, lit by its illuminated
    // share - the planet shines by the sunlight it returns, so its glow is that share; the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_PLANETS_MAX; ++slot) {
        const VivariumPlanetView& planet = sky.planet[slot];
        const float lit = static_cast<float>(planet.phase);
        stand_sky_disc(scene, slot_name(GODOT_PLANT_PLANET_DISC, slot), slot < sky.planets,
                       ::godot::Vector3(static_cast<float>(planet.x), static_cast<float>(planet.y),
                                        static_cast<float>(planet.z)),
                       ::godot::Color(lit, lit, lit), static_cast<float>(planet.size));
    }

    // the companion stars: every slot the port filled shows its disc at its angular size, glowing by the
    // strength of its nebula as ase-ephemeris reckons it; the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_COMPANIONS_MAX; ++slot) {
        const VivariumCompanionView& companion = sky.companion[slot];
        const float glow = static_cast<float>(companion.intensity);
        stand_sky_disc(scene, slot_name(GODOT_PLANT_COMPANION_DISC, slot), slot < sky.companions,
                       ::godot::Vector3(static_cast<float>(companion.x), static_cast<float>(companion.y),
                                        static_cast<float>(companion.z)),
                       ::godot::Color(glow, glow, glow), static_cast<float>(companion.size));
    }

    // the halves of Aurora: every slot the port filled shows its disc at its angular size, glowing by its
    // brightness; the others stand hidden
    for (uint32_t slot = 0u; slot < GODOT_SKY_FRAGMENTS_MAX; ++slot) {
        const VivariumFragmentView& fragment = sky.fragment[slot];
        const float shine = static_cast<float>(fragment.intensity);
        stand_sky_disc(scene, slot_name(GODOT_PLANT_FRAGMENT_DISC, slot), slot < sky.fragments,
                       ::godot::Vector3(static_cast<float>(fragment.x), static_cast<float>(fragment.y),
                                        static_cast<float>(fragment.z)),
                       ::godot::Color(shine, shine, shine), static_cast<float>(fragment.size));
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
                       ::godot::Color(glitter, glitter, glitter),
                       2.0f * ase::math::atan2(GODOT_BELT_GRAIN_RADIUS, GODOT_SKY_DISC_DISTANCE));
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
                           streak_shown, toward, ::godot::Color(fade, fade, fade),
                           2.0f * ase::math::atan2(GODOT_SHOWER_POINT_RADIUS, GODOT_SKY_DISC_DISTANCE));
        }
    }
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - PLANT SCENE IMPLEMENTATION
 *
 * @file        godot_plant_scene.cpp
 * @brief       Camera, sun, environment, ground and the plant nodes of the care view's 3D scene
 * @description See godot_plant_scene.hpp. Every mesh is the arrays of godot_plant_geometry.hpp handed to
 *              Godot as they are, normals included (mesh_of, godot_plant_scene_mesh.cpp): the ground carries the
 *              normals ase-render-data computed, the bark tube those of its rings and the leaf the normals of
 *              its template ports, and Godot derives none. The bark of every body is one continuous tube per
 *              axis (register B0295 point 4, fill_tube); every other instance buffer is the records of an
 *              instance port handed to MultiMesh as they are. A fill replaces the meshes and buffers of its
 *              nodes and keeps the nodes, so the scene graph stands once and only its contents change - a
 *              plant's node comes with its body and goes with it. Materials and organ template files are bound
 *              once (godot_plant_look.hpp); a fill only hands the bound resources on. Soil water and condition
 *              go to the ground and leaf nodes as instance shader parameters every frame (mark_plant_stands):
 *              the value of the record, a Nil where it carried none.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-11
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_asset_showcase_scene.hpp>
#include <ase/adp/godot/godot_plant_environment.hpp>
#include <ase/adp/godot/godot_plant_haze.hpp>
#include <ase/adp/godot/godot_plant_brook.hpp>
#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/godot_plant_look_shader.hpp>
#include <ase/adp/godot/godot_plant_sky.hpp>
#include <ase/adp/godot/godot_plant_templates.hpp>
#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>
#include <ase/adp/godot/asset/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The node of `scene` with that name, cast to its class; nullptr when the scene has none. */
template <typename T>
T* part_of(::godot::Node3D* scene, const char* name) {
    return ::godot::Object::cast_to<T>(scene->get_node_or_null(::godot::NodePath(name)));
}

/** A node of class T under `scene` with that name. */
template <typename T>
T* add_part(::godot::Node3D* scene, const char* name) {
    T* part = memnew(T);
    part->set_name(name);
    scene->add_child(part);
    return part;
}

/**
 * The organ template `code` of the template ports as a mesh (usecase:101011 tessellate the organ templates): the
 * corners and triangles ase-render-data laid, through build_template. Empty while the ports carry no set.
 */
::godot::Ref<::godot::Mesh> template_mesh(const GodotHostResourceManager& resources, uint32_t code) {
    PlantMeshArrays arrays;
    (void)build_template(resources.get_template_corners(), resources.get_template_triangles(), code, arrays);
    return ::godot::Ref<::godot::Mesh>(mesh_of(arrays));
}

/** The name of the node a plant stands as: GODOT_PLANT_BODY_PREFIX and the object id of its patch. */
::godot::String body_name(uint32_t body) {
    return ::godot::String(GODOT_PLANT_BODY_PREFIX) + ::godot::String::num_uint64(body);
}

/** One instance shader parameter of `node`: the number when it is known, else back to the shader's default. */
void hand_on(::godot::GeometryInstance3D* node, const char* name, bool known, float value) {
    if (node == nullptr) {
        return;
    }
    node->set_instance_shader_parameter(::godot::StringName(name), known ? ::godot::Variant(value) : ::godot::Variant());
}

/**
 * The node of the plant on `body` under `bodies`, made when it has none yet with its bark node (GODOT_PLANT_BARK,
 * usecase:101613) holding the empty fine and coarse tube (GODOT_PLANT_TUBES, GODOT_PLANT_TWIGS), the empty fine and
 * coarse wood disks of its cut stubs (GODOT_PLANT_SCARS, GODOT_PLANT_SCAR_TWIGS) and its four empty instance nodes.
 */
::godot::Node3D* plant_node(::godot::Node3D* bodies, uint32_t body) {
    const ::godot::String name = body_name(body);
    ::godot::Node3D* plant =
        ::godot::Object::cast_to<::godot::Node3D>(bodies->get_node_or_null(::godot::NodePath(name)));
    if (plant != nullptr) {
        return plant;
    }
    plant = memnew(::godot::Node3D);
    plant->set_name(name);
    bodies->add_child(plant);
    ::godot::Node3D* bark = add_part<::godot::Node3D>(plant, GODOT_PLANT_BARK);
    (void)add_part<::godot::MeshInstance3D>(bark, GODOT_PLANT_TUBES);
    (void)add_part<::godot::MeshInstance3D>(bark, GODOT_PLANT_TWIGS);
    (void)add_part<::godot::MeshInstance3D>(bark, GODOT_PLANT_SCARS);
    (void)add_part<::godot::MeshInstance3D>(bark, GODOT_PLANT_SCAR_TWIGS);
    for (const char* part : {GODOT_PLANT_LEAVES, GODOT_PLANT_MALE, GODOT_PLANT_FEMALE, GODOT_PLANT_FRUIT}) {
        (void)add_part<::godot::MultiMeshInstance3D>(plant, part);
    }
    make_impostor(plant);
    return plant;
}

}  // namespace

::godot::Node3D* build_plant_scene(::godot::Node* parent, const PlantSceneCamera& camera) {
    ::godot::Node3D* scene = memnew(::godot::Node3D);
    scene->set_name(GODOT_PLANT_SCENE_ROOT);
    parent->add_child(scene);

    ::godot::Camera3D* eye = add_part<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    eye->set_fov(camera.fov_deg);
    eye->set_current(true);

    build_plant_environment(scene);

    (void)add_part<::godot::MeshInstance3D>(scene, GODOT_PLANT_GROUND);
    build_plant_water(scene);
    (void)add_part<::godot::Node3D>(scene, GODOT_PLANT_BODIES);
    (void)add_part<::godot::MultiMeshInstance3D>(scene, GODOT_PLANT_SEEDS);
    build_plant_sky(scene);
    build_plant_haze(scene);
    return scene;
}

void aim_plant_camera(::godot::Node3D* scene, const PlantSceneCamera& camera, const PlantOrigin& focus) {
    ::godot::Camera3D* eye = part_of<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    if (eye == nullptr) {
        return;
    }
    const PatchFocusAim aim = patch_focus_aim(camera, focus);
    eye->look_at_from_position(::godot::Vector3(aim.eye_x, aim.eye_y, aim.eye_z),
                               ::godot::Vector3(aim.target_x, aim.target_y, aim.target_z));
}

void fill_plant_ground(::godot::Node3D* scene, const std::vector<VivariumTerrainView>& samples,
                       const std::vector<VivariumGroundCellView>& cells, const std::vector<PlantGroundDisc>& discs,
                       const std::vector<PlantGroundBed>& beds, float step_m,
                       const ::godot::Ref<::godot::Material>& material, PlantGroundSurface& surface) {
    ::godot::MeshInstance3D* node = part_of<::godot::MeshInstance3D>(scene, GODOT_PLANT_GROUND);
    if (node == nullptr) {
        return;
    }
    const ::godot::Ref<::godot::ArrayMesh> held = node->get_mesh();
    if (held.is_valid() && patch_ground(samples, cells, discs, beds, surface)) {
        send_ground_patch(held, surface);
    } else {
        surface.arrays = PlantMeshArrays();
        surface.slots.clear();
        (void)build_ground(samples, cells, surface.arrays, surface.slots);
        (void)mark_ground(discs, beds, surface.arrays);
        surface.cells = cells;
        const ::godot::Ref<::godot::ArrayMesh> built = mesh_of(surface.arrays);
        node->set_mesh(built);
        hold_ground_surface(built, surface);
    }
    const ::godot::Ref<::godot::ShaderMaterial> shaded = material;
    if (shaded.is_valid()) {
        shaded->set_shader_parameter(GODOT_GROUND_GRID_STEP, step_m);
    }
    node->set_material_override(material);
}

void fill_plant_organs(::godot::Node3D* scene, const GodotHostResourceManager& resources,
                       const std::unordered_map<uint32_t, PlantOrigin>& origins, uint32_t shown_orders,
                       const PlantSceneLook& look) {
    ::godot::Node3D* bodies = part_of<::godot::Node3D>(scene, GODOT_PLANT_BODIES);
    if (bodies == nullptr) {
        return;
    }
    // a plant whose body has no origin any more leaves the scene
    const ::godot::TypedArray<::godot::Node> standing = bodies->get_children();
    for (int64_t i = 0; i < standing.size(); ++i) {
        ::godot::Node* plant = ::godot::Object::cast_to<::godot::Node>(standing[i]);
        if (plant == nullptr) {
            continue;
        }
        bool kept = false;
        for (const auto& origin : origins) {
            kept = kept || plant->get_name() == ::godot::StringName(body_name(origin.first));
        }
        if (!kept) {
            bodies->remove_child(plant);
            plant->queue_free();
        }
    }

    // the shapes: leaves, catkins and fruit stands on the templates of their files, and while a file is missing on
    // the port template that stands in - the leaf, the fine cylinder - in the error material (03.2 point 3)
    const ::godot::Ref<::godot::Mesh> fine = template_mesh(resources, GODOT_TMPL_FINE);
    const ::godot::Ref<::godot::Mesh> stand_in_leaf = template_mesh(resources, GODOT_TMPL_LEAF);
    const ::godot::Ref<::godot::Mesh> leaf = look.leaf.shape.is_valid() ? look.leaf.shape : stand_in_leaf;
    const ::godot::Ref<::godot::Mesh> male = look.male.shape.is_valid() ? look.male.shape : fine;
    const ::godot::Ref<::godot::Mesh> female = look.female.shape.is_valid() ? look.female.shape : fine;
    const ::godot::Ref<::godot::Mesh> fruit = look.fruit.shape.is_valid() ? look.fruit.shape : fine;

    // the instanced classes: which half of its records a bundle draws, with which custom data and sway. The
    // internodes are no bundle any more - their bark is one tube per axis (usecase:101613), the shown orders reach
    // its shader - so every bundle draws all its records, the half above GODOT_ORDER_EVERY. The leaves carry the
    // bend of their cards as custom data (PLAN_ASE_VIVARIUM_TREEGEN T.6); the vertex shader lifts the lattice by it.
    struct Bundle {
        const char* node;
        uint32_t organ_class;
        std::vector<float> PlantBodyRows::*half;
        std::vector<float> PlantBodyRows::*custom;
        std::vector<float> PlantBodyRows::*sway;
        const ::godot::Ref<::godot::Mesh>* shape;
        const ::godot::Ref<::godot::Material>* material;
    };
    const Bundle bundles[] = {
        {GODOT_PLANT_LEAVES, GODOT_ORGAN_LEAF, &PlantBodyRows::above, &PlantBodyRows::above_custom,
         &PlantBodyRows::above_sway, &leaf, &look.leaf.material},
        {GODOT_PLANT_MALE, GODOT_ORGAN_MALE, &PlantBodyRows::above, nullptr, &PlantBodyRows::above_sway, &male,
         &look.male.material},
        {GODOT_PLANT_FEMALE, GODOT_ORGAN_FEMALE, &PlantBodyRows::above, nullptr, &PlantBodyRows::above_sway, &female,
         &look.female.material},
        {GODOT_PLANT_FRUIT, GODOT_ORGAN_FRUIT, &PlantBodyRows::above, nullptr, &PlantBodyRows::above_sway, &fruit,
         &look.fruit.material},
    };
    // the records of every bundled class by body
    std::unordered_map<uint32_t, PlantBodyRows> rows[GODOT_DRAWN_COUNT];
    for (const Bundle& bundle : bundles) {
        rows[bundle.organ_class] = rows_by_body(resources.get_instances(bundle.organ_class),
                                                resources.get_organs(bundle.organ_class), GODOT_ORDER_EVERY);
    }
    // the bark of every body: one continuous tube per axis, its rings laid around every knot of the knot and foot
    // ports from radius, quaternion and v, the cut face of every stub closed on its knot (usecase:101613, B0677)
    const GodotKnotResourceManager& knot_store = resources.knots();
    const std::unordered_map<uint32_t, PlantBodyTube> tubes =
        tubes_by_body(knot_store.get_knots(), knot_store.get_feet(), knot_store.get_scars(),
                      resources.get_organs(GODOT_ORGAN_SEGMENT));

    // the view the plants are seen in: its render height in pixels and the tangent of half the vertical field of
    // view, which set from where the widest ring of a body is thinner than a pixel and its card takes over
    float view_height = 0.0f;
    float tan_half_fov = 0.0f;
    const ::godot::Camera3D* eye = part_of<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    if (eye != nullptr && eye->get_viewport() != nullptr) {
        view_height = eye->get_viewport()->get_visible_rect().size.y;
        tan_half_fov = ase::math::tan(static_cast<float>(eye->get_fov()) * 0.5f * ase::math::PI / 180.0f);
    }

    // every plant at the stem base of its body, its tube and its records as the ports carried them
    const PlantBodyRows none;
    const PlantBodyTube bare;
    for (const auto& origin : origins) {
        ::godot::Node3D* plant = plant_node(bodies, origin.first);
        plant->set_position(::godot::Vector3(origin.second.x, origin.second.y, origin.second.z));
        const auto tube = tubes.find(origin.first);
        const PlantBodyTube& body_tube = tube == tubes.end() ? bare : tube->second;
        // the bark twice over the same vertices: the fine tube up to the shown orders, the coarse tube above them
        ::godot::Node3D* bark = part_of<::godot::Node3D>(plant, GODOT_PLANT_BARK);
        if (bark != nullptr) {
            fill_tube(part_of<::godot::MeshInstance3D>(bark, GODOT_PLANT_TUBES), body_tube, body_tube.arrays.indices,
                      look.bark, shown_orders, false);
            fill_tube(part_of<::godot::MeshInstance3D>(bark, GODOT_PLANT_TWIGS), body_tube, body_tube.twig_indices,
                      look.bark, shown_orders, true);
            // the cut face of every stub (PHASE_09_TREEGEN Schnittfläche): the fan of its wood disk over the same
            // vertices in the cut wood, the fine pass up to the shown orders and the coarse pass above them, so each
            // disk shows once, on the pass that draws the stub it closes
            fill_tube(part_of<::godot::MeshInstance3D>(bark, GODOT_PLANT_SCARS), body_tube, body_tube.scar_indices,
                      look.wood, shown_orders, false);
            fill_tube(part_of<::godot::MeshInstance3D>(bark, GODOT_PLANT_SCAR_TWIGS), body_tube,
                      body_tube.scar_indices, look.wood, shown_orders, true);
        }
        std::vector<const std::vector<float>*> drawn_rows;
        for (const Bundle& bundle : bundles) {
            const auto found = rows[bundle.organ_class].find(origin.first);
            const PlantBodyRows& body_rows = found == rows[bundle.organ_class].end() ? none : found->second;
            const std::vector<float>* custom = bundle.custom == nullptr ? nullptr : &(body_rows.*bundle.custom);
            fill_instances(part_of<::godot::MultiMeshInstance3D>(plant, bundle.node), *bundle.shape,
                           body_rows.*bundle.half, custom, *bundle.material, &(body_rows.*bundle.sway));
            drawn_rows.push_back(&(body_rows.*bundle.half));
        }
        // a far body is drawn as its card (register B0295 point 4)
        fill_impostor(plant, impostor_of(body_tube, drawn_rows, view_height, tan_half_fov));
    }
}

void mark_plant_stands(::godot::Node3D* scene, const std::unordered_map<uint32_t, PlantStand>& stands,
                       const PlantStand& ground) {
    hand_on(part_of<::godot::MeshInstance3D>(scene, GODOT_PLANT_GROUND), GODOT_SHADER_SOIL_REL, ground.soil_known,
            ground.soil_rel);
    ::godot::Node3D* bodies = part_of<::godot::Node3D>(scene, GODOT_PLANT_BODIES);
    if (bodies == nullptr) {
        return;
    }
    for (const auto& stand : stands) {
        ::godot::Node3D* plant =
            ::godot::Object::cast_to<::godot::Node3D>(bodies->get_node_or_null(::godot::NodePath(body_name(stand.first))));
        if (plant == nullptr) {
            continue;
        }
        ::godot::MultiMeshInstance3D* leaves = part_of<::godot::MultiMeshInstance3D>(plant, GODOT_PLANT_LEAVES);
        hand_on(leaves, GODOT_SHADER_WILTED, stand.second.condition_known, stand.second.wilted);
        hand_on(leaves, GODOT_SHADER_DEAD, stand.second.condition_known, stand.second.dead);
        // its own autumn colour (usecase:101646 colour the leaves in autumn): unknown hands the default back, the green
        hand_on(leaves, GODOT_SHADER_AUTUMN, stand.second.autumn_known, stand.second.autumn);
    }
}

void fill_plant_seeds(::godot::Node3D* scene, const GodotHostResourceManager& resources,
                      const std::unordered_map<uint32_t, PlantOrigin>& origins, const PlantSceneLook& look) {
    ::godot::MultiMeshInstance3D* node = part_of<::godot::MultiMeshInstance3D>(scene, GODOT_PLANT_SEEDS);
    if (node == nullptr) {
        return;
    }
    // the nutlet template carries its own size; the leaf template of the ports stands in at the width of a
    // winged nutlet
    const bool bound = look.nutlet.shape.is_valid();
    const ::godot::Ref<::godot::Mesh> shape = bound ? look.nutlet.shape : template_mesh(resources, GODOT_TMPL_LEAF);
    const float scale = bound ? 1.0f : GODOT_NUTLET_STAND_IN_M;
    // one 3x4 per seed in the row order of MultiMesh: the axes at that scale, the origin at the stem base of its
    // body plus the place the port carried
    const std::vector<VivariumSeedView>& seeds = resources.get_seeds();
    std::vector<float> rows;
    rows.reserve(seeds.size() * GODOT_INSTANCE_VALUES);
    for (const VivariumSeedView& seed : seeds) {
        const auto origin = origins.find(seed.body);
        if (origin == origins.end()) {
            continue;
        }
        // all three axes in the frame of the stem foot, the height too: ase-physics lands the seed on the ground
        // its contact measures at the terrain sample, over the levelled disc too, so a hole dug into the disc
        // takes the seed down (D2315); the view adds no ground height of its own (D2312)
        const float at[3] = {static_cast<float>(origin->second.x + seed.x), static_cast<float>(origin->second.y + seed.y),
                             static_cast<float>(origin->second.z + seed.z)};
        for (uint32_t r = 0u; r < 3u; ++r) {
            for (uint32_t c = 0u; c < 3u; ++c) {
                rows.push_back(r == c ? scale : 0.0f);
            }
            rows.push_back(at[r]);
        }
    }
    fill_instances(node, shape, rows, nullptr, look.nutlet.material, nullptr);
}

void light_plant_scene(::godot::Node3D* scene, const VivariumSkyView& sky, const SkyOvercastShade& overcast,
                       const ::godot::Vector3* edge) {
    if (!sky.known) {
        return;
    }
    const float toward_sun[3] = {static_cast<float>(sky.sun_x), static_cast<float>(sky.sun_y),
                                 static_cast<float>(sky.sun_z)};
    const float sun_rgb[3] = {static_cast<float>(sky.sun_r), static_cast<float>(sky.sun_g),
                              static_cast<float>(sky.sun_b)};
    // under an overcast sky the sun lights the land dimmer (m.html light(), row N01)
    light_asset_showcase(scene, toward_sun, sun_rgb, static_cast<float>(sky.sun_intensity) * overcast.light);
    ::godot::DirectionalLight3D* sun = part_of<::godot::DirectionalLight3D>(scene, GODOT_SHOWCASE_SUN);
    if (sun != nullptr) {
        sun->set_shadow(true);
    }
    // under an overcast sky the moons and the stars fade behind the clouds (m.html drawSky, row N02)
    light_plant_sky(scene, sky, overcast, edge);
    // sky colour, ambient light, fog and the lift of the exposure (godot_plant_environment.hpp)
    light_plant_environment(scene, sky, overcast);
}

}  // namespace ase::adp::godot

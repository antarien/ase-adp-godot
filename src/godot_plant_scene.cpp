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
 *              the value of the record, a Nil where it carried none. Every frame show_plant_levels hands each plant
 *              its share of body and view card by the level ase-render-data graded it to (flora.lod.v1,
 *              usecase:101657 grade plants by picture height). The bodies on the patch beside the plants -
 *              seeds and the fruit bodies of the cap patches - stand in godot_plant_scene_patch.cpp; both units
 *              find their nodes and templates through godot_plant_scene_parts.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-11
 * @modified    2026-11-25
 * @version     00.00.22.00022 [refine]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_plant_scene_parts.hpp>
#include <ase/adp/godot/godot_asset_showcase_scene.hpp>
#include <ase/adp/godot/godot_plant_environment.hpp>
#include <ase/adp/godot/godot_plant_haze.hpp>
#include <ase/adp/godot/godot_plant_brook.hpp>
#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/godot_plant_look_shader.hpp>
#include <ase/adp/godot/godot_plant_sky.hpp>
#include <ase/adp/godot/godot_plant_templates.hpp>
#include <ase/adp/godot/godot_plant_template_mesh.hpp>
#include <ase/adp/godot/godot_plant_tubes.hpp>
#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>
#include <ase/adp/godot/asset/types.hpp>
#include <ase/adp/godot/cap/types.hpp>
#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/lod/godot_plant_level_scene.hpp>
#include <ase/adp/godot/lod/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/adp/godot/template/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

::godot::Ref<::godot::Mesh> template_mesh(const GodotHostResourceManager& resources, uint32_t code) {
    PlantMeshArrays arrays;
    std::vector<float> tangents;
    (void)build_template(resources.get_template_corners(), resources.get_template_triangles(), code, arrays,
                         tangents);
    return ::godot::Ref<::godot::Mesh>(template_mesh_of(arrays, tangents));
}

namespace {

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
 * coarse wood disks of its cut stubs (GODOT_PLANT_SCARS, GODOT_PLANT_SCAR_TWIGS) and its five empty instance nodes:
 * the fine and the coarse leaves (TREEGEN T.6), the male and female catkins and the fruit stands.
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
    for (const char* part :
         {GODOT_PLANT_LEAVES, GODOT_PLANT_LEAVES_CORS, GODOT_PLANT_MALE, GODOT_PLANT_FEMALE, GODOT_PLANT_FRUIT}) {
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
    // the shared billboard pages of all plants (LOD 4): its pages come with the first slot they hold
    (void)add_part<::godot::Node>(scene, GODOT_PLANT_BILLBOARD_ATLAS);
    (void)add_part<::godot::MultiMeshInstance3D>(scene, GODOT_PLANT_SEEDS);
    (void)add_part<::godot::Node3D>(scene, GODOT_PLANT_CAPS);
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
        surface.discs = discs;  // the marks of a later patch_ground follow only the discs and beds that changed since
        surface.beds = beds;
        surface.cells = cells;
        surface.revision += 1u;  // a ground built anew moves every place a reader derived from (P0692)
        const ::godot::Ref<::godot::ArrayMesh> built = mesh_of(surface.arrays);
        node->set_mesh(built);
        hold_ground_surface(built, surface);
    }
    shade_plant_ground(scene, step_m, material);
}

void shade_plant_ground(::godot::Node3D* scene, float step_m, const ::godot::Ref<::godot::Material>& material) {
    ::godot::MeshInstance3D* node = part_of<::godot::MeshInstance3D>(scene, GODOT_PLANT_GROUND);
    if (node == nullptr) {
        return;
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
    // a plant whose body has no origin any more leaves the scene and gives its billboard slot back
    ::godot::Node* atlas = part_of<::godot::Node>(scene, GODOT_PLANT_BILLBOARD_ATLAS);
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
            drop_impostor(plant, atlas);
            bodies->remove_child(plant);
            plant->queue_free();
        }
    }

    // the shapes: leaves, catkins and fruit stands on the templates of their files, and while a file is missing on
    // the port template that stands in - the leaf, the fine cylinder - in the error material (03.2 point 3)
    const ::godot::Ref<::godot::Mesh> fine = template_mesh(resources, GODOT_TMPL_FINE);
    const ::godot::Ref<::godot::Mesh> stand_in_leaf = template_mesh(resources, GODOT_TMPL_LEAF);
    const ::godot::Ref<::godot::Mesh> leaf = look.leaf.shape.is_valid() ? look.leaf.shape : stand_in_leaf;
    // the leaves of a body in the far band on the coarse 1 x 1 card of ase-render-data (TREEGEN T.6): four corners
    // and two triangles a face, never the fine lattice of the file
    const ::godot::Ref<::godot::Mesh> coarse_leaf = template_mesh(resources, GODOT_TMPL_LEAF_CORS);
    const ::godot::Ref<::godot::Mesh> male = look.male.shape.is_valid() ? look.male.shape : fine;
    const ::godot::Ref<::godot::Mesh> female = look.female.shape.is_valid() ? look.female.shape : fine;
    const ::godot::Ref<::godot::Mesh> fruit = look.fruit.shape.is_valid() ? look.fruit.shape : fine;

    // the instanced classes: which half of its records a bundle draws, with which custom data and sway. The
    // internodes are no bundle any more - their bark is one tube per axis (usecase:101613), the shown orders reach
    // its shader - so every bundle draws all its records, the half above GODOT_ORDER_EVERY. The leaves carry the
    // bend of their cards as custom data (PLAN_ASE_VIVARIUM_TREEGEN T.6); the vertex shader lifts the lattice by it.
    // The leaves are two bundles: the card each leaf record names (its twelfth value) puts it on the fine card of its
    // file near the observer or on the coarse 1 x 1 card in the far band (TREEGEN T.6).
    struct Bundle {
        const char* node;
        uint32_t organ_class;
        uint32_t card;
        std::vector<float> PlantBodyRows::*half;
        std::vector<float> PlantBodyRows::*custom;
        std::vector<float> PlantBodyRows::*sway;
        const ::godot::Ref<::godot::Mesh>* shape;
        const ::godot::Ref<::godot::Material>* material;
    };
    const Bundle bundles[] = {
        {GODOT_PLANT_LEAVES, GODOT_ORGAN_LEAF, GODOT_TMPL_LEAF, &PlantBodyRows::above, &PlantBodyRows::above_custom,
         &PlantBodyRows::above_sway, &leaf, &look.leaf.material},
        {GODOT_PLANT_LEAVES_CORS, GODOT_ORGAN_LEAF, GODOT_TMPL_LEAF_CORS, &PlantBodyRows::above,
         &PlantBodyRows::above_custom, &PlantBodyRows::above_sway, &coarse_leaf, &look.leaf.material},
        {GODOT_PLANT_MALE, GODOT_ORGAN_MALE, GODOT_TMPL_LEAF, &PlantBodyRows::above, nullptr, &PlantBodyRows::above_sway,
         &male, &look.male.material},
        {GODOT_PLANT_FEMALE, GODOT_ORGAN_FEMALE, GODOT_TMPL_LEAF, &PlantBodyRows::above, nullptr,
         &PlantBodyRows::above_sway, &female, &look.female.material},
        {GODOT_PLANT_FRUIT, GODOT_ORGAN_FRUIT, GODOT_TMPL_LEAF, &PlantBodyRows::above, nullptr,
         &PlantBodyRows::above_sway, &fruit, &look.fruit.material},
    };
    // the records of every bundle by body, in the order of the bundles: two bundles share the leaf class
    std::vector<std::unordered_map<uint32_t, PlantBodyRows>> rows;
    rows.reserve(sizeof(bundles) / sizeof(bundles[0]));
    for (const Bundle& bundle : bundles) {
        rows.push_back(rows_by_body(resources.get_instances(bundle.organ_class), resources.get_organs(bundle.organ_class),
                                    GODOT_ORDER_EVERY, bundle.card));
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
        for (std::size_t b = 0u; b < rows.size(); ++b) {
            const Bundle& bundle = bundles[b];
            const auto found = rows[b].find(origin.first);
            const PlantBodyRows& body_rows = found == rows[b].end() ? none : found->second;
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
        // both leaf bundles, the fine near card and the coarse far card (TREEGEN T.6), wilt, die and colour alike
        for (const char* part : {GODOT_PLANT_LEAVES, GODOT_PLANT_LEAVES_CORS}) {
            ::godot::MultiMeshInstance3D* leaves = part_of<::godot::MultiMeshInstance3D>(plant, part);
            hand_on(leaves, GODOT_SHADER_WILTED, stand.second.condition_known, stand.second.wilted);
            hand_on(leaves, GODOT_SHADER_DEAD, stand.second.condition_known, stand.second.dead);
            // its own autumn colour (usecase:101646 colour the leaves in autumn): unknown hands the default back, the green
            hand_on(leaves, GODOT_SHADER_AUTUMN, stand.second.autumn_known, stand.second.autumn);
        }
    }
}

void show_plant_levels(::godot::Node3D* scene, const std::unordered_map<uint32_t, VivariumLodView>& lods,
                       const std::unordered_map<uint32_t, VivariumCnpyView>& cnpys) {
    ::godot::Node3D* bodies = part_of<::godot::Node3D>(scene, GODOT_PLANT_BODIES);
    if (bodies == nullptr) {
        return;
    }
    // every plant node, graded or not: a body without a record of the level port draws whole (level_share)
    const int64_t prefix = ::godot::String(GODOT_PLANT_BODY_PREFIX).length();
    const int32_t count = bodies->get_child_count();
    for (int32_t i = 0; i < count; ++i) {
        ::godot::Node3D* plant = ::godot::Object::cast_to<::godot::Node3D>(bodies->get_child(i));
        if (plant == nullptr) {
            continue;
        }
        const ::godot::String name = plant->get_name();
        if (!name.begins_with(GODOT_PLANT_BODY_PREFIX)) {
            continue;
        }
        const uint32_t body = static_cast<uint32_t>(name.substr(prefix).to_int());
        const auto found = lods.find(body);
        const PlantLevelShare share = level_share(found == lods.end() ? nullptr : &found->second);
        show_plant_share(plant, share);
        // LOD 5: beyond the picture size of a tree the canopy of the whole stands in, from the rasters of the cell
        const auto cell = cnpys.find(body);
        show_plant_canopy(plant, share.canopy, cell == cnpys.end() ? nullptr : &cell->second);
    }
    // LOD 3: the view tiles that wait are drawn on their view pages, at most the budget of one frame, oldest first
    draw_view_tiles(scene);
    // LOD 4: the billboard pages whose tiles changed in an earlier frame draw now, once
    draw_billboard_pages(part_of<::godot::Node>(scene, GODOT_PLANT_BILLBOARD_ATLAS));
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
        // THE SUN'S SHADOW IS DRAWN ONLY WHERE AN EYE CAN SEE IT (operator order 2026-11-25: under 30 frames per second
        // is a failure of the view). Godot's default splits it into four maps over 100 m and draws every caster into
        // each of them, every frame, because the sun moves: measured 2026-11-24 the render took about 300 ms of a
        // 480 ms frame (h3_pit render_gpu_ms). Two splits keep the sharp map near the camera; the reach ends where the
        // sky port's fog begins (ATM_CONST_*_FOG_NEAR 30 to 50 m, shorter when sight falls), since from there the fog
        // veils the ground and Godot's own fade at the reach's end hands the shadow over to it. The fog's full
        // distance (150 to 350 m) would reach further than Godot's default of 100 m and cost more, not less.
        sun->set_shadow_mode(::godot::DirectionalLight3D::SHADOW_PARALLEL_2_SPLITS);
        if (sky.fog_near > 0.0) {
            sun->set_param(::godot::Light3D::PARAM_SHADOW_MAX_DISTANCE, static_cast<float>(sky.fog_near));
        }
    }
    // under an overcast sky the moons and the stars fade behind the clouds (m.html drawSky, row N02)
    light_plant_sky(scene, sky, overcast, edge);
    // sky colour, ambient light, fog and the lift of the exposure (godot_plant_environment.hpp)
    light_plant_environment(scene, sky, overcast);
}

}  // namespace ase::adp::godot

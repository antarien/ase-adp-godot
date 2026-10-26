/**
 * ASE GODOT ADAPTER - PLANT SCENE IMPLEMENTATION
 *
 * @file        godot_plant_scene.cpp
 * @brief       Camera, sun, environment, ground and the plant nodes of the care view's 3D scene
 * @description See godot_plant_scene.hpp. Every mesh is the arrays of godot_plant_geometry.hpp handed to
 *              Godot as they are, normals included: the ground carries the normals ase-render-data
 *              computed, the cylinders and the leaf the normals of its template ports, and Godot derives
 *              none. Every instance buffer is the records of an instance port handed to MultiMesh as they
 *              are, the internodes with their bark share as custom data. A fill replaces the meshes and
 *              buffers of its nodes and keeps the nodes, so the scene graph stands once and only its
 *              contents change - a plant's node comes with its body and goes with it. Materials and organ
 *              template files are bound once (godot_plant_look.hpp); a fill only hands the bound resources
 *              on. Soil water and condition go to the ground and leaf nodes as instance shader parameters
 *              every frame (mark_plant_stands): the value of the record, a Nil where it carried none.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-11
 * @modified    2026-10-25
 * @version     00.00.06.00000
 */

#include <ase/adp/godot/godot_plant_scene.hpp>
#include <ase/adp/godot/godot_asset_showcase_scene.hpp>
#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/godot_plant_templates.hpp>
#include <ase/adp/godot/godot_scene_exposure.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/camera_attributes_practical.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstddef>
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
 * The arrays of one surface as Godot's mesh arrays, normals included as they stand - the templates
 * those of their ports, the ground those of ase-render-data; Godot derives none. An empty surface is no
 * mesh.
 */
::godot::Ref<::godot::ArrayMesh> mesh_of(const PlantMeshArrays& arrays) {
    if (arrays.positions.empty() || arrays.indices.empty()) {
        return ::godot::Ref<::godot::ArrayMesh>();
    }
    ::godot::PackedVector3Array positions;
    positions.resize(static_cast<int64_t>(arrays.positions.size() / 3u));
    for (std::size_t v = 0u; v < arrays.positions.size() / 3u; ++v) {
        positions.set(static_cast<int64_t>(v), ::godot::Vector3(arrays.positions[v * 3u], arrays.positions[v * 3u + 1u],
                                                                arrays.positions[v * 3u + 2u]));
    }
    ::godot::PackedVector2Array uvs;
    uvs.resize(static_cast<int64_t>(arrays.uvs.size() / 2u));
    for (std::size_t v = 0u; v < arrays.uvs.size() / 2u; ++v) {
        uvs.set(static_cast<int64_t>(v), ::godot::Vector2(arrays.uvs[v * 2u], arrays.uvs[v * 2u + 1u]));
    }
    ::godot::PackedInt32Array indices;
    indices.resize(static_cast<int64_t>(arrays.indices.size()));
    for (std::size_t i = 0u; i < arrays.indices.size(); ++i) {
        indices.set(static_cast<int64_t>(i), arrays.indices[i]);
    }
    ::godot::Array surface;
    surface.resize(::godot::Mesh::ARRAY_MAX);
    surface[::godot::Mesh::ARRAY_VERTEX] = positions;
    surface[::godot::Mesh::ARRAY_TEX_UV] = uvs;
    surface[::godot::Mesh::ARRAY_INDEX] = indices;
    ::godot::PackedVector3Array normals;
    normals.resize(static_cast<int64_t>(arrays.normals.size() / 3u));
    for (std::size_t v = 0u; v < arrays.normals.size() / 3u; ++v) {
        normals.set(static_cast<int64_t>(v), ::godot::Vector3(arrays.normals[v * 3u], arrays.normals[v * 3u + 1u],
                                                              arrays.normals[v * 3u + 2u]));
    }
    surface[::godot::Mesh::ARRAY_NORMAL] = normals;
    // the vertex colours where the surface carries them: the cover of the ground (mark_ground)
    if (!arrays.colors.empty()) {
        ::godot::PackedColorArray colors;
        colors.resize(static_cast<int64_t>(arrays.colors.size() / 4u));
        for (std::size_t v = 0u; v < arrays.colors.size() / 4u; ++v) {
            colors.set(static_cast<int64_t>(v), ::godot::Color(arrays.colors[v * 4u], arrays.colors[v * 4u + 1u],
                                                               arrays.colors[v * 4u + 2u], arrays.colors[v * 4u + 3u]));
        }
        surface[::godot::Mesh::ARRAY_COLOR] = colors;
    }
    ::godot::Ref<::godot::ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(::godot::Mesh::PRIMITIVE_TRIANGLES, surface);
    return mesh;
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

/**
 * Puts the 3x4 records of `rows` - twelve values each, as the instance port carried them - on the
 * MultiMesh of `node`, each drawing `shape`; `material` overrides its surfaces, an empty one leaves the
 * template's own. With `custom`, every instance carries its GODOT_INSTANCE_CUSTOM_VALUES after its twelve
 * values (INSTANCE_CUSTOM in the shader: the bark share of an internode). No node, nothing.
 */
void fill_instances(::godot::MultiMeshInstance3D* node, const ::godot::Ref<::godot::Mesh>& shape,
                    const std::vector<float>& rows, const std::vector<float>* custom,
                    const ::godot::Ref<::godot::Material>& material) {
    if (node == nullptr) {
        return;
    }
    const std::size_t count = rows.size() / GODOT_INSTANCE_VALUES;
    const std::size_t extra = custom == nullptr ? 0u : GODOT_INSTANCE_CUSTOM_VALUES;
    ::godot::Ref<::godot::MultiMesh> multimesh;
    multimesh.instantiate();
    multimesh->set_transform_format(::godot::MultiMesh::TRANSFORM_3D);
    multimesh->set_use_custom_data(custom != nullptr);
    multimesh->set_mesh(shape);
    multimesh->set_instance_count(static_cast<int32_t>(count));
    // one stride per instance: the twelve values of its record, then its custom data
    const std::size_t stride = GODOT_INSTANCE_VALUES + extra;
    ::godot::PackedFloat32Array buffer;
    buffer.resize(static_cast<int64_t>(count * stride));
    for (std::size_t n = 0u; n < count; ++n) {
        for (std::size_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
            buffer.set(static_cast<int64_t>(n * stride + v), rows[n * GODOT_INSTANCE_VALUES + v]);
        }
        for (std::size_t v = 0u; v < extra; ++v) {
            buffer.set(static_cast<int64_t>(n * stride + GODOT_INSTANCE_VALUES + v), (*custom)[n * extra + v]);
        }
    }
    multimesh->set_buffer(buffer);
    node->set_multimesh(multimesh);
    node->set_material_override(material);
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

/** The node of the plant on `body` under `bodies`, made with its six empty instance nodes when it has none yet. */
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
    for (const char* part : {GODOT_PLANT_TUBES, GODOT_PLANT_TWIGS, GODOT_PLANT_LEAVES, GODOT_PLANT_MALE,
                             GODOT_PLANT_FEMALE, GODOT_PLANT_FRUIT}) {
        (void)add_part<::godot::MultiMeshInstance3D>(plant, part);
    }
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

    ::godot::WorldEnvironment* world = add_part<::godot::WorldEnvironment>(scene, GODOT_PLANT_ENVIRONMENT);
    ::godot::Ref<::godot::Environment> environment;
    environment.instantiate();
    environment->set_background(::godot::Environment::BG_COLOR);
    environment->set_ambient_source(::godot::Environment::AMBIENT_SOURCE_COLOR);
    world->set_environment(environment);
    // the camera adapts to the light as an eye does (register D1721): light_plant_scene sets its multiplier
    // every frame from the sky port (scene_exposure); the sky's numbers stay as they are
    ::godot::Ref<::godot::CameraAttributesPractical> exposure;
    exposure.instantiate();
    exposure->set_auto_exposure_enabled(false);
    world->set_camera_attributes(exposure);

    (void)add_part<::godot::MeshInstance3D>(scene, GODOT_PLANT_GROUND);
    (void)add_part<::godot::Node3D>(scene, GODOT_PLANT_BODIES);
    (void)add_part<::godot::MultiMeshInstance3D>(scene, GODOT_PLANT_SEEDS);
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
                       const ::godot::Ref<::godot::Material>& material) {
    ::godot::MeshInstance3D* node = part_of<::godot::MeshInstance3D>(scene, GODOT_PLANT_GROUND);
    if (node == nullptr) {
        return;
    }
    PlantMeshArrays arrays;
    (void)build_ground(samples, cells, arrays);
    (void)mark_ground(discs, beds, arrays);
    node->set_mesh(mesh_of(arrays));
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

    // the shapes: the fine and the coarse cylinder of the template ports for the internodes; leaves, catkins
    // and fruit stands on the templates of their files, and while a file is missing on the port template that
    // stands in - the leaf, the fine cylinder - in the error material (03.2 point 3)
    const ::godot::Ref<::godot::Mesh> fine = template_mesh(resources, GODOT_TMPL_FINE);
    const ::godot::Ref<::godot::Mesh> coarse = template_mesh(resources, GODOT_TMPL_CORS);
    const ::godot::Ref<::godot::Mesh> stand_in_leaf = template_mesh(resources, GODOT_TMPL_LEAF);
    const ::godot::Ref<::godot::Mesh> leaf = look.leaf.shape.is_valid() ? look.leaf.shape : stand_in_leaf;
    const ::godot::Ref<::godot::Mesh> male = look.male.shape.is_valid() ? look.male.shape : fine;
    const ::godot::Ref<::godot::Mesh> female = look.female.shape.is_valid() ? look.female.shape : fine;
    const ::godot::Ref<::godot::Mesh> fruit = look.fruit.shape.is_valid() ? look.fruit.shape : fine;

    // the records of every drawn class by body; only the internodes split at the shown orders
    std::unordered_map<uint32_t, PlantBodyRows> rows[GODOT_DRAWN_COUNT];
    for (uint32_t c = 0u; c < GODOT_DRAWN_COUNT; ++c) {
        const double split = c == GODOT_ORGAN_SEGMENT ? static_cast<double>(shown_orders) : GODOT_ORDER_EVERY;
        rows[c] = rows_by_body(resources.get_instances(c), resources.get_organs(c), split);
    }
    // the internodes carry their bark share as custom data, the other classes none
    struct Bundle {
        const char* node;
        uint32_t organ_class;
        std::vector<float> PlantBodyRows::*half;
        std::vector<float> PlantBodyRows::*custom;
        const ::godot::Ref<::godot::Mesh>* shape;
        const ::godot::Ref<::godot::Material>* material;
    };
    const Bundle bundles[] = {
        {GODOT_PLANT_TUBES, GODOT_ORGAN_SEGMENT, &PlantBodyRows::up_to, &PlantBodyRows::up_to_custom, &fine,
         &look.bark},
        {GODOT_PLANT_TWIGS, GODOT_ORGAN_SEGMENT, &PlantBodyRows::above, &PlantBodyRows::above_custom, &coarse,
         &look.bark},
        {GODOT_PLANT_LEAVES, GODOT_ORGAN_LEAF, &PlantBodyRows::above, nullptr, &leaf, &look.leaf.material},
        {GODOT_PLANT_MALE, GODOT_ORGAN_MALE, &PlantBodyRows::above, nullptr, &male, &look.male.material},
        {GODOT_PLANT_FEMALE, GODOT_ORGAN_FEMALE, &PlantBodyRows::above, nullptr, &female, &look.female.material},
        {GODOT_PLANT_FRUIT, GODOT_ORGAN_FRUIT, &PlantBodyRows::above, nullptr, &fruit, &look.fruit.material},
    };

    // every plant at the stem base of its body, its records as the ports carried them
    const PlantBodyRows none;
    for (const auto& origin : origins) {
        ::godot::Node3D* plant = plant_node(bodies, origin.first);
        plant->set_position(::godot::Vector3(origin.second.x, origin.second.y, origin.second.z));
        for (const Bundle& bundle : bundles) {
            const auto found = rows[bundle.organ_class].find(origin.first);
            const PlantBodyRows& body_rows = found == rows[bundle.organ_class].end() ? none : found->second;
            const std::vector<float>* custom = bundle.custom == nullptr ? nullptr : &(body_rows.*bundle.custom);
            fill_instances(part_of<::godot::MultiMeshInstance3D>(plant, bundle.node), *bundle.shape,
                           body_rows.*bundle.half, custom, *bundle.material);
        }
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
        hand_on(part_of<::godot::MultiMeshInstance3D>(plant, GODOT_PLANT_LEAVES), GODOT_SHADER_CONDITION,
                stand.second.condition_known, stand.second.condition);
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
        const float at[3] = {static_cast<float>(origin->second.x + seed.x),
                             static_cast<float>(origin->second.y + seed.y),
                             static_cast<float>(origin->second.z + seed.z)};
        for (uint32_t r = 0u; r < 3u; ++r) {
            for (uint32_t c = 0u; c < 3u; ++c) {
                rows.push_back(r == c ? scale : 0.0f);
            }
            rows.push_back(at[r]);
        }
    }
    fill_instances(node, shape, rows, nullptr, look.nutlet.material);
}

void light_plant_scene(::godot::Node3D* scene, const VivariumSkyView& sky) {
    if (!sky.known) {
        return;
    }
    const float toward_sun[3] = {static_cast<float>(sky.sun_x), static_cast<float>(sky.sun_y),
                                 static_cast<float>(sky.sun_z)};
    const float sun_rgb[3] = {static_cast<float>(sky.sun_r), static_cast<float>(sky.sun_g),
                              static_cast<float>(sky.sun_b)};
    light_asset_showcase(scene, toward_sun, sun_rgb, static_cast<float>(sky.sun_intensity));
    ::godot::DirectionalLight3D* sun = part_of<::godot::DirectionalLight3D>(scene, GODOT_SHOWCASE_SUN);
    if (sun != nullptr) {
        sun->set_shadow(true);
    }

    ::godot::WorldEnvironment* world = part_of<::godot::WorldEnvironment>(scene, GODOT_PLANT_ENVIRONMENT);
    if (world == nullptr || world->get_environment().is_null()) {
        return;
    }
    const ::godot::Ref<::godot::Environment> environment = world->get_environment();
    environment->set_bg_color(::godot::Color(static_cast<float>(sky.background_r), static_cast<float>(sky.background_g),
                                             static_cast<float>(sky.background_b)));
    environment->set_ambient_light_color(::godot::Color(static_cast<float>(sky.ambient_r),
                                                        static_cast<float>(sky.ambient_g),
                                                        static_cast<float>(sky.ambient_b)));
    environment->set_ambient_light_energy(static_cast<float>(sky.ambient_intensity));
    environment->set_fog_enabled(true);
    environment->set_fog_mode(::godot::Environment::FOG_MODE_DEPTH);
    environment->set_fog_light_color(::godot::Color(static_cast<float>(sky.fog_r), static_cast<float>(sky.fog_g),
                                                    static_cast<float>(sky.fog_b)));
    environment->set_fog_depth_begin(static_cast<float>(sky.fog_near));
    environment->set_fog_depth_end(static_cast<float>(sky.fog_far));
    const ::godot::Ref<::godot::CameraAttributes> exposure = world->get_camera_attributes();
    if (exposure.is_valid()) {
        exposure->set_exposure_multiplier(scene_exposure(sky));
    }
}

}  // namespace ase::adp::godot

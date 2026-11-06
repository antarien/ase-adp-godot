/**
 * ASE GODOT ADAPTER - THE WATER OF THE CARE VIEW
 *
 * @file        godot_plant_brook.cpp
 * @brief       The node of the water in the care view's 3D scene and its fill
 * @description See godot_plant_brook.hpp. The water table is drawn after the opaque ground, transparent; the
 *              ground above NN0 hides it by the depth test, which draws the shore. Its vertex colour carries
 *              velocity, depth and course into the shader.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-01
 * @modified    2026-11-06
 * @version     00.00.03.00000
 */

#include <ase/adp/godot/godot_plant_brook.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_water.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The node of `parent` with that name, cast to its class; nullptr when it has none. */
template <typename T>
T* brook_part(::godot::Node* parent, const char* name) {
    return ::godot::Object::cast_to<T>(parent->get_node_or_null(::godot::NodePath(name)));
}

/** The arrays of build_water_table as one surface of an ArrayMesh; a dry ground is no mesh. */
::godot::Ref<::godot::ArrayMesh> water_mesh(const PlantMeshArrays& arrays) {
    if (arrays.positions.empty() || arrays.indices.empty()) {
        return ::godot::Ref<::godot::ArrayMesh>();
    }
    const auto vertices = static_cast<int64_t>(arrays.positions.size() / 3u);
    ::godot::PackedVector3Array positions;
    ::godot::PackedVector3Array normals;
    ::godot::PackedVector2Array uvs;
    ::godot::PackedColorArray colors;
    positions.resize(vertices);
    normals.resize(vertices);
    uvs.resize(vertices);
    colors.resize(vertices);
    for (int64_t v = 0; v < vertices; ++v) {
        const auto i = static_cast<std::size_t>(v);
        positions.set(v, ::godot::Vector3(arrays.positions[i * 3u], arrays.positions[i * 3u + 1u],
                                          arrays.positions[i * 3u + 2u]));
        normals.set(v, ::godot::Vector3(arrays.normals[i * 3u], arrays.normals[i * 3u + 1u],
                                        arrays.normals[i * 3u + 2u]));
        uvs.set(v, ::godot::Vector2(arrays.uvs[i * 2u], arrays.uvs[i * 2u + 1u]));
        colors.set(v, ::godot::Color(arrays.colors[i * 4u], arrays.colors[i * 4u + 1u], arrays.colors[i * 4u + 2u],
                                     arrays.colors[i * 4u + 3u]));
    }
    ::godot::PackedInt32Array indices;
    indices.resize(static_cast<int64_t>(arrays.indices.size()));
    for (std::size_t i = 0u; i < arrays.indices.size(); ++i) {
        indices.set(static_cast<int64_t>(i), arrays.indices[i]);
    }
    ::godot::Array surface;
    surface.resize(::godot::Mesh::ARRAY_MAX);
    surface[::godot::Mesh::ARRAY_VERTEX] = positions;
    surface[::godot::Mesh::ARRAY_NORMAL] = normals;
    surface[::godot::Mesh::ARRAY_TEX_UV] = uvs;
    surface[::godot::Mesh::ARRAY_COLOR] = colors;
    surface[::godot::Mesh::ARRAY_INDEX] = indices;
    ::godot::Ref<::godot::ArrayMesh> mesh;
    mesh.instantiate();
    mesh->add_surface_from_arrays(::godot::Mesh::PRIMITIVE_TRIANGLES, surface);
    return mesh;
}

}  // namespace

void build_plant_water(::godot::Node3D* scene) {
    ::godot::Ref<::godot::Shader> shader;
    shader.instantiate();
    shader->set_code(GODOT_WATER_SHADER_CODE);
    ::godot::Ref<::godot::ShaderMaterial> material;
    material.instantiate();
    material->set_shader(shader);
    material->set_shader_parameter(GODOT_WATER_VELOCITY_FULL, GODOT_WATER_VELOCITY_FULL_M_S);
    material->set_shader_parameter(GODOT_WATER_SHALLOW,
                                   ::godot::Color(GODOT_WATER_SHALLOW_R, GODOT_WATER_SHALLOW_G, GODOT_WATER_SHALLOW_B));
    material->set_shader_parameter(GODOT_WATER_DEEP,
                                   ::godot::Color(GODOT_WATER_DEEP_R, GODOT_WATER_DEEP_G, GODOT_WATER_DEEP_B));

    ::godot::MeshInstance3D* water = memnew(::godot::MeshInstance3D);
    water->set_name(GODOT_PLANT_WATER);
    water->set_material_override(material);
    water->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    scene->add_child(water);
}

uint32_t fill_plant_water(::godot::Node3D* scene, const std::vector<VivariumStreamView>& streams,
                          const std::vector<VivariumTerrainView>& samples,
                          const std::vector<VivariumGroundCellView>& cells) {
    ::godot::MeshInstance3D* node = brook_part<::godot::MeshInstance3D>(scene, GODOT_PLANT_WATER);
    if (node == nullptr) {
        return 0u;
    }
    PlantMeshArrays arrays;
    const uint32_t wet = build_water_table(streams, samples, cells, arrays);
    node->set_mesh(water_mesh(arrays));
    return wet;
}

}  // namespace ase::adp::godot

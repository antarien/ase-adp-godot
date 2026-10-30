/**
 * ASE GODOT ADAPTER - THE SHIMMER OF HOT AIR IN THE CARE VIEW
 *
 * @file        godot_plant_haze.cpp
 * @brief       The shimmer of hot air over the care view's 3D scene
 * @description See godot_plant_haze.hpp. The quad is a child of the camera, so it moves with every turn of
 *              the view; its vertex stage lays it over the whole clip space, so its size on screen never
 *              depends on the field of view. It reads the drawn image and the depth of the frame, which puts
 *              it into the transparent pass after everything opaque is drawn. It casts no shadow, takes no
 *              fog and writes no depth.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_130
 * @category    render/pipeline/stage
 * @usecase     usecase:101515 - veil sight with weather
 * @actor       Fog of War (FOW)
 * @created     2026-10-28
 * @modified    2026-10-28
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_haze.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

namespace {

/** The node of `parent` with that name, cast to its class; nullptr when it has none. */
template <typename T>
T* haze_part(::godot::Node* parent, const char* name) {
    return ::godot::Object::cast_to<T>(parent->get_node_or_null(::godot::NodePath(name)));
}

}  // namespace

void build_plant_haze(::godot::Node3D* scene) {
    ::godot::Camera3D* eye = haze_part<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    if (eye == nullptr) {
        return;
    }
    ::godot::Ref<::godot::Shader> shader;
    shader.instantiate();
    shader->set_code(GODOT_HAZE_SHADER_CODE);
    ::godot::Ref<::godot::ShaderMaterial> veil;
    veil.instantiate();
    veil->set_shader(shader);
    veil->set_shader_parameter(GODOT_HAZE_ROWS, GODOT_HAZE_WAVE_ROWS);
    veil->set_shader_parameter(GODOT_HAZE_SPEED, GODOT_HAZE_WAVE_SPEED);
    veil->set_shader_parameter(GODOT_HAZE_REACH, GODOT_HAZE_WAVE_REACH);

    ::godot::Ref<::godot::QuadMesh> quad;
    quad.instantiate();
    quad->set_size(::godot::Vector2(GODOT_HAZE_QUAD_SIZE, GODOT_HAZE_QUAD_SIZE));
    quad->set_material(veil);

    ::godot::MeshInstance3D* haze = memnew(::godot::MeshInstance3D);
    haze->set_name(GODOT_PLANT_HAZE);
    haze->set_mesh(quad);
    haze->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    haze->set_position(::godot::Vector3(0.0f, 0.0f, -GODOT_HAZE_QUAD_DEPTH));
    haze->set_visible(false);
    eye->add_child(haze);
}

void veil_plant_scene(::godot::Node3D* scene, const VivariumSkyView& sky) {
    if (!sky.known) {
        return;
    }
    ::godot::Camera3D* eye = haze_part<::godot::Camera3D>(scene, GODOT_PLANT_CAMERA);
    if (eye == nullptr) {
        return;
    }
    ::godot::MeshInstance3D* haze = haze_part<::godot::MeshInstance3D>(eye, GODOT_PLANT_HAZE);
    if (haze == nullptr) {
        return;
    }
    // below the onset ase-atmosphere hands 0: the air stands clear, and the quad draws nothing
    const bool shimmers = sky.fog_shimmer > 0.0;
    haze->set_visible(shimmers);
    if (!shimmers) {
        return;
    }
    const ::godot::Ref<::godot::QuadMesh> quad = haze->get_mesh();
    if (quad.is_null()) {
        return;
    }
    const ::godot::Ref<::godot::ShaderMaterial> veil = quad->get_material();
    if (veil.is_null()) {
        return;
    }
    veil->set_shader_parameter(GODOT_HAZE_SHIMMER, static_cast<float>(sky.fog_shimmer));
    veil->set_shader_parameter(GODOT_HAZE_NEAR, static_cast<float>(sky.fog_near));
    veil->set_shader_parameter(GODOT_HAZE_FAR, static_cast<float>(sky.fog_far));
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - ASSET SHOWCASE SCENE IMPLEMENTATION
 *
 * @file        godot_asset_showcase_scene.cpp
 * @brief       Camera, spheres, planes and models of the asset showcase; the light of the host's sun
 * @description See godot_asset_showcase_scene.hpp. Files are read through the one loader of the adapter
 *              (godot_asset_loader.hpp), which the plant scene shares: a material is read for its
 *              material, the scene of a template is kept and placed. The sun light points from the
 *              direction the view hands in towards the origin, with east as its up axis when the sun
 *              stands upright.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:100347 - show approved assets
 * @actor       Asset Showcase (ASSETSHOW)
 * @created     2026-10-09
 * @modified    2026-10-11
 * @version     00.00.02.00001
 */

#include <ase/adp/godot/godot_asset_showcase_scene.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/plane_mesh.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>

namespace ase::adp::godot {

namespace {

::godot::String text_of(const std::string& value) {
    return ::godot::String::utf8(value.c_str());
}

void add_mesh(::godot::Node3D* root, const ::godot::Ref<::godot::Mesh>& mesh, const ::godot::Ref<::godot::Material>& material,
              const ::godot::Vector3& at, const ::godot::String& name) {
    ::godot::MeshInstance3D* instance = memnew(::godot::MeshInstance3D);
    instance->set_name(name);
    instance->set_mesh(mesh);
    instance->set_material_override(material);
    instance->set_position(at);
    root->add_child(instance);
}

}  // namespace

::godot::Node3D* build_asset_showcase(::godot::Node* parent, const AssetShowcasePlan& plan,
                                      const VivariumAssetBinding& binding, const ::godot::String& asset_root) {
    ::godot::Node3D* root = memnew(::godot::Node3D);
    root->set_name(GODOT_SHOWCASE_ROOT);
    parent->add_child(root);

    float position[3];
    float target[3];
    showcase_camera(plan, position, target);
    ::godot::Camera3D* camera = memnew(::godot::Camera3D);
    camera->set_keep_aspect_mode(::godot::Camera3D::KEEP_WIDTH);
    camera->set_fov(GODOT_SHOWCASE_FOV_DEGREES);
    root->add_child(camera);
    camera->look_at_from_position(::godot::Vector3(position[0], position[1], position[2]),
                                  ::godot::Vector3(target[0], target[1], target[2]));
    camera->set_current(true);

    ::godot::Ref<::godot::SphereMesh> sphere;
    sphere.instantiate();
    sphere->set_radius(GODOT_SHOWCASE_SPHERE_RADIUS);
    sphere->set_height(2.0f * GODOT_SHOWCASE_SPHERE_RADIUS);
    ::godot::Ref<::godot::PlaneMesh> plane;
    plane.instantiate();
    plane->set_size(::godot::Vector2(GODOT_SHOWCASE_SLOT, GODOT_SHOWCASE_SLOT));
    for (std::size_t i = 0u; i < plan.materials.size(); ++i) {
        const ::godot::Ref<::godot::Material> material = bound_material(binding, asset_root, plan.materials[i]);
        add_mesh(root, sphere, material, ::godot::Vector3(plan.material_x[i], GODOT_SHOWCASE_SPHERE_RADIUS, 0.0f),
                 text_of(plan.materials[i] + " sphere"));
        add_mesh(root, plane, material, ::godot::Vector3(plan.material_x[i], 0.0f, GODOT_SHOWCASE_PLANE_ROW),
                 text_of(plan.materials[i] + " plane"));
    }

    ::godot::Ref<::godot::BoxMesh> box;
    box.instantiate();
    box->set_size(::godot::Vector3(GODOT_SHOWCASE_SLOT, GODOT_SHOWCASE_SLOT, GODOT_SHOWCASE_SLOT) * 0.5f);
    for (std::size_t i = 0u; i < plan.templates.size(); ++i) {
        const std::string& name = plan.templates[i];
        const ::godot::Vector3 slot(plan.template_x[i], 0.0f, GODOT_SHOWCASE_TEMPLATE_ROW);
        const auto bound = binding.files.find(name);
        ::godot::Node* scene = bound != binding.files.end() ? load_asset(asset_root, bound->second) : nullptr;
        ::godot::Node3D* model = scene != nullptr ? ::godot::Object::cast_to<::godot::Node3D>(scene) : nullptr;
        ::godot::MeshInstance3D* mesh = model != nullptr ? first_mesh(model) : nullptr;
        if (mesh == nullptr || !mesh->get_mesh().is_valid()) {
            if (scene != nullptr) {
                memdelete(scene);
            }
            add_mesh(root, box, error_material(binding, name), slot, text_of(name + " missing"));
            continue;
        }
        const float scale = template_scale(mesh->get_mesh()->get_aabb().get_longest_axis_size());
        model->set_name(text_of(name));
        model->set_scale(::godot::Vector3(scale, scale, scale));
        model->set_position(slot);
        root->add_child(model);
    }
    return root;
}

void light_asset_showcase(::godot::Node3D* showcase, const float* toward_sun, const float* sun_rgb, float energy) {
    ::godot::DirectionalLight3D* sun =
        ::godot::Object::cast_to<::godot::DirectionalLight3D>(showcase->get_node_or_null(::godot::NodePath(GODOT_SHOWCASE_SUN)));
    if (sun == nullptr) {
        sun = memnew(::godot::DirectionalLight3D);
        sun->set_name(GODOT_SHOWCASE_SUN);
        showcase->add_child(sun);
    }
    // The light shines along its -Z: placed on the side of the sun and turned to the origin.
    const ::godot::Vector3 toward(toward_sun[0], toward_sun[1], toward_sun[2]);
    const bool upright = toward.normalized().y > GODOT_GEOMETRY_UPRIGHT || toward.normalized().y < -GODOT_GEOMETRY_UPRIGHT;
    sun->look_at_from_position(toward.normalized(), ::godot::Vector3(0.0f, 0.0f, 0.0f),
                               upright ? ::godot::Vector3(1.0f, 0.0f, 0.0f) : ::godot::Vector3(0.0f, 1.0f, 0.0f));
    sun->set_color(::godot::Color(sun_rgb[0], sun_rgb[1], sun_rgb[2]));
    sun->set_param(::godot::Light3D::PARAM_ENERGY, energy);
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - ASSET LOADER IMPLEMENTATION
 *
 * @file        godot_asset_loader.cpp
 * @brief       GLTFDocument reads a bound file; the scene it generates is read for its mesh or material
 * @description See godot_asset_loader.hpp. The scene GLTFDocument generates for a material or a template
 *              is read for what the caller asks and freed; the resource it hands out keeps itself alive
 *              by its reference. The asset showcase keeps a template's scene and places it - it frees
 *              nothing it hands to the tree.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    render/pipeline/stage
 * @usecase     usecase:6451 - load flora blueprints
 * @actor       Flora BDIEX Framework (FLOBDIEXFRW)
 * @created     2026-10-11
 * @modified    2026-10-11
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/gltf_document.hpp>
#include <godot_cpp/classes/gltf_state.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

::godot::Node* load_asset(const ::godot::String& asset_root, const std::string& file) {
    ::godot::Ref<::godot::GLTFDocument> document;
    document.instantiate();
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    if (document->append_from_file(asset_root.path_join(::godot::String::utf8(file.c_str())), state) != ::godot::OK) {
        return nullptr;
    }
    return document->generate_scene(state);
}

::godot::MeshInstance3D* first_mesh(::godot::Node* node) {
    ::godot::MeshInstance3D* mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(node);
    if (mesh != nullptr) {
        return mesh;
    }
    for (int32_t i = 0; i < node->get_child_count(); ++i) {
        ::godot::MeshInstance3D* found = first_mesh(node->get_child(i));
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

::godot::Ref<::godot::StandardMaterial3D> error_material(const VivariumAssetBinding& binding, const std::string& name) {
    ::godot::Ref<::godot::StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(::godot::Color::hex(asset_color(binding, name, GODOT_COLOR_ASSET_MISSING)));
    return material;
}

::godot::Ref<::godot::Material> bound_material(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                               const std::string& name) {
    const auto bound = binding.files.find(name);
    if (bound == binding.files.end()) {
        return error_material(binding, name);
    }
    ::godot::Node* scene = load_asset(asset_root, bound->second);
    if (scene == nullptr) {
        return error_material(binding, name);
    }
    ::godot::MeshInstance3D* mesh = first_mesh(scene);
    ::godot::Ref<::godot::Material> material =
        mesh != nullptr && mesh->get_mesh().is_valid() ? mesh->get_active_material(0) : ::godot::Ref<::godot::Material>();
    memdelete(scene);
    if (material.is_valid()) {
        return material;
    }
    return error_material(binding, name);
}

::godot::Ref<::godot::Mesh> bound_template(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                           const std::string& name) {
    const auto bound = binding.files.find(name);
    if (bound == binding.files.end()) {
        return ::godot::Ref<::godot::Mesh>();
    }
    ::godot::Node* scene = load_asset(asset_root, bound->second);
    if (scene == nullptr) {
        return ::godot::Ref<::godot::Mesh>();
    }
    ::godot::MeshInstance3D* mesh = first_mesh(scene);
    // the mesh resource outlives the scene that carried it: the reference taken here holds it
    const ::godot::Ref<::godot::Mesh> shape = mesh != nullptr ? mesh->get_mesh() : ::godot::Ref<::godot::Mesh>();
    memdelete(scene);
    return shape;
}

}  // namespace ase::adp::godot

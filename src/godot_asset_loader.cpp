/**
 * ASE GODOT ADAPTER - ASSET LOADER IMPLEMENTATION
 *
 * @file        godot_asset_loader.cpp
 * @brief       GLTFDocument reads a bound file; the scene it generates is read for its mesh or material
 * @description See godot_asset_loader.hpp. The scene GLTFDocument generates for a material or a template
 *              is read for what the caller asks and freed; the resource it hands out keeps itself alive
 *              by its reference. The asset showcase keeps a template's scene and places it - it frees
 *              nothing it hands to the tree. A file whose image list carries a map ending in
 *              GODOT_IMAGE_TRANSLUCENCY (ase-assgen writes it for leaf, catkin, fruit and grass) gets that
 *              map bound as the backlight of its materials, so a thin organ lets the sunlight through.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    render/pipeline/stage
 * @usecase     usecase:6451 - load flora blueprints
 * @actor       Flora BDIEX Framework (FLOBDIEXFRW)
 * @created     2026-10-11
 * @modified    2026-10-22
 * @version     00.00.02.00002
 */

#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/gltf_document.hpp>
#include <godot_cpp/classes/gltf_state.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

::godot::Node* generate(const ::godot::String& asset_root, const std::string& file,
                        const ::godot::Ref<::godot::GLTFState>& state) {
    ::godot::Ref<::godot::GLTFDocument> document;
    document.instantiate();
    if (document->append_from_file(asset_root.path_join(::godot::String::utf8(file.c_str())), state) != ::godot::OK) {
        return nullptr;
    }
    return document->generate_scene(state);
}

// the image list of the document and the textures of the state stand in the same order
::godot::Ref<::godot::Texture2D> translucency_of(const ::godot::Ref<::godot::GLTFState>& state) {
    const ::godot::Dictionary json = state->get_json();
    const ::godot::Array images = json.get(GODOT_GLTF_KEY_IMAGES, ::godot::Array());
    const ::godot::TypedArray<::godot::Ref<::godot::Texture2D>> textures = state->get_images();
    for (int64_t i = 0; i < images.size() && i < textures.size(); ++i) {
        const ::godot::Dictionary image = images[i];
        const ::godot::String label = image.get(GODOT_GLTF_KEY_IMAGE, ::godot::String());
        if (label.ends_with(GODOT_IMAGE_TRANSLUCENCY)) {
            return ::godot::Ref<::godot::Texture2D>(textures[i]);
        }
    }
    return ::godot::Ref<::godot::Texture2D>();
}

void bind_backlight(const ::godot::Ref<::godot::Material>& material, const ::godot::Ref<::godot::Texture2D>& light) {
    ::godot::BaseMaterial3D* pbr = ::godot::Object::cast_to<::godot::BaseMaterial3D>(material.ptr());
    if (pbr == nullptr || light.is_null()) {
        return;
    }
    pbr->set_feature(::godot::BaseMaterial3D::FEATURE_BACKLIGHT, true);
    pbr->set_texture(::godot::BaseMaterial3D::TEXTURE_BACKLIGHT, light);
}

}  // namespace

::godot::Node* load_asset(const ::godot::String& asset_root, const std::string& file) {
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    return generate(asset_root, file, state);
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
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    ::godot::Node* scene = generate(asset_root, bound->second, state);
    if (scene == nullptr) {
        return error_material(binding, name);
    }
    ::godot::MeshInstance3D* mesh = first_mesh(scene);
    ::godot::Ref<::godot::Material> material =
        mesh != nullptr && mesh->get_mesh().is_valid() ? mesh->get_active_material(0) : ::godot::Ref<::godot::Material>();
    memdelete(scene);
    if (material.is_valid()) {
        bind_backlight(material, translucency_of(state));
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
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    ::godot::Node* scene = generate(asset_root, bound->second, state);
    if (scene == nullptr) {
        return ::godot::Ref<::godot::Mesh>();
    }
    ::godot::MeshInstance3D* mesh = first_mesh(scene);
    // the mesh resource outlives the scene that carried it: the reference taken here holds it
    const ::godot::Ref<::godot::Mesh> shape = mesh != nullptr ? mesh->get_mesh() : ::godot::Ref<::godot::Mesh>();
    memdelete(scene);
    if (shape.is_valid()) {
        const ::godot::Ref<::godot::Texture2D> light = translucency_of(state);
        for (int32_t surface = 0; surface < shape->get_surface_count(); ++surface) {
            bind_backlight(shape->surface_get_material(surface), light);
        }
    }
    return shape;
}

}  // namespace ase::adp::godot

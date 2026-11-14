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
 *              A file with a pack beside it (<file>.textures/textures.json, written by the stage,
 *              scripts/stage_native.py of the client) is read without its own images: every pack is
 *              loaded first, GLTFDocument then discards the PNGs of the file, the packs take their
 *              places in the image list of the state, and every texture slot of every material is bound
 *              to its pack as GLTFDocument binds an image on import. A pack that does not load sends the
 *              whole file the unpacked way, with an error line - never a material with a hole.
 *
 *              A DISCARDED IMAGE IS NEVER DECODED: GLTFDocument decodes every embedded PNG before it
 *              looks at the discard setting, and the figure's forty maps cost seconds of the parse that
 *              way. The document extension AseGltfImageSkip answers each image of a discarding state with
 *              a stand-in of one pixel first, so the decode is skipped; GLTFDocument discards the
 *              stand-in as it would have discarded the map.
 *
 *              parse_asset_bytes takes the same way from bytes a caller has read itself: the figure load reads
 *              its file in blocks so the start screen can count every block (godot_figure_load.hpp), then hands
 *              the bytes here and generates the scene in a step of its own.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    render/pipeline/stage
 * @usecase     usecase:6451 - load flora blueprints
 * @actor       Flora Blueprint Keeper (FLORBLUKEE)
 * @created     2026-10-11
 * @modified    2026-11-14
 * @version     00.00.03.00004
 */

#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/types.hpp>

#include <gdextension_interface.h>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/global_constants_binds.hpp>
#include <godot_cpp/classes/gltf_document.hpp>
#include <godot_cpp/classes/gltf_document_extension.hpp>
#include <godot_cpp/classes/gltf_state.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/method_ptrcall.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

// the document extension that answers an image the state discards: GLTFDocument asks every extension for
// the image before it decodes the PNG or JPEG itself (_parse_image_bytes_into_image) and discards the
// decoded image afterwards when the state says so (_parse_image_save_image)
constexpr const char* SKIP_CLASS = "AseGltfImageSkip";              // the class the extension is registered as
constexpr const char* SKIP_ENGINE_CLASS = "GLTFDocumentExtension";  // the engine class its object is built from
constexpr const char* SKIP_HOOK = "_parse_image_data";              // the one virtual it takes over
constexpr uint32_t SKIP_HOOK_HASH = 3201673288U;                    // its signature hash in godot-cpp's binding
constexpr const char* SKIP_HOLDER = "ase_gltf_image_skip";          // the meta key of Engine that holds it
constexpr int32_t SKIP_IMAGE_SIDE = 1;                              // the stand-in: one pixel of FORMAT_L8

// the pack of a file as stage_native.py writes it (TEXTURE_PACK_SUFFIX, TEXTURE_PACK_INDEX there)
constexpr const char* PACK_SUFFIX = ".textures";        // the directory beside the glb: <file>.textures/
constexpr const char* PACK_INDEX = "textures.json";     // its index: the image list of the glb, one pack each
constexpr const char* PACK_KEY_IMAGES = "images";       // index: the packs, each {index, file, sha256}
constexpr const char* PACK_KEY_INDEX = "index";         // pack: its place in the image list of the glb
constexpr const char* PACK_KEY_FILE = "file";           // pack: its file name inside the directory

// the glTF 2.0 keys a material names its maps by
constexpr const char* GLTF_KEY_MATERIALS = "materials";
constexpr const char* GLTF_KEY_TEXTURES = "textures";
constexpr const char* GLTF_KEY_PBR = "pbrMetallicRoughness";
constexpr const char* GLTF_KEY_BASE_COLOR = "baseColorTexture";
constexpr const char* GLTF_KEY_METALLIC_ROUGHNESS = "metallicRoughnessTexture";
constexpr const char* GLTF_KEY_NORMAL = "normalTexture";
constexpr const char* GLTF_KEY_OCCLUSION = "occlusionTexture";
constexpr const char* GLTF_KEY_EMISSIVE = "emissiveTexture";
constexpr const char* GLTF_KEY_INDEX = "index";         // texture reference: which texture of the document
constexpr const char* GLTF_KEY_SOURCE = "source";       // texture: which image of the document

using Textures = ::godot::TypedArray<::godot::Ref<::godot::Texture2D>>;

// the packs of the file at `path`, at the places of its image list; empty when the stage packed none or
// one of them does not load
Textures packed_images(const ::godot::String& path) {
    const ::godot::String pack_dir = path + ::godot::String(PACK_SUFFIX);
    const ::godot::String index_path = pack_dir.path_join(PACK_INDEX);
    if (!::godot::FileAccess::file_exists(index_path)) {
        return Textures();
    }
    const ::godot::Variant parsed = ::godot::JSON::parse_string(::godot::FileAccess::get_file_as_string(index_path));
    const ::godot::Dictionary index = parsed.get_type() == ::godot::Variant::DICTIONARY ? ::godot::Dictionary(parsed)
                                                                                         : ::godot::Dictionary();
    const ::godot::Array images = index.get(PACK_KEY_IMAGES, ::godot::Array());
    Textures packed;
    packed.resize(images.size());
    for (int64_t i = 0; i < images.size(); ++i) {
        const ::godot::Dictionary image = images[i];
        const int64_t place = image.get(PACK_KEY_INDEX, -1);
        const ::godot::String file = image.get(PACK_KEY_FILE, ::godot::String());
        const ::godot::String pack_path = pack_dir.path_join(file);
        const ::godot::Ref<::godot::Texture2D> texture = ::godot::ResourceLoader::get_singleton()->load(pack_path);
        if (place < 0 || place >= images.size() || file.is_empty() || texture.is_null()) {
            ::godot::UtilityFunctions::push_error(::godot::String("[") + GODOT_LOG_SOURCE + "] texture pack " +
                                                  pack_path + " does not load - " + path +
                                                  " is drawn from its own images");
            return Textures();
        }
        packed[place] = texture;
    }
    return packed;
}

// the pack a texture slot of `owner` names through the texture list of the document; empty without one
::godot::Ref<::godot::Texture2D> slot_pack(const ::godot::Dictionary& owner, const char* slot,
                                           const ::godot::Array& textures, const Textures& packed) {
    const ::godot::Dictionary reference = owner.get(slot, ::godot::Dictionary());
    const int64_t texture = reference.get(GLTF_KEY_INDEX, -1);
    if (texture < 0 || texture >= textures.size()) {
        return ::godot::Ref<::godot::Texture2D>();
    }
    const ::godot::Dictionary entry = textures[texture];
    const int64_t source = entry.get(GLTF_KEY_SOURCE, -1);
    if (source < 0 || source >= packed.size()) {
        return ::godot::Ref<::godot::Texture2D>();
    }
    return ::godot::Ref<::godot::Texture2D>(packed[source]);
}

void bind_slot(::godot::BaseMaterial3D* pbr, ::godot::BaseMaterial3D::TextureParam param,
               const ::godot::Ref<::godot::Texture2D>& pack) {
    if (pack.is_valid()) {
        pbr->set_texture(param, pack);
    }
}

// every material of the state gets the packs its glTF material names, in the slots GLTFDocument fills on
// import; the channels it reads (metallic blue, roughness green, occlusion red) it has set already
void bind_packs(const ::godot::Ref<::godot::GLTFState>& state, const Textures& packed) {
    const ::godot::Dictionary json = state->get_json();
    const ::godot::Array materials = json.get(GLTF_KEY_MATERIALS, ::godot::Array());
    const ::godot::Array textures = json.get(GLTF_KEY_TEXTURES, ::godot::Array());
    const ::godot::TypedArray<::godot::Ref<::godot::Material>> bound = state->get_materials();
    for (int64_t m = 0; m < materials.size() && m < bound.size(); ++m) {
        const ::godot::Ref<::godot::Material> material = bound[m];
        ::godot::BaseMaterial3D* pbr = ::godot::Object::cast_to<::godot::BaseMaterial3D>(material.ptr());
        if (pbr == nullptr) {
            continue;
        }
        const ::godot::Dictionary source_material = materials[m];
        const ::godot::Dictionary pbr_block = source_material.get(GLTF_KEY_PBR, ::godot::Dictionary());
        bind_slot(pbr, ::godot::BaseMaterial3D::TEXTURE_ALBEDO,
                  slot_pack(pbr_block, GLTF_KEY_BASE_COLOR, textures, packed));
        const ::godot::Ref<::godot::Texture2D> surface =
            slot_pack(pbr_block, GLTF_KEY_METALLIC_ROUGHNESS, textures, packed);
        bind_slot(pbr, ::godot::BaseMaterial3D::TEXTURE_METALLIC, surface);
        bind_slot(pbr, ::godot::BaseMaterial3D::TEXTURE_ROUGHNESS, surface);
        bind_slot(pbr, ::godot::BaseMaterial3D::TEXTURE_NORMAL,
                  slot_pack(source_material, GLTF_KEY_NORMAL, textures, packed));
        bind_slot(pbr, ::godot::BaseMaterial3D::TEXTURE_AMBIENT_OCCLUSION,
                  slot_pack(source_material, GLTF_KEY_OCCLUSION, textures, packed));
        bind_slot(pbr, ::godot::BaseMaterial3D::TEXTURE_EMISSION,
                  slot_pack(source_material, GLTF_KEY_EMISSIVE, textures, packed));
    }
}

// before the parse: the packs of the file at `path`; with packs the state discards the file's own images
Textures prepare(const ::godot::String& path, const ::godot::Ref<::godot::GLTFState>& state) {
    const Textures packed = packed_images(path);
    if (!packed.is_empty()) {
        state->set_handle_binary_image(::godot::GLTFState::HANDLE_BINARY_DISCARD_TEXTURES);
    }
    return packed;
}

// after the parse: the packs take the places of the discarded images and every material slot is bound to them
void finish(const ::godot::Ref<::godot::GLTFState>& state, const Textures& packed) {
    if (!packed.is_empty()) {
        state->set_images(packed);
        bind_packs(state, packed);
    }
}

::godot::Node* generate(const ::godot::String& asset_root, const std::string& file,
                        const ::godot::Ref<::godot::GLTFState>& state) {
    const ::godot::String path = asset_root.path_join(::godot::String::utf8(file.c_str()));
    const Textures packed = prepare(path, state);
    ::godot::Ref<::godot::GLTFDocument> document;
    document.instantiate();
    if (document->append_from_file(path, state) != ::godot::OK) {
        return nullptr;
    }
    finish(state, packed);
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

// _parse_image_data(state, bytes, mime type, image) → Error, called by Godot with ptrcall arguments: a state
// that discards its images gets the stand-in, so GLTFDocument skips the decode; any other image stays empty
// and GLTFDocument decodes it as before
void call_parse_image_data(GDExtensionClassInstancePtr instance, const GDExtensionConstTypePtr* args,
                           GDExtensionTypePtr result) {
    (void)instance;
    const ::godot::Ref<::godot::GLTFState> state =
        ::godot::PtrToArg<const ::godot::Ref<::godot::GLTFState>&>::convert(args[0]);
    const ::godot::Ref<::godot::Image> image = ::godot::PtrToArg<const ::godot::Ref<::godot::Image>&>::convert(args[3]);
    if (state.is_valid() && image.is_valid() &&
        state->get_handle_binary_image() == ::godot::GLTFState::HANDLE_BINARY_DISCARD_TEXTURES) {
        ::godot::PackedByteArray pixel;
        pixel.push_back(0);
        image->set_data(SKIP_IMAGE_SIDE, SKIP_IMAGE_SIDE, false, ::godot::Image::FORMAT_L8, pixel);
    }
    ::godot::PtrToArg<::godot::Error>::encode(::godot::OK, result);
}

GDExtensionClassCallVirtual skip_callback(void* class_userdata, GDExtensionConstStringNamePtr name, uint32_t hash) {
    (void)class_userdata;
    const ::godot::StringName& asked = *static_cast<const ::godot::StringName*>(name);
    if (hash == SKIP_HOOK_HASH && asked == ::godot::StringName(SKIP_HOOK)) {
        return &call_parse_image_data;
    }
    return nullptr;
}

// one object of the class: the engine GLTFDocumentExtension; it keeps nothing of its own, so the engine object
// stands as its instance data and free has nothing to release
GDExtensionObjectPtr create_skip(void* class_userdata, GDExtensionBool notify_postinitialize) {
    (void)class_userdata;
    const ::godot::StringName engine_class(SKIP_ENGINE_CLASS);
    const ::godot::StringName skip_class(SKIP_CLASS);
    GDExtensionObjectPtr object =
        ::godot::internal::gdextension_interface_classdb_construct_object2(engine_class._native_ptr());
    ::godot::GLTFDocumentExtension* extension = ::godot::Object::cast_to<::godot::GLTFDocumentExtension>(
        ::godot::internal::get_object_instance_binding(object));
    if (extension == nullptr) {
        ::godot::UtilityFunctions::push_error(::godot::String("[") + GODOT_LOG_SOURCE + "] " + SKIP_CLASS + ": no " +
                                              SKIP_ENGINE_CLASS + " object to attach to");
        if (object != nullptr) {
            ::godot::internal::gdextension_interface_object_destroy(object);
        }
        return nullptr;
    }
    ::godot::internal::gdextension_interface_object_set_instance(object, skip_class._native_ptr(), object);
    if (notify_postinitialize) {
        extension->notification(::godot::Object::NOTIFICATION_POSTINITIALIZE);
    }
    return object;
}

void free_skip(void* class_userdata, GDExtensionClassInstancePtr instance) {
    (void)class_userdata;
    (void)instance;
}

}  // namespace

void register_image_skip() {
    GDExtensionClassCreationInfo5 info = {};
    info.is_exposed = true;
    info.create_instance_func = &create_skip;
    info.free_instance_func = &free_skip;
    info.get_virtual_func = &skip_callback;
    const ::godot::StringName skip_class(SKIP_CLASS);
    const ::godot::StringName engine_class(SKIP_ENGINE_CLASS);
    ::godot::internal::gdextension_interface_classdb_register_extension_class5(
        ::godot::internal::library, skip_class._native_ptr(), engine_class._native_ptr(), &info);
    GDExtensionObjectPtr object = create_skip(nullptr, true);
    ::godot::GLTFDocumentExtension* extension = object != nullptr
        ? ::godot::Object::cast_to<::godot::GLTFDocumentExtension>(::godot::internal::get_object_instance_binding(object))
        : nullptr;
    if (extension == nullptr) {
        ::godot::UtilityFunctions::push_error(::godot::String("[") + GODOT_LOG_SOURCE + "] " + SKIP_CLASS +
                                              " is not registered - a packed file decodes the images it discards");
        return;
    }
    const ::godot::Ref<::godot::GLTFDocumentExtension> held(extension);
    ::godot::GLTFDocument::register_gltf_document_extension(held);
    ::godot::Engine::get_singleton()->set_meta(SKIP_HOLDER, held);
}

void unregister_image_skip() {
    ::godot::Engine* engine = ::godot::Engine::get_singleton();
    {
        // the last reference goes at the end of this block, while the class still stands to free its object
        const ::godot::Ref<::godot::GLTFDocumentExtension> held = engine->get_meta(SKIP_HOLDER, ::godot::Variant());
        if (held.is_valid()) {
            ::godot::GLTFDocument::unregister_gltf_document_extension(held);
        }
        engine->remove_meta(SKIP_HOLDER);
    }
    const ::godot::StringName skip_class(SKIP_CLASS);
    ::godot::internal::gdextension_interface_classdb_unregister_extension_class(::godot::internal::library,
                                                                               skip_class._native_ptr());
}

::godot::Node* load_asset(const ::godot::String& asset_root, const std::string& file) {
    ::godot::Ref<::godot::GLTFState> state;
    state.instantiate();
    return generate(asset_root, file, state);
}

bool parse_asset_bytes(const ::godot::Ref<::godot::GLTFDocument>& document, const ::godot::Ref<::godot::GLTFState>& state,
                       const ::godot::String& asset_root, const std::string& file, const ::godot::PackedByteArray& bytes) {
    const ::godot::String path = asset_root.path_join(::godot::String::utf8(file.c_str()));
    const Textures packed = prepare(path, state);
    // the base directory resolves the buffers and images a .gltf names beside itself, as append_from_file does
    if (document->append_from_buffer(bytes, path.get_base_dir(), state) != ::godot::OK) {
        return false;
    }
    finish(state, packed);
    return true;
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

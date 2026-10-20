/**
 * ASE GODOT ADAPTER - PLANT LOOK IMPLEMENTATION
 *
 * @file        godot_plant_look.cpp
 * @brief       Binds the materials and organ template files of the care view's 3D scene once
 * @description See godot_plant_look.hpp. Every file is resolved through godot_asset_loader.hpp at the
 *              start; the scene fills never read a file. The bark of the internodes is one ShaderMaterial
 *              of GODOT_BARK_SHADER_CODE over both bound bark materials (usecase:100395 age the bark).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_plant_look.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace ase::adp::godot {

namespace {

/** A lit surface of one colour: the bare soil of a focused patch that carries no soil code - no role, no asset. */
::godot::Ref<::godot::StandardMaterial3D> material_of(uint32_t color) {
    ::godot::Ref<::godot::StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(::godot::Color::hex(color));
    return material;
}

/** The asset name the stage gives a role; empty when the stage does not name it - a name the binding holds no file for. */
std::string asset_of(const std::unordered_map<std::string, std::string>& roles, const char* role) {
    const auto found = roles.find(role);
    return found == roles.end() ? std::string() : found->second;
}

/** An organ template role: the mesh of its file, or no mesh and the error material while the file is missing. */
PlantSceneTemplate template_of(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                               const std::string& name) {
    PlantSceneTemplate role;
    role.shape = bound_template(binding, asset_root, name);
    if (role.shape.is_null()) {
        role.material = error_material(binding, name);
    }
    return role;
}

/**
 * The bark of the internodes (usecase:100395 age the bark): a ShaderMaterial of GODOT_BARK_SHADER_CODE that
 * mixes `white` with `base` by the share each instance carries. Its uniforms take the colour, textures,
 * roughness and UV scale of both bound materials. A bound material that is no BaseMaterial3D - a glTF
 * material always is one, the error material too - leaves the bark in the error material of its role.
 */
::godot::Ref<::godot::Material> bark_mix_of(const VivariumAssetBinding& binding, const std::string& white_name,
                                            const ::godot::Ref<::godot::Material>& white,
                                            const std::string& base_name,
                                            const ::godot::Ref<::godot::Material>& base) {
    const ::godot::Ref<::godot::BaseMaterial3D> white_pbr = white;
    if (white_pbr.is_null()) {
        return error_material(binding, white_name);
    }
    const ::godot::Ref<::godot::BaseMaterial3D> base_pbr = base;
    if (base_pbr.is_null()) {
        return error_material(binding, base_name);
    }
    ::godot::Ref<::godot::Shader> shader;
    shader.instantiate();
    shader->set_code(GODOT_BARK_SHADER_CODE);
    ::godot::Ref<::godot::ShaderMaterial> mix;
    mix.instantiate();
    mix->set_shader(shader);
    mix->set_shader_parameter(GODOT_BARK_WHITE_COLOR, white_pbr->get_albedo());
    mix->set_shader_parameter(GODOT_BARK_WHITE_ALBEDO, white_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO));
    mix->set_shader_parameter(GODOT_BARK_WHITE_NORMAL, white_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_NORMAL));
    mix->set_shader_parameter(GODOT_BARK_WHITE_ROUGH,
                              white_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ROUGHNESS));
    mix->set_shader_parameter(GODOT_BARK_WHITE_ROUGHNESS, white_pbr->get_roughness());
    const ::godot::Vector3 white_uv = white_pbr->get_uv1_scale();
    mix->set_shader_parameter(GODOT_BARK_WHITE_UV, ::godot::Vector2(white_uv.x, white_uv.y));
    mix->set_shader_parameter(GODOT_BARK_BASE_COLOR, base_pbr->get_albedo());
    mix->set_shader_parameter(GODOT_BARK_BASE_ALBEDO, base_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO));
    mix->set_shader_parameter(GODOT_BARK_BASE_NORMAL, base_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_NORMAL));
    mix->set_shader_parameter(GODOT_BARK_BASE_ROUGH, base_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ROUGHNESS));
    mix->set_shader_parameter(GODOT_BARK_BASE_ROUGHNESS, base_pbr->get_roughness());
    const ::godot::Vector3 base_uv = base_pbr->get_uv1_scale();
    mix->set_shader_parameter(GODOT_BARK_BASE_UV, ::godot::Vector2(base_uv.x, base_uv.y));
    return mix;
}

}  // namespace

PlantSceneLook bind_plant_look(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                               const std::unordered_map<std::string, std::string>& roles) {
    PlantSceneLook look;
    const std::string white_name = asset_of(roles, GODOT_ROLE_BARK);
    const std::string base_name = asset_of(roles, GODOT_ROLE_BARK_BASE);
    look.bark = bark_mix_of(binding, white_name, bound_material(binding, asset_root, white_name), base_name,
                            bound_material(binding, asset_root, base_name));
    look.ground[GODOT_SOIL_MEADOW] = bound_material(binding, asset_root, asset_of(roles, GODOT_ROLE_GROUND_MEADOW));
    look.ground[GODOT_SOIL_FOREST] = bound_material(binding, asset_root, asset_of(roles, GODOT_ROLE_GROUND_FOREST));
    look.soil = material_of(GODOT_COLOR_SOIL);
    look.leaf = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_LEAF));
    look.male = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_MALE));
    look.female = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_FEMALE));
    look.fruit = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_FRUIT));
    look.nutlet = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_NUTLET));
    return look;
}

}  // namespace ase::adp::godot

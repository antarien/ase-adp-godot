/**
 * ASE GODOT ADAPTER - PLANT LOOK IMPLEMENTATION
 *
 * @file        godot_plant_look.cpp
 * @brief       Binds the materials and organ template files of the care view's 3D scene once
 * @description See godot_plant_look.hpp. Every file is resolved through godot_asset_loader.hpp at the
 *              start; the scene fills never read a file. The bark of the internodes is one ShaderMaterial
 *              of GODOT_BARK_SHADER_CODE over both bark materials (usecase:100395 age the bark); a bark
 *              file that is missing hands on the stand-in colour of its name instead. The ground of every
 *              soil is one ShaderMaterial of GODOT_GROUND_SHADER_CODE whose cover is the soil's role:
 *              slope, levelled disc, brook bed, grid and contour lines are drawn over it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-25
 * @version     00.00.03.00000
 */

#include <ase/adp/godot/godot_plant_look.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace ase::adp::godot {

namespace {

/** The asset name the stage gives a role; empty when the stage does not name it - a name the binding holds no file for. */
std::string asset_of(const std::unordered_map<std::string, std::string>& roles, const char* role) {
    const auto found = roles.find(role);
    return found == roles.end() ? std::string() : found->second;
}

/**
 * The stand-in material of a name whose file is missing (operator order of 2026-10-23): a ShaderMaterial of
 * GODOT_STAND_IN_SHADER_CODE in the colour the asset list gives the name. The ground darkens with the soil
 * water of the focused patch and a leaf takes the colour of its condition through the instance uniforms the
 * scene hands on every frame (mark_plant_stands).
 */
::godot::Ref<::godot::Material> stand_in_of(const VivariumAssetBinding& binding, const std::string& name) {
    ::godot::Ref<::godot::Shader> shader;
    shader.instantiate();
    shader->set_code(GODOT_STAND_IN_SHADER_CODE);
    ::godot::Ref<::godot::ShaderMaterial> material;
    material.instantiate();
    material->set_shader(shader);
    material->set_shader_parameter(GODOT_STAND_IN_COLOR,
                                   ::godot::Color::hex(asset_color(binding, name, GODOT_COLOR_ASSET_MISSING)));
    material->set_shader_parameter(GODOT_STAND_IN_WILTED, ::godot::Color::hex(GODOT_COLOR_WILTED));
    material->set_shader_parameter(GODOT_STAND_IN_DEAD, ::godot::Color::hex(GODOT_COLOR_DEAD));
    material->set_shader_parameter(GODOT_STAND_IN_WILTED_CODE, static_cast<int32_t>(GODOT_COND_WILTED));
    material->set_shader_parameter(GODOT_STAND_IN_DEAD_CODE, static_cast<int32_t>(GODOT_COND_DEAD));
    material->set_shader_parameter(GODOT_STAND_IN_WET, GODOT_STAND_IN_WET_SHADE);
    material->set_shader_parameter(GODOT_STAND_IN_ROUGH, GODOT_STAND_IN_ROUGHNESS);
    return material;
}

/**
 * The material of a material role: the first surface of its file when the binding holds one (a file that
 * does not load stays in the error material of the loader - a broken delivery is a signal), the stand-in
 * material while the file is missing.
 */
::godot::Ref<::godot::Material> role_material(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                              const std::string& name) {
    if (binding.files.count(name) == 0u) {
        return stand_in_of(binding, name);
    }
    return bound_material(binding, asset_root, name);
}

/**
 * An organ template role: the mesh of its file; while the file is missing no mesh and the stand-in material,
 * a file that does not load no mesh and the error material of the loader.
 */
PlantSceneTemplate template_of(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                               const std::string& name) {
    PlantSceneTemplate role;
    role.shape = bound_template(binding, asset_root, name);
    if (role.shape.is_null()) {
        role.material = binding.files.count(name) == 0u ? stand_in_of(binding, name)
                                                        : ::godot::Ref<::godot::Material>(error_material(binding, name));
    }
    return role;
}

/**
 * The bark of the internodes (usecase:100395 age the bark): a ShaderMaterial of GODOT_BARK_SHADER_CODE that
 * mixes `white` with `base` by the share each instance carries. A side bound to a BaseMaterial3D - a glTF
 * material always is one - hands on its colour, textures, roughness and UV scale. A side whose file is
 * missing hands on the stand-in colour of its name and no texture: the shader samples white then, so the
 * tube shows white bark above and the stem-base colour below, mixed by the same share.
 */
::godot::Ref<::godot::Material> bark_mix_of(const VivariumAssetBinding& binding, const std::string& white_name,
                                            const ::godot::Ref<::godot::Material>& white,
                                            const std::string& base_name,
                                            const ::godot::Ref<::godot::Material>& base) {
    ::godot::Ref<::godot::Shader> shader;
    shader.instantiate();
    shader->set_code(GODOT_BARK_SHADER_CODE);
    ::godot::Ref<::godot::ShaderMaterial> mix;
    mix.instantiate();
    mix->set_shader(shader);
    const ::godot::Ref<::godot::BaseMaterial3D> white_pbr = white;
    if (white_pbr.is_valid()) {
        mix->set_shader_parameter(GODOT_BARK_WHITE_COLOR, white_pbr->get_albedo());
        mix->set_shader_parameter(GODOT_BARK_WHITE_ALBEDO,
                                  white_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO));
        mix->set_shader_parameter(GODOT_BARK_WHITE_NORMAL,
                                  white_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_NORMAL));
        mix->set_shader_parameter(GODOT_BARK_WHITE_ROUGH,
                                  white_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ROUGHNESS));
        mix->set_shader_parameter(GODOT_BARK_WHITE_ROUGHNESS, white_pbr->get_roughness());
        const ::godot::Vector3 white_uv = white_pbr->get_uv1_scale();
        mix->set_shader_parameter(GODOT_BARK_WHITE_UV, ::godot::Vector2(white_uv.x, white_uv.y));
    } else {
        mix->set_shader_parameter(GODOT_BARK_WHITE_COLOR,
                                  ::godot::Color::hex(asset_color(binding, white_name, GODOT_COLOR_ASSET_MISSING)));
        mix->set_shader_parameter(GODOT_BARK_WHITE_ROUGHNESS, GODOT_STAND_IN_ROUGHNESS);
    }
    const ::godot::Ref<::godot::BaseMaterial3D> base_pbr = base;
    if (base_pbr.is_valid()) {
        mix->set_shader_parameter(GODOT_BARK_BASE_COLOR, base_pbr->get_albedo());
        mix->set_shader_parameter(GODOT_BARK_BASE_ALBEDO,
                                  base_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO));
        mix->set_shader_parameter(GODOT_BARK_BASE_NORMAL,
                                  base_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_NORMAL));
        mix->set_shader_parameter(GODOT_BARK_BASE_ROUGH,
                                  base_pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ROUGHNESS));
        mix->set_shader_parameter(GODOT_BARK_BASE_ROUGHNESS, base_pbr->get_roughness());
        const ::godot::Vector3 base_uv = base_pbr->get_uv1_scale();
        mix->set_shader_parameter(GODOT_BARK_BASE_UV, ::godot::Vector2(base_uv.x, base_uv.y));
    } else {
        mix->set_shader_parameter(GODOT_BARK_BASE_COLOR,
                                  ::godot::Color::hex(asset_color(binding, base_name, GODOT_COLOR_ASSET_MISSING)));
        mix->set_shader_parameter(GODOT_BARK_BASE_ROUGHNESS, GODOT_STAND_IN_ROUGHNESS);
    }
    return mix;
}

/**
 * The ground of one soil (GODOT_GROUND_SHADER_CODE): a ShaderMaterial whose cover is `cover` - a side bound to
 * a BaseMaterial3D hands on its colour, albedo texture, UV scale and roughness, a cover whose file is
 * missing the colour `fallback` and no texture, so the shader samples white. Bare soil, bank, bed, slope
 * thresholds and the line shares are the same for every soil; the grid step comes with the terrain
 * (fill_plant_ground).
 */
::godot::Ref<::godot::Material> ground_of(const ::godot::Ref<::godot::Material>& cover, uint32_t fallback) {
    ::godot::Ref<::godot::Shader> shader;
    shader.instantiate();
    shader->set_code(GODOT_GROUND_SHADER_CODE);
    ::godot::Ref<::godot::ShaderMaterial> ground;
    ground.instantiate();
    ground->set_shader(shader);
    const ::godot::Ref<::godot::BaseMaterial3D> pbr = cover;
    if (pbr.is_valid()) {
        ground->set_shader_parameter(GODOT_GROUND_COVER_COLOR, pbr->get_albedo());
        ground->set_shader_parameter(GODOT_GROUND_COVER_ALBEDO, pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO));
        const ::godot::Vector3 uv = pbr->get_uv1_scale();
        ground->set_shader_parameter(GODOT_GROUND_COVER_UV, ::godot::Vector2(uv.x, uv.y));
        ground->set_shader_parameter(GODOT_GROUND_ROUGH, pbr->get_roughness());
    } else {
        ground->set_shader_parameter(GODOT_GROUND_COVER_COLOR, ::godot::Color::hex(fallback));
        ground->set_shader_parameter(GODOT_GROUND_ROUGH, GODOT_STAND_IN_ROUGHNESS);
    }
    ground->set_shader_parameter(GODOT_GROUND_SOIL, ::godot::Color::hex(GODOT_COLOR_SOIL));
    ground->set_shader_parameter(GODOT_GROUND_BANK, ::godot::Color::hex(GODOT_COLOR_GROUND_BANK));
    ground->set_shader_parameter(GODOT_GROUND_BED, ::godot::Color::hex(GODOT_COLOR_GROUND_BED));
    ground->set_shader_parameter(GODOT_GROUND_GRID, ::godot::Color::hex(GODOT_COLOR_GROUND_GRID));
    ground->set_shader_parameter(GODOT_GROUND_SLOPE_BANK, GODOT_GROUND_SLOPE_BANK_SHARE);
    ground->set_shader_parameter(GODOT_GROUND_SLOPE_STEEP, GODOT_GROUND_SLOPE_STEEP_SHARE);
    ground->set_shader_parameter(GODOT_GROUND_GRID_MAJOR_KEY, GODOT_GROUND_GRID_MAJOR);
    ground->set_shader_parameter(GODOT_GROUND_CONTOUR, GODOT_GROUND_CONTOUR_M);
    ground->set_shader_parameter(GODOT_GROUND_GRID_WEIGHT, GODOT_GROUND_GRID_SHARE);
    ground->set_shader_parameter(GODOT_GROUND_MAJOR_WEIGHT, GODOT_GROUND_MAJOR_SHARE);
    ground->set_shader_parameter(GODOT_GROUND_CONTOUR_WEIGHT, GODOT_GROUND_CONTOUR_SHARE);
    ground->set_shader_parameter(GODOT_GROUND_WET, GODOT_STAND_IN_WET_SHADE);
    return ground;
}

/** The ground of the soil whose role is `role`: its cover is the material the role's file carries, while the file is missing its stand-in colour. */
::godot::Ref<::godot::Material> soil_ground(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                            const std::unordered_map<std::string, std::string>& roles,
                                            const char* role) {
    const std::string name = asset_of(roles, role);
    const ::godot::Ref<::godot::Material> cover =
        binding.files.count(name) == 0u ? ::godot::Ref<::godot::Material>() : bound_material(binding, asset_root, name);
    return ground_of(cover, asset_color(binding, name, GODOT_COLOR_ASSET_MISSING));
}

}  // namespace

PlantSceneLook bind_plant_look(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                               const std::unordered_map<std::string, std::string>& roles) {
    PlantSceneLook look;
    const std::string white_name = asset_of(roles, GODOT_ROLE_BARK);
    const std::string base_name = asset_of(roles, GODOT_ROLE_BARK_BASE);
    look.bark = bark_mix_of(binding, white_name, role_material(binding, asset_root, white_name), base_name,
                            role_material(binding, asset_root, base_name));
    look.ground[GODOT_SOIL_MEADOW] = soil_ground(binding, asset_root, roles, GODOT_ROLE_GROUND_MEADOW);
    look.ground[GODOT_SOIL_FOREST] = soil_ground(binding, asset_root, roles, GODOT_ROLE_GROUND_FOREST);
    look.soil = ground_of(::godot::Ref<::godot::Material>(), GODOT_COLOR_SOIL);
    look.leaf = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_LEAF));
    look.male = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_MALE));
    look.female = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_FEMALE));
    look.fruit = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_FRUIT));
    look.nutlet = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_NUTLET));
    return look;
}

}  // namespace ase::adp::godot

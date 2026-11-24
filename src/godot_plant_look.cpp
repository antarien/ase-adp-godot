/**
 * ASE GODOT ADAPTER - PLANT LOOK IMPLEMENTATION
 *
 * @file        godot_plant_look.cpp
 * @brief       Binds the materials and organ template files of the care view's 3D scene once
 * @description See godot_plant_look.hpp. Every file is resolved through godot_asset_loader.hpp at the
 *              start; the scene fills never read a file. The bark of the internodes is one ShaderMaterial
 *              of GODOT_BARK_SHADER_FILE over both bark materials (usecase:100395 age the bark); a bark
 *              file that is missing hands on the stand-in colour of its name instead. The ground of every
 *              soil is one ShaderMaterial of GODOT_GROUND_SHADER_FILE whose cover is the soil's role:
 *              slope, levelled disc, brook bed, grid and contour lines are drawn over it. Every shader is a
 *              file under shaders/ (godot_plant_look_shader.hpp), loaded once and shared by its materials;
 *              the swing of a branch (usecase:101335 animate the branch sway) comes in by the shader's own
 *              #include of shaders/sway_uniforms.gdshaderinc and shaders/sway_swing.gdshaderinc.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_look.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/adp/godot/godot_plant_look_shader.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/adp/godot/asset/types.hpp>
#include <ase/adp/godot/cap/godot_plant_cap.hpp>
#include <ase/adp/godot/cap/types.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/adp/godot/scene/types.hpp>

#include <godot_cpp/classes/base_material3d.hpp>
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
 * GODOT_STAND_IN_SHADER_FILE in the colour the asset list gives the name. The ground darkens with the soil
 * water of the focused patch and a leaf takes the colour of its condition through the instance uniforms the
 * scene hands on every frame (mark_plant_stands).
 */
::godot::Ref<::godot::Material> stand_in_of(const VivariumAssetBinding& binding, const std::string& name) {
    ::godot::Ref<::godot::ShaderMaterial> material;
    material.instantiate();
    material->set_shader(load_plant_shader(GODOT_STAND_IN_SHADER_FILE));
    material->set_shader_parameter(GODOT_STAND_IN_COLOR,
                                   ::godot::Color::hex(asset_color(binding, name, GODOT_COLOR_ASSET_MISSING)));
    material->set_shader_parameter(GODOT_STAND_IN_WILTED, ::godot::Color::hex(GODOT_COLOR_WILTED));
    material->set_shader_parameter(GODOT_STAND_IN_DEAD, ::godot::Color::hex(GODOT_COLOR_DEAD));
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
 * The material a bound template swings in (usecase:101335 animate the branch sway): a ShaderMaterial of
 * GODOT_ORGAN_SHADER_FILE that takes what the BaseMaterial3D of its one surface carries. Empty for a template of
 * several surfaces or a material of another form: such a template draws its own surfaces and stands still.
 */
::godot::Ref<::godot::Material> swaying_organ_of(const ::godot::Ref<::godot::Mesh>& shape) {
    if (shape->get_surface_count() != GODOT_ORGAN_SWAY_SURFACES) {
        return ::godot::Ref<::godot::Material>();
    }
    const ::godot::Ref<::godot::BaseMaterial3D> pbr = shape->surface_get_material(0);
    if (pbr.is_null()) {
        return ::godot::Ref<::godot::Material>();
    }
    ::godot::Ref<::godot::ShaderMaterial> organ;
    organ.instantiate();
    organ->set_shader(load_plant_shader(GODOT_ORGAN_SHADER_FILE));
    organ->set_shader_parameter(GODOT_ORGAN_COLOR, pbr->get_albedo());
    organ->set_shader_parameter(GODOT_ORGAN_ALBEDO, pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ALBEDO));
    organ->set_shader_parameter(GODOT_ORGAN_NORMAL, pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_NORMAL));
    organ->set_shader_parameter(GODOT_ORGAN_ROUGH, pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_ROUGHNESS));
    organ->set_shader_parameter(GODOT_ORGAN_LIGHT, pbr->get_texture(::godot::BaseMaterial3D::TEXTURE_BACKLIGHT));
    organ->set_shader_parameter(GODOT_ORGAN_ROUGHNESS, pbr->get_roughness());
    const ::godot::Vector3 uv = pbr->get_uv1_scale();
    organ->set_shader_parameter(GODOT_ORGAN_UV, ::godot::Vector2(uv.x, uv.y));
    if (pbr->get_transparency() == ::godot::BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR) {
        organ->set_shader_parameter(GODOT_ORGAN_CUT, pbr->get_alpha_scissor_threshold());
    }
    return organ;
}

/**
 * An organ template role: the mesh of its file in the material it swings in; while the file is missing no mesh and
 * the stand-in material, a file that does not load no mesh and the error material of the loader.
 */
PlantSceneTemplate template_of(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                               const std::string& name) {
    PlantSceneTemplate role;
    role.shape = bound_template(binding, asset_root, name);
    if (role.shape.is_null()) {
        role.material = binding.files.count(name) == 0u ? stand_in_of(binding, name)
                                                        : ::godot::Ref<::godot::Material>(error_material(binding, name));
        return role;
    }
    role.material = swaying_organ_of(role.shape);
    return role;
}

/**
 * The bark of the internodes (usecase:100395 age the bark): a ShaderMaterial of GODOT_BARK_SHADER_FILE that
 * mixes `white` with `base` by the share each instance carries. A side bound to a BaseMaterial3D - a glTF
 * material always is one - hands on its colour, textures, roughness and UV scale. A side whose file is
 * missing hands on the stand-in colour of its name and no texture: the shader samples white then, so the
 * tube shows white bark above and the stem-base colour below, mixed by the same share.
 */
::godot::Ref<::godot::Material> bark_mix_of(const VivariumAssetBinding& binding, const std::string& white_name,
                                            const ::godot::Ref<::godot::Material>& white,
                                            const std::string& base_name,
                                            const ::godot::Ref<::godot::Material>& base) {
    ::godot::Ref<::godot::ShaderMaterial> mix;
    mix.instantiate();
    mix->set_shader(load_plant_shader(GODOT_BARK_SHADER_FILE));
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
 * The ground of one soil (GODOT_GROUND_SHADER_FILE): a ShaderMaterial whose cover is `cover` - a side bound to
 * a BaseMaterial3D hands on its colour, albedo texture, UV scale and roughness, a cover whose file is
 * missing the colour `fallback` and no texture, so the shader samples white. Bare soil, bank, bed, slope
 * thresholds and the line shares are the same for every soil; the grid step comes with the terrain
 * (fill_plant_ground).
 */
::godot::Ref<::godot::Material> ground_of(const ::godot::Ref<::godot::Material>& cover, uint32_t fallback) {
    ::godot::Ref<::godot::ShaderMaterial> ground;
    ground.instantiate();
    ground->set_shader(load_plant_shader(GODOT_GROUND_SHADER_FILE));
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
    ground->set_shader_parameter(GODOT_GROUND_FRAME_STEP, GODOT_FRAME_STEP_LINEAR);
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
    // the disk of a cut stub lies in the arrays of its bark tube and is set out by the same knot form, so it takes the
    // bark shader too; the cut wood stands on both sides, so the bark share of the stub changes nothing on its face
    const std::string wood_name = asset_of(roles, GODOT_ROLE_WOOD);
    const ::godot::Ref<::godot::Material> wood = role_material(binding, asset_root, wood_name);
    look.wood = bark_mix_of(binding, wood_name, wood, wood_name, wood);
    // its UV is turned around the pith in the middle of the wood texture, sampled once over the disk (cut_face)
    const ::godot::Ref<::godot::ShaderMaterial> wood_shader = look.wood;
    wood_shader->set_shader_parameter(GODOT_BARK_CUT_FACE, true);
    look.ground[GODOT_SOIL_MEADOW] = soil_ground(binding, asset_root, roles, GODOT_ROLE_GROUND_MEADOW);
    look.ground[GODOT_SOIL_FOREST] = soil_ground(binding, asset_root, roles, GODOT_ROLE_GROUND_FOREST);
    look.soil = ground_of(::godot::Ref<::godot::Material>(), GODOT_COLOR_SOIL);
    look.leaf = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_LEAF));
    // the leaf alone takes the colour of its season and the dry olive (m.html leafCol): its own material, stand-in or
    // organ, carries GODOT_LEAF_SEASON; the error material of a file that does not load is no ShaderMaterial and stays
    const ::godot::Ref<::godot::ShaderMaterial> leaf_shader = look.leaf.material;
    if (leaf_shader.is_valid()) {
        leaf_shader->set_shader_parameter(GODOT_LEAF_SEASON, GODOT_LEAF_SEASON_ON);
    }
    look.male = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_MALE));
    look.female = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_FEMALE));
    look.fruit = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_FRUIT));
    look.nutlet = template_of(binding, asset_root, asset_of(roles, GODOT_ROLE_NUTLET));
    return look;
}

PlantSceneTemplate bind_plant_cap(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                  const std::unordered_map<std::string, std::string>& roles, const std::string& species) {
    // the fruit body of a cap patch of `species` (usecase:101362 scale the organ templates): the same role binding as
    // every organ template, under the role fungus.<species>, its stand-in colour the one stand_ins_of gives every cap
    // role
    return template_of(binding, asset_root, asset_of(roles, cap_role_of(species).c_str()));
}

}  // namespace ase::adp::godot

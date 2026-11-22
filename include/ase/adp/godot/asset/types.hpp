#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the asset binding of the care view and its asset showcase
 * @description All compile-time constants of the asset sub-type; NO runtime values - those belong in the views.
 *              The keys of the asset list and its manifest, the ground roles, the glTF image names, the stand-in
 *              colours of an asset that is missing, and the layout of the asset showcase
 *              (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2, PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7).
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These values are read by the asset
 *              binding and the asset showcase.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: none.
 */

/**
 * Asset binding (godot_asset_binding.hpp, PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2).
 */
constexpr uint32_t GODOT_SHA256_HEX_DIGITS = 64u;                            // hex digits of one SHA-256 checksum
constexpr const char* GODOT_ASSET_MISSING = "asset missing ";               // boot line of a name without its asset, the name follows
constexpr const char* GODOT_ASSETS_PATH = "res://config/vivarium_assets.json";  // asset list of the play stages, staged beside the start configuration
constexpr const char* GODOT_ASSETS_KEY_MANIFEST = "manifest";               // asset list: res:// path of the manifest of the asset set
constexpr const char* GODOT_ASSETS_KEY_ROOT = "asset_root";                 // asset list: res:// directory the manifest's files lie in
constexpr const char* GODOT_ASSETS_KEY_STAGES = "stages";                   // asset list: one entry per play stage
constexpr const char* GODOT_ASSETS_KEY_STAGE = "stage";                     // stage entry: the play stage it lists
constexpr const char* GODOT_ASSETS_KEY_MATERIALS = "materials";             // stage entry: role → material of the list
constexpr const char* GODOT_ASSETS_KEY_TEMPLATES = "templates";             // stage entry: role → organ template of the list
constexpr const char* GODOT_ASSETS_KEY_ENTRIES = "assets";                  // manifest: its entries (ase-web-curator scripts/export-avt-assets.cjs)
constexpr const char* GODOT_ASSETS_KEY_ASSET = "name";                      // manifest entry: which asset of the list it delivers
constexpr const char* GODOT_ASSETS_KEY_FILE = "file";                       // manifest entry: its file below the asset root
constexpr const char* GODOT_ASSETS_KEY_SHA256 = "sha256";                   // manifest entry: SHA-256 of that file
constexpr const char* GODOT_ASSETS_COMMENT = "_";                           // a key of the list starting with it is a comment, never a role
constexpr const char* GODOT_ROLE_GROUND_MEADOW = "ground_meadow";           // material role of meadow soil (GODOT_SOIL_MEADOW)
constexpr const char* GODOT_ROLE_GROUND_FOREST = "ground_forest";           // material role of forest floor (GODOT_SOIL_FOREST)
constexpr uint32_t GODOT_COLOR_ASSET_MISSING = 0xFF00FFFFu;                 // signal colour of a name the asset list gives no stand-in colour
constexpr const char* GODOT_GLTF_KEY_IMAGES = "images";                     // glTF document: its image list, in the order GLTFState::get_images holds them
constexpr const char* GODOT_GLTF_KEY_IMAGE = "name";                        // glTF image: which map of ase-assgen it is (assgen_kit.save_map)
constexpr const char* GODOT_IMAGE_TRANSLUCENCY = "_translucency";           // glTF image ending of the light a thin organ lets through (leaf.py, fruit.py, grass.py), bound as backlight

constexpr uint32_t GODOT_COLOR_STAND_IN_BARK = 0xE6E1D3FFu;               // stand-in of GODOT_ROLE_BARK: the white bark of the birch
constexpr uint32_t GODOT_COLOR_STAND_IN_BARK_BASE = 0x3E3630FFu;          // stand-in of GODOT_ROLE_BARK_BASE: the dark cracked bark of the stem base
constexpr uint32_t GODOT_COLOR_STAND_IN_WOOD = 0xD9BD94FFu;               // stand-in of GODOT_ROLE_WOOD: the light birch wood of a cut face, cut_light 0.85/0.74/0.58 of the ase-assgen recipe camp_log
constexpr uint32_t GODOT_COLOR_STAND_IN_MEADOW = 0x587A34FFu;             // stand-in of GODOT_ROLE_GROUND_MEADOW: grass over earth
constexpr uint32_t GODOT_COLOR_STAND_IN_FOREST = 0x5B4632FFu;             // stand-in of GODOT_ROLE_GROUND_FOREST: leaf litter over earth
constexpr uint32_t GODOT_COLOR_STAND_IN_LEAF = 0x5E8F32FFu;               // stand-in of GODOT_ROLE_LEAF: a fresh birch leaf
constexpr uint32_t GODOT_COLOR_STAND_IN_MALE = 0xA38A4CFFu;               // stand-in of GODOT_ROLE_MALE: a male catkin
constexpr uint32_t GODOT_COLOR_STAND_IN_FEMALE = 0x7D8C3FFFu;             // stand-in of GODOT_ROLE_FEMALE: a female catkin
constexpr uint32_t GODOT_COLOR_STAND_IN_FRUIT = 0x8A6A3EFFu;              // stand-in of GODOT_ROLE_FRUIT: a ripe fruit stand
constexpr uint32_t GODOT_COLOR_STAND_IN_NUTLET = 0x9C7A44FFu;             // stand-in of GODOT_ROLE_NUTLET: a winged nutlet

/**
 * Asset showcase (godot_asset_showcase.hpp, PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7).
 */
constexpr const char* GODOT_SHOWCASE_LINE = "asset showcase";   // first words of the showcase's log line
constexpr const char* GODOT_SHOWCASE_ROOT = "AssetShowcase";    // name of the showcase's root node
constexpr const char* GODOT_SHOWCASE_SUN = "ShowcaseSun";       // name of the light the sun of the host sets
constexpr float GODOT_SHOWCASE_SPACING = 1.5f;                  // metres between two slots of one row
constexpr float GODOT_SHOWCASE_SLOT = 1.0f;                     // metres a template is scaled to along its largest extent
constexpr float GODOT_SHOWCASE_SPHERE_RADIUS = 0.5f;            // metres: radius of the sphere a material is shown on
constexpr float GODOT_SHOWCASE_PLANE_ROW = 1.5f;                // z of the row of planes; the spheres stand at z 0
constexpr float GODOT_SHOWCASE_TEMPLATE_ROW = 3.0f;             // z of the row of organ templates
constexpr float GODOT_SHOWCASE_FOV_DEGREES = 60.0f;             // horizontal field of view the showcase camera keeps
constexpr float GODOT_SHOWCASE_HALF_FOV_TAN = 0.57735027f;      // tan of half of GODOT_SHOWCASE_FOV_DEGREES
constexpr float GODOT_SHOWCASE_CAMERA_HEIGHT_SHARE = 0.6f;      // camera height as a share of its distance: a view from above at about 31 degrees

}  // namespace ase::adp::godot

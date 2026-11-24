#pragma once

/**
 * ASE GODOT ADAPTER - THE FRUIT BODY OF A CAP PATCH
 *
 * @file        godot_plant_cap.hpp
 * @brief       Binds the cap template of every species, scales it to the cap radius of its patch and draws it at the stem base
 * @description DSGN_018 ORGXFORM, usecase:101362 scale the organ templates: ase-plant carries the cap radius of
 *              its species (PLT_SPEC_CAP_RADIUS), ase-phytomer passes it on for the individual of a patch
 *              (PHY_CAP_RADIUS), ase-pl-flora files it on flora.cap.v1 and GodotHostResourceManager keeps it per
 *              patch (get_caps). Every species the stage names a cap role for (fungus.<catalog id>, cap_role_of)
 *              gets one child of the caps node at the start, holding the shape of that role (stamp_plant_caps),
 *              and every frame each child draws one fruit body per cap patch of its species at the stem base of
 *              the patch (fill_plant_caps): the bound asset, while its file is missing the unit fruit body of the
 *              template port (GODOT_TMPL_FNGS) in the stand-in colour of the role, scaled uniformly so its widest
 *              horizontal reach equals the radius the record carried (cap_scale, godot_cap_scale.hpp). The radius
 *              is read off the drawn mesh, never a size of ase-render-data repeated here. A porcini and a death cap
 *              thus each show their own body.
 *
 *              WHY A HEADER OF ITS OWN: the plant look (godot_plant_look.hpp) and the patch view
 *              (godot_host_patch_view.hpp) are read by most units of the care view, the patch view above the
 *              header ratchet; a cap entry there would compile all of them again for a part only the cap patches
 *              draw. This header is read by the look that binds the role, the scene that draws it and the node
 *              that stamps the templates at the start. The bound templates live on the caps node, not in the view:
 *              the view header stands at the dedication floor of the size gate.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101362 - scale the organ templates
 * @actor       Organ Transform Shaper (ORGATRASHA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.02.00002 [feat]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/cap/godot_cap_scale.hpp>
#include <ase/adp/godot/godot_host_start_config.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/godot_plant_look.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

class GodotHostResourceManager;

/**
 * The template of the cap role of `species` (cap_role_of): the mesh of the file the stage names for it, in the
 * material it carries; while the file is missing no mesh and the stand-in colour of the role
 * (GODOT_COLOR_STAND_IN_CAP), a file that does not load no mesh and the error material of the loader. Read at the
 * start, never per frame.
 */
PlantSceneTemplate bind_plant_cap(const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                                  const std::unordered_map<std::string, std::string>& roles, const std::string& species);

/**
 * One child of the caps node (GODOT_PLANT_CAPS) per cap role the stage names (cap_species_of), named
 * cap_node_of(species), once at the start: an empty MultiMesh in the shape bind_plant_cap gives that species and
 * its material as the override of the child. The children hold the templates from then on, so the view keeps no
 * member for them and fill_plant_caps reads them back. No caps node, nothing.
 */
void stamp_plant_caps(::godot::Node3D* scene, const VivariumAssetBinding& binding, const ::godot::String& asset_root,
                      const std::unordered_map<std::string, std::string>& roles);

/**
 * One fruit body per patch of `caps` that has an origin, drawn by the child of the species `patches` sows on its
 * place: the shape that child holds (stamp_plant_caps), while it holds none the unit fruit body of the template
 * port, at the stem base of its patch, scaled by cap_scale from the radius the cap port carried. A child whose
 * species has no cap patch draws none; a patch without a record of the cap port, without an origin or of a species
 * the stage names no cap role for shows none. No caps node, nothing.
 */
void fill_plant_caps(::godot::Node3D* scene, const GodotHostResourceManager& resources,
                     const std::unordered_map<uint32_t, PlantOrigin>& origins,
                     const std::unordered_map<uint32_t, double>& caps, const std::vector<VivariumPatchSpec>& patches);

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - THE PARTS OF THE PLANT SCENE
 *
 * @file        godot_plant_scene_parts.hpp
 * @brief       Finds and adds the named nodes of the care view's 3D scene and hands an organ template on as a mesh
 * @description The plant scene stands in two units: godot_plant_scene.cpp lays camera, sun, ground and the plant
 *              nodes, godot_plant_scene_patch.cpp the bodies on the patch beside the plants - the seed bodies of
 *              ase-physics and the fruit bodies of the cap patches. Both find their nodes by name under the same
 *              scene root and draw on the templates of the template ports, so the lookup, the adding of a node and
 *              the template as a mesh stand here once, never once per unit.
 *
 *              WHY A HEADER OF ITS OWN: godot_plant_scene.hpp is read by the view, the node and the tests; the two
 *              templates below and the declaration of template_mesh are needed only by the two units that lay the
 *              scene, so only they compile again when these change.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <cstdint>

namespace ase::adp::godot {

class GodotHostResourceManager;

/** The node of `scene` with that name, cast to its class; nullptr when the scene has none. */
template <typename T>
T* part_of(::godot::Node3D* scene, const char* name) {
    return ::godot::Object::cast_to<T>(scene->get_node_or_null(::godot::NodePath(name)));
}

/** A node of class T under `scene` with that name. */
template <typename T>
T* add_part(::godot::Node3D* scene, const char* name) {
    T* part = memnew(T);
    part->set_name(name);
    scene->add_child(part);
    return part;
}

/**
 * The organ template `code` of the template ports as a mesh (usecase:101011 tessellate the organ templates): the
 * corners and triangles ase-render-data laid, through build_template. Empty while the ports carry no set.
 */
::godot::Ref<::godot::Mesh> template_mesh(const GodotHostResourceManager& resources, uint32_t code);

}  // namespace ase::adp::godot

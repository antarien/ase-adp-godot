#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE NODE
 *
 * @file        godot_figure_node.hpp
 * @brief       The names of a figure's node: where it stands in the scene, what it is called, what it carries as meta
 * @description Two units meet at the node of a figure: the figure scene (godot_figure_scene.cpp) builds it, places
 *              it and marks its meshes; the figure queries (godot_figure_query.cpp) read height, rig, place and
 *              face back from it for the view's ports and log lines. Both reach the node by the same path - the
 *              holder below the scene, the child named by the object id - and read the same metas, so the names
 *              stand once, here. Only these two units include this header; types.hpp, which nearly every unit
 *              of the adapter includes, does not carry them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101611 - animate character motion
 * @actor       SS/PC
 * @created     2026-11-15
 * @modified    2026-11-15
 * @version     00.00.01.00001
 */

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>

namespace ase::adp::godot {

inline constexpr const char* kFigureHolder = "FigureBodies";       // the node the figures stand under, one child per figure
inline constexpr const char* kFigurePrefix = "Figure";             // name of a figure's node: this and the object id of the figure
inline constexpr const char* kFigureMetaHeight = "figure_height";  // meta of a figure's node: height of its level 0, metres
inline constexpr const char* kFigureMetaEye = "figure_eye";        // meta of a figure's node: eye height above the soles from the rest pose of its rig, metres
inline constexpr const char* kFigureMetaRigTaken = "figure_rig_taken";  // meta of a figure's node: its rig was handed to the view for the rig port
inline constexpr const char* kFigureMetaPlaceNoted = "figure_place_noted";  // meta of a figure's node: the place last handed to the view's place line
inline constexpr const char* kFigureMetaLevel = "figure_lod";      // meta of a mesh of the figure file: its detail level

/** The node of `parent` with that name, cast to its class; nullptr when it has none. */
template <typename T>
T* figure_child_of(::godot::Node* parent, const ::godot::String& name) {
    return ::godot::Object::cast_to<T>(parent->get_node_or_null(::godot::NodePath(name)));
}

/** The name of a figure's node. */
inline ::godot::String figure_node_name(uint32_t object_id) {
    return ::godot::String(kFigurePrefix) + ::godot::String::num_uint64(object_id);
}

}  // namespace ase::adp::godot

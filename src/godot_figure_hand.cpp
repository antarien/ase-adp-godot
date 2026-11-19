/**
 * ASE GODOT ADAPTER - FIGURE HAND IMPLEMENTATION
 *
 * @file        godot_figure_hand.cpp
 * @brief       Hangs the asset of the tool a figure carries below its hand socket
 * @description See godot_figure_hand.hpp. The tool is one node below the hand socket (FigureTool) that carries
 *              the kind it was drawn for and whether it is an instance of its file as meta. The look of each
 *              tool is its asset: the glb the asset list names under the role hand.<item key>, built 1:1 from
 *              the mockup's ToolItem by ase-assgen (recipes tool_*, scripts/tool.py) with its origin at the grip
 *              the hand holds, so the adapter places nothing and builds no material. While the file is missing
 *              or loading, the tool is a block in the error material of the asset list (N0448). The kinds are
 *              matched by KernelEmbeddedHost::name_id of the item keys, the same hash ase-pl-avatar files.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
 * @usecase     usecase:101625 - carry a tool in hand
 * @actor       SS/PC
 * @created     2026-11-14
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_hand.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

namespace {

/**
 * THE TOOL NODE. Only this unit builds it, so its names stand here and not in types.hpp, which nearly every unit
 * of the adapter includes.
 */
constexpr const char* kHandTool = "FigureTool";            // name of the node below the hand socket that draws the carried piece
constexpr const char* kHandMetaKind = "figure_tool_kind";  // meta of the tool node: the kind it was drawn for
constexpr const char* kHandMetaFile = "figure_tool_file";  // meta of the tool node: true when it is an instance of its file
constexpr const char* kHandRolePrefix = "hand.";           // the roles of the asset list that name a tool of the hand
constexpr float kToolUnknownEdge = 0.08f;                  // a tool without its file: a block of this edge, metres
constexpr const char* kRackRole = "rack";                  // the role of the asset list that names the body of the tool place
constexpr const char* kToolPlace = "ToolPlace";            // the node below the scene that holds the rack body and the tools on the ground
constexpr const char* kRackBody = "RackBody";              // the node below the tool place that draws the rack body
constexpr const char* kGroundPrefix = "Ground_";           // the name of the node of a thing on the ground: this and the hash of its key
constexpr const char* kToolMetaBelow = "tool_below";       // meta of a tool node: metres from the grip down to its lowest point
constexpr const char* kToolMetaUnder = "tool_under";       // meta of a tool node: metres from its axis to its back, the side it lies on

/** One part of the tool below `tool`: `mesh` in `material`, its centre `drop` below the hand and `reach` ahead. */
void add_part(::godot::Node3D* tool, const ::godot::Ref<::godot::Mesh>& mesh,
              const ::godot::Ref<::godot::Material>& material, float drop, float reach) {
    ::godot::MeshInstance3D* part = memnew(::godot::MeshInstance3D);
    part->set_mesh(mesh);
    part->set_material_override(material);
    part->set_position(::godot::Vector3(0.0f, drop, reach));
    tool->add_child(part);
}

/** A block of the three edges. */
::godot::Ref<::godot::Mesh> block(float width, float height, float depth) {
    ::godot::Ref<::godot::BoxMesh> mesh;
    mesh.instantiate();
    mesh->set_size(::godot::Vector3(width, height, depth));
    return mesh;
}

/** An empty tool node of `kind`; `from_file` says whether it carries an instance of the tool's file. */
::godot::Node3D* tool_node(uint32_t kind, bool from_file) {
    ::godot::Node3D* tool = memnew(::godot::Node3D);
    tool->set_name(kHandTool);
    tool->set_meta(kHandMetaKind, static_cast<int64_t>(kind));
    tool->set_meta(kHandMetaFile, from_file);
    return tool;
}

/** The tool of `kind` as an instance of its loaded file `model`; the file's origin is the grip the hand holds. */
::godot::Node3D* file_tool(uint32_t kind, const ::godot::Ref<::godot::PackedScene>& model) {
    ::godot::Node3D* tool = tool_node(kind, true);
    tool->add_child(model->instantiate());
    return tool;
}

/** The tool of `kind` as a block in `material`, drawn while its file is missing, loading or does not load. */
::godot::Node3D* block_tool(uint32_t kind, const ::godot::Ref<::godot::Material>& material) {
    ::godot::Node3D* tool = tool_node(kind, false);
    add_part(tool, block(kToolUnknownEdge, kToolUnknownEdge, kToolUnknownEdge), material, 0.0f, 0.0f);
    return tool;
}

/**
 * Widens `bounds` by every mesh below `node`, each in the space of the node the walk started at (`placed` carries
 * the transforms down); `found` says whether one mesh was met.
 */
void merge_bounds(::godot::Node* node, const ::godot::Transform3D& placed, ::godot::AABB& bounds, bool& found) {
    for (int64_t i = 0; i < node->get_child_count(); ++i) {
        auto* child = ::godot::Object::cast_to<::godot::Node3D>(node->get_child(i));
        if (child == nullptr) {
            continue;
        }
        const ::godot::Transform3D here = placed * child->get_transform();
        if (auto* mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(child); mesh != nullptr) {
            const ::godot::AABB part = here.xform(mesh->get_aabb());
            bounds = found ? bounds.merge(part) : part;
            found = true;
        }
        merge_bounds(child, here, bounds, found);
    }
}

/**
 * Whether the ray from `origin` along `normal` (world space) meets a mesh below `node`, each tested against its own
 * box in its own space (`placed` carries the world transforms down). One box over all parts in world axes also
 * holds the empty space between and around them: a ground tap beside the rack met that box and opened the tool
 * place instead of digging (run 20261119-p0692-dig, the stab at 216,780 beside the toolrack place 96,799 63x59).
 */
bool ray_meets_part(::godot::Node* node, const ::godot::Transform3D& placed, const ::godot::Vector3& origin,
                    const ::godot::Vector3& normal) {
    for (int64_t i = 0; i < node->get_child_count(); ++i) {
        auto* child = ::godot::Object::cast_to<::godot::Node3D>(node->get_child(i));
        if (child == nullptr) {
            continue;
        }
        const ::godot::Transform3D here = placed * child->get_transform();
        if (auto* mesh = ::godot::Object::cast_to<::godot::MeshInstance3D>(child); mesh != nullptr) {
            const ::godot::Transform3D back = here.affine_inverse();
            if (mesh->get_aabb().intersects_ray(back.xform(origin), back.basis.xform(normal))) {
                return true;
            }
        }
        if (ray_meets_part(child, here, origin, normal)) {
            return true;
        }
    }
    return false;
}

/**
 * Notes on `tool` how far it reaches below its grip and how far its back lies off its axis, measured on what it
 * draws: its file's meshes, else its block. The file hangs down along -y from the grip and is thin along z
 * (scripts/tool.py of ase-assgen), so the lowest y is its foot and the lowest z the side it lies on.
 */
void measure_tool(::godot::Node3D* tool) {
    ::godot::AABB bounds;
    bool found = false;
    merge_bounds(tool, ::godot::Transform3D(), bounds, found);
    const float below = found ? -bounds.position.y : kToolUnknownEdge * 0.5f;
    const float under = found ? -bounds.position.z : kToolUnknownEdge * 0.5f;
    tool->set_meta(kToolMetaBelow, below);
    tool->set_meta(kToolMetaUnder, under);
}

/** The tool node of `kind` from `tools`: an instance of its file once loaded, else a block, measured for its pose. */
::godot::Node3D* ground_tool(uint32_t kind, const FigureHandTool& tool) {
    ::godot::Node3D* drawn = tool.model.is_valid() ? file_tool(kind, tool.model) : block_tool(kind, tool.stand_in);
    measure_tool(drawn);
    return drawn;
}

/** `along` on the ground and of unit length; the world's x when it has no part on the ground. */
::godot::Vector3 flat(const ::godot::Vector3& along) {
    ::godot::Vector3 ground(along.x, 0.0f, along.z);
    return ground.length_squared() > 0.0f ? ground.normalized() : ::godot::Vector3(1.0f, 0.0f, 0.0f);
}

/**
 * The pose of a tool leaning at the place: its foot on the ground at `foot`, its top turned by `lean` off the
 * vertical toward the crossbar that runs along `bar` through `rack`. The grip lies `below` above the foot along
 * the tool, so the file's lowest point touches the ground where the slot is.
 */
::godot::Transform3D leaning(const ::godot::Vector3& foot, const ::godot::Vector3& rack, const ::godot::Vector3& bar,
                             float lean, float below) {
    const ::godot::Vector3 up(0.0f, 1.0f, 0.0f);
    ::godot::Vector3 toward(rack.x - foot.x, 0.0f, rack.z - foot.z);
    toward -= bar * toward.dot(bar);  // only the part across the bar: the tool tips toward it, not along it
    toward = toward.length_squared() > 0.0f ? toward.normalized() : up.cross(bar);
    const ::godot::Vector3 shaft = (up * ::godot::Math::cos(lean) + toward * ::godot::Math::sin(lean)).normalized();
    const ::godot::Basis basis(bar, shaft, bar.cross(shaft));
    return ::godot::Transform3D(basis, foot + shaft * below);
}

/**
 * The pose of a tool lying where it was let go: its axis flat on the ground, turned by `turn` from `bar`, its
 * thin side down and its back `under` above `place`.
 */
::godot::Transform3D lying(const ::godot::Vector3& place, const ::godot::Vector3& bar, float turn, float under) {
    const ::godot::Vector3 up(0.0f, 1.0f, 0.0f);
    const ::godot::Vector3 points = bar.rotated(up, turn);  // the way the tool points from its grip
    const ::godot::Vector3 shaft = -points;                 // the file's +y runs from the foot to the grip
    const ::godot::Basis basis(shaft.cross(up), shaft, up);
    return ::godot::Transform3D(basis, place + up * under);
}

/** The node below `parent` named `name`, or a new empty one added there. */
::godot::Node3D* held_node(::godot::Node3D* parent, const ::godot::String& name) {
    auto* node = ::godot::Object::cast_to<::godot::Node3D>(parent->get_node_or_null(::godot::NodePath(name)));
    if (node == nullptr) {
        node = memnew(::godot::Node3D);
        node->set_name(name);
        parent->add_child(node);
    }
    return node;
}

/**
 * Keeps the one drawn child of `holder` while it was drawn for `kind` from the same source, else replaces it with
 * the node ground_tool builds from `tool`; returns the drawn child.
 */
::godot::Node3D* redraw(::godot::Node3D* holder, uint32_t kind, const FigureHandTool& tool) {
    auto* drawn = ::godot::Object::cast_to<::godot::Node3D>(holder->get_node_or_null(::godot::NodePath(kHandTool)));
    const bool from_file = tool.model.is_valid();
    if (drawn != nullptr && static_cast<uint32_t>(static_cast<int64_t>(drawn->get_meta(kHandMetaKind))) == kind &&
        static_cast<bool>(drawn->get_meta(kHandMetaFile, false)) == from_file) {
        return drawn;
    }
    if (drawn != nullptr) {
        holder->remove_child(drawn);
        drawn->queue_free();
    }
    drawn = ground_tool(kind, tool);
    holder->add_child(drawn);
    return drawn;
}

}  // namespace

std::unordered_map<uint32_t, FigureHandTool> bind_hand_tools(const VivariumAssetBinding& binding,
                                                            const std::unordered_map<std::string, std::string>& roles) {
    std::unordered_map<uint32_t, FigureHandTool> tools;
    const std::string prefix(kHandRolePrefix);
    for (const auto& [role, name] : roles) {
        if (role.size() <= prefix.size() || role.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        // the item key after the prefix keys the tool by the hash ase-pl-avatar files the piece's kind with
        FigureHandTool& tool = tools[ase::kernel::KernelEmbeddedHost::name_id(role.substr(prefix.size()).c_str())];
        tool.name = name;
        const auto file = binding.files.find(name);
        if (file != binding.files.end()) {
            tool.file = file->second;
        }
        tool.stand_in = error_material(binding, name);
    }
    return tools;
}

void carry_tool(::godot::Node3D* socket, uint32_t kind, const std::unordered_map<uint32_t, FigureHandTool>& tools,
                const ::godot::Ref<::godot::Material>& stand_in) {
    if (socket == nullptr) {
        return;
    }
    const auto tool = tools.find(kind);
    const bool from_file = tool != tools.end() && tool->second.model.is_valid();
    auto* drawn = ::godot::Object::cast_to<::godot::Node3D>(socket->get_node_or_null(::godot::NodePath(kHandTool)));
    if (drawn != nullptr && static_cast<uint32_t>(static_cast<int64_t>(drawn->get_meta(kHandMetaKind))) == kind &&
        static_cast<bool>(drawn->get_meta(kHandMetaFile, false)) == from_file) {
        return;  // the hand still holds the kind it was drawn with, from the same source
    }
    if (drawn != nullptr) {
        socket->remove_child(drawn);
        drawn->queue_free();
    }
    if (kind == 0u) {
        return;
    }
    if (from_file) {
        socket->add_child(file_tool(kind, tool->second.model));
        return;
    }
    socket->add_child(block_tool(kind, tool != tools.end() ? tool->second.stand_in : stand_in));
}

FigureHandTool bind_rack_body(const VivariumAssetBinding& binding,
                              const std::unordered_map<std::string, std::string>& roles) {
    FigureHandTool body;
    const auto role = roles.find(kRackRole);
    if (role != roles.end()) {
        body.name = role->second;
        const auto file = binding.files.find(body.name);
        if (file != binding.files.end()) {
            body.file = file->second;
        }
    }
    body.stand_in = error_material(binding, body.name);
    return body;
}

bool lay_tools(::godot::Node3D* scene, const ::godot::Vector3& rack, const ::godot::Vector3& along,
               const FigureHandTool& rack_body, std::vector<GroundThing>& things,
               const std::unordered_map<uint32_t, FigureHandTool>& tools) {
    if (scene == nullptr) {
        return false;
    }
    ::godot::Node3D* place = held_node(scene, kToolPlace);
    const ::godot::Vector3 bar = flat(along);
    const ::godot::Vector3 up(0.0f, 1.0f, 0.0f);

    // the rack body: the file's origin is the ground point between the stakes, its crossbar runs along its x
    ::godot::Node3D* body = held_node(place, kRackBody);
    (void)redraw(body, ase::kernel::KernelEmbeddedHost::name_id(kRackRole), rack_body);
    body->set_transform(::godot::Transform3D(::godot::Basis(bar, up, bar.cross(up)), rack));

    // every tool on the ground; the nodes of things that left the ground leave with them
    ::godot::Dictionary laid;
    for (GroundThing& thing : things) {
        const auto tool = tools.find(ase::kernel::KernelEmbeddedHost::name_id(thing.art.c_str()));
        if (tool == tools.end()) {
            continue;  // no tool of the hand: the thing keeps its drawing on the ground (WorldDrop)
        }
        thing.bodied = true;  // its body is the picture now; WorldDrop keeps its shadow and hit box
        // the node is named by the hash of the key: a key may carry signs a node name does not take
        const ::godot::String name = ::godot::String(kGroundPrefix) +
                                     ::godot::String::num_uint64(ase::kernel::KernelEmbeddedHost::name_id(thing.key.c_str()));
        laid[name] = true;
        ::godot::Node3D* holder = held_node(place, name);
        ::godot::Node3D* drawn = redraw(holder, tool->first, tool->second);
        const float angle = static_cast<float>(thing.angle);
        holder->set_transform(thing.at_rack
                                  ? leaning(thing.place, rack, bar, angle,
                                            static_cast<float>(drawn->get_meta(kToolMetaBelow, kToolUnknownEdge)))
                                  : lying(thing.place, bar, angle,
                                          static_cast<float>(drawn->get_meta(kToolMetaUnder, kToolUnknownEdge))));
    }
    for (int64_t i = place->get_child_count() - 1; i >= 0; --i) {
        ::godot::Node* child = place->get_child(i);
        const ::godot::String name = child->get_name();
        if (name.begins_with(kGroundPrefix) && !laid.has(name)) {
            place->remove_child(child);
            child->queue_free();
        }
    }
    return true;
}

bool rack_struck(::godot::Node3D* scene, const ::godot::Camera3D* camera, const ::godot::Vector2& at) {
    if (scene == nullptr || camera == nullptr) {
        return false;
    }
    // find, never stand: a tap before lay_tools stood the place meets nothing
    auto* body = ::godot::Object::cast_to<::godot::Node3D>(
        scene->get_node_or_null(::godot::NodePath(::godot::String(kToolPlace) + "/" + kRackBody)));
    if (body == nullptr || !body->is_inside_tree()) {
        return false;
    }
    return ray_meets_part(body, body->get_global_transform(), camera->project_ray_origin(at),
                          camera->project_ray_normal(at));
}

uint32_t hand_kind(uint32_t figure, const std::vector<VivariumFigureGearView>& gear, const char* slot) {
    const uint32_t slot_id = ase::kernel::KernelEmbeddedHost::name_id(slot);
    for (const VivariumFigureGearView& piece : gear) {
        if (piece.bearer == figure && piece.slot == slot_id) {
            return piece.kind;
        }
    }
    return 0u;
}

}  // namespace ase::adp::godot

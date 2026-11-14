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
 * @modified    2026-11-14
 * @version     00.00.02.00002
 */

#include <ase/adp/godot/godot_figure_hand.hpp>
#include <ase/adp/godot/godot_asset_loader.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/node_path.hpp>
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

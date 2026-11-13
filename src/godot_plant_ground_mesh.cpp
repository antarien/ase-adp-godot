/**
 * ASE GODOT ADAPTER - GROUND SURFACE IN GODOT
 *
 * @file        godot_plant_ground_mesh.cpp
 * @brief       Reads where Godot keeps the ground's vertices and hands it the spans a patch wrote
 * @description P0690, a dig reaches the drawing as a change (CAUSA_ASE_TERRAIN). After every full build of the
 *              ground, hold_ground_surface asks RenderingServer where the surface's format places the
 *              position, the normal, the colour and the texture coordinates of a vertex, and reads the
 *              vertex and attribute streams Godot made of the surface into the record patch_ground writes
 *              into (godot_plant_ground.cpp). The record is the input of the patch: patch_ground writes the
 *              moved vertices into it in Godot's code, and send_ground_patch hands Godot only the spans it
 *              names, through surface_update_vertex_region and surface_update_attribute_region. The streams
 *              are read once per full build, never per frame: in the RenderingDevice drivers
 *              mesh_get_surface reads the buffers back from the GPU.
 *
 *              A patch is written only once ground_code_holds found the first vertex coded byte for byte as
 *              Godot coded it; otherwise the record stays unpatchable and every change builds the ground anew.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_plant_ground.hpp>
#include <ase/adp/godot/godot_plant_scene.hpp>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The bytes of one stream Godot made of the surface, read into `out` in their order. */
void read_stream(const ::godot::PackedByteArray& stream, std::vector<uint8_t>& out) {
    const int64_t size = stream.size();
    out.resize(static_cast<size_t>(size));
    const uint8_t* from = stream.ptr();
    for (int64_t at = 0; at < size; ++at) {
        out[static_cast<size_t>(at)] = from[at];
    }
}

/** The bytes of `stream` from `span.begin` up to `span.end`, as Godot takes a region update. */
::godot::PackedByteArray span_of(const std::vector<uint8_t>& stream, const PlantGroundSpan& span) {
    ::godot::PackedByteArray out;
    const uint32_t size = span.end - span.begin;
    out.resize(size);
    uint8_t* to = out.ptrw();
    for (uint32_t at = 0u; at < size; ++at) {
        to[at] = stream[span.begin + at];
    }
    return out;
}

}  // namespace

void hold_ground_surface(const ::godot::Ref<::godot::ArrayMesh>& mesh, PlantGroundSurface& surface) {
    surface.patchable = false;
    surface.vertex_bytes.clear();
    surface.attribute_bytes.clear();
    if (mesh.is_null() || mesh->get_surface_count() < 1) {
        return;
    }
    ::godot::RenderingServer* server = ::godot::RenderingServer::get_singleton();
    const int32_t vertices = mesh->surface_get_array_len(0);
    if (server == nullptr || vertices <= 0 ||
        static_cast<size_t>(vertices) * 3u != surface.arrays.positions.size()) {
        return;
    }
    const ::godot::BitField<::godot::RenderingServer::ArrayFormat> format(
        static_cast<int64_t>(mesh->surface_get_format(0)));
    PlantGroundLayout& layout = surface.layout;
    layout.position_offset = server->mesh_surface_get_format_offset(format, vertices, ::godot::RenderingServer::ARRAY_VERTEX);
    layout.position_stride = server->mesh_surface_get_format_vertex_stride(format, vertices);
    layout.normal_offset = server->mesh_surface_get_format_offset(format, vertices, ::godot::RenderingServer::ARRAY_NORMAL);
    layout.normal_stride = server->mesh_surface_get_format_normal_tangent_stride(format, vertices);
    layout.color_offset = server->mesh_surface_get_format_offset(format, vertices, ::godot::RenderingServer::ARRAY_COLOR);
    layout.uv_offset = server->mesh_surface_get_format_offset(format, vertices, ::godot::RenderingServer::ARRAY_TEX_UV);
    layout.attribute_stride = server->mesh_surface_get_format_attribute_stride(format, vertices);

    const ::godot::Dictionary held = server->mesh_get_surface(mesh->get_rid(), 0);
    read_stream(held["vertex_data"], surface.vertex_bytes);
    read_stream(held["attribute_data"], surface.attribute_bytes);

    const ::godot::AABB box = mesh->get_aabb();
    surface.low_m = box.position.y;
    surface.high_m = box.position.y + box.size.y;
    surface.patchable = ground_code_holds(surface);
    if (!surface.patchable) {
        WARN_PRINT_ONCE("ground surface: Godot codes a vertex otherwise than encode_ground, every dig builds anew");
    }
}

void send_ground_patch(const ::godot::Ref<::godot::ArrayMesh>& mesh, const PlantGroundSurface& surface) {
    if (mesh.is_null()) {
        return;
    }
    if (surface.positions.end > surface.positions.begin) {
        mesh->surface_update_vertex_region(0, static_cast<int32_t>(surface.positions.begin),
                                           span_of(surface.vertex_bytes, surface.positions));
    }
    if (surface.normals.end > surface.normals.begin) {
        mesh->surface_update_vertex_region(0, static_cast<int32_t>(surface.normals.begin),
                                           span_of(surface.vertex_bytes, surface.normals));
    }
    if (surface.attributes.end > surface.attributes.begin) {
        mesh->surface_update_attribute_region(0, static_cast<int32_t>(surface.attributes.begin),
                                              span_of(surface.attribute_bytes, surface.attributes));
    }
    if (surface.grown) {
        ::godot::AABB box = mesh->get_aabb();
        box.position.y = surface.low_m;
        box.size.y = surface.high_m - surface.low_m;
        mesh->set_custom_aabb(box);
    }
}

}  // namespace ase::adp::godot

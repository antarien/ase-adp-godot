/**
 * ASE GODOT ADAPTER - FIGURE EDGE GLASS
 *
 * @file        godot_figure_glass.cpp
 * @brief       The red glass pane that flashes where a walking body bumps the edge of the first own chunk
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.13: one quad per bumping body under the holder FigureEdgeGlass of
 *              the scene, drawn with figure_edge_glass.gdshader. The pane keeps the count of bumps it flashed for
 *              and its age as metas of its node, so a new bump of the boundary port (avatar.boundary.v1) reads as a
 *              changed count and the pane flashes once per bump, never per frame. Its glow is an instance uniform:
 *              one material for all panes, each pane fading on its own.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_019
 * @category    render/pipeline/stage
 * @usecase     usecase:101612 - keep figure in loaded area
 * @actor       Game Areal Plate (AREAL)
 * @created     2026-11-18
 * @modified    2026-11-18
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_figure_glass.hpp>
#include <ase/adp/godot/godot_plant_shader.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr const char* kGlassShader = "res://shaders/figure_edge_glass.gdshader";  // the shader of the pane
constexpr const char* kGlassHolder = "FigureEdgeGlass";   // the node the panes stand under, one child per bumping body
constexpr const char* kGlassPane = "FigureEdgeGlass_";    // name of one pane, followed by the object id of the body
constexpr const char* kGlassColor = "glass_color";        // uniform of figure_edge_glass.gdshader: FIGURE_EDGE_GLASS
constexpr const char* kGlassRim = "rim_power";            // uniform of figure_edge_glass.gdshader: hud_figure_edge_glass_rim
constexpr const char* kGlassGlow = "glow";                // instance uniform of figure_edge_glass.gdshader: 1 at the bump, 0 when gone
constexpr const char* kGlassMetaBumps = "figure_edge_bumps";  // meta of a pane: the bump count of the boundary port it flashed for
constexpr const char* kGlassMetaAge = "figure_edge_age";      // meta of a pane: seconds since it flashed
constexpr double kGlassFull = 1.0;  // glow of the pane during the hold
constexpr double kGlassHalf = 0.5;  // the pane's centre stands half its height above the feet

/** The pane of the body `object_id` below `holder`; nullptr while it has none. */
::godot::MeshInstance3D* pane_of(::godot::Node3D* holder, const ::godot::String& name) {
    return ::godot::Object::cast_to<::godot::MeshInstance3D>(holder->get_node_or_null(::godot::NodePath(name)));
}

/** A new pane of the glass's size and material, standing under `holder`. */
::godot::MeshInstance3D* stand_pane(::godot::Node3D* holder, const ::godot::String& name, const FigureSceneGlass& glass) {
    ::godot::Ref<::godot::QuadMesh> quad;
    quad.instantiate();
    quad->set_size(::godot::Vector2(static_cast<float>(glass.width), static_cast<float>(glass.height)));
    ::godot::MeshInstance3D* pane = memnew(::godot::MeshInstance3D);
    pane->set_name(name);
    pane->set_mesh(quad);
    pane->set_material_override(glass.material);
    pane->set_cast_shadows_setting(::godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
    holder->add_child(pane);
    return pane;
}

}  // namespace

FigureSceneGlass bind_figure_glass(double width, double height, double hold, double fade, double rim,
                                   const ::godot::Color& color) {
    FigureSceneGlass glass;
    glass.width = width;
    glass.height = height;
    glass.hold = hold;
    glass.fade = fade;
    const ::godot::Ref<::godot::Shader> shader = load_plant_shader(kGlassShader);
    if (shader.is_null()) {
        return glass;
    }
    glass.material.instantiate();
    glass.material->set_shader(shader);
    glass.material->set_shader_parameter(kGlassColor, color);
    glass.material->set_shader_parameter(kGlassRim, rim);
    return glass;
}

void flash_figure_edges(::godot::Node3D* scene, const std::vector<VivariumFigureBumpView>& bumps,
                        const std::vector<VivariumFigureView>& figures, const FigureSceneGlass& glass,
                        uint32_t held_figure, bool held, double elapsed_s) {
    if (scene == nullptr || glass.material.is_null()) {
        return;
    }
    ::godot::Node3D* holder = ::godot::Object::cast_to<::godot::Node3D>(scene->get_node_or_null(::godot::NodePath(kGlassHolder)));
    if (holder == nullptr) {
        holder = memnew(::godot::Node3D);
        holder->set_name(kGlassHolder);
        scene->add_child(holder);
    }
    // a bump the pane has not flashed for: the count of the boundary port moved, or the body bumps the first time the
    // view sees it. The port lays a record only for a body that bumped, so a record without a pane is a fresh touch.
    for (const VivariumFigureBumpView& bump : bumps) {
        const ::godot::String name = ::godot::String(kGlassPane) + ::godot::String::num_uint64(bump.object_id);
        ::godot::MeshInstance3D* pane = pane_of(holder, name);
        if (pane != nullptr && pane->has_meta(kGlassMetaBumps) &&
            static_cast<int64_t>(pane->get_meta(kGlassMetaBumps)) == static_cast<int64_t>(bump.bumps)) {
            continue;
        }
        // the feet of the body: the record carries the touched point on the ground plane, the figure carries its height
        const VivariumFigureView* body = nullptr;
        for (const VivariumFigureView& figure : figures) {
            if (figure.object_id == bump.object_id) {
                body = &figure;
                break;
            }
        }
        if (body == nullptr) {
            continue;
        }
        if (pane == nullptr) {
            pane = stand_pane(holder, name, glass);
        }
        pane->set_meta(kGlassMetaBumps, static_cast<int64_t>(bump.bumps));
        pane->set_meta(kGlassMetaAge, 0.0);
        // the quad faces +z; turned around y it faces along the outward normal (x east, z north, VivariumFigureBumpView)
        pane->set_position(::godot::Vector3(static_cast<float>(bump.touch_x),
                                            static_cast<float>(body->y + glass.height * kGlassHalf),
                                            static_cast<float>(bump.touch_z)));
        pane->set_rotation(::godot::Vector3(
            0.0f, ase::math::atan2(static_cast<float>(bump.normal_x), static_cast<float>(bump.normal_z)), 0.0f));
        pane->set_visible(true);
    }
    // the rim holds the figure (camera.edge or camera.bound of this frame): its pane stands at full glow at the last
    // touched point and its hold starts anew, so it fades only from the frame the rim lets the figure go (M0157, 07-G)
    if (held) {
        ::godot::MeshInstance3D* pane = pane_of(holder, ::godot::String(kGlassPane) + ::godot::String::num_uint64(held_figure));
        if (pane != nullptr) {
            pane->set_meta(kGlassMetaAge, 0.0);
            pane->set_visible(true);
        }
    }
    // every shown pane ages by the frame: full glow during the hold, then down to nothing over the fade, then hidden
    for (int32_t i = 0; i < holder->get_child_count(); ++i) {
        ::godot::MeshInstance3D* pane = ::godot::Object::cast_to<::godot::MeshInstance3D>(holder->get_child(i));
        if (pane == nullptr || !pane->is_visible() || !pane->has_meta(kGlassMetaAge)) {
            continue;
        }
        const double age = static_cast<double>(pane->get_meta(kGlassMetaAge)) + elapsed_s;
        pane->set_meta(kGlassMetaAge, age);
        if (age >= glass.hold + glass.fade) {
            pane->set_instance_shader_parameter(kGlassGlow, 0.0);
            pane->set_visible(false);
            continue;
        }
        const double glow = age <= glass.hold ? kGlassFull : kGlassFull - (age - glass.hold) / glass.fade;
        pane->set_instance_shader_parameter(kGlassGlow, glow);
    }
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD UI ANCHOR
 *
 * @file        godot_hud_ui_anchor.cpp
 * @brief       Projects a world place through the scene camera onto the HUD's logical surface
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_anchor.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>

#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/rect2.hpp>

namespace ase::adp::godot {

void UiAnchor::follow(const ::godot::Camera3D* camera, const ::godot::Transform2D& frame) {
    camera_ = camera;
    // A frame without area has no way back: no place gets an anchor while it stands.
    framed_ = frame.determinant() != 0.0f;
    forth_ = frame;
    back_ = framed_ ? frame.affine_inverse() : ::godot::Transform2D();
    if (camera_ == nullptr) {
        return;
    }
    // The camera of this frame, read once; place answers every point from it (P0692).
    const ::godot::Transform3D eye = camera_->get_global_transform();
    eye_origin_ = eye.origin;
    eye_dir_ = -eye.basis.get_column(2).normalized();
    near_ = camera_->get_near();
    view_ = camera_->get_camera_transform();
    lens_ = camera_->get_camera_projection();
    const ::godot::Viewport* viewport = camera_->get_viewport();
    screen_ = viewport != nullptr ? viewport->get_visible_rect().size : ::godot::Vector2();
}

bool UiAnchor::place(const ::godot::Vector3& world, ::godot::Vector2& out) const {
    // Camera3D::is_position_behind: the point lies less than the near distance ahead along the eye's forward.
    if (camera_ == nullptr || !framed_ || eye_dir_.dot(world - eye_origin_) < near_) {
        return false;
    }
    // Camera3D::unproject_position: the point into the eye's space, through the projection, the clip space onto
    // the viewport's pixels; a point on the eye's plane (d 0) has no place.
    ::godot::Plane clip(view_.xform_inv(world), 1.0f);
    clip = lens_.xform4(clip);
    if (clip.d == 0.0f) {
        return false;
    }
    clip.normal /= clip.d;
    const ::godot::Vector2 seen((clip.normal.x * 0.5f + 0.5f) * screen_.x, (-clip.normal.y * 0.5f + 0.5f) * screen_.y);
    // The canvas point the camera sees `world` at, back through the HUD's frame into mockup units - the way
    // a touch is mapped, so a mark stands where its tap is taken.
    out = back_.xform(seen);
    return true;
}

bool UiAnchor::lift(const ::godot::Vector2& at, const ::godot::Vector3& depth_of, ::godot::Vector3& out) const {
    if (camera_ == nullptr || !framed_ || camera_->is_position_behind(depth_of)) {
        return false;
    }
    // place run backwards: mockup units onto the canvas, then out of the camera at the distance `depth_of` lies
    // ahead of it along its forward (its negative z axis).
    const float depth = -camera_->get_global_transform().xform_inv(depth_of).z;
    out = camera_->project_position(forth_.xform(at), depth);
    return true;
}

bool UiAnchor::ground(const ::godot::Vector2& at, float level, ::godot::Vector3& out) const {
    if (camera_ == nullptr || !framed_) {
        return false;
    }
    // The ray the camera sees the canvas point along, met with the level plane y = level.
    const ::godot::Vector2 seen = forth_.xform(at);
    const ::godot::Vector3 from = camera_->project_ray_origin(seen);
    const ::godot::Vector3 along = camera_->project_ray_normal(seen);
    if (::godot::Math::is_zero_approx(along.y)) {
        return false;  // the ray runs level and never meets the plane
    }
    const float run = (level - from.y) / along.y;
    if (run <= 0.0f) {
        return false;  // the plane lies behind the camera along this ray
    }
    out = from + along * run;
    return true;
}

bool UiAnchor::horizon(double& y) const {
    if (camera_ == nullptr) {
        return false;
    }
    // The line of sight laid level: the camera's forward (its negative z axis) without its upward share.
    const ::godot::Transform3D eye = camera_->get_global_transform();
    ::godot::Vector3 level = -eye.basis.get_column(2);
    level.y = 0.0f;
    if (level.is_zero_approx()) {
        return false;
    }
    ::godot::Vector2 out;
    if (!place(eye.origin + level.normalized() * camera_->get_far(), out)) {
        return false;
    }
    y = static_cast<double>(out.y);
    return true;
}

bool UiAnchor::eye(::godot::Vector3& out) const {
    if (camera_ == nullptr) {
        return false;
    }
    out = eye_origin_;
    return true;
}

bool UiAnchor::trunk(const PlantOrigin& origin, ::godot::Vector2& out) const {
    return place(::godot::Vector3(origin.x, origin.y, origin.z), out);
}

bool UiAnchor::crown(const PlantOrigin& origin, const std::vector<VivariumInstanceView>& segments,
                     ::godot::Vector2& out) const {
    // rows: x, y, z axis and origin per row (GODOT_INSTANCE_VALUES); the unit tube runs from its origin
    // along its y axis, so its far end is origin plus that axis.
    bool found = false;
    ::godot::Vector3 top;
    for (const VivariumInstanceView& segment : segments) {
        const ::godot::Vector3 end(origin.x + segment.rows[3] + segment.rows[1],
                                   origin.y + segment.rows[7] + segment.rows[5],
                                   origin.z + segment.rows[11] + segment.rows[9]);
        if (!found || end.y > top.y) {
            top = end;
            found = true;
        }
    }
    return found && place(top, out);
}

}  // namespace ase::adp::godot

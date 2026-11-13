#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI ANCHOR
 *
 * @file        godot_hud_ui_anchor.hpp
 * @brief       World to screen: where a thing, a crown, a trunk or a brook point stands on the HUD's surface
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, building block Basis. Information about a thing
 *              hangs on the thing (zone Z2, the world): a leader line starts at its anchor, a sign sits at
 *              its place. UiAnchor turns the world place into a point of the HUD's frame, in the mockup's
 *              units (hud_frame_width wide), through the camera of the scene the view built.
 *
 *              THE CAMERA OF THE SCENE, NOT A SECOND ONE. follow takes the 3D camera of the viewport and the
 *              frame of the HUD (the transform from mockup units onto the canvas the view draws every block
 *              with) once per frame; place projects with Camera3D::unproject_position, as pick_segment and
 *              pick_ground do for a tap, and maps the canvas point back through the inverse of that frame -
 *              the same way a touch is mapped, so a block draws a mark exactly where its tap is taken.
 *              A place behind the camera has no anchor: place answers false and the block draws nothing at
 *              it, never a point at an edge.
 *
 *              THE FOUR PLACES OF THE MOCKUP. A thing and a brook point are a world place the caller holds
 *              (a thing's place from the inventory port, a brook point from the stream port and the ground
 *              under it); the trunk is the stem base the scene placed (PlantOrigin); the crown is the highest
 *              tube end of the plant's internodes, each tube running from its origin along its y axis
 *              (VivariumInstanceView rows, the unit tube of the organ template). The anchor computes no
 *              growth and no height a module computes: every place is a value the ports reported.
 *
 *              THE WAY BACK. lift is place run backwards: a point of the frame in mockup units through the
 *              frame onto the canvas, then out of the camera (Camera3D::project_position) at the depth a given
 *              world place lies along its line of sight. The bridge lays what the mockup draws in its own
 *              units into the world this way while a port is silent, so a block that places world points draws
 *              it exactly where the mockup does.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-13
 * @version     00.00.01.00003
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/variant/projection.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <vector>

namespace ase::adp::godot {

/** The projection of world places into the HUD's frame for one frame. */
class UiAnchor {
public:
    /**
     * The camera and the HUD's frame of this frame (mockup units onto the canvas); a null camera or a frame
     * without area leaves every place without anchor.
     */
    void follow(const ::godot::Camera3D* camera, const ::godot::Transform2D& frame);

    /** The point of the frame, in mockup units, `world` is seen at; false behind the camera or without one. */
    bool place(const ::godot::Vector3& world, ::godot::Vector2& out) const;

    /**
     * The world point the frame shows at `at`, in mockup units, at the depth `depth_of` lies along the camera's
     * line of sight; place of it gives `at` again. False without a camera or frame, or with `depth_of` behind it.
     */
    bool lift(const ::godot::Vector2& at, const ::godot::Vector3& depth_of, ::godot::Vector3& out) const;

    /**
     * The world point the frame shows at `at`, in mockup units, on the level plane at height `level`: where
     * the camera's ray through that point (Camera3D::project_ray_origin, project_ray_normal) meets the plane.
     * place of it gives `at` again. A mark the mockup lays flat on the ground (the tree's shade, the seed mound,
     * PLAN_ASE_VIVARIUM_PHASE_06_HUD row N19) lies there in the scene, so the trunk stands before it. Unlike
     * VivariumView::pick_ground, which snaps a tap to the nearest terrain sample, every point keeps its own
     * place, so an outline stays smooth. False without a camera or frame, with a ray running level, or with the
     * plane behind the camera.
     */
    bool ground(const ::godot::Vector2& at, float level, ::godot::Vector3& out) const;

    /**
     * The horizon: the height in the frame, in mockup units, where the camera's level line of sight meets the
     * far end of its view (Camera3D::get_far). False without a camera or while it looks straight up or down.
     */
    bool horizon(double& y) const;

    /** The trunk: the stem base the scene placed the plant at. */
    bool trunk(const PlantOrigin& origin, ::godot::Vector2& out) const;

    /**
     * The crown: the highest tube end among `segments`, the internode instances of ONE plant whose stem base
     * is `origin`. False without any internode or when that end lies behind the camera.
     */
    bool crown(const PlantOrigin& origin, const std::vector<VivariumInstanceView>& segments,
               ::godot::Vector2& out) const;

private:
    const ::godot::Camera3D* camera_ = nullptr;  // the 3D camera of the viewport this frame
    ::godot::Transform2D forth_;                 // the HUD's frame: mockup units onto the canvas
    ::godot::Transform2D back_;                  // the inverse of the HUD's frame: canvas onto mockup units
    bool framed_ = false;                        // the frame has an area: back_ is its inverse

    // THE CAMERA OF THIS FRAME, read once by follow (P0692). place ran Camera3D::is_position_behind and
    // unproject_position per point, and each call fetched the camera's transform, its projection and the
    // viewport's size again across the extension boundary; the ground lattice places side * side points per frame
    // (TerrainKit::lay_lattice), and the layer ground_terrain cost 19 to 21 ms of the main thread in run
    // 20261113-p0692-split. place computes the same two answers from these values, the engine's own formulas.
    ::godot::Transform3D view_;        // Camera3D::get_camera_transform: the eye with its offsets
    ::godot::Projection lens_;         // Camera3D::get_camera_projection
    ::godot::Vector3 eye_origin_;      // the origin of Camera3D::get_global_transform
    ::godot::Vector3 eye_dir_;         // its forward: the negative z column, normalised
    ::godot::Vector2 screen_;          // the size of the viewport's visible rect
    float near_ = 0.0f;                // Camera3D::get_near
};

}  // namespace ase::adp::godot

#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI GROUND
 *
 * @file        godot_hud_ui_ground.hpp
 * @brief       The ground as the terrain port laid it, read at a world place: its height and its point on the HUD
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, building block Basis, for TerrainKit and GroundReticle
 *              (06.4). The overlay of the ground mode lies ON the drawn ground: the lines of the mesh, the arrows
 *              on the water of a brook, the ring of the brush and of the root zone. Each of them needs the height
 *              the ground stands at under a world place and the point of the logical surface that place is seen
 *              at.
 *
 *              THE SAMPLES OF THE PORT, ON THE SQUARE THE VIEW LAID. ase-terrain samples the square of the
 *              start configuration (VivariumGroundSpec: centre, half edge, step) and the terrain port carries
 *              one sample per lattice point of it. follow indexes the samples by their lattice point, column
 *              (x - (centre x - half edge)) / step and row likewise north, rounded, once per new terrain read
 *              (terrain_generation). A lattice point the port carried no sample for stays
 *              GODOT_HUD_GROUND_NO_SAMPLE. height interpolates on the triangle of the drawn mesh the place
 *              lies on: build_ground draws each cell as (sw, se, ne) and (sw, ne, nw), so a place east of the
 *              diagonal from sw to ne stands on the plane of the first and one north of it on the plane of the
 *              second - the height the drawn mesh shows there. A height linear in both directions over the four
 *              corners leaves that surface by up to a quarter of the cell's twist, centimetres on a carved bank.
 *              It was not what hid the water of the brook: on the triangles the same 1716 of 1716 water points
 *              stay hidden (B0285, run 20261113-d2309-tri), because the water stands 0.43 m under the lip of a
 *              bed carved 0.6 m deep, and the eye 1.9 m high and 9.5 m off looks over that lip onto the far
 *              wall; the picture holds no water colour below the horizon. It computes no height the module
 *              does not carry and answers false off the square or with a corner missing. On the far edge of
 *              the square the last cell carries the place, so the edge itself has a height.
 *
 *              WHAT THE EYE SEES OVER THE GROUND. sees walks the line from the camera to a world point at half
 *              a lattice step, the finest the drawn mesh bends at, and answers false where the ground stands
 *              above it. The camera looks flat over the bank of a carved bed (B0285, run 20261113-1951-m05-rain):
 *              the lip hides the near water, and a mark laid on that hidden water lands on the brown bank under
 *              the strip the eye sees. Where the lip hides all of it, the strip the eye sees is the carved bed
 *              itself, and the mark follows that (build_bed_line).
 *
 *              A PLACE BY ITS SHARE. across names the world place a share of the square's edge east and a
 *              share north of its south-west corner, at the height height gives there. The lines of the
 *              drawn mesh run through such places, as many as the mockup's heightfield has (TerrainKit).
 *
 *              A RING IS ALL OR NOTHING. ring lays steps + 1 points around a centre at a radius on the
 *              ground, the last on the first, each at its ground height and projected through UiAnchor. One
 *              point off the square or behind the camera and the ring answers false: the caller draws no
 *              ring, never one cut at an edge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_hud_ui_anchor.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** The ground of the scene, indexed on the lattice of its square. */
class UiGround {
public:
    /** Indexes `samples` on the lattice of `square` when `generation` is not the one indexed last. */
    void follow(const std::vector<VivariumTerrainView>& samples, const VivariumGroundSpec& square,
                uint64_t generation);

    /** Samples per side of the square; 0 before the first follow or for a square without step. */
    [[nodiscard]] int64_t side() const;

    /** The sample at lattice point `column`, `row`; nullptr off the lattice or where the port carried none. */
    [[nodiscard]] const VivariumTerrainView* at(int64_t column, int64_t row) const;

    /** The ground height under `x`, `z`, metres; false off the square or with a corner sample missing. */
    bool height(double x, double z, double& out) const;

    /**
     * Whether `eye` sees `point` over the ground: false where the ground rises above the line between them at
     * one of the places half a lattice step apart along it. A place off the square tells nothing; the point's
     * own place is not asked, so water standing over its ground is seen where its banks let the eye through.
     * `over` takes the first place whose ground stands above the line, its y that ground; kept where it is seen.
     */
    [[nodiscard]] bool sees(const ::godot::Vector3& eye, const ::godot::Vector3& point, ::godot::Vector3& over) const;

    /** The world place `share_x` across and `share_z` along the square (0 to 1 each) at its ground height. */
    bool across(double share_x, double share_z, ::godot::Vector3& out) const;

    /** The point of the logical surface `x`, `z` is seen at, `lift` metres over the ground; false without one. */
    bool surface(const UiAnchor& anchor, double x, double z, double lift, ::godot::Vector2& out) const;

    /** steps + 1 points of the ring around `x`, `z` at `radius` metres on the ground; false when one has no point. */
    bool ring(const UiAnchor& anchor, double x, double z, double radius, int64_t steps,
              ::godot::PackedVector2Array& out) const;

private:
    /** The lattice coordinate of `world` along an axis starting at `origin`. */
    [[nodiscard]] int64_t lattice(double world, double origin) const;

    const std::vector<VivariumTerrainView>* samples_ = nullptr;  // the samples indexed last
    std::vector<int64_t> index_;  // per lattice point (row * side + column) the sample, GODOT_HUD_GROUND_NO_SAMPLE
    double origin_x_ = 0.0;       // the west edge of the square, world metres east
    double origin_z_ = 0.0;       // the south edge of the square, world metres north
    double step_ = 0.0;           // the distance of two samples, metres
    int64_t side_ = 0;            // samples per side
    uint64_t generation_ = 0;     // the terrain read indexed last
    bool followed_ = false;       // follow ran once
};

}  // namespace ase::adp::godot

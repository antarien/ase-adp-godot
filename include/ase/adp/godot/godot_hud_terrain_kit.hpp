#pragma once

/**
 * ASE GODOT ADAPTER - HUD TERRAIN KIT
 *
 * @file        godot_hud_terrain_kit.hpp
 * @brief       The ground mode over the ground: the lines of the mesh, the arrows of the brook, the root zone, the reticle
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, building block TerrainKit (the ground tool kit,
 *              usecase:100389), as the mockup's drawGround in the mode terrain and its drawTerrainOverlay draw it.
 *              The kit draws only while a ground tool is in hand; it changes no ground - every tap is a request
 *              at the port of the module that owns the ground, sent by the view.
 *
 *              THE LINES OF THE DRAWN MESH. Over the ground lies a lattice of hud_ground_columns points across
 *              the square and hud_ground_rows along it, as the mockup's heightfield has NX by NZ points (m.html
 *              360, drawGround 434); each point stands at the height UiGround interpolates from the samples of
 *              the terrain port under it, so the lines keep the mockup's density at any step the port samples
 *              at. Every row is drawn, and every hud_terrain_column_every-th column. A row fades with
 *              its depth as the mockup's rows do: its share between the highest and the lowest row on the
 *              surface this frame sets its alpha from hud_terrain_grid_alpha up by hud_terrain_grid_alpha_depth,
 *              at hud_terrain_grid_w; a column draws at hud_terrain_column_alpha and hud_terrain_column_w, all
 *              in DOMAIN_GROUND. A lattice point without a sample or behind the camera breaks its line. The
 *              mockup strokes every heightfield row in every mode, so outside the ground mode the rows lie there
 *              too (layer ground_rows, draw_rows): in OVERLAY_SHADOW from hud_ground_line_alpha up by
 *              hud_ground_line_alpha_depth, hud_ground_line_w wide, without columns. The mode terrain names the
 *              layer ground_terrain instead (draw_mesh), so the two never lie over each other.
 *
 *              THE ARROWS OF THE BROOK. Along every brook the stream port carries water for (a bed without
 *              measured water has no record and no arrow), hud_terrain_flow_count arrows in DOMAIN_FLOW ride
 *              the water surface - the ground under the bed plus the record's depth - spaced evenly along the
 *              bed and moving from its start to its end at the record's mean velocity. An arrow points the way
 *              the water runs: from the point hud_terrain_flow_tail of the bed's length behind it, its tip
 *              hud_terrain_flow_tip ahead, its back hud_terrain_flow_back behind and hud_terrain_flow_half to
 *              each side. The mockup turns its arrows against their travel; the kit follows the water.
 *
 *              THE ROOT ZONE. The radius of the zone the roots of the patch hold is the reach of the farthest
 *              living root tip of its body: ase-radix publishes it as RDX_SPREAD, ase-pl-flora files it in the
 *              patch record as root_m (FloraInpSoilComponent.spread_m), and the hand bridge hands it into the
 *              frame (N0463). A body without a living tip holds no ground, and then no ring stands; the ring
 *              grows as the roots spread. It lies on the ground around the trunk, hud_terrain_root_steps
 *              points dashed hud_terrain_root_dash on, hud_terrain_root_gap off in DOMAIN_ALERT at
 *              hud_terrain_root_alpha, hud_terrain_line_w wide.
 *
 *              THE CRUMBS OF LOOSENED SOIL (row N08, m.html drawGround 441 to 444). In every mode the rows and
 *              the mesh carry the crumbs GroundCrumbs draws from the loosened share of each ground point, as the
 *              loose bridge keeps it while flora.patch.v1 carries no loose soil (N0456).
 *
 *              Last the GroundReticle: the brush and the bed point.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0463 (the radius of the root zone: the patch record's root_m, RDX_SPREAD), N0456 (flora.patch.v1 loose), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_stream_view.hpp>
#include <ase/adp/godot/godot_hud_ground_crumbs.hpp>
#include <ase/adp/godot/godot_hud_ground_reticle.hpp>
#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_ground.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the ground mode shows this frame. */
struct TerrainKitFrame {
    const UiAnchor* anchor = nullptr;                          // the projection of this frame
    const UiGround* ground = nullptr;                          // the ground as the terrain port laid it
    const std::vector<VivariumStreamView>* streams = nullptr;  // the brooks with water (stream port)
    double elapsed = 0.0;                                      // seconds the ground mode has been open
    PlantOrigin origin;                                        // the stem base of the plant of the patch
    double root_m = 0.0;                                       // the root zone's radius around the stem base, metres; 0: no ring
    GroundReticleFrame reticle;                                // the brush and the bed point
    GroundCrumbsFrame crumbs;                                  // the loosened soil point by point, in every mode
};

/** The overlay of the ground mode. */
class TerrainKit {
public:
    /** Binds measures, colours, the word and the reticle; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The reticle, whose radius the hand reads. */
    [[nodiscard]] const GroundReticle& reticle() const;

    static constexpr const char* LAYER_GROUND = "ground_rows";             // the rows in every mode but terrain
    static constexpr const char* LAYER_GROUND_TERRAIN = "ground_terrain";  // the rows and columns of the mode terrain

    /** The rows of the ground outside the ground mode, in OVERLAY_SHADOW; false with the reason in `failed`. */
    bool draw_rows(UiPen& canvas, const ::godot::Transform2D& frame, const TerrainKitFrame& now,
                   std::string& failed);

    /** The lines of the mesh in the ground mode: the rows in DOMAIN_GROUND and every column_every-th column. */
    bool draw_mesh(UiPen& canvas, const ::godot::Transform2D& frame, const TerrainKitFrame& now,
                   std::string& failed);

    /** Draws the overlay of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const TerrainKitFrame& now,
              std::string& failed);

private:
    /** Lays the lattice points of this frame; false where no point lies on the screen. */
    bool lay_lattice(const UiAnchor& anchor, const UiGround& ground);

    /** Every row, fading with its depth from `alpha` up by `alpha_depth`, in `tone` at `width`. */
    void draw_lattice_rows(UiPen& canvas, const ::godot::Color& tone, double alpha, double alpha_depth,
                           double width) const;

    /** One line through the lattice points `from` + k * `step`; broken where a point is missing. */
    void draw_line(UiPen& canvas, int64_t column, int64_t row, int64_t step_column, int64_t step_row,
                   const ::godot::Color& color, double width) const;

    /** The arrows along `stream`; false for a bed without length. */
    bool draw_flow(UiPen& canvas, const UiAnchor& anchor, const UiGround& ground,
                   const VivariumStreamView& stream, double elapsed, std::string& failed) const;

    /** The ring of the root zone around the stem base, none while root_m is not above 0; true, `failed` unused. */
    bool draw_roots(UiPen& canvas, const TerrainKitFrame& now, std::string& failed);

    double grid_alpha_ = 0.0, grid_alpha_depth_ = 0.0, grid_w_ = 0.0;            // hud_terrain_grid_*
    double column_every_ = 0.0, column_alpha_ = 0.0, column_w_ = 0.0;            // hud_terrain_column_*
    double root_alpha_ = 0.0, root_dash_ = 0.0, root_gap_ = 0.0, root_steps_ = 0.0;  // hud_terrain_root_*
    double line_w_ = 0.0;                                                        // hud_terrain_line_w
    double flow_count_ = 0.0, flow_tail_ = 0.0;                                  // hud_terrain_flow_*
    double flow_tip_ = 0.0, flow_back_ = 0.0, flow_half_ = 0.0;
    double line_alpha_ = 0.0, line_alpha_depth_ = 0.0, line_ground_w_ = 0.0;     // hud_ground_line_*
    double ground_columns_ = 0.0, ground_rows_ = 0.0;                            // hud_ground_columns, hud_ground_rows
    ::godot::Color ground_, flow_, alert_, shadow_;
    GroundCrumbs crumbs_;
    GroundReticle reticle_;

    std::vector<::godot::Vector2> points_;  // per lattice point (row * columns + column) its point this frame
    std::vector<uint8_t> seen_;             // per lattice point: 1 where it has a point this frame
    int64_t columns_ = 0;                   // lattice points across the square (the mockup's NX)
    int64_t rows_ = 0;                      // lattice points along the square (the mockup's NZ)
    int64_t side_ = 0;                      // samples per side of the port's lattice this frame; under 2 no height
    double top_ = 0.0, span_ = 0.0;         // the highest row point on the screen and the span down to the lowest
};

}  // namespace ase::adp::godot

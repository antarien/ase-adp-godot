#pragma once

/**
 * ASE GODOT ADAPTER - HUD GROUND RETICLE
 *
 * @file        godot_hud_ground_reticle.hpp
 * @brief       The ring of the brush on the ground in the breadth of the tool, and the first point of a bed
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, building block GroundReticle of TerrainKit, as the
 *              mockup's drawTerrainOverlay draws it under a ground tool (the ground tool kit, usecase:100389).
 *
 *              THE VERB DECIDES THE RING. The verb in hand names its op (hud_verb_<v>_op). A brush verb
 *              (lower, raise, loosen, flatten) works under a ring of hud_verb_<v>_brush_m metres around the
 *              brush; a point verb (bed, measure) shows the small ring of hud_terrain_point_m; the level verb
 *              shows no brush but the patch it levels, the disc of the patch the start configuration laid. The
 *              brush ring fills in DOMAIN_GROUND at hud_terrain_brush_alpha, the patch ring at
 *              hud_terrain_patch_alpha, both stroked in DOMAIN_GROUND at hud_terrain_line_w, each of
 *              hud_terrain_brush_steps or hud_terrain_root_steps points on the ground (UiGround::ring).
 *
 *              THE BED IN TWO TAPS. While the first point of a bed is set, a dot of hud_terrain_bed_dot in
 *              DOMAIN_WATER marks it, and with the brush on the ground a line dashed hud_terrain_bed_dash on,
 *              hud_terrain_bed_gap off runs from it to the brush - where the second tap would end the bed.
 *
 *              radius gives the breadth of a brush verb to the hand, whose reading says it in metres
 *              (HandMode, verb_reading.radius): one value, read here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_ground.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>

namespace ase::adp::godot {

/** Where the brush and the bed stand this frame. */
struct GroundReticleFrame {
    const UiAnchor* anchor = nullptr;  // the projection of this frame
    const UiGround* ground = nullptr;  // the ground the rings lie on
    std::string verb;                  // the verb in hand (hud.verbs key)
    bool brush_on = false;             // the brush lies on the ground
    double brush_x = 0.0;              // the brush, world metres east
    double brush_z = 0.0;              // the brush, world metres north
    bool bed_on = false;               // the first point of a bed is set
    double bed_x = 0.0;                // that point, world metres east
    double bed_z = 0.0;                // that point, world metres north
    double patch_x = 0.0;              // the patch the level verb levels, centre east
    double patch_z = 0.0;              // its centre north
    double patch_m = 0.0;              // the radius of its levelled disc, metres
};

/** The brush ring and the bed point of the ground mode. */
class GroundReticle {
public:
    /** Binds measures and colours; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The op of `verb` (hud_verb_<v>_op); false with the missing token in `failed`. */
    bool op(const std::string& verb, std::string& out, std::string& failed) const;

    /** The radius `verb` works under, metres: its brush, the point ring or 0 for the level verb. */
    bool radius(const std::string& verb, double& out, std::string& failed) const;

    /** Draws the reticle of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const GroundReticleFrame& now,
              std::string& failed) const;

private:
    /** Fills `points` in DOMAIN_GROUND at `alpha` and strokes them in DOMAIN_GROUND. */
    void disc(UiPen& canvas, const ::godot::PackedVector2Array& points, double alpha) const;

    const UiTheme* theme_ = nullptr;  // the theme the verb tokens are read from
    double line_w_ = 0.0;             // hud_terrain_line_w
    double bed_dot_ = 0.0;            // hud_terrain_bed_dot
    double bed_dash_ = 0.0;           // hud_terrain_bed_dash
    double bed_gap_ = 0.0;            // hud_terrain_bed_gap
    double point_m_ = 0.0;            // hud_terrain_point_m
    double brush_steps_ = 0.0;        // hud_terrain_brush_steps
    double patch_steps_ = 0.0;        // hud_terrain_root_steps
    double brush_alpha_ = 0.0;        // hud_terrain_brush_alpha
    double patch_alpha_ = 0.0;        // hud_terrain_patch_alpha
    ::godot::Color ground_;           // DOMAIN_GROUND
    ::godot::Color water_;            // DOMAIN_WATER
};

}  // namespace ase::adp::godot

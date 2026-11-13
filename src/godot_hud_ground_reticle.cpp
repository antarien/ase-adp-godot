/**
 * ASE GODOT ADAPTER - HUD GROUND RETICLE
 *
 * @file        godot_hud_ground_reticle.cpp
 * @brief       The ring of the brush on the ground in the breadth of the tool, and the first point of a bed
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_ground_reticle.hpp.
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

#include <ase/adp/godot/godot_hud_ground_reticle.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool GroundReticle::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_terrain_line_w", &line_w_},
             {"hud_terrain_bed_dot", &bed_dot_},
             {"hud_terrain_bed_dash", &bed_dash_},
             {"hud_terrain_bed_gap", &bed_gap_},
             {"hud_terrain_point_m", &point_m_},
             {"hud_terrain_brush_steps", &brush_steps_},
             {"hud_terrain_root_steps", &patch_steps_},
             {"hud_terrain_brush_alpha", &brush_alpha_},
             {"hud_terrain_patch_alpha", &patch_alpha_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.tone("ground", ground_)) {
        failed = "DOMAIN_GROUND";
        return false;
    }
    if (!theme.color("DOMAIN_WATER", water_)) {
        failed = "DOMAIN_WATER";
        return false;
    }
    return true;
}

bool GroundReticle::op(const std::string& verb, std::string& out, std::string& failed) const {
    const std::string token = GODOT_HUD_VERB_TOKEN + verb + GODOT_HUD_VERB_OP_TAIL;
    ::godot::String text;
    if (theme_ == nullptr || !theme_->token_text(token, text)) {
        failed = token;
        return false;
    }
    out = text.utf8().get_data();
    return true;
}

bool GroundReticle::radius(const std::string& verb, double& out, std::string& failed) const {
    std::string code;
    if (!op(verb, code, failed)) {
        return false;
    }
    if (code == GODOT_HUD_VERB_OP_LEVEL) {
        out = 0.0;
        return true;
    }
    if (code == GODOT_HUD_VERB_OP_BED || code == GODOT_HUD_VERB_OP_MEASURE) {
        out = point_m_;
        return true;
    }
    const std::string token = GODOT_HUD_VERB_TOKEN + verb + GODOT_HUD_VERB_BRUSH_TAIL;
    if (!theme_->measure(token, out)) {
        failed = token;
        return false;
    }
    return true;
}

void GroundReticle::disc(UiPen& canvas, const ::godot::PackedVector2Array& points, double alpha) const {
    ::godot::Color fill = ground_;
    fill.a = static_cast<float>(alpha);
    canvas.draw_colored_polygon(points, fill);
    canvas.draw_polyline(points, ground_, static_cast<float>(line_w_), true);
}

bool GroundReticle::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                         const GroundReticleFrame& now, std::string& failed) const {
    if (now.anchor == nullptr || now.ground == nullptr) {
        failed = "GroundReticle: the frame carries no anchor or no ground";
        return false;
    }
    std::string code;
    if (!op(now.verb, code, failed)) {
        return false;
    }
    canvas.draw_set_transform_matrix(frame);

    // The first point of a bed, and the run to where the second tap would end it.
    if (now.bed_on) {
        ::godot::Vector2 start;
        if (now.ground->surface(*now.anchor, now.bed_x, now.bed_z, 0.0, start)) {
            canvas.draw_circle(start, static_cast<float>(bed_dot_), water_);
            ::godot::Vector2 brush;
            if (now.brush_on && now.ground->surface(*now.anchor, now.brush_x, now.brush_z, 0.0, brush)) {
                ::godot::PackedVector2Array run;
                run.push_back(start);
                run.push_back(brush);
                svg_dash_points(canvas, run, water_, line_w_, bed_dash_, bed_gap_);
            }
        }
    }

    ::godot::PackedVector2Array points;
    if (code == GODOT_HUD_VERB_OP_LEVEL) {
        // The level verb has no brush: it shows the patch it levels.
        if (now.ground->ring(*now.anchor, now.patch_x, now.patch_z, now.patch_m, static_cast<int64_t>(patch_steps_),
                             points)) {
            disc(canvas, points, patch_alpha_);
        }
    } else if (now.brush_on) {
        double reach = 0.0;
        if (!radius(now.verb, reach, failed)) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        if (now.ground->ring(*now.anchor, now.brush_x, now.brush_z, reach, static_cast<int64_t>(brush_steps_),
                             points)) {
            disc(canvas, points, brush_alpha_);
        }
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

}  // namespace ase::adp::godot

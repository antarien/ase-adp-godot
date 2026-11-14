/**
 * ASE GODOT ADAPTER - HUD FIGURE SWITCH
 *
 * @file        godot_hud_figure_switch.cpp
 * @brief       FigureSwitch - cam.mode under the debug switch, lit in the player camera, dimmed in the fixed view
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.9 (VIVARIUM_PHASE_07_FIGURE_V1, cam.mode);
 *              godot_hud_figure_switch.hpp names every measure, colour and word.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_figure_switch.hpp>
#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool FigureSwitch::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_figure_switch_x", &x_},
             {"hud_figure_switch_y", &y_},
             {"hud_figure_switch_radius", &radius_},
             {"hud_figure_switch_line", &line_},
             {"hud_figure_switch_glyph_size", &glyph_size_},
             {"hud_figure_switch_hit", &hit_},
             {"hud_figure_switch_off_alpha", &off_alpha_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"FIGURE_SWITCH_GROUND", &ground_}, {"DOMAIN_FLOW", &flow_}, {"OVERLAY_INK", &ink_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return bind_word_icon(theme, "figure.cam_mode.glyph", glyph_, failed);
}

void FigureSwitch::draw(UiPen& canvas, const ::godot::Transform2D& frame, bool player_camera) const {
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));
    if (player_camera) {
        // lit: the hexagon of the switch in the flow's ground and edge, as the debug switch while its layer is on
        canvas.draw_set_transform_matrix(frame);
        ::godot::PackedVector2Array corners = hud_hex_points(centre, radius_);
        canvas.draw_colored_polygon(corners, ground_);
        corners.push_back(corners[0]);
        canvas.draw_polyline(corners, flow_, static_cast<float>(line_), true);
    }
    glyph_.draw(canvas, frame, centre, glyph_size_, player_camera ? flow_ : ink_, player_camera ? 1.0 : off_alpha_);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
}

bool FigureSwitch::hit(const ::godot::Vector2& point) const {
    return ase::math::abs(static_cast<double>(point.x) - x_) <= hit_ &&
           ase::math::abs(static_cast<double>(point.y) - y_) <= hit_;
}

}  // namespace ase::adp::godot

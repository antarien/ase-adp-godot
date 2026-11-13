/**
 * ASE GODOT ADAPTER - HUD BAG WINDOW LOOK
 *
 * @file        godot_hud_bag_window_look.cpp
 * @brief       The measures, the frame limits and the colours of the pack window and the pouch window, read from SHA
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_bag_window_look.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_bag_window_look.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** Reads every token of `slots`; false with the first missing name in `failed`. */
bool read_tokens(const UiTheme& theme, std::initializer_list<std::pair<const char*, double*>> slots,
                 std::string& failed) {
    for (const std::pair<const char*, double*>& slot : slots) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return true;
}

/** Reads every colour of `slots`; false with the first unknown name in `failed`. */
bool read_colors(const UiTheme& theme, std::initializer_list<std::pair<const char*, ::godot::Color*>> slots,
                 std::string& failed) {
    for (const std::pair<const char*, ::godot::Color*>& slot : slots) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return true;
}

bool read_pack(const UiTheme& theme, BagWindowPackSize& p, std::string& failed) {
    return read_tokens(theme,
                       {{"hud_bag_x", &p.x}, {"hud_bag_y", &p.y}, {"hud_bag_w", &p.w}, {"hud_bag_h", &p.h},
                        {"hud_bag_min_y", &p.min_y}, {"hud_bag_flap_inset", &p.flap_inset},
                        {"hud_bag_flap_rise", &p.flap_rise}, {"hud_bag_flap_dy", &p.flap_dy},
                        {"hud_bag_flap_line", &p.flap_line}, {"hud_bag_seam_inset", &p.seam_inset},
                        {"hud_bag_seam_rise", &p.seam_rise}, {"hud_bag_seam_dy", &p.seam_dy},
                        {"hud_bag_seam_line", &p.seam_line}, {"hud_bag_stitch_dash", &p.stitch_dash},
                        {"hud_bag_stitch_gap", &p.stitch_gap}, {"hud_bag_title_dy", &p.title_dy},
                        {"hud_bag_title_size", &p.title_size}, {"hud_bag_corner", &p.corner},
                        {"hud_bag_corner_low", &p.corner_low}, {"hud_bag_corner_low_reach", &p.corner_low_reach},
                        {"hud_bag_line", &p.line}, {"hud_bag_hot_line", &p.hot_line},
                        {"hud_bag_field_dx", &p.field_dx}, {"hud_bag_field_dy", &p.field_dy},
                        {"hud_bag_field_shrink_w", &p.field_shrink_w}, {"hud_bag_field_shrink_h", &p.field_shrink_h},
                        {"hud_bag_field_line", &p.field_line}, {"hud_bag_close_dx", &p.close_dx},
                        {"hud_bag_close_dy", &p.close_dy}, {"hud_bag_close_size", &p.close_size},
                        {"hud_bag_close_hit_dx", &p.close_hit_dx}, {"hud_bag_close_hit_dy", &p.close_hit_dy},
                        {"hud_bag_close_hit_w", &p.close_hit_w}, {"hud_bag_close_hit_h", &p.close_hit_h},
                        {"hud_bag_content_dx", &p.content_dx}, {"hud_bag_content_dy", &p.content_dy},
                        {"hud_bag_load_dx", &p.load_dx}, {"hud_bag_load_dy", &p.load_dy},
                        {"hud_bag_load_size", &p.load_size}, {"hud_bag_mouth_dx", &p.mouth_dx},
                        {"hud_bag_mouth_size", &p.mouth_size}, {"hud_bag_drop_rise", &p.drop_rise}},
                       failed);
}

bool read_pouch(const UiTheme& theme, BagWindowPouchSize& q, std::string& failed) {
    return read_tokens(theme,
                       {{"hud_pouch_x", &q.x}, {"hud_pouch_y", &q.y}, {"hud_pouch_w", &q.w}, {"hud_pouch_h", &q.h},
                        {"hud_pouch_min_y", &q.min_y}, {"hud_pouch_neck_inset", &q.neck_inset},
                        {"hud_pouch_neck_dy", &q.neck_dy}, {"hud_pouch_bulge", &q.bulge},
                        {"hud_pouch_foot_inset", &q.foot_inset}, {"hud_pouch_foot_sag", &q.foot_sag},
                        {"hud_pouch_foot_rise", &q.foot_rise}, {"hud_pouch_line", &q.line},
                        {"hud_pouch_hot_line", &q.hot_line}, {"hud_pouch_fold_dx", &q.fold_dx},
                        {"hud_pouch_fold_dy", &q.fold_dy}, {"hud_pouch_fold_bend_dx", &q.fold_bend_dx},
                        {"hud_pouch_fold_end_dx", &q.fold_end_dx}, {"hud_pouch_fold_end_rise", &q.fold_end_rise},
                        {"hud_pouch_fold_line", &q.fold_line}, {"hud_pouch_string_line", &q.string_line},
                        {"hud_pouch_string_dip", &q.string_dip}, {"hud_pouch_knot_gap", &q.knot_gap},
                        {"hud_pouch_knot_dy", &q.knot_dy}, {"hud_pouch_knot_bend_x", &q.knot_bend_x},
                        {"hud_pouch_knot_bend_y", &q.knot_bend_y}, {"hud_pouch_knot_reach_x", &q.knot_reach_x},
                        {"hud_pouch_knot_reach_y", &q.knot_reach_y}, {"hud_pouch_knot_line", &q.knot_line},
                        {"hud_pouch_grip_dx", &q.grip_dx}, {"hud_pouch_grip_dy", &q.grip_dy},
                        {"hud_pouch_grip_shrink_w", &q.grip_shrink_w}, {"hud_pouch_grip_h", &q.grip_h},
                        {"hud_pouch_title_dx", &q.title_dx}, {"hud_pouch_title_dy", &q.title_dy},
                        {"hud_pouch_title_size", &q.title_size}, {"hud_pouch_close_dx", &q.close_dx},
                        {"hud_pouch_close_size", &q.close_size}, {"hud_pouch_close_hit_dx", &q.close_hit_dx},
                        {"hud_pouch_close_hit_dy", &q.close_hit_dy}, {"hud_pouch_close_hit_w", &q.close_hit_w},
                        {"hud_pouch_close_hit_h", &q.close_hit_h}, {"hud_pouch_content_dx", &q.content_dx},
                        {"hud_pouch_content_dy", &q.content_dy}, {"hud_pouch_load_dy", &q.load_dy},
                        {"hud_pouch_load_size", &q.load_size}, {"hud_pouch_drop_rise", &q.drop_rise},
                        {"hud_pouch_drop_sink", &q.drop_sink}},
                       failed);
}

bool read_inks(const UiTheme& theme, BagWindowInks& k, std::string& failed) {
    return read_colors(theme,
                       {{"LEATHER_BAG_FLAP", &k.flap}, {"LEATHER_BAG_EDGE", &k.edge}, {"LEATHER_BAG_SEAM", &k.seam},
                        {"LEATHER_BAG_BODY", &k.body}, {"LEATHER_BAG_INNER", &k.inner},
                        {"LEATHER_BAG_INNER_SEAM", &k.inner_seam}, {"LEATHER_BAG_TITLE", &k.title},
                        {"LEATHER_BAG_LOAD", &k.load}, {"LEATHER_BAG_SPEC", &k.spec},
                        {"LINEN_POUCH_BODY", &k.pouch_body}, {"LINEN_POUCH_EDGE", &k.pouch_edge},
                        {"LINEN_POUCH_FOLD", &k.pouch_fold}, {"LINEN_POUCH_STRING", &k.pouch_string},
                        {"LINEN_POUCH_TITLE", &k.pouch_title}, {"LINEN_POUCH_INK", &k.pouch_ink},
                        {"OVERLAY_INK_SOFT", &k.pouch_load}, {"DOMAIN_LIFE", &k.life}, {"DOMAIN_ALERT", &k.alert}},
                       failed);
}

}  // namespace

bool read_bag_window_look(const UiTheme& theme, BagWindowLook& look, std::string& failed) {
    return read_pack(theme, look.pack, failed) && read_pouch(theme, look.pouch, failed) &&
           read_tokens(theme,
                       {{"hud_frame_width", &look.frame_w}, {"hud_window_margin_x", &look.margin_x},
                        {"hud_window_max_bottom", &look.max_bottom}, {"hud_load_decimals", &look.decimals},
                        {"hud_load_fine_below", &look.fine_below}, {"hud_load_fine_decimals", &look.fine_decimals}},
                       failed) &&
           read_inks(theme, look.inks, failed);
}

}  // namespace ase::adp::godot

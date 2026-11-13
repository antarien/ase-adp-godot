#pragma once

/**
 * ASE GODOT ADAPTER - HUD BAG WINDOW LOOK
 *
 * @file        godot_hud_bag_window_look.hpp
 * @brief       The measures, the frame limits and the colours of the pack window and the pouch window, read from SHA
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the look of block BagWindow (godot_hud_bag_window.hpp).
 *
 *              EVERY NUMBER IS A TOKEN OF THEME.CSS. The pack's measures are the hud_bag_* tokens, the pouch's the
 *              hud_pouch_* tokens, the frame the windows are dragged inside hud_frame_width, hud_window_margin_x
 *              and hud_window_max_bottom, the printing of a weight or a volume hud_load_decimals,
 *              hud_load_fine_below and hud_load_fine_decimals (the mockup's f1 and kgf). The colours are the
 *              LEATHER_BAG_* and LINEN_POUCH_* names of colors.json, OVERLAY_INK_SOFT for the pouch's load, and
 *              DOMAIN_LIFE / DOMAIN_ALERT for the hot edge.
 *
 *              read_bag_window_look fills the whole look in one pass and refuses at the first name the theme
 *              does not carry, naming it in `failed`: the window is never drawn with a measure it did not read.
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

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <godot_cpp/variant/color.hpp>

#include <string>

namespace ase::adp::godot {

/** The drawing measures of the pack (hud_bag_*). */
struct BagWindowPackSize {
    double x = 0.0, y = 0.0, w = 0.0, h = 0.0, min_y = 0.0;
    double flap_inset = 0.0, flap_rise = 0.0, flap_dy = 0.0, flap_line = 0.0;
    double seam_inset = 0.0, seam_rise = 0.0, seam_dy = 0.0, seam_line = 0.0, stitch_dash = 0.0, stitch_gap = 0.0;
    double title_dy = 0.0, title_size = 0.0;
    double corner = 0.0, corner_low = 0.0, corner_low_reach = 0.0, line = 0.0, hot_line = 0.0;
    double field_dx = 0.0, field_dy = 0.0, field_shrink_w = 0.0, field_shrink_h = 0.0, field_line = 0.0;
    double close_dx = 0.0, close_dy = 0.0, close_size = 0.0;
    double close_hit_dx = 0.0, close_hit_dy = 0.0, close_hit_w = 0.0, close_hit_h = 0.0;
    double content_dx = 0.0, content_dy = 0.0;
    double load_dx = 0.0, load_dy = 0.0, load_size = 0.0, mouth_dx = 0.0, mouth_size = 0.0, drop_rise = 0.0;
};

/** The drawing measures of the pouch (hud_pouch_*). */
struct BagWindowPouchSize {
    double x = 0.0, y = 0.0, w = 0.0, h = 0.0, min_y = 0.0;
    double neck_inset = 0.0, neck_dy = 0.0, bulge = 0.0, foot_inset = 0.0, foot_sag = 0.0, foot_rise = 0.0;
    double line = 0.0, hot_line = 0.0;
    double fold_dx = 0.0, fold_dy = 0.0, fold_bend_dx = 0.0, fold_end_dx = 0.0, fold_end_rise = 0.0;
    double fold_line = 0.0;
    double string_line = 0.0, string_dip = 0.0;
    double knot_gap = 0.0, knot_dy = 0.0, knot_bend_x = 0.0, knot_bend_y = 0.0, knot_reach_x = 0.0;
    double knot_reach_y = 0.0, knot_line = 0.0;
    double grip_dx = 0.0, grip_dy = 0.0, grip_shrink_w = 0.0, grip_h = 0.0;
    double title_dx = 0.0, title_dy = 0.0, title_size = 0.0;
    double close_dx = 0.0, close_size = 0.0;
    double close_hit_dx = 0.0, close_hit_dy = 0.0, close_hit_w = 0.0, close_hit_h = 0.0;
    double content_dx = 0.0, content_dy = 0.0, load_dy = 0.0, load_size = 0.0, drop_rise = 0.0, drop_sink = 0.0;
};

/** The colours of both windows. */
struct BagWindowInks {
    ::godot::Color flap, edge, seam, body, inner, inner_seam, title, load, spec;      // LEATHER_BAG_*
    ::godot::Color pouch_body, pouch_edge, pouch_fold, pouch_string, pouch_title;    // LINEN_POUCH_*
    ::godot::Color pouch_ink, pouch_load;                                            // LINEN_POUCH_INK, OVERLAY_INK_SOFT
    ::godot::Color life, alert;                                                      // DOMAIN_LIFE, DOMAIN_ALERT
};

/** The whole look of the two windows. */
struct BagWindowLook {
    BagWindowPackSize pack;
    BagWindowPouchSize pouch;
    BagWindowInks inks;
    double frame_w = 0.0;        // hud_frame_width
    double margin_x = 0.0;       // hud_window_margin_x
    double max_bottom = 0.0;     // hud_window_max_bottom
    double decimals = 0.0;       // hud_load_decimals: f1
    double fine_below = 0.0;     // hud_load_fine_below: kgf prints finer below this weight
    double fine_decimals = 0.0;  // hud_load_fine_decimals
};

/** Reads every measure and colour of `look` from `theme`; false with the first missing name in `failed`. */
bool read_bag_window_look(const UiTheme& theme, BagWindowLook& look, std::string& failed);

}  // namespace ase::adp::godot

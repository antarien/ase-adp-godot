#pragma once

/**
 * ASE GODOT ADAPTER - HUD SYSTEM RING
 *
 * @file        godot_hud_system_ring.hpp
 * @brief       The system ring of the mode sys: hold it to reset the patch, with its progress arc, sentence and close
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, building block SystemRing (the mockup's sysRing()), the
 *              place the game button Reiniciar moves to (section "Debug-Ebene": Reiniciar becomes the system ring
 *              with holding). The reset itself is VivariumView::restart: the same teardown as exit_tree, then a
 *              new host with the whole set, so the ring starts the client window anew (usecase:101019).
 *
 *              THE DIM. The world dims from hud_sys_dim_y, hud_sys_dim_h high, in OVERLAY_SCRIM_SYSTEM, stretched
 *              to the surface the HUD sets in `cover` (hud_cover_dim).
 *
 *              THE RING. At hud_sys_x, hud_sys_y a disc of hud_sys_radius in DOMAIN_ALERT_HOLD with a ring in
 *              OVERLAY_INK_FAINT of hud_line; over it the arc from the top clockwise to the held share of a turn
 *              in DOMAIN_ALERT of hud_sys_arc_width, and the glyph texts.hud.reset_glyph in DOMAIN_ALERT of
 *              hud_sys_glyph_size. The held share is the seconds held against hud_gesture_system_hold_ms, at most
 *              one; full() says when it is reached, and the caller then runs the reset. The tap target is a
 *              circle of hud_sys_hit_radius.
 *
 *              THE WORDS. Middle-anchored at hud_sys_x: texts.hud.reset_title (class tb, hud_sys_title_size,
 *              DOMAIN_ALERT) at hud_sys_title_y, texts.hud.reset_line (class t, hud_sys_sentence_size,
 *              OVERLAY_INK_SOFT) at hud_sys_sentence_y, texts.hud.reset_hold (class t, hud_sys_hint_size,
 *              OVERLAY_INK_FAINT) at hud_sys_hint_y.
 *
 *              THE CLOSE. The glyph texts.hud.window_close at hud_sheet_close_x, hud_sheet_close_y of
 *              hud_sheet_close_size in OVERLAY_INK_SOFT, the same place as on the calendar sheet; its tap
 *              target hud_sheet_close_hit_* rests.
 *
 *              No backend gap: the reset runs on the adapter's own host.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @category    ecs/module
 * @usecase     usecase:101019 - start the client window
 * @actor       Vivarium Steward (VIVARIUSTE)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>

namespace ase::adp::godot {

/** What the system ring shows this frame. */
struct SystemRingFrame {
    double held = 0.0;      // seconds the ring has been held without letting go; 0 while it is not held
    double elapsed = 0.0;   // seconds since the mode sys opened
    bool still = false;     // every motion held
    HudCover cover;         // the surface the dim reaches, set by the HUD
};

/** What a tap on the system ring hit. */
struct SystemRingHit {
    bool hold = false;      // the ring: holding starts here
    bool rest = false;      // the close: back to rest
};

/** The system ring. */
class SystemRing {
public:
    /** Binds measures, colours, words, the glyphs and the ring; false with the name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the dim, the ring with its arc and glyph, the words and the close; false with the reason. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SystemRingFrame& now,
              std::string& failed);

    /** What `point` (in mockup units) hits, in `out`; false where it hits neither the ring nor the close. */
    bool hit(const ::godot::Vector2& point, SystemRingHit& out) const;

    /** The square around the ring's hold target (mockup units): its middle is the ring's centre, and it holds
     *  the disc of hud_sys_hit_radius hit() asks. */
    ::godot::Rect2 hold_place() const;

    /** The close target (mockup units), the rect hit() answers `rest` on: the mockup's CLOSE, the one way out of
     *  the mode sys, since a short tap on the ring is its hold and leaves the mode standing. */
    ::godot::Rect2 close_place() const;

    /** The share of a turn `held` seconds fill, at most one. */
    double share(double held) const;

    /** True once `held` seconds reach hud_gesture_system_hold_ms: the caller resets. */
    bool full(double held) const;

private:
    double dim_y_ = 0.0;            // hud_sys_dim_y
    double dim_h_ = 0.0;            // hud_sys_dim_h
    double x_ = 0.0;                // hud_sys_x
    double y_ = 0.0;                // hud_sys_y
    double radius_ = 0.0;           // hud_sys_radius
    double arc_width_ = 0.0;        // hud_sys_arc_width
    double glyph_size_ = 0.0;       // hud_sys_glyph_size
    double hit_radius_ = 0.0;       // hud_sys_hit_radius
    double title_y_ = 0.0;          // hud_sys_title_y
    double title_size_ = 0.0;       // hud_sys_title_size
    double sentence_y_ = 0.0;       // hud_sys_sentence_y
    double sentence_size_ = 0.0;    // hud_sys_sentence_size
    double hint_y_ = 0.0;           // hud_sys_hint_y
    double hint_size_ = 0.0;        // hud_sys_hint_size
    double line_ = 0.0;             // hud_line
    double hold_ms_ = 0.0;          // hud_gesture_system_hold_ms
    double close_x_ = 0.0;          // hud_sheet_close_x
    double close_y_ = 0.0;          // hud_sheet_close_y
    double close_size_ = 0.0;       // hud_sheet_close_size
    ::godot::Rect2 close_hit_;      // hud_sheet_close_hit_x, _y, _w, _h

    ::godot::Color scrim_;          // OVERLAY_SCRIM_SYSTEM
    ::godot::Color hold_;           // DOMAIN_ALERT_HOLD
    ::godot::Color alert_;          // DOMAIN_ALERT
    ::godot::Color soft_;           // OVERLAY_INK_SOFT
    ::godot::Color faint_;          // OVERLAY_INK_FAINT

    UiTextBlock title_;             // texts.hud.reset_title, class tb
    UiTextBlock sentence_;          // texts.hud.reset_line, class t
    UiTextBlock hint_;              // texts.hud.reset_hold, class t
    UiIcon glyph_;                  // texts.hud.reset_glyph
    UiIcon close_;                  // texts.hud.window_close
    UiRing ring_;
};

}  // namespace ase::adp::godot

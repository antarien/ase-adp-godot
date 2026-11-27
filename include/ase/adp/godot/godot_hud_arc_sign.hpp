#pragma once

/**
 * ASE GODOT ADAPTER - HUD ARC SIGN
 *
 * @file        godot_hud_arc_sign.hpp
 * @brief       ArcSign: one occurrence on a ring, a sign on a dark disc, a node or a dot, its word tangent to the ring
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, elements A05 to A07, A09 to A10 and A12 to A13 of
 *              the mockup's ringLay (REG, glyphAt, tan). One class for every kind of the register: what differs is data
 *              of SHA, never a branch on the kind. A kind is its name (GODOT_ARC_KIND_*); its mark
 *              (hud_arc_kind_<kind>_mark: hud_arc_mark_node, _dot, _glyph, _turn or _start), its tone
 *              (hud_arc_kind_<kind>_domain: a DOMAIN_* by its code, hud_arc_domain_ink for OVERLAY_INK,
 *              hud_arc_domain_soft for OVERLAY_INK_SOFT, hud_arc_domain_season for the SEASON_* of its body) and the key of its word (hud_arc_kind_<kind>_word: the word of that key with the body appended
 *              where one exists, else the key alone) are tokens, the symbol of a glyph the word arc.glyph.<kind>.
 *              An eclipse whose body is hud_arc_eclipse_lunar_body is the kind eclipse_lunar.
 *
 *              THE ANGLE IS THE OWNER'S SHARE TURNED, NOTHING MORE. A sign stands at (share - now) times
 *              hud_arc_<ring>_half_deg from the needle (arc_disc_angle); the ring hands that angle's circle in
 *              ArcSignPlace. A node is a disc of OVERLAY_CHIP_OFF at hud_arc_node_fill_alpha, rimmed in its tone at
 *              hud_arc_node_line, radius hud_arc_node_day_radius on the day ring and hud_arc_node_radius on the others;
 *              its word stands at the middle of its span inside the day ring (hud_arc_period_label_inset, size
 *              hud_arc_period_label_size, OVERLAY_INK_SOFT) and outside the others (hud_arc_node_label_outset, size
 *              hud_arc_node_label_size, its tone). A dot is a disc of its tone, radius hud_arc_dot_radius. A glyph sits
 *              on a dark disc of hud_arc_sign_disc_share of its size, rimmed hud_arc_sign_disc_line, its word outside
 *              the ring at hud_arc_sign_[day_]label_outset in hud_arc_sign_[day_]label_size at hud_arc_sign_label_alpha.
 *              A turn (the kind tumbrae, A05) is a filled disc of its tone, radius hud_arc_day_turn_radius, its word
 *              "TUM n" tangent inside the ring at hud_arc_day_turn_label_inset, hud_arc_day_turn_label_deg after the
 *              disc, in class tb at hud_arc_day_turn_label_size. A start (the kind lunbrex, A08) is a tick of its tone
 *              hud_arc_lun_start_tick_half each side of the ring at hud_arc_lun_start_tick_line, its word "LUN n"
 *              tangent inside the ring at hud_arc_lun_label_inset in hud_arc_lun_label_size.
 *              Every opacity is multiplied by UiArcFade at the sign's x place.
 *
 *              LAID ONCE PER SET. lay resolves a sign's tone, word and glyph and holds them in an ArcLaidSign; draw
 *              only moves it by the share of now. draw writes the flank line `debug arc sign kind= body= ring= x= y=`
 *              once per laid sign, with its place in logical pixels.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_frame.hpp>
#include <ase/adp/godot/godot_hud_ui_arc_fade.hpp>
#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** Where a ring stands this frame, as ArcSign::draw needs it. */
struct ArcSignPlace {
    ::godot::Vector2 centre;            // the common centre of the rings, hud_arc_cx and hud_arc_cy
    double radius = 0.0;                // the ring's radius, hud_arc_<ring>_r
    double half_deg = 0.0;              // the ring's half span, hud_arc_<ring>_half_deg
    double now = 0.0;                   // the share of now against the anchor of the ring's set
    uint32_t ring = 0;                  // GODOT_ARC_SLOT_*
    const UiArcFade* fade = nullptr;    // the fade of the arc's ends
    double elapsed = 0.0;               // seconds of the view's clock
    bool still = false;                 // every motion held
};

/** One sign laid: resolved once per set, moved every frame. */
struct ArcLaidSign {
    std::string kind;                   // the kind's name, the lunar eclipse resolved
    double body = 0.0;                  // the owner's body
    double share = 0.0;                 // the owner's place
    double span = 0.0;                  // the owner's length
    uint32_t ring = 0;                  // the ring it stands on
    bool node = false;                  // its mark is a node
    bool dot = false;                   // its mark is a dot
    bool turn = false;                  // its mark is the turn of a day: a filled disc, its word tangent inside, SemiBold
    bool start = false;                 // its mark is the start of a month: a long tick, its word tangent inside
    ::godot::Color tone;                // its colour
    bool has_word = false;              // a word stands beside it
    UiTextBlock word;                   // its word, class t
    UiIcon glyph;                       // its symbol, bound for a glyph mark
    bool announced = false;             // its flank line was written
};

/** The signs of the arc. */
class ArcSign {
public:
    /** Binds the measures, the colours and the table of the kinds; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Lays `sign` on `ring`: its mark, tone, word and symbol. False with the reason in `failed`. */
    bool lay(const ArcSignFrame& sign, uint32_t ring, ArcLaidSign& out, std::string& failed) const;

    /** The tone of a kind of `body`, for the readout; false with the missing name in `failed`. */
    bool tone_of(const std::string& kind, double body, ::godot::Color& out, std::string& failed) const;

    /** The word of a kind of `body`, for the readout; false with the missing key in `failed`. */
    bool word_of(const std::string& kind, double body, std::vector<HudRun>& out, std::string& failed) const;

    /** Draws `sign` at its angle under `frame`, and writes its flank line once into `flank`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const ArcSignPlace& place, ArcLaidSign& sign,
              std::vector<std::string>& flank) const;

private:
    /** One kind of the register as SHA describes it. */
    struct Look {
        std::string name;        // GODOT_ARC_KIND_*
        double mark = 0.0;       // hud_arc_kind_<kind>_mark: one of hud_arc_mark_*
        std::string domain;      // hud_arc_kind_<kind>_domain
        std::string word;        // hud_arc_kind_<kind>_word
        UiIcon glyph;            // arc.glyph.<kind>, bound for a glyph mark
    };

    const Look* look(const std::string& name) const;
    std::string name_of(const ArcSignFrame& sign) const;
    std::string name_with(const std::string& kind, double body) const;
    ::godot::Vector2 below(const ::godot::Vector2& middle, double size) const;
    void draw_mark(UiPen& canvas, const ::godot::Transform2D& frame, const ArcSignPlace& place,
                   const ArcLaidSign& sign, const ::godot::Vector2& at, double opacity) const;
    void draw_word(UiPen& canvas, const ::godot::Transform2D& frame, const ArcSignPlace& place, ArcLaidSign& sign,
                   double degrees, double opacity) const;

    const UiTheme* theme_ = nullptr;
    std::vector<Look> looks_;       // every kind of the register, the lunar eclipse beside the solar one
    double mark_node_ = 0.0;        // hud_arc_mark_node
    double mark_dot_ = 0.0;         // hud_arc_mark_dot
    double mark_glyph_ = 0.0;       // hud_arc_mark_glyph
    double mark_turn_ = 0.0;        // hud_arc_mark_turn
    double mark_start_ = 0.0;       // hud_arc_mark_start
    double lunar_body_ = 0.0;       // hud_arc_eclipse_lunar_body: the body an eclipse of the moon carries
    std::string domain_ink_;        // hud_arc_domain_ink
    std::string domain_season_;     // hud_arc_domain_season
    std::string domain_soft_;       // hud_arc_domain_soft
    double node_day_r_ = 0.0;       // hud_arc_node_day_radius
    double node_r_ = 0.0;           // hud_arc_node_radius
    double node_fill_alpha_ = 0.0;  // hud_arc_node_fill_alpha
    double node_line_ = 0.0;        // hud_arc_node_line
    double node_outset_ = 0.0;      // hud_arc_node_label_outset
    double node_size_ = 0.0;        // hud_arc_node_label_size
    double period_inset_ = 0.0;     // hud_arc_period_label_inset
    double period_size_ = 0.0;      // hud_arc_period_label_size
    double dot_r_ = 0.0;            // hud_arc_dot_radius
    double turn_r_ = 0.0;           // hud_arc_day_turn_radius
    double turn_inset_ = 0.0;       // hud_arc_day_turn_label_inset
    double turn_deg_ = 0.0;         // hud_arc_day_turn_label_deg
    double turn_size_ = 0.0;        // hud_arc_day_turn_label_size
    double start_half_ = 0.0;       // hud_arc_lun_start_tick_half
    double start_line_ = 0.0;       // hud_arc_lun_start_tick_line
    double start_inset_ = 0.0;      // hud_arc_lun_label_inset
    double start_size_ = 0.0;       // hud_arc_lun_label_size
    double sign_day_size_ = 0.0;    // hud_arc_sign_day_size
    double sign_size_ = 0.0;        // hud_arc_sign_size
    double disc_share_ = 0.0;       // hud_arc_sign_disc_share
    double disc_line_ = 0.0;        // hud_arc_sign_disc_line
    double day_outset_ = 0.0;       // hud_arc_sign_day_label_outset
    double outset_ = 0.0;           // hud_arc_sign_label_outset
    double day_label_ = 0.0;        // hud_arc_sign_day_label_size
    double label_ = 0.0;            // hud_arc_sign_label_size
    double label_alpha_ = 0.0;      // hud_arc_sign_label_alpha
    double central_ = 0.0;          // hud_arc_label_central: how far under its middle a word's baseline lies, a share of its size
    ::godot::Color ink_, soft_, chip_;       // OVERLAY_INK, OVERLAY_INK_SOFT, OVERLAY_CHIP_OFF
    std::vector<::godot::Color> seasons_;    // SEASON_* in the order of the codes the calendar numbers them with
};

}  // namespace ase::adp::godot

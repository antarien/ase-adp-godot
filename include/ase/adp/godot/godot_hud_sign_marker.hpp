#pragma once

/**
 * ASE GODOT ADAPTER - HUD SIGN MARKER
 *
 * @file        godot_hud_sign_marker.hpp
 * @brief       The glyph of each sign at its place in the world; the open one hands its reading to SignReading
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block SignMarker (the mockup's markers()). The
 *              reading of the open sign (the mockup's signOpen()) is SignReading, godot_hud_sign_reading.hpp.
 *
 *              THE PLACE. Each sign of hud.signs stands where its SHA tokens put it:
 *                hud_sign_<id>_at crown: the mockup's crownPt(x, y) with H the tree's height but no lower than
 *                  hud_sign_crown_min_h, at (trunk.x + x * H * hud_sign_crown_spread, trunk.y - H * y);
 *                trunk: (trunk.x + x, trunk.y + y);
 *                brook: the point at the share x along the brook's line, y above it;
 *                screen: (x, y) of the mockup's screen.
 *              A sign whose anchor the frame does not place this frame is not drawn and takes no tap.
 *
 *              THE MARKER. The sign i fades in with hud_anim_fade after i * hud_sign_stagger. A sign that alerts,
 *              or every sign at perception levels[0], pulses at hud_sign_pulse_radius in its tone at hud_line
 *              while it is not open. A disc of hud_sign_radius in OVERLAY_DISC carries a ring in its tone,
 *              hud_sign_on_line wide when open and hud_sign_line otherwise, and its glyph texts.hud.signs.<id>
 *              .glyph at hud_sign_glyph_size in its tone. The tap target is hud_sign_hit_radius around it.
 *              Over all markers the open sign's reading.
 *
 *              THE FRAME IS WHOLE. The markers draw from SignMarkerFrame alone and never ask whether a port
 *              delivered: while the state fields of each sign case (N0461) and the weather signatures SE02, SE24,
 *              SE30 (N0449) have no port, VivariumView::bridge_signs (godot_vivarium_hud_signs.cpp) writes the
 *              mockup's signs into the frame and marks both gaps on its edge. The values of the readings come
 *              from the same bridge, the place of each carried item from VivariumCarryBridge::fill (N0451).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    ecs/module
 * @usecase     usecase:100370 - wilt from drought
 * @actor       Hänge-Birke (Betula pendula)
 * @register    N0461 (the state fields of each sign case), N0449 (the weather signatures), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_stream_view.hpp>
#include <ase/adp/godot/godot_hud_context_resolver.hpp>
#include <ase/adp/godot/godot_hud_sign_reading.hpp>
#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_ground.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The mark hud.signs.sap.alert names: the vessel under the tap is full. */
constexpr const char* GODOT_HUD_MARK_VESSEL_FULL = "vessel_full";

/** What the signs show this frame. */
struct SignMarkerFrame {
    HudShown shown;                       // the context's decision: the signs, the open one, the level
    bool trunk_placed = false;            // the tree stands on the screen this frame
    ::godot::Vector2 trunk;               // the mockup's (TX, TY()): the foot of the trunk, in mockup units
    double height = 0.0;                  // the mockup's treeH(): the height of the tree, in mockup units
    ::godot::PackedVector2Array brook;    // the brook's line in mockup units, its points at equal steps of the share
    std::vector<SignValue> values;        // the values the readings and bars name
    std::vector<SignPlace> places;        // where each item in possession lies (VivariumCarryBridge::fill, N0451)
    double elapsed = 0.0;                 // seconds since the signs appeared
    double opened = 0.0;                  // seconds since the open sign opened
    bool still = false;                   // every motion held
};

/** What a tap on the signs hit. */
struct SignMarkerHit {
    uint32_t sign = UINT32_MAX;    // index into HudContextSpec::signs of the tapped marker
    uint32_t offer = UINT32_MAX;   // index into HudContextSpec::tools of the tapped offer chip
};

/** The square round one drawn marker's tap target, in mockup units, and the sign it opens. */
struct SignMarkerPlace {
    uint32_t sign = UINT32_MAX;    // index into HudContextSpec::signs
    ::godot::Rect2 rect;
};

/** The signs of the care view. */
class SignMarker {
public:
    /** Binds measures, colours, glyphs, words and levels and the reading; false with the missing name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the markers and the open reading of `now` under `frame`; false with the reason. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SignMarkerFrame& now,
              std::string& failed);

    /** The tone of the sign `sign` (index into HudContextSpec::signs), for the edge rail. */
    bool tone(uint32_t sign, ::godot::Color& out) const;

    /** What `point` (in mockup units) hits: the offer chip of the open reading first, then the markers. */
    bool hit(const ::godot::Vector2& point, SignMarkerHit& out) const;

    /** The square round each marker drawn last, hud_sign_hit_radius from its centre: where hit() answers a sign. */
    void places(std::vector<SignMarkerPlace>& out) const;

    /**
     * Lays the brook's line into `out` as SignMarkerFrame::brook carries it (B0285): `seen` holds the points of
     * the brook's water in mockup units from its start to its end; the line is cut to its part over the drawn
     * surface from `left` to `right` (the whole line where all of it or none of it lies there), then laid again
     * at as many points as `seen` holds, at equal steps of its length, so the share a sign names is the share of
     * the drawn line. Fewer than two points lay nothing.
     */
    static void lay_brook(const std::vector<::godot::Vector2>& seen, float left, float right,
                          ::godot::PackedVector2Array& out);

    /**
     * Lays the line of `stream`'s water as the camera sees it into `out` through lay_brook (B0285): the stations
     * of build_water_line over `ground`, only the water `land` lets the eye of `anchor` see over the bank,
     * placed through `anchor` in mockup units. Where the bank hides all of the water, the line is that of the
     * bed the eye sees (build_bed_line), which is then what the picture shows of the brook. `flank` is the
     * line's account (stations, points seen, the columns, the points), for the caller to tell when it changes.
     */
    static void follow_brook(const VivariumStreamView& stream, const PlantMeshArrays& ground, const UiAnchor& anchor,
                             const UiGround& land, float left, float right, ::godot::PackedVector2Array& out,
                             std::string& flank);

private:
    /** One sign as SHA places and marks it. */
    struct Sign {
        ::godot::Color tone;    // hud_sign_<id>_tone
        std::string at;         // hud_sign_<id>_at: crown, trunk, brook or screen
        double x = 0.0;         // hud_sign_<id>_x
        double y = 0.0;         // hud_sign_<id>_y
        UiIcon glyph;           // texts.hud.signs.<id>.glyph
    };

    /** One marker drawn last, for hit(). */
    struct Drawn {
        uint32_t sign = UINT32_MAX;
        ::godot::Vector2 centre;
    };

    /** Binds the sign `spec`; false with the missing name in `failed`. */
    bool bind_sign(const UiTheme& theme, const HudSignSpec& spec, Sign& out, std::string& failed) const;

    /** Where the sign stands this frame; false where the frame does not place its anchor. */
    bool place(const Sign& sign, const SignMarkerFrame& now, ::godot::Vector2& out) const;

    /** The marker of the shown sign at `centre`, the `order`-th to fade in. */
    void draw_marker(UiPen& canvas, const ::godot::Transform2D& frame, const SignMarkerFrame& now,
                     const HudSignShown& shown, size_t order, const ::godot::Vector2& centre);

    double radius_ = 0.0;               // hud_sign_radius
    double glyph_size_ = 0.0;           // hud_sign_glyph_size
    double line_w_ = 0.0;               // hud_sign_line
    double on_line_ = 0.0;              // hud_sign_on_line
    double pulse_radius_ = 0.0;         // hud_sign_pulse_radius
    double hit_radius_ = 0.0;           // hud_sign_hit_radius
    double stagger_ = 0.0;              // hud_sign_stagger
    double crown_min_h_ = 0.0;          // hud_sign_crown_min_h
    double crown_spread_ = 0.0;         // hud_sign_crown_spread
    double line_ = 0.0;                 // hud_line: the pulse
    uint32_t level_words_ = 0;          // hud.perception.levels[0]
    ::godot::Color disc_;               // OVERLAY_DISC
    std::vector<Sign> signs_;           // one per entry of hud.signs, in its order
    UiMotion fade_;                     // hud_anim_fade
    UiRing ring_;                       // the pulse, the disc and its ring
    SignReading reading_;               // the reading of the open sign
    std::vector<Drawn> drawn_;          // the markers drawn last
};

}  // namespace ase::adp::godot

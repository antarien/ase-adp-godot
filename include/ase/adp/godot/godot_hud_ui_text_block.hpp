#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI TEXT BLOCK
 *
 * @file        godot_hud_ui_text_block.hpp
 * @brief       One line of HUD text as the mockup's T draws it: face, tracking, case, anchor, shadow, fade
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiTextBlock. The mockup's T(x, y, s, o)
 *              is an SVG text of one of three classes, always with the class sh:
 *                t   the display face in its medium weight, letter spacing hud_tracking_label, upper case
 *                tb  the display face in its semibold weight, letter spacing hud_tracking_title, upper case
 *                d   the data face (Fira Code, the Nerd Font that also carries every icon), no spacing, as written
 *                sh  paint-order stroke: an outline of OVERLAY_SHADOW at hud_text_shadow_width under the fill
 *              One text of the HUD is written past T, a bare <text class="d"> without sh: the stack count of
 *              itemsIn (m.html 756), dark ink on the pouch's linen. UiTextPlace::bare draws it without the outline;
 *              with it the count was a black smudge (run 20261113-box80-bag, likeness_bag_pouch).
 *              font-size o.z (hud_text_size when the caller gives none - the caller reads it through size()),
 *              text-anchor o.a (start, middle, end), the fill o.col (OVERLAY_INK when it gives none), the
 *              opacity o.op, the fade-in (.fade, hud_anim_fade_ms and hud_anim_fade_ease) after the delay o.d,
 *              and the one rotation the mockup uses, rotate(a px py).
 *
 *              SHAPED, NOT SPELLED. The line is shaped by Godot's TextServer, so the font's kerning and its
 *              glyph substitutions stand as a browser draws them; the letter spacing is added after every
 *              cluster in floating point, as CSS letter-spacing does, never rounded to whole pixels.
 *
 *              RASTERED AT THE SCREEN'S PIXELS. The HUD draws in the mockup's units under a frame transform,
 *              and the project stretches its canvas onto the window (canvas_items), so neither a mockup unit nor
 *              a canvas unit is a screen pixel. A glyph rastered at either and carried onto the screen is
 *              resampled, and that resampling is the blur of a letter that is a vector. So the line takes the
 *              device transform, the pen's screen() times the frame, rasters at the nearest whole screen pixel
 *              size it gives (hud_raster) and draws its glyphs 1:1 with oversampling 1, NEVER scaled: the start of
 *              its baseline lands on a whole screen pixel, and the width it reports is the pixel advance over the
 *              device scale. Godot's font sizes are whole numbers while the mockup's are not (7.2, 8.6); the
 *              nearest whole pixel size is the one a browser would pick. A rotated line turns in screen pixels.
 *
 *              RUNS. A line is a sequence of HudRun: text in the block's face, or an ICON_* drawn through
 *              UiIcon on the line's baseline as the inline glyph it is. Port values ({name}) are filled in by
 *              the caller through fill_words (godot_hud_ui_word_fill.hpp) before set; this unit draws what it
 *              is handed.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** Where and how one line is drawn this frame: the mockup's x, y and o. */
struct UiTextPlace {
    ::godot::Vector2 at;           // x, y: the baseline point the anchor stands on, in the mockup's units
    double size = 0.0;             // o.z: the font size in the mockup's units
    double anchor = 0.0;           // o.a: the share of the width left of `at` (GODOT_TEXT_ANCHOR_*)
    ::godot::Color color;          // o.col: the fill; the caller passes ink() where the mockup gives none
    double opacity = 0.0;          // o.op: the opacity of the whole text
    bool fade = false;             // o.fade: fades in with hud_anim_fade
    double delay = 0.0;            // o.d: the seconds before the fade starts
    double elapsed = 0.0;          // the seconds since the text appeared
    bool still = false;            // .still and reduced motion: every animation at its end
    bool bare = false;             // a <text> written past T, without the class sh: no outline (m.html 756)
    double rotate_deg = 0.0;       // o.tr rotate(a px py): the angle, clockwise in degrees
    ::godot::Vector2 pivot;        // o.tr rotate(a px py): the point it turns about
};

/** One shaped glyph of a line, in raster pixels from the line's start. */
struct UiTextGlyph {
    ::godot::RID font;      // the font the shaper took the glyph from
    int64_t index = 0;      // the glyph index in that font
    int64_t raster = 0;     // the raster size the glyph was shaped at
    ::godot::Vector2 pos;   // the glyph's origin on the baseline, in raster pixels
};

/** One icon of a line, in raster pixels from the line's start. */
struct UiTextIcon {
    UiIcon icon;      // the bound icon
    double x = 0.0;   // the centre of its advance, in raster pixels
};

/** One line of HUD text of one of the mockup's classes t, tb and d. */
class UiTextBlock {
public:
    /** Binds the class t: display medium, its spacing, upper case. False with the missing name in `failed`. */
    bool bind_label(const UiTheme& theme, std::string& failed);

    /** Binds the class tb: display semibold, its spacing, upper case. */
    bool bind_title(const UiTheme& theme, std::string& failed);

    /** Binds the class d: the data face, no spacing, as written. */
    bool bind_data(const UiTheme& theme, std::string& failed);

    /** The mockup's default font size (hud_text_size), for a caller whose T gives no o.z. */
    double size() const { return size_; }

    /** The mockup's default fill (OVERLAY_INK, var(--hud)), for a caller whose T gives no o.col. */
    const ::godot::Color& ink() const { return ink_; }

    /** The text of the line as one run. False when an icon of it cannot be bound, with its name in `failed`. */
    bool set(const std::string& text, std::string& failed);

    /** The runs of the line. False when an icon of them cannot be bound, with its name in `failed`. */
    bool set(const std::vector<HudRun>& runs, std::string& failed);

    /**
     * The advance of the whole line at `size` under `frame` on `canvas`, in the mockup's units (spacing included):
     * the line is shaped at the screen pixels the pen's screen() times the frame gives it, as draw shapes it.
     */
    double width(const UiPen& canvas, const ::godot::Transform2D& frame, double size);

    /** Draws the line under `frame`, the transform from the mockup's units to the canvas the HUD draws on. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiTextPlace& place);

    /** The CPU time of the drawing thread the last draw spent shaping the line anew, 0 when it kept its glyphs. */
    uint64_t shape_usec() const { return shape_usec_; }

    /**
     * Readies the face at the raster `size` under `frame` on `canvas` gives, before the line first shows (P0692):
     * the text server builds a raster size of a face on the first line shaped and drawn at it, 71.3 ms for the
     * reading's value in the hand at the first act (run p0692-split), and rasters a glyph on its first draw, 40.1 ms
     * for the letters of the reading's label and value while only the digits were readied (run p0692-warm).
     * warm() shapes WARM_NUMBERS and the theme's alphabet() at that raster in one frame, then rasters the glyphs the
     * shaper named for the fill and the outline draw() uses, `budget` glyphs per call and fewer when the budget is
     * spent; it takes one from `budget` for each glyph it rasters and spends all of it in the frame it shapes.
     */
    void warm(const UiPen& canvas, const ::godot::Transform2D& frame, double size, int64_t& budget);

    /** The glyphs every frame may raster ahead of their first draw, shared by every block a widget warms. */
    static constexpr int64_t WARM_GLYPHS_PER_FRAME = 16;  // at 8 readying cost at most 1.2 ms a frame (p0692-budget)

private:
    static constexpr const char32_t* WARM_NUMBERS = U"+-.0123456789";  // what String::num and a sign write

    /** The outline radius draw() strokes the shadow with, in the text server's units, at `pixel` per unit. */
    int64_t outline(double pixel) const;

    bool bind_face(const UiTheme& theme, const ::godot::Ref<::godot::FontFile>& face, const char* face_name,
                   double spacing_em, bool upper, std::string& failed);
    void shape(int64_t raster);

    const UiTheme* theme_ = nullptr;           // the theme the icons of a run bind against
    ::godot::Ref<::godot::FontFile> face_;     // the face of the class
    ::godot::Ref<::godot::FontFile> icon_face_;  // the data face, whose metrics place an icon on the baseline
    double spacing_em_ = 0.0;                  // the letter spacing of the class, in em
    bool upper_ = false;                       // text-transform uppercase of the class
    double size_ = 0.0;                        // hud_text_size
    double shadow_width_ = 0.0;                // hud_text_shadow_width
    ::godot::Color ink_;                       // OVERLAY_INK
    ::godot::Color shadow_;                    // OVERLAY_SHADOW
    UiMotion fade_;                            // hud_anim_fade
    std::vector<HudRun> runs_;                 // the runs as set
    std::vector<UiTextGlyph> glyphs_;          // the shaped glyphs of the text runs
    std::vector<UiTextIcon> icons_;            // the icons of the icon runs
    int64_t shaped_raster_ = 0;                // the raster size glyphs_ were shaped at; 0 = not shaped
    double advance_ = 0.0;                     // the advance of the whole line, in raster pixels
    uint64_t shape_usec_ = 0;                  // the CPU time of the last draw's shape(), 0 when it kept glyphs_
    int64_t warmed_raster_ = 0;                // the raster size warm() readied the face at; 0 = none
    int64_t warm_raster_ = 0;                  // the raster size warm() shaped warm_glyphs_ at; 0 = none
    int64_t warm_stroke_ = 0;                  // the outline radius at warm_raster_ (outline())
    size_t warm_next_ = 0;                     // the first glyph of warm_glyphs_ not yet rastered
    std::vector<UiTextGlyph> warm_glyphs_;     // the glyphs of the numbers and the alphabet at warm_raster_
};

}  // namespace ase::adp::godot

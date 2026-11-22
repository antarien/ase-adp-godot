#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI PEN
 *
 * @file        godot_hud_ui_pen.hpp
 * @brief       The one drawer of the HUD: every shape laid in mockup units, rendered anew at the screen's pixels
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiPen. Every block of the HUD lays its
 *              shapes in the units of the mockup's frame (360 x 640) and hands the frame's transform with
 *              draw_set_transform_matrix, as a canvas would take it.
 *
 *              NOTHING IS SCALED. A canvas under a scaling transform rasterises a shape in mockup units and
 *              stretches the result: a line's antialiased edge grows by the frame's factor, a circle keeps the
 *              corners of its mockup radius, a polygon's edge stays a staircase of mockup units. The pen moves
 *              every point through the transform AND the screen() stretch itself, and sets the canvas to the
 *              inverse of screen(), so each shape is rendered at the size it stands on the SCREEN, as the browser
 *              renders the mockup's SVG at its device pixels - on the window 450 wide, on the acceptance window
 *              720 wide and on a handset 1080 wide alike:
 *                points    each point through screen() times the transform into screen pixels
 *                widths    a line's width, a dash and a radius times that transform's scale; a width below 0
 *                          stays the canvas's hairline of one pixel
 *                round     a circle or an arc is divided by its length in screen pixels, never fewer points
 *                          than its caller asked (GODOT_PEN_SEGMENT_PX)
 *                edges     every line, arc and circle antialiased in screen pixels, its coverage the browser's
 *                          (see COVERAGE); an opaque polygon of one colour gets a rim of GODOT_PEN_RIM_PX
 *                          antialiased along its outline drawn in, as the browser smooths a filled path's edge
 *              A transform that is not a uniform scale (the oval of a hole, a shadow's ellipse) moves the
 *              circle's points one by one into a polygon, so the shape is right on every axis.
 *
 *              COVERAGE AS THE BROWSER'S. Godot antialiases a line, a polyline and a circle with a feather of
 *              GODOT_PEN_FEATHER_PX that it attaches OUTSIDE the nominal width, full colour at the edge and
 *              transparent at its end (renderer_canvas_cull.cpp, FEATHER_SIZE; below a core of 1 it scales the
 *              feather by the core). Drawn at its nominal width, every stroke covers GODOT_PEN_FEATHER_PX more
 *              than the browser's: a tool's outline of 1.26 pixels covered 2.5 and the rack's stake of 3.2
 *              covered 4.3 - the strokes went thick and soft. The pen hands Godot the core whose core plus
 *              feather covers exactly the nominal width, so the feather's ramp lies centred on the nominal edge,
 *              and pulls an open line's ends in by half their end feather; a filled circle takes the radius whose
 *              feather's middle lies on the nominal radius. An opaque polygon is filled drawn in by the rim's half
 *              width plus half the feather (Geometry2D::offset_polygon) and its rim runs along that outline, so
 *              the rim's ramp lies centred on the nominal edge; a polygon thinner than that draws as given,
 *              without a rim.
 *
 *              GLYPHS ARE RASTERED BY THE FONT, AT THE SCREEN'S PIXELS. The project stretches its canvas
 *              (window/stretch/mode canvas_items: a canvas 720 wide shown on a window 450 wide or a handset 1080
 *              wide), so a canvas unit is no screen pixel. A HUD line shapes its glyphs at the whole screen pixel
 *              size that screen() times the frame gives it (hud_raster) and draws them through the text server on
 *              get_canvas_item with oversampling 1: the transform last set is the inverse of screen() and a move to
 *              a whole screen pixel, so the glyphs land 1:1 as rasterised, never resampled.
 *              draw_string moves the string's start into screen pixels and asks the font at its size times the
 *              scale of screen() times the transform.
 *
 *              A GROUP'S BRIGHTNESS IS A TINT OF THE PEN. The mockup darkens the world group with the hour
 *              (drawWorld's filter); the pen multiplies every colour it draws by the tint set last, and the glyphs
 *              on get_canvas_item take theirs through tinted. The canvas item's modulate would darken every block
 *              drawn after the group, the band and the windows with it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>

namespace ase::adp::godot {

constexpr double GODOT_PEN_SEGMENT_PX = 2.0;     // longest chord, in screen pixels, of a circle or an arc the pen divides
constexpr float GODOT_PEN_RIM_PX = 1.0f;         // core width, in screen pixels, of the antialiased rim along an opaque polygon's drawn-in outline
constexpr float GODOT_PEN_FEATHER_PX = 1.25f;    // Godot's antialiasing feather outside a line's, a polyline's and a circle's nominal edge (renderer_canvas_cull.cpp FEATHER_SIZE), screen pixels
constexpr float GODOT_PEN_UNIFORM_EPS = 1.0e-4f; // largest difference of the axes' lengths and of their dot product that still counts as one scale

/** The HUD's drawer onto one canvas: the shapes in the units of the transform set last, rendered at screen pixels. */
class UiPen {
public:
    explicit UiPen(::godot::CanvasItem& canvas);

    /**
     * The transform from the units of the shapes that follow into the canvas's units; the pen carries the shapes on
     * through screen() into screen pixels, and the canvas keeps the inverse of screen().
     */
    void draw_set_transform_matrix(const ::godot::Transform2D& matrix);

    void draw_line(const ::godot::Vector2& from, const ::godot::Vector2& to, const ::godot::Color& color,
                   float width = -1.0f, bool antialiased = false);
    void draw_dashed_line(const ::godot::Vector2& from, const ::godot::Vector2& to, const ::godot::Color& color,
                          float width = -1.0f, float dash = 2.0f, bool aligned = true, bool antialiased = false);
    void draw_polyline(const ::godot::PackedVector2Array& points, const ::godot::Color& color, float width = -1.0f,
                       bool antialiased = false);
    void draw_polygon(const ::godot::PackedVector2Array& points, const ::godot::PackedColorArray& colors);
    void draw_colored_polygon(const ::godot::PackedVector2Array& points, const ::godot::Color& color);
    void draw_circle(const ::godot::Vector2& centre, float radius, const ::godot::Color& color, bool filled = true,
                     float width = -1.0f, bool antialiased = false);
    void draw_arc(const ::godot::Vector2& centre, float radius, float start_angle, float end_angle, int32_t point_count,
                  const ::godot::Color& color, float width = -1.0f, bool antialiased = false);
    void draw_rect(const ::godot::Rect2& rect, const ::godot::Color& color, bool filled = true, float width = -1.0f,
                   bool antialiased = false);
    void draw_texture_rect_region(const ::godot::Ref<::godot::Texture2D>& texture, const ::godot::Rect2& rect,
                                  const ::godot::Rect2& source);
    void draw_string(const ::godot::Ref<::godot::Font>& font, const ::godot::Vector2& at, const ::godot::String& text,
                     ::godot::HorizontalAlignment alignment, float width, int32_t font_size, const ::godot::Color& color);
    /**
     * A brightness over the shapes that follow, as the mockup's filter brightness() over one group (drawWorld): the
     * red, green and blue of every colour the pen draws are multiplied by the tint's, white draws them as given. The
     * canvas item's own modulate stays untouched, so a tint ends where its group ends, with set_tint of white.
     */
    void set_tint(const ::godot::Color& tint);

    /** A colour as the pen draws it under the tint set last; the glyphs drawn on get_canvas_item take it from here. */
    ::godot::Color tinted(const ::godot::Color& color) const;

    /**
     * The canvas item for the text server's glyphs: the canvas takes the transform set last, whose scale a HUD line
     * makes one by shaping at its whole pixel size (hud_raster).
     */
    ::godot::RID get_canvas_item();

    /**
     * The canvas's units into the screen's pixels: the viewport's stretch (canvas_items) and the item's own place.
     * A glyph rastered at a size of the canvas is stretched once more by this; one rastered at screen pixels and set
     * under its inverse lands 1:1 (UiTextBlock, UiIcon).
     */
    ::godot::Transform2D screen() const;

private:
    /** The canvas back at the inverse of screen() before a shape the pen moves into screen pixels itself. */
    void plain();

    /** The points through screen() times the transform into screen pixels. */
    ::godot::PackedVector2Array moved(const ::godot::PackedVector2Array& points) const;

    /** A width in the shapes' units at screen pixels; a width below 0 stays the hairline. */
    float pixels(float width) const;

    /** An open run of screen points with its first and last point pulled in by half the end feather of `core`. */
    ::godot::PackedVector2Array trimmed(const ::godot::PackedVector2Array& points, float core) const;

    /** The points of the circle or arc from `start` to `end` radians in the shapes' units, moved into screen pixels. */
    ::godot::PackedVector2Array round(const ::godot::Vector2& centre, float radius, float start, float end,
                                      int32_t least, bool closed) const;

    /** An opaque polygon of one colour in screen pixels: filled drawn in, its rim along that outline. */
    void fill_rimmed(const ::godot::PackedVector2Array& drawn, const ::godot::Color& shade);

    ::godot::CanvasItem& canvas_;
    ::godot::Transform2D matrix_;    // the shapes' units into the canvas's units, as the caller set it
    ::godot::Transform2D unscreen_;  // the inverse of screen(): the canvas's transform while the pen draws a shape
    ::godot::Transform2D device_;    // screen() times matrix_: the shapes' units into screen pixels
    float scale_ = 1.0f;     // the length of a unit of the shapes in screen pixels (uniform transforms)
    bool uniform_ = true;    // the transform scales both axes alike and keeps them square
    bool upright_ = true;    // the transform does not turn
    bool handed_ = false;    // the canvas carries matrix_ (get_canvas_item) or a turn, not the inverse of screen()
    ::godot::Color tint_ = ::godot::Color(1.0f, 1.0f, 1.0f, 1.0f);  // the brightness of the group drawn now, white outside one
};

}  // namespace ase::adp::godot

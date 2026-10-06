#pragma once

/**
 * ASE GODOT ADAPTER - VIEW LAYOUT PLACEMENT
 *
 * @file        godot_view_layout.hpp
 * @brief       Places the three bands of the Vivarium layout on a surface of any aspect ratio
 * @description The start configuration lays the view out on its logical size (720 x 1280) in
 *              three vertical bands: header, patches with their info lines, buttons with the note.
 *              A handset of the Android market has another aspect ratio and insets for camera
 *              cut-out and system bars, and Godot's expand aspect hands the view the whole visible
 *              surface. This placement maps the bands onto the safe part of that surface: the top
 *              band to its top, the bottom band to its bottom, the middle band into the middle of
 *              the space between, the column centred horizontally. A safe part smaller than the
 *              logical size scales everything down evenly; a larger one never magnifies - the
 *              spare space is distributed between the bands.
 *
 *              NO GODOT TYPE: plain numbers in, plain numbers out, so the host tests check every
 *              aspect ratio without a Godot process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-06
 * @modified    2026-10-06
 * @version     00.00.01.00001
 */

namespace ase::adp::godot {

/** The layout as the start configuration states it: logical size and the two band edges. */
struct ViewBands {
    float width = 0.0f;          // logical width the layout is drawn for
    float height = 0.0f;         // logical height the layout is drawn for
    float top_end = 0.0f;        // the top band is [0, top_end)
    float bottom_start = 0.0f;   // the bottom band is [bottom_start, height]; between them the middle band
};

/** The visible surface and its safe insets, in the logical units of the canvas. */
struct ViewSurface {
    float width = 0.0f;
    float height = 0.0f;
    float inset_left = 0.0f;
    float inset_top = 0.0f;
    float inset_right = 0.0f;
    float inset_bottom = 0.0f;
};

/**
 * Where the layout lands: a layout point (x, y) stands at (offset_x + x * scale,
 * band_offset(y) + y * scale). scale is 0 when the surface or the layout has no area.
 */
struct ViewPlacement {
    float scale = 0.0f;
    float offset_x = 0.0f;
    float top_y = 0.0f;      // vertical offset of the top band
    float middle_y = 0.0f;   // vertical offset of the middle band
    float bottom_y = 0.0f;   // vertical offset of the bottom band
};

/** The placement of the bands on the surface. */
[[nodiscard]] ViewPlacement place_view(const ViewBands& bands, const ViewSurface& surface);

/** The vertical offset of the band a layout y belongs to. */
[[nodiscard]] float band_offset(const ViewBands& bands, const ViewPlacement& placement, float layout_y);

/**
 * True when a layout span [top, bottom] lies inside ONE band - the start configuration is refused
 * otherwise, because a span across a band edge would be torn apart on a taller surface.
 */
[[nodiscard]] bool in_one_band(const ViewBands& bands, float top, float bottom);

}  // namespace ase::adp::godot

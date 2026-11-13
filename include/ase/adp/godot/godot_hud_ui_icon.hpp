#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI ICON
 *
 * @file        godot_hud_ui_icon.hpp
 * @brief       The one drawer of a symbol of the HUD: an ICON_* of ui_icons.json in the Nerd Font, centred on a point
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, building block Basis: UiIcon draws only an ICON_*
 *              of SHA's icon table (ui_icons.json, the generate of icon-definitions.ts and Icon.tsx) through
 *              read_icon and never a raw glyph; no second reader of symbols stands beside it.
 *
 *              BOUND ONCE, DRAWN EVERY FRAME. bind looks the name up in the table UiTheme read and checks
 *              that the data face of the HUD (hud.fonts.data, the Nerd Font) carries its glyph; an ICON_*
 *              the table lacks or a glyph the face lacks refuses with the name, so a symbol never draws as a
 *              blank box. draw then only places the bound glyph.
 *
 *              THE GLYPH OF THE MOCKUP. The mockup's G(x, y, k, z, col, o) sets the glyph with text-anchor
 *              middle and dominant-baseline central at font size z in col, at opacity o.op: draw centres the
 *              glyph's advance on `centre` horizontally and the middle of the face's ascent and descent on it
 *              vertically, and shifts it by the offset Icon.tsx gives the glyph, CSS pixels at web_size_px,
 *              scaled to `size` - the same share draw_icon_label applies. That offset is the glyph's own
 *              centring: icon-replacer.ts derives it from the ink box of the outline, so at web_size_px it is
 *              the distance of the ink centre from the advance centre, negated, and for the solid play
 *              triangle the distance of its area centroid (measured over every HUD symbol by
 *              clients/avt-client-android/data/probes/icon_centre_probe.py). Without it a glyph sits up to
 *              0.15 em right of the middle of its hexagon or circle. The fade the mockup lays over a
 *              glyph is an opacity its group computes from the tokens hud_anim_fade_ms and hud_anim_fade_ease;
 *              this unit takes the opacity and computes no time.
 *
 *              THE MIDDLE OF THE FACE, NOT OF ITS ROUNDED METRICS. The browser places dominant-baseline central
 *              from the face's ascent and descent as the font designs them (hhea 1800 and 600 of 1950 for the
 *              Nerd Font); the text server answers get_ascent and get_descent at a raster size rounded to whole
 *              pixels, which at 37 px moves the baseline from 11.39 to 11.5 below the centre and the pen's whole
 *              pixel then to 12: the glyph sat 0.8 px low in its hexagon. bind reads both once at
 *              GODOT_TEXT_DESIGN_PX as shares of the size, and draw places the baseline from those shares.
 *
 *              RASTERED AT THE SCREEN'S PIXELS. The HUD draws in the mockup's units under a frame transform,
 *              and the project stretches its canvas onto the window (canvas_items): a glyph rastered at the
 *              mockup's or the canvas's size and carried onto the screen is resampled and blurs, while Godot's
 *              font sizes are whole numbers and the mockup's are not (r * 0.92). The draw that takes the frame
 *              rasters at hud_raster, the nearest whole screen pixel size the device transform (the pen's
 *              screen() times the frame) gives `size`, and draws the glyph 1:1 with oversampling 1 and its
 *              centre on a whole screen pixel, NEVER scaled back onto `size` - the rule UiTextBlock shares. The
 *              draw without a frame is the one under such a pixel transform: its pen start is set on a whole
 *              pixel and the font draws on the pen's canvas item, never through the pen's own scaled string.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-11
 * @version     00.00.01.00004
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

constexpr float GODOT_TEXT_OVERSAMPLING = 1.0f;  // the text server's oversampling for a HUD glyph: its raster size already is the screen's pixels
constexpr int32_t GODOT_TEXT_DESIGN_PX = 4096;   // the size the face's ascent and descent are read at once: their share of the size to 1/4096, where a raster size rounds them to whole pixels

/**
 * The whole raster size a glyph of `size` mockup units gets under `device`, the transform from the mockup's units
 * into the screen's pixels (UiPen::screen() times the frame): never magnified, never blurred.
 */
int64_t hud_raster(const ::godot::Transform2D& device, double size);

/** One ICON_* of the HUD, bound to its glyph, offset and face. */
class UiIcon {
public:
    /**
     * Binds the ICON_* `name` of the theme's icon table to the theme's data face. False with `failed` set to
     * "icon.<name>" when the table does not name it or the face does not carry its glyph.
     */
    bool bind(const UiTheme& theme, const std::string& name, std::string& failed);

    /** True once bind succeeded; draw and advance do nothing before. */
    bool bound() const { return font_.is_valid() && !glyph_.is_empty(); }

    /**
     * The glyph centred on `centre` at font size `size` in `color`, its alpha multiplied by `opacity` (0..1), drawn
     * 1:1 under the transform the pen holds: `centre` and `size` are in the screen pixels that transform keeps.
     */
    void draw(UiPen& canvas, const ::godot::Vector2& centre, double size, const ::godot::Color& color,
              double opacity) const;

    /** The same glyph under `frame` (mockup units to canvas), rastered at hud_raster and drawn 1:1, never scaled. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& centre,
              double size, const ::godot::Color& color, double opacity) const;

    /** The horizontal advance of the glyph at font size `size`, for a block that sets it inline with text. */
    double advance(double size) const;

    /** The ICON_* name it was bound to. */
    const std::string& name() const { return name_; }

private:
    ::godot::Ref<::godot::FontFile> font_;  // the data face of the HUD (hud.fonts.data), the Nerd Font
    ::godot::String glyph_;                 // the glyph of the codepoint ui_icons.json names
    ::godot::Vector2 offset_css_;           // the offset Icon.tsx gives the glyph, CSS pixels at web_px_
    double web_px_ = 0.0;                   // ui_icons.json: web_size_px
    double ascent_share_ = 0.0;             // the face's ascent per pixel of size, read at GODOT_TEXT_DESIGN_PX
    double descent_share_ = 0.0;            // the face's descent per pixel of size, read at GODOT_TEXT_DESIGN_PX
    std::string name_;                  // the ICON_* name
};

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD UI TEXT BLOCK
 *
 * @file        godot_hud_ui_text_block.cpp
 * @brief       Shapes and draws one line of HUD text as the mockup's T does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_text_block.hpp.
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

#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_vivarium_watch.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/text_server_manager.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ase::adp::godot {

namespace {

// The text server's outline size per screen pixel of stroke radius: Godot 4.5.2 text_server_adv _ensure_glyph strokes
// with FT_Stroker_Set(stroker, (int)(size * 16.0), ...) in 26.6 fixed point, so a radius of one pixel is size 4
constexpr float GODOT_TEXT_OUTLINE_PER_PX = 4.0f;

/** `color` with its alpha multiplied by `opacity`. */
::godot::Color faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

}  // namespace

bool UiTextBlock::bind_label(const UiTheme& theme, std::string& failed) {
    return bind_face(theme, theme.faces().display_medium, "hud.fonts.display_medium",
                     theme.faces().spacing_medium_em, true, failed);
}

bool UiTextBlock::bind_title(const UiTheme& theme, std::string& failed) {
    return bind_face(theme, theme.faces().display_semibold, "hud.fonts.display_semibold",
                     theme.faces().spacing_semibold_em, true, failed);
}

bool UiTextBlock::bind_data(const UiTheme& theme, std::string& failed) {
    return bind_face(theme, theme.faces().data, "hud.fonts.data", 0.0, false, failed);
}

bool UiTextBlock::bind_face(const UiTheme& theme, const ::godot::Ref<::godot::FontFile>& face, const char* face_name,
                            double spacing_em, bool upper, std::string& failed) {
    if (face.is_null()) {
        failed = face_name;
        return false;
    }
    if (theme.faces().data.is_null()) {
        failed = "hud.fonts.data";
        return false;
    }
    if (!theme.measure("hud_text_size", size_) || size_ <= 0.0) {
        failed = "hud_text_size";
        return false;
    }
    if (!theme.measure("hud_text_shadow_width", shadow_width_) || shadow_width_ < 0.0) {
        failed = "hud_text_shadow_width";
        return false;
    }
    if (!theme.color("OVERLAY_INK", ink_)) {
        failed = "OVERLAY_INK";
        return false;
    }
    if (!theme.color("OVERLAY_SHADOW", shadow_)) {
        failed = "OVERLAY_SHADOW";
        return false;
    }
    if (!fade_.bind(theme, "hud_anim_fade", failed)) {
        return false;
    }
    theme_ = &theme;
    face_ = face;
    icon_face_ = theme.faces().data;
    spacing_em_ = spacing_em;
    upper_ = upper;
    runs_.clear();
    glyphs_.clear();
    icons_.clear();
    shaped_raster_ = 0;
    advance_ = 0.0;
    return true;
}

bool UiTextBlock::set(const std::string& text, std::string& failed) {
    HudRun run;
    run.text = text;
    return set(std::vector<HudRun>{run}, failed);
}

bool UiTextBlock::set(const std::vector<HudRun>& runs, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.text_block.unbound";
        return false;
    }
    bool same = runs.size() == runs_.size();
    for (size_t index = 0; same && index < runs.size(); ++index) {
        same = runs[index].text == runs_[index].text && runs[index].icon == runs_[index].icon;
    }
    if (same && shaped_raster_ != 0) {
        return true;
    }
    // The icons bind here, once per change of the runs: a missing ICON_* fails the line at once.
    icons_.clear();
    for (const HudRun& run : runs) {
        if (run.icon.empty()) {
            continue;
        }
        UiTextIcon placed;
        if (!placed.icon.bind(*theme_, run.icon, failed)) {
            icons_.clear();
            return false;
        }
        icons_.push_back(placed);
    }
    runs_ = runs;
    shaped_raster_ = 0;
    return true;
}

void UiTextBlock::shape(int64_t raster) {
    if (shaped_raster_ == raster) {
        return;
    }
    // The shaping's own share of the draw (P0692): a line shaped anew asks the text server for every glyph.
    const uint64_t shape_start = thread_cpu_usec();
    glyphs_.clear();
    const ::godot::Ref<::godot::TextServer> server = ::godot::TextServerManager::get_singleton()->get_primary_interface();
    const ::godot::String language = ::godot::String::utf8(theme_->language().c_str());
    // letter-spacing: the spacing in em of the font size, added after every cluster - here in raster pixels.
    const double spacing = spacing_em_ * static_cast<double>(raster);
    double pen = 0.0;
    size_t icon = 0;
    for (const HudRun& run : runs_) {
        if (!run.icon.empty()) {
            // An icon is one more glyph of the line: its advance at the raster size, then the spacing.
            const double advance = icons_[icon].icon.advance(static_cast<double>(raster));
            icons_[icon].x = pen + advance * GODOT_HALF;
            pen += advance + spacing;
            ++icon;
            continue;
        }
        ::godot::String text = ::godot::String::utf8(run.text.c_str());
        if (upper_) {
            text = text.to_upper();
        }
        const ::godot::RID shaped = server->create_shaped_text();
        server->shaped_text_add_string(shaped, text, face_->get_rids(), raster, ::godot::Dictionary(), language);
        server->shaped_text_shape(shaped);
        const ::godot::TypedArray<::godot::Dictionary> glyphs = server->shaped_text_get_glyphs(shaped);
        for (int64_t index = 0; index < glyphs.size(); ++index) {
            const ::godot::Dictionary glyph = glyphs[index];
            const double advance = static_cast<double>(glyph["advance"]);
            // The shaper sets count on the first glyph of a cluster only: a new cluster closes the one before
            // it, and the spacing follows that closed cluster as a whole.
            const int64_t count = static_cast<int64_t>(glyph["count"]);
            if (count > 0 && index > 0) {
                pen += spacing;
            }
            UiTextGlyph drawn;
            drawn.font = glyph["font_rid"];
            drawn.index = static_cast<int64_t>(glyph["index"]);
            drawn.raster = static_cast<int64_t>(glyph["font_size"]);
            const ::godot::Vector2 offset = glyph["offset"];
            drawn.pos = ::godot::Vector2(static_cast<float>(pen) + offset.x, offset.y);
            glyphs_.push_back(drawn);
            pen += advance;
        }
        // The run's last cluster: its spacing stands as well, as the browser keeps it after the last letter.
        if (glyphs.size() > 0) {
            pen += spacing;
        }
        server->free_rid(shaped);
    }
    advance_ = pen;
    shaped_raster_ = raster;
    shape_usec_ = thread_cpu_usec() - shape_start;
}

double UiTextBlock::width(const UiPen& canvas, const ::godot::Transform2D& frame, double size) {
    if (theme_ == nullptr) {
        return 0.0;
    }
    // The glyphs stand unscaled in screen pixels: one raster pixel is one screen pixel, so the advance in the
    // mockup's units is the pixel advance over the device scale.
    const ::godot::Transform2D device = canvas.screen() * frame;
    const int64_t raster = hud_raster(device, size);
    shape(raster);
    const double pixel = static_cast<double>(ase::math::abs(device.get_scale().x));
    return pixel > 0.0 ? advance_ / pixel : 0.0;
}

void UiTextBlock::draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiTextPlace& place) {
    shape_usec_ = 0;  // this draw's share only: shape() below writes it when it shapes anew
    if (theme_ == nullptr || place.size <= 0.0) {
        return;
    }
    const double shown = place.fade ? fade_.progress(place.elapsed, place.delay, place.still) : 1.0;
    const double opacity = place.opacity * shown;
    if (opacity <= 0.0) {
        return;
    }
    // The device transform: the mockup's units into the screen's pixels, through the frame and the canvas's
    // stretch onto the window (canvas_items). A canvas pixel is no screen pixel, so the raster lives here.
    const ::godot::Transform2D screen = canvas.screen();
    const ::godot::Transform2D device = screen * frame;
    const double pixel = static_cast<double>(ase::math::abs(device.get_scale().x));
    if (pixel <= 0.0) {
        return;
    }
    const int64_t raster = hud_raster(device, place.size);
    shape(raster);
    // A glyph is never scaled: it is rastered at the whole screen pixel size and drawn 1:1 with oversampling 1,
    // its baseline start on a whole screen pixel. Scaling a rastered glyph resamples it, and that is the blur of
    // a vector letter. The anchored start in the mockup's units, the pixel advance over the device scale.
    const ::godot::Vector2 start(place.at.x - static_cast<float>(advance_ / pixel * place.anchor), place.at.y);
    const ::godot::Vector2 device_start = device.xform(start);
    const ::godot::Vector2 snapped(ase::math::round(device_start.x), ase::math::round(device_start.y));
    // rotate(a px py) in the mockup's units, carried into screen pixels: device * rotation * device⁻¹.
    ::godot::Transform2D turned;
    if (place.rotate_deg != 0.0) {
        const float turn = static_cast<float>(place.rotate_deg / GODOT_RING_TURN_DEG) * ase::math::TWO_PI;
        const ::godot::Transform2D rotation =
            ::godot::Transform2D(0.0f, place.pivot) * ::godot::Transform2D(turn, ::godot::Vector2()) *
            ::godot::Transform2D(0.0f, -place.pivot);
        turned = device * rotation * device.affine_inverse();
    }
    // The canvas takes screen⁻¹ first, so the stretch onto the window cancels and a raster pixel is a screen pixel.
    canvas.draw_set_transform_matrix(screen.affine_inverse() * turned * ::godot::Transform2D(0.0f, snapped));

    const ::godot::Ref<::godot::TextServer> server = ::godot::TextServerManager::get_singleton()->get_primary_interface();
    const ::godot::RID item = canvas.get_canvas_item();
    // The glyphs pass the pen's draw calls by, so they take the tint of the group drawn now from it.
    const ::godot::Color fill = canvas.tinted(faded(place.color, opacity));
    const ::godot::Color shadow = canvas.tinted(faded(shadow_, opacity));
    // paint-order stroke: the whole outline under the whole fill. The mockup's stroke-width is centred on the glyph's
    // edge, so half of it stands outside the fill: that half is the radius, in screen pixels, which the text server
    // takes in its own outline units. Oversampling 1: the raster size already is the screen's, a second factor from
    // the viewport would raster it again.
    // A bare text (m.html 756, the stack count) carries no sh: its fill alone.
    const int64_t stroke = place.bare ? 0 : outline(pixel);
    if (stroke > 0) {
        for (const UiTextGlyph& glyph : glyphs_) {
            server->font_draw_glyph_outline(glyph.font, item, glyph.raster, stroke, glyph.pos, glyph.index, shadow,
                                            GODOT_TEXT_OVERSAMPLING);
        }
    }
    for (const UiTextGlyph& glyph : glyphs_) {
        server->font_draw_glyph(glyph.font, item, glyph.raster, glyph.pos, glyph.index, fill, GODOT_TEXT_OVERSAMPLING);
    }
    // An icon sits on the baseline as an inline glyph: UiIcon centres on the middle of the data face's
    // ascent and descent, so that middle is handed as its centre.
    const float middle = ase::math::round(static_cast<float>((icon_face_->get_ascent(static_cast<int32_t>(raster)) -
                                                              icon_face_->get_descent(static_cast<int32_t>(raster))) *
                                                             GODOT_HALF));
    for (const UiTextIcon& icon : icons_) {
        icon.icon.draw(canvas, ::godot::Vector2(static_cast<float>(icon.x), -middle), static_cast<double>(raster),
                       place.color, opacity);
    }
    canvas.draw_set_transform_matrix(frame);
}

int64_t UiTextBlock::outline(double pixel) const {
    return static_cast<int64_t>(
        ase::math::ceil(static_cast<float>(shadow_width_ * pixel) * GODOT_HALF * GODOT_TEXT_OUTLINE_PER_PX));
}

void UiTextBlock::warm(const UiPen& canvas, const ::godot::Transform2D& frame, double size, int64_t& budget) {
    if (theme_ == nullptr || size <= 0.0 || budget <= 0) {
        return;
    }
    const ::godot::Transform2D device = canvas.screen() * frame;
    const double pixel = static_cast<double>(ase::math::abs(device.get_scale().x));
    if (pixel <= 0.0) {
        return;
    }
    const int64_t raster = hud_raster(device, size);
    if (warmed_raster_ == raster) {
        return;
    }
    const ::godot::Ref<::godot::TextServer> server = ::godot::TextServerManager::get_singleton()->get_primary_interface();
    if (warm_raster_ != raster) {
        // The shaping: the face's shaping tables at this raster are built on the first line shaped at it, and the
        // shaper names each character's glyph in the face's own font (its rids; UiTheme gives no face a second
        // font) as draw() takes it from shape(). Upper case where the class writes it, so the glyphs are the drawn ones.
        ::godot::String sample = ::godot::String(WARM_NUMBERS) + theme_->alphabet();
        if (upper_) {
            sample = sample.to_upper();
        }
        const ::godot::RID shaped = server->create_shaped_text();
        server->shaped_text_add_string(shaped, sample, face_->get_rids(), raster, ::godot::Dictionary(),
                                       ::godot::String::utf8(theme_->language().c_str()));
        server->shaped_text_shape(shaped);
        const ::godot::TypedArray<::godot::Dictionary> glyphs = server->shaped_text_get_glyphs(shaped);
        warm_glyphs_.clear();
        for (int64_t index = 0; index < glyphs.size(); ++index) {
            const ::godot::Dictionary glyph = glyphs[index];
            UiTextGlyph named;
            named.font = glyph["font_rid"];
            named.index = static_cast<int64_t>(glyph["index"]);
            named.raster = static_cast<int64_t>(glyph["font_size"]);
            warm_glyphs_.push_back(named);
        }
        server->free_rid(shaped);
        warm_raster_ = raster;
        warm_stroke_ = outline(pixel);
        warm_next_ = 0;
        // the shaping is this frame's share: no glyph is rastered beside it
        budget = 0;
        return;
    }
    // The raster: the fill at (raster, 0) and the shadow's outline at (raster, outline), the keys draw() draws at,
    // a glyph the text server already holds at a key costing a lookup only.
    while (budget > 0 && warm_next_ < warm_glyphs_.size()) {
        const UiTextGlyph& glyph = warm_glyphs_[warm_next_];
        if (glyph.font.is_valid()) {
            server->font_render_glyph(glyph.font, ::godot::Vector2i(static_cast<int32_t>(glyph.raster), 0),
                                      glyph.index);
            if (warm_stroke_ > 0) {
                server->font_render_glyph(glyph.font,
                                          ::godot::Vector2i(static_cast<int32_t>(glyph.raster),
                                                            static_cast<int32_t>(warm_stroke_)),
                                          glyph.index);
            }
        }
        ++warm_next_;
        --budget;
    }
    if (warm_next_ >= warm_glyphs_.size()) {
        warmed_raster_ = raster;
        warm_glyphs_.clear();
    }
}

}  // namespace ase::adp::godot

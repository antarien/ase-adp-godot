/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW DRAWING
 *
 * @file        godot_vivarium_draw.cpp
 * @brief       VivariumView - the placement of the layout on the visible surface and the canvas drawn over the scene
 * @description THE LAYOUT IS PLACED, NOT STRETCHED (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3): the expand aspect
 *              of the project hands the view the whole visible surface, of any aspect ratio. draw() fills
 *              it with the background, places the three bands of the layout through place_view
 *              (godot_view_layout.hpp) and draws every element with the transform of its band; the header
 *              colour reaches up to the top edge, over a camera cut-out. The rectangles placed here are
 *              the ones a tap is matched against (godot_vivarium_input.cpp).
 *
 *              PROJECTIONS OF THE LAST SNAPSHOTS, NOTHING ELSE: every drawn value is a value the ports
 *              reported after the last advance. While the 3D scene stands (godot_plant_scene.hpp) a patch
 *              is a frame over it - its label, its soil water and the selection; without the scene the
 *              patch draws its ground, cover and seeds flat. A ground whose asset is missing is drawn in
 *              the error material (03.2, ground_color).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-10-18
 * @modified    2026-10-25
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace ase::adp::godot {

namespace {

/** A share held inside 0..1 - a drawn share, never a simulation value. */
double share(double value) {
    if (!(value > 0.0)) {
        return 0.0;
    }
    return value < 1.0 ? value : 1.0;
}

/** The cover colour of a patch: its condition first (a dead or wilted stand), then its stage. */
::godot::Color cover_color(const VivariumPatchView& patch) {
    if (patch.condition == GODOT_COND_DEAD) {
        return ::godot::Color::hex(GODOT_COLOR_DEAD);
    }
    if (patch.condition == GODOT_COND_WILTED) {
        return ::godot::Color::hex(GODOT_COLOR_WILTED);
    }
    if (patch.condition == GODOT_CODE_UNKNOWN || patch.stage == GODOT_CODE_UNKNOWN) {
        return ::godot::Color::hex(GODOT_COLOR_UNKNOWN);
    }
    if (patch.stage == GODOT_STAGE_MATURE) {
        return ::godot::Color::hex(GODOT_COLOR_MATURE);
    }
    if (patch.stage == GODOT_STAGE_GROW) {
        return ::godot::Color::hex(GODOT_COLOR_GROW);
    }
    return ::godot::Color::hex(GODOT_COLOR_SPROUT);
}

}  // namespace

// PLACEMENT - the layout's three bands on the visible surface (godot_view_layout.hpp)

ViewSurface VivariumView::surface() const {
    // The visible part of the canvas. With the expand aspect it starts at the origin and carries
    // the window's aspect ratio, in the logical units of the layout.
    const ::godot::Rect2 visible = node_->get_viewport_rect();
    ViewSurface surface;
    surface.width = visible.size.x;
    surface.height = visible.size.y;
    // Camera cut-out and gesture bar exist on a handset only. On the desktop the safe area is the
    // screen's, not the window's, and would push the layout off any window that is not maximised.
    if (!::godot::OS::get_singleton()->has_feature(GODOT_FEATURE_MOBILE)) {
        return surface;
    }
    // DisplayServer reports the safe area in window pixels; the viewport's final transform maps
    // the canvas onto them, so its inverse maps the safe area back onto the canvas.
    const ::godot::Rect2i safe = ::godot::DisplayServer::get_singleton()->get_display_safe_area();
    const ::godot::Transform2D to_canvas = node_->get_viewport()->get_final_transform().affine_inverse();
    const ::godot::Vector2 safe_start = to_canvas.xform(::godot::Vector2(safe.position));
    const ::godot::Vector2 safe_end = to_canvas.xform(::godot::Vector2(safe.position + safe.size));
    surface.inset_left = ase::math::max(0.0f, safe_start.x - visible.position.x);
    surface.inset_top = ase::math::max(0.0f, safe_start.y - visible.position.y);
    surface.inset_right = ase::math::max(0.0f, visible.get_end().x - safe_end.x);
    surface.inset_bottom = ase::math::max(0.0f, visible.get_end().y - safe_end.y);
    return surface;
}

::godot::Rect2 VivariumView::placed(const ::godot::Rect2& rect) const {
    // The same mapping use_band hands the canvas: scale, then the column and the band's offset.
    const float scale = placement_.scale;
    return ::godot::Rect2(placement_.offset_x + rect.position.x * scale,
                          band_offset(layout_.bands, placement_, rect.position.y) + rect.position.y * scale,
                          rect.size.x * scale, rect.size.y * scale);
}

void VivariumView::use_band(float layout_y) {
    const float scale = placement_.scale;
    node_->draw_set_transform(
        ::godot::Vector2(placement_.offset_x, band_offset(layout_.bands, placement_, layout_y)), 0.0f,
        ::godot::Vector2(scale, scale));
}

void VivariumView::use_canvas() {
    node_->draw_set_transform(::godot::Vector2(), 0.0f, ::godot::Vector2(1.0f, 1.0f));
}

// DRAWING - projections of the last snapshots, nothing else

void VivariumView::draw() {
    if (in_editor()) {
        return;
    }
    drawn_surface_ = surface();
    const ::godot::Rect2 visible(0.0f, 0.0f, drawn_surface_.width, drawn_surface_.height);
    use_canvas();
    // The canvas draws over the 3D scene (03.1 point 2): while the scene stands, no background covers it.
    if (scene_ == nullptr) {
        node_->draw_rect(visible, ::godot::Color::hex(GODOT_COLOR_BACKGROUND));
    }
    if (!configured_) {
        // Without a configuration there is no layout to place: the error panel takes the surface.
        placement_ = ViewPlacement{};
        draw_failure(visible.grow(-GODOT_PATCH_INSET));
        return;
    }
    placement_ = place_view(layout_.bands, drawn_surface_);
    if (!(placement_.scale > 0.0f)) {
        return;  // a surface without room for the layout: the background stands alone
    }
    // The header colour runs over the whole width and up to the top edge - over a camera
    // cut-out too - and ends where the placed header band ends.
    node_->draw_rect(::godot::Rect2(0.0f, 0.0f, visible.size.x,
                                    placement_.top_y + layout_.header_height * placement_.scale),
                     ::godot::Color::hex(GODOT_COLOR_HEADER));
    const ::godot::Ref<::godot::Font> font = view_font();
    use_band(layout_.title_at.y);
    node_->draw_string(font, layout_.title_at, texts_.title, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       GODOT_FONT_TITLE, ::godot::Color::hex(GODOT_COLOR_TEXT));
    use_band(layout_.build_at.y);
    node_->draw_string(font, layout_.build_at, build_line_, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       build_font_size_, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));

    if (failure_text_.is_empty()) {
        for (std::size_t place = 0; place < layout_.place_rects.size(); ++place) {
            use_band(layout_.place_rects[place].position.y);
            draw_patch(layout_.place_rects[place], static_cast<uint32_t>(place));
        }
    } else {
        // The error panel stands where the patches stand, in their band.
        ::godot::Rect2 panel = layout_.place_rects.front();
        for (const ::godot::Rect2& rect : layout_.place_rects) {
            panel = panel.merge(rect);
        }
        use_band(panel.position.y);
        draw_failure(panel);
    }
    draw_info();
    use_band(layout_.water_rect.position.y);
    draw_button(layout_.water_rect, texts_.water, can_water());
    use_band(layout_.level_rect.position.y);
    draw_button(layout_.level_rect, texts_.level, can_level());
    use_band(layout_.prune_rect.position.y);
    draw_button(layout_.prune_rect, texts_.prune, can_prune());
    use_band(layout_.bed_rect.position.y);
    draw_button(layout_.bed_rect, texts_.bed, can_bed());
    use_band(layout_.flatten_rect.position.y);
    draw_button(layout_.flatten_rect, texts_.flatten, can_flatten());
    use_band(layout_.reset_rect.position.y);
    draw_button(layout_.reset_rect, texts_.reset, true);
    use_canvas();
}

::godot::String VivariumView::stage_text(uint8_t stage) const {
    if (stage == GODOT_STAGE_MATURE) {
        return texts_.stage_mature;
    }
    if (stage == GODOT_STAGE_GROW) {
        return texts_.stage_grow;
    }
    if (stage == GODOT_STAGE_SPROUT) {
        return texts_.stage_sprout;
    }
    if (stage == GODOT_STAGE_SEED) {
        return texts_.stage_seed;
    }
    return texts_.unknown;
}

::godot::String VivariumView::patch_text(const VivariumPatchView& patch) const {
    // "área 2 · Broto" - and the condition, when the stand carries one. A String first: in a const
    // member texts_ is const, and godot-cpp's String + const char* is no const member.
    ::godot::String text = ::godot::String(texts_.area) + " " + ::godot::String::num_uint64(patch.place + 1u) +
                           texts_.separator + stage_text(patch.stage);
    if (patch.condition == GODOT_COND_DEAD) {
        text += texts_.separator + texts_.condition_dead;
    } else if (patch.condition == GODOT_COND_WILTED) {
        text += texts_.separator + texts_.condition_wilted;
    } else if (patch.condition == GODOT_CODE_UNKNOWN) {
        text += texts_.separator + texts_.unknown;
    }
    return text;
}

uint32_t VivariumView::ground_color(const VivariumPatchView* patch) const {
    // The ground of a patch is a material of the asset list (03.2): its role follows the soil the
    // record carries, and while that role's asset is missing it is drawn in the error material. A
    // place without a record, or with a soil code the view does not know, has no role to bind.
    if (patch == nullptr || patch->soil >= GODOT_SOIL_COUNT) {
        return GODOT_COLOR_SOIL;
    }
    const auto role = asset_roles_.find(patch->soil == GODOT_SOIL_FOREST ? GODOT_ROLE_GROUND_FOREST
                                                                         : GODOT_ROLE_GROUND_MEADOW);
    return asset_color(assets_, role == asset_roles_.end() ? std::string() : role->second, GODOT_COLOR_SOIL);
}

void VivariumView::draw_patch(const ::godot::Rect2& rect, uint32_t place) {
    const VivariumPatchView* patch = resources_.get_patch_at(place);
    // With the 3D scene standing the patch is a frame over it - its label, its soil water and the
    // selection; the ground, the cover and the seeds are the scene's (03.1 point 3).
    const bool flat = scene_ == nullptr;
    if (flat) {
        node_->draw_rect(rect, ::godot::Color::hex(ground_color(patch)));
    }
    const ::godot::Rect2 inner = rect.grow(-GODOT_PATCH_INSET);
    if (patch != nullptr) {
        // The coverage decides the covered AREA: the cover square grows with its square root.
        const double cover = flat ? share(patch->coverage) : 0.0;
        if (cover > 0.0) {
            const ::godot::Vector2 cover_size = inner.size * ase::math::sqrt(static_cast<float>(cover));
            const ::godot::Rect2 cover_rect(inner.get_center() - cover_size * GODOT_HALF, cover_size);
            node_->draw_rect(cover_rect, cover_color(*patch));
            if (patch->condition == GODOT_COND_DEAD) {
                const ::godot::Vector2 end = cover_rect.get_end();
                node_->draw_line(cover_rect.position, end, ::godot::Color::hex(GODOT_COLOR_SOIL),
                                 GODOT_BORDER_WIDTH);
                node_->draw_line(::godot::Vector2(cover_rect.position.x, end.y),
                                 ::godot::Vector2(end.x, cover_rect.position.y),
                                 ::godot::Color::hex(GODOT_COLOR_SOIL), GODOT_BORDER_WIDTH);
            }
        }
        if (flat && patch->stage == GODOT_STAGE_SEED) {
            // Sown soil shows its seeds while nothing covers it yet.
            const float step_x = inner.size.x / static_cast<float>(GODOT_SEED_DOTS + 1);
            const float step_y = inner.size.y / static_cast<float>(GODOT_SEED_DOTS + 1);
            for (int32_t row = 1; row <= GODOT_SEED_DOTS; ++row) {
                for (int32_t column = 1; column <= GODOT_SEED_DOTS; ++column) {
                    node_->draw_circle(::godot::Vector2(inner.position.x + step_x * static_cast<float>(column),
                                                        inner.position.y + step_y * static_cast<float>(row)),
                                       GODOT_SEED_DOT_RADIUS, ::godot::Color::hex(GODOT_COLOR_SEED));
                }
            }
        }
        // The soil water as a bar along the bottom edge: its share of the field capacity.
        const ::godot::Rect2 track(inner.position.x, inner.get_end().y - GODOT_BAR_HEIGHT, inner.size.x,
                                   GODOT_BAR_HEIGHT);
        node_->draw_rect(track, ::godot::Color::hex(GODOT_COLOR_WATER_TRACK));
        const double wet = share(patch->soil_rel);
        if (wet > 0.0) {
            node_->draw_rect(::godot::Rect2(track.position,
                                            ::godot::Vector2(track.size.x * static_cast<float>(wet), track.size.y)),
                             ::godot::Color::hex(GODOT_COLOR_WATER));
        }
        node_->draw_string(view_font(), rect.position + ::godot::Vector2(GODOT_PATCH_INSET, GODOT_LABEL_OFFSET),
                           patch_text(*patch), ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, GODOT_FONT_SMALL,
                           ::godot::Color::hex(GODOT_COLOR_TEXT));
    }
    node_->draw_rect(rect, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM), false, GODOT_BORDER_WIDTH);
    if (place == selected_place_) {
        node_->draw_rect(rect.grow(GODOT_FRAME_WIDTH * GODOT_HALF), ::godot::Color::hex(GODOT_COLOR_SELECTION),
                         false, GODOT_FRAME_WIDTH);
    }
}

void VivariumView::draw_button(const ::godot::Rect2& rect, const ::godot::String& label, bool enabled) {
    const ::godot::Ref<::godot::Font> font = view_font();
    node_->draw_rect(rect, ::godot::Color::hex(enabled ? GODOT_COLOR_BUTTON : GODOT_COLOR_BUTTON_OFF));
    node_->draw_rect(rect, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM), false, GODOT_BORDER_WIDTH);
    const float baseline = rect.get_center().y +
                           (font->get_ascent(GODOT_FONT_BODY) - font->get_descent(GODOT_FONT_BODY)) * GODOT_HALF;
    node_->draw_string(font, ::godot::Vector2(rect.position.x, baseline), label,
                       ::godot::HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, GODOT_FONT_BODY,
                       ::godot::Color::hex(enabled ? GODOT_COLOR_TEXT : GODOT_COLOR_TEXT_DIM));
}

void VivariumView::draw_info() {
    // Each line in the band of its baseline (load_bands holds the three info lines in one).
    const ::godot::Ref<::godot::Font> font = view_font();
    ::godot::Vector2 at = layout_.info_at;
    const VivariumPatchView* patch =
        selected_place_ == GODOT_NO_PLACE ? nullptr : resources_.get_patch_at(selected_place_);
    use_band(at.y);
    node_->draw_string(font, at, texts_.selection + ": " + (patch == nullptr ? texts_.selection_none : patch_text(*patch)),
                       ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, GODOT_FONT_BODY, ::godot::Color::hex(GODOT_COLOR_TEXT));
    at.y += layout_.info_line;
    if (patch != nullptr) {
        use_band(at.y);
        node_->draw_string(font, at,
                           texts_.coverage + " " +
                               ::godot::String::num(patch->coverage * GODOT_PERCENT, GODOT_TEXT_DECIMALS) + " %" +
                               texts_.separator + texts_.soil + " " +
                               ::godot::String::num(patch->soil_mm, GODOT_TEXT_DECIMALS) + " mm (" +
                               ::godot::String::num(patch->soil_rel * GODOT_PERCENT, GODOT_TEXT_DECIMALS) + " %)",
                           ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, GODOT_FONT_BODY,
                           ::godot::Color::hex(GODOT_COLOR_TEXT));
    }
    at.y += layout_.info_line;
    const VivariumClockView& clock = resources_.get_clock();
    ::godot::String time_line = texts_.simulation + " " +
                                ::godot::String::num(resources_.simulation_seconds(), GODOT_TEXT_DECIMALS) + " s" +
                                texts_.separator + ::godot::String::num_uint64(resources_.tick_count()) + " " +
                                texts_.ticks;
    if (clock.known) {
        time_line = texts_.day + " " + ::godot::String::num(clock.day, 0) + texts_.separator + texts_.hour + " " +
                    ::godot::String::num(clock.hour, GODOT_TEXT_DECIMALS) + texts_.separator + time_line;
    }
    use_band(at.y);
    node_->draw_string(font, at, time_line, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, GODOT_FONT_BODY,
                       ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));
    use_band(layout_.note_at.y);
    node_->draw_string(font, layout_.note_at, texts_.restart_note, ::godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f,
                       GODOT_FONT_SMALL, ::godot::Color::hex(GODOT_COLOR_TEXT_DIM));
}

void VivariumView::draw_failure(const ::godot::Rect2& panel) {
    // The error picture instead of an empty surface: where it broke and with which status. The
    // caller hands the panel in the coordinates of the transform it set.
    node_->draw_rect(panel, ::godot::Color::hex(GODOT_COLOR_ERROR));
    node_->draw_multiline_string(view_font(),
                                 panel.position + ::godot::Vector2(GODOT_PATCH_INSET, GODOT_LABEL_OFFSET),
                                 failure_text_, ::godot::HORIZONTAL_ALIGNMENT_LEFT,
                                 panel.size.x - GODOT_PATCH_INSET - GODOT_PATCH_INSET, GODOT_FONT_BODY,
                                 GODOT_ERROR_LINES, ::godot::Color::hex(GODOT_COLOR_TEXT));
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD HAND SLOT
 *
 * @file        godot_hud_hand_slot.cpp
 * @brief       The hand at the foot of the view: the thing in it, its name, what to do with it and one reading
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_hand_slot.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.01.00007
 */

#include <ase/adp/godot/godot_hud_hand_slot.hpp>
#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_vivarium_watch.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>

namespace ase::adp::godot {

namespace {

::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

::godot::Color faded(const ::godot::Color& color, double alpha) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * alpha);
    return out;
}

// The CPU time of the drawing thread since `mark`, moving `mark` to now (P0692).
uint64_t since(uint64_t& mark) {
    const uint64_t now = thread_cpu_usec();
    const uint64_t spent = now - mark;
    mark = now;
    return spent;
}

}  // namespace

bool HandSlot::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    looks_.clear();
    sprites_.clear();
    place_ = HandThingPlace();
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_hand_x", &hand_x_},
             {"hud_hand_y", &hand_y_},
             {"hud_hand_radius", &radius_},
             {"hud_hand_fill_alpha", &fill_alpha_},
             {"hud_hand_empty_fill_alpha", &empty_fill_alpha_},
             {"hud_hand_art", &art_},
             {"hud_hand_box", &box_},
             {"hud_hand_hit_radius", &hit_radius_},
             {"hud_hand_line", &line_},
             {"hud_hand_hot_line", &hot_line_},
             {"hud_hand_empty_line", &empty_line_},
             {"hud_hand_empty_dash", &dash_},
             {"hud_hand_empty_gap", &dash_gap_},
             {"hud_hand_empty_glyph_size", &empty_glyph_size_},
             {"hud_hand_empty_label_dy", &empty_label_dy_},
             {"hud_hand_empty_label_size", &empty_label_size_},
             {"hud_hand_hint_y", &hint_y_},
             {"hud_hand_hint_size", &hint_size_},
             {"hud_hand_count_dx", &count_dx_},
             {"hud_hand_count_size", &count_size_},
             {"hud_hand_divider_y", &divider_y_},
             {"hud_hand_divider_left_from", &left_from_},
             {"hud_hand_divider_left_to", &left_to_},
             {"hud_hand_divider_right_from", &right_from_},
             {"hud_hand_divider_right_to", &right_to_},
             {"hud_hand_divider_line", &divider_line_},
             {"hud_hand_title_x", &title_x_},
             {"hud_hand_title_y", &title_y_},
             {"hud_hand_title_size", &title_size_},
             {"hud_hand_hint_line_y", &hint_line_y_},
             {"hud_hand_hint_line_size", &hint_line_size_},
             {"hud_hand_case_y", &case_y_},
             {"hud_hand_case_size", &case_size_},
             {"hud_hand_reading_x", &reading_x_},
             {"hud_hand_reading_label_y", &label_y_},
             {"hud_hand_reading_label_size", &label_size_},
             {"hud_hand_reading_value_y", &value_y_},
             {"hud_hand_reading_value_size", &value_size_},
             {"hud_hand_put_away_x", &stow_x_},
             {"hud_hand_put_away_y", &stow_y_},
             {"hud_hand_put_away_size", &stow_size_},
             {"hud_hand_put_away_label_y", &stow_label_y_},
             {"hud_hand_put_away_label_size", &stow_label_size_},
             {"hud_hand_put_away_hit_x", &stow_hit_x_},
             {"hud_hand_put_away_hit_y", &stow_hit_y_},
             {"hud_hand_put_away_hit_w", &stow_hit_w_},
             {"hud_hand_put_away_hit_h", &stow_hit_h_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_},
                                                                       {"OVERLAY_HAND_EMPTY", &empty_},
                                                                       {"DOMAIN_LIFE", &hot_},
                                                                       {"DOMAIN_ALERT", &refuse_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"hand_free", &hand_free_},
                                                                             {"hint_rest", &hint_rest_},
                                                                             {"stow", &stow_},
                                                                             {"stack_count", &stack_count_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    if (!bind_word_icon(theme, "hand_glyph", hand_icon_, failed) ||
        !bind_word_icon(theme, "stow_glyph", stow_icon_, failed) || !title_text_.bind_title(theme, failed) ||
        !hint_text_.bind_label(theme, failed) || !label_text_.bind_label(theme, failed) ||
        !stow_text_.bind_label(theme, failed) || !free_text_.bind_label(theme, failed) ||
        !rest_text_.bind_label(theme, failed) || !count_text_.bind_data(theme, failed) ||
        !case_text_.bind_data(theme, failed) || !value_text_.bind_data(theme, failed) || !mode_.bind(theme, failed)) {
        return false;
    }
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_words_ = levels[0];
    level_detail_ = levels[2];
    return true;
}

const HandItemLook* HandSlot::look(const std::string& id, std::string& failed) {
    for (const std::pair<std::string, HandItemLook>& entry : looks_) {
        if (entry.first == id) {
            return &entry.second;
        }
    }
    HandItemLook made;
    const std::string stem = "hud_item_" + id;
    ::godot::String text;
    if (!theme_->token_text(stem + "_tone", text)) {
        failed = stem + "_tone";
        return nullptr;
    }
    const std::string tone = text.utf8().get_data();
    if (!theme_->tone(tone, made.tone)) {
        failed = "DOMAIN_" + tone;
        return nullptr;
    }
    if (!theme_->token_text(stem + "_hold", text)) {
        failed = stem + "_hold";
        return nullptr;
    }
    const std::string hold = text.utf8().get_data();
    if (hold != GODOT_HUD_ITEM_HOLD_NONE && !theme_->words(hold, made.hold)) {
        failed = "texts.hud." + hold;
        return nullptr;
    }
    const std::string name = std::string(GODOT_HUD_WORD_ITEM_HEAD) + id + GODOT_HUD_WORD_ITEM_TAIL;
    const std::string case_key = std::string(GODOT_HUD_WORD_ITEM_HEAD) + id + GODOT_HUD_WORD_ITEM_CASE;
    if (!theme_->words(name, made.name)) {
        failed = "texts.hud." + name;
        return nullptr;
    }
    if (!theme_->words(case_key, made.case_words)) {
        failed = "texts.hud." + case_key;
        return nullptr;
    }
    looks_.emplace_back(id, made);
    return &looks_.back().second;
}

const UiItemSprite* HandSlot::sprite(const std::string& art, std::string& failed) {
    for (const std::pair<std::string, UiItemSprite>& entry : sprites_) {
        if (entry.first == art) {
            return &entry.second;
        }
    }
    UiItemSprite drawing;
    if (theme_ == nullptr || !drawing.bind(*theme_, failed) || !drawing.set(art, failed)) {
        return nullptr;
    }
    sprites_.emplace_back(art, drawing);
    return &sprites_.back().second;
}

bool HandSlot::say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
                   const std::vector<HudRun>& runs, double x, double y, double size, double anchor,
                   const ::godot::Color& color, std::string& failed) {
    if (!block.set(runs, failed)) {
        return false;
    }
    UiTextPlace place;
    place.at = at(x, y);
    place.size = size;
    place.anchor = anchor;
    place.color = color;
    place.opacity = 1.0;
    block.draw(canvas, frame, place);
    cost_.shape_usec += block.shape_usec();
    canvas.draw_set_transform_matrix(frame);
    return true;
}

bool HandSlot::with_thing(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                          const HandThing& thing, std::string& failed) {
    uint64_t mark = thread_cpu_usec();
    const HandItemLook* item = look(thing.id, failed);
    const UiItemSprite* drawing = sprite(thing.art, failed);
    if (item == nullptr || drawing == nullptr) {
        return false;
    }
    cost_.look_usec = since(mark);
    const ::godot::Vector2 hand = at(hand_x_, hand_y_);
    const ::godot::PackedVector2Array corners = hud_hex_points(hand, radius_);
    ::godot::PackedVector2Array outline = corners;
    outline.push_back(corners[0]);
    canvas.draw_colored_polygon(corners, faded(item->tone, fill_alpha_));
    // hand(): stroke hc||c at 1.4 either way; only the empty hand thickens to 1.3 when it is a target.
    canvas.draw_polyline(outline, now.hot ? (now.hot_bad ? refuse_ : hot_) : item->tone,
                         static_cast<float>(line_), true);
    UiItemSpritePlace fit;
    fit.centre = hand;
    fit.box = art_;
    fit.level = thing.level;  // the bridge's level: the water from o.fill, the sap above hud_item_sap_from
    fit.opacity = 1.0;
    drawing->draw(canvas, frame, fit);
    canvas.draw_set_transform_matrix(frame);

    const double side = ::ase::math::max(drawing->view_width(), drawing->view_height());
    place_.key = thing.key;
    place_.k = side > 0.0 ? box_ / side : 0.0;
    place_.box = ::godot::Rect2(static_cast<float>(hand_x_ - box_ / 2.0), static_cast<float>(hand_y_ - box_ / 2.0),
                                static_cast<float>(box_), static_cast<float>(box_));
    pours_ = now.mode == GODOT_HUD_MODE_WATER;
    cost_.art_usec = since(mark);

    std::vector<HudRun> runs;
    if (thing.stack && thing.count > 1) {
        runs = stack_count_;
        // Named for the whole block, as in HandMode::water: a list built inside the || chain dies at its end.
        const std::vector<HudPortValue> count_values{HudPortValue{"n", hud_whole(thing.count)}};
        if (!fill_words(runs, count_values, failed) ||
            !say(canvas, frame, count_text_, runs, hand_x_ + count_dx_, hand_y_ + count_dx_, count_size_,
                 GODOT_TEXT_ANCHOR_START, item->tone, failed)) {
            return false;
        }
    }

    // The stow button.
    stow_icon_.draw(canvas, frame, at(stow_x_, stow_y_), stow_size_, soft_, 1.0);
    canvas.draw_set_transform_matrix(frame);
    if (!say(canvas, frame, stow_text_, stow_, stow_x_, stow_label_y_, stow_label_size_, GODOT_TEXT_ANCHOR_MIDDLE,
             faint_, failed)) {
        return false;
    }
    place_.stow = ::godot::Rect2(static_cast<float>(stow_hit_x_), static_cast<float>(stow_hit_y_),
                                 static_cast<float>(stow_hit_w_), static_cast<float>(stow_hit_h_));
    cost_.stow_usec = since(mark);

    // What the mode adds, and the lines beside the hand.
    HandLines lines;
    if (!mode_.draw(canvas, frame, now, thing, *item, lines, failed)) {
        return false;
    }
    cost_.mode_usec = since(mark);
    for (const std::pair<std::string, UiHexChipPlace>& chip : mode_.verb_places()) {
        // The square round the hexagon the mode drew: the probe taps its centre, which hit() answers.
        const float r = static_cast<float>(chip.second.radius);
        place_.verbs.emplace_back(chip.first, ::godot::Rect2(chip.second.centre.x - r, chip.second.centre.y - r,
                                                             r * 2.0f, r * 2.0f));
    }
    since(mark);  // the verb squares above belong to no line: each line below counts from here
    if (!say(canvas, frame, title_text_, item->name, title_x_, title_y_, title_size_, GODOT_TEXT_ANCHOR_START,
             item->tone, failed)) {
        return false;
    }
    cost_.title_usec = since(mark);
    if (!say(canvas, frame, hint_text_, lines.hint, title_x_, hint_line_y_, hint_line_size_, GODOT_TEXT_ANCHOR_START,
             soft_, failed)) {
        return false;
    }
    cost_.hint_usec = since(mark);
    if (now.level >= level_detail_ && !lines.case_words.empty() &&
        !say(canvas, frame, case_text_, lines.case_words, title_x_, case_y_, case_size_, GODOT_TEXT_ANCHOR_START,
             faint_, failed)) {
        return false;
    }
    cost_.case_usec = since(mark);
    if (lines.label.empty()) {
        return true;
    }
    if (!say(canvas, frame, label_text_, lines.label, reading_x_, label_y_, label_size_, GODOT_TEXT_ANCHOR_END, soft_,
             failed)) {
        return false;
    }
    const uint64_t label_end = thread_cpu_usec();
    const bool read = lines.value.empty() || say(canvas, frame, value_text_, lines.value, reading_x_, value_y_,
                                                 value_size_, GODOT_TEXT_ANCHOR_END, item->tone, failed);
    cost_.value_usec = thread_cpu_usec() - label_end;
    cost_.reading_usec = since(mark);
    return read;
}

bool HandSlot::without_thing(UiPen& canvas, const ::godot::Transform2D& frame,
                             const HandSlotFrame& now, std::string& failed) {
    const ::godot::Vector2 hand = at(hand_x_, hand_y_);
    const ::godot::PackedVector2Array corners = hud_hex_points(hand, radius_);
    ::godot::PackedVector2Array outline = corners;
    outline.push_back(corners[0]);
    canvas.draw_colored_polygon(corners, faded(empty_, empty_fill_alpha_));
    const ::godot::Color ink = now.hot ? (now.hot_bad ? refuse_ : hot_) : faint_;
    if (now.hot) {
        canvas.draw_polyline(outline, ink, static_cast<float>(hot_line_), true);
    } else {
        svg_dash_points(canvas, outline, ink, empty_line_, dash_, dash_gap_);
    }
    hand_icon_.draw(canvas, frame, hand, empty_glyph_size_, ink, 1.0);
    canvas.draw_set_transform_matrix(frame);
    if (!say(canvas, frame, free_text_, hand_free_, hand_x_, hand_y_ + empty_label_dy_, empty_label_size_,
             GODOT_TEXT_ANCHOR_MIDDLE, faint_, failed)) {
        return false;
    }
    return now.mode != GODOT_HUD_MODE_REST || now.level != level_words_ || now.toast_posted ||
           say(canvas, frame, rest_text_, hint_rest_, hand_x_, hint_y_, hint_size_, GODOT_TEXT_ANCHOR_MIDDLE, faint_,
               failed);
}

bool HandSlot::draw(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                    std::string& failed) {
    place_ = HandThingPlace();  // the thing and its stow button: nothing drawn yet in this frame
    pours_ = false;
    cost_ = HandSlotCost();
    canvas.draw_set_transform_matrix(frame);
    for (const std::pair<double, double>& span :
         std::initializer_list<std::pair<double, double>>{{left_from_, left_to_}, {right_from_, right_to_}}) {
        canvas.draw_line(at(span.first, divider_y_), at(span.second, divider_y_), faint_,
                         static_cast<float>(divider_line_));
    }
    // Every line of the hand is readied at its raster before it first shows: the reading's value appears with the
    // first act, and its first draw built the face's raster size in that frame, 71.3 ms (P0692, run p0692-split),
    // and rastered the letters of the label and value, 40.1 ms (run p0692-warm). The glyphs come a few per frame,
    // one budget for every line of the hand, so readying spreads over the frames of the rest before the act.
    // THE ORDER IS THE ORDER OF NEED: the lines the act first shows come first (the reading's value and label, the
    // name, the instruction), the lines of the rest last, as they were drawn before and are ready by their draw.
    // Last in the list the value was never reached before the act (run p0692-budget: value 68.9 ms, of it shaping
    // 34.5 ms, in the act frame, while readying cost at most 1.2 ms a frame).
    int64_t budget = UiTextBlock::WARM_GLYPHS_PER_FRAME;
    const uint64_t warm_start = thread_cpu_usec();
    for (const std::pair<UiTextBlock*, double>& line : std::initializer_list<std::pair<UiTextBlock*, double>>{
             {&value_text_, value_size_},
             {&label_text_, label_size_},
             {&title_text_, title_size_},
             {&hint_text_, hint_line_size_},
             {&case_text_, case_size_},
             {&count_text_, count_size_},
             {&stow_text_, stow_label_size_},
             {&free_text_, empty_label_size_},
             {&rest_text_, hint_size_}}) {
        line.first->warm(canvas, frame, line.second, budget);
    }
    mode_.warm(canvas, frame, budget);
    cost_.warm_usec = thread_cpu_usec() - warm_start;
    const bool held = now.thing != nullptr && !now.dragged;
    uint64_t mark = thread_cpu_usec();
    const bool drawn = held ? with_thing(canvas, frame, now, *now.thing, failed)
                            : without_thing(canvas, frame, now, failed);
    if (held) {
        // The lines beside the hand end with_thing on several returns; their share is the rest of its time.
        const uint64_t whole = since(mark);
        const uint64_t parts = cost_.look_usec + cost_.art_usec + cost_.stow_usec + cost_.mode_usec;
        cost_.text_usec = whole > parts ? whole - parts : 0;
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return drawn;
}

std::string HandSlot::hit(const ::godot::Vector2& point, std::string& key) const {
    key.clear();
    if (place_.key.empty()) {
        return std::string();
    }
    const std::string verb = mode_.hit(point, key);
    if (!verb.empty()) {
        return verb;
    }
    if (place_.stow.has_point(point)) {
        return GODOT_HUD_ACT_STOW;
    }
    if (static_cast<double>(point.distance_to(at(hand_x_, hand_y_))) < hit_radius_) {
        if (pours_) {
            return GODOT_HUD_ACT_POUR;
        }
        key = place_.key;
        return GODOT_HUD_ACT_ITEM;
    }
    return std::string();
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD FOCUS CLUSTER
 *
 * @file        godot_hud_focus_cluster.cpp
 * @brief       The focus around the chosen plant: its ring, the head, the spine rows and the stage forecast
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_focus_cluster.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_focus_cluster.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool FocusCluster::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_focus_column_dx", &column_dx_},
             {"hud_focus_leader_knee", &leader_knee_},
             {"hud_focus_leader_top", &leader_top_},
             {"hud_focus_leader_delay", &leader_delay_},
             {"hud_focus_title_dx", &title_dx_},
             {"hud_focus_title_y", &title_y_},
             {"hud_focus_title_size", &title_size_},
             {"hud_focus_title_delay", &title_delay_},
             {"hud_focus_subtitle_y", &subtitle_y_},
             {"hud_focus_subtitle_size", &subtitle_size_},
             {"hud_focus_subtitle_delay", &subtitle_delay_},
             {"hud_focus_next_dy", &next_dy_},
             {"hud_focus_next_from_dx", &next_from_dx_},
             {"hud_focus_next_to_dx", &next_to_dx_},
             {"hud_focus_next_delay", &next_delay_},
             {"hud_focus_next_label_dx", &next_label_dx_},
             {"hud_focus_next_label_dy", &next_label_dy_},
             {"hud_focus_next_label_size", &next_label_size_},
             {"hud_focus_next_label_delay", &next_label_delay_},
             {"hud_focus_next_value_dy", &next_value_dy_},
             {"hud_focus_next_value_size", &next_value_size_},
             {"hud_focus_next_value_delay", &next_value_delay_},
             {"hud_focus_soil_dry_below", &soil_dry_},
             {"hud_focus_soil_moist_below", &soil_moist_},
             {"hud_focus_cover_few_below", &cover_few_},
             {"hud_focus_loose_shown_above", &loose_shown_},
             {"hud_focus_decimals", &decimals_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK", &ink_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"DOMAIN_WATER", &water_},
                                                                       {"DOMAIN_LIFE", &life_},
                                                                       {"DOMAIN_GROUND", &ground_},
                                                                       {"DOMAIN_FLOW", &flow_},
                                                                       {"DOMAIN_ALERT", &alert_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    std::vector<HudRun> species;
    std::vector<HudRun> next_stage;
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {"focus_share", &share_words_},
             {"focus_soil_full", &soil_full_words_},
             {"focus_loose_share", &loose_share_words_},
             {"focus_sap_fill", &sap_fill_words_},
             {"next_in_days", &next_days_words_},
             {"next_soon", &next_soon_words_},
             {"selection_of", &selection_words_},
             {"species", &species},
             {"next_stage", &next_stage}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }

    // One word per stage the view knows, never a count of this unit.
    stages_.clear();
    for (uint8_t stage = 0; stage < GODOT_STAGE_COUNT; ++stage) {
        const std::string key = "stages." + std::to_string(static_cast<uint32_t>(stage));
        std::vector<HudRun> word;
        if (!theme.words(key, word)) {
            failed = "texts.hud." + key;
            return false;
        }
        stages_.push_back(word);
    }

    // The thresholds of perception are the spec's levels, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_bars_ = levels[1];
    level_numbers_ = levels[2];

    return ring_.bind(theme, failed) && leader_.bind(theme, failed) && spine_.bind(theme, failed) &&
           title_.bind_title(theme, failed) && subtitle_.bind_label(theme, failed) &&
           next_label_.bind_label(theme, failed) && next_value_.bind_label(theme, failed) &&
           title_.set(species, failed) && next_label_.set(next_stage, failed);
}

bool FocusCluster::filled(const std::vector<HudRun>& runs, const std::vector<HudPortValue>& values,
                          std::string& out, std::string& failed) {
    std::vector<HudRun> copy = runs;
    return fill_words(copy, values, failed) && word_text(copy, out, failed);
}

std::string FocusCluster::percent(double share) {
    return hud_percent(share);
}

bool FocusCluster::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                        const FocusClusterFrame& now, std::string& failed) {
    if (now.condition >= GODOT_COND_COUNT) {
        failed = "FocusCluster: condition code " + std::to_string(static_cast<uint32_t>(now.condition)) +
                 " outside GODOT_COND_*";
        return false;
    }
    FocusRingFrame ring;
    ring.trunk = now.trunk;
    ring.height = now.height;
    ring.level = now.level;
    ring.stage = now.stage;
    ring.dead = now.condition == GODOT_COND_DEAD;
    ring.soil_rel = now.soil_rel;
    ring.growth_share = now.growth_share;
    ring.elapsed = now.elapsed;
    ring.still = now.still;

    std::vector<UiSpineRow> lines;
    if (!ring_.draw(canvas, frame, ring, failed) || !draw_head(canvas, frame, now, failed) ||
        !rows(now, lines, failed) || !marks(now, lines, failed) || !spine_.set(lines, failed)) {
        return false;
    }
    spine_.draw(canvas, frame, ring_.centre().x + column_dx_, now.elapsed, now.still, now.debug);
    return draw_next(canvas, frame, now, failed);
}

bool FocusCluster::draw_head(UiPen& canvas, const ::godot::Transform2D& frame,
                             const FocusClusterFrame& now, std::string& failed) {
    // L([[TX+R*k, ry-R*k], [SX, ry-R*k-(SX-TX-R*k)], [SX, top]]): forty-five degrees up, then straight up.
    const ::godot::Vector2& centre = ring_.centre();
    const double column_x = centre.x + column_dx_;
    const double knee = ring_.radius() * leader_knee_;
    const double start_x = centre.x + knee;
    const double start_y = centre.y - knee;
    ::godot::PackedVector2Array leader;
    leader.push_back(::godot::Vector2(static_cast<float>(start_x), static_cast<float>(start_y)));
    leader.push_back(
        ::godot::Vector2(static_cast<float>(column_x), static_cast<float>(start_y - (column_x - start_x))));
    leader.push_back(::godot::Vector2(static_cast<float>(column_x), static_cast<float>(leader_top_)));
    canvas.draw_set_transform_matrix(frame);
    leader_.draw(canvas, leader, soft_, leader_.width(), leader_.drawn(now.elapsed, leader_delay_, now.still));

    std::string area;
    if (!filled(selection_words_, {HudPortValue{"place", hud_whole(now.area)}},
                area, failed) ||
        !subtitle_.set(area, failed)) {
        return false;
    }
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(column_x + title_dx_), static_cast<float>(title_y_));
    text.size = title_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = ink_;
    text.opacity = 1.0;
    text.fade = true;
    text.delay = title_delay_;
    text.elapsed = now.elapsed;
    text.still = now.still;
    title_.draw(canvas, frame, text);
    UiTextPlace line = text;
    line.at = ::godot::Vector2(static_cast<float>(column_x + title_dx_), static_cast<float>(subtitle_y_));
    line.size = subtitle_size_;
    line.color = soft_;
    line.delay = subtitle_delay_;
    subtitle_.draw(canvas, frame, line);
    return true;
}

bool FocusCluster::rows(const FocusClusterFrame& now, std::vector<UiSpineRow>& out, std::string& failed) const {
    out.clear();
    const bool bars = now.level >= level_bars_;
    const bool dead = now.condition == GODOT_COND_DEAD;
    const bool dry = now.soil_rel < soil_dry_;

    // ESTÁGIO: the stage's word, or the dash for a dead stand; the pips lit up to the stage.
    UiSpineRow stage;
    if (!plain_word(*theme_, "row_stage", stage.label, failed) ||
        !(dead ? plain_word(*theme_, "words.none", stage.value, failed)
               : word_text(stages_[now.stage], stage.value, failed)) ||
        !plain_word(*theme_, "focus_source.stage", stage.source, failed)) {
        return false;
    }
    stage.tone = life_;
    stage.pips = true;
    stage.pips_lit = dead ? 0 : static_cast<int32_t>(now.stage) + 1;
    out.push_back(stage);

    // ÁGUA NO SOLO: a word, the share with its bar, or the millimetres beside the share.
    UiSpineRow soil;
    if (!plain_word(*theme_, "row_soil", soil.label, failed) ||
        !plain_word(*theme_, "focus_source.soil", soil.source, failed)) {
        return false;
    }
    const std::string soil_mm = hud_rounded(now.soil_mm, static_cast<int32_t>(decimals_));
    const bool soil_set =
        now.level >= level_numbers_
            ? filled(soil_full_words_, {HudPortValue{"mm", soil_mm}, HudPortValue{"rel", percent(now.soil_rel)}},
                     soil.value, failed)
            : (bars ? filled(share_words_, {HudPortValue{"rel", percent(now.soil_rel)}}, soil.value, failed)
                    : plain_word(*theme_, dry ? "words.dry" : (now.soil_rel < soil_moist_ ? "words.moist" : "words.wet"),
                                 soil.value, failed));
    if (!soil_set) {
        return false;
    }
    soil.tone = dry ? alert_ : water_;
    soil.alert = dry;
    soil.bar = bars;
    soil.share = now.soil_rel;
    out.push_back(soil);

    // COBERTURA: few or good, or the share with its bar.
    UiSpineRow cover;
    if (!plain_word(*theme_, "row_cover", cover.label, failed) ||
        !plain_word(*theme_, "focus_source.cover", cover.source, failed) ||
        !(bars ? filled(share_words_, {HudPortValue{"rel", percent(now.coverage)}}, cover.value, failed)
               : plain_word(*theme_, now.coverage < cover_few_ ? "words.few" : "words.good", cover.value, failed))) {
        return false;
    }
    cover.tone = ground_;
    cover.bar = bars;
    cover.share = now.coverage;
    out.push_back(cover);
    return true;
}

bool FocusCluster::marks(const FocusClusterFrame& now, std::vector<UiSpineRow>& out, std::string& failed) const {
    const bool bars = now.level >= level_bars_;

    // SOLO: the loosened share above its threshold (the mockup's st.loose > .05), the word or the share.
    if (now.loose_share > loose_shown_) {
        UiSpineRow loose;
        if (!plain_word(*theme_, "row_loose", loose.label, failed) ||
            !plain_word(*theme_, "focus_source.loose", loose.source, failed)) {
            return false;
        }
        loose.tone = ground_;
        loose.bar = bars;
        loose.share = now.loose_share;
        const bool set = bars ? filled(loose_share_words_, {HudPortValue{"rel", percent(now.loose_share)}},
                                       loose.value, failed)
                              : plain_word(*theme_, "words.loose", loose.value, failed);
        if (!set) {
            return false;
        }
        out.push_back(loose);
    }

    // SEIVA: the vessel's fill while a tap runs (the mockup's st.tap), the word or litres over capacity.
    if (now.sap_running) {
        UiSpineRow sap;
        if (!plain_word(*theme_, "row_sap", sap.label, failed) ||
            !plain_word(*theme_, "focus_source.sap", sap.source, failed)) {
            return false;
        }
        const int decimals = static_cast<int>(decimals_);
        sap.tone = flow_;
        sap.bar = bars && now.sap_cap_l > 0.0;
        sap.share = sap.bar ? now.sap_l / now.sap_cap_l : 0.0;
        const bool set = bars ? filled(sap_fill_words_,
                                       {HudPortValue{"l", hud_rounded(now.sap_l, decimals)},
                                        HudPortValue{"cap", hud_rounded(now.sap_cap_l, decimals)}},
                                       sap.value, failed)
                              : plain_word(*theme_, "words.flowing", sap.value, failed);
        if (!set) {
            return false;
        }
        out.push_back(sap);
    }

    // CONDIÇÃO: the mark ase-plant set on the stand.
    if (now.condition != GODOT_COND_HEALTHY) {
        UiSpineRow condition;
        if (!plain_word(*theme_, "row_condition", condition.label, failed) ||
            !plain_word(*theme_, now.condition == GODOT_COND_DEAD ? "words.dead" : "words.wilted", condition.value,
                        failed) ||
            !plain_word(*theme_, "focus_source.condition", condition.source, failed)) {
            return false;
        }
        condition.tone = alert_;
        condition.alert = true;
        out.push_back(condition);
    }
    return true;
}

bool FocusCluster::draw_next(UiPen& canvas, const ::godot::Transform2D& frame,
                             const FocusClusterFrame& now, std::string& failed) {
    if (now.level < level_bars_ || now.condition == GODOT_COND_DEAD || !now.next_due) {
        return true;
    }
    // L([[TX+R*k, ry+R*k], [TX+from, fy], [TX+to, fy]]) under the ring.
    const ::godot::Vector2& centre = ring_.centre();
    const double knee = ring_.radius() * leader_knee_;
    const double line_y = centre.y + next_dy_;
    ::godot::PackedVector2Array leader;
    leader.push_back(::godot::Vector2(static_cast<float>(centre.x + knee), static_cast<float>(centre.y + knee)));
    leader.push_back(::godot::Vector2(static_cast<float>(centre.x + next_from_dx_), static_cast<float>(line_y)));
    leader.push_back(::godot::Vector2(static_cast<float>(centre.x + next_to_dx_), static_cast<float>(line_y)));
    canvas.draw_set_transform_matrix(frame);
    leader_.draw(canvas, leader, soft_, leader_.width(), leader_.drawn(now.elapsed, next_delay_, now.still));

    // PRÓXIMO ESTÁGIO: the next stage's word, with the days from levels[2] on (ceil, as the mockup's daysToNext).
    std::string value;
    const uint32_t next = static_cast<uint32_t>(now.stage) + 1u;
    if (next >= GODOT_STAGE_COUNT) {
        failed = "FocusCluster: flora.patch.v1 names a next stage after the last one";
        return false;
    }
    std::string word;
    if (!word_text(stages_[next], word, failed)) {
        return false;
    }
    const std::string days = hud_whole(static_cast<int64_t>(::godot::Math::ceil(now.next_days)));
    const bool set =
        now.level >= level_numbers_
            ? filled(next_days_words_, {HudPortValue{"stage", word}, HudPortValue{"days", days}}, value, failed)
            : filled(next_soon_words_, {HudPortValue{"stage", word}}, value, failed);
    if (!set) {
        return false;
    }
    if (!next_value_.set(value, failed)) {
        return false;
    }

    UiTextPlace label;
    label.at = ::godot::Vector2(static_cast<float>(centre.x + next_label_dx_),
                                static_cast<float>(line_y + next_label_dy_));
    label.size = next_label_size_;
    label.anchor = GODOT_TEXT_ANCHOR_START;
    label.color = soft_;
    label.opacity = 1.0;
    label.fade = true;
    label.delay = next_label_delay_;
    label.elapsed = now.elapsed;
    label.still = now.still;
    next_label_.draw(canvas, frame, label);
    UiTextPlace text = label;
    text.at = ::godot::Vector2(static_cast<float>(centre.x + next_label_dx_),
                               static_cast<float>(line_y + next_value_dy_));
    text.size = next_value_size_;
    text.color = ink_;
    text.delay = next_value_delay_;
    next_value_.draw(canvas, frame, text);
    return true;
}

}  // namespace ase::adp::godot

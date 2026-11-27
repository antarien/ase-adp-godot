/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW DATE ENTRY
 *
 * @file        godot_vivarium_date.cpp
 * @brief       VivariumView - the free date entry of the debug mode: any SURA date, ahead or back
 * @description Operator order of 2026-11-08: the debug mode jumps to ANY date, not only to the fixed targets of
 *              debug.jumps. The free date entry (debug.date) holds a SURA date of four parts - VECTUM, LUNBREX,
 *              TUMBRAE and the whole HORAX - and sends it through the date port. The fixed jump buttons stay
 *              beside it; the entry is the case every target reaches.
 *
 *              THE FIELDS show the clock record's date while the keeper set none, so after every jump they show
 *              the date it landed on - an eclipse the search found, a stage the birch reached, years ahead. A
 *              STEP moves one part by whole units: LUNBREX, TUMBRAE and HORAX turn round within ase-calendar's
 *              months of a VECTUM and days of a LUNBREX and ase-time's hours of a day, the VECTUM stops at the
 *              first of the run. None of these bounds is this file's: they come from the star through the clock
 *              record (FloraHostClkExptSystem files CAL_CONST_DAYS_PER_MONTH, CAL_CONST_MONTHS_PER_YEAR and
 *              ase-time's hour constants). NOW gives the fields back to the clock record. A TAP ON A FIELD picks
 *              its part and the KEYS type its value (operator order of 2026-11-12, register N0475); the pick, the
 *              keys and their loading stand in godot_vivarium_date_type.cpp, this unit routes the taps to them,
 *              draws the keys and refuses a go with a part a clear left below its first.
 *
 *              GO sends the date held through the date port (GodotHostResourceManager::seek_date): ase-calendar
 *              turns it into the game hour with its own constants (CalendarDatSeekCalcSystem), ase-time judges it
 *              against its clock and sets the clock there when it lies ahead (TimeClkSeekCalcSystem), and every
 *              module computes its state at the new time as on a start of the app. The view computes no calendar
 *              of its own and compares no clock: the calendar is never written, the pace stays.
 *
 *              A JUMP IS ONE WRITE (operator order of 2026-11-09, CAUSA_ASE_TIME "The Seek Contract"): the target
 *              date goes once through the date port into ase-time's clock (VivariumView::send_date), carrying the
 *              next request number of the run (VivariumView::take_seq). ase-time answers that number with its
 *              verdict in the clock record - AHEAD, AT, BEHIND or REFUSED (VivariumClockView::seek_order) - and
 *              the jump ends on it (godot_vivarium_jump.cpp, VivariumView::follow_debug): AHEAD and AT land after
 *              debug.settle_s of the run's own pace, BEHIND rebuilds the host with this run's seeds and pace and
 *              sends the date again from its start clock (godot_vivarium_rewind.cpp), REFUSED ends. The shoot of
 *              ase-phytomer counts every day since its state in one cycle (PhytomerClkCalcSystem), so a jump over
 *              years needs no date before the target (ARCH_ASE_KERNEL_EMBEDDED, register B0100).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_046
 * @category    time/period/cycle
 * @usecase     usecase:101120 - clock the growth cycle
 * @usecase     usecase:100390 - return to the garden
 * @actor       Growth Clock Controller (GROWCLOCON), Plant Keeper (PLNTKEEPER)
 * @created     2026-11-08
 * @modified    2026-11-18
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/hud/log/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/array.hpp>

#include <cstdint>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

namespace {

// The GODOT_DATE_PART_* a part word of debug.date names, GODOT_DATE_PART_NONE for a word of none.
uint8_t date_part_of(const ::godot::String& word) {
    if (word == GODOT_DATE_KEY_YEAR) {
        return GODOT_DATE_PART_YEAR;
    }
    if (word == GODOT_DATE_KEY_MONTH) {
        return GODOT_DATE_PART_MONTH;
    }
    if (word == GODOT_DATE_KEY_DAY) {
        return GODOT_DATE_PART_DAY;
    }
    if (word == GODOT_DATE_KEY_HOUR) {
        return GODOT_DATE_PART_HOUR;
    }
    return GODOT_DATE_PART_NONE;
}

// The SURA date of the clock record as the entry holds it: VECTUM, LUNBREX, TUMBRAE and the whole HORAX.
void take_clock_date(double (&value)[GODOT_DATE_PARTS], const VivariumClockView& clock) {
    value[GODOT_DATE_PART_YEAR] = clock.year;
    value[GODOT_DATE_PART_MONTH] = clock.month;
    value[GODOT_DATE_PART_DAY] = clock.month_day;
    value[GODOT_DATE_PART_HOUR] = static_cast<double>(ase::math::floor(static_cast<float>(clock.hour)));
}

// A part moved by a step: within [first, first + count) it turns round; a count of 0 (the VECTUM) knows no
// end ahead and stops at first.
double turned(double moved, double first, double count) {
    if (!(count > 0.0)) {
        return moved < first ? first : moved;
    }
    const double offset = moved - first;
    const double rounds = static_cast<double>(ase::math::floor(static_cast<float>(offset / count)));
    return first + offset - rounds * count;
}

// The date of a jump as its note names it.
::godot::String date_words(const VivariumJump& jump) {
    return " year=" + ::godot::String::num(jump.target_year, 0) +
           " month=" + ::godot::String::num(jump.target_month, 0) +
           " month_day=" + ::godot::String::num(jump.target_month_day, 0) +
           " hour=" + ::godot::String::num(jump.target_hour, 0);
}

// The forms of a seek request as its note names them; a form of GODOT_SEEK_FORM_NONE is left out.
::godot::String form_words(double game_hour, double ahead_h, double day_hour) {
    ::godot::String words;
    if (game_hour != GODOT_SEEK_FORM_NONE) {
        words += " game_hour=" + ::godot::String::num(game_hour, GODOT_LOG_DECIMALS);
    }
    if (ahead_h != GODOT_SEEK_FORM_NONE) {
        words += " ahead_h=" + ::godot::String::num(ahead_h, GODOT_LOG_DECIMALS);
    }
    if (day_hour != GODOT_SEEK_FORM_NONE) {
        words += " day_hour=" + ::godot::String::num(day_hour, GODOT_LOG_DECIMALS);
    }
    return words;
}

}  // namespace

bool VivariumView::load_debug_date(const ::godot::Dictionary& debug) {
    // THE FREE DATE ENTRY: every field names one part of the SURA date, every step a part and the whole units
    // it moves it by, go and now a rectangle and a label each. The bounds of the parts are not this file's:
    // they come from the clock record at the step (ase-calendar's and ase-time's constants).
    ::godot::Dictionary date;
    if (!read_section(debug, "date", date)) {
        return fail_config("debug.date");
    }
    const ::godot::Variant fields_value = date.get("fields", ::godot::Variant());
    if (fields_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.date.fields");
    }
    const ::godot::Array fields = fields_value;
    for (int64_t i = 0; i < fields.size(); ++i) {
        if (fields[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.date.fields[]");
        }
        const ::godot::Dictionary entry = fields[i];
        VivariumDebugDateField field;
        ::godot::String part_word;
        if (!read_rect(entry, "rect", field.rect) || !read_text(entry, "label", field.label) ||
            !read_text(entry, "part", part_word)) {
            return fail_config("debug.date.fields[]");
        }
        field.part = date_part_of(part_word);
        if (field.part == GODOT_DATE_PART_NONE) {
            return fail_config("debug.date.fields[].part");
        }
        if (!in_one_band(layout_.bands, field.rect.position.y, field.rect.get_end().y)) {
            return fail_config("layout.bands: debug.date.fields[].rect");
        }
        debug_.date.fields.push_back(field);
    }
    const ::godot::Variant steps_value = date.get("steps", ::godot::Variant());
    if (steps_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.date.steps");
    }
    const ::godot::Array steps = steps_value;
    for (int64_t i = 0; i < steps.size(); ++i) {
        if (steps[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.date.steps[]");
        }
        const ::godot::Dictionary entry = steps[i];
        VivariumDebugDateStep step;
        ::godot::String part_word;
        if (!read_rect(entry, "rect", step.rect) || !read_text(entry, "label", step.label) ||
            !read_text(entry, "part", part_word)) {
            return fail_config("debug.date.steps[]");
        }
        step.part = date_part_of(part_word);
        if (step.part == GODOT_DATE_PART_NONE) {
            return fail_config("debug.date.steps[].part");
        }
        // A step moves by whole units and never by none.
        if (!read_number(entry, "delta", step.delta) || step.delta == 0.0 ||
            ase::math::floor(static_cast<float>(step.delta)) != static_cast<float>(step.delta)) {
            return fail_config("debug.date.steps[].delta");
        }
        if (!in_one_band(layout_.bands, step.rect.position.y, step.rect.get_end().y)) {
            return fail_config("layout.bands: debug.date.steps[].rect");
        }
        debug_.date.steps.push_back(step);
    }
    ::godot::Dictionary go;
    if (!read_section(date, "go", go) || !read_rect(go, "rect", debug_.date.go_rect) ||
        !read_text(go, "label", debug_.date.go_label)) {
        return fail_config("debug.date.go");
    }
    if (!in_one_band(layout_.bands, debug_.date.go_rect.position.y, debug_.date.go_rect.get_end().y)) {
        return fail_config("layout.bands: debug.date.go.rect");
    }
    ::godot::Dictionary now;
    if (!read_section(date, "now", now) || !read_rect(now, "rect", debug_.date.now_rect) ||
        !read_text(now, "label", debug_.date.now_label)) {
        return fail_config("debug.date.now");
    }
    if (!in_one_band(layout_.bands, debug_.date.now_rect.position.y, debug_.date.now_rect.get_end().y)) {
        return fail_config("layout.bands: debug.date.now.rect");
    }
    return load_debug_keys(date);  // the keys of the typed entry (godot_vivarium_date_type.cpp)
}

bool VivariumView::date_tap(const ::godot::Vector2& at) {
    // A step moves its part, go jumps to the date held, now gives the fields back to the clock record. A tap
    // on a field picks its part for the keys, a key types its digit into it, clear empties it; none of these
    // taps reaches the place frame below.
    for (const VivariumDebugDateStep& step : debug_.date.steps) {
        if (placed(step.rect).has_point(at)) {
            step_date(step);
            return true;
        }
    }
    if (placed(debug_.date.go_rect).has_point(at)) {
        ask_date();
        return true;
    }
    if (placed(debug_.date.now_rect).has_point(at)) {
        debug_.date.held = false;
        debug_.date.typing = GODOT_DATE_PART_NONE;
        note("debug date now result=ok");
        node_->queue_redraw();
        return true;
    }
    for (const VivariumDebugDateKey& key : debug_.date.keys) {
        if (placed(key.rect).has_point(at)) {
            type_date(&key);
            return true;
        }
    }
    if (placed(debug_.date.clear_rect).has_point(at)) {
        type_date(nullptr);
        return true;
    }
    for (const VivariumDebugDateField& field : debug_.date.fields) {
        if (placed(field.rect).has_point(at)) {
            pick_date(field.part);
            return true;
        }
    }
    return false;
}

void VivariumView::step_date(const VivariumDebugDateStep& step) {
    // A STEP takes the date the fields show when the keeper set none yet - the clock record's, so after a jump
    // the date it landed on - and moves its part within the bounds the clock record carries.
    const VivariumClockView& clock = resources_.get_clock();
    const double hours_per_day = hour_length_.hours_per_day;
    VivariumDebugDate& date = debug_.date;
    ::godot::String step_word = ::godot::String("debug date step part=") +
                                      ::godot::String::num_uint64(step.part) +
                                      " delta=" + ::godot::String::num(step.delta, 0);
    if (!clock.known || !clock.date_known || !(hours_per_day > 0.0)) {
        note(step_word + " result=unmeasured");
        return;
    }
    if (!date.held) {
        take_clock_date(date.value, clock);
        date.held = true;
    }
    const double first[GODOT_DATE_PARTS] = {GODOT_DATE_YEAR_FIRST, GODOT_DATE_FIRST, GODOT_DATE_FIRST,
                                            GODOT_DATE_HOUR_FIRST};
    const double count[GODOT_DATE_PARTS] = {0.0, clock.months_per_year, clock.days_per_month, hours_per_day};
    date.value[step.part] = turned(date.value[step.part] + step.delta, first[step.part], count[step.part]);
    note(step_word + " year=" + ::godot::String::num(date.value[GODOT_DATE_PART_YEAR], 0) +
         " month=" + ::godot::String::num(date.value[GODOT_DATE_PART_MONTH], 0) +
         " month_day=" + ::godot::String::num(date.value[GODOT_DATE_PART_DAY], 0) +
         " hour=" + ::godot::String::num(date.value[GODOT_DATE_PART_HOUR], 0) + " result=ok");
    node_->queue_redraw();
}

bool VivariumView::take_seq(VivariumJump& jump) {
    // THE REQUEST NUMBER: every seek and date request of this run carries the next number, and the numbers
    // only grow - across a rewind too, so a verdict of the host before the rewind never answers a request
    // after it. ase-time answers the newest number; the jump ends on the verdict for its own. A float slot
    // holds whole numbers exactly up to ase-time's TIM_CONST_SEEK_SEQ_MAX, and the first number is its
    // TIM_CONST_SEEK_SEQ_MIN, both read from the seek bounds record (VivariumClockView::seek_seq_min and
    // seek_seq_max); past the largest, or while that record has not come, the jump is refused, never left waiting.
    const VivariumClockView& clock = resources_.get_clock();
    if (!clock.seek_bounds_known) {
        return false;
    }
    const double next = ase::math::max(debug_.seq + 1.0, clock.seek_seq_min);
    if (next > clock.seek_seq_max) {
        return false;
    }
    debug_.seq = next;
    jump.seq = next;
    return true;
}

HostStatus VivariumView::send_date(const VivariumClockView& clock) {
    // THE ONE WRITE OF A DATE JUMP: the target date and its request number through the date port.
    // ase-calendar turns the date into the game hour with its own constants (CalendarDatSeekCalcSystem),
    // ase-time judges it against its clock and sets the clock there when it lies ahead
    // (TimeClkSeekCalcSystem), and every module computes its state at the new time in its own pass, as on a
    // start of the app at that date. A written request clears the settle stamp: the stamp is the clock's write,
    // set by follow_debug once ase-time's verdict first answers this request number, and the landing waits
    // debug.settle_s of the run after it - the run up to the write never spends the settle.
    VivariumJump& jump = debug_.jump;
    if (!take_seq(jump)) {
        note("debug date seek" + date_words(jump) + " seq_max=" + ::godot::String::num(clock.seek_seq_max, 0) +
             " result=" + (clock.seek_bounds_known ? "seq_spent" : "bounds_unknown"));
        return ase::kernel::HostStatusCapacity;
    }
    const HostStatus status = resources_.seek_date(jump.seq, jump.target_year, jump.target_month,
                                                   jump.target_month_day, jump.target_hour);
    note("debug date seek" + date_words(jump) + " seq=" + ::godot::String::num(jump.seq, 0) +
         " from_year=" + ::godot::String::num(clock.year, 0) +
         " from_hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS) +
         " result=" + GodotHostResourceManager::status_name(status));
    if (status == ase::kernel::HostStatusOk) {
        jump.sought_run_s = GODOT_HOUR_NONE;
    }
    return status;
}

HostStatus VivariumView::send_seek(VivariumJump& jump, double game_hour, double ahead_h, double day_hour) {
    // THE ONE WRITE OF EVERY OTHER JUMP: its target in the form its source names - the absolute game hour of
    // a stage, the hours ahead of a forecast, the hour of the day of a button - with its request number
    // through the seek port into ase-time's clock. ase-time lays the target and judges it; the view computes
    // no game hour of its own. From here every module reads the new time in its own schedule.
    const VivariumClockView& clock = resources_.get_clock();
    const ::godot::String forms = form_words(game_hour, ahead_h, day_hour);
    if (!take_seq(jump)) {
        note("debug jump seek" + forms + " seq_max=" + ::godot::String::num(clock.seek_seq_max, 0) +
             " result=" + (clock.seek_bounds_known ? "seq_spent" : "bounds_unknown"));
        return ase::kernel::HostStatusCapacity;
    }
    const HostStatus status = resources_.seek(jump.seq, game_hour, ahead_h, day_hour);
    note("debug jump seek" + forms + " seq=" + ::godot::String::num(jump.seq, 0) +
         " from_elapsed_s=" + ::godot::String::num(clock.elapsed_s, GODOT_LOG_DECIMALS) +
         " result=" + GodotHostResourceManager::status_name(status));
    return status;
}

void VivariumView::ask_date() {
    // GO: the date held - or the clock record's while the keeper set none - through the date port. The pace
    // stays; the jump runs like every other one behind the start screen until ase-time's verdict on it ends
    // it. The view judges no direction: AT lands at once, BEHIND rebuilds the host and sends the date again.
    const VivariumClockView& clock = resources_.get_clock();
    VivariumDebugDate& date = debug_.date;
    if (!clock.known || !clock.date_known) {
        note("debug date go result=unmeasured");
        return;
    }
    double target[GODOT_DATE_PARTS] = {date.value[GODOT_DATE_PART_YEAR], date.value[GODOT_DATE_PART_MONTH],
                                       date.value[GODOT_DATE_PART_DAY], date.value[GODOT_DATE_PART_HOUR]};
    if (!date.held) {
        take_clock_date(target, clock);
    }
    // A typed part emptied by clear stands below its first until a key fills it; such a date is refused here,
    // never sent for ase-calendar to refuse.
    const double first[GODOT_DATE_PARTS] = {GODOT_DATE_YEAR_FIRST, GODOT_DATE_FIRST, GODOT_DATE_FIRST,
                                            GODOT_DATE_HOUR_FIRST};
    for (uint32_t part = 0; part < GODOT_DATE_PARTS; ++part) {
        if (target[part] < first[part]) {
            note("debug date go part=" + ::godot::String::num_uint64(part) +
                 " value=" + ::godot::String::num(target[part], 0) + " result=out_of_range");
            return;
        }
    }
    date.typing = GODOT_DATE_PART_NONE;
    VivariumJump next;
    next.running = true;
    next.entry = true;
    next.ahead = true;
    next.target_year = target[GODOT_DATE_PART_YEAR];
    next.target_month = target[GODOT_DATE_PART_MONTH];
    next.target_month_day = target[GODOT_DATE_PART_DAY];
    next.target_hour = target[GODOT_DATE_PART_HOUR];
    next.asked_day = clock.day;
    debug_.jump = next;
    const HostStatus status = send_date(clock);
    note("debug date go" + date_words(next) + " from_year=" + ::godot::String::num(clock.year, 0) +
         " from_month=" + ::godot::String::num(clock.month, 0) +
         " from_month_day=" + ::godot::String::num(clock.month_day, 0) +
         " from_hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS) +
         " result=" + GodotHostResourceManager::status_name(status));
    date.held = false;
    if (status != ase::kernel::HostStatusOk) {
        debug_.jump = VivariumJump{};
        node_->queue_redraw();
        return;
    }
    raise_jump_screen();
}

bool VivariumView::shown_date(double (&shown)[GODOT_DATE_PARTS]) const {
    // The date the entry shows: the one held, or the clock record's while the keeper set none. The canvas of
    // the debug buttons and the HUD's debug layer read the same date from here.
    const VivariumClockView& clock = resources_.get_clock();
    const VivariumDebugDate& date = debug_.date;
    for (uint32_t part = 0; part < GODOT_DATE_PARTS; ++part) {
        shown[part] = date.value[part];
    }
    const bool known = date.held || (clock.known && clock.date_known);
    if (!date.held && known) {
        take_clock_date(shown, clock);
    }
    return known;
}

void VivariumView::draw_debug_date(float text_lift) {
    // The fields show the date held, or the clock record's while the keeper set none - drawn with the wider
    // outline once a date is held; now carries it while the fields follow the clock, go while its jump runs.
    const ::godot::Ref<::godot::Font> font = view_font();
    const VivariumDebugDate& date = debug_.date;
    double shown[GODOT_DATE_PARTS] = {};
    const bool known = shown_date(shown);
    const auto face = [&](const ::godot::Rect2& rect, const ::godot::String& label, bool wide) {
        use_band(rect.position.y);
        node_->draw_rect(rect, debug_.fill);
        node_->draw_rect(rect, debug_.text, false, wide ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        node_->draw_string(font, ::godot::Vector2(rect.position.x, rect.get_center().y + text_lift), label,
                           ::godot::HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, GODOT_FONT_SMALL, debug_.text);
    };
    for (const VivariumDebugDateField& field : date.fields) {
        const ::godot::String value = known ? ::godot::String::num(shown[field.part], 0) : ::godot::String("?");
        // The part the keys type into carries the cursor behind its value.
        const ::godot::String cursor = date.typing == field.part ? ::godot::String("_") : ::godot::String();
        face(field.rect, field.label + ::godot::String(" ") + value + cursor, date.held);
    }
    for (const VivariumDebugDateStep& step : date.steps) {
        face(step.rect, step.label, false);
    }
    for (const VivariumDebugDateKey& key : date.keys) {
        face(key.rect, key.label, false);
    }
    face(date.clear_rect, date.clear_label, false);
    face(date.now_rect, date.now_label, !date.held);
    face(date.go_rect, date.go_label, debug_.jump.running && debug_.jump.entry);
}

}  // namespace ase::adp::godot

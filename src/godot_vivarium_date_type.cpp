/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW TYPED DATE ENTRY
 *
 * @file        godot_vivarium_date_type.cpp
 * @brief       VivariumView - the typed date entry of the debug mode: a field picked, its value typed key by key
 * @description Operator order of 2026-11-12 (register N0475): a date is typed, never walked to. A LUNBREX lies up to
 *              CAL_CONST_MONTHS_PER_YEAR - 1 steps of one away on the month step of the free date entry
 *              (godot_vivarium_date.cpp); typed, it is a tap on the field and at most debug.date.digits keys.
 *
 *              THE PICK. A tap on a field (debug.date.fields, or the field cell of the HUD's debug layer) picks its
 *              part for the keys. The entry takes the date the fields show, as a step does, so every part not
 *              typed keeps the clock record's date - after a jump the date it landed on.
 *
 *              THE KEYS. debug.date.keys carry one digit each below debug.date.base. The first key after the pick
 *              replaces the value, every further key appends its digit, up to debug.date.digits digits. A value
 *              past the last LUNBREX, TUMBRAE or HORAX the clock record carries (ase-calendar's
 *              CAL_CONST_MONTHS_PER_YEAR and CAL_CONST_DAYS_PER_MONTH, ase-time's hours of a day, filed by
 *              FloraHostClkExptSystem) is refused with its key and the value stays; the VECTUM knows no end ahead.
 *              None of these bounds is this file's. CLEAR (debug.date.clear) empties the picked part; go refuses a
 *              date with a part below its first (VivariumView::ask_date), so an emptied LUNBREX or TUMBRAE is
 *              never sent.
 *
 *              Every pick and key writes a note line "debug date pick ..." or "debug date type ..." with its
 *              result, so a probe types a date and reads what the entry holds; the pick line carries the whole
 *              held date, so the probe learns where the keys start from the pick itself.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_046
 * @category    time/period/cycle
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/array.hpp>

#include <cstdint>

namespace ase::adp::godot {

bool VivariumView::load_debug_keys(const ::godot::Dictionary& date) {
    // THE TYPED ENTRY: the digits one part takes and the base they are typed in, both whole and above 0; every
    // key a whole digit below the base, at least one key, and clear - each rectangle inside one band.
    VivariumDebugDate& entry = debug_.date;
    if (!read_number(date, "digits", entry.digits) || !(entry.digits > 0.0) ||
        ase::math::floor(static_cast<float>(entry.digits)) != static_cast<float>(entry.digits)) {
        return fail_config("debug.date.digits");
    }
    if (!read_number(date, "base", entry.base) || !(entry.base > 0.0) ||
        ase::math::floor(static_cast<float>(entry.base)) != static_cast<float>(entry.base)) {
        return fail_config("debug.date.base");
    }
    const ::godot::Variant keys_value = date.get("keys", ::godot::Variant());
    if (keys_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.date.keys");
    }
    const ::godot::Array keys = keys_value;
    for (int64_t i = 0; i < keys.size(); ++i) {
        if (keys[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.date.keys[]");
        }
        const ::godot::Dictionary one = keys[i];
        VivariumDebugDateKey key;
        if (!read_rect(one, "rect", key.rect) || !read_text(one, "label", key.label) ||
            !read_number(one, "digit", key.digit)) {
            return fail_config("debug.date.keys[]");
        }
        if (key.digit < 0.0 || !(key.digit < entry.base) ||
            ase::math::floor(static_cast<float>(key.digit)) != static_cast<float>(key.digit)) {
            return fail_config("debug.date.keys[].digit");
        }
        if (!in_one_band(layout_.bands, key.rect.position.y, key.rect.get_end().y)) {
            return fail_config("layout.bands: debug.date.keys[].rect");
        }
        entry.keys.push_back(key);
    }
    if (entry.keys.empty()) {
        return fail_config("debug.date.keys");
    }
    ::godot::Dictionary clear;
    if (!read_section(date, "clear", clear) || !read_rect(clear, "rect", entry.clear_rect) ||
        !read_text(clear, "label", entry.clear_label)) {
        return fail_config("debug.date.clear");
    }
    if (!in_one_band(layout_.bands, entry.clear_rect.position.y, entry.clear_rect.get_end().y)) {
        return fail_config("layout.bands: debug.date.clear.rect");
    }
    return true;
}

void VivariumView::pick_date(uint8_t part) {
    // THE PICK: the part the keys type into from now on, on the date the fields show (VivariumView::shown_date),
    // held from here; the first key after the pick starts the value afresh.
    VivariumDebugDate& date = debug_.date;
    ::godot::String pick_word =::godot::String("debug date pick part=") + ::godot::String::num_uint64(part);
    if (part >= GODOT_DATE_PARTS) {
        note(pick_word + " result=no_part");
        return;
    }
    if (!date.held) {
        double shown[GODOT_DATE_PARTS] = {};
        if (!shown_date(shown)) {
            note(pick_word + " result=unmeasured");
            return;
        }
        for (uint32_t one = 0; one < GODOT_DATE_PARTS; ++one) {
            date.value[one] = shown[one];
        }
        date.held = true;
    }
    date.typing = part;
    date.typed = 0.0;
    // The whole held date beside the picked value: the keeper reads from this one line where the keys start,
    // without a step to read it back (the window acceptance types its targets from it).
    note(pick_word + " value=" + ::godot::String::num(date.value[part], 0) +
         " year=" + ::godot::String::num(date.value[GODOT_DATE_PART_YEAR], 0) +
         " month=" + ::godot::String::num(date.value[GODOT_DATE_PART_MONTH], 0) +
         " month_day=" + ::godot::String::num(date.value[GODOT_DATE_PART_DAY], 0) +
         " hour=" + ::godot::String::num(date.value[GODOT_DATE_PART_HOUR], 0) + " result=ok");
    node_->queue_redraw();
}

void VivariumView::type_date(const VivariumDebugDateKey* key) {
    // A KEY appends its digit to the picked part; the first key after the pick replaces the value. The bounds
    // of LUNBREX, TUMBRAE and HORAX come from the clock record; a value past the last one is refused with its
    // key and the value stays. CLEAR (key nullptr) empties the picked part.
    const VivariumClockView& clock = resources_.get_clock();
    const double hours_per_day = hour_length_.hours_per_day;
    VivariumDebugDate& date = debug_.date;
    const uint8_t part = date.typing;
    ::godot::String type_word =
        ::godot::String("debug date type part=") + ::godot::String::num_uint64(part) +
        (key != nullptr ? ::godot::String(" digit=") + ::godot::String::num(key->digit, 0) : ::godot::String(" clear"));
    if (part >= GODOT_DATE_PARTS) {
        note(type_word + " result=no_field");
        return;
    }
    if (key == nullptr) {
        date.value[part] = 0.0;
        date.typed = 0.0;
        note(type_word + " value=0 result=ok");
        node_->queue_redraw();
        return;
    }
    if (!(date.typed < date.digits)) {
        note(type_word + " digits=" + ::godot::String::num(date.digits, 0) + " result=digits_spent");
        return;
    }
    const double first[GODOT_DATE_PARTS] = {GODOT_DATE_YEAR_FIRST, GODOT_DATE_FIRST, GODOT_DATE_FIRST,
                                            GODOT_DATE_HOUR_FIRST};
    const double count[GODOT_DATE_PARTS] = {0.0, clock.months_per_year, clock.days_per_month, hours_per_day};
    // The VECTUM knows no end ahead; every other part needs its bound from the clock record.
    if (part != GODOT_DATE_PART_YEAR && (!clock.known || !clock.date_known || !(count[part] > 0.0))) {
        note(type_word + " result=unmeasured");
        return;
    }
    const double typed = date.typed > 0.0 ? date.value[part] * date.base + key->digit : key->digit;
    if (part != GODOT_DATE_PART_YEAR && !(typed < first[part] + count[part])) {
        note(type_word + " value=" + ::godot::String::num(typed, 0) +
             " end=" + ::godot::String::num(first[part] + count[part], 0) + " result=out_of_range");
        return;
    }
    date.value[part] = typed;
    date.typed += 1.0;
    note(type_word + " value=" + ::godot::String::num(typed, 0) +
         " year=" + ::godot::String::num(date.value[GODOT_DATE_PART_YEAR], 0) +
         " month=" + ::godot::String::num(date.value[GODOT_DATE_PART_MONTH], 0) +
         " month_day=" + ::godot::String::num(date.value[GODOT_DATE_PART_DAY], 0) +
         " hour=" + ::godot::String::num(date.value[GODOT_DATE_PART_HOUR], 0) + " result=ok");
    node_->queue_redraw();
}

}  // namespace ase::adp::godot

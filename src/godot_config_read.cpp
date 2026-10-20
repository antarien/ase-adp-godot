/**
 * ASE GODOT ADAPTER - CONFIGURATION READERS IMPLEMENTATION
 *
 * @file        godot_config_read.cpp
 * @brief       The readers of one key of the start configuration
 * @description See godot_config_read.hpp. A number is accepted as an int or a float of the JSON and must be
 *              finite; a rectangle and a point are arrays of exactly GODOT_RECT_FIELDS or GODOT_POINT_FIELDS
 *              numbers.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100360 - parse a json document
 * @actor       Entity Component System (ECS)
 * @created     2026-10-18
 * @modified    2026-10-18
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

namespace {

bool numeric(const ::godot::Variant& value) {
    return value.get_type() == ::godot::Variant::FLOAT || value.get_type() == ::godot::Variant::INT;
}

bool read_numbers(const ::godot::Variant& value, int64_t count, ::godot::Array& out) {
    if (value.get_type() != ::godot::Variant::ARRAY) {
        return false;
    }
    out = static_cast<::godot::Array>(value);
    if (out.size() != count) {
        return false;
    }
    for (int64_t i = 0; i < count; ++i) {
        if (!numeric(out[i])) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool read_number(const ::godot::Dictionary& from, const char* key, double& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (!numeric(value)) {
        return false;
    }
    out = static_cast<double>(value);
    return std::isfinite(out);
}

bool read_text(const ::godot::Dictionary& from, const char* key, ::godot::String& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (value.get_type() != ::godot::Variant::STRING) {
        return false;
    }
    out = static_cast<::godot::String>(value);
    return !out.is_empty();
}

bool read_section(const ::godot::Dictionary& from, const char* key, ::godot::Dictionary& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (value.get_type() != ::godot::Variant::DICTIONARY) {
        return false;
    }
    out = static_cast<::godot::Dictionary>(value);
    return true;
}

bool read_rect(const ::godot::Dictionary& from, const char* key, ::godot::Rect2& out) {
    ::godot::Array values;
    if (!read_numbers(from.get(key, ::godot::Variant()), GODOT_RECT_FIELDS, values)) {
        return false;
    }
    out = ::godot::Rect2(static_cast<float>(values[0]), static_cast<float>(values[1]),
                         static_cast<float>(values[2]), static_cast<float>(values[3]));
    return out.size.x > 0.0f && out.size.y > 0.0f;
}

bool read_point(const ::godot::Dictionary& from, const char* key, ::godot::Vector2& out) {
    ::godot::Array values;
    if (!read_numbers(from.get(key, ::godot::Variant()), GODOT_POINT_FIELDS, values)) {
        return false;
    }
    out = ::godot::Vector2(static_cast<float>(values[0]), static_cast<float>(values[1]));
    return true;
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD WEATHER VEIL
 *
 * @file        godot_hud_weather_veil.cpp
 * @brief       The weather drawn over the scene and under the shade: fog bands, rain, drizzle, snow
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_weather_veil.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0449 (flora.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_weather_veil.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>
#include <ase/math/random.hpp>

#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool WeatherVeil::bind(const UiTheme& theme, std::string& failed) {
    motes_.clear();
    key_ = 0;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_frame_width", &width_},
             {"hud_frame_height", &height_},
             {"hud_weather_fog_bands", &fog_bands_},
             {"hud_weather_fog_y", &fog_y_},
             {"hud_weather_fog_step", &fog_step_},
             {"hud_weather_fog_half", &fog_half_},
             {"hud_weather_particles", &particles_},
             {"hud_weather_seed", &seed_},
             {"hud_weather_speed", &speed_},
             {"hud_weather_speed_span", &speed_span_},
             {"hud_weather_spawn_x", &spawn_x_},
             {"hud_weather_spawn_span", &spawn_span_},
             {"hud_weather_spawn_y", &spawn_y_},
             {"hud_weather_wrap_y", &wrap_y_},
             {"hud_weather_wrap_x", &wrap_x_},
             {"hud_weather_wrap_width", &wrap_width_},
             {"hud_weather_wind", &wind_factor_},
             {"hud_weather_rain_fall", &rain_fall_},
             {"hud_weather_rain_drift", &rain_drift_},
             {"hud_weather_rain_lean", &rain_lean_},
             {"hud_weather_rain_len", &rain_len_},
             {"hud_weather_rain_line", &rain_line_},
             {"hud_weather_drizzle_fall", &drizzle_fall_},
             {"hud_weather_drizzle_drift", &drizzle_drift_},
             {"hud_weather_drizzle_w", &drizzle_w_},
             {"hud_weather_drizzle_h", &drizzle_h_},
             {"hud_weather_snow_fall", &snow_fall_},
             {"hud_weather_snow_drift", &snow_drift_},
             {"hud_weather_snow_sway", &snow_sway_},
             {"hud_weather_snow_sway_rate", &snow_sway_rate_},
             {"hud_weather_snow_radius", &snow_radius_},
             {"hud_weather_snow_radius_span", &snow_radius_span_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"WEATHER_RAIN", &rain_},
                                                                       {"WEATHER_DRIZZLE", &drizzle_},
                                                                       {"WEATHER_SNOW", &snow_},
                                                                       {"WEATHER_FOG", &fog_},
                                                                       {"WEATHER_FOG_CLEAR", &fog_clear_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(particles_ >= 0.0) || !(wrap_width_ > 0.0)) {
        failed = "hud_weather_particles must not be negative and hud_weather_wrap_width must be positive";
        return false;
    }

    // The mockup's PART: x across the spawn span, y over the frame, a speed share and a phase over one turn.
    const uint32_t count = static_cast<uint32_t>(particles_);
    motes_.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        WeatherMote mote;
        mote.x = next_draw() * spawn_span_ + spawn_x_;
        mote.y = next_draw() * height_;
        mote.share = speed_ + next_draw() * speed_span_;
        mote.phase = next_draw() * static_cast<double>(::ase::math::TWO_PI);
        motes_.push_back(mote);
    }
    spread_ = ::godot::Rect2(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_));
    return true;
}

double WeatherVeil::next_draw() {
    const double draw = static_cast<double>(::ase::math::random_hash_float(static_cast<uint32_t>(seed_), key_));
    ++key_;
    return draw;
}

void WeatherVeil::draw_fog(UiPen& canvas, double horizon_y, const ::godot::Rect2& surface) const {
    const float left = surface.position.x;
    const float w = surface.get_end().x;
    const uint32_t bands = static_cast<uint32_t>(fog_bands_);
    for (uint32_t i = 0; i < bands; ++i) {
        const float middle = static_cast<float>(horizon_y + fog_y_ + static_cast<double>(i) * fog_step_);
        const float top = middle - static_cast<float>(fog_half_);
        const float bottom = middle + static_cast<float>(fog_half_);

        ::godot::PackedVector2Array upper;
        upper.push_back(::godot::Vector2(left, top));
        upper.push_back(::godot::Vector2(w, top));
        upper.push_back(::godot::Vector2(w, middle));
        upper.push_back(::godot::Vector2(left, middle));
        ::godot::PackedColorArray upper_colors;
        upper_colors.push_back(fog_clear_);
        upper_colors.push_back(fog_clear_);
        upper_colors.push_back(fog_);
        upper_colors.push_back(fog_);
        canvas.draw_polygon(upper, upper_colors);

        ::godot::PackedVector2Array lower;
        lower.push_back(::godot::Vector2(left, middle));
        lower.push_back(::godot::Vector2(w, middle));
        lower.push_back(::godot::Vector2(w, bottom));
        lower.push_back(::godot::Vector2(left, bottom));
        ::godot::PackedColorArray lower_colors;
        lower_colors.push_back(fog_);
        lower_colors.push_back(fog_);
        lower_colors.push_back(fog_clear_);
        lower_colors.push_back(fog_clear_);
        canvas.draw_polygon(lower, lower_colors);
    }
}

void WeatherVeil::spread(const ::godot::Rect2& surface) {
    if (surface == spread_) {
        return;
    }
    for (WeatherMote& mote : motes_) {
        mote.x = surface.position.x + (mote.x - spread_.position.x) * surface.size.x / spread_.size.x;
        mote.y = surface.position.y + (mote.y - spread_.position.y) * surface.size.y / spread_.size.y;
    }
    spread_ = surface;
}

void WeatherVeil::move(double fall, double drift, const WeatherVeilFrame& now, double sway) {
    const double wind = now.wind * wind_factor_;
    // The mockup's margins around its frame, laid around the surface: what the surface reaches beyond the frame
    // on each side moves the spawn and the wrap with it.
    const ::godot::Rect2& surface = now.cover.surface;
    const double grow_x = static_cast<double>(surface.size.x) - width_;
    const double spawn_x = spawn_x_ + surface.position.x;
    const double spawn_span = spawn_span_ + grow_x;
    const double spawn_y = spawn_y_ + surface.position.y;
    const double wrap_y = wrap_y_ + static_cast<double>(surface.get_end().y) - height_;
    const double wrap_x = wrap_x_ + static_cast<double>(surface.get_end().x) - width_;
    const double wrap_width = wrap_width_ + grow_x;
    for (WeatherMote& mote : motes_) {
        mote.y += now.dt * fall * mote.share;
        mote.x += now.dt * wind * drift;
        if (sway > 0.0) {
            mote.x += static_cast<double>(::ase::math::sin(static_cast<float>(now.time * snow_sway_rate_ + mote.phase))) *
                      sway;
        }
        if (mote.y > wrap_y) {
            mote.y = spawn_y;
            mote.x = next_draw() * spawn_span + spawn_x;
        }
        if (mote.x > wrap_x) {
            mote.x -= wrap_width;
        }
    }
}

bool WeatherVeil::draw(UiPen& canvas, const ::godot::Transform2D& frame, const WeatherVeilFrame& now,
                       std::string& failed) {
    // The frame always carries a kind of weather: flora.weather.v1's, or the mockup's the bridge writes while the
    // port is silent (bridge_weather). A frame without one is a broken frame, never a picture to leave out.
    if (now.kind == nullptr) {
        failed = "WeatherVeil: the frame carries no kind of weather (WeatherVeilFrame::kind)";
        return false;
    }
    if (!now.cover.surface.has_area()) {
        failed = "WeatherVeil: the HUD set no surface for the veil (WeatherVeilFrame::cover)";
        return false;
    }
    const std::string kind(now.kind);
    const double wind = now.wind * wind_factor_;
    spread(now.cover.surface);

    canvas.draw_set_transform_matrix(frame);
    if (kind == GODOT_HUD_WEATHER_FOG) {
        draw_fog(canvas, now.horizon_y, now.cover.surface);
    } else if (kind == GODOT_HUD_WEATHER_RAIN) {
        move(rain_fall_, rain_drift_, now, 0.0);
        const float line = static_cast<float>(rain_line_);
        const ::godot::Vector2 streak(static_cast<float>(-wind * rain_lean_), static_cast<float>(-rain_len_));
        for (const WeatherMote& mote : motes_) {
            const ::godot::Vector2 at(static_cast<float>(mote.x), static_cast<float>(mote.y));
            canvas.draw_line(at, at + streak, rain_, line, true);
        }
    } else if (kind == GODOT_HUD_WEATHER_DRIZZLE) {
        move(drizzle_fall_, drizzle_drift_, now, 0.0);
        for (const WeatherMote& mote : motes_) {
            canvas.draw_rect(::godot::Rect2(static_cast<float>(mote.x), static_cast<float>(mote.y),
                                            static_cast<float>(drizzle_w_), static_cast<float>(drizzle_h_)),
                             drizzle_);
        }
    } else if (kind == GODOT_HUD_WEATHER_SNOW) {
        move(snow_fall_, snow_drift_, now, snow_sway_);
        for (const WeatherMote& mote : motes_) {
            canvas.draw_circle(::godot::Vector2(static_cast<float>(mote.x), static_cast<float>(mote.y)),
                               static_cast<float>(snow_radius_ + mote.share * snow_radius_span_), snow_);
        }
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

}  // namespace ase::adp::godot

/**
 * ASE GODOT ADAPTER - HUD CONTEXT RESOLVER
 *
 * @file        godot_hud_context_resolver.cpp
 * @brief       The brain of the HUD: from the state of the game to the set the view shows, no drawing
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.2, see godot_hud_context_resolver.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_hud_context_resolver.hpp>

#include <utility>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kNone = UINT32_MAX;

// the position of `name` in a list of names, kNone when it is not there
uint32_t position_of(const std::vector<std::string>& names, const std::string& name) {
    for (uint32_t index = 0; index < names.size(); ++index) {
        if (names[index] == name) {
            return index;
        }
    }
    return kNone;
}

// the position of the entry called `name` in a list of specs, kNone when none is
template <typename Spec>
uint32_t spec_position(const std::vector<Spec>& specs, const std::string& name) {
    for (uint32_t index = 0; index < specs.size(); ++index) {
        if (specs[index].name == name) {
            return index;
        }
    }
    return kNone;
}

// whether every one of `required` stands in `held`
bool all_hold(const std::vector<std::string>& required, const std::vector<std::string>& held) {
    for (const std::string& mark : required) {
        if (position_of(held, mark) == kNone) {
            return false;
        }
    }
    return true;
}

// whether the tool needs no item or its item is in possession
bool tool_got(const HudToolSpec& tool, const std::vector<std::string>& possessed) {
    return tool.has.empty() || position_of(possessed, tool.has) != kNone;
}

// whether every spec of one list carries a name of its own; names the first repeated or empty one in `failed`
template <typename Spec>
bool names_unique(const std::vector<Spec>& specs, const char* section, std::string& failed) {
    for (uint32_t index = 0; index < specs.size(); ++index) {
        if (specs[index].name.empty() || spec_position(specs, specs[index].name) != index) {
            failed = std::string(section) + "." + specs[index].name;
            return false;
        }
    }
    return true;
}

}  // namespace

bool ContextResolver::set_spec(HudContextSpec spec, std::string& failed) {
    ready_ = false;
    if (spec.modes.empty()) {
        failed = "hud.modes";
        return false;
    }
    if (!names_unique(spec.modes, "hud.modes", failed) || !names_unique(spec.tools, "hud.tools", failed) ||
        !names_unique(spec.signs, "hud.signs", failed)) {
        return false;
    }
    if (spec_position(spec.modes, spec.start) == kNone) {
        failed = "hud.modes.start";
        return false;
    }
    if (spec_position(spec.modes, spec.sign_lost) == kNone) {
        failed = "hud.modes.sign_lost";
        return false;
    }
    if (spec_position(spec.modes, spec.tool_lost) == kNone) {
        failed = "hud.modes.tool_lost";
        return false;
    }
    for (const HudLayerRule& rule : spec.over) {
        if (rule.layer.empty() || rule.marks.empty()) {
            failed = "hud.modes.over." + rule.layer;
            return false;
        }
    }
    for (const HudLayerRule& rule : spec.hides) {
        if (rule.layer.empty() || rule.marks.empty()) {
            failed = "hud.modes.hides." + rule.layer;
            return false;
        }
    }
    if (spec.fan.size() > spec.fan_places) {
        failed = "hud.focus.fan.tools";
        return false;
    }
    for (uint32_t index = 0; index < spec.fan.size(); ++index) {
        const std::string& name = spec.fan[index];
        if (spec_position(spec.tools, name) == kNone || position_of(spec.fan, name) != index) {
            failed = "hud.focus.fan.tools." + name;
            return false;
        }
    }
    for (const HudSignSpec& sign : spec.signs) {
        if (!sign.offer.empty() && spec_position(spec.tools, sign.offer) == kNone) {
            failed = "hud.signs." + sign.name + ".offer";
            return false;
        }
        if (sign.offer.empty() && !sign.offer_when.empty()) {
            failed = "hud.signs." + sign.name + ".offer_when";
            return false;
        }
    }
    bool start_level_known = false;
    for (const uint32_t level : spec.levels) {
        if (level == 0u) {
            failed = "hud.perception.levels";
            return false;
        }
        start_level_known = start_level_known || level == spec.level_start;
    }
    if (!start_level_known) {
        failed = "hud.perception.start";
        return false;
    }
    spec_ = std::move(spec);
    ready_ = true;
    return true;
}

bool ContextResolver::resolve(const HudGameState& state, HudShown& shown, std::string& failed) const {
    if (!ready_) {
        failed = "hud";
        return false;
    }

    // the reported signs, in the order of hud.signs; a sign no rule knows is refused
    for (const std::string& reported : state.signs) {
        if (spec_position(spec_.signs, reported) == kNone) {
            failed = "hud.signs." + reported;
            return false;
        }
    }
    shown.signs.clear();
    shown.open_sign = kNone;
    shown.alerts = 0;
    for (uint32_t index = 0; index < spec_.signs.size(); ++index) {
        const HudSignSpec& sign = spec_.signs[index];
        if (position_of(state.signs, sign.name) == kNone) {
            continue;
        }
        HudSignShown entry;
        entry.sign = index;
        entry.alert = sign.alert || (!sign.alert_when.empty() && position_of(state.marks, sign.alert_when) != kNone);
        entry.open = !state.open_sign.empty() && state.open_sign == sign.name;
        if (!sign.offer.empty() && all_hold(sign.offer_when, state.marks)) {
            entry.offer = spec_position(spec_.tools, sign.offer);
            entry.offer_got = tool_got(spec_.tools[entry.offer], state.possessed);
        }
        if (entry.alert) {
            ++shown.alerts;
        }
        if (entry.open) {
            shown.open_sign = static_cast<uint32_t>(shown.signs.size());
        }
        shown.signs.push_back(entry);
    }

    // the mode the state names; the one for a lost sign when the open sign is no longer reported, the
    // one for a lost tool when the mode is a tool's and its item is no longer in possession
    const bool sign_lost = !state.open_sign.empty() && shown.open_sign == kNone;
    const uint32_t mode_tool = spec_position(spec_.tools, state.mode);
    const bool tool_lost = mode_tool != kNone && !spec_.tools[mode_tool].has.empty() &&
                           !tool_got(spec_.tools[mode_tool], state.possessed);
    const std::string& mode_name = sign_lost ? spec_.sign_lost : (tool_lost ? spec_.tool_lost : state.mode);
    shown.mode = spec_position(spec_.modes, mode_name);
    if (shown.mode == kNone) {
        failed = "hud.modes." + mode_name;
        return false;
    }
    const HudModeSpec& mode = spec_.modes[shown.mode];
    shown.hits = mode.hits;
    shown.layers.clear();
    for (const std::string& layer : mode.layers) {
        bool hidden = false;
        for (const HudLayerRule& rule : spec_.hides) {
            hidden = hidden || (rule.layer == layer && all_hold(rule.marks, state.marks));
        }
        if (!hidden) {
            shown.layers.push_back(layer);
        }
    }
    for (const HudLayerRule& rule : spec_.over) {
        if (all_hold(rule.marks, state.marks)) {
            shown.layers.push_back(rule.layer);
        }
    }

    // the fan: while a place is selected, every tool whose marks hold and whose item is in possession
    shown.fan.clear();
    if (state.place_selected) {
        for (const std::string& name : spec_.fan) {
            const uint32_t tool = spec_position(spec_.tools, name);
            const HudToolSpec& fan_tool = spec_.tools[tool];
            if (!all_hold(fan_tool.marks_required, state.marks) || !tool_got(fan_tool, state.possessed)) {
                continue;
            }
            HudFanTool entry;
            entry.tool = tool;
            entry.place = static_cast<uint32_t>(shown.fan.size());
            shown.fan.push_back(entry);
        }
    }

    // the depth of perception: one of hud.perception.levels, never 0
    bool known = false;
    for (const uint32_t level : spec_.levels) {
        known = known || level == state.level;
    }
    if (!known) {
        failed = "hud.perception.levels";
        return false;
    }
    shown.level = state.level;
    return true;
}

}  // namespace ase::adp::godot

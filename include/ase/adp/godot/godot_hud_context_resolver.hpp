#pragma once

/**
 * ASE GODOT ADAPTER - HUD CONTEXT RESOLVER
 *
 * @file        godot_hud_context_resolver.hpp
 * @brief       The brain of the HUD: from the state of the game to the set the view shows, no drawing
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.2, section "Das Gehirn". One function from the
 *              state of the game to what the HUD shows: the mode with its hit areas and its layers in
 *              the order of drawing, the layers over every mode, the tools of the fan, the signs with
 *              their alert and their offer, the possession behind every offer, the depth of perception.
 *
 *              EVERY RULE STANDS IN THE START CONFIGURATION. hud.modes names the hits and layers of the
 *              fourteen modes, the layers over every mode and the layers a mode leaves out, each with
 *              the view marks that must all hold; hud.tools names the marks a tool requires and the
 *              item it needs; hud.focus.fan the tools of the fan in the order of its places; hud.signs
 *              the alert, the offer and the marks an offer waits for; hud.perception the levels. UiTheme
 *              reads them into a HudContextSpec, this unit only applies it.
 *
 *              A MARK IS A STATE A PORT RECKONED, NEVER A THRESHOLD OF THIS UNIT. "alive", "soil_not_full",
 *              "stage_grow" or "tap_open" come as marks of the ports about the selected patch and the
 *              clock (06.10), "debug_on", "drawer_open" and "dragging" as marks of the view. A sign
 *              shows because a port reports it; the resolver lays it into its order, gives it its alert
 *              and its offer and derives no sign from a number.
 *
 *              THE FAN AND THE OFFER DIFFER IN POSSESSION. A tool in the fan shows only while its item
 *              is in possession - what lies neither in the bag, on the belt nor at the tool place does
 *              not appear. The offer of a sign shows without it, flagged as missing: the label says
 *              FALTA and a tap toasts it (mockup: the fan and useTool).
 *
 *              A TOOL RUNS ONLY WHILE ITS ITEM IS IN POSSESSION. The mode of a tool (water, prune, sow,
 *              sap, plug: a mode carrying the name of a tool that needs an item) is the tool at work, the
 *              item in the hand (mockup: equip sets the mode of the item's act). When that item leaves the
 *              possession the mode is hud.modes.tool_lost, as the mockup returns to focus once the item
 *              leaves the hand - a tool nobody holds has nothing the player could stop.
 *
 *              No Godot type: the unit stands in the host library ase-adp-godot-host and its test runs
 *              without an engine (tests/test_godot_hud_context_resolver.cpp, target
 *              ase-adp-godot-context-test).
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

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One mode of the play loop as hud.modes states it. */
struct HudModeSpec {
    std::string name;                 // the key under hud.modes
    std::vector<std::string> hits;    // the hit areas laid first: world, tree, trunk, ground
    std::vector<std::string> layers;  // the layers in the order of drawing
};

/** A layer and the view marks that must all hold for its rule (hud.modes.over, hud.modes.hides). */
struct HudLayerRule {
    std::string layer;
    std::vector<std::string> marks;
};

/** One tool as hud.tools states it. */
struct HudToolSpec {
    std::string name;                         // the key under hud.tools
    std::vector<std::string> marks_required;  // hud.tools.<name>.requires: the marks that must all hold
    std::string has;                          // the item that must be in possession, empty = none
};

/** One sign as hud.signs states it. */
struct HudSignSpec {
    std::string name;                     // the key under hud.signs, the id a port reports
    bool alert = false;                   // hud.signs.<name>.alert true: the sign always alerts
    std::string alert_when;               // hud.signs.<name>.alert as a mark: it alerts while that mark holds
    std::string offer;                    // the tool it offers, empty = none (null)
    std::vector<std::string> offer_when;  // the marks that must all hold for the offer to show
};

/** All rules of the context, filled by UiTheme from the start configuration. */
struct HudContextSpec {
    std::vector<HudModeSpec> modes;    // the fourteen modes
    std::vector<HudLayerRule> over;    // the layers over every mode
    std::vector<HudLayerRule> hides;   // the layers a mode leaves out
    std::string start;                 // the mode the view starts in
    std::string sign_lost;             // the mode when the open sign is no longer reported
    std::string tool_lost;             // the mode when the item of the tool mode is no longer in possession
    std::vector<HudToolSpec> tools;    // every tool of the fan and of an offer
    std::vector<std::string> fan;      // hud.focus.fan.tools, in the order of the places
    uint32_t fan_places = 0;           // hud.focus.fan.offsets: how many places the fan has
    std::vector<HudSignSpec> signs;    // the twelve signs, in the order of the rail
    std::vector<uint32_t> levels;      // hud.perception.levels
    uint32_t level_start = 0;          // hud.perception.start
};

/** The state of the game and of the view the rules are applied to. */
struct HudGameState {
    std::string mode;                    // the mode the view is in; the view starts in spec().start
    std::string open_sign;               // the sign opened by a tap, empty = none
    bool place_selected = false;         // a patch is selected
    std::vector<std::string> marks;      // the marks that hold: of the ports and of the view
    std::vector<std::string> possessed;  // the items in possession: bag, pouch, belt, hand, tool place
    std::vector<std::string> signs;      // the signs the ports report, in any order
    uint32_t level = 0;                  // the depth of perception; the view starts at spec().level_start
};

/** One tool in the fan. */
struct HudFanTool {
    uint32_t tool = UINT32_MAX;   // index into HudContextSpec::tools
    uint32_t place = UINT32_MAX;  // index into hud.focus.fan.offsets, from 0 in fan order
};

/** One sign the HUD shows. */
struct HudSignShown {
    uint32_t sign = UINT32_MAX;   // index into HudContextSpec::signs
    bool alert = false;           // the sign alerts: it pulses and counts on the rail
    bool open = false;            // the sign is the open one
    uint32_t offer = UINT32_MAX;  // index into HudContextSpec::tools, UINT32_MAX = no offer
    bool offer_got = false;       // the offered tool needs no item or its item is in possession
};

/** What the HUD shows. */
struct HudShown {
    uint32_t mode = UINT32_MAX;          // index into HudContextSpec::modes
    std::vector<std::string> hits;       // the hit areas of the mode
    std::vector<std::string> layers;     // the layers in the order of drawing, those over every mode last
    std::vector<HudFanTool> fan;         // the tools of the fan, in the order of their places
    std::vector<HudSignShown> signs;     // the reported signs, in the order of hud.signs
    uint32_t open_sign = UINT32_MAX;     // index into signs of the open one, UINT32_MAX = none
    uint32_t alerts = 0;                 // how many shown signs alert
    uint32_t level = 0;                  // the depth of perception: 1 words, 2 bars, 3 numbers
};

/**
 * The context of the HUD: holds the rules and applies them to a state. set_spec checks every
 * reference of the rules; resolve refuses a state that names a mode, a sign or a level the rules
 * do not know, an empty mode and the level 0 among them, and names the key in `failed`. Nothing
 * missing is replaced by a value of the rules: the view sets its start state itself from
 * spec().start and spec().level_start, and the skill port's level replaces it once it carries one.
 */
class ContextResolver {
public:
    bool set_spec(HudContextSpec spec, std::string& failed);
    bool resolve(const HudGameState& state, HudShown& shown, std::string& failed) const;
    const HudContextSpec& spec() const { return spec_; }

private:
    HudContextSpec spec_;
    bool ready_ = false;
};

}  // namespace ase::adp::godot

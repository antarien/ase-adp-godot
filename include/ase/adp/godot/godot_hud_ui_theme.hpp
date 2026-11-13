#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI THEME
 *
 * @file        godot_hud_ui_theme.hpp
 * @brief       The one reader of the HUD's look and words: SHA's colours, tokens, words and fonts, the zones and the brain rules
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.1. Every building block of the HUD asks this unit
 *              for what it draws with and never reads a file of its own.
 *
 *              THE LOOK COMES FROM SHA. The colours are colors.json (colors.ts through generate-colors.cjs),
 *              every measure, gesture time and animation is a token of design_tokens.json (theme.css through
 *              generate-design-tokens.cjs, the VIVARIUM HUD sections among them), every word is a key of
 *              translations_hud.json (sha-web-translations/hud/<lang>.yaml through
 *              generate-translations-hud.cjs), every drawing of a carried thing is an entry of item_art.json
 *              (sha-web-styles/src/item-art.json through generate-item-art.cjs) and every symbol is an
 *              ICON_* of ui_icons.json drawn in the Nerd Font. The SHA cascade brings the five files into the client and avt-stage stages them
 *              into res://config/; the start configuration names their paths under hud.distribution and
 *              debug.icon_table, so no path stands in this code.
 *
 *              FROM THE START CONFIGURATION ONLY THE SHAPE OF THIS VIEW: the language (hud.language), the
 *              four faces (hud.fonts), the zones (hud.zones) and the brain rules ContextResolver applies
 *              (hud.modes, hud.tools, hud.focus.fan, hud.signs, hud.perception), read into a HudContextSpec.
 *
 *              A WORD IS A LIST OF RUNS. A run is either text, drawn in the face its block names, or an
 *              icon, an ICON_* name a block resolves through UiIcon - this unit hands the name on and draws
 *              nothing. A lower-case {name} inside a text run is a port value the block fills through
 *              fill_words (godot_hud_ui_word_fill.hpp), the one filler of every block.
 *
 *              NOTHING FALLS BACK. load refuses a missing file, key or face and names it in `failed`; every
 *              lookup answers false for a name the files do not carry, and the caller refuses with that
 *              name instead of drawing a default. A translation the generator flagged as drift loads and is
 *              named on the log, so stale words are visible instead of shipped as current.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-12
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_context_resolver.hpp>

#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** One run of a word: exactly one of the two is set. */
struct HudRun {
    std::string text;  // drawn as text; a lower-case {name} inside it is a port value
    std::string icon;  // an ICON_* name, drawn through UiIcon
};

/** The four faces of the mockup (--f-disp in two weights, --f-body, --f-data). */
struct HudFaces {
    ::godot::Ref<::godot::FontFile> display_medium;
    ::godot::Ref<::godot::FontFile> display_semibold;
    ::godot::Ref<::godot::FontFile> body;
    ::godot::Ref<::godot::FontFile> data;  // the Nerd Font: every number and every icon
    double spacing_medium_em = 0.0;        // SHA token hud_tracking_label
    double spacing_semibold_em = 0.0;      // SHA token hud_tracking_title
};

/**
 * The surface around the mockup frame, in mockup units of the middle band's frame (PLAN_ASE_VIVARIUM_PHASE_06_HUD,
 * the zones on a tall surface). The zones of hud.zones sit on the bands of the layout: the band zone on the top
 * band, the hand zone on the bottom band, rail, world and full on the middle band. Every block draws in the frame of
 * the middle band; a block of an outer zone stands `head` or `hand` units off it, and a dim, the veil and the shade
 * reach `surface`. On a surface of the mockup's own aspect head and hand are 0 and surface is the frame itself.
 */
struct HudCover {
    ::godot::Rect2 surface;  // the whole surface
    double head = 0.0;       // the top band's offset against the middle band (the band zone), <= 0
    double hand = 0.0;       // the bottom band's offset against the middle band (the hand zone), >= 0
};

/**
 * The dim a mode lays over the world from `y` over `h` (mockup units of the frame), stretched to `cover`: it begins
 * where the band zone ends on the top band (y + head), ends where its foot stands on the bottom band
 * (y + h + hand) and reaches across the surface. On a surface of the mockup's aspect it is the mockup's own rect
 * from x 0 over hud_frame_width.
 */
::godot::Rect2 hud_cover_dim(const HudCover& cover, double y, double h);

/**
 * The theme of the HUD. load reads everything once; the lookups are const and answer false for a name
 * the files do not carry.
 */
class UiTheme {
public:
    /** Reads the four SHA files, the faces, the zones and the brain rules; names the first missing key in `failed`. */
    bool load(const ::godot::Dictionary& root, std::string& failed);

    /** A colour of colors.json by its name (DOMAIN_WATER, OVERLAY_INK, ...). */
    bool color(const std::string& name, ::godot::Color& out) const;

    /** The colour of a domain tone of the HUD (water, flow, life, ...): DOMAIN_<TONE> of colors.json. */
    bool tone(const std::string& name, ::godot::Color& out) const;

    /** A number token of design_tokens.json (px, em or bare). */
    bool measure(const std::string& token, double& out) const;

    /** A text token of design_tokens.json (an easing, a duration with its unit). */
    bool token_text(const std::string& token, ::godot::String& out) const;

    /** The runs of a word of translations_hud.json in the language of hud.language. */
    bool words(const std::string& key, std::vector<HudRun>& out) const;

    /** A zone of hud.zones. */
    bool zone(const std::string& name, ::godot::Rect2& out) const;

    /** The drawing of a carried thing of item_art.json (vw, vh, art, level) by its name (pa, regador, ...). */
    bool item_art(const std::string& name, ::godot::Dictionary& out) const;

    /**
     * The item of hud.items whose widget names the class `widget` (DibberTool is plantador), its mode and its
     * ground verbs in their order (an item that names none has none); false when no item names the class.
     */
    bool item_widget(const std::string& widget, std::string& id, std::string& mode,
                     std::vector<std::string>& verbs) const;

    /** The mode hud.items.`id` puts the hand in (the mockup's D.act); false when the item names none. */
    bool item_mode(const std::string& id, std::string& mode) const;

    /**
     * What hud.items.`id` says of the item beside its drawing: its domain tone (D.dom), the asset of phase 04
     * (D.asset, empty where the start names it null: no sprite yet, drawn from its art), its ground verbs in their
     * order (none where it names none) and the view box of its drawing (vw, vh); false when the item or one of
     * tone, asset, vw and vh is missing.
     */
    bool item_thing(const std::string& id, std::string& tone, std::string& asset, std::vector<std::string>& verbs,
                    double& view_w, double& view_h) const;

    /** The widget class hud.items.`id` names (ItemWidget::bind takes it); false when the item names none. */
    bool item_widget_class(const std::string& id, std::string& widget) const;

    /**
     * What hud.items.`id` says the item is drawn from in phase 04 while it has no asset (the mockup's D.phase04, read
     * where D.asset is null); empty for an item with an asset. False when the item is missing, or names asset null
     * without a phase04 text.
     */
    bool item_phase04(const std::string& id, std::string& phase04) const;

    const HudFaces& faces() const { return faces_; }
    const ::godot::Dictionary& icons() const { return icons_; }
    double icon_web_px() const { return icon_web_px_; }
    const std::string& language() const { return language_; }
    const HudContextSpec& spec() const { return spec_; }

    /**
     * Every character a text run of the words of hud.language shows, each once in the order first met: the
     * letters a line can draw besides the numbers it is filled with. UiTextBlock::warm rasters them before a line
     * first shows (P0692): the reading's label and value drew their letters first at the first act, 40.1 ms in
     * that frame (run p0692-warm), while only the digits were readied.
     */
    const ::godot::String& alphabet() const { return alphabet_; }

private:
    /** The brain rules of the section hud (godot_hud_ui_theme_spec.cpp); false with the missing key in `failed`. */
    static bool read_spec(const ::godot::Dictionary& hud, HudContextSpec& spec, std::string& failed);

    ::godot::Dictionary colors_;  // colors.json: colors
    ::godot::Dictionary tokens_;  // design_tokens.json: tokens
    ::godot::Dictionary words_;   // translations_hud.json: languages.<language>.keys
    ::godot::Dictionary zones_;   // hud.zones
    ::godot::Dictionary items_;   // item_art.json: items
    ::godot::Dictionary things_;  // hud.items: the widget class, mode and ground verbs of each item
    ::godot::Dictionary icons_;  // ui_icons.json: icons
    double icon_web_px_ = 0.0;    // ui_icons.json: web_size_px
    HudFaces faces_;
    std::string language_;
    ::godot::String alphabet_;    // every character of the text runs of words_, each once (alphabet())
    HudContextSpec spec_;
};

}  // namespace ase::adp::godot

/**
 * ASE ADAPTER TESTS - THE THIRTEEN ACTIVITY SYMBOLS TRAVEL AND ARE READ LIKE THE ITEM ART
 *
 * @file        test_godot_hud_activity_art.cpp
 * @design      DSGN_067
 * @brief       Pins activity-art.json of SHA: the thirteen symbols in the format of item-art.json, every colour
 *              a token of colors.ts, their words in every language, and the way the file reaches UiActivityArt
 * @description MIG_ASE_SKILL_RSM B7, PLAN_ASE_VIVARIUM_PHASE_06_HUD "Item-Art: die Zeichnung jedes Dings" and
 *              "Herkunft: SHA ist die Quelle". The source stands in sha-web-styles/src/activity-art.json,
 *              generate-item-art.cjs writes it as activity_art.json (key activities), AvtGodot.cmake hands it
 *              to avt-stage as one more --hud-data file, the start configuration names it under
 *              hud.distribution.activities, and UiActivityArt::load refuses the configuration where it is
 *              missing or unreadable. UiActivityArt needs the Godot runtime, so this test reads the files of the
 *              tree as text, the way test_godot_hud_figure_layout.cpp reads theme.css, and pins each link of
 *              that chain. Pure computation, no Godot type.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace {

// a file of the tree, from the adapter's folder
std::string tree_text(const std::string& relative) {
    const std::string adapter = ase::fileio::parent_of(ase::fileio::parent_of(__FILE__));
    return ase::fileio::read_text(ase::fileio::path_join(adapter, "../../" + relative));
}

// the thirteen symbols of the table of MIG_ASE_SKILL_RSM B7
std::vector<std::string> activity_names() {
    return {"picareta", "vagoneta",  "pa_monte", "guincho", "veio",         "monte",          "toras",
            "tabuas",   "cavalete",  "mata",     "arvore_seca", "machado_toco", "serra_tracador"};
}

// the text of the entry `name` of the file, up to the next entry at the same depth; empty without the entry
std::string entry_of(const std::string& file, const std::string& name) {
    const std::string key = "\n  \"" + name + "\": {";
    const std::size_t at = file.find(key);
    if (at == std::string::npos) {
        return "";
    }
    const std::size_t next = file.find("\n  \"", at + key.size());
    return file.substr(at, next == std::string::npos ? std::string::npos : next - at);
}

// every value of the field `"<field>": "<value>"` in `text`
std::vector<std::string> values_of(const std::string& text, const std::string& field) {
    std::vector<std::string> values;
    const std::string key = "\"" + field + "\": \"";
    for (std::size_t at = text.find(key); at != std::string::npos; at = text.find(key, at + 1)) {
        const std::size_t from = at + key.size();
        values.push_back(text.substr(from, text.find('"', from) - from));
    }
    return values;
}

std::size_t count_of(const std::string& text, const std::string& needle) {
    std::size_t count = 0;
    for (std::size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) {
        ++count;
    }
    return count;
}

}  // namespace

TEST_CASE("B7: activity-art.json carries the thirteen symbols in the format of item-art.json") {
    const std::string file = tree_text("clients/sha-client-web/sha-web-styles/src/activity-art.json");
    REQUIRE_MESSAGE(!file.empty(), "clients/sha-client-web/sha-web-styles/src/activity-art.json");

    // positive control: the scanner finds nothing for a name the file does not carry
    CHECK(entry_of(file, "podao").empty());

    for (const std::string& name : activity_names()) {
        const std::string entry = entry_of(file, name);
        INFO(name);
        REQUIRE(!entry.empty());
        CHECK(entry.find("\"vw\":") != std::string::npos);
        CHECK(entry.find("\"vh\":") != std::string::npos);
        CHECK(entry.find("\"art\": [") != std::string::npos);
        // every path of the entry has its d, a stroke or a fill, and a stroke width w
        const std::size_t paths = count_of(entry, "{\"d\":");
        CHECK(paths > 0);
        CHECK(count_of(entry, "\"w\":") == paths);
    }
}

TEST_CASE("B7: every colour of the activity symbols is a token of colors.ts") {
    const std::string file = tree_text("clients/sha-client-web/sha-web-styles/src/activity-art.json");
    const std::string colors = tree_text("clients/sha-client-web/sha-web-styles/src/colors.ts");
    REQUIRE(!file.empty());
    REQUIRE(!colors.empty());

    std::vector<std::string> used = values_of(file, "stroke");
    const std::vector<std::string> fills = values_of(file, "fill");
    used.insert(used.end(), fills.begin(), fills.end());
    CHECK(!used.empty());  // positive control: colours are read
    for (const std::string& token : used) {
        INFO(token);
        CHECK(colors.find("export const " + token + " ") != std::string::npos);
    }
}

TEST_CASE("B7: each activity has its name in every language of the hud words") {
    for (const char* language : {"pt-br", "de", "en"}) {
        const std::string words =
            tree_text(std::string("clients/sha-client-web/sha-web-translations/hud/") + language + ".yaml");
        REQUIRE_MESSAGE(!words.empty(), language);
        for (const std::string& name : activity_names()) {
            INFO(language << " " << name);
            CHECK(words.find("activities." + name + ".name: \"") != std::string::npos);
        }
    }
}

TEST_CASE("B7: the file travels the way item_art.json travels and is read without a fallback") {
    // the generator writes it from the same source format, under the key UiActivityArt::load reads
    const std::string generator = tree_text("clients/sha-client-web/sha-web-console/generate-item-art.cjs");
    CHECK(generator.find("activity_art.json") != std::string::npos);
    CHECK(generator.find("key: \"activities\"") != std::string::npos);
    // SHA's prebuild runs it, the cascade carries console/generated/ whole
    const std::string prebuild = tree_text("clients/sha-client-web/scripts/prebuild.cjs");
    CHECK(prebuild.find("\"generate-item-art.cjs\", \"activity\"") != std::string::npos);
    // avt-stage stages it into res://config/, and refuses the stage when the file is missing
    const std::string cmake = tree_text("clients/avt-client-android/cmake/AvtGodot.cmake");
    CHECK(cmake.find("--hud-data ${AVT_HUD_DATA_ACTIVITIES}") != std::string::npos);
    CHECK(cmake.find("console/generated/activity_art.json") != std::string::npos);
    // the start configuration names it under hud.distribution
    const std::string start = tree_text("clients/avt-client-android/config/vivarium_start.json");
    CHECK(start.find("\"activities\": \"res://config/activity_art.json\"") != std::string::npos);
    // the reader refuses the configuration where the name, the file or its section is missing
    const std::string reader = tree_text("adapter/ase-adp-godot/src/godot_hud_ui_activity_art.cpp");
    CHECK(reader.find("failed = \"hud.distribution.activities\";") != std::string::npos);
    CHECK(reader.find("read_section(file, \"activities\", activities_)") != std::string::npos);
}

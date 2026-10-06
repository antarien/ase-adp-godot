/**
 * ASE ADAPTER TESTS - VIEW LAYOUT PLACEMENT
 *
 * @file        test_godot_view_layout.cpp
 * @design      DSGN_021
 * @brief       Pins place_view, band_offset and in_one_band on the surfaces of the Android market
 * @description PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3: the Vivarium fills every handset without
 *              bars. The layout is the client's (vivarium_start.json): logical size 720 x 1280, the
 *              top band up to 160, the bottom band from 1090. The surfaces are those Godot's expand
 *              aspect hands the view: a 9:20 handset such as the Pixel 8 (1080 x 2400 pixels) is
 *              720 x 1600 logical, a 3:4 tablet in portrait 960 x 1280. Each expectation is computed
 *              by hand from the rule the placement states - top band at the top of the safe part,
 *              bottom band at its bottom, the middle band in the middle, never magnified.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @created     2026-10-06
 * @modified    2026-10-06
 * @version     00.00.01.00001
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_view_layout.hpp>

namespace ase::adp::godot {

namespace {

// The client's layout: logical size and the two band edges of vivarium_start.json.
ViewBands vivarium_bands() {
    ViewBands bands;
    bands.width = 720.0f;
    bands.height = 1280.0f;
    bands.top_end = 160.0f;
    bands.bottom_start = 1090.0f;
    return bands;
}

ViewSurface surface_of(float width, float height) {
    ViewSurface surface;
    surface.width = width;
    surface.height = height;
    return surface;
}

}  // namespace

TEST_CASE("a surface of the logical size places the layout unchanged") {
    const ViewPlacement placement = place_view(vivarium_bands(), surface_of(720.0f, 1280.0f));
    CHECK(placement.scale == doctest::Approx(1.0f));
    CHECK(placement.offset_x == doctest::Approx(0.0f));
    CHECK(placement.top_y == doctest::Approx(0.0f));
    CHECK(placement.middle_y == doctest::Approx(0.0f));
    CHECK(placement.bottom_y == doctest::Approx(0.0f));
}

TEST_CASE("a 9:20 handset keeps the header at the top and the buttons at the bottom") {
    // Pixel 8: 1080 x 2400 pixels, expand aspect at a stretch scale of 1.5 - 720 x 1600 logical.
    const ViewBands bands = vivarium_bands();
    const ViewPlacement placement = place_view(bands, surface_of(720.0f, 1600.0f));
    CHECK(placement.scale == doctest::Approx(1.0f));
    CHECK(placement.offset_x == doctest::Approx(0.0f));
    CHECK(placement.top_y == doctest::Approx(0.0f));
    CHECK(placement.middle_y == doctest::Approx(160.0f));
    CHECK(placement.bottom_y == doctest::Approx(320.0f));
    // The bottom band ends where the surface ends: no bar below the note.
    CHECK(placement.bottom_y + bands.height * placement.scale == doctest::Approx(1600.0f));
}

TEST_CASE("camera cut-out and gesture bar stay free") {
    const ViewBands bands = vivarium_bands();
    ViewSurface surface = surface_of(720.0f, 1600.0f);
    surface.inset_top = 60.0f;
    surface.inset_bottom = 32.0f;
    const ViewPlacement placement = place_view(bands, surface);
    CHECK(placement.scale == doctest::Approx(1.0f));
    CHECK(placement.top_y == doctest::Approx(60.0f));
    CHECK(placement.middle_y == doctest::Approx(174.0f));
    CHECK(placement.bottom_y == doctest::Approx(288.0f));
    CHECK(placement.bottom_y + bands.height == doctest::Approx(1600.0f - 32.0f));
}

TEST_CASE("a wider surface centres the column") {
    // A 3:4 tablet in portrait: 960 x 1280 logical.
    const ViewPlacement placement = place_view(vivarium_bands(), surface_of(960.0f, 1280.0f));
    CHECK(placement.scale == doctest::Approx(1.0f));
    CHECK(placement.offset_x == doctest::Approx(120.0f));
    CHECK(placement.top_y == doctest::Approx(0.0f));
    CHECK(placement.bottom_y == doctest::Approx(0.0f));
}

TEST_CASE("a safe part smaller than the logical size scales everything down evenly") {
    ViewSurface surface = surface_of(720.0f, 1280.0f);
    surface.inset_top = 64.0f;
    surface.inset_bottom = 32.0f;
    const ViewPlacement placement = place_view(vivarium_bands(), surface);
    CHECK(placement.scale == doctest::Approx(1184.0f / 1280.0f));
    CHECK(placement.offset_x == doctest::Approx((720.0f - 720.0f * 1184.0f / 1280.0f) * 0.5f));
    CHECK(placement.top_y == doctest::Approx(64.0f));
    CHECK(placement.middle_y == doctest::Approx(64.0f));
    CHECK(placement.bottom_y == doctest::Approx(64.0f));
}

TEST_CASE("a surface or a layout without area places nothing") {
    CHECK(place_view(vivarium_bands(), surface_of(0.0f, 0.0f)).scale == doctest::Approx(0.0f));
    ViewSurface swallowed = surface_of(720.0f, 1600.0f);
    swallowed.inset_top = 900.0f;
    swallowed.inset_bottom = 900.0f;
    CHECK(place_view(vivarium_bands(), swallowed).scale == doctest::Approx(0.0f));
    ViewBands empty = vivarium_bands();
    empty.width = 0.0f;
    CHECK(place_view(empty, surface_of(720.0f, 1600.0f)).scale == doctest::Approx(0.0f));
}

TEST_CASE("band_offset gives every layout y the offset of its band") {
    const ViewBands bands = vivarium_bands();
    const ViewPlacement placement = place_view(bands, surface_of(720.0f, 1600.0f));
    CHECK(band_offset(bands, placement, 96.0f) == doctest::Approx(placement.top_y));       // title
    CHECK(band_offset(bands, placement, 160.0f) == doctest::Approx(placement.middle_y));   // edge: middle
    CHECK(band_offset(bands, placement, 900.0f) == doctest::Approx(placement.middle_y));   // info
    CHECK(band_offset(bands, placement, 1090.0f) == doctest::Approx(placement.bottom_y));  // edge: bottom
    CHECK(band_offset(bands, placement, 1262.0f) == doctest::Approx(placement.bottom_y));  // note
}

TEST_CASE("in_one_band holds a span inside one band and refuses one across an edge") {
    const ViewBands bands = vivarium_bands();
    CHECK(in_one_band(bands, 0.0f, 160.0f));         // header band
    CHECK(in_one_band(bands, 180.0f, 480.0f));       // a patch
    CHECK(in_one_band(bands, 1110.0f, 1220.0f));     // the buttons
    CHECK_FALSE(in_one_band(bands, 150.0f, 200.0f)); // across the top edge
    CHECK_FALSE(in_one_band(bands, 1000.0f, 1100.0f)); // across the bottom edge
    CHECK_FALSE(in_one_band(bands, 1100.0f, 1300.0f)); // below the layout
    CHECK_FALSE(in_one_band(bands, 500.0f, 400.0f)); // upside down
}

}  // namespace ase::adp::godot

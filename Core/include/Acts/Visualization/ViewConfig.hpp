// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <array>
#include <filesystem>
#include <string_view>

namespace Acts {

/// Struct describing a color. Internally, the color is represented using a
/// triplet of integers between 0 and 255 corresponding to red green and blue.
struct Color {
  // Copy and move constructors are defaulted
  Color() = default;
  Color(const Color&) = default;
  Color(Color&&) = default;
  Color& operator=(const Color&) = default;
  Color& operator=(Color&&) = default;

  /// Constructor from raw integer rgb values [0, 255]
  /// @param r The red component
  /// @param g The green component
  /// @param b The blue component
  constexpr Color(int r, int g, int b) : rgb{r, g, b} {}

  /// Constructor from array of integer rgb values [0, 255]
  /// @param values The rgb values
  constexpr explicit Color(std::array<int, 3> values) : rgb(values) {}

  /// Constructor from array of double rgb values [0, 1]
  /// @param values The rgb values
  constexpr explicit Color(std::array<double, 3> values) {
    rgb[0] = static_cast<int>(values[0] * 255);
    rgb[1] = static_cast<int>(values[1] * 255);
    rgb[2] = static_cast<int>(values[2] * 255);
  }

  /// Constructor from raw double dgb values [0, 1]
  /// @param r The red component
  /// @param g The green component
  /// @param b The blue component
  constexpr Color(double r, double g, double b)
      : Color{std::array<double, 3>{r, g, b}} {}

 private:
  constexpr static int hexToInt(std::string_view hex) {
    constexpr auto hexCharToInt = [](char c) {
      if (c >= '0' && c <= '9') {
        return c - '0';
      } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
      } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
      } else {
        throw std::invalid_argument("Invalid hex character");
      }
    };

    int value = 0;
    for (char c : hex) {
      value = (value << 4) + hexCharToInt(c);
    }
    return value;
  };

 public:
  /// Constructor from hex string. The expected format is `#RRGGBB`
  /// @param hex The hex string
  constexpr explicit Color(std::string_view hex) {
    if (hex[0] == '#' && hex.size() == 7) {
      rgb[0] = hexToInt(hex.substr(1, 2));  // Extract R component
      rgb[1] = hexToInt(hex.substr(3, 2));  // Extract G component
      rgb[2] = hexToInt(hex.substr(5, 2));  // Extract B component
    } else {
      throw std::invalid_argument{
          "Invalid hex color format. Expected format: #RRGGBB"};
    }
  }

  /// Operator to access the color components
  /// @param i The index of the component
  /// @return The color component
  int operator[](unsigned int i) const { return rgb.at(i); }

  /// Operator to access the color components
  /// @param i The index of the component
  /// @return The color component
  int& operator[](unsigned int i) { return rgb.at(i); }

  /// Operator to compare two colors
  /// @param lhs The first color
  /// @param rhs The second color
  /// @return True if the colors are equal
  friend bool operator==(const Color& lhs, const Color& rhs) = default;
  /// Output stream operator
  /// @param os The output stream
  /// @param color The color to be printed
  /// @return The output stream
  friend std::ostream& operator<<(std::ostream& os, const Color& color) {
    os << "[" << color.rgb[0] << ", " << color.rgb[1] << ", " << color.rgb[2]
       << "]";
    return os;
  }

  std::array<int, 3> rgb{};
};

constexpr Color s_defaultSurfaceColor{"#0000aa"};
constexpr Color s_defaultPortalColor{"#308c48"};
constexpr Color s_defaultVolumColor{"#ffaa00"};

/// @brief Struct to concentrate all visualization configurations
/// in order to harmonize visualization interfaces
struct ViewConfig {
  /// Visible flag
  bool visible = true;
  /// The color for this object
  Color color = {250, 0, 0};
  /// Out of plane drawing parameter for objects
  double offset = 0.1;
  /// The visual line thickness for this object
  double lineThickness = 0.15;
  /// The visual surface thickness for this object
  double surfaceThickness = 0.15;
  /// The number of segments to approximate a quarter of the circle
  unsigned int quarterSegments = 72;
  /// Whether to triangulate or not
  bool triangulate = false;
  /// Write name - non-empty string indicates writing
  std::filesystem::path outputName = std::filesystem::path("");
};

static const ViewConfig s_viewSurface = {.color = {170, 170, 170}};
static const ViewConfig s_viewPortal = {.color = Color{"#308c48"}};
static const ViewConfig s_viewSensitive = {.color = {0, 180, 240}};
static const ViewConfig s_viewPassive = {.color = {240, 280, 0}};
static const ViewConfig s_viewVolume = {.color = {220, 220, 0}};
static const ViewConfig s_viewGrid = {.color = {220, 0, 0}};
static const ViewConfig s_viewLine = {.color = {0, 0, 220}};

}  // namespace Acts

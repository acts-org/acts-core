// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include <actsvg/meta.hpp>

#include <array>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

namespace Acts::Svg {

/// @brief Style struct
struct Style {
  // Fill parameters
  std::array<int, 3> fillColor = {255, 255, 255};
  double fillOpacity = 1.;

  // Highlight parameters
  std::array<int, 3> highlightColor = {0, 0, 0};
  std::vector<std::string> highlights = {};

  double strokeWidth = 0.5;
  std::array<int, 3> strokeColor = {0, 0, 0};

  double highlightStrokeWidth = 2;
  std::array<int, 3> highlightStrokeColor = {0, 0, 0};

  std::vector<int> strokeDasharray = {};

  unsigned int fontSize = 14u;
  std::array<int, 3> fontColor = {0};

  /// Number of segments to approximate a quarter of a circle
  unsigned int quarterSegments = 72u;

  /// Conversion to fill and stroke object from the base library
  /// @return a tuple of actsvg digestable objects
  std::tuple<actsvg::style::fill, actsvg::style::stroke> fillAndStroke() const {
    actsvg::style::fill fll;
    fll._fc._rgb = fillColor;
    fll._fc._opacity = fillOpacity;
    fll._fc._hl_rgb = highlightColor;
    fll._fc._highlight = highlights;

    actsvg::style::stroke str;
    str._sc._rgb = strokeColor;
    str._sc._hl_rgb = highlightStrokeColor;
    str._width = strokeWidth;
    str._hl_width = highlightStrokeWidth;
    str._dasharray = strokeDasharray;

    return {fll, str};
  }

  /// Conversion to fill, stroke and font
  /// @return a tuple of actsvg digestable objects
  std::tuple<actsvg::style::fill, actsvg::style::stroke, actsvg::style::font>
  fillStrokeFont() const {
    auto [fll, str] = fillAndStroke();

    actsvg::style::font fnt;
    fnt._size = fontSize;
    fnt._fc._rgb = fontColor;

    return std::tie(fll, str, fnt);
  }
};

/// Create a group
///
/// @param objects are the individual objects to be grouped
/// @param name is the name of the group
///
/// @return a single svg object as a group
inline static actsvg::svg::object group(
    const std::vector<actsvg::svg::object>& objects, const std::string& name) {
  actsvg::svg::object gr;
  gr._tag = "g";
  gr._id = name;
  for (const auto& o : objects) {
    gr.add_object(o);
  }
  return gr;
}

/// Helper method to a measure
///
/// @param xStart the start position x
/// @param yStart the start position y
/// @param xEnd the end position x
/// @param yEnd the end position y
///
/// @return a single svg object as a measure
inline static actsvg::svg::object measure(double xStart, double yStart,
                                          double xEnd, double yEnd,
                                          const std::string& variable = "",
                                          double value = 0.,
                                          const std::string& unit = "") {
  std::string mlabel = "";
  if (!variable.empty()) {
    mlabel = variable + " = ";
  }
  if (value != 0.) {
    mlabel += actsvg::utils::to_string(static_cast<actsvg::scalar>(value));
  }
  if (!unit.empty()) {
    mlabel += " ";
    mlabel += unit;
  }
  return actsvg::draw::measure(
      "measure",
      {static_cast<actsvg::scalar>(xStart),
       static_cast<actsvg::scalar>(yStart)},
      {static_cast<actsvg::scalar>(xEnd), static_cast<actsvg::scalar>(yEnd)},
      actsvg::style::stroke(), actsvg::style::marker({"o"}),
      actsvg::style::marker({"|<<"}), actsvg::style::font(), mlabel);
}

// Helper method to draw axes
///
/// @param xMin the minimum x value
/// @param xMax the maximum x value
/// @param yMin the minimum y value
/// @param yMax the maximum y value
///
/// @return an svg object
inline static actsvg::svg::object axesXY(double xMin, double xMax, double yMin,
                                         double yMax) {
  return actsvg::draw::x_y_axes(
      "x_y_axis",
      {static_cast<actsvg::scalar>(xMin), static_cast<actsvg::scalar>(xMax)},
      {static_cast<actsvg::scalar>(yMin), static_cast<actsvg::scalar>(yMax)});
}

// Helper method to draw axes
///
/// @param xPos the minimum x value
/// @param yPos the maximum x value
/// @param title the title of the info box
/// @param titleStyle the title of the info box
/// @param info the text of the info box
/// @param infoStyle the style of the info box (body)
/// @param object the connected object
///
/// @return an svg object
inline static actsvg::svg::object infoBox(
    double xPos, double yPos, const std::string& title, const Style& titleStyle,
    const std::vector<std::string>& info, const Style& infoStyle,
    actsvg::svg::object& object,
    const std::vector<std::string>& highlights = {"mouseover", "mouseout"}) {
  auto [titleFill, titleStroke, titleFont] = titleStyle.fillStrokeFont();
  auto [infoFill, infoStroke, infoFont] = infoStyle.fillStrokeFont();

  actsvg::style::stroke stroke;

  return actsvg::draw::connected_info_box(
      object._id + "_infoBox",
      {static_cast<actsvg::scalar>(xPos), static_cast<actsvg::scalar>(yPos)},
      title, titleFill, titleFont, info, infoFill, infoFont, stroke, object,
      highlights);
}

/// Helper method to write to file
///
/// @param objects to be written out
/// @param fileName the file name is to be given
///
inline static void toFile(const std::vector<actsvg::svg::object>& objects,
                          const std::string& fileName) {
  actsvg::svg::file foutFile;

  for (const auto& o : objects) {
    foutFile.add_object(o);
  }

  std::ofstream fout;
  fout.open(fileName);
  fout << foutFile;
  fout.close();
}

}  // namespace Acts::Svg

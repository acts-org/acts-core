// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include <actsvg/meta.hpp>

#include <array>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

namespace Acts::Svg {

struct Style {
  // Fill parameters
  std::array<int, 3> fillColor = {255, 255, 255};
  ActsScalar fillOpacity = 1.;

  // Highlight parameters
  std::array<int, 3> highlightColor = {0, 0, 0};
  std::vector<std::string> highlights = {};

  ActsScalar strokeWidth = 0.5;
  std::array<int, 3> strokeColor = {0, 0, 0};

  ActsScalar highlightStrokeWidth = 2;
  std::array<int, 3> highlightStrokeColor = {0, 0, 0};

  std::vector<int> strokeDasharray = {};

  unsigned int fontSize = 14u;

  unsigned int nSegments = 72u;

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

    return std::tie(fll, str);
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
inline static actsvg::svg::object measure(ActsScalar xStart, ActsScalar yStart,
                                          ActsScalar xEnd, ActsScalar yEnd,
                                          const std::string& variable = "",
                                          ActsScalar value = 0.,
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
inline static actsvg::svg::object axesXY(ActsScalar xMin, ActsScalar xMax,
                                         ActsScalar yMin, ActsScalar yMax) {
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
/// @param info the text of the info box
/// @param infoBoxStyle the style of the info box
/// @param object the connected object
///
/// @return an svg object
inline static actsvg::svg::object infoBox(ActsScalar xPos, ActsScalar yPos,
                                          const std::string& title,
                                          const std::vector<std::string>& info,
                                          const Style& infoBoxStyle,
                                          const actsvg::svg::object& object) {
  auto [fill, stroke] = infoBoxStyle.fillAndStroke();

  actsvg::style::font titleFont;
  titleFont._fc = actsvg::style::color{{255, 255, 255}};
  titleFont._size = infoBoxStyle.fontSize;

  actsvg::style::fill infoFill = fill;
  infoFill._fc._opacity = 0.4;
  actsvg::style::font infoFont;
  infoFont._size = infoBoxStyle.fontSize;

  return actsvg::draw::connected_info_box(
      object._id + "_infoBox",
      {static_cast<actsvg::scalar>(xPos), static_cast<actsvg::scalar>(yPos)},
      title, fill, titleFont, info, infoFill, infoFont, stroke, object);
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

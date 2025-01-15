// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/ActSVG/GridSvgConverter.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "actsvg/display/grids.hpp"

#include <fstream>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

using namespace Acts;
using namespace Acts::detail;

namespace {
/// Helper method to turn a local bin into a string
///
/// @tparam local_bin_t the type for the local bin
/// @param lBin the local bin to printed
///
/// @return a string for screen output
template <typename local_bin_t, std::size_t DIM>
std::string localToString(const local_bin_t& lBin) {
  std::string lbString = "[";
  for (std::size_t ib = 0; ib < DIM; ++ib) {
    if (ib > 0u) {
      lbString += std::string(", ");
    }
    lbString += std::to_string(lBin[ib]);
  }
  lbString += std::string("]");
  return lbString;
}
}  // namespace

BOOST_AUTO_TEST_SUITE(ActSvg)

BOOST_AUTO_TEST_CASE(BoundGridXY) {
  using GlobalBin = std::size_t;
  using LocalBin = std::array<std::size_t, 2u>;

  // x-y axis and grid
  Axis axisX(AxisBound, -200., 200, 4);
  Axis axisY(AxisBound, -200, 200, 6);
  Grid gridXY(Type<std::tuple<GlobalBin, LocalBin>>, axisX, axisY);

  Svg::GridConverter::Options cOptions;
  auto pGrid = Svg::GridConverter::convert(
      gridXY, {AxisDirection::AxisX, AxisDirection::AxisY}, cOptions);
  BOOST_CHECK_EQUAL(pGrid._type, actsvg::proto::grid::type::e_x_y);

  // Labelling the grid tiles
  auto edgesX = axisX.getBinEdges();
  auto edgesY = axisY.getBinEdges();

  std::vector<actsvg::svg::object> targets = {};
  for (auto [ix, x] : Acts::enumerate(edgesX)) {
    if (ix > 0u) {
      double xp = 0.2 * edgesX[ix] + 0.8 * edgesX[ix - 1u];
      for (auto [iy, y] : Acts::enumerate(edgesY)) {
        if (iy > 0u) {
          double yp = 0.8 * edgesY[iy] + 0.2 * edgesY[iy - 1u];
          decltype(gridXY)::point_t p = {xp, yp};
          // Get local and global index
          auto g = gridXY.globalBinFromPosition(p);
          auto l = gridXY.localBinsFromPosition(p);
          std::string gBin = std::string("g = ") + std::to_string(g);
          std::string lBin =
              std::string("l = ") +
              localToString<decltype(gridXY)::index_t, decltype(gridXY)::DIM>(
                  l);
          std::vector<std::string> glBin = {gBin, lBin};
          std::string gBinID = "g_" + std::to_string(g);
          targets.push_back(
              actsvg::draw::text(gBinID,
                                 {static_cast<actsvg::scalar>(xp),
                                  static_cast<actsvg::scalar>(yp)},
                                 glBin));
        }
      }
    }
  }
  pGrid._connections = targets;

  auto oGrid = actsvg::display::grid("BoundGridXY", pGrid);

  // Add some labelling
  actsvg::style::stroke axis_stroke{{{0, 0, 255}}, 3};
  actsvg::style::marker axis_marker{{"<"}, 4, {{{0, 0, 255}}}, axis_stroke};
  actsvg::style::font axis_font{{{0, 0, 255}}, "Andale Mono", 16};

  auto xAxis = actsvg::draw::arrow("x_axis", {0, 0}, {220, 0}, axis_stroke,
                                   actsvg::style::marker({""}), axis_marker);
  auto xLabel = actsvg::draw::text("x_label", {230, -4}, {"x"}, axis_font);
  auto yAxis = actsvg::draw::arrow("y_axis", {0, 0}, {0, 220}, axis_stroke,
                                   actsvg::style::marker({""}), axis_marker);
  auto yLabel = actsvg::draw::text("y_label", {-4, 232}, {"y"}, axis_font);

  std::vector<std::string> captionText = {
      "Binning schema for global and local bins: ",
      "- axis 0 : AxisBoundaryType::Bound, (-200., 200, 4), "
      "AxisDirection::AxisX",
      "- axis 1 : AxisBoundaryType::Bound, (-200, 200, 6), "
      "AxisDirection::AxisY"};

  auto caption = actsvg::draw::text("caption", {-180, -220}, captionText);

  oGrid.add_object(xAxis);
  oGrid.add_object(xLabel);
  oGrid.add_object(yAxis);
  oGrid.add_object(yLabel);
  oGrid.add_object(caption);

  Acts::Svg::toFile({oGrid}, oGrid._id + ".svg");
}

BOOST_AUTO_TEST_CASE(OpenGridXY) {
  using GlobalBin = std::size_t;
  using LocalBin = std::array<std::size_t, 2u>;

  // x-y axis and grid
  Axis axisX(AxisOpen, -200., 200, 4);
  Axis axisY(AxisOpen, -200, 200, 6);
  Grid gridXY(Type<std::tuple<GlobalBin, LocalBin>>, axisX, axisY);

  Svg::GridConverter::Options cOptions;
  auto pGrid = Svg::GridConverter::convert(
      gridXY, {AxisDirection::AxisX, AxisDirection::AxisY}, cOptions);
  BOOST_CHECK_EQUAL(pGrid._type, actsvg::proto::grid::type::e_x_y);

  // Labelling the grid tiles
  auto edgesX = axisX.getBinEdges();
  auto edgesY = axisY.getBinEdges();

  std::vector<actsvg::svg::object> targets = {};
  std::size_t ig = 0;
  for (auto [ix, x] : Acts::enumerate(edgesX)) {
    if (ix > 0u) {
      double xp = 0.2 * edgesX[ix] + 0.8 * edgesX[ix - 1u];
      for (auto [iy, y] : Acts::enumerate(edgesY)) {
        if (iy > 0u) {
          double yp = 0.8 * edgesY[iy] + 0.2 * edgesY[iy - 1u];
          decltype(gridXY)::point_t p = {xp, yp};
          // Get local and global index
          auto g = gridXY.globalBinFromPosition(p);
          auto l = gridXY.localBinsFromPosition(p);
          std::string gBin = std::string("g = ") + std::to_string(g);
          std::string lBin =
              std::string("l = ") +
              localToString<decltype(gridXY)::index_t, decltype(gridXY)::DIM>(
                  l);
          std::vector<std::string> glBin = {gBin, lBin};
          std::string gBinID = "g_" + std::to_string(ig++);
          targets.push_back(
              actsvg::draw::text(gBinID,
                                 {static_cast<actsvg::scalar>(xp),
                                  static_cast<actsvg::scalar>(yp)},
                                 glBin));
        }
      }
    }
  }
  pGrid._connections = targets;

  // Add some labelling
  actsvg::style::stroke axis_stroke{{{0, 0, 255}}, 3};
  actsvg::style::marker axis_marker{{"<"}, 4, {{{0, 0, 255}}}, axis_stroke};
  actsvg::style::font axis_font{{{0, 0, 255}}, "Andale Mono", 16};

  auto xAxis = actsvg::draw::arrow("x_axis", {0, 0}, {220, 0}, axis_stroke,
                                   actsvg::style::marker({""}), axis_marker);
  auto xLabel = actsvg::draw::text("x_label", {230, -4}, {"x"}, axis_font);
  auto yAxis = actsvg::draw::arrow("y_axis", {0, 0}, {0, 220}, axis_stroke,
                                   actsvg::style::marker({""}), axis_marker);
  auto yLabel = actsvg::draw::text("y_label", {-4, 232}, {"y"}, axis_font);

  std::vector<std::string> captionText = {
      "Binning schema for global and local bins: ",
      "- axis 0 : AxisBoundaryType::Open, (-200., 200, 4), "
      "AxisDirection::AxisX",
      "- axis 1 : AxisBoundaryType::Open, (-200, 200, 6), "
      "AxisDirection::AxisY"};

  auto caption = actsvg::draw::text("caption", {-180, -220}, captionText);
  auto oGrid = actsvg::display::grid("OpenGridXY", pGrid);

  oGrid.add_object(xAxis);
  oGrid.add_object(xLabel);
  oGrid.add_object(yAxis);
  oGrid.add_object(yLabel);
  oGrid.add_object(caption);

  Acts::Svg::toFile({oGrid}, oGrid._id + ".svg");
}

BOOST_AUTO_TEST_CASE(ClosedCylinderGridZPhi) {
  using GlobalBin = std::size_t;
  using LocalBin = std::array<std::size_t, 2u>;

  // z-phi Axes & Grid
  Axis axisZ(AxisBound, -200., 200., 3);
  Axis axisPhi(AxisClosed, -std::numbers::pi, std::numbers::pi, 6);
  Grid gridZPhi(Type<std::tuple<GlobalBin, LocalBin>>, axisZ, axisPhi);

  Svg::GridConverter::Options cOptions;
  auto pGrid = Svg::GridConverter::convert(
      gridZPhi, {AxisDirection::AxisZ, AxisDirection::AxisPhi}, cOptions);
  BOOST_CHECK_EQUAL(pGrid._type, actsvg::proto::grid::type::e_z_phi);

  pGrid._reference_r = 80.;

  // Labelling the grid tiles
  auto edgesZ = axisZ.getBinEdges();
  auto edgesPhi = axisPhi.getBinEdges();

  std::vector<actsvg::svg::object> targets = {};
  std::size_t ig = 0;
  for (auto [iz, z] : Acts::enumerate(edgesZ)) {
    if (iz > 0u) {
      double zp = 0.2 * edgesZ[iz] + 0.8 * edgesZ[iz - 1u];
      for (auto [iphi, phi] : Acts::enumerate(edgesPhi)) {
        if (iphi > 0u) {
          double phip = 0.8 * edgesPhi[iphi] + 0.2 * edgesPhi[iphi - 1u];
          decltype(gridZPhi)::point_t p = {zp, phip};
          // Get local and global index
          auto g = gridZPhi.globalBinFromPosition(p);
          auto l = gridZPhi.localBinsFromPosition(p);
          std::string gBin = std::string("g = ") + std::to_string(g);
          std::string lBin =
              std::string("l = ") + localToString<decltype(gridZPhi)::index_t,
                                                  decltype(gridZPhi)::DIM>(l);
          std::vector<std::string> glBin = {gBin, lBin};
          std::string gBinID = "g_" + std::to_string(ig++);
          targets.push_back(actsvg::draw::text(
              gBinID,
              {static_cast<actsvg::scalar>(zp),
               static_cast<actsvg::scalar>(pGrid._reference_r * phip)},
              glBin));
        }
      }
    }
  }
  pGrid._connections = targets;

  // Add some labelling
  actsvg::style::stroke axis_stroke{{{0, 0, 255}}, 3};
  actsvg::style::marker axis_marker{{"<"}, 4, {{{0, 0, 255}}}, axis_stroke};
  actsvg::style::font axis_font{{{0, 0, 255}}, "Andale Mono", 16};

  auto xAxis = actsvg::draw::arrow("x_axis", {0, 0}, {220, 0}, axis_stroke,
                                   actsvg::style::marker({""}), axis_marker);
  auto xLabel = actsvg::draw::text("x_label", {230, -4}, {"z"}, axis_font);
  auto yAxis = actsvg::draw::arrow("y_axis", {0, 0}, {0, 260}, axis_stroke,
                                   actsvg::style::marker({""}), axis_marker);
  auto yLabel = actsvg::draw::text("y_label", {-4, 270}, {"phi"}, axis_font);

  std::vector<std::string> captionText = {
      "Binning schema for global and local bins: ",
      "- axis 0 : AxisBoundaryType::Bound, (-200., 200, 3), "
      "AxisDirection::AxisZ",
      "- axis 1 : AxisBoundaryType::Closed, (-PI, PI, 6), "
      "AxisDirection::AxisPhi",
      "- draw reference radius set to 80"};

  auto caption = actsvg::draw::text("caption", {-180, -270}, captionText);
  auto oGrid = actsvg::display::grid("ClosedCylinderGridZPhi", pGrid);

  oGrid.add_object(xAxis);
  oGrid.add_object(xLabel);
  oGrid.add_object(yAxis);
  oGrid.add_object(yLabel);
  oGrid.add_object(caption);

  Acts::Svg::toFile({oGrid}, oGrid._id + ".svg");
}

BOOST_AUTO_TEST_CASE(ClosedDiscGridRPhi) {
  using GlobalBin = std::size_t;
  using LocalBin = std::array<std::size_t, 2u>;

  // r-phi Axes & Grid
  Axis axisR(AxisBound, 100., 400., 3);
  Axis axisPhi(AxisClosed, -std::numbers::pi, std::numbers::pi, 4);
  Grid gridRPhi(Type<std::tuple<GlobalBin, LocalBin>>, axisR, axisPhi);

  Svg::GridConverter::Options cOptions;
  auto pGrid = Svg::GridConverter::convert(
      gridRPhi, {AxisDirection::AxisR, AxisDirection::AxisPhi}, cOptions);
  BOOST_CHECK_EQUAL(pGrid._type, actsvg::proto::grid::type::e_r_phi);

  // Labelling the grid tiles
  auto edgesR = axisR.getBinEdges();
  auto edgesPhi = axisPhi.getBinEdges();

  std::vector<actsvg::svg::object> targets = {};
  std::size_t ig = 0;
  for (auto [ir, r] : Acts::enumerate(edgesR)) {
    if (ir > 0u) {
      double rp = 0.5 * (edgesR[ir] + edgesR[ir - 1u]);
      for (auto [iphi, phi] : Acts::enumerate(edgesPhi)) {
        if (iphi > 0u) {
          double phip = 0.5 * (edgesPhi[iphi] + edgesPhi[iphi - 1u]);
          decltype(gridRPhi)::point_t p = {rp, phip};
          // Get local and global index
          auto g = gridRPhi.globalBinFromPosition(p);
          auto l = gridRPhi.localBinsFromPosition(p);
          std::string gBin = std::string("g = ") + std::to_string(g);
          std::string lBin =
              std::string("l = ") + localToString<decltype(gridRPhi)::index_t,
                                                  decltype(gridRPhi)::DIM>(l);
          std::vector<std::string> glBin = {gBin, lBin};
          std::string gBinID = "g_" + std::to_string(ig++);
          targets.push_back(
              actsvg::draw::text(gBinID,
                                 {static_cast<actsvg::scalar>(rp * cos(phip)),
                                  static_cast<actsvg::scalar>(rp * sin(phip))},
                                 glBin));
        }
      }
    }
  }
  pGrid._connections = targets;

  // Add some labelling
  actsvg::style::stroke axis_stroke{{{0, 0, 255}}, 3};
  actsvg::style::marker axis_marker{{"<"}, 4, {{{0, 0, 255}}}, axis_stroke};
  actsvg::style::font axis_font{{{0, 0, 255}}, "Andale Mono", 16};

  auto rAxis = actsvg::draw::arrow("r_axis", {0, 0}, {420, 0}, axis_stroke,
                                   actsvg::style::marker({""}), axis_marker);
  auto rLabel = actsvg::draw::text("r_label", {440, -4}, {"r"}, axis_font);

  auto phiAxis = actsvg::draw::arc_measure(
      "phi_axis", 410., {410, 0.},
      {static_cast<actsvg::scalar>(410. * cos(0.25)),
       static_cast<actsvg::scalar>(410. * sin(0.25))},
      axis_stroke, actsvg::style::marker(), axis_marker);

  auto phiLabel =
      actsvg::draw::text("phi_label", {410, 60}, {"phi"}, axis_font);

  std::vector<std::string> captionText = {
      "Binning schema for global and local bins: ",
      "- axis 0 : AxisBoundaryType::Bound, (100., 400, 3), AxisR",
      "- axis 1 : AxisBoundaryType::Closed, (-PI, PI, 4), "
      "binPhi"};

  auto caption = actsvg::draw::text("caption", {-180, -420}, captionText);
  auto oGrid = actsvg::display::grid("ClosedDiscGridRPhi", pGrid);

  oGrid.add_object(rAxis);
  oGrid.add_object(rLabel);
  oGrid.add_object(phiAxis);
  oGrid.add_object(phiLabel);
  oGrid.add_object(caption);

  Acts::Svg::toFile({oGrid}, oGrid._id + ".svg");
}

BOOST_AUTO_TEST_SUITE_END()

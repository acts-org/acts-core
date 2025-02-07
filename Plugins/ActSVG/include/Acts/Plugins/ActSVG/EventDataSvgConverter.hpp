// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/Logger.hpp"
#include <actsvg/core.hpp>

namespace Acts::Svg::EventDataConverter {

/// Write/create a 3D point in XY view
///
/// @param pos the position
/// @param size the size of the object
/// @param style the style of the object
/// @param idx the running index
///
/// @return a vector of svg objects
actsvg::svg::object pointXY(const Vector3& pos, double size, const Style& style,
                            unsigned int idx = 0);

/// Write/create a 3D point in ZR view
///
/// @param pos the position
/// @param size the size of the object
/// @param style the style of the object
/// @param indx the running index
///
/// @return a vector of svg objects
actsvg::svg::object pointZR(const Vector3& pos, double size, const Style& style,
                            unsigned int idx = 0);

/// Write/create a 3D point in a given view
///
/// @param pos the position
/// @param size the size of the object
/// @param style the style of the object
/// @param indx the running index
///
/// @return a vector of svg objects
template <typename view_type>
actsvg::svg::object point(const Vector3& pos, double size, const Style& style,
                          unsigned int idx) {
  view_type view;
  std::vector<Vector3> ps = {pos};
  auto ppos = view(ps)[0];
  auto [fill, stroke] = style.fillAndStroke();
  auto circle =
      actsvg::draw::circle("p_" + std::to_string(idx), ppos,
                           static_cast<actsvg::scalar>(size), fill, stroke);
  return circle;
}

}  // namespace Acts::Svg::EventDataConverter

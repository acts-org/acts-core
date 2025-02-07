// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/ActSVG/EventDataSvgConverter.hpp"

actsvg::svg::object Acts::Svg::EventDataConverter::pointXY(const Vector3& pos,
                                                           double size,
                                                           const Style& style,
                                                           unsigned int idx) {
  return point<actsvg::views::x_y>(pos, size, style, idx);
}

actsvg::svg::object Acts::Svg::EventDataConverter::pointZR(const Vector3& pos,
                                                           double size,
                                                           const Style& style,
                                                           unsigned int idx) {
  return point<actsvg::views::z_r>(pos, size, style, idx);
}

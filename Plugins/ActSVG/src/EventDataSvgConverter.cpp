// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/EventDataSvgConverter.hpp"

actsvg::svg::object Acts::Svg::EventDataConverter::pointXY(const Vector3& pos,
                                                           ActsScalar size,
                                                           const Style& style,
                                                           unsigned int idx) {
  return point<actsvg::views::x_y>(pos, size, style, idx);
}

actsvg::svg::object Acts::Svg::EventDataConverter::pointZR(const Vector3& pos,
                                                           ActsScalar size,
                                                           const Style& style,
                                                           unsigned int idx) {
  return point<actsvg::views::z_r>(pos, size, style, idx);
}

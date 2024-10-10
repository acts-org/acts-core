// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <fstream>

namespace ActsFatras {

/// Helper to write out Digitization tesbed into csv files.
struct DigitizationCsvOutput {
  /// Helper method to format the output of a line into csv
  ///
  /// @param outf The output stream
  /// @param p0 The start point
  /// @param p1 The end point
  void writeLine(std::ostream& outf, const Acts::Vector2& p0,
                 const Acts::Vector2& p1) const {
    outf << "l," << p0.x() << "," << p0.y() << "," << p1.x() << "," << p1.y()
         << "\n";
  };

  /// Helper method to write an arc into csv
  ///
  /// @param outf The output stream
  /// @param r The radius of the arc
  /// @param phiMin The minimum phi
  /// @param phiMin The maximum phi
  void writeArc(std::ostream& outf, double r, double phiMin,
                double phiMax) const {
    outf << "a," << r << "," << r << "," << phiMin << "," << phiMax << "\n";
  };

  /// Helper method to write a polygon
  ///
  /// @param outf The output stream
  /// @param vertices The vertices of the polygon
  void writePolygon(std::ostream& outf,
                    const std::vector<Acts::Vector2>& vertices,
                    const Acts::Vector2& shift = Acts::Vector2(0., 0.)) const {
    auto nvertices = vertices.size();
    for (unsigned long iv = 1; iv < nvertices; ++iv) {
      writeLine(outf, vertices[iv - 1] + shift, vertices[iv] + shift);
    }
    writeLine(outf, vertices[nvertices - 1] + shift, vertices[0] + shift);
  };
};

}  // namespace ActsFatras

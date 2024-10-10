// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/MagneticField/FieldMapTextIo.hpp"

#include "Acts/MagneticField/BFieldMapUtils.hpp"

#include <fstream>
#include <vector>

namespace {
constexpr std::size_t kDefaultSize = 1 << 15;
}

ActsExamples::detail::InterpolatedMagneticField2
ActsExamples::makeMagneticFieldMapRzFromText(
    const std::function<std::size_t(std::array<std::size_t, 2> binsRZ,
                                    std::array<std::size_t, 2> nBinsRZ)>&
        localToGlobalBin,
    const std::string& fieldMapFile, Acts::ActsScalar lengthUnit,
    Acts::ActsScalar BFieldUnit, bool firstQuadrant) {
  /// [1] Read in field map file
  // Grid position points in r and z
  std::vector<double> rPos;
  std::vector<double> zPos;
  // components of magnetic field on grid points
  std::vector<Acts::Vector2> bField;
  // reserve estimated size
  rPos.reserve(kDefaultSize);
  zPos.reserve(kDefaultSize);
  bField.reserve(kDefaultSize);
  // [1] Read in file and fill values
  std::ifstream map_file(fieldMapFile.c_str(), std::ios::in);
  std::string line;
  double r = 0., z = 0.;
  double br = 0., bz = 0.;
  while (std::getline(map_file, line)) {
    if (line.empty() || line[0] == '%' || line[0] == '#' ||
        line.find_first_not_of(' ') == std::string::npos) {
      continue;
    }

    std::istringstream tmp(line);
    tmp >> r >> z >> br >> bz;
    rPos.push_back(r);
    zPos.push_back(z);
    bField.push_back(Acts::Vector2(br, bz));
  }
  map_file.close();
  rPos.shrink_to_fit();
  zPos.shrink_to_fit();
  bField.shrink_to_fit();
  /// [2] use helper function in core
  return Acts::fieldMapRZ(localToGlobalBin, rPos, zPos, bField, lengthUnit,
                          BFieldUnit, firstQuadrant);
}

ActsExamples::detail::InterpolatedMagneticField3
ActsExamples::makeMagneticFieldMapXyzFromText(
    const std::function<std::size_t(std::array<std::size_t, 3> binsXYZ,
                                    std::array<std::size_t, 3> nBinsXYZ)>&
        localToGlobalBin,
    const std::string& fieldMapFile, Acts::ActsScalar lengthUnit,
    Acts::ActsScalar BFieldUnit, bool firstOctant) {
  /// [1] Read in field map file
  // Grid position points in x, y and z
  std::vector<double> xPos;
  std::vector<double> yPos;
  std::vector<double> zPos;
  // components of magnetic field on grid points
  std::vector<Acts::Vector3> bField;
  // reserve estimated size
  xPos.reserve(kDefaultSize);
  yPos.reserve(kDefaultSize);
  zPos.reserve(kDefaultSize);
  bField.reserve(kDefaultSize);
  // [1] Read in file and fill values
  std::ifstream map_file(fieldMapFile.c_str(), std::ios::in);
  std::string line;
  double x = 0., y = 0., z = 0.;
  double bx = 0., by = 0., bz = 0.;
  while (std::getline(map_file, line)) {
    if (line.empty() || line[0] == '%' || line[0] == '#' ||
        line.find_first_not_of(' ') == std::string::npos) {
      continue;
    }

    std::istringstream tmp(line);
    tmp >> x >> y >> z >> bx >> by >> bz;
    xPos.push_back(x);
    yPos.push_back(y);
    zPos.push_back(z);
    bField.push_back(Acts::Vector3(bx, by, bz));
  }
  map_file.close();
  xPos.shrink_to_fit();
  yPos.shrink_to_fit();
  zPos.shrink_to_fit();
  bField.shrink_to_fit();
  return Acts::fieldMapXYZ(localToGlobalBin, xPos, yPos, zPos, bField,
                           lengthUnit, BFieldUnit, firstOctant);
}

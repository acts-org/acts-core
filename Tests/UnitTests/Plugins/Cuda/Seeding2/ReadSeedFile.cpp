// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Local include(s).
#include "ReadSeedFile.hpp"

// System include(s).
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

std::vector<std::unique_ptr<TestSpacePoint> > readSeedFile(
    const std::string& fileName, bool filterDuplicates) {
  // The result object.
  std::vector<std::unique_ptr<TestSpacePoint> > result;

  // Open the input file.
  std::ifstream spFile(fileName);
  if (!spFile.is_open()) {
    throw std::runtime_error("Could not open file: " + fileName);
  }

  // Read the file's lines one by one, and create spacepoint objects out of
  // them.
  std::size_t duplicatesFound = 0;
  while (!spFile.eof()) {
    std::string line;
    std::getline(spFile, line);
    std::stringstream ss(line);
    std::string linetype;

    ss >> linetype;
    if (linetype != "lxyz") {
      continue;
    }

    int layer;
    float x, y, z, varianceR, varianceZ;
    ss >> layer >> x >> y >> z >> varianceR >> varianceZ;
    const float r = std::hypot(x, y);
    const float f22 = varianceR;
    const float wid = varianceZ;
    float cov = wid * wid * .08333;

    if (cov < f22) {
      cov = f22;
    }
    if (std::abs(z) > 450.) {
      varianceZ = 9. * cov;
      varianceR = .06;
    } else {
      varianceR = 9. * cov;
      varianceZ = .06;
    }

    // Create a new spacepoint object.
    std::unique_ptr<TestSpacePoint> sp(
        new TestSpacePoint{x, y, z, r, layer, varianceR, varianceZ});

    // Check if we already have another spacepoint with the same coordinates.
    if (filterDuplicates) {
      bool discardSP = false;
      for (const auto& otherSP : result) {
        if (*sp == *otherSP) {
          ++duplicatesFound;
          discardSP = true;
          break;
        }
      }
      if (discardSP) {
        continue;
      }
    }

    // Store the new spacepoint.
    result.push_back(std::move(sp));
  }
  // Tell the user how many duplicates were found.
  if (duplicatesFound) {
    std::cerr << duplicatesFound << " duplicates found in: " << fileName
              << std::endl;
  }

  return result;
}

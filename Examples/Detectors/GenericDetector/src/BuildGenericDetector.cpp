// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/GenericDetector/BuildGenericDetector.hpp"

#include <cmath>

namespace ActsExamples::Generic {

/// helper method for cylinder
std::vector<Acts::Vector3> modulePositionsCylinder(
    double radius, double zStagger, double moduleHalfLength, double lOverlap,
    const std::pair<int, int>& binningSchema) {
  int nPhiBins = binningSchema.first;
  int nZbins = binningSchema.second;
  // prepare the return value
  std::vector<Acts::Vector3> mPositions;
  mPositions.reserve(nPhiBins * nZbins);
  // prep work
  double phiStep = 2 * M_PI / nPhiBins;
  double minPhi = -M_PI + 0.5 * phiStep;
  double zStart = -0.5 * (nZbins - 1) * (2 * moduleHalfLength - lOverlap);
  double zStep = 2 * std::abs(zStart) / (nZbins - 1);
  // loop over the bins
  for (std::size_t zBin = 0; zBin < static_cast<std::size_t>(nZbins); ++zBin) {
    // prepare z and r
    double moduleZ = zStart + zBin * zStep;
    double moduleR =
        (zBin % 2) != 0u ? radius - 0.5 * zStagger : radius + 0.5 * zStagger;
    for (std::size_t phiBin = 0; phiBin < static_cast<std::size_t>(nPhiBins);
         ++phiBin) {
      // calculate the current phi value
      double modulePhi = minPhi + phiBin * phiStep;
      mPositions.push_back(Acts::Vector3(moduleR * cos(modulePhi),
                                         moduleR * sin(modulePhi), moduleZ));
    }
  }
  return mPositions;
}

/// helper method for disc
std::vector<std::vector<Acts::Vector3>> modulePositionsDisc(
    double z, double ringStagger, std::vector<double> phiStagger,
    std::vector<double> phiSubStagger, double innerRadius, double outerRadius,
    const std::vector<std::size_t>& discBinning,
    const std::vector<double>& moduleHalfLength) {
  // calculate the radii
  std::vector<double> radii;
  // the radial span of the disc
  double deltaR = outerRadius - innerRadius;
  // quick exits
  if (discBinning.size() == 1) {
    radii.push_back(0.5 * (innerRadius + outerRadius));
  } else {
    double totalLength = 0;
    // sum up the total length
    for (auto& mhlength : moduleHalfLength) {
      totalLength += 2 * mhlength;
    }
    // now calculate the overlap (equal pay)
    double rOverlap = (totalLength - deltaR) / (moduleHalfLength.size() - 1);
    // and now fill the radii and gaps
    double lastR = innerRadius;
    double lastHl = 0.;
    double lastOl = 0.;
    // now calculate
    for (auto& mhlength : moduleHalfLength) {
      // calculate the radius
      radii.push_back(lastR + lastHl - lastOl + mhlength);
      lastR = radii[radii.size() - 1];
      lastOl = rOverlap;
      lastHl = mhlength;
    }
  }
  // now prepare the return method
  std::vector<std::vector<Acts::Vector3>> mPositions;
  for (std::size_t ir = 0; ir < radii.size(); ++ir) {
    // generate the z value
    // convention inner ring is closer to origin : makes sense
    double rz = radii.size() == 1 ? z
                                  : ((ir % 2) != 0u ? z + 0.5 * ringStagger
                                                    : z - 0.5 * ringStagger);
    // fill the ring positions
    double psStagger = phiSubStagger.empty() ? 0. : phiSubStagger[ir];
    mPositions.push_back(modulePositionsRing(rz, radii[ir], phiStagger[ir],
                                             psStagger, discBinning[ir]));
  }
  return mPositions;
}

/// Helper method for positioning
std::vector<Acts::Vector3> modulePositionsRing(double z, double radius,
                                               double phiStagger,
                                               double phiSubStagger,
                                               int nPhiBins) {
  // create and fill the positions
  std::vector<Acts::Vector3> rPositions;
  rPositions.reserve(nPhiBins);
  // prep work
  double phiStep = 2 * M_PI / nPhiBins;
  double minPhi = -M_PI + 0.5 * phiStep;
  // phi loop
  for (std::size_t iphi = 0; iphi < static_cast<std::size_t>(nPhiBins);
       ++iphi) {
    // if we have a phi sub stagger presents
    double rzs = 0.;
    // phi stagger affects 0 vs 1, 2 vs 3 ... etc
    // -> only works if it is a %4
    // phi sub stagger affects 2 vs 4, 1 vs 3 etc.
    if (phiSubStagger != 0. && ((nPhiBins % 4) == 0)) {
      // switch sides
      if ((iphi % 4) == 0u) {
        rzs = phiSubStagger;
      } else if (((iphi + 1) % 4) == 0u) {
        rzs = -phiSubStagger;
      }
    }
    // the module phi
    double phi = minPhi + iphi * phiStep;
    // main z position depending on phi bin
    double rz = (iphi % 2) != 0u ? z - 0.5 * phiStagger : z + 0.5 * phiStagger;
    rPositions.push_back(
        Acts::Vector3(radius * cos(phi), radius * sin(phi), rz + rzs));
  }
  return rPositions;
}

}  // namespace ActsExamples::Generic

// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/detail/Axis.hpp"

#include <memory>

template <typename SpacePoint>
std::unique_ptr<Acts::SpacePointGrid<SpacePoint>>
Acts::SpacePointGridCreator::createGrid(
    const Acts::SpacePointGridConfig& _config) {
  Acts::SpacePointGridConfig config = _config.toInternalUnits();
  using AxisScalar = Acts::Vector3::Scalar;

  int phiBins;
  // for no magnetic field, create 100 phi-bins
  if (config.bFieldInZ == 0) {
    phiBins = 100;
  } else {
    // calculate circle intersections of helix and max detector radius
    float minHelixRadius =
        config.minPt /
        (300. * config.bFieldInZ);  // in mm -> R[mm] =pT[GeV] / (3·10−4×B[T]) =
                                    // pT[MeV] / (300 *Bz[kT])
    float maxR2 = config.rMax * config.rMax;
    float xOuter = maxR2 / (2 * minHelixRadius);
    float yOuter = std::sqrt(maxR2 - xOuter * xOuter);
    float outerAngle = std::atan(xOuter / yOuter);
    // intersection of helix and max detector radius minus maximum R distance
    // from middle SP to top SP
    float innerAngle = 0;
    float rMin = config.rMax;
    if (config.rMax > config.deltaRMax) {
      rMin = config.rMax - config.deltaRMax;
      float innerCircleR2 =
          (config.rMax - config.deltaRMax) * (config.rMax - config.deltaRMax);
      float xInner = innerCircleR2 / (2 * minHelixRadius);
      float yInner = std::sqrt(innerCircleR2 - xInner * xInner);
      innerAngle = std::atan(xInner / yInner);
    }

    // evaluating the azimutal deflection including the maximum impact parameter
    float deltaAngleWithMaxD0 =
        std::abs(std::asin(config.impactMax / (rMin)) -
                 std::asin(config.impactMax / config.rMax));

    // evaluating delta Phi based on the inner and outer angle, and the azimutal
    // deflection including the maximum impact parameter
    float deltaPhi = (outerAngle - innerAngle + deltaAngleWithMaxD0);

    // divide 2pi by angle delta to get number of phi-bins
    // size is always 2pi even for regions of interest
    phiBins = std::ceil(2 * M_PI / deltaPhi);
    // need to scale the number of phi bins accordingly to the number of
    // consecutive phi bins in the seed making step.
    // Each individual bin should be approximately a fraction (depending on this
    // number) of the maximum expected azimutal deflection.
  }

  Acts::detail::Axis<detail::AxisType::Equidistant,
                     detail::AxisBoundaryType::Closed>
      phiAxis(-M_PI, M_PI, phiBins);

  // vector that will store the edges of the bins of z
  std::vector<AxisScalar> zValues;

  // If zBinEdges is not defined, calculate the edges as zMin + bin * zBinSize
  if (config.zBinEdges.empty()) {
    // TODO: can probably be optimized using smaller z bins
    // and returning (multiple) neighbors only in one z-direction for forward
    // seeds
    // FIXME: zBinSize must include scattering
    float zBinSize = config.cotThetaMax * config.deltaRMax;
    int zBins = std::floor((config.zMax - config.zMin) / zBinSize);

    for (int bin = 0; bin <= zBins; bin++) {
      AxisScalar edge = config.zMin + bin * zBinSize;
      zValues.push_back(edge);
    }

  } else {
    // Use the zBinEdges defined in the config
    for (auto& bin : config.zBinEdges) {
      zValues.push_back(bin);
    }
  }

  detail::Axis<detail::AxisType::Variable, detail::AxisBoundaryType::Bound>
      zAxis(zValues);
  return std::make_unique<Acts::SpacePointGrid<SpacePoint>>(
      std::make_tuple(phiAxis, zAxis));
}

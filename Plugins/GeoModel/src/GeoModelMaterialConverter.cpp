// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelMaterialConverter.hpp"

#include "Acts/Material/Material.hpp"

#include "GeoModelKernel/GeoMaterial.h"
#include "GeoModelKernel/Units.h"
namespace {
constexpr double s_densityCnvFactor = 1. / GeoModelKernelUnits::gram;
}
Acts::Material Acts::GeoModel::geoMaterialConverter(const GeoMaterial& gm,
                                                    bool useMolarDensity) {
  double x0 = gm.getRadLength();
  double l0 = gm.getIntLength();
  double density = gm.getDensity() * s_densityCnvFactor;
  double A = 0.;
  double Z = 0.;
  // Get number elements
  int numberOfElements = gm.getNumElements();
  // Loop
  for (int iEl = 0; iEl < numberOfElements; ++iEl) {
    const GeoElement* geoEl = gm.getElement(iEl);
    double fraction = gm.getFraction(iEl);
    A += fraction * (geoEl->getA() / GeoModelKernelUnits::gram);
    Z += fraction * geoEl->getZ();
  }
  if (useMolarDensity) {
    return Material::fromMolarDensity(x0, l0, A, Z, density);
  } else {
    return Material::fromMassDensity(x0, l0, A, Z, density);
  }
}

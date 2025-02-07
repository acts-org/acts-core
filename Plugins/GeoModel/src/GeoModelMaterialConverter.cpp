// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/TGeo/TGeoMaterialConverter.hpp"

#include "Acts/Material/Material.hpp"

#include "TGeoMaterial.h"

Acts::MaterialSlab Acts::TGeoMaterialConverter::materialSlab(
    const TGeoMaterial& tgMaterial, double thicknessIn, double thicknessOut,
    const Options& options) {
  // Scalable properties
  double matX0 = tgMaterial.GetRadLen();
  double matL0 = tgMaterial.GetIntLen();
  double matRho = tgMaterial.GetDensity();

  // X0, L0, rho scale with the thickness
  double cFactor = thicknessIn / thicknessOut;
  double uScalor = options.unitLengthScalor;
  double rScalar = options.unitMassScalor / pow(options.unitLengthScalor, 3);

  auto material = Material::fromMassDensity(
      matX0 * uScalor / cFactor, matL0 * uScalor / cFactor, tgMaterial.GetA(),
      tgMaterial.GetZ(), matRho * rScalar * cFactor);

  return MaterialSlab(material, thicknessOut);
}

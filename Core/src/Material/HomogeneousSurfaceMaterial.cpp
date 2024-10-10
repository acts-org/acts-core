// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"

#include "Acts/Material/MaterialSlab.hpp"

#include <ostream>

Acts::HomogeneousSurfaceMaterial::HomogeneousSurfaceMaterial(
    const MaterialSlab& full, double splitFactor, Acts::MappingType mappingType)
    : ISurfaceMaterial(splitFactor, mappingType), m_fullMaterial(full) {}

Acts::HomogeneousSurfaceMaterial& Acts::HomogeneousSurfaceMaterial::operator*=(
    double scale) {
  m_fullMaterial.scaleThickness(scale);
  return (*this);
}

std::ostream& Acts::HomogeneousSurfaceMaterial::toStream(
    std::ostream& sl) const {
  sl << "Acts::HomogeneousSurfaceMaterial : " << std::endl;
  sl << "   - fullMaterial : " << m_fullMaterial << std::endl;
  sl << "   - split factor : " << m_splitFactor << std::endl;
  return sl;
}

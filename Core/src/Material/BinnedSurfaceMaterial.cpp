// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/BinnedSurfaceMaterial.hpp"

#include "Acts/Material/MaterialSlab.hpp"

#include <ostream>
#include <utility>
#include <vector>

Acts::BinnedSurfaceMaterial::BinnedSurfaceMaterial(
    const BinUtility& binUtility, MaterialSlabVector fullProperties,
    double splitFactor, Acts::MappingType mappingType)
    : ISurfaceMaterial(splitFactor, mappingType), m_binUtility(binUtility) {
  // fill the material with deep copy
  m_fullMaterial.push_back(std::move(fullProperties));
}

Acts::BinnedSurfaceMaterial::BinnedSurfaceMaterial(
    const BinUtility& binUtility, MaterialSlabMatrix fullProperties,
    double splitFactor, Acts::MappingType mappingType)
    : ISurfaceMaterial(splitFactor, mappingType),
      m_binUtility(binUtility),
      m_fullMaterial(std::move(fullProperties)) {}

Acts::BinnedSurfaceMaterial& Acts::BinnedSurfaceMaterial::operator*=(
    double scale) {
  for (auto& materialVector : m_fullMaterial) {
    for (auto& materialBin : materialVector) {
      materialBin.scaleThickness(scale);
    }
  }
  return (*this);
}

const Acts::MaterialSlab& Acts::BinnedSurfaceMaterial::materialSlab(
    const Vector2& lp) const {
  // the first bin
  std::size_t ibin0 = m_binUtility.bin(lp, 0);
  std::size_t ibin1 = m_binUtility.max(1) != 0u ? m_binUtility.bin(lp, 1) : 0;
  return m_fullMaterial[ibin1][ibin0];
}

const Acts::MaterialSlab& Acts::BinnedSurfaceMaterial::materialSlab(
    const Acts::Vector3& gp) const {
  // the first bin
  std::size_t ibin0 = m_binUtility.bin(gp, 0);
  std::size_t ibin1 = m_binUtility.max(1) != 0u ? m_binUtility.bin(gp, 1) : 0;
  return m_fullMaterial[ibin1][ibin0];
}

std::ostream& Acts::BinnedSurfaceMaterial::toStream(std::ostream& sl) const {
  sl << "Acts::BinnedSurfaceMaterial : " << std::endl;
  sl << "   - Number of Material bins [0,1] : " << m_binUtility.max(0) + 1
     << " / " << m_binUtility.max(1) + 1 << std::endl;
  sl << "   - Parse full update material    : " << std::endl;  //
  // output  the full material
  unsigned int imat1 = 0;
  for (auto& materialVector : m_fullMaterial) {
    unsigned int imat0 = 0;
    // the vector iterator
    for (auto& materialBin : materialVector) {
      sl << " Bin [" << imat1 << "][" << imat0 << "] - " << (materialBin);
      ++imat0;
    }
    ++imat1;
  }
  sl << "  - BinUtility: " << m_binUtility << std::endl;
  return sl;
}

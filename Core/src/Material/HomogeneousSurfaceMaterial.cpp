// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"

#include "Acts/Material/MaterialSlab.hpp"

#include <ostream>

namespace Acts {

HomogeneousSurfaceMaterial::HomogeneousSurfaceMaterial(const MaterialSlab& full,
                                                       double splitFactor,
                                                       MappingType mappingType)
    : ISurfaceMaterial(splitFactor, mappingType), m_fullMaterial(full) {}

HomogeneousSurfaceMaterial& HomogeneousSurfaceMaterial::scale(double factor) {
  m_fullMaterial.scaleThickness(factor);
  return *this;
}

const MaterialSlab& HomogeneousSurfaceMaterial::materialSlab(
    const Vector2& /*lp*/) const {
  return m_fullMaterial;
}

const MaterialSlab& HomogeneousSurfaceMaterial::materialSlab(
    const Vector3& /*gp*/) const {
  return m_fullMaterial;
}

std::ostream& HomogeneousSurfaceMaterial::toStream(std::ostream& sl) const {
  sl << "HomogeneousSurfaceMaterial : " << std::endl;
  sl << "   - fullMaterial : " << m_fullMaterial << std::endl;
  sl << "   - split factor : " << m_splitFactor << std::endl;
  return sl;
}

}  // namespace Acts

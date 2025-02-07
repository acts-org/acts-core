// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Material/HomogeneousVolumeMaterial.hpp"

#include "Acts/Material/Material.hpp"

#include <ostream>

namespace Acts {

HomogeneousVolumeMaterial::HomogeneousVolumeMaterial(const Material& material)
    : m_material(material) {}

const Material HomogeneousVolumeMaterial::material(
    const Vector3& /*position*/) const {
  return m_material;
}

std::ostream& HomogeneousVolumeMaterial::toStream(std::ostream& sl) const {
  sl << "HomogeneousVolumeMaterial : " << std::endl;
  sl << "   - material : " << m_material << std::endl;
  return sl;
}

}  // namespace Acts

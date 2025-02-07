// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Material/ProtoVolumeMaterial.hpp"

#include <ostream>

Acts::ProtoVolumeMaterial::ProtoVolumeMaterial(const BinUtility& binUtility)
    : m_binUtility(binUtility), m_material() {}

std::ostream& Acts::ProtoVolumeMaterial::toStream(std::ostream& sl) const {
  sl << "Acts::ProtoVolumeMaterial : " << std::endl;
  if (m_binUtility.bins(0) * m_binUtility.bins(1) * m_binUtility.bins(2) > 1) {
    sl << "   - Number of Material bins [0,1] : " << m_binUtility.bins(0)
       << " / " << m_binUtility.bins(1) << " / " << m_binUtility.bins(2)
       << std::endl;
  } else {
    sl << "   - Homogeneous Material" << std::endl;
  }
  return sl;
}

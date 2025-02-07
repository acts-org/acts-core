// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Material/AccumulatedVolumeMaterial.hpp"

#include "Acts/Material/detail/AverageMaterials.hpp"

void Acts::AccumulatedVolumeMaterial::accumulate(const MaterialSlab& mat) {
  m_average = detail::combineSlabs(m_average, mat);
}

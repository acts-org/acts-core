// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/MaterialSlab.hpp"

#include "Acts/Material/detail/AverageMaterials.hpp"

#include <limits>
#include <numeric>
#include <ostream>
#include <stdexcept>

namespace Acts {

namespace {
static constexpr auto eps = 2 * std::numeric_limits<float>::epsilon();
}

MaterialSlab::MaterialSlab(float thickness) : m_thickness(thickness) {}

MaterialSlab::MaterialSlab(const Material& material, float thickness)
    : m_material(material),
      m_thickness(thickness),
      m_thicknessInX0((eps < material.X0()) ? (thickness / material.X0()) : 0),
      m_thicknessInL0((eps < material.L0()) ? (thickness / material.L0()) : 0) {
  if (thickness < 0) {
    throw std::runtime_error("thickness < 0");
  }
}

MaterialSlab MaterialSlab::averageLayers(const MaterialSlab& layerA,
                                         const MaterialSlab& layerB) {
  return detail::combineSlabs(layerA, layerB);
}

MaterialSlab MaterialSlab::averageLayers(
    const std::vector<MaterialSlab>& layers) {
  // NOTE 2020-08-26 msmk
  //   the reduce work best (in the numerical stability sense) if the input
  //   layers are sorted by thickness/mass density. then, the later terms
  //   of the averaging are only small corrections to the large average of
  //   the initial layers. this could be enforced by sorting the layers first,
  //   but I am not sure if this is actually a problem.
  // NOTE yes, this loop is exactly like std::reduce which apparently does not
  //   exist on gcc 8 although it is required by C++17.
  MaterialSlab result;
  for (const auto& layer : layers) {
    result = detail::combineSlabs(result, layer);
  }
  return result;
}

void MaterialSlab::scaleThickness(float scale) {
  if (scale < 0) {
    throw std::runtime_error("scale < 0");
  }

  m_thickness *= scale;
  m_thicknessInX0 *= scale;
  m_thicknessInL0 *= scale;
}

std::ostream& operator<<(std::ostream& os, const MaterialSlab& materialSlab) {
  os << materialSlab.material() << "|t=" << materialSlab.thickness();
  return os;
}

}  // namespace Acts

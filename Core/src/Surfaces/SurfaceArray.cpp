// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/SurfaceArray.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <utility>

// implementation for pure virtual destructor of ISurfaceGridLookup
Acts::SurfaceArray::ISurfaceGridLookup::~ISurfaceGridLookup() = default;

Acts::SurfaceArray::SurfaceArray(
    std::unique_ptr<ISurfaceGridLookup> gridLookup,
    std::vector<std::shared_ptr<const Surface>> surfaces,
    const Transform3& transform)
    : p_gridLookup(std::move(gridLookup)),
      m_surfaces(std::move(surfaces)),
      m_surfacesRawPointers(unpack_shared_vector(m_surfaces)),
      m_transform(transform) {}

Acts::SurfaceArray::SurfaceArray(std::shared_ptr<const Surface> srf)
    : p_gridLookup(
          static_cast<ISurfaceGridLookup*>(new SingleElementLookup(srf.get()))),
      m_surfaces({std::move(srf)}) {
  m_surfacesRawPointers.push_back(m_surfaces.at(0).get());
}

std::ostream& Acts::SurfaceArray::toStream(const GeometryContext& /*gctx*/,
                                           std::ostream& sl) const {
  sl << std::fixed << std::setprecision(4);
  sl << "SurfaceArray:" << std::endl;
  sl << " - no surfaces: " << m_surfaces.size() << std::endl;
  sl << " - grid dim:    " << p_gridLookup->dimensions() << std::endl;

  auto axes = p_gridLookup->getAxes();

  for (std::size_t j = 0; j < axes.size(); ++j) {
    AxisBoundaryType bdt = axes.at(j)->getBoundaryType();
    sl << " - axis " << (j + 1) << std::endl;
    sl << "   - boundary type: ";
    if (bdt == AxisBoundaryType::Open) {
      sl << "open";
    }
    if (bdt == AxisBoundaryType::Bound) {
      sl << "bound";
    }
    if (bdt == AxisBoundaryType::Closed) {
      sl << "closed";
    }
    sl << std::endl;
    sl << "   - type: "
       << (axes.at(j)->isEquidistant() ? "equidistant" : "variable")
       << std::endl;
    sl << "   - n bins: " << axes.at(j)->getNBins() << std::endl;
    sl << "   - bin edges: [ ";
    auto binEdges = axes.at(j)->getBinEdges();
    for (std::size_t i = 0; i < binEdges.size(); ++i) {
      if (i > 0) {
        sl << ", ";
      }
      auto binEdge = binEdges.at(i);
      // Do not display negative zeroes
      sl << ((std::abs(binEdge) >= 5e-4) ? binEdge : 0.0);
    }
    sl << " ]" << std::endl;
  }
  return sl;
}

// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/NavigationLayer.hpp"

#include "Acts/Surfaces/SurfaceArray.hpp"

Acts::NavigationLayer::NavigationLayer(
    std::shared_ptr<const Surface> surfaceRepresentation, double thickness)
    : Acts::Layer(nullptr),
      m_surfaceRepresentation(std::move(surfaceRepresentation)) {
  m_layerThickness = thickness;
  m_layerType = navigation;
}

Acts::NavigationLayer::~NavigationLayer() = default;

// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/PassiveLayerBuilder.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"

#include <cstddef>
#include <ostream>
#include <utility>

namespace Acts {
class DiscBounds;
}  // namespace Acts

Acts::PassiveLayerBuilder::PassiveLayerBuilder(
    const PassiveLayerBuilder::Config& plConfig,
    std::unique_ptr<const Logger> logger)
    : m_cfg(), m_logger(std::move(logger)) {
  setConfiguration(plConfig);
}

void Acts::PassiveLayerBuilder::setConfiguration(
    const PassiveLayerBuilder::Config& plConfig) {
  //!< @todo add configuration check
  m_cfg = plConfig;
}

void Acts::PassiveLayerBuilder::setLogger(
    std::unique_ptr<const Logger> newLogger) {
  m_logger = std::move(newLogger);
}

const Acts::LayerVector Acts::PassiveLayerBuilder::positiveLayers(
    const GeometryContext& gctx) const {
  return endcapLayers(gctx, 1);
}

const Acts::LayerVector Acts::PassiveLayerBuilder::negativeLayers(
    const GeometryContext& gctx) const {
  return endcapLayers(gctx, -1);
}

const Acts::LayerVector Acts::PassiveLayerBuilder::endcapLayers(
    const Acts::GeometryContext& /*gctx*/, int side) const {
  LayerVector eLayers;
  // pos/neg layers
  std::size_t numpnLayers = m_cfg.posnegLayerPositionZ.size();
  if (numpnLayers != 0u) {
    ACTS_DEBUG("Configured to build " << numpnLayers
                                      << " passive layers on side :" << side);
    eLayers.reserve(numpnLayers);
    // loop through
    for (std::size_t ipnl = 0; ipnl < numpnLayers; ++ipnl) {
      // some screen output
      ACTS_VERBOSE("- build layers "
                   << (ipnl)
                   << " at  = " << side * m_cfg.posnegLayerPositionZ.at(ipnl)
                   << " and rMin/rMax = " << m_cfg.posnegLayerRmin.at(ipnl)
                   << " / " << m_cfg.posnegLayerRmax.at(ipnl));
      // create the share disc bounds
      std::shared_ptr<const DiscBounds> dBounds =
          std::make_shared<const RadialBounds>(m_cfg.posnegLayerRmin.at(ipnl),
                                               m_cfg.posnegLayerRmax.at(ipnl));
      // create the layer transforms
      const Transform3 eTransform(
          Translation3(0., 0., side * m_cfg.posnegLayerPositionZ.at(ipnl)));
      // create the layers
      MutableLayerPtr eLayer = DiscLayer::create(
          eTransform, dBounds, nullptr, m_cfg.posnegLayerThickness.at(ipnl));

      // assign the material to the layer surface
      std::shared_ptr<const ISurfaceMaterial> material = nullptr;
      // create the material from jobOptions
      if (!m_cfg.posnegLayerMaterial.empty()) {
        // create homogeneous material
        material = m_cfg.posnegLayerMaterial.at(ipnl);
        // sign it to the surface
        eLayer->surfaceRepresentation().assignSurfaceMaterial(material);
      }
      // push it into the layer vector
      eLayers.push_back(eLayer);
    }
  }
  return eLayers;
}

const Acts::LayerVector Acts::PassiveLayerBuilder::centralLayers(
    const Acts::GeometryContext& /*gctx*/) const {
  LayerVector cLayers;
  // the central layers
  std::size_t numcLayers = m_cfg.centralLayerRadii.size();
  if (numcLayers != 0u) {
    ACTS_DEBUG("Configured to build " << numcLayers
                                      << " passive central layers.");
    cLayers.reserve(numcLayers);
    // loop through
    for (std::size_t icl = 0; icl < numcLayers; ++icl) {
      // some screen output
      ACTS_VERBOSE("- build layer "
                   << icl
                   << " with radius = " << m_cfg.centralLayerRadii.at(icl)
                   << " and halfZ = " << m_cfg.centralLayerHalflengthZ.at(icl));
      // create the layer and push it back
      auto cBounds = std::make_shared<const CylinderBounds>(
          m_cfg.centralLayerRadii[icl], m_cfg.centralLayerHalflengthZ.at(icl));
      // create the layer
      MutableLayerPtr cLayer =
          CylinderLayer::create(Transform3::Identity(), cBounds, nullptr,
                                m_cfg.centralLayerThickness.at(icl));
      // assign the material to the layer surface
      std::shared_ptr<const ISurfaceMaterial> material = nullptr;
      // create the material from jobOptions
      if (!m_cfg.centralLayerMaterial.empty()) {
        // create homogeneous material
        material = m_cfg.centralLayerMaterial.at(icl);
        // sign it to the surface
        cLayer->surfaceRepresentation().assignSurfaceMaterial(material);
      }
      // push it into the layer vector
      cLayers.push_back(cLayer);
    }
  }
  return cLayers;
}

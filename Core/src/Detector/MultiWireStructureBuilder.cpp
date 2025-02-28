// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/MultiWireStructureBuilder.hpp"

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/DetectorVolumeBuilder.hpp"
#include "Acts/Detector/VolumeStructureBuilder.hpp"
#include "Acts/Detector/detail/IndexedSurfacesGenerator.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

class MultiWireInternalStructureBuilder
    : public Acts::Experimental::IInternalStructureBuilder {
 public:
  struct Config {
    /// The internal surfaces
    std::vector<std::shared_ptr<Acts::Surface>> iSurfaces;

    /// Definition of Binning
    std::vector<std::tuple<Acts::ProtoAxis, std::size_t>> binning;

    /// Extra information, mainly for screen output
    std::string auxiliary = "";

    /// The transform into the local binning schema
    Acts::Transform3 transform = Acts::Transform3::Identity();
  };

  // Constructor
  explicit MultiWireInternalStructureBuilder(
      const Config& cfg,
      std::unique_ptr<const Acts::Logger> mlogger = Acts::getDefaultLogger(
          "MUltiWireInternalBuilder", Acts::Logging::INFO))
      : Acts::Experimental::IInternalStructureBuilder(),
        m_cfg(cfg),
        m_logger(std::move(mlogger)) {}

  Acts::Experimental::InternalStructure construct(
      const Acts::GeometryContext& gctx) const final {
    if (!m_cfg.auxiliary.empty()) {
      ACTS_DEBUG(m_cfg.auxiliary);
    }

    Acts::Experimental::ExternalNavigationDelegate internalVolumeUpdater =
        Acts::Experimental::tryNoVolumes();

    if (m_cfg.binning.size() < 2) {
      throw std::runtime_error(
          "MultiWireStructureBuilder: At least 2 binning axes required");
    }

    auto [protoAxisA, expansionA] = m_cfg.binning.at(0);
    auto [protoAxisB, expansionB] = m_cfg.binning.at(1);

    const auto& iaxisA = protoAxisA.getAxis();
    const auto& iaxisB = protoAxisB.getAxis();
    // Binning needs to be equidistant
    if (iaxisA.getType() != Acts::AxisType::Equidistant ||
        iaxisB.getType() != Acts::AxisType::Equidistant) {
      throw std::runtime_error(
          "MultiWireStructureBuilder: Binning axes need to be equidistant");
    }

    Acts::Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Bound>
        axisA(iaxisA.getBinEdges().front(), iaxisA.getBinEdges().back(),
              iaxisA.getNBins());

    Acts::Axis<Acts::AxisType::Equidistant, Acts::AxisBoundaryType::Bound>
        axisB(iaxisB.getBinEdges().front(), iaxisB.getBinEdges().back(),
              iaxisB.getNBins());

    Acts::Grid<std::vector<std::size_t>, decltype(axisA), decltype(axisB)> grid(
        axisA, axisB);

    // Prepare the indexed updator
    std::array<Acts::AxisDirection, 2u> axisDirs = {
        protoAxisA.getAxisDirection(), protoAxisB.getAxisDirection()};
    Acts::Experimental::MultiLayerSurfacesNavigation<decltype(grid)>
        indexedSurfaces(std::move(grid), axisDirs, m_cfg.transform);

    std::vector<std::size_t> fillExpansion = {expansionA, expansionB};

    Acts::Experimental::detail::CenterReferenceGenerator rGenerator;
    Acts::Experimental::detail::IndexedGridFiller filler{fillExpansion};
    filler.fill(gctx, indexedSurfaces, m_cfg.iSurfaces, rGenerator, {});

    Acts::Experimental::InternalNavigationDelegate sfCandidatesUpdater;

    // The portal delegate
    Acts::Experimental::AllPortalsNavigation allPortals;

    // The chained delegate: indexed surfaces and all portals
    using DelegateType =
        Acts::Experimental::IndexedSurfacesAllPortalsNavigation<
            decltype(grid), Acts::Experimental::MultiLayerSurfacesNavigation>;
    auto indexedSurfacesAllPortals = std::make_unique<const DelegateType>(
        std::tie(allPortals, indexedSurfaces));

    // Create the delegate and connect it
    sfCandidatesUpdater.connect<&DelegateType::update>(
        std::move(indexedSurfacesAllPortals));

    return {m_cfg.iSurfaces,
            {},
            std::move(sfCandidatesUpdater),
            std::move(internalVolumeUpdater)};
  }

 private:
  /// Configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Acts::Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

Acts::Experimental::MultiWireStructureBuilder::MultiWireStructureBuilder(
    const Acts::Experimental::MultiWireStructureBuilder::Config& config,
    std::unique_ptr<const Acts::Logger> logger)
    : mCfg(config), mLogger(std::move(logger)) {
  // check if the surfaces are set
  if (mCfg.mlSurfaces.empty()) {
    throw std::invalid_argument(
        "MultiWireStructureBuilder: No surfaces are given");
  } else if (mCfg.mlBinning.size() != 2u) {
    throw ::std::invalid_argument(
        "MultiWireStructureBuilder: Invalid binning provided");
  }
}

Acts::Experimental::DetectorComponent
Acts::Experimental::MultiWireStructureBuilder::construct(
    const Acts::GeometryContext& gctx) {
  // Configure the external structure builder for the internal structure
  Acts::Experimental::VolumeStructureBuilder::Config vsConfig;
  vsConfig.boundsType = Acts::VolumeBounds::eTrapezoid;
  vsConfig.transform = mCfg.transform;
  vsConfig.boundValues = mCfg.mlBounds;
  vsConfig.auxiliary = "Construct External Structure";

  // Configure the internal structure builder for the internal structure
  MultiWireInternalStructureBuilder::Config iConfig;
  iConfig.iSurfaces = mCfg.mlSurfaces;
  iConfig.binning = mCfg.mlBinning;
  iConfig.transform = mCfg.transform.inverse();
  iConfig.auxiliary = "Construct Internal Structure";

  Acts::Experimental::DetectorVolumeBuilder::Config dvConfig;
  dvConfig.auxiliary = "Construct Detector Volume";
  dvConfig.name = mCfg.name;
  dvConfig.internalsBuilder =
      std::make_shared<MultiWireInternalStructureBuilder>(
          iConfig,
          Acts::getDefaultLogger("MultiWire Internal Structure Builder",
                                 Acts::Logging::VERBOSE));
  dvConfig.externalsBuilder =
      std::make_shared<Acts::Experimental::VolumeStructureBuilder>(
          vsConfig, Acts::getDefaultLogger("VolumeStructureBuilder",
                                           Acts::Logging::VERBOSE));
  auto dvBuilder = std::make_shared<Acts::Experimental::DetectorVolumeBuilder>(
      dvConfig,
      Acts::getDefaultLogger("DetectorVolumeBuilder", Acts::Logging::VERBOSE));

  auto dvComponent = dvBuilder->construct(gctx);

  return dvComponent;
}

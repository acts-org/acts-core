// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/DD4hepDetectorStructure.hpp"

#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Detector/DetectorBuilder.hpp"
#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/detail/BlueprintDrawer.hpp"
#include "Acts/Detector/detail/BlueprintHelper.hpp"
#include "Acts/Plugins/DD4hep/DD4hepBlueprintFactory.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorSurfaceFactory.hpp"
#include "Acts/Plugins/DD4hep/DD4hepLayerStructure.hpp"

#include <fstream>

#include <DD4hep/DetElement.h>

Acts::Experimental::DD4hepDetectorStructure::DD4hepDetectorStructure(
    std::unique_ptr<const Logger> mlogger)
    : m_logger(std::move(mlogger)) {}

std::tuple<std::shared_ptr<const Acts::Experimental::Detector>,
           Acts::DD4hepDetectorElement::Store>
Acts::Experimental::DD4hepDetectorStructure::construct(
    const GeometryContext& gctx, const dd4hep::DetElement& dd4hepElement,
    const Options& options) const {
  ACTS_DEBUG("Building detector from " << dd4hepElement.name());

  // Return objects
  std::shared_ptr<const Detector> detector = nullptr;
  DD4hepDetectorElement::Store detectorStore;

  // Set up the tools
  auto surfaceFactory = std::make_shared<DD4hepDetectorSurfaceFactory>(
      getDefaultLogger("DD4hepDetectorSurfaceFactory", options.logLevel));

  auto layerStructure = std::make_shared<DD4hepLayerStructure>(
      std::move(surfaceFactory),
      getDefaultLogger("DD4hepLayerStructure", options.logLevel));

  // Draw the blue print from the dd4hep detector element tree
  DD4hepBlueprintFactory::Config bpdCfg{layerStructure};
  DD4hepBlueprintFactory::Cache bpdCache;

  DD4hepBlueprintFactory dd4hepBlueprintDrawer(
      bpdCfg, getDefaultLogger("DD4hepBlueprintFactory", options.logLevel));
  auto dd4hepBlueprint =
      dd4hepBlueprintDrawer.create(bpdCache, gctx, dd4hepElement);
  detectorStore = bpdCache.dd4hepStore;

  // Draw the raw graph
  if (!options.emulateToGraph.empty()) {
    ACTS_DEBUG("Writing the initial bluepring to file before gap filling.");
    std::ofstream bpi(options.emulateToGraph + "_initial.dot");
    detail::BlueprintDrawer::dotStream(bpi, *dd4hepBlueprint);
    bpi.close();
  }

  if (dd4hepBlueprint->boundsType == VolumeBounds::eCylinder) {
    ACTS_DEBUG("Cylindrical detector building detected.");

    // Now fill the gaps
    detail::BlueprintHelper::fillGaps(*dd4hepBlueprint);

    // Draw the synchronized graph
    if (!options.emulateToGraph.empty()) {
      ACTS_DEBUG("Writing the final bluepring to file.");
      std::ofstream bpf(options.emulateToGraph + "_final.dot");
      detail::BlueprintDrawer::dotStream(bpf, *dd4hepBlueprint);
      bpf.close();
      // Return without building
      return std::tie(detector, detectorStore);
    }

    // Create a Cylindrical detector builder from this blueprint
    auto detectorBuilder = std::make_shared<CylindricalContainerBuilder>(
        *dd4hepBlueprint, options.logLevel);

    // Detector builder
    DetectorBuilder::Config dCfg;
    dCfg.auxiliary =
        "*** DD4hep : auto generated cylindrical detector builder  ***";
    dCfg.name = "Cylindrical detector from DD4hep blueprint";
    dCfg.builder = detectorBuilder;
    dCfg.geoIdGenerator = options.geoIdGenerator != nullptr
                              ? options.geoIdGenerator
                              : dd4hepBlueprint->geoIdGenerator;
    dCfg.materialDecorator = options.materialDecorator;
    detector = DetectorBuilder(dCfg, getDefaultLogger("DD4hepDetectorBuilder",
                                                      options.logLevel))
                   .construct(gctx);
  } else {
    throw std::invalid_argument(
        "DD4hepDetectorStructure: Only cylindrical detectors are (currently) "
        "supported.");
  }
  return std::tie(detector, detectorStore);
}

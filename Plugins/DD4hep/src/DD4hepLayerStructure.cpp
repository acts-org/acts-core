// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/DD4hepLayerStructure.hpp"

#include "Acts/Utilities/BinningType.hpp"

Acts::Experimental::DD4hepLayerStructure::DD4hepLayerStructure(
    std::shared_ptr<DD4hepDetectorSurfaceFactory> surfaceFactory,
    std::unique_ptr<const Logger> logger)
    : m_surfaceFactory(std::move(surfaceFactory)), m_logger(std::move(logger)) {
  if (m_surfaceFactory == nullptr) {
    throw std::invalid_argument(
        "DD4hepLayerStructure: no surface factory provided");
  }
}

std::tuple<std::shared_ptr<Acts::Experimental::LayerStructureBuilder>,
           std::optional<Acts::Extent>>
Acts::Experimental::DD4hepLayerStructure::builder(
    DD4hepDetectorElement::Store& dd4hepStore, const GeometryContext& gctx,
    const dd4hep::DetElement& dd4hepElement, const Options& options) const {
  // Check for misconfiguration with double naming
  if (dd4hepStore.contains(options.name)) {
    std::string reMessage = "DD4hepLayerStructure: structure with name '";
    reMessage += options.name;
    reMessage += "' already registered in DetectorElementStore";
    throw std::runtime_error(reMessage.c_str());
  }

  // This object is going to be filled with the created surfaces
  DD4hepDetectorSurfaceFactory::Cache fCache;
  fCache.sExtent = options.extent;
  fCache.pExtent = options.extent;
  fCache.extentConstraints = options.extentConstraints;
  fCache.nExtentQSegments = options.quarterSegments;
  m_surfaceFactory->construct(fCache, gctx, dd4hepElement,
                              options.conversionOptions);

  ACTS_DEBUG("Conversion from DD4Hep : " << fCache.sensitiveSurfaces.size()
                                         << " sensitive surfaces");

  ACTS_DEBUG("Conversion from DD4Hep : " << fCache.passiveSurfaces.size()
                                         << " passive surfaces");

  // Check if binning was provided or detected
  if (fCache.binnings.empty() &&
      (fCache.sensitiveSurfaces.size() + fCache.passiveSurfaces.size()) > 0u) {
    ACTS_VERBOSE(
        "Surface binning neither provided nor found, navigation will be "
        "'tryAll' (could result in slow navigation).");
  }

  // Surfaces are prepared for creating the builder
  LayerStructureBuilder::Config lsbConfig;
  lsbConfig.auxiliary = "*** DD4hep auto-generated builder for: ";
  lsbConfig.extent = fCache.sExtent;
  lsbConfig.auxiliary += options.name;

  // Patch the binning to the extent parameters
  if (fCache.sExtent.has_value() && options.patchBinningWithExtent) {
    const auto& extent = fCache.sExtent.value();
    // Check if the binning
    ACTS_VERBOSE("Checking if surface binning ranges can be patched.");
    for (auto& b : fCache.binnings) {
      if (extent.constrains(b.binValue)) {
        ACTS_VERBOSE("Binning '" << binningValueName(b.binValue)
                                 << "' is patched.");
        ACTS_VERBOSE(" <- from : [" << b.edges.front() << ", " << b.edges.back()
                                    << "]");
        b.edges.front() = extent.min(b.binValue);
        b.edges.back() = extent.max(b.binValue);
        ACTS_VERBOSE(" -> to   : [" << b.edges.front() << ", " << b.edges.back()
                                    << "]");
      }
    }
  }

  // Translate binings and supports
  lsbConfig.binnings = fCache.binnings;
  lsbConfig.supports = fCache.supports;

  std::vector<std::shared_ptr<Surface>> lSurfaces;
  lSurfaces.reserve(fCache.sensitiveSurfaces.size() +
                    fCache.passiveSurfaces.size());

  std::vector<std::shared_ptr<DD4hepDetectorElement>> cElements;
  cElements.reserve(fCache.sensitiveSurfaces.size());

  // Fill them in to the surface provider struct and detector store
  for (const auto& [de, ds] : fCache.sensitiveSurfaces) {
    lSurfaces.push_back(ds);
    cElements.push_back(de);
  }
  dd4hepStore[options.name] = cElements;

  // Passive surfaces to be added
  for (const auto& [ps, toAll] : fCache.passiveSurfaces) {
    // Passive surface is not declared to be added to all navigation bins
    if (!toAll) {
      lSurfaces.push_back(ps);
    } else {
      // Passive surface is indeed declared to be added to all navigaiton bins
      Experimental::ProtoSupport pSupport;
      pSupport.surface = ps;
      pSupport.assignToAll = true;
      lsbConfig.supports.push_back(pSupport);
    }
  }

  // Create the surface provider from the layer structure
  lsbConfig.surfacesProvider =
      std::make_shared<Experimental::LayerStructureBuilder::SurfacesHolder>(
          lSurfaces);

  ACTS_DEBUG("Configured with " << lsbConfig.binnings.size() << " binnings.");
  ACTS_DEBUG("Configured to build " << lsbConfig.supports.size()
                                    << " supports.");

  // Make one common extent
  if (fCache.sExtent.has_value() && fCache.pExtent.has_value()) {
    ACTS_DEBUG(
        "Sensitive extent determined: " << fCache.sExtent.value().toString());
    ACTS_DEBUG(
        "Passive   extent determined: " << fCache.pExtent.value().toString());
    fCache.sExtent.value().extend(fCache.pExtent.value());
  }

  // Return the structure builder
  return {std::make_shared<LayerStructureBuilder>(
              lsbConfig, getDefaultLogger(options.name, options.logLevel)),
          fCache.sExtent};
}

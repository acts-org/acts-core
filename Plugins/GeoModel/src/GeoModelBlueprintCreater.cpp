// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelBlueprintCreater.hpp"

#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/detail/BlueprintDrawer.hpp"
#include "Acts/Detector/detail/BlueprintHelper.hpp"
#include "Acts/Detector/interface/IGeometryIdGenerator.hpp"
#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoModelBinningHelper.hpp"
#include "Acts/Plugins/GeoModel/detail/GeoModelExtentHelper.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <fstream>

#include <boost/algorithm/string.hpp>

using namespace Acts::detail;

Acts::GeoModelBlueprintCreater::GeoModelBlueprintCreater(
    const Config& cfg, std::unique_ptr<const Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {}

Acts::GeoModelBlueprintCreater::Blueprint
Acts::GeoModelBlueprintCreater::create(const GeometryContext& gctx,
                                       const GeoModelTree& gmTree,
                                       const Options& options) const {
  // The blueprint to be created
  Acts::GeoModelBlueprintCreater::Blueprint blueprint;

  // The GeoModel tree must have a reader
  if (gmTree.geoReader == nullptr) {
    throw std::invalid_argument(
        "GeoModelBlueprintCreater: GeoModelTree has no GeoModelReader");
  }

  auto blueprintTable =
      gmTree.geoReader->getTableFromTableName_String(options.table);

  // Prepare the map
  std::map<std::string, TableEntry> blueprintTableMap;

  Cache cache;
  // Prepare the KdtSurfaces if configured to do so
  //
  if (!m_cfg.detectorSurfaces.empty()) {
    std::array<AxisDirection, 3u> kdtBinning = {
        AxisDirection::AxisX, AxisDirection::AxisY, AxisDirection::AxisZ};
    if (m_cfg.kdtBinning.empty()) {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: At least one binning value for KDTree "
          "structure has to be given");
    } else if (m_cfg.kdtBinning.size() == 1u) {
      kdtBinning = {m_cfg.kdtBinning[0u], m_cfg.kdtBinning[0u],
                    m_cfg.kdtBinning[0u]};
    } else if (m_cfg.kdtBinning.size() == 2u) {
      kdtBinning = {m_cfg.kdtBinning[0u], m_cfg.kdtBinning[1u],
                    m_cfg.kdtBinning[1u]};
    } else if (m_cfg.kdtBinning.size() == 3u) {
      kdtBinning = {m_cfg.kdtBinning[0u], m_cfg.kdtBinning[1u],
                    m_cfg.kdtBinning[2u]};
    } else {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: Too many binning values for KDTree "
          "structure");
    }

    // Create the KdtSurfaces
    cache.kdtSurfaces = std::make_shared<Experimental::KdtSurfaces<3u>>(
        gctx, m_cfg.detectorSurfaces, kdtBinning);
  }

  // In order to guarantee the correct order
  // of the building sequence we need to build a representative tree first
  for (const auto& line : blueprintTable) {
    if (line.size() != 7u) {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: Blueprint table has wrong number of "
          "columns");
    }

    int volumeId = std::stoi(line.at(0));
    std::string volumeType = line.at(1);
    std::string volumeName = line.at(2);
    // The bit more complicated strings from the database
    // volume bounds of top volume might be overruled for top node
    std::string volumeBounds =
        (!options.topBoundsOverride.empty() && volumeName == options.topEntry)
            ? options.topBoundsOverride
            : line.at(3);
    std::vector<std::string> volumeInternals;
    boost::split(volumeInternals, line.at(4), boost::is_any_of(":"));
    std::vector<std::string> volumeBinnings;
    boost::split(volumeBinnings, line.at(5), boost::is_any_of(";"));
    std::vector<std::string> volumeMaterials;
    boost::split(volumeMaterials, line.at(6), boost::is_any_of("|"));

    // Split the bounds on the deliminater
    ACTS_DEBUG("Creating (" << volumeType << ") Blueprint node for volume "
                            << volumeName << " (id: " << volumeId << ")");

    // Create a table entry per defined volume
    TableEntry entry{volumeId,       volumeType,      volumeName,
                     volumeBounds,   volumeInternals, volumeBinnings,
                     volumeMaterials};

    // This will guarantee to have access to the building
    blueprintTableMap[volumeName] = entry;
  }

  // Now we can build the tree
  auto topEntry = blueprintTableMap.find(options.topEntry);
  if (topEntry == blueprintTableMap.end()) {
    throw std::invalid_argument("GeoModelBlueprintCreater: Top node '" +
                                options.topEntry +
                                "' not found in blueprint table");
  }

  // Recursively create the nodes
  blueprint.name = topEntry->second.name;
  blueprint.topNode =
      createNode(cache, gctx, topEntry->second, blueprintTableMap, Extent());

  // Export to dot graph if configured
  if (!options.dotGraph.empty()) {
    std::ofstream dotFile(options.dotGraph);
    Experimental::detail::BlueprintDrawer::dotStream(dotFile,
                                                     *blueprint.topNode);
    dotFile.close();
  }

  // Return the ready-to-use blueprint
  return blueprint;
}

std::unique_ptr<Acts::Experimental::Blueprint::Node>
Acts::GeoModelBlueprintCreater::createNode(
    Cache& cache, const GeometryContext& gctx, const TableEntry& entry,
    const std::map<std::string, TableEntry>& tableEntryMap,
    const Extent& motherExtent) const {
  ACTS_DEBUG("Build Blueprint node for '" << entry.name << "'.");

  // Peak into the volume entry to understand which one should be constraint
  // by the internals building
  std::vector<AxisDirection> internalConstraints =
      detail::GeoModelExentHelper::readBoundsConstaints(entry.bounds, "i");
  // Check if the binnning will also use the internal constraints
  std::vector<AxisDirection> binningConstraints =
      detail::GeoModelExentHelper::readBinningConstraints(entry.binnings);
  // Concatenate the binning constraints
  for (const auto& bc : binningConstraints) {
    if (!rangeContainsValue(internalConstraints, bc)) {
      internalConstraints.push_back(bc);
    }
  }

  if (!internalConstraints.empty()) {
    ACTS_VERBOSE("Found " << internalConstraints.size()
                          << " internal constraints to check for: ");
    for (const auto& ic : internalConstraints) {
      ACTS_VERBOSE("- " << axisDirectionName(ic));
    }
  }

  // Create and return the container node with internal constrtins
  auto [internalsBuilder, internalExtent] = createInternalStructureBuilder(
      cache, gctx, entry, motherExtent, internalConstraints);

  if (internalsBuilder != nullptr) {
    ACTS_VERBOSE("Internal building yielded extent "
                 << internalExtent.toString());
  }

  // Parse the bounds
  auto [boundsType, extent, boundValues, translation] =
      parseBounds(entry.bounds, motherExtent, internalExtent);

  ACTS_VERBOSE("Creating with extent " << extent.toString());

  Transform3 transform = Acts::Transform3::Identity();
  transform.translation() = translation;

  std::vector<std::string> entryTypeSplit;
  boost::split(entryTypeSplit, entry.type, boost::is_any_of(":"));
  std::string entryType = entryTypeSplit[0u];

  // Check if material has to be attached
  std::map<unsigned int, Experimental::BinningDescription>
      portalMaterialBinning;
  if (!entry.materials.empty()) {
    for (const auto& material : entry.materials) {
      std::vector<std::string> materialTokens;
      boost::split(materialTokens, material, boost::is_any_of(":"));
      ACTS_DEBUG(" - Material detected for " << materialTokens[0u]);
      auto pPos = materialTokens[0u].find("p");
      if (pPos != std::string::npos) {
        // Erase the p
        materialTokens[0u].erase(pPos, 1);
        // Get the portal number
        unsigned int portalNumber = std::stoi(materialTokens[0u]);
        // Get the binning description - first split the string
        std::vector<std::string> binningTokens;
        boost::split(binningTokens, materialTokens[1u], boost::is_any_of(";"));

        std::vector<Experimental::ProtoBinning> protoBinnings;
        for (const auto& bToken : binningTokens) {
          ACTS_VERBOSE("   - Binning: " << bToken);
          protoBinnings.push_back(
              detail::GeoModelBinningHelper::toProtoBinning(bToken));
        }
        portalMaterialBinning[portalNumber] =
            Experimental::BinningDescription{protoBinnings};
      }
    }
    ACTS_VERBOSE("Node " << entry.name << " has "
                         << portalMaterialBinning.size()
                         << " material portals.");
  }

  // Block for branch or container nodes that have children
  if (entryType == "branch" || entryType == "container" ||
      entryType == "root") {
    std::vector<std::unique_ptr<Experimental::Blueprint::Node>> children;
    // Check for gap filling
    bool gapFilling = false;
    // Check if the entry has children
    if (entry.internals.size() < 2u) {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: Branch node '" + entry.name +
          "' has no children defined in blueprint table");
    }
    std::vector<std::string> childrenNames;
    boost::split(childrenNames, entry.internals[1u], boost::is_any_of(","));
    // Create the sub nodes and keep track of the raw values
    for (const auto& childName : childrenNames) {
      std::string fChildName = entry.name + std::string("/") + childName;
      if (childName == "*") {
        // Gap volume detected
        gapFilling = true;
        ACTS_VERBOSE("Gap volume detected, gap filling will be triggered.");
        continue;
      }
      // Check for child and build it
      auto childEntry = tableEntryMap.find(fChildName);
      if (childEntry == tableEntryMap.end()) {
        throw std::invalid_argument("GeoModelBlueprintCreater: Child node '" +
                                    childName + "' of '" + entry.name +
                                    "' NOT found in blueprint table");
      }
      auto node =
          createNode(cache, gctx, childEntry->second, tableEntryMap, extent);
      children.push_back(std::move(node));
    }

    // Create the binnings
    std::vector<Acts::AxisDirection> binnings;
    std::for_each(
        entry.binnings.begin(), entry.binnings.end(),
        [&binnings](const std::string& b) {
          binnings.push_back(detail::GeoModelBinningHelper::toAxisDirection(b));
        });

    // Complete the children
    auto node = std::make_unique<Experimental::Blueprint::Node>(
        entry.name, transform, boundsType, boundValues, binnings,
        std::move(children), extent);
    node->portalMaterialBinning = portalMaterialBinning;

    if (gapFilling) {
      // Find the first child that is not a gap
      Experimental::detail::BlueprintHelper::fillGaps(*node, true);
    }

    // Attach a geoID generator if configured
    if (entryTypeSplit.size() > 1u) {
      // Get the geometry ID from the string
      int geoID = std::stoi(entryTypeSplit[1u]);
      Experimental::GeometryIdGenerator::Config geoIDCfg;
      geoIDCfg.containerMode = true;
      geoIDCfg.containerId = geoID;
      geoIDCfg.resetSubCounters = true;
      // Make the container geoID generator
      node->geoIdGenerator =
          std::make_shared<Experimental::GeometryIdGenerator>(
              geoIDCfg, m_logger->clone(entry.name + "_GeometryIdGenerator"));
    }

    // Create the branch node
    return node;

  } else if (entryType == "leaf") {
    auto node = std::make_unique<Experimental::Blueprint::Node>(
        entry.name, transform, boundsType, boundValues, internalsBuilder,
        extent);
    node->portalMaterialBinning = portalMaterialBinning;
    return node;
  } else {
    throw std::invalid_argument(
        "GeoModelBlueprintCreater: Unknown node type '" + entry.type + "'");
  }

  return nullptr;
}

std::tuple<std::shared_ptr<const Acts::Experimental::IInternalStructureBuilder>,
           Acts::Extent>
Acts::GeoModelBlueprintCreater::createInternalStructureBuilder(
    Cache& cache, const GeometryContext& gctx, const TableEntry& entry,
    const Extent& externalExtent,
    const std::vector<AxisDirection>& internalConstraints) const {
  // Check if the internals entry is empty
  if (entry.internals.empty()) {
    return {nullptr, Extent()};
  }

  // Build a layer structure
  if (entry.internals[0u] == "layer") {
    // Check if the internals entry is interpretable
    if (entry.internals.size() < 2u) {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: Internals entry not complete.");
    }

    // Internal split of the internals
    std::vector<std::string> internalsSplit;
    boost::split(internalsSplit, entry.internals[1u], boost::is_any_of(","));

    // Prepare an internal extent
    Extent internalExtent;
    if (internalsSplit[0u] == "kdt" && cache.kdtSurfaces != nullptr) {
      std::vector<std::string> internalsData = {internalsSplit.begin() + 1,
                                                internalsSplit.end()};
      auto [boundsType, rangeExtent] =
          detail::GeoModelExentHelper::extentFromTable(internalsData,
                                                       externalExtent);

      ACTS_VERBOSE("Requested range: " << rangeExtent.toString());
      std::array<double, 3u> mins = {};
      std::array<double, 3u> maxs = {};

      // Fill what we have - follow the convention to fill up with the last
      for (std::size_t ibv = 0; ibv < 3u; ++ibv) {
        if (ibv < m_cfg.kdtBinning.size()) {
          AxisDirection v = m_cfg.kdtBinning[ibv];
          mins[ibv] = rangeExtent.min(v);
          maxs[ibv] = rangeExtent.max(v);
          continue;
        }
        mins[ibv] = rangeExtent.min(m_cfg.kdtBinning.back());
        maxs[ibv] = rangeExtent.max(m_cfg.kdtBinning.back());
      }
      // Create the search range
      RangeXD<3u, double> searchRange{mins, maxs};
      auto surfaces = cache.kdtSurfaces->surfaces(searchRange);
      // Loop over surfaces and create an internal extent
      for (auto& sf : surfaces) {
        auto sfExtent =
            sf->polyhedronRepresentation(gctx, m_cfg.quarterSegments).extent();
        internalExtent.extend(sfExtent, internalConstraints);
      }
      ACTS_VERBOSE("Found " << surfaces.size() << " surfaces in range "
                            << searchRange.toString());
      ACTS_VERBOSE("Internal extent: " << internalExtent.toString());

      // Create the layer structure builder
      Experimental::LayerStructureBuilder::Config lsbCfg;
      lsbCfg.surfacesProvider =
          std::make_shared<Experimental::LayerStructureBuilder::SurfacesHolder>(
              surfaces);

      // Let's check the binning description
      if (!entry.binnings.empty()) {
        ACTS_VERBOSE("Binning description detected for this layer structure.");
        for (const auto& binning : entry.binnings) {
          if (!binning.empty()) {
            ACTS_VERBOSE("- Adding binning: " << binning);
            lsbCfg.binnings.push_back(
                detail::GeoModelBinningHelper::toProtoBinning(binning,
                                                              internalExtent));
          }
        }
      } else {
        lsbCfg.nMinimalSurfaces = surfaces.size() + 1u;
      }

      return {
          std::make_shared<Experimental::LayerStructureBuilder>(
              lsbCfg, m_logger->clone(entry.name + "_LayerStructureBuilder")),
          internalExtent};

    } else {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: Unknown layer internals type '" +
          entry.internals[1u] + "' / or now kdt surfaces provided.");
    }
  }
  return {nullptr, Extent()};
}

std::tuple<Acts::VolumeBounds::BoundsType, Acts::Extent, std::vector<double>,
           Acts::Vector3>
Acts::GeoModelBlueprintCreater::parseBounds(
    const std::string& boundsEntry, const Extent& externalExtent,
    const Extent& internalExtent) const {
  std::vector<std::string> boundsEntrySplit;
  boost::split(boundsEntrySplit, boundsEntry, boost::is_any_of(","));

  // Create the return values
  Vector3 translation{0., 0., 0.};
  std::vector<double> boundValues = {};
  auto [boundsType, extent] = detail::GeoModelExentHelper::extentFromTable(
      boundsEntrySplit, externalExtent, internalExtent);

  // Switch on the bounds type
  if (boundsType == VolumeBounds::BoundsType::eCylinder) {
    // Create the translation & bound values
    translation = Acts::Vector3(0., 0., extent.medium(AxisDirection::AxisZ));
    boundValues = {extent.min(AxisDirection::AxisR),
                   extent.max(AxisDirection::AxisR),
                   0.5 * extent.interval(AxisDirection::AxisZ)};
  } else {
    throw std::invalid_argument(
        "GeoModelBlueprintCreater: Unknown bounds type, only 'cyl' is "
        "supported for the moment.");
  }

  return {boundsType, extent, boundValues, translation};
}

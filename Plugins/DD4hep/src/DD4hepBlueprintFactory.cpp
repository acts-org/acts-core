// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/DD4hepBlueprintFactory.hpp"

#include "Acts/Detector/GeometryIdGenerator.hpp"
#include "Acts/Detector/IndexedRootVolumeFinderBuilder.hpp"
#include "Acts/Plugins/DD4hep/DD4hepBinningHelpers.hpp"
#include "Acts/Plugins/DD4hep/DD4hepConversionHelpers.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <sstream>

Acts::Experimental::DD4hepBlueprintFactory::DD4hepBlueprintFactory(
    const Config& cfg, std::unique_ptr<const Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {
  ACTS_DEBUG("UnitLength conversion factor (DD4hep -> Acts): " << unitLength);
}

std::unique_ptr<Acts::Experimental::Blueprint::Node>
Acts::Experimental::DD4hepBlueprintFactory::create(
    Cache& cache, const GeometryContext& gctx,
    const dd4hep::DetElement& dd4hepElement) const {
  ACTS_DEBUG("Drawing a blueprint from the DD4hep element '"
             << dd4hepElement.name() << "'.");

  // Create the root node
  std::vector<ActsScalar> bValues = {0., 150., 1000.};
  std::vector<BinningValue> binning = {Acts::BinningValue::binR};
  auto root = std::make_unique<Acts::Experimental::Blueprint::Node>(
      dd4hepElement.name(), Acts::Transform3::Identity(),
      Acts::VolumeBounds::eCylinder, bValues, binning);

  // Recursively parse the tree
  recursiveParse(cache, *root, gctx, dd4hepElement);
  // Return the top node
  return root;
}

void Acts::Experimental::DD4hepBlueprintFactory::recursiveParse(
    Cache& cache, Blueprint::Node& mother, const GeometryContext& gctx,
    const dd4hep::DetElement& dd4hepElement, unsigned int hiearchyLevel) const {
  // This will allow to skip empty hierarchy levels
  Blueprint::Node* current = &mother;
  unsigned int hierarchyAddOn = 0;

  std::string ofs(hiearchyLevel * 2u, ' ');

  // Node types
  std::vector<std::string> nodeTypes = {"acts_world", "acts_container",
                                        "acts_volume"};
  for (const auto& nType : nodeTypes) {
    // Check if it complies with the given definition
    bool ntt = getParamOr<bool>(nType, dd4hepElement, false);
    if (ntt) {
      ACTS_DEBUG(ofs << "ACTS node '" << nType
                     << "' attached to dd4hep element '" << dd4hepElement.name()
                     << "',");
      // Extract potential internal builders and tools
      auto [internalsBuilder, rootsFinderBuilder, geoIdGenerator, auxInt,
            extOpt] =
          extractInternals(cache.dd4hepStore, gctx, dd4hepElement, nType);
      // Extract the bounds type, values and binning
      auto [transform, bValueType, bValues, binning, auxExt] =
          extractExternals(gctx, dd4hepElement, nType, extOpt);
      // Screen output of position and shape
      ACTS_DEBUG(ofs << " - translation  : "
                     << toString(transform.translation()));
      ACTS_DEBUG(ofs << " - bounds type  : " << bValueType);
      ACTS_DEBUG(ofs << " - bound values : " << toString(bValues));
      // If it is not the world node, create a new one
      if (nType == "acts_world") {
        mother.transform = transform;
        mother.boundsType = bValueType;
        mother.boundaryValues = bValues;
        mother.binning = binning;

      } else if (nType == "acts_container") {
        // Creating the branch node
        auto branch = std::make_unique<Acts::Experimental::Blueprint::Node>(
            dd4hepElement.name(), transform, bValueType, bValues, binning);
        current = branch.get();
        mother.add(std::move(branch));

      } else if (nType == "acts_volume") {
        // Crreating a leaf node
        auto leaf = std::make_unique<Acts::Experimental::Blueprint::Node>(
            dd4hepElement.name(), transform, bValueType, bValues);
        current = leaf.get();
        mother.add(std::move(leaf));
      }
      // Current is set now appropriately, adding auxiliary information
      if (!auxExt.empty()) {
        ACTS_VERBOSE(ofs << " - " << auxExt);
        current->auxiliary.push_back(auxExt);
      }
      // Adding the internals builder - if present
      if (internalsBuilder != nullptr) {
        ACTS_VERBOSE(ofs << " - " << auxInt[0u]);
        current->internalsBuilder = internalsBuilder;
      }
      // Adding root finder builder - if present
      if (rootsFinderBuilder != nullptr) {
        ACTS_VERBOSE(ofs << " - " << auxInt[1u]);
        current->rootVolumeFinderBuilder = rootsFinderBuilder;
      }

      // Check for proto material for the portals, max portal number
      // can be changed in configuration
      for (unsigned int p = 0u; p < m_cfg.maxPortals; ++p) {
        std::string pmName = "acts_portal_proto_material_" + std::to_string(p);
        auto protoMaterial = getParamOr<bool>(pmName, dd4hepElement, false);
        if (protoMaterial) {
          ACTS_VERBOSE(ofs << " - proto material binning for portal " << p
                           << " found");
          auto pmProtoBinnings = DD4hepBinningHelpers::convertBinning(
              dd4hepElement, pmName + "_binning");
          current->portalMaterialBinning[p] =
              BinningDescription{pmProtoBinnings};
          ACTS_VERBOSE(ofs << " - binning description is "
                           << current->portalMaterialBinning[p].toString());
        }
      }

      // Adding geo Id generator - if present
      if (geoIdGenerator != nullptr) {
        ACTS_VERBOSE(ofs << " - " << auxInt[2u]);
        current->geoIdGenerator = geoIdGenerator;
      }
    }
  }

  // Step down to the children - not possible for leaf nodes
  const dd4hep::DetElement::Children& children = dd4hepElement.children();
  if (!children.empty()) {
    ACTS_VERBOSE(ofs << "dd4hep element '" << dd4hepElement.name() << "' has "
                     << children.size() << " children.");
    for (auto& child : children) {
      dd4hep::DetElement dd4hepChild = child.second;
      recursiveParse(cache, *current, gctx, dd4hepChild,
                     hiearchyLevel + hierarchyAddOn);
    }
  }
}

std::tuple<Acts::Transform3, Acts::VolumeBounds::BoundsType,
           std::vector<Acts::ActsScalar>, std::vector<Acts::BinningValue>,
           std::string>
Acts::Experimental::DD4hepBlueprintFactory::extractExternals(
    [[maybe_unused]] const GeometryContext& gctx,
    const dd4hep::DetElement& dd4hepElement, const std::string& baseName,
    const std::optional<Extent>& extOpt) const {
  std::string aux = "";

  /// Get the transform - extract from values first
  auto transform = extractTransform(dd4hepElement, baseName, unitLength);

  // Get the bounds type
  auto bValueInt =
      getParamOr<int>(baseName + "_type", dd4hepElement,
                      static_cast<int>(VolumeBounds::BoundsType::eOther));
  auto bValueType = static_cast<VolumeBounds::BoundsType>(bValueInt);
  std::vector<ActsScalar> bValues = {};

  // Get the bound values from parsed internals if possible
  if (extOpt.has_value() && bValueType == VolumeBounds::BoundsType::eCylinder) {
    // Set as defaults
    bValues = {0., 0., 0.};
    auto parsedExtent = extOpt.value();
    if (parsedExtent.constrains(BinningValue::binR)) {
      bValues[0u] = std::floor(parsedExtent.min(BinningValue::binR));
      bValues[1u] = std::ceil(parsedExtent.max(BinningValue::binR));
    }
    if (parsedExtent.constrains(BinningValue::binZ)) {
      ActsScalar minZ = parsedExtent.min(BinningValue::binZ) > 0.
                            ? std::floor(parsedExtent.min(BinningValue::binZ))
                            : std::ceil(parsedExtent.min(BinningValue::binZ));
      ActsScalar maxZ = parsedExtent.max(BinningValue::binZ) > 0.
                            ? std::floor(parsedExtent.max(BinningValue::binZ))
                            : std::ceil(parsedExtent.max(BinningValue::binZ));
      bValues[2u] = 0.5 * (maxZ - minZ);
      transform.translation().z() = 0.5 * (maxZ + minZ);
    }
    ACTS_VERBOSE("   cylindrical bounds determined from internals as "
                 << toString(bValues));
  }

  // Get the bounds values from the series if not found before
  if (bValues.empty()) {
    bValues = extractSeries<ActsScalar>(dd4hepElement, baseName + "_bvalues",
                                        unitLength);
    ACTS_VERBOSE(" - cylindrical determined from variant parameters as "
                 << toString(bValues));
  }

  // Get the binning values
  auto binningString =
      getParamOr<std::string>(baseName + "_binning", dd4hepElement, "");
  std::vector<BinningValue> bBinning =
      Acts::stringToBinningValues(binningString);
  if (!binningString.empty()) {
    aux += "vol. binning : " + binningString;
  }
  // Return the tuple
  return std::make_tuple(transform, bValueType, bValues, bBinning, aux);
}

std::tuple<std::shared_ptr<const Acts::Experimental::IInternalStructureBuilder>,
           std::shared_ptr<const Acts::Experimental::IRootVolumeFinderBuilder>,
           std::shared_ptr<const Acts::Experimental::IGeometryIdGenerator>,
           std::array<std::string, 3u>, std::optional<Acts::Extent>>
Acts::Experimental::DD4hepBlueprintFactory::extractInternals(
    Acts::DD4hepDetectorElement::Store& dd4hepStore,
    const GeometryContext& gctx, const dd4hep::DetElement& dd4hepElement,
    const std::string& baseName) const {
  // Return objects
  std::shared_ptr<const Acts::Experimental::IInternalStructureBuilder>
      internalsBuilder = nullptr;
  std::shared_ptr<const Acts::Experimental::IRootVolumeFinderBuilder>
      rootsFinderBuilder = nullptr;
  std::shared_ptr<const Acts::Experimental::IGeometryIdGenerator>
      geoIdGenerator = nullptr;
  /// The hand-over information for externals
  std::optional<Extent> ext = std::nullopt;
  /// Auxiliary information
  std::array<std::string, 3u> aux = {"", "", ""};

  // Check for internal structure builder
  auto internals =
      Acts::getParamOr<bool>(baseName + "_internals", dd4hepElement, false);
  if (internals) {
    auto internalsType = Acts::getParamOr<std::string>(
        baseName + "_internals_type", dd4hepElement, "");
    if (internalsType == "layer") {
      aux[0u] = "int. struct : layer";
      // Create a new layer builder
      DD4hepLayerStructure::Options lOptions;
      lOptions.name = dd4hepElement.name();
      // Check whether internal/sensitive surfaces should have directly
      // translated material
      auto convertMaterial = Acts::getParamOr<bool>(
          "acts_surface_material_conversion", dd4hepElement, false);
      lOptions.conversionOptions.convertMaterial = convertMaterial;
      // Check if the extent should be measured
      auto interenalsMeasure = Acts::getParamOr<std::string>(
          baseName + "_internals_measure", dd4hepElement, "");
      auto internalsClearance =
          unitLength *
          Acts::getParamOr<ActsScalar>(baseName + "_internals_clearance",
                                       dd4hepElement, 0.);
      auto internalBinningValues = stringToBinningValues(interenalsMeasure);
      if (!internalBinningValues.empty()) {
        ACTS_VERBOSE(" - internals extent measurement requested");
        Extent internalsExtent;
        ExtentEnvelope clearance = ExtentEnvelope::Zero();
        for (const auto& bv : internalBinningValues) {
          ACTS_VERBOSE("   -> measuring extent for " << binningValueName(bv));
          ACTS_VERBOSE("   -> with clearance :" << internalsClearance);
          clearance[bv] = {internalsClearance, internalsClearance};
        }
        internalsExtent.setEnvelope(clearance);
        lOptions.extent = internalsExtent;
        lOptions.extentConstraints = internalBinningValues;
      }
      // Create the builder from the dd4hep element
      auto [ib, extOpt] = m_cfg.layerStructure->builder(
          dd4hepStore, gctx, dd4hepElement, lOptions);
      internalsBuilder = std::move(ib);
      if (extOpt.has_value()) {
        ACTS_VERBOSE(" - internals extent measured as "
                     << extOpt.value().toString());
      }
      ext = extOpt;
    }
  }

  // Check for root volume finder
  auto rootFinder = Acts::getParamOr<std::string>(
      baseName + "_root_volume_finder", dd4hepElement, "");
  if (rootFinder == "indexed") {
    aux[1u] = "root finder : indexed";
    std::vector<BinningValue> binning = {BinningValue::binZ,
                                         BinningValue::binR};
    rootsFinderBuilder =
        std::make_shared<Acts::Experimental::IndexedRootVolumeFinderBuilder>(
            binning);
  }

  // Check for geo Id generator
  auto geoIdGen =
      Acts::getParamOr<std::string>(baseName + "_geo_id", dd4hepElement, "");
  if (geoIdGen == "incremental") {
    aux[2u] = "geo_id gen. : incremental";
    Acts::Experimental::GeometryIdGenerator::Config geoIdCfg;
    geoIdGenerator =
        std::make_shared<Acts::Experimental::GeometryIdGenerator>(geoIdCfg);
  } else if (geoIdGen == "container") {
    aux[2u] = "geo_id gen. : container";
    Acts::Experimental::GeometryIdGenerator::Config geoIdCfg;
    geoIdCfg.containerMode = true;
    geoIdCfg.containerId =
        Acts::getParamOr<int>(baseName + "_geo_id_base", dd4hepElement, 1);
    geoIdGenerator =
        std::make_shared<Acts::Experimental::GeometryIdGenerator>(geoIdCfg);
  }

  return std::make_tuple(internalsBuilder, rootsFinderBuilder, geoIdGenerator,
                         aux, ext);
}

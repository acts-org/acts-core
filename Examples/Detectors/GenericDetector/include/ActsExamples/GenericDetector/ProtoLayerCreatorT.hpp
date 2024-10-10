// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/DetectorElementBase.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

#include <iostream>

namespace Acts {

class LayerCreator;
class Surface;
class DetecorElementBase;
}  // namespace Acts

namespace ActsExamples::Generic {

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

using SurfacePosition = std::pair<const Acts::Surface*, Acts::Vector3>;

struct ProtoLayerSurfaces {
  Acts::ProtoLayer protoLayer;
  std::vector<std::shared_ptr<const Acts::Surface>> surfaces;
  std::size_t bins0;
  std::size_t bins1;
};

/// @class ProtoLayerCreatorT
///
/// The ProtoLayerCreatorT is the first setp in creating a geometry
/// from code input, it creates the ProtoLayer and returns the
/// created detector elements for the DetectorStore emulation
template <typename detector_element_t>
class ProtoLayerCreatorT {
 public:
  using LayerStore = std::vector<std::shared_ptr<detector_element_t>>;

  using DetectorStore = std::vector<LayerStore>;

  /// @struct Config
  ///
  /// Nested configuration struct for the ProtoLayerCreatorT
  struct Config {
    /// a single parameter for the approach surface envelope
    double approachSurfaceEnvelope = 0.5;
    /// central layer specification
    /// bin multipliers in rphi,z for finer module binning
    std::pair<int, int> centralLayerBinMultipliers;
    /// layer radii for the sensitive layers
    std::vector<double> centralLayerRadii;
    /// the (additional) layer envelope in R/Z
    std::vector<std::pair<double, double>> centralLayerEnvelopes;
    /// the binning schema: nPhi x nZ
    std::vector<std::pair<int, int>> centralModuleBinningSchema;
    /// the module center positions
    std::vector<std::vector<Acts::Vector3>> centralModulePositions;
    /// the module tilt for this layer
    std::vector<double> centralModuleTiltPhi;
    /// the module bounds: local x
    std::vector<double> centralModuleHalfX;
    /// the module bounds: local y
    std::vector<double> centralModuleHalfY;
    /// the module bounds: local z -> thickness
    std::vector<double> centralModuleThickness;
    /// the module material
    std::vector<std::shared_ptr<const Acts::ISurfaceMaterial>>
        centralModuleMaterial;
    /// the module front side stereo (if exists)
    std::vector<double> centralModuleFrontsideStereo;
    /// the module back side stereo (if exists)
    std::vector<double> centralModuleBacksideStereo;
    /// the module gap between frontside and backside
    std::vector<double> centralModuleBacksideGap;

    /// the layers at p/e side
    /// bin multipliers in r,phi for finer module binning
    std::pair<int, int> posnegLayerBinMultipliers;
    /// layer positions in Z
    std::vector<double> posnegLayerPositionsZ;
    /// the envelope definitions
    std::vector<double> posnegLayerEnvelopeR;
    /// the module center positions
    std::vector<std::vector<std::vector<Acts::Vector3>>> posnegModulePositions;
    /// the phi binning
    std::vector<std::vector<std::size_t>> posnegModulePhiBins;
    /// the module bounds: min halfx
    std::vector<std::vector<double>> posnegModuleMinHalfX;
    /// the module bounds: max halfx
    std::vector<std::vector<double>> posnegModuleMaxHalfX;
    /// the module bounds: local y
    std::vector<std::vector<double>> posnegModuleHalfY;
    /// the module bounds: local z -> thickness
    std::vector<std::vector<double>> posnegModuleThickness;
    /// the module material
    std::vector<std::vector<std::shared_ptr<const Acts::ISurfaceMaterial>>>
        posnegModuleMaterial;
    /// the module front side stereo (if exists)
    std::vector<std::vector<double>> posnegModuleFrontsideStereo;
    /// the module back side stereo (if exists)
    std::vector<std::vector<double>> posnegModuleBacksideStereo;
    /// the module gap between frontside and backside
    std::vector<std::vector<double>> posnegModuleBacksideGap;
  };

  /// Constructor
  /// @param cfg is the configuration class
  /// @param logger is the logging class for screen output
  ProtoLayerCreatorT(const Config& cfg,
                     std::unique_ptr<const Acts::Logger> logger =
                         Acts::getDefaultLogger("ProtoLayerCreatorT",
                                                Acts::Logging::INFO));

  /// @brief construct the negative side layers
  /// @param gctx The geometry context for this construction call
  /// @param detectorStore The reference store for the detector elements
  /// @return the protolayers and surfaces on the negative detector side
  std::vector<ProtoLayerSurfaces> negativeProtoLayers(
      const Acts::GeometryContext& gctx, DetectorStore& detectorStore) const;

  /// @brief construct the central layers
  /// @param gctx The geometry context for this construction call
  /// @param detectorStore The reference store for the detector elements
  /// @return the protolayers and surfaces on the central detector side
  std::vector<ProtoLayerSurfaces> centralProtoLayers(
      const Acts::GeometryContext& gctx, DetectorStore& detectorStore) const;

  /// @brief construct the positive side layers
  /// @param gctx The geometry context for this construction call
  /// @param detectorStore The reference store for the detector elements
  /// @return the protolayers and surfaces on the  positive detector side
  std::vector<ProtoLayerSurfaces> positiveProtoLayers(
      const Acts::GeometryContext& gctx, DetectorStore& detectorStore) const;

 private:
  /// @brief private helper method to create the proto layers on the
  /// left respectively right side
  /// @param gctx The geometry context for this construction call
  /// @param detectorStore The reference store for the detector elements
  /// @param side is the indiciator whether to build on negative/positive
  /// @return the protolayers and surfaces on the neg/pos detector side
  std::vector<ProtoLayerSurfaces> createProtoLayers(
      const Acts::GeometryContext& gctx, DetectorStore& detectorStore,
      int side) const;

  /// Configuration member
  Config m_cfg;

  /// the logging instance
  std::unique_ptr<const Acts::Logger> m_logger;

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }
};

template <typename detector_element_t>
std::vector<ProtoLayerSurfaces>
ProtoLayerCreatorT<detector_element_t>::centralProtoLayers(
    const Acts::GeometryContext& gctx, DetectorStore& detectorStore) const {
  // create the vector
  std::vector<ProtoLayerSurfaces> cpLayers;
  // create the detector store entry
  LayerStore layerStore;

  // Count the current detector modules identifiers
  std::size_t imodule = 0;
  for (auto& eLayers : detectorStore) {
    imodule += eLayers.size();
  }
  ACTS_VERBOSE("Starting with identifier " << imodule);

  // ----------------------- central layers -------------------------
  // the central layers
  std::size_t numcLayers = m_cfg.centralLayerRadii.size();
  if (numcLayers != 0u) {
    ACTS_DEBUG("Configured to build " << numcLayers
                                      << " active central layers.");
    cpLayers.reserve(numcLayers);
    // loop through
    for (std::size_t icl = 0; icl < numcLayers; ++icl) {
      // layer R/Z
      double layerR = m_cfg.centralLayerRadii.at(icl);
      // some screen output
      ACTS_DEBUG("Build layer " << icl << " with target radius = " << layerR);

      // prepare the Surface vector
      std::vector<std::shared_ptr<const Acts::Surface>> sVector;
      // assign the current envelope
      double layerEnvelopeCoverZ =
          !m_cfg.centralLayerEnvelopes.empty()
              ? m_cfg.centralLayerEnvelopes.at(icl).second
              : 0.;
      // module size & tilt
      double modulePhiTilt = m_cfg.centralModuleTiltPhi.at(icl);
      double moduleHalfX = m_cfg.centralModuleHalfX.at(icl);
      double moduleHalfY = m_cfg.centralModuleHalfY.at(icl);
      double moduleThickness = m_cfg.centralModuleThickness.at(icl);
      // create the shared module
      std::shared_ptr<const Acts::PlanarBounds> moduleBounds(
          new Acts::RectangleBounds(moduleHalfX, moduleHalfY));
      std::size_t nCentralModules =
          m_cfg.centralModuleBinningSchema.at(icl).first *
          m_cfg.centralModuleBinningSchema.at(icl).second;

      ACTS_DEBUG("- number of modules "
                 << nCentralModules << " ( from "
                 << m_cfg.centralModuleBinningSchema.at(icl).first << " x "
                 << m_cfg.centralModuleBinningSchema.at(icl).second << " )");

      sVector.reserve(nCentralModules);

      // prepartation :
      // create the Module material from input
      std::shared_ptr<const Acts::ISurfaceMaterial> moduleMaterialPtr = nullptr;
      if (!m_cfg.centralModuleMaterial.empty()) {
        // get the sensor material from configuration
        moduleMaterialPtr = m_cfg.centralModuleMaterial.at(icl);
      }

      // confirm
      if (m_cfg.centralModulePositions.at(icl).size() != nCentralModules) {
        ACTS_WARNING("Mismatching module numbers, configuration error!");
        ACTS_WARNING("- Binning schema suggests : " << nCentralModules);
        ACTS_WARNING("- Positions provided are  : "
                     << m_cfg.centralModulePositions.at(icl).size());
      }
      // loop over the position, create the modules
      for (auto& moduleCenter : m_cfg.centralModulePositions.at(icl)) {
        // create the association transform
        double modulePhi = phi(moduleCenter);
        // the local z axis is the normal vector
        Acts::Vector3 moduleLocalZ(cos(modulePhi + modulePhiTilt),
                                   sin(modulePhi + modulePhiTilt), 0.);
        // the local y axis is the global z axis
        Acts::Vector3 moduleLocalY(0., 0., 1);
        // the local x axis the normal to local y,z
        Acts::Vector3 moduleLocalX(-sin(modulePhi + modulePhiTilt),
                                   cos(modulePhi + modulePhiTilt), 0.);
        // create the RotationMatrix
        Acts::RotationMatrix3 moduleRotation;
        moduleRotation.col(0) = moduleLocalX;
        moduleRotation.col(1) = moduleLocalY;
        moduleRotation.col(2) = moduleLocalZ;
        // get the moduleTransform
        std::shared_ptr<Acts::Transform3> mutableModuleTransform =
            std::make_shared<Acts::Transform3>(
                Acts::Translation3(moduleCenter) * moduleRotation);
        // stereo angle if necessary
        if (!m_cfg.centralModuleFrontsideStereo.empty() &&
            m_cfg.centralModuleFrontsideStereo.at(icl) != 0.) {
          // twist by the stereo angle
          double stereo = m_cfg.centralModuleFrontsideStereo.at(icl);
          (*mutableModuleTransform.get()) *=
              Acts::AngleAxis3(-stereo, Acts::Vector3::UnitZ());
        }
        // count the modules
        GenericDetectorElement::Identifier moduleIdentifier =
            static_cast<GenericDetectorElement::Identifier>(imodule++);

        // Finalize the transform
        auto moduleTransform = std::const_pointer_cast<const Acts::Transform3>(
            mutableModuleTransform);
        // create the module
        auto module = std::make_shared<detector_element_t>(
            moduleIdentifier, moduleTransform, moduleBounds, moduleThickness,
            moduleMaterialPtr);

        // put the module into the detector store
        layerStore.push_back(module);
        // register the surface
        sVector.push_back(module->surface().getSharedPtr());
        // IF double modules exist
        // and the backside one (if configured to do so)
        if (!m_cfg.centralModuleBacksideGap.empty()) {
          // create the module identifier
          moduleIdentifier =
              static_cast<GenericDetectorElement::Identifier>(imodule++);

          Acts::Vector3 bsModuleCenter =
              moduleCenter +
              m_cfg.centralModuleBacksideGap.at(icl) * moduleLocalZ;
          mutableModuleTransform = std::make_shared<Acts::Transform3>(
              Acts::Translation3(bsModuleCenter) * moduleRotation);
          // apply the stereo
          if (!m_cfg.centralModuleBacksideStereo.empty()) {
            // twist by the stereo angle
            double stereoBackSide = m_cfg.centralModuleBacksideStereo.at(icl);
            (*mutableModuleTransform.get()) *=
                Acts::AngleAxis3(-stereoBackSide, Acts::Vector3::UnitZ());
          }
          // Finalize the transform
          moduleTransform = std::const_pointer_cast<const Acts::Transform3>(
              mutableModuleTransform);
          // create the backseide moulde
          auto bsmodule = std::make_shared<detector_element_t>(
              moduleIdentifier, moduleTransform, moduleBounds, moduleThickness,
              moduleMaterialPtr);
          // everything is set for the next module
          layerStore.push_back(std::move(bsmodule));
        }
      }

      std::size_t phiBins = m_cfg.centralModuleBinningSchema.at(icl).first;
      phiBins *= m_cfg.centralLayerBinMultipliers.first;
      std::size_t zBins = m_cfg.centralModuleBinningSchema.at(icl).second;
      zBins *= m_cfg.centralLayerBinMultipliers.second;
      // create the surface array - it will also fill the accessible binmember
      // cache if available
      Acts::ProtoLayer pl(gctx, sVector);
      pl.envelope[Acts::BinningValue::binR] = {m_cfg.approachSurfaceEnvelope,
                                               m_cfg.approachSurfaceEnvelope};
      pl.envelope[Acts::BinningValue::binZ] = {layerEnvelopeCoverZ,
                                               layerEnvelopeCoverZ};

      // Record the proto layer and the surfaces for the later layer building
      ProtoLayerSurfaces pls{std::move(pl), sVector, phiBins, zBins};
      cpLayers.push_back(std::move(pls));
      // fill the detector store
      detectorStore.push_back(std::move(layerStore));
    }
  }
  return cpLayers;
}

template <typename detector_element_t>
std::vector<ProtoLayerSurfaces>
ProtoLayerCreatorT<detector_element_t>::negativeProtoLayers(
    const Acts::GeometryContext& gctx, DetectorStore& detectorStore) const {
  return createProtoLayers(gctx, detectorStore, -1);
}

template <typename detector_element_t>
std::vector<ProtoLayerSurfaces>
ProtoLayerCreatorT<detector_element_t>::positiveProtoLayers(
    const Acts::GeometryContext& gctx, DetectorStore& detectorStore) const {
  return createProtoLayers(gctx, detectorStore, 1);
}

template <typename detector_element_t>
ProtoLayerCreatorT<detector_element_t>::ProtoLayerCreatorT(
    const ProtoLayerCreatorT<detector_element_t>::Config& cfg,
    std::unique_ptr<const Acts::Logger> log)
    : m_cfg(cfg), m_logger(std::move(log)) {}

template <typename detector_element_t>
std::vector<ProtoLayerSurfaces>
ProtoLayerCreatorT<detector_element_t>::createProtoLayers(
    const Acts::GeometryContext& gctx, DetectorStore& detectorStore,
    int side) const {
  // Count the current detector modules identifiers
  std::size_t imodule = 0;
  for (auto& eLayers : detectorStore) {
    imodule += eLayers.size();
  }
  ACTS_VERBOSE("Starting with identifier " << imodule);
  // the return layers
  std::vector<ProtoLayerSurfaces> epLayers;
  // create the detector store entry
  LayerStore layerStore;
  // -------------------------------- endcap type layers
  // pos/neg layers
  std::size_t numpnLayers = m_cfg.posnegLayerPositionsZ.size();
  if (numpnLayers != 0u) {
    ACTS_DEBUG("Configured to build 2 * "
               << numpnLayers << " passive positive/negative side layers.");
    epLayers.reserve(numpnLayers);

    /// this is the loop over the layer positions
    for (std::size_t ipnl = 0; ipnl < numpnLayers; ++ipnl) {
      // some screen output
      ACTS_VERBOSE("- building layer "
                   << ipnl << " and " << numpnLayers + ipnl << " at z = "
                   << side * m_cfg.posnegLayerPositionsZ.at(ipnl));
      /// some preparation work
      // define the layer envelope
      double layerEnvelopeR = m_cfg.posnegLayerEnvelopeR.at(ipnl);
      // prepare for the r binning
      std::vector<std::shared_ptr<const Acts::Surface>> esVector;
      // now fill the vectors
      std::size_t ipnR = 0;
      for (auto& discModulePositions : m_cfg.posnegModulePositions.at(ipnl)) {
        ACTS_VERBOSE("- building ring " << ipnR << " for this layer.");
        // now prepare all the shared stuff
        // (0) module specifications
        double moduleThickness = m_cfg.posnegModuleThickness.at(ipnl).at(ipnR);
        double moduleMinHalfX = m_cfg.posnegModuleMinHalfX.at(ipnl).at(ipnR);
        double moduleMaxHalfX = 0.;
        if (m_cfg.posnegModuleMaxHalfX.size() > ipnl &&
            m_cfg.posnegModuleMaxHalfX.at(ipnl).size() > ipnR) {
          moduleMaxHalfX = m_cfg.posnegModuleMaxHalfX.at(ipnl).at(ipnR);
        }
        double moduleHalfY = m_cfg.posnegModuleHalfY.at(ipnl).at(ipnR);
        // (1) module bounds
        // create the bounds
        Acts::PlanarBounds* pBounds = nullptr;
        if (moduleMaxHalfX != 0. && moduleMinHalfX != moduleMaxHalfX) {
          pBounds = new Acts::TrapezoidBounds(moduleMinHalfX, moduleMaxHalfX,
                                              moduleHalfY);
        } else {
          pBounds = new Acts::RectangleBounds(moduleMinHalfX, moduleHalfY);
        }
        // now create the shared bounds from it
        std::shared_ptr<const Acts::PlanarBounds> moduleBounds(pBounds);
        // (2)) module material
        // create the Module material from input
        std::shared_ptr<const Acts::ISurfaceMaterial> moduleMaterialPtr =
            nullptr;
        if (!m_cfg.posnegModuleMaterial.empty()) {
          // and create the shared pointer
          moduleMaterialPtr = m_cfg.posnegModuleMaterial.at(ipnl).at(ipnR);
        }

        // low loop over the phi positions and build the stuff
        for (auto& ringModulePosition : discModulePositions) {
          // the module transform from the position
          double modulePhi = phi(ringModulePosition);
          // the center position of the modules
          Acts::Vector3 moduleCenter(ringModulePosition);
          moduleCenter.z() *= side;
          // the rotation matrix of the module
          Acts::Vector3 moduleLocalY(cos(modulePhi), sin(modulePhi), 0.);
          // take different axis to have the same readout direction
          Acts::Vector3 moduleLocalZ(0., 0., side * 1.);
          Acts::Vector3 moduleLocalX = moduleLocalY.cross(moduleLocalZ);
          // local rotation matrices
          // create the RotationMatrix - negative side
          Acts::RotationMatrix3 moduleRotation;
          moduleRotation.col(0) = moduleLocalX;
          moduleRotation.col(1) = moduleLocalY;
          moduleRotation.col(2) = moduleLocalZ;
          // the transforms for the two modules
          std::shared_ptr<const Acts::Transform3> moduleTransform =
              std::make_shared<const Acts::Transform3>(
                  Acts::Translation3(moduleCenter) * moduleRotation);

          // create the modules identifier
          GenericDetectorElement::Identifier moduleIdentifier =
              static_cast<GenericDetectorElement::Identifier>(imodule++);

          // create the module
          auto module = std::make_shared<detector_element_t>(
              moduleIdentifier, moduleTransform, moduleBounds, moduleThickness,
              moduleMaterialPtr);
          layerStore.push_back(module);

          // now deal with the potential backside
          if (!m_cfg.posnegModuleBacksideGap.empty()) {
            // increase the counter
            moduleIdentifier =
                static_cast<GenericDetectorElement::Identifier>(imodule++);
            // the new centers
            moduleCenter =
                moduleCenter +
                m_cfg.posnegModuleBacksideGap.at(ipnl).at(ipnR) * moduleLocalZ;
            // the new transforms
            auto mutableModuleTransform = std::make_shared<Acts::Transform3>(
                Acts::Translation3(moduleCenter) * moduleRotation);
            // apply the stereo
            if (!m_cfg.posnegModuleBacksideStereo.empty()) {
              // twist by the stereo angle
              double stereoBackSide =
                  m_cfg.posnegModuleBacksideStereo.at(ipnl).at(ipnR);
              (*mutableModuleTransform.get()) *=
                  Acts::AngleAxis3(-stereoBackSide, Acts::Vector3::UnitZ());
            }
            // Finalize the transform
            moduleTransform = std::const_pointer_cast<const Acts::Transform3>(
                mutableModuleTransform);
            // everything is set for the next module
            auto bsmodule = std::make_shared<detector_element_t>(
                moduleIdentifier, moduleTransform, moduleBounds,
                moduleThickness, moduleMaterialPtr);
            // Put into the detector store
            layerStore.push_back(std::move(bsmodule));
          }
          // create the surface
          esVector.push_back(module->surface().getSharedPtr());
        }
        // counter of rings
        ++ipnR;
      }
      // the binning
      std::size_t layerBinsR = m_cfg.posnegModulePhiBins.at(ipnl).size();
      // never multiply 1 single r-bin, does not make sense
      if (layerBinsR > 1) {
        // multiply with the given bin multiplier
        layerBinsR *= m_cfg.posnegLayerBinMultipliers.first;
      }
      std::size_t layerBinsPhi = 0;
      // take the minimum phi bins in that layer
      for (unsigned int phiBins : m_cfg.posnegModulePhiBins.at(ipnl)) {
        layerBinsPhi = phiBins < layerBinsPhi ? phiBins : layerBinsPhi;
        layerBinsPhi *= m_cfg.posnegLayerBinMultipliers.second;
      }
      // create the layers with the surface arrays
      Acts::ProtoLayer ple(gctx, esVector);
      ple.envelope[Acts::BinningValue::binR] = {layerEnvelopeR, layerEnvelopeR};
      ple.envelope[Acts::BinningValue::binZ] = {m_cfg.approachSurfaceEnvelope,
                                                m_cfg.approachSurfaceEnvelope};

      // push it into the layer vector
      ProtoLayerSurfaces ples{std::move(ple), esVector, layerBinsR,
                              layerBinsPhi};
      epLayers.push_back(std::move(ples));
      // fill the detector store
      detectorStore.push_back(std::move(layerStore));
    }
  }
  return epLayers;
}

}  // namespace ActsExamples::Generic

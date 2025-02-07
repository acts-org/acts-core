// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/DD4hep/DD4hepLayerBuilder.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Plugins/DD4hep/DD4hepConversionHelpers.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Plugins/DD4hep/DD4hepMaterialHelpers.hpp"
#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <utility>

#include <boost/algorithm/string.hpp>

#include "DD4hep/Alignments.h"
#include "DD4hep/DetElement.h"
#include "DD4hep/Volumes.h"
#include "DDRec/DetectorData.h"
#include "RtypesCore.h"
#include "TGeoManager.h"
#include "TGeoMatrix.h"

Acts::DD4hepLayerBuilder::DD4hepLayerBuilder(
    const Acts::DD4hepLayerBuilder::Config& config,
    std::unique_ptr<const Logger> logger)
    : m_cfg(), m_logger(std::move(logger)) {
  setConfiguration(config);
}

Acts::DD4hepLayerBuilder::~DD4hepLayerBuilder() = default;

void Acts::DD4hepLayerBuilder::setConfiguration(
    const Acts::DD4hepLayerBuilder::Config& config) {
  m_cfg = config;
}

const Acts::LayerVector Acts::DD4hepLayerBuilder::endcapLayers(
    const GeometryContext& gctx,
    const std::vector<dd4hep::DetElement>& dendcapLayers,
    const std::string& side) const {
  LayerVector layers;
  if (dendcapLayers.empty()) {
    ACTS_VERBOSE(" No layers handed over for " << side << " volume!");
  } else {
    ACTS_VERBOSE(" Received layers for " << side
                                         << " volume -> creating "
                                            "disc layers");
    // go through layers
    for (auto& detElement : dendcapLayers) {
      ACTS_VERBOSE("=> Translating layer from: " << detElement.name());
      // prepare the layer surfaces
      std::vector<std::shared_ptr<const Surface>> layerSurfaces;
      // access the extension of the layer
      // at this stage all layer detElements have extension (checked in
      // ConvertDD4hepDetector)
      auto& params = getParams(detElement);
      // collect the sensitive detector elements possibly contained by the layer
      resolveSensitive(detElement, layerSurfaces);
      // access the global transformation matrix of the layer
      auto transform =
          convertTransform(&(detElement.nominal().worldTransformation()));
      // get the shape of the layer
      TGeoShape* geoShape =
          detElement.placement().ptr()->GetVolume()->GetShape();
      // create the proto layer
      ProtoLayer pl(gctx, layerSurfaces);
      if (logger().doPrint(Logging::VERBOSE)) {
        std::stringstream ss;
        pl.toStream(ss);
        ACTS_VERBOSE("Extent from surfaces: " << ss.str());

        std::vector<double> rvalues;
        std::transform(layerSurfaces.begin(), layerSurfaces.end(),
                       std::back_inserter(rvalues), [&](const auto& surface) {
                         return VectorHelpers::perp(surface->center(gctx));
                       });
        std::ranges::sort(rvalues);
        std::vector<std::string> locs;
        std::transform(rvalues.begin(),
                       std::unique(rvalues.begin(), rvalues.end()),
                       std::back_inserter(locs),
                       [](const auto& v) { return std::to_string(v); });
        ACTS_VERBOSE(
            "-> unique r locations: " << boost::algorithm::join(locs, ", "));
      }

      if (params.contains("envelope_r_min") &&
          params.contains("envelope_r_max") &&
          params.contains("envelope_z_min") &&
          params.contains("envelope_z_max")) {
        // set the values of the proto layer in case enevelopes are handed
        // over
        pl.envelope[Acts::AxisDirection::AxisR] = {
            params.get<double>("envelope_r_min"),
            params.get<double>("envelope_r_max")};
        pl.envelope[Acts::AxisDirection::AxisZ] = {
            params.get<double>("envelope_z_min"),
            params.get<double>("envelope_z_max")};
      } else if (geoShape != nullptr) {
        TGeoTubeSeg* tube = dynamic_cast<TGeoTubeSeg*>(geoShape);
        if (tube == nullptr) {
          ACTS_ERROR(" Disc layer has wrong shape - needs to be TGeoTubeSeg!");
          throw std::logic_error{
              "Disc layer has wrong shape - needs to be TGeoTubeSeg!"};
        }
        // extract the boundaries
        double rMin = tube->GetRmin() * UnitConstants::cm;
        double rMax = tube->GetRmax() * UnitConstants::cm;
        double zMin =
            (transform.translation() -
             transform.rotation().col(2) * tube->GetDz() * UnitConstants::cm)
                .z();
        double zMax =
            (transform.translation() +
             transform.rotation().col(2) * tube->GetDz() * UnitConstants::cm)
                .z();
        if (zMin > zMax) {
          std::swap(zMin, zMax);
        }
        // check if layer has surfaces
        if (layerSurfaces.empty()) {
          ACTS_VERBOSE(" Disc layer has no sensitive surfaces.");
          // in case no surfaces are handed over the layer thickness will be
          // set to a default value to allow attaching material layers
          double z = (zMin + zMax) * 0.5;
          // create layer without surfaces
          // manually create a proto layer
          double eiz = (z != 0.) ? z - m_cfg.defaultThickness : 0.;
          double eoz = (z != 0.) ? z + m_cfg.defaultThickness : 0.;
          pl.extent.range(Acts::AxisDirection::AxisZ).set(eiz, eoz);
          pl.extent.range(Acts::AxisDirection::AxisR).set(rMin, rMax);
          pl.envelope[Acts::AxisDirection::AxisR] = {0., 0.};
          pl.envelope[Acts::AxisDirection::AxisZ] = {0., 0.};
        } else {
          ACTS_VERBOSE(" Disc layer has " << layerSurfaces.size()
                                          << " sensitive surfaces.");
          // set the values of the proto layer in case dimensions are given by
          // geometry
          pl.envelope[Acts::AxisDirection::AxisZ] = {
              std::abs(zMin - pl.min(Acts::AxisDirection::AxisZ)),
              std::abs(zMax - pl.max(Acts::AxisDirection::AxisZ))};
          pl.envelope[Acts::AxisDirection::AxisR] = {
              std::abs(rMin - pl.min(Acts::AxisDirection::AxisR)),
              std::abs(rMax - pl.max(Acts::AxisDirection::AxisR))};
          pl.extent.range(Acts::AxisDirection::AxisR).set(rMin, rMax);
        }
      } else {
        throw std::logic_error(
            std::string("Layer DetElement: ") + detElement.name() +
            std::string(" has neither a shape nor tolerances for envelopes "
                        "added to its extension. Please check your detector "
                        "constructor!"));
      }

      std::shared_ptr<Layer> endcapLayer = nullptr;

      // Check if DD4hep pre-defines the surface binning
      bool hasSurfaceBinning =
          getParamOr<bool>("surface_binning", detElement, true);
      std::size_t nPhi = 1;
      std::size_t nR = 1;
      if (hasSurfaceBinning) {
        if (params.contains("surface_binning_n_phi")) {
          nPhi = static_cast<std::size_t>(
              params.get<int>("surface_binning_n_phi"));
        }
        if (params.contains("surface_binning_n_r")) {
          nR = static_cast<std::size_t>(params.get<int>("surface_binning_n_r"));
        }
        hasSurfaceBinning = nR * nPhi > 1;
      }

      // In case the layer is sensitive
      if (detElement.volume().isSensitive()) {
        // Create the sensitive surface
        auto sensitiveSurf = createSensitiveSurface(detElement, true);
        // Create the surfaceArray
        auto sArray = std::make_unique<SurfaceArray>(sensitiveSurf);

        // create the share disc bounds
        auto dBounds = std::make_shared<const RadialBounds>(
            pl.min(Acts::AxisDirection::AxisR),
            pl.max(Acts::AxisDirection::AxisR));
        double thickness = std::abs(pl.max(Acts::AxisDirection::AxisZ) -
                                    pl.min(Acts::AxisDirection::AxisZ));
        // Create the layer containing the sensitive surface
        endcapLayer = DiscLayer::create(transform, dBounds, std::move(sArray),
                                        thickness, nullptr, Acts::active);

      } else if (hasSurfaceBinning) {
        // This method uses the binning from DD4hep/xml
        endcapLayer = m_cfg.layerCreator->discLayer(
            gctx, layerSurfaces, nR, nPhi, pl, transform, nullptr);
      } else {
        // This method determines the binning automatically
        endcapLayer = m_cfg.layerCreator->discLayer(
            gctx, layerSurfaces, m_cfg.bTypeR, m_cfg.bTypePhi, pl, transform,
            nullptr);
      }
      // Add the ProtoMaterial if present
      addDiscLayerProtoMaterial(detElement, *endcapLayer, logger());
      // push back created layer
      layers.push_back(endcapLayer);
    }
  }
  return layers;
}

const Acts::LayerVector Acts::DD4hepLayerBuilder::negativeLayers(
    const GeometryContext& gctx) const {
  return endcapLayers(gctx, m_cfg.negativeLayers, "negative");
}

const Acts::LayerVector Acts::DD4hepLayerBuilder::centralLayers(
    const GeometryContext& gctx) const {
  LayerVector layers;
  if (m_cfg.centralLayers.empty()) {
    ACTS_VERBOSE(" No layers handed over for central volume!");
  } else {
    ACTS_VERBOSE(
        " Received layers for central volume -> creating "
        "cylindrical layers");
    // go through layers
    for (auto& detElement : m_cfg.centralLayers) {
      ACTS_VERBOSE("=> Translating layer from: " << detElement.name());
      // prepare the layer surfaces
      std::vector<std::shared_ptr<const Surface>> layerSurfaces;
      // access the extension of the layer
      // at this stage all layer detElements have extension (checked in
      // ConvertDD4hepDetector)
      auto& params = getParams(detElement);
      // collect the sensitive detector elements possibly contained by the layer
      resolveSensitive(detElement, layerSurfaces);
      // access the global transformation matrix of the layer
      auto transform =
          convertTransform(&(detElement.nominal().worldTransformation()));
      // get the shape of the layer
      TGeoShape* geoShape =
          detElement.placement().ptr()->GetVolume()->GetShape();
      // create the proto layer
      ProtoLayer pl(gctx, layerSurfaces);
      if (logger().doPrint(Logging::VERBOSE)) {
        std::stringstream ss;
        pl.toStream(ss);
        ACTS_VERBOSE("Extent from surfaces: " << ss.str());
        std::vector<double> zvalues;
        std::transform(layerSurfaces.begin(), layerSurfaces.end(),
                       std::back_inserter(zvalues), [&](const auto& surface) {
                         return surface->center(gctx)[eZ];
                       });
        std::ranges::sort(zvalues);
        std::vector<std::string> locs;
        std::transform(zvalues.begin(),
                       std::unique(zvalues.begin(), zvalues.end()),
                       std::back_inserter(locs),
                       [](const auto& v) { return std::to_string(v); });
        ACTS_VERBOSE(
            "-> unique z locations: " << boost::algorithm::join(locs, ", "));
      }

      if (params.contains("envelope_r_min") &&
          params.contains("envelope_r_max") &&
          params.contains("envelope_z_min") &&
          params.contains("envelope_z_max")) {
        // set the values of the proto layer in case enevelopes are handed over
        pl.envelope[Acts::AxisDirection::AxisR] = {
            params.get<double>("envelope_r_min"),
            params.get<double>("envelope_r_max")};
        pl.envelope[Acts::AxisDirection::AxisZ] = {
            params.get<double>("envelope_z_min"),
            params.get<double>("envelope_z_max")};
      } else if (geoShape != nullptr) {
        TGeoTubeSeg* tube = dynamic_cast<TGeoTubeSeg*>(geoShape);
        if (tube == nullptr) {
          ACTS_ERROR(
              " Cylinder layer has wrong shape - needs to be TGeoTubeSeg!");
          throw std::logic_error{
              " Cylinder layer has wrong shape - needs to be TGeoTubeSeg!"};
        }

        // extract the boundaries
        double rMin = tube->GetRmin() * UnitConstants::cm;
        double rMax = tube->GetRmax() * UnitConstants::cm;
        double dz = tube->GetDz() * UnitConstants::cm;
        // check if layer has surfaces
        if (layerSurfaces.empty()) {
          // in case no surfaces are handed over the layer thickness will be set
          // to a default value to allow attaching material layers
          double r = (rMin + rMax) * 0.5;
          // create layer without surfaces
          // manually create a proto layer
          double eir = (r != 0.) ? r - m_cfg.defaultThickness : 0.;
          double eor = (r != 0.) ? r + m_cfg.defaultThickness : 0.;
          pl.extent.range(Acts::AxisDirection::AxisR).set(eir, eor);
          pl.extent.range(Acts::AxisDirection::AxisZ).set(-dz, dz);
          pl.envelope[Acts::AxisDirection::AxisR] = {0., 0.};
          pl.envelope[Acts::AxisDirection::AxisZ] = {0., 0.};
        } else {
          // set the values of the proto layer in case dimensions are given by
          // geometry
          pl.envelope[Acts::AxisDirection::AxisZ] = {
              std::abs(-dz - pl.min(Acts::AxisDirection::AxisZ)),
              std::abs(dz - pl.max(Acts::AxisDirection::AxisZ))};
          pl.envelope[Acts::AxisDirection::AxisR] = {
              std::abs(rMin - pl.min(Acts::AxisDirection::AxisR)),
              std::abs(rMax - pl.max(Acts::AxisDirection::AxisR))};
        }
      } else {
        throw std::logic_error(
            std::string("Layer DetElement: ") + detElement.name() +
            std::string(" has neither a shape nor tolerances for envelopes "
                        "added to it¥s extension. Please check your detector "
                        "constructor!"));
      }

      double halfZ = (pl.min(Acts::AxisDirection::AxisZ) -
                      pl.max(Acts::AxisDirection::AxisZ)) *
                     0.5;

      std::shared_ptr<Layer> centralLayer = nullptr;
      // In case the layer is sensitive
      if (detElement.volume().isSensitive()) {
        // Create the sensitive surface
        auto sensitiveSurf = createSensitiveSurface(detElement);
        // Create the surfaceArray
        std::unique_ptr<Acts::SurfaceArray> sArray =
            std::make_unique<SurfaceArray>(sensitiveSurf);

        // create the layer
        double layerR = (pl.min(Acts::AxisDirection::AxisR) +
                         pl.max(Acts::AxisDirection::AxisR)) *
                        0.5;
        double thickness = std::abs(pl.max(Acts::AxisDirection::AxisR) -
                                    pl.min(Acts::AxisDirection::AxisR));
        auto cBounds = std::make_shared<CylinderBounds>(layerR, halfZ);
        // Create the layer containing the sensitive surface
        centralLayer =
            CylinderLayer::create(transform, cBounds, std::move(sArray),
                                  thickness, nullptr, Acts::active);

      } else {
        centralLayer = m_cfg.layerCreator->cylinderLayer(
            gctx, layerSurfaces, m_cfg.bTypePhi, m_cfg.bTypeZ, pl, transform,
            nullptr);
      }
      // Add the ProtoMaterial if present
      addCylinderLayerProtoMaterial(detElement, *centralLayer, logger());
      // push back created layer
      layers.push_back(centralLayer);
    }
  }
  return layers;
}

const Acts::LayerVector Acts::DD4hepLayerBuilder::positiveLayers(
    const GeometryContext& gctx) const {
  return endcapLayers(gctx, m_cfg.positiveLayers, "positive");
}

void Acts::DD4hepLayerBuilder::resolveSensitive(
    const dd4hep::DetElement& detElement,
    std::vector<std::shared_ptr<const Acts::Surface>>& surfaces) const {
  const dd4hep::DetElement::Children& children = detElement.children();
  if (!children.empty()) {
    for (auto& child : children) {
      dd4hep::DetElement childDetElement = child.second;
      if (childDetElement.volume().isSensitive()) {
        // create the surface
        surfaces.push_back(createSensitiveSurface(childDetElement, false));
      }
      resolveSensitive(childDetElement, surfaces);
    }
  }
}

std::shared_ptr<const Acts::Surface>
Acts::DD4hepLayerBuilder::createSensitiveSurface(
    const dd4hep::DetElement& detElement, bool isDisc) const {
  std::string detAxis =
      getParamOr<std::string>("axis_definitions", detElement, "XYZ");
  // Create the corresponding detector element !- memory leak --!
  auto dd4hepDetElement = std::make_shared<Acts::DD4hepDetectorElement>(
      detElement, detAxis, UnitConstants::cm, isDisc, nullptr);

  detElement.addExtension<DD4hepDetectorElementExtension>(
      new dd4hep::rec::StructExtension(
          DD4hepDetectorElementExtension(dd4hepDetElement)));

  // return the surface
  return dd4hepDetElement->surface().getSharedPtr();
}

Acts::Transform3 Acts::DD4hepLayerBuilder::convertTransform(
    const TGeoMatrix* tGeoTrans) const {
  // get the placement and orientation in respect to its mother
  const Double_t* rotation = tGeoTrans->GetRotationMatrix();
  const Double_t* translation = tGeoTrans->GetTranslation();
  return TGeoPrimitivesHelper::makeTransform(
      Acts::Vector3(rotation[0], rotation[3], rotation[6]),
      Acts::Vector3(rotation[1], rotation[4], rotation[7]),
      Acts::Vector3(rotation[2], rotation[5], rotation[8]),
      Acts::Vector3(translation[0] * UnitConstants::cm,
                    translation[1] * UnitConstants::cm,
                    translation[2] * UnitConstants::cm));
}

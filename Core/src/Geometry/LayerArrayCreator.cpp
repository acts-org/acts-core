// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/LayerArrayCreator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryObjectSorter.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/NavigationLayer.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArrayXD.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <algorithm>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

std::unique_ptr<const Acts::LayerArray> Acts::LayerArrayCreator::layerArray(
    const GeometryContext& gctx, const LayerVector& layersInput, double min,
    double max, BinningType bType, AxisDirection aDir) const {
  ACTS_VERBOSE("Build LayerArray with " << layersInput.size()
                                        << " layers at input.");
  ACTS_VERBOSE("       min/max provided : " << min << " / " << max);
  ACTS_VERBOSE("       binning type     : " << bType);
  ACTS_VERBOSE("       binning value    : " << aDir);

  // create a local copy of the layer vector
  LayerVector layers(layersInput);

  // sort it accordingly to the binning value
  GeometryObjectSorterT<std::shared_ptr<const Layer>> layerSorter(gctx, aDir);
  std::ranges::sort(layers, layerSorter);
  // useful typedef
  using LayerOrderPosition = std::pair<std::shared_ptr<const Layer>, Vector3>;
  // needed for all cases
  std::shared_ptr<const Layer> layer = nullptr;
  std::unique_ptr<const BinUtility> binUtility = nullptr;
  std::vector<LayerOrderPosition> layerOrderVector;

  // switch the binning type
  switch (bType) {
    // equidistant binning - no navigation layers built - only equdistant layers
    case equidistant: {
      // loop over layers and put them in
      for (auto& layIter : layers) {
        ACTS_VERBOSE("equidistant : registering a Layer at binning position : "
                     << (layIter->referencePosition(gctx, aDir)));
        layerOrderVector.push_back(LayerOrderPosition(
            layIter, layIter->referencePosition(gctx, aDir)));
      }
      // create the binUitlity
      binUtility = std::make_unique<const BinUtility>(layers.size(), min, max,
                                                      open, aDir);
      ACTS_VERBOSE("equidistant : created a BinUtility as " << *binUtility);
    } break;

    // arbitrary binning
    case arbitrary: {
      std::vector<float> boundaries;
      // initial step
      boundaries.push_back(min);
      double layerValue = 0.;
      double layerThickness = 0.;
      std::shared_ptr<const Layer> navLayer = nullptr;
      std::shared_ptr<const Layer> lastLayer = nullptr;
      // loop over layers
      for (auto& layIter : layers) {
        // estimate the offset
        layerThickness = layIter->thickness();
        layerValue = layIter->referencePositionValue(gctx, aDir);
        // register the new boundaries in the step vector
        boundaries.push_back(layerValue - 0.5 * layerThickness);
        boundaries.push_back(layerValue + 0.5 * layerThickness);
        // calculate the layer value for the offset
        double navigationValue = 0.5 * ((layerValue - 0.5 * layerThickness) +
                                        boundaries.at(boundaries.size() - 3));
        // if layers are attached to each other bail out - navigation will not
        // work anymore
        if (navigationValue == (layerValue - 0.5 * layerThickness)) {
          ACTS_ERROR(
              "Layers are attached to each other at: "
              << layerValue - 0.5 * layerThickness
              << ", which corrupts "
                 "navigation. This should never happen. Please detach the "
                 "layers in your geometry description.");
        }
        // if layers are overlapping bail out
        if (navigationValue > (layerValue - 0.5 * layerThickness)) {
          ACTS_ERROR("Layers are overlapping at: "
                     << layerValue - 0.5 * layerThickness
                     << ". This should never happen. "
                        "Please check your geometry description.");
        }

        // create the navigation layer surface from the layer
        std::shared_ptr<const Surface> navLayerSurface =
            createNavigationSurface(gctx, *layIter, aDir,
                                    -std::abs(layerValue - navigationValue));
        ACTS_VERBOSE(
            "arbitrary : creating a  NavigationLayer at "
            << (navLayerSurface->referencePosition(gctx, aDir)).x() << ", "
            << (navLayerSurface->referencePosition(gctx, aDir)).y() << ", "
            << (navLayerSurface->referencePosition(gctx, aDir)).z());
        navLayer = NavigationLayer::create(std::move(navLayerSurface));
        // push the navigation layer in
        layerOrderVector.push_back(LayerOrderPosition(
            navLayer, navLayer->referencePosition(gctx, aDir)));

        // push the original layer in
        layerOrderVector.push_back(LayerOrderPosition(
            layIter, layIter->referencePosition(gctx, aDir)));
        ACTS_VERBOSE("arbitrary : registering MaterialLayer at  "
                     << (layIter->referencePosition(gctx, aDir)).x() << ", "
                     << (layIter->referencePosition(gctx, aDir)).y() << ", "
                     << (layIter->referencePosition(gctx, aDir)).z());
        // remember the last
        lastLayer = layIter;
      }
      // a final navigation layer
      // calculate the layer value for the offset
      double navigationValue =
          0.5 * (boundaries.at(boundaries.size() - 1) + max);
      // create navigation layer only when necessary
      if (navigationValue != max && lastLayer != nullptr) {
        // create the navigation layer surface from the layer
        std::shared_ptr<const Surface> navLayerSurface =
            createNavigationSurface(gctx, *lastLayer, aDir,
                                    navigationValue - layerValue);
        ACTS_VERBOSE(
            "arbitrary : creating a  NavigationLayer at "
            << (navLayerSurface->referencePosition(gctx, aDir)).x() << ", "
            << (navLayerSurface->referencePosition(gctx, aDir)).y() << ", "
            << (navLayerSurface->referencePosition(gctx, aDir)).z());
        navLayer = NavigationLayer::create(std::move(navLayerSurface));
        // push the navigation layer in
        layerOrderVector.push_back(LayerOrderPosition(
            navLayer, navLayer->referencePosition(gctx, aDir)));
      }
      // now close the boundaries
      boundaries.push_back(max);
      // some screen output
      ACTS_VERBOSE(layerOrderVector.size()
                   << " Layers (material + navigation) built. ");
      // create the BinUtility
      binUtility = std::make_unique<const BinUtility>(boundaries, open, aDir);
      ACTS_VERBOSE("arbitrary : created a BinUtility as " << *binUtility);

    } break;
    // default return nullptr
    default: {
      return nullptr;
    }
  }
  // return the binned array
  return std::make_unique<const BinnedArrayXD<LayerPtr>>(layerOrderVector,
                                                         std::move(binUtility));
}

std::shared_ptr<Acts::Surface> Acts::LayerArrayCreator::createNavigationSurface(
    const GeometryContext& gctx, const Layer& layer, AxisDirection aDir,
    double offset) const {
  // surface reference
  const Surface& layerSurface = layer.surfaceRepresentation();
  // translation to be applied
  Vector3 translation(0., 0., 0.);
  // switching he binnig values
  switch (aDir) {
    // case x
    case AxisDirection::AxisX: {
      translation = Vector3(offset, 0., 0.);
    } break;
    // case y
    case AxisDirection::AxisY: {
      translation = Vector3(0., offset, 0.);
    } break;
    // case z
    case AxisDirection::AxisZ: {
      translation = Vector3(0., 0., offset);
    } break;
    // case R
    case AxisDirection::AxisR: {
      // binning in R and cylinder surface means something different
      if (layerSurface.type() == Surface::Cylinder) {
        break;
      }
      translation = Vector3(offset, 0., 0.);
    } break;
    // do nothing for the default
    default: {
      ACTS_WARNING("Not yet implemented.");
    }
  }
  // navigation surface
  std::shared_ptr<Surface> navigationSurface;
  // for everything else than a cylinder it's a copy with shift
  if (layerSurface.type() == Surface::Plane) {
    // create a transform that does the shift
    Transform3 shift = Transform3(Translation3(translation));
    const PlaneSurface* plane =
        dynamic_cast<const PlaneSurface*>(&layerSurface);
    navigationSurface = Surface::makeShared<PlaneSurface>(gctx, *plane, shift);
  } else if (layerSurface.type() == Surface::Disc) {
    // create a transform that does the shift
    Transform3 shift = Transform3(Translation3(translation));
    const DiscSurface* disc = dynamic_cast<const DiscSurface*>(&layerSurface);
    navigationSurface = Surface::makeShared<DiscSurface>(gctx, *disc, shift);
  } else if (layerSurface.type() == Surface::Cylinder) {
    // get the bounds
    const CylinderBounds* cBounds =
        dynamic_cast<const CylinderBounds*>(&(layerSurface.bounds()));
    double navigationR = cBounds->get(CylinderBounds::eR) + offset;
    double halflengthZ = cBounds->get(CylinderBounds::eHalfLengthZ);
    // new navigation layer
    auto cylinderBounds =
        std::make_shared<CylinderBounds>(navigationR, halflengthZ);
    navigationSurface = Surface::makeShared<CylinderSurface>(
        layerSurface.transform(gctx), cylinderBounds);
  } else {
    ACTS_WARNING("Not implemented.");
  }
  return navigationSurface;
}

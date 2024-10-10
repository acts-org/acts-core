// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Visualization/GeometryView3D.hpp"

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/IAxis.hpp"
#include "Acts/Utilities/UnitVectors.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

void Acts::GeometryView3D::drawPolyhedron(IVisualization3D& helper,
                                          const Polyhedron& polyhedron,
                                          const ViewConfig& viewConfig) {
  polyhedron.visualize(helper, viewConfig);
}

void Acts::GeometryView3D::drawSurface(IVisualization3D& helper,
                                       const Surface& surface,
                                       const GeometryContext& gctx,
                                       const Transform3& /*transform*/,
                                       const ViewConfig& viewConfig) {
  surface.visualize(helper, gctx, viewConfig);
}

void Acts::GeometryView3D::drawSurfaceArray(
    IVisualization3D& helper, const SurfaceArray& surfaceArray,
    const GeometryContext& gctx, const Transform3& transform,
    const ViewConfig& sensitiveConfig, const ViewConfig& passiveConfig,
    const ViewConfig& gridConfig, const std::filesystem::path& outputDir) {
  // Draw all the surfaces
  Extent arrayExtent;
  for (const auto& sf : surfaceArray.surfaces()) {
    ViewConfig vConfig = sf->associatedDetectorElement() != nullptr
                             ? sensitiveConfig
                             : passiveConfig;
    drawSurface(helper, *sf, gctx, transform, vConfig);
    auto sfExtent = sf->polyhedronRepresentation(gctx, 1).extent();
    arrayExtent.extend(sfExtent);
  }

  if (!sensitiveConfig.outputName.empty()) {
    helper.write(outputDir / sensitiveConfig.outputName);
    helper.clear();
  }

  double thickness = gridConfig.lineThickness;
  // Draw the grid itself
  auto binning = surfaceArray.binningValues();
  auto axes = surfaceArray.getAxes();
  if (!binning.empty() && binning.size() == 2 && axes.size() == 2) {
    // Cylinder surface array
    if (binning[0] == BinningValue::binPhi &&
        binning[1] == BinningValue::binZ) {
      double R = arrayExtent.medium(BinningValue::binR) + gridConfig.offset;
      auto phiValues = axes[0]->getBinEdges();
      auto zValues = axes[1]->getBinEdges();
      ViewConfig gridRadConfig = gridConfig;
      // Longitudinal lines
      for (auto phi : phiValues) {
        double cphi = std::cos(phi);
        double sphi = std::sin(phi);
        Vector3 p1(R * cphi, R * sphi, axes[1]->getMin());
        Vector3 p0(R * cphi, R * sphi, axes[1]->getMax());
        drawSegment(helper, transform * p0, transform * p1, gridConfig);
      }
      CylinderVolumeBounds cvb(R - 0.5 * thickness, R + 0.5 * thickness,
                               0.5 * thickness);
      auto cvbOrientedSurfaces = cvb.orientedSurfaces();
      for (auto z : zValues) {
        for (const auto& cvbSf : cvbOrientedSurfaces) {
          drawSurface(helper, *cvbSf.surface, gctx,
                      Translation3(0., 0., z) * transform, gridRadConfig);
        }
      }

    } else if (binning[0] == BinningValue::binR &&
               binning[1] == BinningValue::binPhi) {
      double z = arrayExtent.medium(BinningValue::binZ) + gridConfig.offset;
      auto rValues = axes[0]->getBinEdges();
      auto phiValues = axes[1]->getBinEdges();
      ViewConfig gridRadConfig = gridConfig;
      gridRadConfig.quarterSegments = phiValues.size();
      for (auto r : rValues) {
        CylinderVolumeBounds cvb(r - 0.5 * thickness, r + 0.5 * thickness,
                                 0.5 * thickness);
        auto cvbOrientedSurfaces = cvb.orientedSurfaces();
        for (const auto& cvbSf : cvbOrientedSurfaces) {
          drawSurface(helper, *cvbSf.surface, gctx,
                      Translation3(0., 0., z) * transform, gridRadConfig);
        }
      }
      double rMin = axes[0]->getMin();
      double rMax = axes[0]->getMax();
      for (auto phi : phiValues) {
        double cphi = std::cos(phi);
        double sphi = std::sin(phi);
        Vector3 p1(rMax * cphi, rMax * sphi, z);
        Vector3 p0(rMin * cphi, rMin * sphi, z);
        drawSegment(helper, transform * p0, transform * p1, gridConfig);
      }
    }
  }

  if (!gridConfig.outputName.empty()) {
    helper.write(outputDir / gridConfig.outputName);
    helper.clear();
  }
}

void Acts::GeometryView3D::drawVolume(IVisualization3D& helper,
                                      const Volume& volume,
                                      const GeometryContext& gctx,
                                      const Transform3& /*transform*/,
                                      const ViewConfig& viewConfig) {
  volume.visualize(helper, gctx, viewConfig);
}

void Acts::GeometryView3D::drawPortal(IVisualization3D& helper,
                                      const Experimental::Portal& portal,
                                      const GeometryContext& gctx,
                                      const Transform3& transform,
                                      const ViewConfig& connected,
                                      const ViewConfig& disconnected) {
  // color the portal based on if it contains two links(green)
  // or one link(red)
  auto surface = &(portal.surface());
  auto links = &(portal.portalNavigation());
  if (links->size() == 2) {
    drawSurface(helper, *surface, gctx, transform, connected);
  } else {
    drawSurface(helper, *surface, gctx, transform, disconnected);
  }
}

void Acts::GeometryView3D::drawDetectorVolume(
    IVisualization3D& helper, const Experimental::DetectorVolume& volume,
    const GeometryContext& gctx, const Transform3& transform,
    const ViewConfig& connected, const ViewConfig& unconnected,
    const ViewConfig& viewConfig) {
  // draw the surfaces of the mother volume
  for (auto surface : volume.surfaces()) {
    drawSurface(helper, *surface, gctx, transform, viewConfig);
  }

  // draw the envelope first
  for (auto portal : volume.portals()) {
    drawPortal(helper, *portal, gctx, transform, connected, unconnected);
  }

  // recurse if there are subvolumes
  for (auto subvolume : volume.volumes()) {
    drawDetectorVolume(helper, *subvolume, gctx, transform, connected,
                       unconnected, viewConfig);
  }
}

void Acts::GeometryView3D::drawLayer(
    IVisualization3D& helper, const Layer& layer, const GeometryContext& gctx,
    const ViewConfig& layerConfig, const ViewConfig& sensitiveConfig,
    const ViewConfig& gridConfig, const std::filesystem::path& outputDir) {
  if (layerConfig.visible) {
    auto layerVolume = layer.representingVolume();
    if (layerVolume != nullptr) {
      drawVolume(helper, *layerVolume, gctx, Transform3::Identity(),
                 layerConfig);
    } else {
      const auto& layerSurface = layer.surfaceRepresentation();
      drawSurface(helper, layerSurface, gctx, Transform3::Identity(),
                  layerConfig);
    }
    if (!layerConfig.outputName.empty()) {
      helper.write(outputDir / layerConfig.outputName);
      helper.clear();
    }
  }

  if (sensitiveConfig.visible || gridConfig.visible) {
    auto surfaceArray = layer.surfaceArray();
    if (surfaceArray != nullptr) {
      drawSurfaceArray(helper, *surfaceArray, gctx, Transform3::Identity(),
                       sensitiveConfig, layerConfig, gridConfig, outputDir);
    }
  }
}

void Acts::GeometryView3D::drawTrackingVolume(
    IVisualization3D& helper, const TrackingVolume& tVolume,
    const GeometryContext& gctx, const ViewConfig& containerView,
    const ViewConfig& volumeView, const ViewConfig& layerView,
    const ViewConfig& sensitiveView, const ViewConfig& gridView, bool writeIt,
    const std::string& tag, const std::filesystem::path& outputDir) {
  if (tVolume.confinedVolumes() != nullptr) {
    const auto& subVolumes = tVolume.confinedVolumes()->arrayObjects();
    for (const auto& tv : subVolumes) {
      drawTrackingVolume(helper, *tv, gctx, containerView, volumeView,
                         layerView, sensitiveView, gridView, writeIt, tag,
                         outputDir);
    }
  }

  ViewConfig cConfig = containerView;
  ViewConfig vConfig = volumeView;
  ViewConfig lConfig = layerView;
  ViewConfig sConfig = sensitiveView;
  ViewConfig gConfig = gridView;
  gConfig.quarterSegments = 8;

  ViewConfig vcConfig = cConfig;
  std::string vname = tVolume.volumeName();
  if (writeIt) {
    std::vector<std::string> repChar = {"::" /*, "|", " ", "{", "}"*/};
    for (const auto& rchar : repChar) {
      while (vname.find(rchar) != std::string::npos) {
        vname.replace(vname.find(rchar), rchar.size(), std::string("_"));
      }
    }
    if (tVolume.confinedVolumes() == nullptr) {
      vcConfig = vConfig;
      vcConfig.outputName =
          std::filesystem::path(vname + std::string("_boundaries") + tag);
    } else {
      std::vector<GeometryIdentifier::Value> ids{tVolume.geometryId().volume()};

      for (const auto* current = &tVolume; current->motherVolume() != nullptr;
           current = current->motherVolume()) {
        ids.push_back(current->motherVolume()->geometryId().volume());
      }

      std::ranges::reverse(ids);
      vname = "Container";
      for (const auto& id : ids) {
        vname += "_v" + std::to_string(id);
      }

      vcConfig.outputName =
          std::filesystem::path(vname + std::string("_boundaries") + tag);
    }
  }

  auto bSurfaces = tVolume.boundarySurfaces();
  for (const auto& bs : bSurfaces) {
    drawSurface(helper, bs->surfaceRepresentation(), gctx,
                Transform3::Identity(), vcConfig);
  }
  if (writeIt) {
    const std::filesystem::path outputName = outputDir / vcConfig.outputName;
    helper.write(outputName);
    helper.clear();
  }

  if (tVolume.confinedLayers() != nullptr) {
    const auto& layers = tVolume.confinedLayers()->arrayObjects();
    std::size_t il = 0;
    for (const auto& tl : layers) {
      if (writeIt) {
        lConfig.outputName = std::filesystem::path(
            vname + std::string("_passives_l") + std::to_string(il) + tag);
        sConfig.outputName = std::filesystem::path(
            vname + std::string("_sensitives_l") + std::to_string(il) + tag);
        gConfig.outputName = std::filesystem::path(
            vname + std::string("_grids_l") + std::to_string(il) + tag);
      }
      drawLayer(helper, *tl, gctx, lConfig, sConfig, gConfig, outputDir);
      ++il;
    }
  }
}

void Acts::GeometryView3D::drawSegmentBase(IVisualization3D& helper,
                                           const Vector3& start,
                                           const Vector3& end, int arrows,
                                           double arrowLength,
                                           double arrowWidth,
                                           const ViewConfig& viewConfig) {
  double thickness = viewConfig.lineThickness;

  // Draw the parameter shaft and cone
  auto direction = Vector3(end - start).normalized();
  double hlength = 0.5 * Vector3(end - start).norm();

  auto unitVectors = makeCurvilinearUnitVectors(direction);
  RotationMatrix3 lrotation;
  lrotation.col(0) = unitVectors.first;
  lrotation.col(1) = unitVectors.second;
  lrotation.col(2) = direction;

  Vector3 lcenter = 0.5 * (start + end);
  double alength = (thickness > 0.) ? arrowLength * thickness : 2.;
  if (alength > hlength) {
    alength = hlength;
  }

  if (arrows == 2) {
    hlength -= alength;
  } else if (arrows != 0) {
    hlength -= 0.5 * alength;
    lcenter -= Vector3(arrows * 0.5 * alength * direction);
  }

  // Line - draw a line
  if (thickness > 0.) {
    auto ltransform = Transform3::Identity();
    ltransform.prerotate(lrotation);
    ltransform.pretranslate(lcenter);

    auto lbounds = std::make_shared<CylinderBounds>(thickness, hlength);
    auto line = Surface::makeShared<CylinderSurface>(ltransform, lbounds);

    drawSurface(helper, *line, GeometryContext(), Transform3::Identity(),
                viewConfig);
  } else {
    helper.line(start, end, viewConfig.color);
  }

  // Arrowheads - if configured
  if (arrows != 0) {
    double awith = thickness * arrowWidth;
    double alpha = atan2(thickness * arrowWidth, alength);
    auto plateBounds = std::make_shared<RadialBounds>(thickness, awith);

    if (arrows > 0) {
      auto aetransform = Transform3::Identity();
      aetransform.prerotate(lrotation);
      aetransform.pretranslate(end);
      // Arrow cone
      auto coneBounds = std::make_shared<ConeBounds>(alpha, -alength, 0.);
      auto cone = Surface::makeShared<ConeSurface>(aetransform, coneBounds);
      drawSurface(helper, *cone, GeometryContext(), Transform3::Identity(),
                  viewConfig);
      // Arrow end plate
      auto aptransform = Transform3::Identity();
      aptransform.prerotate(lrotation);
      aptransform.pretranslate(Vector3(end - alength * direction));

      auto plate = Surface::makeShared<DiscSurface>(aptransform, plateBounds);
      drawSurface(helper, *plate, GeometryContext(), Transform3::Identity(),
                  viewConfig);
    }
    if (arrows < 0 || arrows == 2) {
      auto astransform = Transform3::Identity();
      astransform.prerotate(lrotation);
      astransform.pretranslate(start);

      // Arrow cone
      auto coneBounds = std::make_shared<ConeBounds>(alpha, 0., alength);
      auto cone = Surface::makeShared<ConeSurface>(astransform, coneBounds);
      drawSurface(helper, *cone, GeometryContext(), Transform3::Identity(),
                  viewConfig);
      // Arrow end plate
      auto aptransform = Transform3::Identity();
      aptransform.prerotate(lrotation);
      aptransform.pretranslate(Vector3(start + alength * direction));

      auto plate = Surface::makeShared<DiscSurface>(aptransform, plateBounds);
      drawSurface(helper, *plate, GeometryContext(), Transform3::Identity(),
                  viewConfig);
    }
  }
}

void Acts::GeometryView3D::drawSegment(IVisualization3D& helper,
                                       const Vector3& start, const Vector3& end,
                                       const ViewConfig& viewConfig) {
  drawSegmentBase(helper, start, end, 0, 0., 0., viewConfig);
}

void Acts::GeometryView3D::drawArrowBackward(
    IVisualization3D& helper, const Vector3& start, const Vector3& end,
    double arrowLength, double arrowWidth, const ViewConfig& viewConfig) {
  drawSegmentBase(helper, start, end, -1, arrowLength, arrowWidth, viewConfig);
}

void Acts::GeometryView3D::drawArrowForward(
    IVisualization3D& helper, const Vector3& start, const Vector3& end,
    double arrowLength, double arrowWidth, const ViewConfig& viewConfig) {
  drawSegmentBase(helper, start, end, 1, arrowLength, arrowWidth, viewConfig);
}

void Acts::GeometryView3D::drawArrowsBoth(IVisualization3D& helper,
                                          const Vector3& start,
                                          const Vector3& end,
                                          double arrowLength, double arrowWidth,
                                          const ViewConfig& viewConfig) {
  drawSegmentBase(helper, start, end, 2, arrowLength, arrowWidth, viewConfig);
}

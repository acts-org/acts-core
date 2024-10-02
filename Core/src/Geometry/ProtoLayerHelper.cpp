// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/ProtoLayerHelper.hpp"

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <array>
#include <ostream>
#include <string>

std::vector<Acts::ProtoLayer> Acts::ProtoLayerHelper::protoLayers(
    const GeometryContext& gctx, const std::vector<const Surface*>& surfaces,
    const SortingConfig& sorting) const {
  std::vector<Acts::ProtoLayer> protoLayers;

  using SurfaceCluster = std::pair<Extent, std::vector<const Surface*>>;
  std::vector<SurfaceCluster> clusteredSurfaces;
  /// Helper function to find/create the cluster of surfaces where
  /// the Extent belongs to. In case none is found, a new one is inserted
  ///
  /// @param extent The test extent for finding the cluster
  ///
  /// @return the reference of the SurfaceCluster for insertion
  auto findCluster = [&](const Extent& extent) -> SurfaceCluster& {
    for (auto& cluster : clusteredSurfaces) {
      if (cluster.first.intersects(extent, sorting.first)) {
        return cluster;
      }
    }
    // No cluster found, let's create a new one
    clusteredSurfaces.push_back(SurfaceCluster(extent, {}));
    return clusteredSurfaces.back();
  };

  // Loop over surfaces and sort into clusters
  for (auto& sf : surfaces) {
    // To prevent problematic isInsidePolygon check for straw surfaces with only
    // one lseg
    int lseg = (sf->type() != Surface::Straw) ? 1 : 2;
    auto sfExtent = sf->polyhedronRepresentation(gctx, lseg).extent();
    sfExtent.envelope()[sorting.first] = {sorting.second, sorting.second};
    auto& sfCluster = findCluster(sfExtent);
    sfCluster.first.extend(sfExtent);
    sfCluster.second.push_back(sf);
  }
  // Loop over clusters and create ProtoLayer
  protoLayers.reserve(clusteredSurfaces.size());
  for (auto& clusters : clusteredSurfaces) {
    ACTS_VERBOSE("Creating ProtoLayer with " << clusters.second.size()
                                             << " surfaces.");
    protoLayers.push_back(ProtoLayer(gctx, clusters.second));
  }
  return protoLayers;
}

std::vector<Acts::ProtoLayer> Acts::ProtoLayerHelper::protoLayers(
    const GeometryContext& gctx, const std::vector<const Surface*>& surfaces,
    const std::vector<SortingConfig>& sortings) const {
  ACTS_DEBUG("Received " << surfaces.size() << " surfaces at input.");
  std::vector<std::vector<const Surface*>> sortSurfaces = {surfaces};
  for (const auto& sorting : sortings) {
    ACTS_VERBOSE("-> Sorting a set of " << sortSurfaces.size() << " in "
                                        << binningValueName(sorting.first));
    std::vector<std::vector<const Surface*>> subSurfaces;
    for (const auto& ssurfaces : sortSurfaces) {
      ACTS_VERBOSE("-> Surfaces for this sorting step: " << ssurfaces.size());
      auto pLayers = protoLayers(gctx, ssurfaces, sorting);
      ACTS_VERBOSE("-> Resulted in " << pLayers.size() << " ProtoLayers.");
      for (const auto& pLayer : pLayers) {
        ACTS_VERBOSE("--> ProtoLayer contains " << pLayer.surfaces().size()
                                                << " surfaces.");
        subSurfaces.push_back(pLayer.surfaces());
      }
    }
    sortSurfaces = subSurfaces;
  }
  ACTS_DEBUG("Yielded " << sortSurfaces.size() << " at output.");

  std::vector<Acts::ProtoLayer> finalProtoLayers;

  for (const auto& ssurfaces : sortSurfaces) {
    finalProtoLayers.push_back(ProtoLayer(gctx, ssurfaces));
  }

  return finalProtoLayers;
}

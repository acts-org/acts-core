// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArrayXD.hpp"

namespace Acts {

///  helper function to create a cylinder
TrackingVolumePtr constructCylinderVolume(
    const GeometryContext& gctx, double surfaceHalfLengthZ, double surfaceR,
    double surfaceRstagger, double surfaceZoverlap, double layerEnvelope,
    double volumeEnvelope, double innerVolumeR, double outerVolumeR,
    const std::string& name) {
  ///  the surface transforms
  auto sfnPosition = Vector3(0., 0., -3 * surfaceHalfLengthZ - surfaceZoverlap);
  auto sfnTransform = Transform3(Translation3(sfnPosition));
  auto sfcTransform = Transform3::Identity();
  auto sfpPosition = Vector3(0., 0., 3 * surfaceHalfLengthZ - surfaceZoverlap);
  auto sfpTransform = Transform3(Translation3(sfpPosition));
  ///  the surfaces
  auto sfnBounds = std::make_shared<CylinderBounds>(
      surfaceR - 0.5 * surfaceRstagger, surfaceHalfLengthZ);
  auto sfn = Surface::makeShared<CylinderSurface>(sfnTransform, sfnBounds);
  auto sfcBounds = std::make_shared<CylinderBounds>(
      surfaceR + 0.5 * surfaceRstagger, surfaceHalfLengthZ);
  auto sfc = Surface::makeShared<CylinderSurface>(sfcTransform, sfcBounds);
  auto sfpBounds = std::make_shared<CylinderBounds>(
      surfaceR - 0.5 * surfaceRstagger, surfaceHalfLengthZ);
  auto sfp = Surface::makeShared<CylinderSurface>(sfpTransform, sfpBounds);

  ///  prepare the surfaces

  ///  make the binned array
  double bUmin = sfnPosition.z() - surfaceHalfLengthZ;
  double bUmax = sfpPosition.z() + surfaceHalfLengthZ;

  std::vector<std::shared_ptr<const Surface>> surfaces_only = {{sfn, sfc, sfp}};
  std::vector<const Surface*> surfaces_only_raw = {
      {sfn.get(), sfc.get(), sfp.get()}};

  Axis<AxisType::Equidistant, AxisBoundaryType::Bound> axis(
      bUmin, bUmax, surfaces_only.size());
  auto g2l = [](const Vector3& glob) {
    return std::array<double, 1>({{glob.z()}});
  };
  auto l2g = [](const std::array<double, 1>& loc) {
    return Vector3(0, 0, loc[0]);
  };
  auto sl = std::make_unique<SurfaceArray::SurfaceGridLookup<decltype(axis)>>(
      g2l, l2g, std::make_tuple(axis));
  sl->fill(gctx, surfaces_only_raw);
  auto bArray = std::make_unique<SurfaceArray>(std::move(sl), surfaces_only);

  ///  now create the Layer
  auto layer0bounds = std::make_shared<const CylinderBounds>(surfaceR, bUmax);
  auto layer0 = CylinderLayer::create(Transform3::Identity(), layer0bounds,
                                      std::move(bArray),
                                      surfaceRstagger + 2 * layerEnvelope);
  std::unique_ptr<const LayerArray> layerArray =
      std::make_unique<const BinnedArrayXD<LayerPtr>>(layer0);

  ///  create the volume
  auto volumeBounds = std::make_shared<CylinderVolumeBounds>(
      innerVolumeR, outerVolumeR, bUmax + volumeEnvelope);

  TrackingVolumePtr volume = std::make_shared<TrackingVolume>(
      Transform3::Identity(), volumeBounds, nullptr, std::move(layerArray),
      nullptr, MutableTrackingVolumeVector{}, name);
  ///  return the volume
  return volume;
}

///  helper function to create a container
MutableTrackingVolumePtr constructContainerVolume(const GeometryContext& gctx,
                                                  TrackingVolumePtr iVolume,
                                                  TrackingVolumePtr oVolume,
                                                  double hVolumeR,
                                                  double hVolumeHalflength,
                                                  const std::string& name) {
  ///  create the volume array
  using VAP = std::pair<TrackingVolumePtr, Vector3>;
  std::vector<VAP> volumes = {
      {iVolume, iVolume->binningPosition(gctx, BinningValue::binR)},
      {oVolume, oVolume->binningPosition(gctx, BinningValue::binR)}};
  ///  the bounds for the container
  auto hVolumeBounds =
      std::make_shared<CylinderVolumeBounds>(0., hVolumeR, hVolumeHalflength);
  ///  create the BinUtility & the BinnedArray
  auto vUtility = std::make_unique<const BinUtility>(
      volumes.size(), 0., hVolumeR, open, BinningValue::binR);
  std::shared_ptr<const TrackingVolumeArray> vArray =
      std::make_shared<const BinnedArrayXD<TrackingVolumePtr>>(
          volumes, std::move(vUtility));
  ///  create the container volume
  auto hVolume = std::make_shared<TrackingVolume>(
      Transform3::Identity(), hVolumeBounds, nullptr, nullptr, vArray,
      MutableTrackingVolumeVector{}, name);
  // return the container
  return hVolume;
}
}  // namespace Acts

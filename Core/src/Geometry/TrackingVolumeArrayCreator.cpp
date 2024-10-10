// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryObjectSorter.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArrayXD.hpp"

#include <algorithm>
#include <vector>

std::shared_ptr<const Acts::TrackingVolumeArray>
Acts::TrackingVolumeArrayCreator::trackingVolumeArray(
    const GeometryContext& gctx, const TrackingVolumeVector& tVolumes,
    BinningValue bValue) const {
  // MSG_VERBOSE("Create VolumeArray of "<< tVolumes.size() << " TrackingVolumes
  // with binning in : " << binningValueName(bValue) );
  // let's copy and sort
  TrackingVolumeVector volumes(tVolumes);
  // sort it accordingly to the binning value
  GeometryObjectSorterT<std::shared_ptr<const TrackingVolume>> volumeSorter(
      gctx, bValue);
  std::ranges::sort(volumes, volumeSorter);

  // prepare what we need :
  // (1) arbitrary binning for volumes is fast enough
  std::vector<float> boundaries;
  boundaries.reserve(tVolumes.size() + 1);
  // (2) the vector needed for the BinnedArray
  std::vector<TrackingVolumeOrderPosition> tVolumesOrdered;

  // let's loop over the (sorted) volumes
  for (auto& tVolume : volumes) {
    // get the binning position
    Vector3 binningPosition = tVolume->binningPosition(gctx, bValue);
    double binningBorder = tVolume->volumeBounds().binningBorder(bValue);
    // get the center value according to the bin
    double value = tVolume->binningPositionValue(gctx, bValue);
    // for the first one take low and high boundary
    if (boundaries.empty()) {
      boundaries.push_back(value - binningBorder);
    }
    // always take the high boundary
    boundaries.push_back(value + binningBorder);
    // record the volume to be ordered
    tVolumesOrdered.push_back(
        TrackingVolumeOrderPosition(tVolume, binningPosition));
  }

  // now create the bin utility
  auto binUtility =
      std::make_unique<const BinUtility>(boundaries, open, bValue);

  // and return the newly created binned array
  return std::make_shared<const BinnedArrayXD<TrackingVolumePtr>>(
      tVolumesOrdered, std::move(binUtility));
}

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
    AxisDirection aDir) const {
  // MSG_VERBOSE("Create VolumeArray of "<< tVolumes.size() << " TrackingVolumes
  // with binning in : " << axisDirectionName(aDir) );
  // let's copy and sort
  TrackingVolumeVector volumes(tVolumes);
  // sort it accordingly to the binning value
  GeometryObjectSorterT<std::shared_ptr<const TrackingVolume>> volumeSorter(
      gctx, aDir);
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
    Vector3 referencePosition = tVolume->referencePosition(gctx, aDir);
    double referenceBorder = tVolume->volumeBounds().referenceBorder(aDir);
    // get the center value according to the bin
    double value = tVolume->referencePositionValue(gctx, aDir);
    // for the first one take low and high boundary
    if (boundaries.empty()) {
      boundaries.push_back(value - referenceBorder);
    }
    // always take the high boundary
    boundaries.push_back(value + referenceBorder);
    // record the volume to be ordered
    tVolumesOrdered.push_back(
        TrackingVolumeOrderPosition(tVolume, referencePosition));
  }

  // now create the bin utility
  auto binUtility = std::make_unique<const BinUtility>(boundaries, open, aDir);

  // and return the newly created binned array
  return std::make_shared<const BinnedArrayXD<TrackingVolumePtr>>(
      tVolumesOrdered, std::move(binUtility));
}

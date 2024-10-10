// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Utilities/BinnedArray.hpp"

#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Acts {

class TrackingVolume;

using TrackingVolumePtr = std::shared_ptr<const TrackingVolume>;
using TrackingVolumeArray = BinnedArray<TrackingVolumePtr>;

///  @class GlueVolumesDescriptor
///
/// Descriptor class to hold GlueVolumes of a TrackingGeometry object.
/// Should ease the wrapping of a TrackingGeometry object describing one
/// Detector
/// by another one.
///
class GlueVolumesDescriptor {
 public:
  /// Constructor
  GlueVolumesDescriptor() = default;
  /// Constructor - with arguments
  ///
  /// @param gvs are the glue volume arrays mapped to the volume faces
  GlueVolumesDescriptor(
      const std::map<BoundarySurfaceFace,
                     std::shared_ptr<const TrackingVolumeArray>>& gvs);

  /// Destructor
  ~GlueVolumesDescriptor() = default;
  /// Register the volumes
  ///
  /// @param bsf is the boundary surface face where the volume array is attached
  /// @param gvs is the array of volumes to be attached
  void registerGlueVolumes(Acts::BoundarySurfaceFace bsf,
                           std::shared_ptr<const TrackingVolumeArray> gvs);

  /// Retrieve the glue volumes
  ///
  /// @param bsf is the boundary surface face for which you want to get the
  /// array
  ///
  /// @return the shared pointer to the TrackingVolume array
  std::shared_ptr<const TrackingVolumeArray> glueVolumes(
      BoundarySurfaceFace bsf) const;

  /// Retrieve the available Glue Faces
  /// @return the list of faces for which glue information is there
  const std::vector<BoundarySurfaceFace>& glueFaces() const;

  /// Dump it to the screen
  std::string screenOutput() const;

 private:
  std::map<BoundarySurfaceFace, std::shared_ptr<const TrackingVolumeArray>>
      m_glueVolumes;
  std::vector<BoundarySurfaceFace> m_glueFaces;
};

inline const std::vector<BoundarySurfaceFace>&
GlueVolumesDescriptor::glueFaces() const {
  return m_glueFaces;
}

std::ostream& operator<<(std::ostream& sl, const GlueVolumesDescriptor& gvd);
}  // namespace Acts

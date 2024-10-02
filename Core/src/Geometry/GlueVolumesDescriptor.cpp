// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/GlueVolumesDescriptor.hpp"

#include "Acts/Geometry/TrackingVolume.hpp"

#include <ostream>
#include <utility>

Acts::GlueVolumesDescriptor::GlueVolumesDescriptor(
    const std::map<BoundarySurfaceFace,
                   std::shared_ptr<const TrackingVolumeArray>>& gvs)
    : m_glueVolumes(gvs) {
  // fill the available faces
  for (auto& gvIter : m_glueVolumes) {
    m_glueFaces.push_back(gvIter.first);
  }
}

void Acts::GlueVolumesDescriptor::registerGlueVolumes(
    Acts::BoundarySurfaceFace bsf,
    std::shared_ptr<const TrackingVolumeArray> gvs) {
  // register the face
  auto searchIter = m_glueVolumes.find(bsf);
  if (searchIter == m_glueVolumes.end()) {
    m_glueFaces.push_back(bsf);
  }
  // simple assignment overwrites already existing entries
  m_glueVolumes[bsf] =
      std::move(gvs);  //!< @todo change to addGlueVolumes principle
}

std::shared_ptr<const Acts::TrackingVolumeArray>
Acts::GlueVolumesDescriptor::glueVolumes(Acts::BoundarySurfaceFace bsf) const {
  // searching for the glue volumes according
  auto searchIter = m_glueVolumes.find(bsf);
  if (searchIter != m_glueVolumes.end()) {
    return searchIter->second;
  }
  return nullptr;
}

std::string Acts::GlueVolumesDescriptor::screenOutput() const {
  std::stringstream sl;
  sl << "Acts::GlueVolumesDescriptor: " << std::endl;
  const std::vector<Acts::BoundarySurfaceFace>& glueFaceVector = glueFaces();
  sl << "     has Tracking Volumes registered for : " << glueFaceVector.size()
     << " Volume faces." << std::endl;
  // loop over the faces
  for (auto& gFace : glueFaceVector) {
    const std::vector<TrackingVolumePtr>& glueVolumesVector =
        glueVolumes(gFace)->arrayObjects();
    // loop over the TrackingVolumes
    sl << "        -----> Processing Face: " << static_cast<int>(gFace)
       << " - has ";
    sl << glueVolumesVector.size()
       << " TrackingVolumes marked as 'GlueVolumes' " << std::endl;
    for (auto& glueVolume : glueVolumesVector) {
      sl << "             - TrackingVolume: " << glueVolume->volumeName()
         << std::endl;
    }
  }
  return sl.str();
}

std::ostream& Acts::operator<<(std::ostream& sl,
                               const GlueVolumesDescriptor& gvd) {
  sl << gvd.screenOutput();
  return sl;
}

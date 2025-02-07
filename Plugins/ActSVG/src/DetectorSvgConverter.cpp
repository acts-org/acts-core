// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/ActSVG/DetectorSvgConverter.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Plugins/ActSVG/DetectorVolumeSvgConverter.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <map>

Acts::Svg::ProtoDetector Acts::Svg::DetectorConverter::convert(
    const GeometryContext& gctx, const Experimental::Detector& detector,
    const DetectorConverter::Options& detectorOptions) {
  ProtoDetector pDetector;

  const auto volumes = detector.volumes();
  // Create the index map vor volumes
  std::map<const Experimental::DetectorVolume*, unsigned int> volumeIndices;
  for (auto [iv, v] : enumerate(volumes)) {
    volumeIndices[v] = iv;
  }

  // Create the index map for portals
  unsigned int ip = 0;
  std::map<const Experimental::Portal*, unsigned int> portalIndices;
  for (const auto& v : volumes) {
    for (const auto& p : v->portals()) {
      portalIndices[p] = ip++;
    }
  }

  DetectorVolumeConverter::Options volumeOptions =
      detectorOptions.volumeOptions;
  volumeOptions.portalIndices = portalIndices;
  volumeOptions.portalOptions.volumeIndices = volumeIndices;

  // Loop over the list of volumes, remember internal ones
  for (auto [iv, v] : enumerate(volumes)) {
    auto [pVolume, pGrid] = convert(gctx, *v, volumeOptions);
    pVolume._index = iv;
    pDetector._volumes.push_back(pVolume);
  }

  return pDetector;
}

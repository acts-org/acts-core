// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Plugins/GeoModel/GeoModelToDetectorVolume.hpp"
#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <GeoModelHelpers/getChildNodesWithTrf.h>

#include "GeoModelKernel/GeoDefinitions.h"

class GeoShape;
struct GeoModelTree;
class Surface;
namespace Acts {
class GeoModelDetectorObjectFactory {
 public:
  using GeoModelBoundingBox = std::shared_ptr<Experimental::DetectorVolume>;

  struct Options {
    std::vector<std::string> queries = {};
  };

  // substring matching for additional parameters
  // empty vectors will be matched to everything
  struct Config {
    // List for names to match
    std::vector<std::string> nameList;
    // List for materials to match
    std::vector<std::string> materialList;

    // boolean flag to build subvolumes
    bool convertSubVolumes = false;

    // flag to build the desired bounding Boxes
    std::vector<std::string> convertBox;
  };

  struct Cache {
    // The created detector elements and their surfaces
    std::vector<GeoModelSensitiveSurface> sensitiveSurfaces;
    // The created representation of bounding box
    std::vector<GeoModelBoundingBox> boundingBoxes;
  };

  GeoModelDetectorObjectFactory(
      const Config& cfg,
      std::unique_ptr<const Logger> mlogger = getDefaultLogger(
          "GeoModelDetectorObjectFactory", Acts::Logging::WARNING));

  void construct(Cache& cache, const GeometryContext& gctx,
                 const GeoModelTree& geoModelTree, const Options& options);

  void convertSensitive(const PVConstLink& geoPV,
                        const Acts::Transform3& transform,
                        std::vector<GeoModelSensitiveSurface>& sensitives);

  std::vector<GeoChildNodeWithTrf> findAllSubVolumes(const PVConstLink& vol);

  bool convertBox(std::string name);
  bool matches(const std::string& name, const PVConstLink& physvol);

  void convertFpv(const std::string& name, GeoFullPhysVol* fpv, Cache& cache,
                  const GeometryContext& gctx);

 private:
  std::unique_ptr<const Logger> m_logger;
  Config m_cfg;

  const Logger& logger() const { return *m_logger; }
};
}  // namespace Acts

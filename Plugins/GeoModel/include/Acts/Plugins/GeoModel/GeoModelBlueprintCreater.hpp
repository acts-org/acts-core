// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Detector/KdtSurfacesProvider.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace Acts {

struct GeoModelTree;

/// @brief This is the GeoModel blueprint creator for the Gen2 ("Acts::Experimental::Detector")
/// geometry concept.
///
class GeoModelBlueprintCreater {
 public:
  /// The configuration struct
  struct Config {
    /// The detector surfaces - leave empty if filling is not done
    /// with a kdtree sorting structure
    std::vector<std::shared_ptr<Surface>> detectorSurfaces = {};
    /// The binning values for the KDTree sorting
    std::vector<AxisDirection> kdtBinning = {};
    /// Polyhedron approximation: number of segments per circlequarter
    unsigned int quarterSegments = 1u;
  };

  /// The cache struct
  struct Cache {
    /// The kdtree of surfaces
    std::shared_ptr<Experimental::KdtSurfaces<3u>> kdtSurfaces = nullptr;
  };

  /// The Options struct
  struct Options {
    /// The table name of the blueprint in the database
    std::string table;
    /// The top level node name
    std::string topEntry;
    /// Optionally override the top node bounds
    std::string topBoundsOverride = "";
    /// Export dot graph
    std::string dotGraph = "";
  };

  /// The Blueprint return object
  struct Blueprint {
    std::string name;
    std::unique_ptr<Acts::Experimental::Gen2Blueprint::Node> topNode;

    /// Access to the top node
    const Acts::Experimental::Gen2Blueprint::Node& node() const {
      if (topNode == nullptr) {
        throw std::runtime_error(
            "GeoModelBlueprintCreater::Blueprint: No top node created");
      }
      return *topNode;
    }
  };

  /// Table entry representing the Blueprint tablein the database
  ///
  /// Blueprint table is:
  ///
  /// cursor.execute("CREATE TABLE Blueprint(id INT, type TEXT, name TEXT,
  /// bounds TEXT, internals TEXT, binnings TEXT, materials TEXT)")
  ///
  struct TableEntry {
    int id{};
    std::string type;
    std::string name;
    std::string bounds;
    std::vector<std::string> internals;
    std::vector<std::string> binnings;
    std::vector<std::string> materials;
  };

  /// The GeoModel blueprint creator from the database
  ///
  /// @param cfg the configuration struct
  /// @param mlogger a screen output logger
  GeoModelBlueprintCreater(const Config& cfg,
                           std::unique_ptr<const Logger> mlogger =
                               getDefaultLogger("GeoModelBlueprintCreater",
                                                Acts::Logging::INFO));

  /// Method that reads the GeoModel blueprint from database
  ///
  /// @param gctx the geometry context
  /// @param gmTree the GeoModel tree
  /// @param options the options
  Blueprint create(const GeometryContext& gctx, const GeoModelTree& gmTree,
                   const Options& options) const;

 private:
  /// Create a blueprint node from a table entry
  ///
  /// @param cache the cache object
  /// @param gctx the geometry context
  /// @param entry the table entry
  /// @param tableEntryMap the map of table entries allows construction of children
  /// @param motherExtent an extent given from external parameters (e.g. mother volume)
  ///
  /// @return a newly created node
  std::unique_ptr<Experimental::Gen2Blueprint::Node> createNode(
      Cache& cache, const GeometryContext& gctx, const TableEntry& entry,
      const std::map<std::string, TableEntry>& tableEntryMap,
      const Extent& motherExtent = Extent()) const;

  /// Create an IInternalStructureBuilder
  ///
  /// @param cache the cache object
  /// @param gctx the geometry context
  /// @param entry the teable entry map
  /// @param externalExtent an extent given from external parameters (e.g. confining volume)
  /// @param interalContstraints a set of binning values to be estimated
  ///
  /// @return a newly created IInternalStructureBuilder and the internal extent from it
  std::tuple<std::shared_ptr<const Experimental::IInternalStructureBuilder>,
             Extent>
  createInternalStructureBuilder(
      Cache& cache, const GeometryContext& gctx, const TableEntry& entry,
      const Extent& externalExtent = Extent(),
      const std::vector<AxisDirection>& internalConstraints = {}) const;

  /// @brief Parse bound value string from the database
  ///
  /// @param boundsEntry in the database
  /// @param rawValuesMother the raw bound values of the mother
  /// @param externalExtent the extend from external constraints (marked "e" in the database)
  /// @param internalExtent the extend of the internal objects (marked "i" in the database)
  ///
  /// @return The bounds type, raw bound values, deduced bound values, and a translation vector
  std::tuple<VolumeBounds::BoundsType, Extent, std::vector<double>, Vector3>
  parseBounds(const std::string& boundsEntry,
              const Extent& externalExtent = Extent(),
              const Extent& internalExtent = Extent()) const;

  /// Private access to the logger
  const Logger& logger() const { return *m_logger; }

  /// The configuration
  Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts

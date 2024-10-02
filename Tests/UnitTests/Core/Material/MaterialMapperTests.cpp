// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/AccumulatedMaterialSlab.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialMapper.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"
#include "Acts/Material/interface/ISurfaceMaterialAccumulater.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <limits>

namespace Acts::Test {

auto tContext = GeometryContext();

/// @brief Interface for the material mapping that seeks the possible
/// assignment candidates for the material interactiosn
class IntersectSurfacesFinder : public IAssignmentFinder {
 public:
  std::vector<const Surface*> surfaces;

  /// @brief Interface method for generating assignment candidates for the
  /// material interaction assignment to surfaces or volumes
  ///
  /// @param gctx is the geometry context
  /// @param mctx is the magnetic field context
  /// @param position is the position of the initial ray
  /// @param direction is the direction of initial ray
  ///
  /// @return a vector of Surface Assignments and Volume Assignments
  std::pair<std::vector<IAssignmentFinder::SurfaceAssignment>,
            std::vector<IAssignmentFinder::VolumeAssignment>>
  assignmentCandidates(const GeometryContext& gctx,
                       const MagneticFieldContext& /*ignored*/,
                       const Vector3& position,
                       const Vector3& direction) const override {
    std::vector<IAssignmentFinder::SurfaceAssignment> surfaceAssignments;
    std::vector<IAssignmentFinder::VolumeAssignment> volumeAssignments;
    // Intersect the surfaces
    for (auto& surface : surfaces) {
      // Get the intersection
      auto sMultiIntersection = surface->intersect(gctx, position, direction,
                                                   BoundaryTolerance::None());
      // One solution, take it
      if (sMultiIntersection.size() == 1u &&
          sMultiIntersection[0u].status() >=
              Acts::IntersectionStatus::reachable &&
          sMultiIntersection[0u].pathLength() >= 0.0) {
        surfaceAssignments.push_back(
            {surface, sMultiIntersection[0u].position(), direction});
        continue;
      }
      if (sMultiIntersection.size() > 1u) {
        // Multiple intersections, take the closest
        auto closestForward = sMultiIntersection.closestForward();
        if (closestForward.status() >= Acts::IntersectionStatus::reachable &&
            closestForward.pathLength() > 0.0) {
          surfaceAssignments.push_back(
              {surface, closestForward.position(), direction});
          continue;
        }
      }
    }
    return {surfaceAssignments, volumeAssignments};
  }
};

/// @brief Interface for the material mapping, this is the accumulation step
class MaterialBlender : public ISurfaceMaterialAccumulater {
 public:
  MaterialBlender(const std::vector<std::shared_ptr<Surface>>& surfaces = {})
      : m_surfaces(surfaces) {}

  /// The state of the material accumulater, this is used
  /// to cache information across tracks/events
  class State final : public ISurfaceMaterialAccumulater::State {
   public:
    std::map<const Surface*, AccumulatedMaterialSlab> accumulatedMaterial;
  };

  /// Factory for creating the state
  std::unique_ptr<ISurfaceMaterialAccumulater::State> createState()
      const override {
    auto state = std::make_unique<State>();
    for (auto& surface : m_surfaces) {
      state->accumulatedMaterial[surface.get()] = AccumulatedMaterialSlab();
    }
    return state;
  };

  /// @brief Accumulate the material interaction on the surface
  ///
  /// @param state is the state of the accumulater
  /// @param interactions is the material interactions, with assigned surfaces
  /// @param surfacesWithoutAssignment are the surfaces without assignment
  ///
  /// @note this the track average over the binned material
  void accumulate(ISurfaceMaterialAccumulater::State& state,
                  const std::vector<MaterialInteraction>& interactions,
                  const std::vector<IAssignmentFinder::SurfaceAssignment>&
                  /*surfacesWithoutAssignment*/) const override {
    auto cState = static_cast<State*>(&state);
    for (const auto& mi : interactions) {
      // Get the surface
      const Surface* surface = mi.surface;
      // Get the accumulated material
      auto accMaterial = cState->accumulatedMaterial.find(surface);
      if (accMaterial == cState->accumulatedMaterial.end()) {
        throw std::invalid_argument(
            "Surface material is not found, inconsistent configuration.");
      }
      // Accumulate the material
      accMaterial->second.accumulate(mi.materialSlab);
    }
    // Average over the track
    for (auto& [surface, accumulatedMaterial] : cState->accumulatedMaterial) {
      accumulatedMaterial.trackAverage();
    }
  };

  /// Finalize the surface material maps
  ///
  /// @param state the state of the accumulator
  ///
  /// @note this does the run average over the (binned) material
  std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
  finalizeMaterial(ISurfaceMaterialAccumulater::State& state) const override {
    auto cState = static_cast<State*>(&state);

    std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
        materialMaps;
    for (auto& [surface, accumulatedMaterial] : cState->accumulatedMaterial) {
      materialMaps[surface->geometryId()] =
          std::make_shared<HomogeneousSurfaceMaterial>(
              accumulatedMaterial.totalAverage().first);
    }
    return materialMaps;
  }

 private:
  std::vector<std::shared_ptr<Surface>> m_surfaces;
};

BOOST_AUTO_TEST_SUITE(MaterialMapperTestSuite)

/// @brief This test checks the data flow of the material mapper, it is not
/// a test of the single components, which are tested individually
///
/// @note Currently only surface mapping is implemented, volume mapping/assigning
/// is being added in future PRs
BOOST_AUTO_TEST_CASE(MaterialMapperFlowTest) {
  // Create a vector of surfaces
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 50.0,
                                           100.0)};

  for (auto [is, surface] : enumerate(surfaces)) {
    surface->assignGeometryId(GeometryIdentifier().setSensitive(is + 1));
  }

  // The assigner
  auto assigner = std::make_shared<IntersectSurfacesFinder>();
  assigner->surfaces = {surfaces[0].get(), surfaces[1].get(),
                        surfaces[2].get()};

  // The accumulater - which blends all the material
  auto accumulator = std::make_shared<MaterialBlender>(surfaces);

  // Create the mapper
  MaterialMapper::Config mmConfig;
  mmConfig.assignmentFinder = assigner;
  mmConfig.surfaceMaterialAccumulater = accumulator;

  MaterialMapper mapper(mmConfig);

  auto state = mapper.createState();
  BOOST_CHECK(state.get() != nullptr);
  BOOST_CHECK(state->surfaceMaterialAccumulaterState.get() != nullptr);

  std::vector<RecordedMaterialTrack> mappedTracks;
  std::vector<RecordedMaterialTrack> unmappedTracks;

  // Track loop
  Vector3 position(0., 0., 0.);
  for (unsigned int it = 0; it < 11; ++it) {
    Vector3 direction =
        Vector3(0.9 + it * 0.02, 1.1 - it * 0.02, 0.).normalized();
    RecordedMaterialTrack mTrack{{position, direction}, {}};
    for (unsigned int im = 0; im < 60; ++im) {
      MaterialInteraction mi;
      mi.materialSlab = MaterialSlab(
          Material::fromMassDensity(it + 1, it + 1, it + 1, it + 1, it + 1),
          0.1);
      mi.position = position + (im + 1) * direction;
      mi.direction = direction;
      mTrack.second.materialInteractions.push_back(mi);
    }
    auto [mapped, unmapped] = mapper.mapMaterial(*state, tContext, {}, mTrack);
    mappedTracks.push_back(mapped);
    unmappedTracks.push_back(unmapped);
  }

  // Get the maps
  auto [surfaceMaps, volumeMaps] = mapper.finalizeMaps(*state);

  BOOST_CHECK(surfaceMaps.size() == 3);
  BOOST_CHECK(volumeMaps.empty());
}

BOOST_AUTO_TEST_CASE(MaterialMapperInvalidTest) {
  // The assigner
  auto assigner = std::make_shared<IntersectSurfacesFinder>();

  // The accumulater - which blends all the material
  auto accumulator = std::make_shared<MaterialBlender>();

  // Create the mapper
  MaterialMapper::Config mmConfigInvalid;
  mmConfigInvalid.assignmentFinder = nullptr;
  mmConfigInvalid.surfaceMaterialAccumulater = accumulator;

  BOOST_CHECK_THROW(auto mapperIA = MaterialMapper(mmConfigInvalid),
                    std::invalid_argument);

  // BOOST_CHECK_THROW(MaterialMapper(mmConfigInvalid), std::invalid_argument);

  mmConfigInvalid.assignmentFinder = assigner;
  mmConfigInvalid.surfaceMaterialAccumulater = nullptr;

  BOOST_CHECK_THROW(auto mapperIS = MaterialMapper(mmConfigInvalid),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test

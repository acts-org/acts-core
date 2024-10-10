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
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialInteractionAssignment.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Material/interface/IAssignmentFinder.hpp"
#include "Acts/Propagator/SurfaceCollector.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <limits>

namespace Acts::Test {

auto tContext = GeometryContext();

BOOST_AUTO_TEST_SUITE(MaterialInteractionAssignmentSuite)

BOOST_AUTO_TEST_CASE(AssignToClosest) {
  // Create a vector of surfaces
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 50.0,
                                           100.0)};

  for (auto [is, surface] : enumerate(surfaces)) {
    surface->assignGeometryId(GeometryIdentifier().setSensitive(is + 1));
  }

  std::vector<IAssignmentFinder::SurfaceAssignment> intersectedSurfaces = {
      {surfaces[0].get(), {20., 0., 0.}, {1., 0., 0.}},
      {surfaces[1].get(), {30., 0., 0.}, {1., 0., 0.}},
      {surfaces[2].get(), {50., 0., 0.}, {1., 0., 0.}}};

  // Create a material
  Material material = Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0);

  std::vector<MaterialInteraction> materialInteractions;
  materialInteractions.reserve(50);
  // Generate some material interactions
  for (unsigned int i = 1; i < 50; ++i) {
    MaterialInteraction materialInteraction;
    materialInteraction.materialSlab = MaterialSlab(material, 0.1);
    materialInteraction.position = Vector3{i * 1.0, 0, 0};
    materialInteraction.direction = Vector3{1.0, 0., 0.};
    materialInteractions.push_back(materialInteraction);
  }

  MaterialInteractionAssignment::Options options;

  // Assign the material interactions to the surface hits
  auto [assigned, unassigned, surfacesLeft] =
      MaterialInteractionAssignment::assign(tContext, materialInteractions,
                                            intersectedSurfaces, options);

  // Check that the material interaction was assigned
  BOOST_CHECK_EQUAL(assigned.size(), materialInteractions.size());
  BOOST_CHECK_EQUAL(unassigned.size(), 0u);
  BOOST_CHECK_EQUAL(surfacesLeft.size(), 0u);

  // Check that it is assigned to the closest surface always
  for (const auto& mi : assigned) {
    ActsScalar minDistance = std::numeric_limits<ActsScalar>::max();
    const Surface* closestSurface = nullptr;
    for (const auto& [surface, position, direction] : intersectedSurfaces) {
      ActsScalar distance = (mi.position - position).norm();
      if (distance < minDistance) {
        minDistance = distance;
        closestSurface = surface;
      }
    }
    BOOST_CHECK_EQUAL(mi.surface, closestSurface);
  }
}

BOOST_AUTO_TEST_CASE(AssignToClosest_withGlobalVeto) {
  // Create a vector of surfaces
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 50.0,
                                           100.0)};

  for (auto [is, surface] : enumerate(surfaces)) {
    surface->assignGeometryId(GeometryIdentifier().setSensitive(is + 1));
  }

  std::vector<IAssignmentFinder::SurfaceAssignment> intersectedSurfaces = {
      {surfaces[0].get(), {20., 0., 0.}, {1., 0., 0.}},
      {surfaces[1].get(), {30., 0., 0.}, {1., 0., 0.}},
      {surfaces[2].get(), {50., 0., 0.}, {1., 0., 0.}}};

  // Create a material
  Material material = Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0);

  std::vector<MaterialInteraction> materialInteractions;
  materialInteractions.reserve(50);
  // Generate some material interactions
  for (unsigned int i = 1; i < 50; ++i) {
    MaterialInteraction materialInteraction;
    materialInteraction.materialSlab = MaterialSlab(material, 0.1);
    materialInteraction.position = Vector3{i * 1.0, 0, 0};
    materialInteraction.direction = Vector3{1.0, 0., 0.};
    materialInteractions.push_back(materialInteraction);
  }

  // Veto everything above 40 mm
  struct RadialVeto {
    ActsScalar rMax = 40.0;
    bool operator()(const MaterialInteraction& mi) const {
      return VectorHelpers::perp(mi.position) > rMax;
    }
  };
  MaterialInteractionAssignment::Options options;
  options.globalVetos.push_back(RadialVeto{40});

  // Assign the material interactions to the surface hits
  auto [assigned, unassigned, surfacesLeft] =
      MaterialInteractionAssignment::assign(tContext, materialInteractions,
                                            intersectedSurfaces, options);

  // Check that the material interaction was assigned
  BOOST_CHECK_EQUAL(assigned.size(), 40u);
  BOOST_CHECK_EQUAL(unassigned.size(), 9u);
  BOOST_CHECK_EQUAL(surfacesLeft.size(), 1u);
}

BOOST_AUTO_TEST_CASE(AssignToClosest_withLocalVeto) {
  // Create a vector of surfaces
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 50.0,
                                           100.0)};

  for (auto [is, surface] : enumerate(surfaces)) {
    surface->assignGeometryId(GeometryIdentifier().setSensitive(is + 1));
  }

  std::vector<IAssignmentFinder::SurfaceAssignment> intersectedSurfaces = {
      {surfaces[0].get(), {20., 0., 0.}, {1., 0., 0.}},
      {surfaces[1].get(), {30., 0., 0.}, {1., 0., 0.}},
      {surfaces[2].get(), {50., 0., 0.}, {1., 0., 0.}}};

  // Create a material
  Material material = Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0);

  std::vector<MaterialInteraction> materialInteractions;
  materialInteractions.reserve(50);
  // Generate some material interactions
  for (unsigned int i = 1; i < 50; ++i) {
    MaterialInteraction materialInteraction;
    materialInteraction.materialSlab = MaterialSlab(material, 0.1);
    materialInteraction.position = Vector3{i * 1.0, 0, 0};
    materialInteraction.direction = Vector3{1.0, 0., 0.};
    materialInteractions.push_back(materialInteraction);
  }

  // Veto in a specific one
  struct VetoThisOne {
    bool operator()(
        const MaterialInteraction& /*m*/,
        const IAssignmentFinder::SurfaceAssignment& /*suggestedAssignment*/)
        const {
      return true;
    }
  };

  // We assign this to
  std::vector<
      std::pair<GeometryIdentifier, MaterialInteractionAssignment::LocalVeto>>
      localVetoVector = {{GeometryIdentifier().setSensitive(2), VetoThisOne{}}};
  GeometryHierarchyMap<MaterialInteractionAssignment::LocalVeto> localVetos(
      localVetoVector);
  MaterialInteractionAssignment::Options options;
  options.localVetos = localVetos;

  // Assign the material interactions to the surface hits
  auto [assigned, unassigned, surfacesLeft] =
      MaterialInteractionAssignment::assign(tContext, materialInteractions,
                                            intersectedSurfaces, options);

  // Check that the material interaction was assigned
  BOOST_CHECK_EQUAL(assigned.size(), 34u);
  BOOST_CHECK_EQUAL(unassigned.size(), 15u);
  BOOST_CHECK_EQUAL(surfacesLeft.size(), 1u);
}

BOOST_AUTO_TEST_CASE(AssignToClosest_withReassignment) {
  // Create a vector of surfaces
  std::vector<std::shared_ptr<Surface>> surfaces = {
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 30.0, 100.0),
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 50.0,
                                           100.0)};

  for (auto [is, surface] : enumerate(surfaces)) {
    surface->assignGeometryId(GeometryIdentifier().setSensitive(is + 1));
  }

  std::vector<IAssignmentFinder::SurfaceAssignment> intersectedSurfaces = {
      {surfaces[0].get(), {20., 0., 0.}, {1., 0., 0.}},
      {surfaces[1].get(), {30., 0., 0.}, {1., 0., 0.}},
      {surfaces[2].get(), {50., 0., 0.}, {1., 0., 0.}}};

  // Create a material
  Material material = Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0);

  std::vector<MaterialInteraction> materialInteractions;
  materialInteractions.reserve(50);
  // Generate some material interactions
  for (unsigned int i = 1; i < 50; ++i) {
    MaterialInteraction materialInteraction;
    materialInteraction.materialSlab = MaterialSlab(material, 0.1);
    materialInteraction.position = Vector3{i * 1.0, 0, 0};
    materialInteraction.direction = Vector3{1.0, 0., 0.};
    materialInteractions.push_back(materialInteraction);
  }

  // Veto in a specific one
  struct ReAssignToNeighbor {
    void operator()(
        MaterialInteraction& m,
        const IAssignmentFinder::SurfaceAssignment& /*suggestedAssignment*/,
        const IAssignmentFinder::SurfaceAssignment& n) const {
      auto [surface, position, direction] = n;
      m.surface = surface;
      m.position = position;
      m.direction = direction;
      m.intersectionID = surface->geometryId();
      return;
    }
  };

  // We assign this to
  std::vector<std::pair<GeometryIdentifier,
                        MaterialInteractionAssignment::ReAssignment>>
      reassignmentVector = {
          {GeometryIdentifier().setSensitive(2), ReAssignToNeighbor{}}};
  GeometryHierarchyMap<MaterialInteractionAssignment::ReAssignment>
      reassignments(reassignmentVector);
  MaterialInteractionAssignment::Options options;
  options.reAssignments = reassignments;

  // Assign the material interactions to the surface hits
  auto [assigned, unassigned, surfaceLeft] =
      MaterialInteractionAssignment::assign(tContext, materialInteractions,
                                            intersectedSurfaces, options);

  // Check that the material interaction was assigned
  BOOST_CHECK_EQUAL(assigned.size(), 49u);
  BOOST_CHECK_EQUAL(unassigned.size(), 0u);
  BOOST_CHECK_EQUAL(surfaceLeft.size(), 1u);

  // Check that the geoid with number 2 never shows up
  for (const auto& mi : assigned) {
    BOOST_CHECK_NE(mi.intersectionID, GeometryIdentifier().setSensitive(2));
  }
}

BOOST_AUTO_TEST_CASE(AssignWithPathLength) {
  auto surface =
      Surface::makeShared<CylinderSurface>(Transform3::Identity(), 20.0, 100.0);
  surface->assignGeometryId(GeometryIdentifier().setSensitive(1));

  Vector3 position = {20., 10., 0.};
  Vector3 direction = position.normalized();

  IAssignmentFinder::SurfaceAssignment surfaceHit{surface.get(), position,
                                                  direction};

  Material material = Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0);

  MaterialInteraction materialInteraction;
  materialInteraction.materialSlab = MaterialSlab(material, 0.1);
  materialInteraction.position = position + 0.5 * direction;
  materialInteraction.direction = direction;

  MaterialInteractionAssignment::Options options;

  auto [assigned, unassigned, surfaceLeft] =
      MaterialInteractionAssignment::assign(tContext, {materialInteraction},
                                            {surfaceHit}, options);

  // Check that the material interaction was assigned
  BOOST_CHECK_EQUAL(assigned.size(), 1u);
  BOOST_CHECK_EQUAL(unassigned.size(), 0u);
  BOOST_CHECK_EQUAL(surfaceLeft.size(), 0u);

  // Check that the path correction is set
  BOOST_CHECK_NE(assigned[0].pathCorrection, 0.);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test

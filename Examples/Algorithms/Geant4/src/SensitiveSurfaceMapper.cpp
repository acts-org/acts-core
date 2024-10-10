// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/SensitiveSurfaceMapper.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Visualization/GeometryView3D.hpp"
#include "Acts/Visualization/ObjVisualization3D.hpp"

#include <algorithm>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4Polyhedron.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VSolid.hh>
#include <boost/container/flat_set.hpp>
#include <boost/geometry.hpp>

// Add some type traits for boost::geometry, so we can use the machinery
// directly with Acts::Vector2 / Eigen::Matrix
namespace boost::geometry::traits {

template <typename T, int D>
struct tag<Eigen::Matrix<T, D, 1>> {
  using type = point_tag;
};
template <typename T, int D>
struct dimension<Eigen::Matrix<T, D, 1>> : std::integral_constant<int, D> {};
template <typename T, int D>
struct coordinate_type<Eigen::Matrix<T, D, 1>> {
  using type = T;
};
template <typename T, int D>
struct coordinate_system<Eigen::Matrix<T, D, 1>> {
  using type = boost::geometry::cs::cartesian;
};

template <typename T, int D, std::size_t Index>
struct access<Eigen::Matrix<T, D, 1>, Index> {
  static_assert(Index < D, "Out of range");
  using Point = Eigen::Matrix<T, D, 1>;
  using CoordinateType = typename coordinate_type<Point>::type;
  static inline CoordinateType get(Point const& p) { return p[Index]; }
  static inline void set(Point& p, CoordinateType const& value) {
    p[Index] = value;
  }
};

}  // namespace boost::geometry::traits

namespace {
void writeG4Polyhedron(
    Acts::IVisualization3D& visualizer, const G4Polyhedron& polyhedron,
    const Acts::Transform3& trafo = Acts::Transform3::Identity(),
    Acts::Color color = {0, 0, 0}) {
  constexpr double convertLength = CLHEP::mm / Acts::UnitConstants::mm;

  for (int i = 1; i <= polyhedron.GetNoFacets(); ++i) {
    // This is a bit ugly but I didn't find an easy way to compute this
    // beforehand.
    constexpr std::size_t maxPoints = 1000;
    G4Point3D points[maxPoints];
    int nPoints = 0;
    polyhedron.GetFacet(i, nPoints, points);
    assert(static_cast<std::size_t>(nPoints) < maxPoints);

    std::vector<Acts::Vector3> faces;
    for (int j = 0; j < nPoints; ++j) {
      faces.emplace_back(points[j][0] * convertLength,
                         points[j][1] * convertLength,
                         points[j][2] * convertLength);
      faces.back() = trafo * faces.back();
    }

    visualizer.face(faces, color);
  }
}
}  // namespace

std::vector<const Acts::Surface*>
ActsExamples::SensitiveCandidates::queryPosition(
    const Acts::GeometryContext& gctx, const Acts::Vector3& position) const {
  std::vector<const Acts::Surface*> surfaces;

  if (trackingGeometry == nullptr) {
    return surfaces;
  }

  // In case we do not find a layer at this position for whatever reason
  const auto layer = trackingGeometry->associatedLayer(gctx, position);
  if (layer == nullptr) {
    return surfaces;
  }

  const auto surfaceArray = layer->surfaceArray();
  if (surfaceArray == nullptr) {
    return surfaces;
  }

  for (const auto& surface : surfaceArray->surfaces()) {
    if (surface->associatedDetectorElement() != nullptr) {
      surfaces.push_back(surface);
    }
  }
  return surfaces;
}

std::vector<const Acts::Surface*> ActsExamples::SensitiveCandidates::queryAll()
    const {
  std::vector<const Acts::Surface*> surfaces;

  const bool restrictToSensitives = true;
  trackingGeometry->visitSurfaces(
      [&](auto surface) { surfaces.push_back(surface); }, restrictToSensitives);

  return surfaces;
}

ActsExamples::SensitiveSurfaceMapper::SensitiveSurfaceMapper(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {}

void ActsExamples::SensitiveSurfaceMapper::remapSensitiveNames(
    State& state, const Acts::GeometryContext& gctx,
    G4VPhysicalVolume* g4PhysicalVolume,
    const Acts::Transform3& motherTransform) const {
  // Make sure the unit conversion is correct
  constexpr double convertLength = CLHEP::mm / Acts::UnitConstants::mm;

  auto g4ToActsVector = [](const G4ThreeVector& g4vec) {
    return Acts::Vector3(g4vec[0] * convertLength, g4vec[1] * convertLength,
                         g4vec[2] * convertLength);
  };

  auto actsToG4Vec = [](const Acts::Vector3& actsVec) {
    return G4ThreeVector(actsVec[0] / convertLength, actsVec[1] / convertLength,
                         actsVec[2] / convertLength);
  };

  auto g4LogicalVolume = g4PhysicalVolume->GetLogicalVolume();
  auto g4SensitiveDetector = g4LogicalVolume->GetSensitiveDetector();

  // Get the transform of the G4 object
  Acts::Transform3 localG4ToGlobal;
  {
    auto g4Translation = g4PhysicalVolume->GetTranslation();
    auto g4Rotation = g4PhysicalVolume->GetRotation();
    Acts::Vector3 g4RelPosition = g4ToActsVector(g4Translation);
    Acts::Translation3 translation(g4RelPosition);
    if (g4Rotation == nullptr) {
      localG4ToGlobal = motherTransform * translation;
    } else {
      Acts::RotationMatrix3 rotation;
      rotation << g4Rotation->xx(), g4Rotation->yx(), g4Rotation->zx(),
          g4Rotation->xy(), g4Rotation->yy(), g4Rotation->zy(),
          g4Rotation->xz(), g4Rotation->yz(), g4Rotation->zz();
      localG4ToGlobal = motherTransform * (translation * rotation);
    }
  }

  Acts::Vector3 g4AbsPosition = localG4ToGlobal * Acts::Vector3::Zero();

  if (G4int nDaughters = g4LogicalVolume->GetNoDaughters(); nDaughters > 0) {
    // Step down to all daughters
    for (G4int id = 0; id < nDaughters; ++id) {
      remapSensitiveNames(state, gctx, g4LogicalVolume->GetDaughter(id),
                          localG4ToGlobal);
    }
    return;
  }

  std::string volumeName = g4LogicalVolume->GetName();
  std::string volumeMaterialName = g4LogicalVolume->GetMaterial()->GetName();

  const bool isSensitive = g4SensitiveDetector != nullptr;
  const bool isMappedMaterial =
      Acts::rangeContainsValue(m_cfg.materialMappings, volumeMaterialName);
  const bool isMappedVolume =
      Acts::rangeContainsValue(m_cfg.volumeMappings, volumeName);

  if (!(isSensitive || isMappedMaterial || isMappedVolume)) {
    ACTS_VERBOSE("Did not try mapping '"
                 << g4PhysicalVolume->GetName() << "' at "
                 << g4AbsPosition.transpose()
                 << " because g4SensitiveDetector (=" << g4SensitiveDetector
                 << ") is null and volume name (=" << volumeName
                 << ") and material name (=" << volumeMaterialName
                 << ") were not found");
    return;
  }

  // Prepare the mapped surface
  const Acts::Surface* mappedSurface = nullptr;

  std::vector<const Acts::Surface*> candidateSurfaces;
  const auto g4Polyhedron = g4LogicalVolume->GetSolid()->GetPolyhedron();
  for (int i = 1; i < g4Polyhedron->GetNoVertices(); ++i) {
    auto vtx = g4ToActsVector(g4Polyhedron->GetVertex(i));
    auto vtxGlobal = localG4ToGlobal * vtx;

    candidateSurfaces = m_cfg.candidateSurfaces->queryPosition(gctx, vtxGlobal);

    if (!candidateSurfaces.empty()) {
      break;
    }
  }

  // Fall back to query all surfaces
  if (candidateSurfaces.empty()) {
    ACTS_DEBUG("No candidate surfaces for volume '" << volumeName << "' at "
                                                    << g4AbsPosition.transpose()
                                                    << ", query all surfaces");
    candidateSurfaces = m_cfg.candidateSurfaces->queryAll();
  }

  ACTS_VERBOSE("Found " << candidateSurfaces.size()
                        << " candidate surfaces for " << volumeName);

  for (const auto& candidateSurface : candidateSurfaces) {
    if (candidateSurface->center(gctx).isApprox(g4AbsPosition)) {
      ACTS_VERBOSE("Successful match with center matching");
      mappedSurface = candidateSurface;
      break;
    } else if (candidateSurface->bounds().type() ==
               Acts::SurfaceBounds::eAnnulus) {
      const auto& bounds =
          *static_cast<const Acts::AnnulusBounds*>(&candidateSurface->bounds());

      const auto vertices = bounds.vertices(0);

      constexpr bool clockwise = false;
      constexpr bool closed = false;
      using Polygon =
          boost::geometry::model::polygon<Acts::Vector2, clockwise, closed>;

      Polygon poly;
      boost::geometry::assign_points(poly, vertices);

      Acts::Vector2 boundsCentroidSurfaceFrame = Acts::Vector2::Zero();
      boost::geometry::centroid(poly, boundsCentroidSurfaceFrame);

      Acts::Vector3 boundsCentroidGlobal{boundsCentroidSurfaceFrame[0],
                                         boundsCentroidSurfaceFrame[1], 0.0};
      boundsCentroidGlobal =
          candidateSurface->transform(gctx) * boundsCentroidGlobal;

      const auto boundsCentroidG4Frame =
          localG4ToGlobal.inverse() * boundsCentroidGlobal;

      if (g4LogicalVolume->GetSolid()->Inside(
              actsToG4Vec(boundsCentroidG4Frame)) != EInside::kOutside) {
        ACTS_VERBOSE("Successful match with centroid matching");
        mappedSurface = candidateSurface;
        break;
      }
    }
  }

  if (mappedSurface == nullptr) {
    ACTS_DEBUG("No mapping found for '"
               << volumeName << "' with material '" << volumeMaterialName
               << "' at position " << g4AbsPosition.transpose());
    state.missingVolumes.emplace_back(g4PhysicalVolume, localG4ToGlobal);
    return;
  }

  // A mapped surface was found, a new name will be set that G4PhysVolume
  ACTS_DEBUG("Matched " << volumeName << " to " << mappedSurface->geometryId()
                        << " at position " << g4AbsPosition.transpose());
  // Check if the prefix is not yet assigned
  if (volumeName.find(mappingPrefix) == std::string::npos) {
    // Set the new name
    std::string mappedName = std::string(mappingPrefix) + volumeName;
    g4PhysicalVolume->SetName(mappedName);
  }
  // Insert into the multi-map
  state.g4VolumeToSurfaces.insert({g4PhysicalVolume, mappedSurface});
}

bool ActsExamples::SensitiveSurfaceMapper::checkMapping(
    const State& state, const Acts::GeometryContext& gctx,
    bool writeMissingG4VolsAsObj, bool writeMissingSurfacesAsObj) const {
  auto allSurfaces = m_cfg.candidateSurfaces->queryAll();
  std::ranges::sort(allSurfaces);

  std::vector<const Acts::Surface*> found;
  for (const auto [_, surfacePtr] : state.g4VolumeToSurfaces) {
    found.push_back(surfacePtr);
  }
  std::ranges::sort(found);
  auto newEnd = std::unique(found.begin(), found.end());
  found.erase(newEnd, found.end());

  std::vector<const Acts::Surface*> missing;
  std::set_difference(allSurfaces.begin(), allSurfaces.end(), found.begin(),
                      found.end(), std::back_inserter(missing));

  ACTS_INFO("Number of overall sensitive surfaces: " << allSurfaces.size());
  ACTS_INFO("Number of mapped volume->surface mappings: "
            << state.g4VolumeToSurfaces.size());
  ACTS_INFO(
      "Number of sensitive surfaces that are not mapped: " << missing.size());
  ACTS_INFO("Number of G4 volumes without a matching Surface: "
            << state.missingVolumes.size());

  if (writeMissingG4VolsAsObj) {
    Acts::ObjVisualization3D visualizer;
    for (const auto& [g4vol, trafo] : state.missingVolumes) {
      auto polyhedron = g4vol->GetLogicalVolume()->GetSolid()->GetPolyhedron();
      writeG4Polyhedron(visualizer, *polyhedron, trafo);
    }

    std::ofstream os("missing_g4_volumes.obj");
    visualizer.write(os);
  }

  if (writeMissingSurfacesAsObj) {
    Acts::ObjVisualization3D visualizer;
    Acts::ViewConfig vcfg;
    vcfg.quarterSegments = 720;
    for (auto srf : missing) {
      Acts::GeometryView3D::drawSurface(visualizer, *srf, gctx,
                                        Acts::Transform3::Identity(), vcfg);
    }

    std::ofstream os("missing_acts_surfaces.obj");
    visualizer.write(os);
  }

  return missing.empty();
}

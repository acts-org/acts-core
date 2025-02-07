// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/DetectorElementBase.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>

class G4VPhysicalVolume;

namespace Acts {

class ISurfaceMaterial;
class Surface;

/// @class Geant4DetectorElement
///
/// Detector element representative for Geant4 sensitive
/// elements.
class Geant4DetectorElement : public DetectorElementBase {
 public:
  /// Broadcast the context type
  using ContextType = GeometryContext;

  /// @brief  Constructor with arguments
  /// @param surface the surface representing this detector element
  /// @param g4physVol the physical volume representing this detector element
  /// @param toGlobal the global transformation before the volume
  /// @param thickness the thickness of this detector element
  Geant4DetectorElement(std::shared_ptr<Surface> surface,
                        const G4VPhysicalVolume& g4physVol,
                        const Transform3& toGlobal, double thickness);

  /// Return local to global transform associated with this detector element
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  const Transform3& transform(const GeometryContext& gctx) const override;

  /// Return surface associated with this detector element
  const Surface& surface() const override;

  /// Non-const access to surface associated with this detector element
  Surface& surface() override;

  /// Return the thickness of this detector element
  double thickness() const override;

  /// @return to the Geant4 physical volume
  const G4VPhysicalVolume& g4PhysicalVolume() const;

 private:
  /// Corresponding Surface
  std::shared_ptr<Surface> m_surface;
  /// The GEant4 physical volume
  const G4VPhysicalVolume* m_g4physVol{nullptr};
  /// The global transformation before the volume
  Transform3 m_toGlobal;
  ///  Thickness of this detector element
  double m_thickness{0.};
};

}  // namespace Acts

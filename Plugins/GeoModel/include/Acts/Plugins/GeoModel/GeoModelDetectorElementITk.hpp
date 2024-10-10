// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Utilities/MultiIndex.hpp"

namespace Acts {

class ITkIdentifier {
  Acts::MultiIndex<std::size_t, 1, 2, 20, 1, 19, 20, 1> m_identifier{};

 public:
  ITkIdentifier(int hardware, int barrelEndcap, int layerWheel, int etaModule,
                int phiModule, int side);

  /// Access the hardware specifier (pixel=0, strip=1)
  int hardware() const;

  /// Access the barrel-endcap specifier (-2,0,2)
  int barrelEndcap() const;

  /// Access the layer specifier
  int layerWheel() const;

  /// Access the phi module specifier
  int phiModule() const;

  /// Access the eta module specifier
  int etaModule() const;

  /// Access the side (for double sided strip modules)
  int side() const;

  /// A unique identifier that represents the combination of specifiers
  std::size_t value() const;
};

std::ostream& operator<<(std::ostream& os, const ITkIdentifier& id);

/// Specialization of the GeoModelDetectorElement for the ITk. This allows
/// mapping of Acts::GeometryIdentifiers to ITk modules in a straight-forward
/// way.
class GeoModelDetectorElementITk : public GeoModelDetectorElement {
 public:
  GeoModelDetectorElementITk(const PVConstLink& geoPhysVol,
                             std::shared_ptr<Surface> surface,
                             const Transform3& sfTransform,
                             ActsScalar thickness, int hardware,
                             int barrelEndcap, int layerWheel, int etaModule,
                             int phiModule, int side)
      : GeoModelDetectorElement(geoPhysVol, std::move(surface), sfTransform,
                                thickness),
        m_identifier(hardware, barrelEndcap, layerWheel, etaModule, phiModule,
                     side) {}

  ITkIdentifier identifier() const { return m_identifier; }

  /// Convert a GeoModelDetectorElement to a GeoModelDetectorElementITk
  /// A new surface is constructed.
  /// @todo Remove redundancy in signature once plugin is refactored
  static std::tuple<std::shared_ptr<GeoModelDetectorElementITk>,
                    std::shared_ptr<Surface>>
  convertFromGeomodel(std::shared_ptr<GeoModelDetectorElement> detEl,
                      std::shared_ptr<Surface> srf, const GeometryContext& gctx,
                      int hardware, int barrelEndcap, int layerWheel,
                      int etaModule, int phiModule, int side);

 private:
  ITkIdentifier m_identifier;
};

}  // namespace Acts

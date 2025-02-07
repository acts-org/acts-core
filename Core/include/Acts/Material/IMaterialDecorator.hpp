// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

namespace Acts {

class Surface;
class TrackingVolume;

/// @class IMaterialDecorator
///
/// Virtual base class for decorators that allow to load
/// material onto a TrackingGeometry. The geometry allows material
/// to be assigned either to surfaces or to volumes, hence there are
/// two decorate interface methods.
///
class IMaterialDecorator {
 public:
  /// Virtual Destructor
  virtual ~IMaterialDecorator() = default;

  /// Decorate a surface
  ///
  /// @param surface the non-cost surface that is decorated
  virtual void decorate(Surface& surface) const = 0;

  /// Decorate a TrackingVolume
  ///
  /// @param volume the non-cost volume that is decorated
  virtual void decorate(TrackingVolume& volume) const = 0;
};

}  // namespace Acts

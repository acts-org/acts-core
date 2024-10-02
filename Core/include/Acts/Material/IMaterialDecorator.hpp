// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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

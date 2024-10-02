// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/Layer.hpp"

#include "../Surfaces/SurfaceStub.hpp"

namespace Acts {
/// Layer derived class stub
/// Note: Layer classes in general have a static 'create' factory method, but
/// nothing
/// in the baseclasses mandates this.
class LayerStub : virtual public SurfaceStub, public Layer {
 public:
  /// constructor (deleted in Surface baseclass)
  LayerStub() = delete;
  /// copy constructor (deleted in Surface baseclass)
  LayerStub(const LayerStub& otherLayer) = delete;
  /// constructor with pointer to SurfaceArray (protected in Layer baseclass)
  LayerStub(std::unique_ptr<SurfaceArray> surfaceArray, double thickness = 0,
            std::unique_ptr<ApproachDescriptor> ad = nullptr,
            LayerType ltype = passive)
      : SurfaceStub(),
        Layer(std::move(surfaceArray), thickness, std::move(ad), ltype) {}

  /// Destructor
  ~LayerStub() override = default;

  /// Assignment is deleted in the Layer baseclass
  LayerStub& operator=(const LayerStub& lay) = delete;

  /// surfaceRepresentation is pure virtual in baseclass
  const Surface& surfaceRepresentation() const override { return (*this); }

  Surface& surfaceRepresentation() override { return (*this); }

  /// simply return true to show a method can be called on the constructed
  /// object
  bool constructedOk() const { return true; }

  /// Other methods have implementation in baseclass
  /// templated 'onLayer()' from baseclass ?
};
}  // namespace Acts

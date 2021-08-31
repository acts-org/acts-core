// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "ActsExamples/GenericDetector/GenericDetectorElement.hpp"

#include <iostream>
#include <map>
#include <memory>

namespace ActsExamples {

namespace Contextual {

class AlignedDetectorElement : public Generic::GenericDetectorElement {
 public:
  // Inherit constructor
  using Generic::GenericDetectorElement::GenericDetectorElement;

  /// Return local to global transform associated with this identifier
  ///
  /// @param alignedTransform is a new transform
  /// @oaram iov is the batch for which it is meant
  virtual void addAlignedTransform(const Acts::Transform3& alignedTransform,
                                   Acts::GeometryContext& context) = 0;

  virtual Acts::GeometryContext makeContext(unsigned int iov) const = 0;
};

}  // end of namespace Contextual
}  // end of namespace ActsExamples

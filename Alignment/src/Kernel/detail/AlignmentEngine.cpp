// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsAlignment/Kernel/detail/AlignmentEngine.hpp"

namespace ActsAlignment::detail {

void resetAlignmentDerivative(Acts::AlignmentToBoundMatrix& alignToBound,
                              AlignmentMask mask) {
  if (!ACTS_CHECK_BIT(mask, AlignmentMask::Center0)) {
    alignToBound.col(Acts::eAlignmentCenter0) = Acts::AlignmentVector::Zero();
  }
  if (!ACTS_CHECK_BIT(mask, AlignmentMask::Center1)) {
    alignToBound.col(Acts::eAlignmentCenter1) = Acts::AlignmentVector::Zero();
  }
  if (!ACTS_CHECK_BIT(mask, AlignmentMask::Center2)) {
    alignToBound.col(Acts::eAlignmentCenter2) = Acts::AlignmentVector::Zero();
  }
  if (!ACTS_CHECK_BIT(mask, AlignmentMask::Rotation0)) {
    alignToBound.col(Acts::eAlignmentRotation0) = Acts::AlignmentVector::Zero();
  }
  if (!ACTS_CHECK_BIT(mask, AlignmentMask::Rotation1)) {
    alignToBound.col(Acts::eAlignmentRotation1) = Acts::AlignmentVector::Zero();
  }
  if (!ACTS_CHECK_BIT(mask, AlignmentMask::Rotation2)) {
    alignToBound.col(Acts::eAlignmentRotation2) = Acts::AlignmentVector::Zero();
  }
}

}  // namespace ActsAlignment::detail

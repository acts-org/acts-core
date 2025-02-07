// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

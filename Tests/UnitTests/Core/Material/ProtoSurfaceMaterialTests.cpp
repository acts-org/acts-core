// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <utility>

namespace Acts::Test {

/// Test the constructors
BOOST_AUTO_TEST_CASE(ProtoSurfaceMaterial_construction_test) {
  BinUtility smpBU(10, -10., 10., open, AxisDirection::AxisX);
  smpBU += BinUtility(10, -10., 10., open, AxisDirection::AxisY);

  // Constructor from arguments
  ProtoSurfaceMaterial smp(smpBU);
  // Copy constructor
  ProtoSurfaceMaterial smpCopy(smp);
  // Copy move constructor
  ProtoSurfaceMaterial smpCopyMoved(std::move(smpCopy));
}

}  // namespace Acts::Test

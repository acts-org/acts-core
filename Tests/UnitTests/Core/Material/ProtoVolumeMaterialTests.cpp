// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Material/ProtoVolumeMaterial.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <utility>

namespace Acts::Test {

/// Test the constructors
BOOST_AUTO_TEST_CASE(ProtoVolumeMaterial_construction_test) {
  BinUtility vmpBU(10, -10., 10., open, AxisDirection::AxisX);
  vmpBU += BinUtility(10, -10., 10., open, AxisDirection::AxisY);
  vmpBU += BinUtility(10, -10., 10., open, AxisDirection::AxisZ);

  // Constructor from arguments
  ProtoVolumeMaterial vmp(vmpBU);
  // Copy constructor
  ProtoVolumeMaterial vmpCopy(vmp);
  // Copy move constructor
  ProtoVolumeMaterial vmpCopyMoved(std::move(vmpCopy));
}

}  // namespace Acts::Test

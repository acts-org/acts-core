// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <iostream>

namespace Acts {

/// Check whether the BinningData objects are equal
///
/// @param ba The first BinningData object
/// @param bb The second BinningData object
/// @param tolerance a tolerance parameter
///
/// @return a boolean
inline static bool isEqual(const BinningData& ba, const BinningData& bb,
                           float tolerance) {
  bool equalBool = (ba.type == bb.type) && (ba.option == bb.option) &&
                   (ba.binvalue == bb.binvalue) && (ba.zdim == bb.zdim) &&
                   (ba.subBinningAdditive == bb.subBinningAdditive);

  BOOST_CHECK(equalBool);
  bool equalRange = (std::abs(ba.min - bb.min) < tolerance) &&
                    (std::abs(ba.max - bb.max) < tolerance) &&
                    (std::abs(ba.step - bb.step) < tolerance);

  BOOST_CHECK(equalRange);
  bool euqalStructure =
      (ba.subBinningData != nullptr)
          ? isEqual(*ba.subBinningData, *bb.subBinningData, tolerance)
          : (bb.subBinningData == nullptr);

  BOOST_CHECK(euqalStructure);

  bool equalBoundaries = (ba.boundaries().size() == bb.boundaries().size());
  if (equalBoundaries) {
    for (std::size_t ib = 0; ib < ba.boundaries().size(); ++ib) {
      equalBoundaries =
          (std::abs(ba.boundaries()[ib] - bb.boundaries()[ib]) < tolerance);
      if (!equalBoundaries) {
        break;
      }
    }
  }
  BOOST_CHECK(equalBoundaries);

  return equalBool && equalRange && euqalStructure;
}

/// Check whether the BinUtility objects are equal
///
/// @param ba The first BinUtility object
/// @param bb the second BinUtility object
/// @param tolerance a tolerance parameter
///
/// @return a bollean if equal
inline static bool isEqual(const BinUtility& ba, const BinUtility& bb,
                           float tolerance) {
  bool equal = (ba.binningData().size() == bb.binningData().size());
  BOOST_CHECK(equal);
  if (equal) {
    for (std::size_t ib = 0; ib < ba.binningData().size(); ++ib) {
      equal = isEqual(ba.binningData()[ib], bb.binningData()[ib], tolerance);
      BOOST_CHECK(equal);
    }
  }
  return equal;
}

/// check whether Extnet objects are equal - with tolerance
///
/// @param ea the first extent object
/// @param eb the second extent object
/// @param tolerance the tolerance parameter
///
/// @return bool for euqal
inline static bool isEqual(const Acts::Extent& ea, const Acts::Extent& eb,
                           Acts::ActsScalar tolerance = 0.) {
  bool equalConstrains = true;
  bool equalRange = true;
  for (auto& bVal : allBinningValues()) {
    equalConstrains =
        equalConstrains && (ea.constrains(bVal) == eb.constrains(bVal));
    BOOST_CHECK(equalConstrains);
    if (ea.constrains(bVal) && eb.constrains(bVal)) {
      equalRange =
          equalRange && std::abs(ea.min(bVal) - eb.min(bVal)) < tolerance;
      equalRange =
          equalRange && std::abs(ea.max(bVal) - eb.max(bVal)) < tolerance;
      BOOST_CHECK(equalRange);
    }
  }
  BOOST_CHECK(equalConstrains);
  BOOST_CHECK(equalRange);
  return equalRange && equalConstrains;
}

}  // namespace Acts

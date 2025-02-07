// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MultiRangeBField.hpp"
#include "Acts/Utilities/Result.hpp"

namespace Acts::Test {

MagneticFieldContext mfContext = MagneticFieldContext();

BOOST_AUTO_TEST_CASE(TestMultiRangeBField) {
  std::vector<std::pair<RangeXD<3, double>, Vector3>> inputs;

  inputs.emplace_back(RangeXD<3, double>{{0., 0., 0.}, {3., 3., 3.}},
                      Vector3{0., 0., 2.});
  inputs.emplace_back(RangeXD<3, double>{{1., 1., 1.}, {2., 2., 10.}},
                      Vector3{2., 0., 0.});

  const MultiRangeBField bfield(std::move(inputs));

  auto bcache = bfield.makeCache(mfContext);

  // Test a point inside the first volume.
  {
    Result<Vector3> r = bfield.getField({0.5, 0.5, 0.5}, bcache);
    BOOST_CHECK(r.ok());
    BOOST_CHECK_CLOSE((*r)[0], 0., 0.05);
    BOOST_CHECK_CLOSE((*r)[1], 0., 0.05);
    BOOST_CHECK_CLOSE((*r)[2], 2., 0.05);
  }

  // Test a point inside the second volume, not overlapping the first.
  {
    Result<Vector3> r = bfield.getField({1.5, 1.5, 5.}, bcache);
    BOOST_CHECK(r.ok());
    BOOST_CHECK_CLOSE((*r)[0], 2., 0.05);
    BOOST_CHECK_CLOSE((*r)[1], 0., 0.05);
    BOOST_CHECK_CLOSE((*r)[2], 0., 0.05);
  }

  // Test a point inside the first volume again.
  {
    Result<Vector3> r = bfield.getField({2.5, 2.5, 2.5}, bcache);
    BOOST_CHECK(r.ok());
    BOOST_CHECK_CLOSE((*r)[0], 0., 0.05);
    BOOST_CHECK_CLOSE((*r)[1], 0., 0.05);
    BOOST_CHECK_CLOSE((*r)[2], 2., 0.05);
  }

  // Test a point inside the second volume in the overlap region.
  {
    Result<Vector3> r = bfield.getField({1.5, 1.5, 1.5}, bcache);
    BOOST_CHECK(r.ok());
    BOOST_CHECK_CLOSE((*r)[0], 2., 0.05);
    BOOST_CHECK_CLOSE((*r)[1], 0., 0.05);
    BOOST_CHECK_CLOSE((*r)[2], 0., 0.05);
  }

  // Test a point outside all volumes.
  {
    Result<Vector3> r = bfield.getField({-1., -1., -1}, bcache);
    BOOST_CHECK(!r.ok());
  }
}
}  // namespace Acts::Test

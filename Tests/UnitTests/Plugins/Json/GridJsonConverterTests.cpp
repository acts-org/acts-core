// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/Json/GridJsonConverter.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/GridAccessHelpers.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <array>
#include <fstream>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>

BOOST_AUTO_TEST_SUITE(GridJsonConversion)

BOOST_AUTO_TEST_CASE(Grid1DSingleEntry) {
  // Bound equidistant
  using EqBound = Acts::GridAxisGenerators::EqBound;

  EqBound eqBound{{0., 5.}, 5};
  // Create the grid with the provided axis generator
  using GridTypeEQB = typename EqBound::template grid_type<std::size_t>;
  GridTypeEQB eqBoundGrid(eqBound());

  eqBoundGrid.at(1u) = 1u;
  eqBoundGrid.at(2u) = 2u;
  eqBoundGrid.at(3u) = 3u;
  eqBoundGrid.at(4u) = 4u;
  eqBoundGrid.at(5u) = 5u;

  auto p1 = typename GridTypeEQB::point_t{0.5};
  BOOST_CHECK_EQUAL(eqBoundGrid.atPosition(p1), 1u);
  auto p2 = typename GridTypeEQB::point_t{1.5};
  BOOST_CHECK_EQUAL(eqBoundGrid.atPosition(p2), 2u);
  auto p3 = typename GridTypeEQB::point_t{2.5};
  BOOST_CHECK_EQUAL(eqBoundGrid.atPosition(p3), 3u);
  auto p4 = typename GridTypeEQB::point_t{3.5};
  BOOST_CHECK_EQUAL(eqBoundGrid.atPosition(p4), 4u);
  auto p5 = typename GridTypeEQB::point_t{4.5};
  BOOST_CHECK_EQUAL(eqBoundGrid.atPosition(p5), 5u);

  nlohmann::json eqBoundJson = Acts::GridJsonConverter::toJson(eqBoundGrid);

  auto eqBoundGridRead =
      Acts::GridJsonConverter::fromJson<EqBound, std::size_t>(eqBoundJson,
                                                              eqBound);

  BOOST_CHECK_EQUAL(eqBoundGridRead.at(1u), 1u);
  BOOST_CHECK_EQUAL(eqBoundGridRead.at(2u), 2u);
  BOOST_CHECK_EQUAL(eqBoundGridRead.at(3u), 3u);
  BOOST_CHECK_EQUAL(eqBoundGridRead.at(4u), 4u);
  BOOST_CHECK_EQUAL(eqBoundGridRead.at(5u), 5u);

  // Bound variable
  using VarBound = Acts::GridAxisGenerators::VarBound;

  VarBound varBound{{10., 11., 22., 333., 4444., 55555.}};
  // Create the grid with the provided axis generator
  using GridTypeEQV = typename VarBound::template grid_type<std::size_t>;
  GridTypeEQV varBoundGrid(varBound());

  varBoundGrid.at(1u) = 1u;
  varBoundGrid.at(2u) = 2u;
  varBoundGrid.at(3u) = 3u;
  varBoundGrid.at(4u) = 4u;
  varBoundGrid.at(5u) = 5u;

  nlohmann::json varBoundJson = Acts::GridJsonConverter::toJson(varBoundGrid);

  auto varBoundGridRead =
      Acts::GridJsonConverter::fromJson<VarBound, std::size_t>(varBoundJson,
                                                               varBound);

  BOOST_CHECK_EQUAL(varBoundGridRead.at(1u), 1u);
  BOOST_CHECK_EQUAL(varBoundGridRead.at(2u), 2u);
  BOOST_CHECK_EQUAL(varBoundGridRead.at(3u), 3u);
  BOOST_CHECK_EQUAL(varBoundGridRead.at(4u), 4u);
  BOOST_CHECK_EQUAL(varBoundGridRead.at(5u), 5u);

  // Closed equidistant
  using EqClosed = Acts::GridAxisGenerators::EqClosed;

  EqClosed eqClosed{{0., 5.}, 5};
  // Create the grid with the provided axis generator
  using GridTypeEQC = typename EqClosed::template grid_type<std::size_t>;
  GridTypeEQC eqClosedGrid(eqClosed());

  eqClosedGrid.at(1u) = 1u;
  eqClosedGrid.at(2u) = 2u;
  eqClosedGrid.at(3u) = 3u;
  eqClosedGrid.at(4u) = 4u;
  eqClosedGrid.at(5u) = 5u;

  nlohmann::json eqClosedJson = Acts::GridJsonConverter::toJson(eqClosedGrid);

  auto eqClosedGridRead =
      Acts::GridJsonConverter::fromJson<EqClosed, std::size_t>(eqClosedJson,
                                                               eqClosed);

  BOOST_CHECK_EQUAL(eqClosedGridRead.at(1u), 1u);
  BOOST_CHECK_EQUAL(eqClosedGridRead.at(2u), 2u);
  BOOST_CHECK_EQUAL(eqClosedGridRead.at(3u), 3u);
  BOOST_CHECK_EQUAL(eqClosedGridRead.at(4u), 4u);
  BOOST_CHECK_EQUAL(eqClosedGridRead.at(5u), 5u);
}

BOOST_AUTO_TEST_CASE(Grid1DArrayEntry) {
  // Bound equidistant
  using EqBound = Acts::GridAxisGenerators::EqBound;

  EqBound eqBound{{0., 5.}, 5};
  // Create the grid with the provided axis generator
  using GridTypeEQB =
      typename EqBound::template grid_type<std::array<std::size_t, 2u>>;
  GridTypeEQB eqBoundGrid(eqBound());

  eqBoundGrid.at(1u) = {1u, 1u};
  eqBoundGrid.at(2u) = {2u, 2u};
  eqBoundGrid.at(3u) = {3u, 3u};
  eqBoundGrid.at(4u) = {4u, 4u};
  eqBoundGrid.at(5u) = {5u, 5u};

  nlohmann::json eqBoundJson = Acts::GridJsonConverter::toJson(eqBoundGrid);

  auto eqBoundGridRead =
      Acts::GridJsonConverter::fromJson<EqBound, std::array<std::size_t, 2u>>(
          eqBoundJson, eqBound);

  BOOST_CHECK((eqBoundGridRead.at(1u) == std::array<std::size_t, 2u>{1u, 1u}));
  BOOST_CHECK((eqBoundGridRead.at(2u) == std::array<std::size_t, 2u>{2u, 2u}));
  BOOST_CHECK((eqBoundGridRead.at(3u) == std::array<std::size_t, 2u>{3u, 3u}));
  BOOST_CHECK((eqBoundGridRead.at(4u) == std::array<std::size_t, 2u>{4u, 4u}));
  BOOST_CHECK((eqBoundGridRead.at(5u) == std::array<std::size_t, 2u>{5u, 5u}));
}

BOOST_AUTO_TEST_CASE(Grid2DSingleEntryBound) {
  using EqBoundEqBound = Acts::GridAxisGenerators::EqBoundEqBound;

  EqBoundEqBound eqBound2{{0., 5.}, 5, {0., 2.}, 2};
  // Create the grid with the provided axis generator
  using GridTypeEQB2 = typename EqBoundEqBound::template grid_type<std::size_t>;
  GridTypeEQB2 eqBound2Grid(eqBound2());

  // Let's write in local coordinates
  using GridPoint = typename GridTypeEQB2::point_t;

  // First row access
  GridPoint p11{0.5, 0.5};
  GridPoint p12{1.5, 0.5};
  GridPoint p13{2.5, 0.5};
  GridPoint p14{3.5, 0.5};
  GridPoint p15{4.5, 0.5};
  eqBound2Grid.atPosition(p11) = 11u;
  eqBound2Grid.atPosition(p12) = 12u;
  eqBound2Grid.atPosition(p13) = 13u;
  eqBound2Grid.atPosition(p14) = 14u;
  eqBound2Grid.atPosition(p15) = 15u;

  // Second row access
  GridPoint p21{0.5, 1.5};
  GridPoint p22{1.5, 1.5};
  GridPoint p23{2.5, 1.5};
  GridPoint p24{3.5, 1.5};
  GridPoint p25{4.5, 1.5};
  eqBound2Grid.atPosition(p21) = 21u;
  eqBound2Grid.atPosition(p22) = 22u;
  eqBound2Grid.atPosition(p23) = 23u;
  eqBound2Grid.atPosition(p24) = 24u;
  eqBound2Grid.atPosition(p25) = 25u;

  nlohmann::json eqBound2Json = Acts::GridJsonConverter::toJson(eqBound2Grid);

  auto eqBound2JsonRead =
      Acts::GridJsonConverter::fromJson<EqBoundEqBound, std::size_t>(
          eqBound2Json, eqBound2);

  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p11), 11u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p12), 12u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p13), 13u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p14), 14u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p15), 15u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p21), 21u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p22), 22u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p23), 23u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p24), 24u);
  BOOST_CHECK_EQUAL(eqBound2JsonRead.atPosition(p25), 25u);
}

BOOST_AUTO_TEST_CASE(Grid2DSingleEntryBoundClosed) {
  using EqBoundEqClosed = Acts::GridAxisGenerators::EqBoundEqClosed;

  EqBoundEqClosed eqBoundEqClosed{{-6., 6.}, 3, {-M_PI, M_PI}, 3};
  // Create the grid with the provided axis generator
  using GridTypeEQBEQC =
      typename EqBoundEqClosed::template grid_type<std::size_t>;
  GridTypeEQBEQC eqBoundEqClosedGrid(eqBoundEqClosed());

  // Let's write in local coordinates
  using GridPoint = typename GridTypeEQBEQC::point_t;

  // First row access
  GridPoint p11{-5, -2.};
  GridPoint p12{0., -2};
  GridPoint p13{5, -2};
  eqBoundEqClosedGrid.atPosition(p11) = 11u;
  eqBoundEqClosedGrid.atPosition(p12) = 12u;
  eqBoundEqClosedGrid.atPosition(p13) = 13u;

  // Middle row access
  GridPoint p21{-5., 0.};
  GridPoint p22{0., 0.};
  GridPoint p23{5., 0.};
  eqBoundEqClosedGrid.atPosition(p21) = 21u;
  eqBoundEqClosedGrid.atPosition(p22) = 22u;
  eqBoundEqClosedGrid.atPosition(p23) = 23u;

  // Last row access
  GridPoint p31{-5., 2.};
  GridPoint p32{0., 2.};
  GridPoint p33{5., 2.};
  eqBoundEqClosedGrid.atPosition(p31) = 31u;
  eqBoundEqClosedGrid.atPosition(p32) = 32u;
  eqBoundEqClosedGrid.atPosition(p33) = 33u;

  nlohmann::json eqBoundEqClosedJson =
      Acts::GridJsonConverter::toJson(eqBoundEqClosedGrid);

  auto eqBoundEqClosedJsonRead =
      Acts::GridJsonConverter::fromJson<EqBoundEqClosed, std::size_t>(
          eqBoundEqClosedJson, eqBoundEqClosed);

  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p11), 11u);
  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p12), 12u);
  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p13), 13u);

  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p21), 21u);
  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p22), 22u);
  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p23), 23u);

  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p31), 31u);
  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p32), 32u);
  BOOST_CHECK_EQUAL(eqBoundEqClosedJsonRead.atPosition(p33), 33u);
}

namespace {
template <typename ReferenceType, typename CheckTypeUniquePtr>
bool checkType(const ReferenceType& /**unused*/,
               const CheckTypeUniquePtr& g2l) {
  return (dynamic_cast<const ReferenceType*>(g2l.get()) != nullptr);
}

template <typename SubspactTuple>
void checkGlobalSubspaceTuple(const SubspactTuple& sstuple) {
  // Test without transform
  std::vector<nlohmann::json> jsspace;
  std::apply(
      [&](auto&&... vals) {
        (jsspace.push_back(Acts::GridAccessJsonConverter::toJson(vals)), ...);
      },
      sstuple);

  // Test that none of them are empty
  for (auto& jss : jsspace) {
    BOOST_CHECK(!jss.empty());
  }

  // Read back in
  std::vector<std::unique_ptr<const Acts::GridAccess::IGlobalToGridLocal>>
      sspaceRead;
  for (auto& jss : jsspace) {
    sspaceRead.push_back(
        Acts::GridAccessJsonConverter::globalToGridLocalFromJson(jss));
    if (jss["accessors"].size() == 1u) {
      auto delegate =
          Acts::GridAccessJsonConverter::globalToGridLocal1DimDelegateFromJson(
              jss);
      BOOST_CHECK(delegate.connected());
    } else if (jss["accessors"].size() == 2u) {
      auto delegate =
          Acts::GridAccessJsonConverter::globalToGridLocal2DimDelegateFromJson(
              jss);
      BOOST_CHECK(delegate.connected());
    } else {
      BOOST_CHECK(false);
    }
  }

  // Test that none of them are empty
  for (auto& ssp : sspaceRead) {
    BOOST_CHECK(ssp != nullptr);
  }

  // Check that the type is correct
  std::size_t irn = 0;
  bool good = true;
  std::apply(
      [&](auto&&... vals) {
        ((good = good && checkType(vals, sspaceRead[irn++])), ...);
      },
      sstuple);
  BOOST_CHECK(good);

  Acts::Transform3 tTransform;
  tTransform.pretranslate(Acts::Vector3{0., 0., 100.});

  // Test with transform
  std::vector<nlohmann::json> jsspaceTransform;
  std::apply(
      [&](auto... vals) {
        (jsspaceTransform.push_back(Acts::GridAccessJsonConverter::toJson(
             Acts::GridAccess::Affine3Transformed<decltype(vals)>(vals,
                                                                  tTransform))),
         ...);
      },
      sstuple);

  // Test that none of them are empty & everyone has a stransform
  for (auto& jss : jsspaceTransform) {
    BOOST_CHECK(!jss.empty());
    BOOST_CHECK(jss.find("transform") != jss.end());
  }

  // Read back in
  std::vector<std::unique_ptr<const Acts::GridAccess::IGlobalToGridLocal>>
      sspaceTransformRead;
  for (auto& jss : jsspaceTransform) {
    sspaceTransformRead.push_back(
        Acts::GridAccessJsonConverter::globalToGridLocalFromJson(jss));
  }

  // Test that none of them are empty
  for (auto& ssp : sspaceTransformRead) {
    BOOST_CHECK(ssp != nullptr);
  }

  // Check that the type is correct
  irn = 0;
  good = true;
  std::apply(
      [&](auto... vals) {
        ((good = good &&
                 checkType(Acts::GridAccess::Affine3Transformed<decltype(vals)>(
                               vals, tTransform),
                           sspaceTransformRead[irn++])),
         ...);
      },
      sstuple);
  BOOST_CHECK(good);
}

}  // namespace

BOOST_AUTO_TEST_CASE(GlobalSubSpaceTests1D) {
  // One dimensional sub spaces
  const std::tuple<Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binX>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binY>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binZ>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binR>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binPhi>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binEta>>
      sspace1D;

  // Check the tuple for 1D
  checkGlobalSubspaceTuple(sspace1D);
}

BOOST_AUTO_TEST_CASE(GlobalSubSpaceTests2D) {
  // Two dimensional sub spaces
  const std::tuple<Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binX,
                                                    Acts::BinningValue::binY>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binY,
                                                    Acts::BinningValue::binX>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binX,
                                                    Acts::BinningValue::binZ>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binZ,
                                                    Acts::BinningValue::binX>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binY,
                                                    Acts::BinningValue::binZ>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binZ,
                                                    Acts::BinningValue::binY>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binR,
                                                    Acts::BinningValue::binPhi>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binPhi,
                                                    Acts::BinningValue::binR>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binZ,
                                                    Acts::BinningValue::binPhi>,
                   Acts::GridAccess::GlobalSubspace<Acts::BinningValue::binPhi,
                                                    Acts::BinningValue::binZ>>
      sspace2D = {};

  // Check the tuple for 2D
  checkGlobalSubspaceTuple(sspace2D);
}

BOOST_AUTO_TEST_CASE(LocalSubspaceTests) {
  const std::tuple<Acts::GridAccess::LocalSubspace<0u>,
                   Acts::GridAccess::LocalSubspace<1u>,
                   Acts::GridAccess::LocalSubspace<0u, 1u>,
                   Acts::GridAccess::LocalSubspace<1u, 0u>>
      lspace1D;

  // Write them to json
  std::vector<nlohmann::json> jlspace;
  std::apply(
      [&](auto&&... vals) {
        (jlspace.push_back(Acts::GridAccessJsonConverter::toJson(vals)), ...);
      },
      lspace1D);

  // Check that none of them is empty
  for (auto& jls : jlspace) {
    BOOST_CHECK(!jls.empty());
  }

  std::vector<std::unique_ptr<const Acts::GridAccess::IBoundToGridLocal>>
      lspaceRead;
  for (auto& jls : jlspace) {
    lspaceRead.push_back(
        Acts::GridAccessJsonConverter::boundToGridLocalFromJson(jls));
    if (jls["accessors"].size() == 1u) {
      auto delegate =
          Acts::GridAccessJsonConverter::boundToGridLocal1DimDelegateFromJson(
              jls);
      BOOST_CHECK(delegate.connected());
    } else if (jls["accessors"].size() == 2u) {
      auto delegate =
          Acts::GridAccessJsonConverter::boundToGridLocal2DimDelegateFromJson(
              jls);
      BOOST_CHECK(delegate.connected());
    } else {
      BOOST_CHECK(false);
    }
  }

  // Test that none of them are empty
  for (auto& lsp : lspaceRead) {
    BOOST_CHECK(lsp != nullptr);
  }

  // Check that the type is correct
  std::size_t irn = 0;
  bool good = true;
  std::apply(
      [&](auto&&... vals) {
        ((good = good && checkType(vals, lspaceRead[irn++])), ...);
      },
      lspace1D);
  BOOST_CHECK(good);
}

BOOST_AUTO_TEST_CASE(BoundCylinderToZPhiTest) {
  Acts::GridAccess::BoundCylinderToZPhi boundCylinderToZPhi(100., 10.);

  nlohmann::json jboundCylinderToZPhi =
      Acts::GridAccessJsonConverter::toJson(boundCylinderToZPhi);

  // Check it is not empty
  BOOST_CHECK(!jboundCylinderToZPhi.empty());

  auto boundCylinderToZPhiRead =
      Acts::GridAccessJsonConverter::boundToGridLocalFromJson(
          jboundCylinderToZPhi);

  // Check that it is not empty
  BOOST_REQUIRE(boundCylinderToZPhiRead != nullptr);

  const Acts::GridAccess::BoundCylinderToZPhi* bct =
      dynamic_cast<const Acts::GridAccess::BoundCylinderToZPhi*>(
          boundCylinderToZPhiRead.get());

  auto delegate =
      Acts::GridAccessJsonConverter::boundToGridLocal2DimDelegateFromJson(
          jboundCylinderToZPhi);
  BOOST_CHECK(delegate.connected());

  BOOST_REQUIRE(bct != nullptr);
  CHECK_CLOSE_ABS(bct->radius, 100., 1e-5);
  CHECK_CLOSE_ABS(bct->shift, 10., 1e-5);
}

BOOST_AUTO_TEST_SUITE_END()

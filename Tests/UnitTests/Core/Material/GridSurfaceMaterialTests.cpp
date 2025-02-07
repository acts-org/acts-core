// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Material/GridSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <numbers>
#include <vector>

// this is a global access to the x coordinate
class GlobalAccessX final : public Acts::GridAccess::IGlobalToGridLocal {
 public:
  std::array<double, 1u> g2X(const Acts::Vector3& global) const {
    return {global.x()};
  }
};

class LocalAccessX final : public Acts::GridAccess::IBoundToGridLocal {
 public:
  std::array<double, 1u> l2X(const Acts::Vector2& local) const {
    return {local.x()};
  }
};

class GlobalToZPhi final : public Acts::GridAccess::IGlobalToGridLocal {
 public:
  double zShift = 0.;

  GlobalToZPhi(double shift) : zShift(shift) {}

  std::array<double, 2u> g2ZPhi(const Acts::Vector3& global) const {
    return {global.z() + zShift, Acts::VectorHelpers::phi(global)};
  }
};

// Local on cylinder surface is rPhi, z
class LocalToZPhi final : public Acts::GridAccess::IBoundToGridLocal {
 public:
  double radius = 1.;

  LocalToZPhi(double r) : radius(r) {}

  std::array<double, 2u> l2ZPhi(const Acts::Vector2& local) const {
    return {local[1u], local[0u] / radius};
  }
};

BOOST_AUTO_TEST_SUITE(Material)

// This test covers some wrongly configured cases
BOOST_AUTO_TEST_CASE(GridIndexedMaterial_invalid_bound2Grid_Unconnected) {
  std::vector<Acts::MaterialSlab> material;

  using EqBound = Acts::GridAxisGenerators::EqBound;
  using EqGrid = EqBound::grid_type<std::size_t>;

  EqBound eqBound{{0., 5.}, 5};
  EqGrid eqGrid{eqBound()};

  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX;

  auto globalX = std::make_unique<const GlobalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX;
  gToX.connect<&GlobalAccessX::g2X>(std::move(globalX));

  BOOST_CHECK_THROW(
      auto ism = Acts::IndexedSurfaceMaterial<EqGrid>(
          std::move(eqGrid), Acts::IndexedMaterialAccessor{std::move(material)},
          std::move(bToX), std::move(gToX)),
      std::invalid_argument);
}

// This test covers some wrongly configured cases
BOOST_AUTO_TEST_CASE(GridIndexedMaterial_invalid_global2Grid_Unconnected) {
  std::vector<Acts::MaterialSlab> material;

  using EqBound = Acts::GridAxisGenerators::EqBound;
  using EqGrid = EqBound::grid_type<std::size_t>;

  EqBound eqBound{{0., 5.}, 5};
  EqGrid eqGrid{eqBound()};

  auto localX = std::make_unique<const LocalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX;
  bToX.connect<&LocalAccessX::l2X>(std::move(localX));

  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX;

  BOOST_CHECK_THROW(
      auto ism = Acts::IndexedSurfaceMaterial<EqGrid>(
          std::move(eqGrid), Acts::IndexedMaterialAccessor{std::move(material)},
          std::move(bToX), std::move(gToX)),
      std::invalid_argument);
}

// This test covers the locally indexed grid material in 1D
BOOST_AUTO_TEST_CASE(GridIndexedMaterial1D) {
  std::vector<Acts::MaterialSlab> material;
  material.emplace_back(Acts::Material(), 0.0);  // vacuum
  material.emplace_back(
      Acts::Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0), 1.0);
  material.emplace_back(
      Acts::Material::fromMolarDensity(11.0, 12.0, 13.0, 14.0, 15.0), 2.0);
  material.emplace_back(
      Acts::Material::fromMolarDensity(21.0, 22.0, 23.0, 24.0, 25.0), 3.0);

  using EqBound = Acts::GridAxisGenerators::EqBound;
  using EqGrid = EqBound::grid_type<std::size_t>;
  using Point = EqGrid::point_t;

  EqBound eqBound{{0., 5.}, 5};
  EqGrid eqGrid{eqBound()};

  eqGrid.atPosition(Point{0.5}) = 1u;  // material 1
  eqGrid.atPosition(Point{1.5}) = 0u;  // vacuum
  eqGrid.atPosition(Point{2.5}) = 2u;  // material 2
  eqGrid.atPosition(Point{3.5}) = 2u;  // material 2
  eqGrid.atPosition(Point{4.5}) = 3u;  // material 3

  auto localX = std::make_unique<const LocalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX;
  bToX.connect<&LocalAccessX::l2X>(std::move(localX));

  auto globalX = std::make_unique<const GlobalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX;
  gToX.connect<&GlobalAccessX::g2X>(std::move(globalX));

  Acts::IndexedSurfaceMaterial<EqGrid> ism(
      std::move(eqGrid), Acts::IndexedMaterialAccessor{std::move(material)},
      std::move(bToX), std::move(gToX));

  // Global access test
  Acts::Vector3 g0(0.5, 0., 0.);
  Acts::Vector3 g1(1.5, 0., 0.);
  Acts::Vector3 g2(2.5, 0., 0.);
  Acts::Vector3 g3(3.5, 0., 0.);
  Acts::Vector3 g4(4.5, 0., 0.);

  const Acts::MaterialSlab& mg0 = ism.materialSlab(g0);
  const Acts::MaterialSlab& mg1 = ism.materialSlab(g1);
  const Acts::MaterialSlab& mg2 = ism.materialSlab(g2);
  const Acts::MaterialSlab& mg3 = ism.materialSlab(g3);
  const Acts::MaterialSlab& mg4 = ism.materialSlab(g4);

  BOOST_CHECK_EQUAL(mg0.material().X0(), 1.);
  BOOST_CHECK(!mg1.material().isValid());
  BOOST_CHECK_EQUAL(mg2.material().X0(), 11.);
  BOOST_CHECK_EQUAL(mg3.material().X0(), 11.);
  BOOST_CHECK_EQUAL(mg4.material().X0(), 21.);

  // Local access test
  Acts::Vector2 l0(0.5, 0.);
  Acts::Vector2 l1(1.5, 0.);
  Acts::Vector2 l2(2.5, 0.);
  Acts::Vector2 l3(3.5, 0.);
  Acts::Vector2 l4(4.5, 0.);

  const Acts::MaterialSlab& ml0 = ism.materialSlab(l0);
  const Acts::MaterialSlab& ml1 = ism.materialSlab(l1);
  const Acts::MaterialSlab& ml2 = ism.materialSlab(l2);
  const Acts::MaterialSlab& ml3 = ism.materialSlab(l3);
  const Acts::MaterialSlab& ml4 = ism.materialSlab(l4);

  BOOST_CHECK_EQUAL(ml0.material().X0(), 1.);
  BOOST_CHECK(!ml1.material().isValid());
  BOOST_CHECK_EQUAL(ml2.material().X0(), 11.);
  BOOST_CHECK_EQUAL(ml3.material().X0(), 11.);
  BOOST_CHECK_EQUAL(ml4.material().X0(), 21.);

  // Now scale it - and access again
  ism.scale(2.);
  const Acts::MaterialSlab& sml0 = ism.materialSlab(l0);
  const Acts::MaterialSlab& sml1 = ism.materialSlab(l1);
  const Acts::MaterialSlab& sml2 = ism.materialSlab(l2);
  const Acts::MaterialSlab& sml3 = ism.materialSlab(l3);
  const Acts::MaterialSlab& sml4 = ism.materialSlab(l4);

  BOOST_CHECK_EQUAL(sml0.thickness(), 2.);
  BOOST_CHECK(!sml1.material().isValid());
  BOOST_CHECK_EQUAL(sml2.thickness(), 4.);
  BOOST_CHECK_EQUAL(sml3.thickness(), 4.);
  BOOST_CHECK_EQUAL(sml4.thickness(), 6.);
}

// This test covers the locally indexed grid material in 2D
BOOST_AUTO_TEST_CASE(GridIndexedMaterial2D) {
  std::vector<Acts::MaterialSlab> material;
  material.emplace_back(Acts::Material(), 1.0);  // vacuum
  material.emplace_back(
      Acts::Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0), 1.0);
  material.emplace_back(
      Acts::Material::fromMolarDensity(11.0, 12.0, 13.0, 14.0, 15.0), 1.0);
  material.emplace_back(
      Acts::Material::fromMolarDensity(21.0, 22.0, 23.0, 24.0, 25.0), 1.0);

  using EqBoundEqClosed = Acts::GridAxisGenerators::EqBoundEqClosed;
  using EqEqGrid = EqBoundEqClosed::grid_type<std::size_t>;
  using Point = EqEqGrid::point_t;

  EqBoundEqClosed eqeqBound{
      {-1., 1.}, 2, {-std::numbers::pi, std::numbers::pi}, 4};
  EqEqGrid eqeqGrid{eqeqBound()};

  eqeqGrid.atPosition(Point{-0.5, -std::numbers::pi * 0.75}) =
      1u;                                                          // material 1
  eqeqGrid.atPosition(Point{-0.5, -std::numbers::pi / 4.}) = 1u;   // material 1
  eqeqGrid.atPosition(Point{-0.5, std::numbers::pi / 4.}) = 0u;    // vacuum
  eqeqGrid.atPosition(Point{-0.5, std::numbers::pi * 0.75}) = 2u;  // material 2

  eqeqGrid.atPosition(Point{0.5, -std::numbers::pi * 0.75}) = 0u;  // vacuum
  eqeqGrid.atPosition(Point{0.5, -std::numbers::pi / 4.}) = 3u;    // material 3
  eqeqGrid.atPosition(Point{0.5, std::numbers::pi / 4.}) = 3u;     // material 3
  eqeqGrid.atPosition(Point{0.5, std::numbers::pi * 0.75}) = 0u;   // vacuum

  // With radius 20
  auto boundToGrid = std::make_unique<const LocalToZPhi>(20.);
  Acts::IndexedSurfaceMaterial<EqEqGrid>::BoundToGridLocalDelegate bToZPhi;
  bToZPhi.connect<&LocalToZPhi::l2ZPhi>(std::move(boundToGrid));

  // With z shift 10
  auto globalToGrid = std::make_unique<const GlobalToZPhi>(10.);
  Acts::IndexedSurfaceMaterial<EqEqGrid>::GlobalToGridLocalDelegate gToZphi;
  gToZphi.connect<&GlobalToZPhi::g2ZPhi>(std::move(globalToGrid));

  // Create the indexed material grid
  Acts::IndexedSurfaceMaterial<EqEqGrid> ism(
      std::move(eqeqGrid), Acts::IndexedMaterialAccessor{std::move(material)},
      std::move(bToZPhi), std::move(gToZphi));

  // Global access test, both should give material 1
  Acts::Vector3 g0(-0.5, -0.5, -10.5);
  const Acts::MaterialSlab& mg0 = ism.materialSlab(g0);
  BOOST_CHECK_EQUAL(mg0.material().X0(), 1.);

  Acts::Vector3 g1(0.5, -0.5, -11.5);  // checking out of bound access
  const Acts::MaterialSlab& mg1 = ism.materialSlab(g1);
  BOOST_CHECK_EQUAL(mg1.material().X0(), 1.);

  Acts::Vector3 g2(0.5, 0.5, -10.5);
  const Acts::MaterialSlab& mg2 = ism.materialSlab(g2);
  BOOST_CHECK(!mg2.material().isValid());  // vacuum

  Acts::Vector3 g3(0.5, 0.5,
                   -9.5);  // should be material 3, same phi but different z
  const Acts::MaterialSlab& mg3 = ism.materialSlab(g3);
  BOOST_CHECK_EQUAL(mg3.material().X0(), 21.);
}

// This test covers the globally indexed grid material with non-shared material
BOOST_AUTO_TEST_CASE(GridGloballyIndexedMaterialNonShared) {
  auto material = std::make_shared<std::vector<Acts::MaterialSlab>>();

  material->emplace_back(Acts::Material(), 0.0);  // vacuum
  material->emplace_back(
      Acts::Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0), 1.0);
  material->emplace_back(
      Acts::Material::fromMolarDensity(11.0, 12.0, 13.0, 14.0, 15.0), 2.0);
  material->emplace_back(
      Acts::Material::fromMolarDensity(21.0, 22.0, 23.0, 24.0, 25.0), 3.0);
  material->emplace_back(
      Acts::Material::fromMolarDensity(31.0, 22.0, 23.0, 24.0, 25.0), 4.0);

  using EqBound = Acts::GridAxisGenerators::EqBound;
  using EqGrid = EqBound::grid_type<std::size_t>;
  using Point = EqGrid::point_t;

  EqBound eqBound{{0., 5.}, 5};
  EqGrid eqGrid{eqBound()};

  eqGrid.atPosition(Point{0.5}) = 1u;  // material 1
  eqGrid.atPosition(Point{1.5}) = 0u;  // vacuum
  eqGrid.atPosition(Point{2.5}) = 2u;  // material 2
  eqGrid.atPosition(Point{3.5}) = 2u;  // material 2
  eqGrid.atPosition(Point{4.5}) = 3u;  // material 3

  auto localX = std::make_unique<const LocalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX;
  bToX.connect<&LocalAccessX::l2X>(std::move(localX));

  auto globalX = std::make_unique<const GlobalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX;
  gToX.connect<&GlobalAccessX::g2X>(std::move(globalX));

  Acts::GloballyIndexedSurfaceMaterial<EqGrid> ism(
      std::move(eqGrid), Acts::GloballyIndexedMaterialAccessor{material, false},
      std::move(bToX), std::move(gToX));

  // Local access test
  Acts::Vector2 l0(0.5, 0.);
  Acts::Vector2 l1(1.5, 0.);
  Acts::Vector2 l2(2.5, 0.);
  Acts::Vector2 l3(3.5, 0.);
  Acts::Vector2 l4(4.5, 0.);

  const Acts::MaterialSlab& ml0 = ism.materialSlab(l0);
  const Acts::MaterialSlab& ml1 = ism.materialSlab(l1);
  const Acts::MaterialSlab& ml2 = ism.materialSlab(l2);
  const Acts::MaterialSlab& ml3 = ism.materialSlab(l3);
  const Acts::MaterialSlab& ml4 = ism.materialSlab(l4);

  BOOST_CHECK_EQUAL(ml0.material().X0(), 1.);
  BOOST_CHECK(!ml1.material().isValid());
  BOOST_CHECK_EQUAL(ml2.material().X0(), 11.);
  BOOST_CHECK_EQUAL(ml3.material().X0(), 11.);
  BOOST_CHECK_EQUAL(ml4.material().X0(), 21.);

  EqBound eqBound1{{0., 5.}, 1};
  EqGrid eqGrid1{eqBound1()};

  auto localX1 = std::make_unique<const LocalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX1;
  bToX1.connect<&LocalAccessX::l2X>(std::move(localX1));

  auto globalX1 = std::make_unique<const GlobalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX1;
  gToX1.connect<&GlobalAccessX::g2X>(std::move(globalX1));

  eqGrid1.atPosition(Point{2.5}) = 4u;  // material 4

  Acts::GloballyIndexedSurfaceMaterial<EqGrid> ism1(
      std::move(eqGrid1),
      Acts::GloballyIndexedMaterialAccessor{material, false}, std::move(bToX1),
      std::move(gToX1));

  Acts::Vector2 l0g1(2.5, 0.);
  const Acts::MaterialSlab& ml0g1 = ism1.materialSlab(l0g1);
  BOOST_CHECK_EQUAL(ml0g1.material().X0(), 31.);

  // Scale
  ism1.scale(2.);
  const Acts::MaterialSlab& sml0g1 = ism1.materialSlab(l0g1);
  BOOST_CHECK_EQUAL(sml0g1.thickness(), 8.);

  // First one stays unscaled
  const Acts::MaterialSlab& sml0 = ism.materialSlab(l0);
  BOOST_CHECK_EQUAL(sml0.thickness(), 1.);
}

// This test covers the globally indexed grid material with shared
BOOST_AUTO_TEST_CASE(GridGloballyIndexedMaterialShared) {
  auto material = std::make_shared<std::vector<Acts::MaterialSlab>>();

  material->emplace_back(Acts::Material(), 0.0);  // vacuum
  material->emplace_back(
      Acts::Material::fromMolarDensity(1.0, 2.0, 3.0, 4.0, 5.0), 1.0);

  using EqBound = Acts::GridAxisGenerators::EqBound;
  using EqGrid = EqBound::grid_type<std::size_t>;
  using Point = EqGrid::point_t;

  EqBound eqBound0{{0., 5.}, 1};
  EqGrid eqGrid0{eqBound0()};

  eqGrid0.atPosition(Point{2.5}) = 1u;  // material 1
  auto localX0 = std::make_unique<const LocalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX0;
  bToX0.connect<&LocalAccessX::l2X>(std::move(localX0));

  auto globalX0 = std::make_unique<const GlobalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX0;
  gToX0.connect<&GlobalAccessX::g2X>(std::move(globalX0));

  Acts::GloballyIndexedSurfaceMaterial<EqGrid> ism0(
      std::move(eqGrid0), Acts::GloballyIndexedMaterialAccessor{material, true},
      std::move(bToX0), std::move(gToX0));

  EqBound eqBound1{{0., 5.}, 1};
  EqGrid eqGrid1{eqBound1()};

  eqGrid1.atPosition(Point{2.5}) = 1u;  // material 1
  auto localX1 = std::make_unique<const LocalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX1;
  bToX1.connect<&LocalAccessX::l2X>(std::move(localX1));

  auto globalX1 = std::make_unique<const GlobalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX1;
  gToX1.connect<&GlobalAccessX::g2X>(std::move(globalX1));

  Acts::GloballyIndexedSurfaceMaterial<EqGrid> ism1(
      std::move(eqGrid1), Acts::GloballyIndexedMaterialAccessor{material, true},
      std::move(bToX1), std::move(gToX1));

  Acts::Vector2 l0(2.5, 0.);

  // check grid material 0
  const Acts::MaterialSlab& ml0 = ism0.materialSlab(l0);
  BOOST_CHECK_EQUAL(ml0.material().X0(), 1.);

  const Acts::MaterialSlab& ml0g1 = ism1.materialSlab(l0);
  BOOST_CHECK_EQUAL(ml0g1.material().X0(), 1.);

  // scaling shared material should throw a std::invalid_argument
  BOOST_CHECK_THROW(ism1.scale(2.), std::invalid_argument);
}

// This test covers the grid material (non-indexed accessor)
//
// In this setup, the material is not indexed, but filled directly
// into the grid structure.
BOOST_AUTO_TEST_CASE(GridSurfaceMaterialTests) {
  using EqBound = Acts::GridAxisGenerators::EqBound;
  using EqGrid = EqBound::grid_type<Acts::MaterialSlab>;
  using Point = EqGrid::point_t;

  EqBound eqBound{{0., 5.}, 5};
  EqGrid eqGrid{eqBound()};

  eqGrid.atPosition(Point{0.5}) = Acts::MaterialSlab(Acts::Material(), 0.0);
  eqGrid.atPosition(Point{1.5}) = Acts::MaterialSlab(Acts::Material(), 1.0);
  eqGrid.atPosition(Point{2.5}) = Acts::MaterialSlab(Acts::Material(), 2.0);
  eqGrid.atPosition(Point{3.5}) = Acts::MaterialSlab(Acts::Material(), 3.0);
  eqGrid.atPosition(Point{4.5}) = Acts::MaterialSlab(Acts::Material(), 4.0);

  auto localX = std::make_unique<const LocalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::BoundToGridLocalDelegate bToX;
  bToX.connect<&LocalAccessX::l2X>(std::move(localX));

  auto globalX = std::make_unique<const GlobalAccessX>();
  Acts::IndexedSurfaceMaterial<EqGrid>::GlobalToGridLocalDelegate gToX;
  gToX.connect<&GlobalAccessX::g2X>(std::move(globalX));

  Acts::GridSurfaceMaterial<EqGrid> gsm(std::move(eqGrid),
                                        Acts::GridMaterialAccessor{},
                                        std::move(bToX), std::move(gToX));

  // Global access test
  Acts::Vector3 g0(0.5, 0., 0.);
  Acts::Vector3 g1(1.5, 0., 0.);
  Acts::Vector3 g2(2.5, 0., 0.);
  Acts::Vector3 g3(3.5, 0., 0.);
  Acts::Vector3 g4(4.5, 0., 0.);

  const Acts::MaterialSlab& mg0 = gsm.materialSlab(g0);
  const Acts::MaterialSlab& mg1 = gsm.materialSlab(g1);
  const Acts::MaterialSlab& mg2 = gsm.materialSlab(g2);
  const Acts::MaterialSlab& mg3 = gsm.materialSlab(g3);
  const Acts::MaterialSlab& mg4 = gsm.materialSlab(g4);

  BOOST_CHECK_EQUAL(mg0.thickness(), 0.);
  BOOST_CHECK_EQUAL(mg1.thickness(), 1.);
  BOOST_CHECK_EQUAL(mg2.thickness(), 2.);
  BOOST_CHECK_EQUAL(mg3.thickness(), 3.);
  BOOST_CHECK_EQUAL(mg4.thickness(), 4.);

  // Local access test
  Acts::Vector2 l0(0.5, 0.);
  Acts::Vector2 l1(1.5, 0.);
  Acts::Vector2 l2(2.5, 0.);
  Acts::Vector2 l3(3.5, 0.);
  Acts::Vector2 l4(4.5, 0.);

  const Acts::MaterialSlab& ml0 = gsm.materialSlab(l0);
  const Acts::MaterialSlab& ml1 = gsm.materialSlab(l1);
  const Acts::MaterialSlab& ml2 = gsm.materialSlab(l2);
  const Acts::MaterialSlab& ml3 = gsm.materialSlab(l3);
  const Acts::MaterialSlab& ml4 = gsm.materialSlab(l4);

  BOOST_CHECK_EQUAL(ml0.thickness(), 0.);
  BOOST_CHECK_EQUAL(ml1.thickness(), 1.);
  BOOST_CHECK_EQUAL(ml2.thickness(), 2.);
  BOOST_CHECK_EQUAL(ml3.thickness(), 3.);
  BOOST_CHECK_EQUAL(ml4.thickness(), 4.);

  // Now scale it - and access again
  gsm.scale(2.);

  const Acts::MaterialSlab& sml0 = gsm.materialSlab(l0);
  const Acts::MaterialSlab& sml1 = gsm.materialSlab(l1);
  const Acts::MaterialSlab& sml2 = gsm.materialSlab(l2);
  const Acts::MaterialSlab& sml3 = gsm.materialSlab(l3);
  const Acts::MaterialSlab& sml4 = gsm.materialSlab(l4);

  BOOST_CHECK_EQUAL(sml0.thickness(), 0.);
  BOOST_CHECK_EQUAL(sml1.thickness(), 2.);
  BOOST_CHECK_EQUAL(sml2.thickness(), 4.);
  BOOST_CHECK_EQUAL(sml3.thickness(), 6.);
  BOOST_CHECK_EQUAL(sml4.thickness(), 8.);
}

BOOST_AUTO_TEST_SUITE_END()

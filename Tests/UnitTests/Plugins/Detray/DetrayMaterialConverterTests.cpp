// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Plugins/Detray/DetrayConversionUtils.hpp"
#include "Acts/Plugins/Detray/DetrayMaterialConverter.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalDetector.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <numbers>

#include <detray/definitions/grid_axis.hpp>
#include <detray/io/frontend/payloads.hpp>

auto tContext = Acts::GeometryContext();

BOOST_AUTO_TEST_SUITE(DetrayConversion)

// These tests check the conversion to the payload objects, the full test
auto materialSlab12345 =
    Acts::MaterialSlab(Acts::Material::fromMolarDensity(1, 2, 3, 4, 5), 1.);

auto materialSlab678910 =
    Acts::MaterialSlab(Acts::Material::fromMolarDensity(6, 7, 8, 9, 10), 2.);

auto materialSlab54321 =
    Acts::MaterialSlab(Acts::Material::fromMolarDensity(5, 4, 3, 2, 1), 3.);

auto materialSlab109876 =
    Acts::MaterialSlab(Acts::Material::fromMolarDensity(10, 9, 8, 7, 6), 4.);

auto logger =
    Acts::getDefaultLogger("DetrayMaterialConverterTests", Acts::Logging::INFO);

BOOST_AUTO_TEST_CASE(DetrayMaterialSlabConversion) {
  // Convert the material slab
  detray::io::material_slab_payload payload =
      Acts::DetrayMaterialConverter::convertMaterialSlab(materialSlab12345);

  // Material type should be set to slab
  BOOST_CHECK(payload.type ==
              detray::io::material_slab_payload::mat_type::slab);
  // Thickness should be set to one
  CHECK_CLOSE_ABS(payload.thickness, 1.,
                  std::numeric_limits<double>::epsilon());
  // Index in collection not set at this simple conversion
  BOOST_CHECK(!payload.index_in_coll.has_value());
  // Material parameters in detray are (x0, l0, ar, z, mass_density,
  // molar_density, solid/liquid/etc. flag ... ignored currently)
  CHECK_CLOSE_ABS(payload.mat.params[0u], 1.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.mat.params[1u], 2.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.mat.params[2u], 3.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.mat.params[3u], 4.,
                  std::numeric_limits<double>::epsilon());
  BOOST_CHECK_NE(payload.mat.params[4u], payload.mat.params[5u]);
  CHECK_CLOSE_ABS(payload.mat.params[5u], 5.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.mat.params[6u], 0.,
                  std::numeric_limits<double>::epsilon());
}

BOOST_AUTO_TEST_CASE(DetrayHomogeneousMaterialConversion) {
  // Convert homogeneous material
  Acts::HomogeneousSurfaceMaterial homMaterial(materialSlab12345);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          homMaterial, *logger);

  // Check the payload - empty, this should run through another converter
  BOOST_CHECK(payload.axes.empty());
  BOOST_CHECK(payload.bins.empty());
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionX) {
  // Create a binned material in 4 bins in x direction
  Acts::BinUtility binUtility(4u, -2., 2., Acts::BinningOption::open,
                              Acts::AxisDirection::AxisX);

  std::vector<Acts::MaterialSlab> materialSlabs = {
      materialSlab12345, materialSlab678910, materialSlab54321,
      materialSlab109876};

  auto binnedMaterial = Acts::BinnedSurfaceMaterial(
      binUtility, {materialSlabs}, 0., Acts::MappingType::Default);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          binnedMaterial, *logger);
  // Check the payload
  // - we fake 2D always for detray to minimize the number of containers
  // - four material payloads in the grid
  BOOST_CHECK_EQUAL(payload.axes.size(), 2u);
  BOOST_CHECK_EQUAL(payload.bins.size(), 4u);
  BOOST_CHECK(!payload.transform.has_value());
  BOOST_CHECK(payload.grid_link.type ==
              detray::io::material_id::rectangle2_map);
  // x-axis
  BOOST_CHECK(payload.axes[0u].label == detray::axis::label::e_x);
  BOOST_CHECK_EQUAL(payload.axes[0u].bins, 4u);
  CHECK_CLOSE_ABS(payload.axes[0u].edges[0u], -2.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.axes[0u].edges[1u], 2.,
                  std::numeric_limits<double>::epsilon());
  BOOST_CHECK(payload.axes[0u].binning == detray::axis::binning::e_regular);
  BOOST_CHECK(payload.axes[0u].bounds == detray::axis::bounds::e_closed);
  // axis is dummy
  BOOST_CHECK(payload.axes[1u].label == detray::axis::label::e_y);
  BOOST_CHECK_EQUAL(payload.axes[1u].bins, 1u);

  // Check the local indices
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index.size(), 2u);

  // Bins should be : [0,0], [1,0], [2,0], [3,0]
  //
  // [0,0] - materialSlab12345
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index[0], 0u);
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index[1], 0u);
  BOOST_CHECK_EQUAL(payload.bins[0u].content[0u].mat.params[0u], 1.);
  // [1,0] - materialSlab678910
  BOOST_CHECK_EQUAL(payload.bins[1u].loc_index[0], 1u);
  BOOST_CHECK_EQUAL(payload.bins[1u].loc_index[1], 0u);
  BOOST_CHECK_EQUAL(payload.bins[1u].content[0u].mat.params[0u], 6.);
  // [2,0] - materialSlab54321
  BOOST_CHECK_EQUAL(payload.bins[2u].loc_index[0], 2u);
  BOOST_CHECK_EQUAL(payload.bins[2u].loc_index[1], 0u);
  BOOST_CHECK_EQUAL(payload.bins[2u].content[0u].mat.params[0u], 5.);
  // [3,0] - materialSlab109876
  BOOST_CHECK_EQUAL(payload.bins[3u].loc_index[0], 3u);
  BOOST_CHECK_EQUAL(payload.bins[3u].loc_index[1], 0u);
  BOOST_CHECK_EQUAL(payload.bins[3u].content[0u].mat.params[0u], 10.);
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionY) {
  // Create a binned material in 4 bins in y direction
  Acts::BinUtility binUtility(4u, -2., 2., Acts::BinningOption::open,
                              Acts::AxisDirection::AxisY);

  std::vector<Acts::MaterialSlab> materialSlabs = {
      materialSlab12345, materialSlab678910, materialSlab54321,
      materialSlab109876};

  auto binnedMaterial = Acts::BinnedSurfaceMaterial(
      binUtility, {materialSlabs}, 0., Acts::MappingType::Default);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          binnedMaterial, *logger);
  // Check the payload
  // - we fake 2D always for detray to minimize the number of containers
  // - four material payloads in the grid
  BOOST_CHECK_EQUAL(payload.axes.size(), 2u);
  BOOST_CHECK_EQUAL(payload.bins.size(), 4u);
  BOOST_CHECK(!payload.transform.has_value());
  BOOST_CHECK(payload.grid_link.type ==
              detray::io::material_id::rectangle2_map);
  // x-axis is dummy
  BOOST_CHECK(payload.axes[0u].label == detray::axis::label::e_x);
  BOOST_CHECK_EQUAL(payload.axes[0u].bins, 1u);
  // y-axis
  BOOST_CHECK(payload.axes[1u].label == detray::axis::label::e_y);
  BOOST_CHECK_EQUAL(payload.axes[1u].bins, 4u);
  CHECK_CLOSE_ABS(payload.axes[1u].edges[0u], -2.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.axes[1u].edges[1u], 2.,
                  std::numeric_limits<double>::epsilon());
  BOOST_CHECK(payload.axes[1u].binning == detray::axis::binning::e_regular);
  BOOST_CHECK(payload.axes[1u].bounds == detray::axis::bounds::e_closed);

  // Check the local indices
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index.size(), 2u);

  // Bins should be : [0,0], [0,1], [0,2], [0,3]
  //
  // [0, 0] - materialSlab12345
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index[0], 0u);
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index[1], 0u);
  BOOST_CHECK_EQUAL(payload.bins[0u].content[0u].mat.params[0u], 1.);
  // [0, 1]  - materialSlab678910
  BOOST_CHECK_EQUAL(payload.bins[1u].loc_index[0], 0u);
  BOOST_CHECK_EQUAL(payload.bins[1u].loc_index[1], 1u);
  BOOST_CHECK_EQUAL(payload.bins[1u].content[0u].mat.params[0u], 6.);
  // [0, 2] - materialSlab54321
  BOOST_CHECK_EQUAL(payload.bins[2u].loc_index[0], 0u);
  BOOST_CHECK_EQUAL(payload.bins[2u].loc_index[1], 2u);
  BOOST_CHECK_EQUAL(payload.bins[2u].content[0u].mat.params[0u], 5.);
  // [0, 3] - materialSlab109876
  BOOST_CHECK_EQUAL(payload.bins[3u].loc_index[0], 0u);
  BOOST_CHECK_EQUAL(payload.bins[3u].loc_index[1], 3u);
  BOOST_CHECK_EQUAL(payload.bins[3u].content[0u].mat.params[0u], 10.);
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionXY) {
  // Create a binned material in 2 x2  bins in x-y direction
  Acts::BinUtility binUtility(2u, -1., 1., Acts::BinningOption::open,
                              Acts::AxisDirection::AxisX);
  binUtility += Acts::BinUtility(2u, -2., 2., Acts::BinningOption::open,
                                 Acts::AxisDirection::AxisY);

  std::vector<Acts::MaterialSlab> materialSlabs0 = {materialSlab12345,
                                                    materialSlab678910};
  std::vector<Acts::MaterialSlab> materialSlabs1 = {materialSlab54321,
                                                    materialSlab109876};

  auto binnedMaterial =
      Acts::BinnedSurfaceMaterial(binUtility, {materialSlabs0, materialSlabs1},
                                  0., Acts::MappingType::Default);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          binnedMaterial, *logger);
  // Check the payload
  BOOST_CHECK_EQUAL(payload.axes.size(), 2u);
  BOOST_CHECK(payload.grid_link.type ==
              detray::io::material_id::rectangle2_map);
  //  The axis are real x-y
  BOOST_CHECK(payload.axes[0u].label == detray::axis::label::e_x);
  BOOST_CHECK(payload.axes[1u].label == detray::axis::label::e_y);
  BOOST_CHECK_EQUAL(payload.bins.size(), 4u);
  BOOST_CHECK(!payload.transform.has_value());
  // 2 bins in x, 2 bins in y
  BOOST_CHECK_EQUAL(payload.axes[0u].bins, 2u);
  BOOST_CHECK_EQUAL(payload.axes[1u].bins, 2u);

  // Check the local indices
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index.size(), 2u);

  // Bins should be : [0,0], [1,0], [0,1], [1,1]
  //
  // [0, 0] - materialSlab12345
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index[0], 0u);
  BOOST_CHECK_EQUAL(payload.bins[0u].loc_index[1], 0u);
  BOOST_CHECK_EQUAL(payload.bins[0u].content[0u].mat.params[0u], 1.);
  // [0, 1]  - materialSlab678910
  BOOST_CHECK_EQUAL(payload.bins[1u].loc_index[0], 1u);
  BOOST_CHECK_EQUAL(payload.bins[1u].loc_index[1], 0u);
  BOOST_CHECK_EQUAL(payload.bins[1u].content[0u].mat.params[0u], 6.);
  // [1, 0] - materialSlab54321
  BOOST_CHECK_EQUAL(payload.bins[2u].loc_index[0], 0u);
  BOOST_CHECK_EQUAL(payload.bins[2u].loc_index[1], 1u);
  BOOST_CHECK_EQUAL(payload.bins[2u].content[0u].mat.params[0u], 5.);
  // [1, 1] - materialSlab109876
  BOOST_CHECK_EQUAL(payload.bins[3u].loc_index[0], 1u);
  BOOST_CHECK_EQUAL(payload.bins[3u].loc_index[1], 1u);
  BOOST_CHECK_EQUAL(payload.bins[3u].content[0u].mat.params[0u], 10.);
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionR) {
  // Create a binned material in 4 bins (irregularly) in r direction
  std::vector<float> binEdges = {0., 5., 10., 15., 20.};
  Acts::BinUtility binUtility(binEdges, Acts::BinningOption::open,
                              Acts::AxisDirection::AxisR);

  std::vector<Acts::MaterialSlab> materialSlabs = {
      materialSlab12345, materialSlab678910, materialSlab54321,
      materialSlab109876};

  auto binnedMaterial = Acts::BinnedSurfaceMaterial(
      binUtility, {materialSlabs}, 0., Acts::MappingType::Default);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          binnedMaterial, *logger);
  // Check the payload
  // - we fake 2D always for detray to minimize the number of containers
  // - four material payloads in the grid
  BOOST_CHECK_EQUAL(payload.axes.size(), 2u);
  BOOST_CHECK_EQUAL(payload.bins.size(), 4u);
  BOOST_CHECK(!payload.transform.has_value());
  BOOST_CHECK(payload.grid_link.type == detray::io::material_id::ring2_map);
  // first axis is r axis
  BOOST_CHECK(payload.axes[0u].label == detray::axis::label::e_r);
  BOOST_CHECK_EQUAL(payload.axes[0u].bins, 4u);
  BOOST_CHECK_EQUAL(payload.axes[0u].edges.size(), 5);
  CHECK_CLOSE_ABS(payload.axes[0u].edges.front(), 0.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.axes[0u].edges.back(), 20.,
                  std::numeric_limits<double>::epsilon());
  BOOST_CHECK(payload.axes[0u].binning == detray::axis::binning::e_irregular);
  BOOST_CHECK(payload.axes[0u].bounds == detray::axis::bounds::e_closed);
  // 2nd-axis is dummy
  BOOST_CHECK(payload.axes[1u].label == detray::axis::label::e_phi);
  BOOST_CHECK(payload.axes[1u].bounds == detray::axis::bounds::e_circular);
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionRPhi) {
  // Create a binned material in 2 bins - irregularly in r, 2 bins in phi
  std::vector<float> binEdges = {0., 5., 20.};
  Acts::BinUtility binUtility(binEdges, Acts::BinningOption::open,
                              Acts::AxisDirection::AxisR);
  binUtility += Acts::BinUtility(2u, -std::numbers::pi, std::numbers::pi,
                                 Acts::BinningOption::closed,
                                 Acts::AxisDirection::AxisPhi);

  std::vector<Acts::MaterialSlab> materialSlabs0 = {materialSlab12345,
                                                    materialSlab678910};
  std::vector<Acts::MaterialSlab> materialSlabs1 = {materialSlab54321,
                                                    materialSlab109876};

  auto binnedMaterial =
      Acts::BinnedSurfaceMaterial(binUtility, {materialSlabs0, materialSlabs1},
                                  0., Acts::MappingType::Default);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          binnedMaterial, *logger);
  // Check the payload
  BOOST_CHECK_EQUAL(payload.axes.size(), 2u);
  BOOST_CHECK_EQUAL(payload.bins.size(), 4u);
  BOOST_CHECK(!payload.transform.has_value());
  BOOST_CHECK(payload.grid_link.type == detray::io::material_id::ring2_map);
  // 2 bins irregularly in r
  BOOST_CHECK(payload.axes[0u].label == detray::axis::label::e_r);
  BOOST_CHECK_EQUAL(payload.axes[0u].bins, 2u);
  BOOST_CHECK(payload.axes[0u].bounds == detray::axis::bounds::e_closed);
  BOOST_CHECK(payload.axes[0u].binning == detray::axis::binning::e_irregular);
  // 2bins regularly in phi
  BOOST_CHECK(payload.axes[1u].label == detray::axis::label::e_phi);
  BOOST_CHECK_EQUAL(payload.axes[1u].bins, 2u);
  BOOST_CHECK(payload.axes[1u].bounds == detray::axis::bounds::e_circular);
  BOOST_CHECK(payload.axes[1u].binning == detray::axis::binning::e_regular);
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionZ) {
  // Create a binned material in 4 bins in x direction
  std::vector<float> binEdges = {-20, 0, 25, 50, 100};
  Acts::BinUtility binUtility(binEdges, Acts::BinningOption::open,
                              Acts::AxisDirection::AxisZ);

  std::vector<Acts::MaterialSlab> materialSlabs = {
      materialSlab12345, materialSlab678910, materialSlab54321,
      materialSlab109876};

  auto binnedMaterial = Acts::BinnedSurfaceMaterial(
      binUtility, {materialSlabs}, 0., Acts::MappingType::Default);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          binnedMaterial, *logger);

  // Check the payload
  BOOST_CHECK_EQUAL(payload.axes.size(), 2u);
  BOOST_CHECK_EQUAL(payload.bins.size(), 4u);
  BOOST_CHECK(!payload.transform.has_value());
  BOOST_CHECK(payload.grid_link.type ==
              detray::io::material_id::concentric_cylinder2_map);
  // 1st-axis is dummy
  BOOST_CHECK(payload.axes[0u].label == detray::axis::label::e_phi);
  BOOST_CHECK(payload.axes[0u].bounds == detray::axis::bounds::e_circular);
  BOOST_CHECK_EQUAL(payload.axes[0u].bins, 1u);
  // second axis is z axis
  BOOST_CHECK(payload.axes[1u].label == detray::axis::label::e_z);
  BOOST_CHECK_EQUAL(payload.axes[1u].bins, 4u);
  BOOST_CHECK_EQUAL(payload.axes[1u].edges.size(), 5);
  CHECK_CLOSE_ABS(payload.axes[1u].edges.front(), -20.,
                  std::numeric_limits<double>::epsilon());
  CHECK_CLOSE_ABS(payload.axes[1u].edges.back(), 100.,
                  std::numeric_limits<double>::epsilon());
  BOOST_CHECK(payload.axes[1u].binning == detray::axis::binning::e_irregular);
  BOOST_CHECK(payload.axes[1u].bounds == detray::axis::bounds::e_closed);
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionZPhi) {
  // Create a binned material in 2 x2  bins in x-y direction
  Acts::BinUtility binUtility(2u, -1., 1., Acts::BinningOption::open,
                              Acts::AxisDirection::AxisZ);
  binUtility += Acts::BinUtility(2u, -std::numbers::pi, std::numbers::pi,
                                 Acts::BinningOption::closed,
                                 Acts::AxisDirection::AxisPhi);

  std::vector<Acts::MaterialSlab> materialSlabs0 = {materialSlab12345,
                                                    materialSlab678910};
  std::vector<Acts::MaterialSlab> materialSlabs1 = {materialSlab54321,
                                                    materialSlab109876};

  auto binnedMaterial =
      Acts::BinnedSurfaceMaterial(binUtility, {materialSlabs0, materialSlabs1},
                                  0., Acts::MappingType::Default);

  detray::io::grid_payload<detray::io::material_slab_payload,
                           detray::io::material_id>
      payload = Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
          binnedMaterial, *logger);
  // Check the payload
  BOOST_CHECK_EQUAL(payload.axes.size(), 2u);
  BOOST_CHECK(payload.grid_link.type ==
              detray::io::material_id::concentric_cylinder2_map);
  //  The axis are real aphi-z
  BOOST_CHECK(payload.axes[0u].label == detray::axis::label::e_phi);
  BOOST_CHECK(payload.axes[1u].label == detray::axis::label::e_z);
  BOOST_CHECK_EQUAL(payload.bins.size(), 4u);
  BOOST_CHECK(!payload.transform.has_value());
}

BOOST_AUTO_TEST_CASE(DetrayBinnedMaterialConversionInvalid) {
  // Create a binned material in 4 bins in x direction
  Acts::BinUtility binUtility(2u, -1., 1., Acts::BinningOption::open,
                              Acts::AxisDirection::AxisR);
  binUtility += Acts::BinUtility(2u, -2., 2., Acts::BinningOption::open,
                                 Acts::AxisDirection::AxisEta);

  std::vector<Acts::MaterialSlab> materialSlabs0 = {materialSlab12345,
                                                    materialSlab678910};
  std::vector<Acts::MaterialSlab> materialSlabs1 = {materialSlab54321,
                                                    materialSlab109876};

  auto binnedMaterial =
      Acts::BinnedSurfaceMaterial(binUtility, {materialSlabs0, materialSlabs1},
                                  0., Acts::MappingType::Default);

  BOOST_CHECK_THROW(Acts::DetrayMaterialConverter::convertGridSurfaceMaterial(
                        binnedMaterial, *logger),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/ProtoAxisJsonConverter.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/ProtoAxis.hpp"

BOOST_AUTO_TEST_SUITE(ProtoAxisJsonConversion)

BOOST_AUTO_TEST_CASE(EquidistantProtoAxisJsonConversion) {
  using enum Acts::AxisBoundaryType;
  using enum Acts::AxisDirection;
  using enum Acts::AxisType;

  // Bound, equidistant axis
  Acts::ProtoAxis epab(AxisX, Bound, 0.0, 1.0, 10);

  nlohmann::json jProtoAxis = Acts::ProtoAxisJsonConverter::toJson(epab);

  BOOST_CHECK(jProtoAxis.contains("axis"));
  BOOST_CHECK(jProtoAxis.contains("axis_dir"));
  BOOST_CHECK(jProtoAxis.contains("autorange"));

  Acts::ProtoAxis epabRead = Acts::ProtoAxisJsonConverter::fromJson(jProtoAxis);

  BOOST_CHECK_EQUAL(epabRead.getAxisDirection(), epab.getAxisDirection());
  BOOST_CHECK_EQUAL(epabRead.getAxis(), epab.getAxis());
  BOOST_CHECK_EQUAL(epabRead.isAutorange(), epab.isAutorange());
  BOOST_CHECK_EQUAL(epabRead.toString(), epab.toString());
}

BOOST_AUTO_TEST_CASE(AutorangeProtoAxisJsonConversion) {
  using enum Acts::AxisBoundaryType;
  using enum Acts::AxisDirection;
  using enum Acts::AxisType;

  // Bound, equidistant axis, autorange
  Acts::ProtoAxis epa(AxisX, Bound, 10);

  nlohmann::json jProtoAxis = Acts::ProtoAxisJsonConverter::toJson(epa);

  BOOST_CHECK(jProtoAxis.contains("axis"));
  BOOST_CHECK(jProtoAxis.contains("axis_dir"));
  BOOST_CHECK(jProtoAxis.contains("autorange"));

  Acts::ProtoAxis epaRead = Acts::ProtoAxisJsonConverter::fromJson(jProtoAxis);

  BOOST_CHECK_EQUAL(epaRead.getAxisDirection(), epa.getAxisDirection());
  BOOST_CHECK_EQUAL(epaRead.getAxis(), epa.getAxis());
  BOOST_CHECK_EQUAL(epaRead.isAutorange(), epa.isAutorange());
  BOOST_CHECK_EQUAL(epaRead.toString(), epa.toString());
}

BOOST_AUTO_TEST_CASE(VariableProtoAxisJsonConversion) {
  using enum Acts::AxisBoundaryType;
  using enum Acts::AxisDirection;
  using enum Acts::AxisType;

  // Bound, variable axis
  Acts::ProtoAxis vpab(AxisX, Bound, {0.0, 1.0, 10});

  nlohmann::json jProtoAxis = Acts::ProtoAxisJsonConverter::toJson(vpab);
  BOOST_CHECK(jProtoAxis.contains("axis"));
  BOOST_CHECK(jProtoAxis.contains("axis_dir"));
  BOOST_CHECK(jProtoAxis.contains("autorange"));

  Acts::ProtoAxis vpabRead = Acts::ProtoAxisJsonConverter::fromJson(jProtoAxis);

  BOOST_CHECK_EQUAL(vpabRead.getAxisDirection(), vpab.getAxisDirection());
  BOOST_CHECK_EQUAL(vpabRead.getAxis(), vpab.getAxis());
  BOOST_CHECK_EQUAL(vpabRead.isAutorange(), vpab.isAutorange());
  BOOST_CHECK_EQUAL(vpabRead.toString(), vpab.toString());
}

BOOST_AUTO_TEST_CASE(InvalidAndValidInputJson) {
  // valid eq axis input
  nlohmann::json jValidEqAxis = {{"bins", 10},
                                 {"boundary_type", "Bound"},
                                 {"range", std::array<double, 2>{0.0, 1.0}},
                                 {"type", "Equidistant"}};

  // Valid input first
  nlohmann::json jValidEq = {
      {"axis", jValidEqAxis}, {"axis_dir", "AxisX"}, {"autorange", false}};

  BOOST_CHECK_NO_THROW(Acts::ProtoAxisJsonConverter::fromJson(jValidEq));

  // Invalid input - zero bins
  nlohmann::json jInvalidEqAxis = jValidEqAxis;
  jInvalidEqAxis["bins"] = 0;

  nlohmann::json jInvalidEq = {
      {"axis", jInvalidEqAxis}, {"axis_dir", "AxisX"}, {"autorange", false}};

  BOOST_CHECK_THROW(Acts::ProtoAxisJsonConverter::fromJson(jInvalidEq),
                    std::invalid_argument);

  // Invalid input - auto range without bins
  jInvalidEq = {
      {"axis", jInvalidEqAxis}, {"axis_dir", "AxisX"}, {"autorange", true}};
  BOOST_CHECK_THROW(Acts::ProtoAxisJsonConverter::fromJson(jInvalidEq),
                    std::invalid_argument);

  // Invalid input - min >= max
  jInvalidEqAxis = jValidEqAxis;
  jInvalidEqAxis["range"] = std::array<double, 2>{1.0, 0.0};

  jInvalidEq = {
      {"axis", jInvalidEqAxis}, {"axis_dir", "AxisX"}, {"autorange", false}};

  BOOST_CHECK_THROW(Acts::ProtoAxisJsonConverter::fromJson(jInvalidEq),
                    std::invalid_argument);

  nlohmann::json jValidVarAxis = {
      {"boundary_type", "Bound"},
      {"boundaries", std::vector<double>{0.0, 0.25, 0.75, 1.0}},
      {"type", "Variable"}};

  // Valid input first
  nlohmann::json jValidVar = {
      {"axis", jValidVarAxis}, {"axis_dir", "AxisX"}, {"autorange", false}};
  BOOST_CHECK_NO_THROW(Acts::ProtoAxisJsonConverter::fromJson(jValidVar));

  // Invalid input - less than two edges
  nlohmann::json jInvalidVarAxis = jValidVarAxis;
  jInvalidVarAxis["boundaries"] = std::vector<double>{0.0};

  nlohmann::json jInvalidVar = {
      {"axis", jInvalidVarAxis}, {"axis_dir", "AxisX"}, {"autorange", false}};
  BOOST_CHECK_THROW(Acts::ProtoAxisJsonConverter::fromJson(jInvalidVar),
                    std::invalid_argument);

  // Invalid input - non-increasing edges
  jInvalidVarAxis = jValidVarAxis;
  jInvalidVarAxis["boundaries"] = std::vector<double>{0.0, 0.75, 0.25, 1.0};

  jInvalidVar = {
      {"axis", jInvalidVarAxis}, {"axis_dir", "AxisX"}, {"autorange", false}};

  BOOST_CHECK_THROW(Acts::ProtoAxisJsonConverter::fromJson(jInvalidVar),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

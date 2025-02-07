// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/KDTree.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace {
std::vector<std::pair<std::array<double, 3>, int>> test_vector{
    {{-8.5, 2.3, -1.6}, 84},   {{-9.7, -1.4, 7.6}, -56},
    {{6.4, 2.7, -1.0}, -94},   {{-2.2, -9.3, 3.6}, 56},
    {{-9.2, -2.8, -2.5}, -90}, {{3.8, 0.9, 7.2}, 43},
    {{7.6, 1.9, -1.0}, 83},    {{2.1, -1.6, -1.7}, -15},
    {{5.2, 3.2, -1.3}, 14},    {{-6.0, -7.5, 6.5}, 48},
    {{3.5, -7.1, -4.4}, 83},   {{5.0, 6.9, -0.7}, 1},
    {{8.0, 0.2, 3.8}, 69},     {{2.5, 0.8, 4.1}, -11},
    {{-1.5, -6.6, 8.4}, -98},  {{0.8, 6.1, 2.7}, 14},
    {{5.1, -5.1, 5.8}, 46},    {{-9.6, 0.7, 0.3}, 59},
    {{-8.8, 1.3, -9.8}, -19},  {{-7.5, -9.3, -2.2}, 6},
    {{-5.3, 1.9, -3.9}, 57},   {{-4.8, -9.8, -0.2}, 73},
    {{-6.3, -7.4, 9.2}, -94},  {{-4.8, -9.8, -2.6}, 77},
    {{2.4, 4.9, -8.4}, -72},   {{8.4, 6.1, -7.7}, -66},
    {{6.1, -8.2, 9.3}, 67},    {{9.4, 7.4, 7.5}, -99},
    {{9.4, 1.8, -4.4}, -87},   {{9.0, -3.7, 6.3}, -12},
    {{-1.8, 2.8, 6.3}, 71},    {{-7.6, 7.6, -4.5}, -77},
    {{-7.7, -1.2, 7.7}, -92},  {{-6.9, -0.5, -6.6}, -61},
    {{0.9, 5.7, 0.5}, 60},     {{-0.3, 3.7, -7.0}, 15},
    {{5.3, -0.6, -6.6}, 46},   {{-6.9, -3.0, 4.1}, -13},
    {{1.8, -6.5, 1.8}, -29},   {{1.4, -7.8, -0.3}, 100},
    {{8.2, 0.1, -7.6}, 69},    {{6.5, -9.1, 6.6}, -61},
    {{4.2, 6.6, -0.7}, 56},    {{8.6, -3.1, -6.8}, -88},
    {{9.7, -6.0, 2.9}, -44},   {{7.0, -3.2, 9.8}, 29},
    {{-6.1, 3.3, -3.0}, 54},   {{-2.6, 7.6, -4.2}, -52},
    {{-5.9, 7.8, 5.1}, -81},   {{0.6, 5.3, -2.5}, -69},
    {{-8.5, 1.8, 6.0}, 56},    {{6.4, 2.0, -6.8}, -14},
    {{-9.0, -2.1, 8.0}, 84},   {{7.0, 0.1, 6.3}, -17},
    {{8.0, 4.5, -0.8}, -85},   {{-5.5, 9.2, 7.5}, 13},
    {{-3.8, 2.1, 3.4}, 19},    {{-8.0, -5.2, 9.5}, -25},
    {{6.2, 6.0, -2.2}, 81},    {{5.1, -5.1, -9.7}, 74},
    {{-1.3, -9.0, 8.1}, -74},  {{5.4, -0.8, 2.4}, 32},
    {{-5.6, 3.8, 6.2}, 73},    {{8.9, -3.8, -7.8}, 93},
    {{7.7, 0.4, -2.9}, 78},    {{-6.9, -1.5, -3.3}, -75},
    {{-5.3, 2.5, -3.6}, 48},   {{-6.6, -9.2, -2.1}, -7},
    {{5.5, -7.7, 2.0}, 3},     {{4.1, -9.5, -6.9}, 82},
    {{-2.3, -0.2, -0.7}, -3},  {{2.3, 0.4, -5.5}, 43},
    {{5.0, 4.0, 9.0}, 59},     {{-8.7, -4.0, 1.0}, -17},
    {{7.6, -7.6, -7.9}, 18},   {{4.2, 6.3, -4.4}, 2},
    {{8.6, -6.3, 3.5}, -75},   {{9.8, 7.3, 0.1}, 8},
    {{-2.2, 9.3, -6.2}, -54},  {{-0.9, -0.4, 1.4}, 45},
    {{-5.3, 6.7, 7.6}, 20},    {{-2.7, 2.4, 8.8}, 42},
    {{9.3, -0.0, 9.1}, -84},   {{0.5, 1.5, -2.3}, -47},
    {{7.5, -5.7, 1.3}, 21},    {{0.4, 0.4, -2.0}, 50},
    {{0.8, -6.2, -7.7}, 46},   {{5.1, -7.3, -0.7}, 26},
    {{9.7, 2.8, 9.6}, 80},     {{7.3, -2.1, -6.7}, -91},
    {{-9.4, -5.3, -3.1}, -24}, {{-2.4, -1.6, -0.2}, -88},
    {{-9.9, -5.9, 0.0}, -90},  {{1.3, -3.0, 9.8}, 72},
    {{0.9, -6.8, -2.7}, 92},   {{-1.7, -3.8, 2.8}, -78},
    {{-6.4, -0.6, -0.6}, 95},  {{-4.7, 4.8, -8.0}, 95},
    {{-6.0, 3.5, -7.4}, 7},    {{3.2, -6.2, 3.9}, -25}};
}

namespace Acts::Test {

struct TreeFixture1DDoubleInt1 {
  TreeFixture1DDoubleInt1()
      : tree(std::vector<std::pair<std::array<double, 1>, int>>{
            {{1.0}, 5}, {{2.0}, 6}, {{-1.2}, 2}, {{0.9}, 10}}) {}

  Acts::KDTree<1, int, double> tree;
};

struct TreeFixture1DDoubleInt2 {
  TreeFixture1DDoubleInt2()
      : tree(std::vector<std::pair<std::array<double, 1>, int>>{
            {{1.0}, 5},
            {{2.0}, 6},
            {{-1.2}, 2},
            {{0.9}, 10},
            {{-10.0}, 9},
            {{110.0}, 1},
            {{-1000.0}, 3}}) {}

  Acts::KDTree<1, int, double> tree;
};

struct TreeFixture2DDoubleInt1 {
  TreeFixture2DDoubleInt1()
      : tree(std::vector<std::pair<std::array<double, 2>, int>>{
            {{1.0, 5.0}, 5}, {{2.0, -2.5}, 6}}) {}

  Acts::KDTree<2, int, double> tree;
};

struct TreeFixture3DDoubleInt1 {
  TreeFixture3DDoubleInt1()
      : tree(std::vector<std::pair<std::array<double, 3>, int>>{
            {{-4.7, -2.0, -1.7}, 63}, {{8.2, -0.0, 9.5}, -82},
            {{7.1, -3.6, -4.0}, -49}, {{-9.9, -4.6, 2.9}, 86},
            {{5.0, -3.8, -2.8}, -12}, {{-4.8, -1.8, -1.6}, -60},
            {{8.8, 9.6, -0.2}, 60},   {{7.6, 1.9, 8.8}, 74},
            {{9.0, -6.9, -6.2}, 35},  {{4.3, 7.5, -2.0}, 19},
            {{-7.7, 3.7, 7.1}, -54},  {{3.4, 7.4, -4.0}, -8},
            {{8.5, 3.0, -3.2}, -48},  {{6.5, -0.3, -3.1}, -25},
            {{6.9, -6.6, -8.0}, -4},  {{6.8, 5.5, -2.5}, 17},
            {{-2.8, 8.8, -7.2}, 62},  {{-0.1, 3.5, 5.5}, -95},
            {{-1.3, 6.9, 5.3}, -23},  {{6.2, 6.6, 7.1}, -84}}) {}

  Acts::KDTree<3, int, double> tree;
};

struct TreeFixture3DDoubleInt2 {
  TreeFixture3DDoubleInt2()
      : tree(std::vector<std::pair<std::array<double, 3>, int>>(test_vector)) {}

  Acts::KDTree<3, int, double> tree;
};

struct TreeFixture3DDoubleInt3 {
  TreeFixture3DDoubleInt3()
      : tree(std::vector<std::pair<std::array<double, 3>, int>>{
            {{-100.0, -1.1, -1.7}, 0},
            {{100.0, -0.0, 9.5}, 4},
            {{-100.0, -0.6, -1.0}, 1},
            {{100.0, -4.6, 2.9}, 5},
            {{-100.0, -1.4, -2.8}, 2},
            {{100.0, -1.8, -1.6}, 6},
            {{-100.0, -1.0, -0.2}, 3},
            {{100.0, 1.9, 8.8}, 7}}) {}

  Acts::KDTree<3, int, double> tree;
};

struct TreeFixture10DDoubleInt1 {
  TreeFixture10DDoubleInt1()
      : tree(std::vector<std::pair<std::array<double, 10>, int>>{
            {{5.6, 7.5, -9.8, 9.6, 3.3, -7.3, 2.0, 4.7, -2.1, 5.9}, 32},
            {{1.2, -1.5, -0.2, 0.9, 1.1, -1.4, -8.9, -1.2, -9.0, 7.4}, -66},
            {{1.6, 6.2, 9.9, -2.5, 4.0, 8.9, 3.9, 8.4, -1.2, -4.1}, -51},
            {{0.7, -5.7, -3.1, 5.4, 0.6, -8.7, 0.2, -0.2, 0.3, -2.7}, -19},
            {{-5.0, -5.5, -7.1, 9.2, 5.5, 0.7, 9.9, -7.0, -6.8, 3.1}, 27},
            {{0.4, 5.5, -5.8, -3.0, 6.0, -7.3, 2.1, 7.9, -8.0, -6.9}, -13},
            {{-2.3, -5.9, 9.0, 1.4, 4.6, 3.4, -9.5, -6.3, 3.3, -7.0}, 97},
            {{8.1, 3.6, -8.6, -0.4, 7.5, 10.0, -3.9, -5.8, -2.9, 9.8}, 15},
            {{8.4, -4.0, 6.3, 1.1, -5.7, 8.1, -8.0, -2.5, -0.5, 3.2}, -56},
            {{2.3, 5.8, 1.4, 4.0, 9.0, -6.4, 1.0, -7.8, 4.3, -5.3}, -83}}) {}

  Acts::KDTree<10, int, double> tree;
};

struct TreeFixture3DDoubleString1 {
  TreeFixture3DDoubleString1()
      : tree(std::vector<std::pair<std::array<double, 3>, std::string>>{
            {{-0.2, -0.2, -3.8}, "string0"},
            {{4.9, -7.8, -10.0}, "string1"},
            {{3.5, -10.0, -8.1}, "string2"},
            {{8.2, -6.1, 2.0}, "string3"},
            {{-9.9, 7.7, -1.4}, "string4"},
            {{-2.2, 2.1, 8.6}, "string5"},
            {{-6.9, -5.8, 5.3}, "string6"},
            {{-0.7, 0.9, -6.5}, "string7"},
            {{-2.7, -5.9, -7.3}, "string8"},
            {{3.1, -9.4, -2.5}, "string9"}}) {}

  Acts::KDTree<3, std::string, double> tree;
};

struct TreeFixture1DIntInt1 {
  TreeFixture1DIntInt1()
      : tree(std::vector<std::pair<std::array<int, 1>, int>>{
            {{1}, 5}, {{2}, 6}, {{-1}, 2}, {{5}, 10}}) {}

  Acts::KDTree<1, int, int> tree;
};

struct TreeFixture2DIntInt1 {
  TreeFixture2DIntInt1()
      : tree(std::vector<std::pair<std::array<int, 2>, int>>{
            {{1, 7}, 5}, {{2, 1}, 6}, {{-1, -11}, 2}, {{5, -2}, 10}}) {}

  Acts::KDTree<2, int, int> tree;
};

BOOST_AUTO_TEST_SUITE(Utilities)

BOOST_AUTO_TEST_SUITE(KDTree)

BOOST_FIXTURE_TEST_CASE(size_1, TreeFixture1DDoubleInt1) {
  BOOST_CHECK_EQUAL(tree.size(), 4);
}

BOOST_FIXTURE_TEST_CASE(size_2, TreeFixture1DDoubleInt2) {
  BOOST_CHECK_EQUAL(tree.size(), 7);
}

BOOST_FIXTURE_TEST_CASE(size_3, TreeFixture2DDoubleInt1) {
  BOOST_CHECK_EQUAL(tree.size(), 2);
}

BOOST_FIXTURE_TEST_CASE(size_4, TreeFixture3DDoubleInt1) {
  BOOST_CHECK_EQUAL(tree.size(), 20);
}

BOOST_FIXTURE_TEST_CASE(size_5, TreeFixture10DDoubleInt1) {
  BOOST_CHECK_EQUAL(tree.size(), 10);
}

BOOST_FIXTURE_TEST_CASE(size_6, TreeFixture3DDoubleString1) {
  BOOST_CHECK_EQUAL(tree.size(), 10);
}

BOOST_FIXTURE_TEST_CASE(size_7, TreeFixture1DIntInt1) {
  BOOST_CHECK_EQUAL(tree.size(), 4);
}

BOOST_FIXTURE_TEST_CASE(size_8, TreeFixture3DDoubleInt2) {
  BOOST_CHECK_EQUAL(tree.size(), 100);
}

BOOST_FIXTURE_TEST_CASE(range_search_1, TreeFixture1DDoubleInt2) {
  RangeXD<1, double> range;
  range[0].shrink(0.0, 2.5);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 3);
  BOOST_CHECK(rangeContainsValue(result, 5));
  BOOST_CHECK(rangeContainsValue(result, 6));
  BOOST_CHECK(rangeContainsValue(result, 10));
  BOOST_CHECK(!rangeContainsValue(result, 7));
  BOOST_CHECK(!rangeContainsValue(result, 2));
  BOOST_CHECK(!rangeContainsValue(result, 9));
}

BOOST_FIXTURE_TEST_CASE(range_search_2, TreeFixture1DDoubleInt2) {
  RangeXD<1, double> range;
  range[0].shrink(-10000.0, 10000.0);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 7);
  BOOST_CHECK(rangeContainsValue(result, 1));
  BOOST_CHECK(rangeContainsValue(result, 2));
  BOOST_CHECK(rangeContainsValue(result, 3));
  BOOST_CHECK(rangeContainsValue(result, 5));
  BOOST_CHECK(rangeContainsValue(result, 6));
  BOOST_CHECK(rangeContainsValue(result, 9));
  BOOST_CHECK(rangeContainsValue(result, 10));
}

BOOST_FIXTURE_TEST_CASE(range_search_3, TreeFixture1DDoubleInt2) {
  RangeXD<1, double> range;
  range[0].shrink(5000.0, 10000.0);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 0);
}

BOOST_FIXTURE_TEST_CASE(range_search_4, TreeFixture2DDoubleInt1) {
  RangeXD<2, double> range;
  range[0].shrink(0.0, 10.0);
  range[1].shrink(0.0, 10.0);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 1);
  BOOST_CHECK(rangeContainsValue(result, 5));
}

BOOST_FIXTURE_TEST_CASE(range_search_5, TreeFixture2DDoubleInt1) {
  RangeXD<2, double> range;
  range[0].shrink(0.0, 10.0);
  range[1].shrink(-10.0, 10.0);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 2);
  BOOST_CHECK(rangeContainsValue(result, 5));
  BOOST_CHECK(rangeContainsValue(result, 6));
}

BOOST_FIXTURE_TEST_CASE(range_search_6, TreeFixture10DDoubleInt1) {
  RangeXD<10, double> range;
  range[0].shrink(0.0, 5.0);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 5);
  BOOST_CHECK(rangeContainsValue(result, -66));
  BOOST_CHECK(rangeContainsValue(result, -51));
  BOOST_CHECK(rangeContainsValue(result, -19));
  BOOST_CHECK(rangeContainsValue(result, -13));
  BOOST_CHECK(rangeContainsValue(result, -83));
}

BOOST_FIXTURE_TEST_CASE(range_search_7, TreeFixture10DDoubleInt1) {
  RangeXD<10, double> range;
  range[9].shrink(0.0, 5.0);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 2);
  BOOST_CHECK(rangeContainsValue(result, 27));
  BOOST_CHECK(rangeContainsValue(result, -56));
}

BOOST_FIXTURE_TEST_CASE(range_search_8, TreeFixture3DDoubleString1) {
  RangeXD<3, double> range;
  range[0].shrink(-5.0, 5.0);
  range[1].shrink(-5.0, 5.0);
  range[2].shrink(-5.0, 5.0);

  std::vector<std::string> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 1);
  BOOST_CHECK(rangeContainsValue(result, "string0"));
}

BOOST_FIXTURE_TEST_CASE(range_search_9, TreeFixture3DDoubleString1) {
  RangeXD<3, double> range;
  range[0].shrink(-10.0, 10.0);
  range[1].shrink(-10.0, 10.0);
  range[2].shrink(-5.0, 5.0);

  std::vector<std::string> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 4);
  BOOST_CHECK(rangeContainsValue(result, "string0"));
  BOOST_CHECK(rangeContainsValue(result, "string3"));
  BOOST_CHECK(rangeContainsValue(result, "string4"));
  BOOST_CHECK(rangeContainsValue(result, "string9"));
}

BOOST_FIXTURE_TEST_CASE(range_search_10, TreeFixture1DIntInt1) {
  RangeXD<1, int> range;
  range[0].shrink(0, 3);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 2);
  BOOST_CHECK(rangeContainsValue(result, 5));
  BOOST_CHECK(rangeContainsValue(result, 6));
}

BOOST_FIXTURE_TEST_CASE(range_search_11, TreeFixture2DIntInt1) {
  RangeXD<2, int> range;
  range[1].shrink(0, 10);

  std::vector<int> result = tree.rangeSearch(range);
  BOOST_CHECK_EQUAL(result.size(), 2);
  BOOST_CHECK(rangeContainsValue(result, 5));
  BOOST_CHECK(rangeContainsValue(result, 6));
}

BOOST_FIXTURE_TEST_CASE(range_search_inplace_1, TreeFixture10DDoubleInt1) {
  RangeXD<10, double> range;
  range[0].shrink(0.0, 5.0);

  std::vector<int> result;
  tree.rangeSearch(range, result);

  BOOST_CHECK_EQUAL(result.size(), 5);
  BOOST_CHECK(rangeContainsValue(result, -66));
  BOOST_CHECK(rangeContainsValue(result, -51));
  BOOST_CHECK(rangeContainsValue(result, -19));
  BOOST_CHECK(rangeContainsValue(result, -13));
  BOOST_CHECK(rangeContainsValue(result, -83));
}

BOOST_FIXTURE_TEST_CASE(range_search_inserter_1, TreeFixture10DDoubleInt1) {
  RangeXD<10, double> range;
  range[0].shrink(0.0, 5.0);

  std::vector<int> result;
  tree.rangeSearchInserter(range, std::back_inserter(result));

  BOOST_CHECK_EQUAL(result.size(), 5);
  BOOST_CHECK(rangeContainsValue(result, -66));
  BOOST_CHECK(rangeContainsValue(result, -51));
  BOOST_CHECK(rangeContainsValue(result, -19));
  BOOST_CHECK(rangeContainsValue(result, -13));
  BOOST_CHECK(rangeContainsValue(result, -83));
}

BOOST_FIXTURE_TEST_CASE(range_search_map_1, TreeFixture10DDoubleInt1) {
  RangeXD<10, double> range;
  range[0].shrink(0.0, 5.0);

  std::vector<int> result = tree.rangeSearchMap<int>(
      range,
      [](const std::array<double, 10>&, const int& i) -> int { return 2 * i; });

  BOOST_CHECK_EQUAL(result.size(), 5);
  BOOST_CHECK(rangeContainsValue(result, -132));
  BOOST_CHECK(rangeContainsValue(result, -102));
  BOOST_CHECK(rangeContainsValue(result, -38));
  BOOST_CHECK(rangeContainsValue(result, -26));
  BOOST_CHECK(rangeContainsValue(result, -166));
}

BOOST_FIXTURE_TEST_CASE(range_search_map_inserter_1, TreeFixture10DDoubleInt1) {
  RangeXD<10, double> range;
  range[0].shrink(0.0, 5.0);

  std::vector<std::string> result;

  tree.rangeSearchMapInserter<std::string>(
      range,
      [](const std::array<double, 10>&, const int& i) -> std::string {
        return std::to_string(i);
      },
      std::back_inserter(result));

  BOOST_CHECK_EQUAL(result.size(), 5);
  BOOST_CHECK(rangeContainsValue(result, "-66"));
  BOOST_CHECK(rangeContainsValue(result, "-51"));
  BOOST_CHECK(rangeContainsValue(result, "-19"));
  BOOST_CHECK(rangeContainsValue(result, "-13"));
  BOOST_CHECK(rangeContainsValue(result, "-83"));
}

BOOST_FIXTURE_TEST_CASE(range_search_map_inserter_2, TreeFixture2DIntInt1) {
  RangeXD<2, int> range;
  range[1].shrink(0, 10);

  std::vector<int> result;
  tree.rangeSearchMapInserter<int>(
      range,
      [](const std::array<int, 2>& c, const int& i) -> int {
        return i * (c[0] + c[1]);
      },
      std::back_inserter(result));

  BOOST_CHECK_EQUAL(result.size(), 2);
  BOOST_CHECK(rangeContainsValue(result, 40));
  BOOST_CHECK(rangeContainsValue(result, 18));
}

BOOST_FIXTURE_TEST_CASE(range_search_map_discard_1, TreeFixture2DIntInt1) {
  RangeXD<2, int> range;
  range[1].shrink(-100, 5);

  int result = 0;

  tree.rangeSearchMapDiscard(
      range, [&result](const std::array<int, 2>& c, const int& i) {
        result += i * (c[0] + c[1]);
      });

  BOOST_CHECK_EQUAL(result, 24);
}

BOOST_FIXTURE_TEST_CASE(range_search_map_discard_2, TreeFixture3DDoubleInt2) {
  RangeXD<3, double> range;
  range[0].shrinkMin(0.0);

  int result = 0;

  tree.rangeSearchMapDiscard(range, [&result](const std::array<double, 3>&,
                                              const int& i) { result += i; });

  BOOST_CHECK_EQUAL(result, 555);
}

BOOST_FIXTURE_TEST_CASE(range_search_combinatorial, TreeFixture3DDoubleInt2) {
  for (double xmin = -10.0; xmin <= 10.0; xmin += 2.0) {
    for (double xmax = -10.0; xmax <= 10.0; xmax += 2.0) {
      for (double ymin = -10.0; ymin <= 10.0; ymin += 2.0) {
        for (double ymax = -10.0; ymax <= 10.0; ymax += 2.0) {
          for (double zmin = -10.0; zmin <= 10.0; zmin += 2.0) {
            for (double zmax = -10.0; zmax <= 10.0; zmax += 2.0) {
              RangeXD<3, double> range;

              range[0].shrink(xmin, xmax);
              range[1].shrink(ymin, ymax);
              range[2].shrink(zmin, zmax);

              std::vector<int> valid;

              for (const auto& [c, value] : test_vector) {
                if (xmin <= c[0] && c[0] < xmax && ymin <= c[1] &&
                    c[1] < ymax && zmin <= c[2] && c[2] < zmax) {
                  valid.push_back(value);
                }
              }

              std::vector<int> result = tree.rangeSearch(range);

              BOOST_CHECK_EQUAL(result.size(), valid.size());

              for (int j : valid) {
                BOOST_CHECK(rangeContainsValue(result, j));
              }
            }
          }
        }
      }
    }
  }
}

BOOST_FIXTURE_TEST_CASE(range_search_dominate1, TreeFixture3DDoubleInt3) {
  RangeXD<3, double> range1;
  range1[0].shrink(-200, 0);

  std::vector<int> result = tree.rangeSearch(range1);
  BOOST_CHECK_EQUAL(result.size(), 4);
  BOOST_CHECK(rangeContainsValue(result, 0));
  BOOST_CHECK(rangeContainsValue(result, 1));
  BOOST_CHECK(rangeContainsValue(result, 2));
  BOOST_CHECK(rangeContainsValue(result, 3));
}

BOOST_FIXTURE_TEST_CASE(range_search_dominate2, TreeFixture3DDoubleInt3) {
  RangeXD<3, double> range1;
  range1[0].shrink(0, 200);

  std::vector<int> result = tree.rangeSearch(range1);
  BOOST_CHECK_EQUAL(result.size(), 4);
  BOOST_CHECK(rangeContainsValue(result, 4));
  BOOST_CHECK(rangeContainsValue(result, 5));
  BOOST_CHECK(rangeContainsValue(result, 6));
  BOOST_CHECK(rangeContainsValue(result, 7));
}

BOOST_AUTO_TEST_CASE(range_search_very_big) {
  int q = 0;

  std::vector<std::pair<std::array<double, 3>, int>> points;

  for (double x = -10.0; x < 10.0; x += 0.5) {
    for (double y = -10.0; y < 10.0; y += 0.5) {
      for (double z = -10.0; z < 10.0; z += 0.5) {
        points.push_back({{x, y, z}, q++});
      }
    }
  }

  std::vector<std::pair<std::array<double, 3>, int>> copy(points);

  Acts::KDTree<3, int, double> tree(std::move(copy));

  for (double xmin = -10.0; xmin <= 10.0; xmin += 1.0) {
    for (double ymin = -10.0; ymin <= 10.0; ymin += 1.0) {
      for (double zmin = -10.0; zmin <= 10.0; zmin += 1.0) {
        RangeXD<3, double> range;

        double xmax = xmin + 1.0;
        double ymax = ymin + 1.0;
        double zmax = zmin + 1.0;

        range[0].shrink(xmin, xmax);
        range[1].shrink(ymin, ymax);
        range[2].shrink(zmin, zmax);

        std::vector<int> valid;

        for (const std::pair<std::array<double, 3>, int>& i : points) {
          const std::array<double, 3>& c = i.first;
          if (xmin <= c[0] && c[0] < xmax && ymin <= c[1] && c[1] < ymax &&
              zmin <= c[2] && c[2] < zmax) {
            valid.push_back(i.second);
          }
        }

        std::vector<int> result = tree.rangeSearch(range);

        BOOST_CHECK_EQUAL(result.size(), valid.size());

        for (int j : valid) {
          BOOST_CHECK(rangeContainsValue(result, j));
        }
      }
    }
  }
}

BOOST_AUTO_TEST_CASE(range_search_many_same) {
  int q = 0;

  std::vector<std::pair<std::array<double, 3>, int>> points;

  for (std::size_t i = 0; i < 50; ++i) {
    points.push_back({{64.0, 64.0, 64.0}, q++});
  }

  for (std::size_t i = 0; i < 50; ++i) {
    points.push_back({{-64.0, -64.0, -64.0}, q++});
  }

  Acts::KDTree<3, int, double> tree(std::move(points));

  RangeXD<3, double> range1;
  range1[0].shrink(50.0, 70.0);
  range1[1].shrink(50.0, 70.0);
  range1[2].shrink(50.0, 70.0);

  RangeXD<3, double> range2;
  range2[0].shrink(-70.0, -50.0);
  range2[1].shrink(-70.0, -50.0);
  range2[2].shrink(-70.0, -50.0);

  std::vector<int> result1 = tree.rangeSearch(range1);
  std::vector<int> result2 = tree.rangeSearch(range2);

  BOOST_CHECK_EQUAL(result1.size(), 50);
  BOOST_CHECK_EQUAL(result2.size(), 50);

  for (int i = 0; i < 50; ++i) {
    BOOST_CHECK(rangeContainsValue(result1, i));
  }

  for (int i = 50; i < 100; ++i) {
    BOOST_CHECK(rangeContainsValue(result2, i));
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Acts::Test

// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Json/GeometryHierarchyMapJsonConverter.hpp"
#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

using Acts::GeometryIdentifier;
using nlohmann::json;

// helper function to create geometry ids.
GeometryIdentifier makeId(int volume = 0, int layer = 0, int sensitive = 0) {
  return GeometryIdentifier().setVolume(volume).setLayer(layer).setSensitive(
      sensitive);
}

// example element for the container

struct Thing {
  double x = 1.0;
  int y = 23;

  friend constexpr bool operator==(const Thing& lhs, const Thing& rhs) {
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
  }
};

// custom Json encoder/decoder. naming is mandated by nlohmann::json and thus
// can not match our naming guidelines.

void to_json(json& j, const Thing& t) {
  j = json{{"x", t.x}, {"y", t.y}};
}

void from_json(const json& j, Thing& t) {
  j.at("x").get_to(t.x);
  j.at("y").get_to(t.y);
}

std::ostream& operator<<(std::ostream& os, const Thing& t) {
  os << nlohmann::json(t);
  return os;
}

class ThingDecorator {
 public:
  void decorate(const Thing* a_thing, nlohmann::json& a_json) const {
    if (a_thing != nullptr) {
      a_json["product"] = a_thing->x * a_thing->y;
    }
  }
};

using Container = Acts::GeometryHierarchyMap<Thing>;
using Converter =
    Acts::GeometryHierarchyMapJsonConverter<Thing, ThingDecorator>;

}  // namespace

template <>
void Acts::decorateJson<Thing>(const ThingDecorator* decorator,
                               const Thing& src, nlohmann::json& dest) {
  if (decorator != nullptr) {
    decorator->decorate(&src, dest);
  }
}

BOOST_TEST_DONT_PRINT_LOG_VALUE(json::iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(Container::Iterator)

BOOST_AUTO_TEST_SUITE(GeometryHierarchyMapJsonConverter)

BOOST_AUTO_TEST_CASE(ToJson) {
  ThingDecorator decorator;
  Container c = {
      {makeId(1), {2.0, -3}},
      {makeId(2, 3), {-4.5, 5}},
      {makeId(4, 5, 6), {7.25, -8}},
  };
  json j = Converter("thing").toJson(c, &decorator);

  BOOST_CHECK(j.is_object());
  // check header
  auto header = j.find("acts-geometry-hierarchy-map");
  BOOST_CHECK_NE(header, j.end());
  BOOST_CHECK(header->is_object());
  BOOST_CHECK(header->at("format-version").is_number_integer());
  BOOST_CHECK_EQUAL(header->at("format-version").get<int>(), 0);
  BOOST_CHECK(header->at("value-identifier").is_string());
  BOOST_CHECK_EQUAL(header->at("value-identifier").get<std::string>(), "thing");
  // check entries
  auto entries = j.find("entries");
  BOOST_CHECK_NE(entries, j.end());
  BOOST_CHECK(entries->is_array());
  BOOST_CHECK_EQUAL(entries->size(), 3u);
}

BOOST_AUTO_TEST_CASE(FromJson) {
  json j = {
      {
          "acts-geometry-hierarchy-map",
          {
              {"format-version", 0},
              {"value-identifier", "thing"},
          },
      },
      {
          "entries",
          {
              {
                  {"volume", 2},
                  {"layer", 3},
                  {"value", {{"x", 4.0}, {"y", 4}}},
              },
              {
                  {"volume", 5},
                  {"layer", 6},
                  {"sensitive", 7},
                  {"value", {{"x", 3.0}, {"y", 3}}},
              },
          },
      },
  };
  Container c = Converter("thing").fromJson(j);

  BOOST_CHECK(!c.empty());
  BOOST_CHECK_EQUAL(c.size(), 2);
  {
    auto it = c.find(makeId(2, 3));
    BOOST_CHECK_NE(it, c.end());
    BOOST_CHECK_EQUAL(it->x, 4.0);
    BOOST_CHECK_EQUAL(it->y, 4);
  }
  {
    auto it = c.find(makeId(5, 6, 7));
    BOOST_CHECK_NE(it, c.end());
    BOOST_CHECK_EQUAL(it->x, 3.0);
    BOOST_CHECK_EQUAL(it->y, 3);
  }
}

BOOST_AUTO_TEST_CASE(FromJsonMissingHeader) {
  json j = {
      {"entries", {}},
  };
  BOOST_CHECK_THROW(Converter("an-identifier").fromJson(j),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(FromJsonInvalidFormatVersion) {
  json j = {
      {
          "acts-geometry-hierarchy-map",
          {
              {"format-version", -1},
              {"value-identifier", "an-identifier"},
          },
      },
      {"entries", {}},
  };
  BOOST_CHECK_THROW(Converter("an-identifier").fromJson(j),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(FromJsonInvalidValueIdentifier) {
  json j = {
      {
          "acts-geometry-hierarchy-map",
          {
              {"format-version", 0},
              {"value-identifier", "an-identifier"},
          },
      },
      {"entries", {}},
  };
  BOOST_CHECK_THROW(Converter("not-the-identifier").fromJson(j),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(FromJsonMissingEntries) {
  json j = {
      {
          "acts-geometry-hierarchy-map",
          {
              {"format-version", 0},
              {"value-identifier", "an-identifier"},
          },
      },
  };
  BOOST_CHECK_THROW(Converter("an-identifier").fromJson(j),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Roundtrip) {
  ThingDecorator decorator;
  Container c0 = {
      {makeId(1), {2.0, -3}},
      {makeId(2, 3), {-4.5, 5}},
      {makeId(4, 5, 6), {7.25, -8}},
  };
  auto j = Converter("the-identifier").toJson(c0, &decorator);
  auto c1 = Converter("the-identifier").fromJson(j);

  BOOST_CHECK_EQUAL(c0.size(), c1.size());
  for (auto i = std::min(c0.size(), c1.size()); 0 < i--;) {
    BOOST_CHECK_EQUAL(c0.idAt(i), c1.idAt(i));
    BOOST_CHECK_EQUAL(c0.valueAt(i), c1.valueAt(i));
  }
}

BOOST_AUTO_TEST_CASE(FromFile) {
  // read json data from file
  auto path = Acts::Test::getDataPath("geometry-hierarchy-map.json");
  auto file = std::ifstream(path, std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(file.good());
  json j;
  file >> j;
  BOOST_CHECK(file.good());
  // convert json to container
  Container c = Converter("thing").fromJson(j);
  // check container content
  BOOST_CHECK(!c.empty());
  BOOST_CHECK_EQUAL(c.size(), 4);
  BOOST_CHECK_NE(c.find(makeId()), c.end());
  BOOST_CHECK_NE(c.find(makeId(1, 2)), c.end());
  BOOST_CHECK_NE(c.find(makeId(3)), c.end());
  BOOST_CHECK_NE(c.find(makeId(3, 4)), c.end());
}

BOOST_AUTO_TEST_SUITE_END()

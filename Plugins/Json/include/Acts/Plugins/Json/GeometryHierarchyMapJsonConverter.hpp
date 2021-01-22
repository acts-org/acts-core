// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryHierarchyMap.hpp"

#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace Acts {

/// Convert a geometry hierarchy map to/from Json.
///
/// @tparam value_t value type stored in the geometry hierarchy map
///
/// The value type is expected to be directly convertible to/from a Json object.
/// It has to be either a fundamental type or appropriate `to_json(json&, const
/// value_t&)` and `from_json(const json&, value_t&)` functions must be
/// available. See the relevant [nlohmann::json documentation][1] for further
/// information.
///
/// A user-defined identifier is stored in the encoded Json object that is used
/// to identify which value type is stored in the file. The identifier is
/// checked for consistency when decoding the Json object.
///
/// [1]: https://nlohmann.github.io/json/features/arbitrary_types/
template <typename value_t>
class GeometryHierarchyMapJsonConverter {
 public:
  using Value = value_t;
  using Container = GeometryHierarchyMap<value_t>;

  /// Construct the converter.
  ///
  /// @param valueIdentifier user-defined identifier for the stored value
  GeometryHierarchyMapJsonConverter(std::string valueIdentifier)
      : m_valueIdentifier(std::move(valueIdentifier)) {
    if (m_valueIdentifier.empty()) {
      throw std::invalid_argument("Value identifier must be non-empty");
    }
  }

  /// Encode the geometry hierarchy map into a Json object.
  ///
  /// @param container Geometry hierarchy map that should be encoded
  /// @return Encoded Json object
  nlohmann::json toJson(const Container& container) const;

  /// Decode a Json object into a geometry hierarchy map.
  ///
  /// @param encoded Json object that should be decoded
  /// @return Decoded geometry hierarchy map
  /// @throw std::invalid_argument in case of format errors
  Container fromJson(const nlohmann::json& encoded) const;

 private:
  static constexpr const char* const kHeaderKey = "acts-geometry-hierarchy-map";
  static constexpr const char* const kEntriesKey = "entries";
  /// The version of the encoded Json container format. This must be increased
  /// manually everytime the container format changes.
  static constexpr int kFormatVersion = 0;

  std::string m_valueIdentifier;

  static nlohmann::json encodeIdentifier(GeometryIdentifier id) {
    nlohmann::json encoded;
    // only store non-zero identifiers
    if (id.volume()) {
      encoded["Volume"] = id.volume();
    }
    if (id.boundary()) {
      encoded["_Boundary"] = id.boundary();
    }
    if (id.layer()) {
      encoded["_Layer"] = id.layer();
    }
    if (id.approach()) {
      encoded["__Approach"] = id.approach();
    }
    if (id.sensitive()) {
      encoded["__Sensitive"] = id.sensitive();
    }
    return encoded;
  }
  static GeometryIdentifier decodeIdentifier(const nlohmann::json& encoded) {
    return GeometryIdentifier()
        .setVolume(encoded.value("Volume", GeometryIdentifier::Value(0u)))
        .setBoundary(encoded.value("_Boundary", GeometryIdentifier::Value(0u)))
        .setLayer(encoded.value("_Layer", GeometryIdentifier::Value(0u)))
        .setApproach(encoded.value("__Approach", GeometryIdentifier::Value(0u)))
        .setSensitive(
            encoded.value("__Sensitive", GeometryIdentifier::Value(0u)));
  }
};

// implementations

template <typename value_t>
nlohmann::json GeometryHierarchyMapJsonConverter<value_t>::toJson(
    const Container& container) const {
  // encode header
  nlohmann::json encoded = nlohmann::json::object();
  encoded[kHeaderKey] = {
      {"format-version", kFormatVersion},
      {"value-identifier", m_valueIdentifier},
  };
  // encode entries
  nlohmann::json entries = nlohmann::json::array();
  for (std::size_t i = 0; i < container.size(); ++i) {
    auto entry = encodeIdentifier(container.idAt(i));
    entry["value"] = nlohmann::json(container.valueAt(i));
    entries.push_back(std::move(entry));
  }
  encoded[kEntriesKey] = std::move(entries);
  return encoded;
}

template <typename value_t>
auto GeometryHierarchyMapJsonConverter<value_t>::fromJson(
    const nlohmann::json& encoded) const -> Container {
  // verify json format header
  auto header = encoded.find(kHeaderKey);
  if (header == encoded.end()) {
    throw std::invalid_argument(
        "Missing header entry in json geometry hierarchy map");
  }
  if (header->at("format-version").get<int>() != kFormatVersion) {
    throw std::invalid_argument(
        "Invalid format version in json geometry hierarchy map");
  }
  if (header->at("value-identifier").get<std::string>() != m_valueIdentifier) {
    throw std::invalid_argument(
        "Inconsistent value identifier in Json geometry hierarchy map");
  }
  // decode json entries
  if (not encoded.contains(kEntriesKey)) {
    throw std::invalid_argument(
        "Missing entries in json geometry hierarchy map");
  }
  std::vector<std::pair<GeometryIdentifier, Value>> elements;
  for (const auto& entry : encoded.at(kEntriesKey)) {
    auto id = decodeIdentifier(entry);
    auto value = entry.at("value").get<Value>();
    elements.emplace_back(std::move(id), std::move(value));
  }
  return elements;
}

}  // namespace Acts

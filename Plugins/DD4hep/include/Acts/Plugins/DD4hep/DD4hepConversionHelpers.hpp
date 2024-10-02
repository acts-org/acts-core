// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"

#include <DD4hep/DetElement.h>
#include <DD4hep/DetFactoryHelper.h>
#include <DD4hep/Objects.h>
#include <DDRec/DetectorData.h>

namespace Acts {

/// Helper function to extract a parameter value from a dd4hep detector element
/// from VariantParameters
/// @tparam T The value type
/// @param key The key of the value to extract
/// @param elt The detector element instance
/// @return A copy of the value contained in the params instance
template <typename T>
T getParam(const std::string& key, dd4hep::DetElement& elt) {
  auto* params = elt.extension<dd4hep::rec::VariantParameters>(false);
  if (params == nullptr) {
    throw std::runtime_error{"Detector Element has no VariantParameters"};
  }
  return params->get<T>(key);
}

/// Helper function to extract a VariantParameters instance
/// @param elt The detector element instance
/// @return The VariantParameters instance
inline dd4hep::rec::VariantParameters& getParams(dd4hep::DetElement& elt) {
  auto* params = elt.extension<dd4hep::rec::VariantParameters>(false);
  if (params == nullptr) {
    throw std::runtime_error{"Detector Element has no VariantParameters"};
  }
  return *params;
}

/// Helper function to extract a VariantParameters instance, const version
/// @param elt The detector element instance
/// @return The VariantParameters instance
inline const dd4hep::rec::VariantParameters& getParams(
    const dd4hep::DetElement& elt) {
  const auto* params = elt.extension<dd4hep::rec::VariantParameters>(false);
  if (params == nullptr) {
    throw std::runtime_error{"Detector Element has no VariantParameters"};
  }
  return *params;
}

/// Get a parameter value or an alternative value if either the
/// VariantParameters extension isn't set, or it doesn't contain the demanded
/// key
/// @tparam T The value type
/// @param key The key of the value to extract
/// @param elt The detector element instance
/// @param alternative The value to return if no params are set of the key doesn't exist
/// @return The value behind key, or @p alternative
template <typename T>
T getParamOr(const std::string& key, const dd4hep::DetElement& elt,
             T alternative) {
  auto* params = elt.extension<dd4hep::rec::VariantParameters>(false);
  if (params == nullptr) {
    return alternative;
  }
  return params->value_or<T>(key, alternative);
}

/// Check if a detector element has a key set in its VariantParameters
/// @param key The key to check existence for
/// @param elt The detector element instance
/// @return True if the element has VariantParameters and the key exists, false if
///         either of these is not true
inline bool hasParam(const std::string& key, dd4hep::DetElement& elt) {
  auto* params = elt.extension<dd4hep::rec::VariantParameters>(false);
  if (params == nullptr) {
    return false;
  }
  return params->contains(key);
}

/// Check if a detector element has VariantParameters set
/// @param elt The detector element instance
/// @return True if the VariantParameters exist, false if not
inline bool hasParams(dd4hep::DetElement& elt) {
  return elt.extension<dd4hep::rec::VariantParameters>(false) != nullptr;
}

/// @brief Helper method to get an attribute with fallback
///
/// @note the fallback value has to be provided
///
/// @tparam value_type the primitive type allowed by variant parameters
///
/// @param node the node object from DD4hep
/// @param attrName the name of the attribute that is checked
/// @param fallbackValue the fallbackValue
///
/// @return either the gathered attribute or the fallback
template <typename value_type>
value_type getAttrValueOr(const dd4hep::xml::Component& node,
                          const std::string& attrName,
                          const value_type& fallbackValue) {
  if (node.hasAttr(dd4hep::xml::Strng_t(attrName.c_str()))) {
    return node.attr<value_type>(attrName.c_str());
  } else {
    return fallbackValue;
  }
}

/// @brief A simple helper function to extract a series
///
/// @tparam value_type the primitive type allowed by variant parameters
///
/// @param dd4hepElement the detector element with associated variant parameters
/// @param bname The base name attribute of the variant parameter pack
/// @param unitConversion is a conversion factor DD4hep -> ACTS
///
/// @return the extracted series as a vector
template <typename value_type>
std::vector<value_type> extractSeries(const dd4hep::DetElement& dd4hepElement,
                                      const std::string& bname,
                                      const value_type& unitConversion = 1) {
  std::vector<value_type> series = {};

  int fallBack = 0;
  int nVals = getParamOr<int>(bname + "_n", dd4hepElement, fallBack);
  series.reserve(nVals);
  for (auto ib = 0; ib < nVals; ++ib) {
    auto val = unitConversion *
               getParamOr<value_type>(bname + "_" + std::to_string(ib),
                                      dd4hepElement, 0.);
    series.push_back(val);
  }
  return series;
}

/// @brief A simple helper function to extract a transform
///
/// @param dd4hepElement the detector element with associated variant parameters
/// @param bname The base name attribute of the variant parameter pack
/// @param unitConversion is a conversion factor DD4hep -> ACTS
///
/// @return a transform extracted from parameters
inline Transform3 extractTransform(const dd4hep::DetElement& dd4hepElement,
                                   const std::string& bname,
                                   const ActsScalar unitConversion = 1.) {
  Transform3 transform = Transform3::Identity();
  ActsScalar x =
      unitConversion * getParamOr<ActsScalar>(bname + "_x", dd4hepElement, 0.);
  ActsScalar y =
      unitConversion * getParamOr<ActsScalar>(bname + "_y", dd4hepElement, 0.);
  ActsScalar z =
      unitConversion * getParamOr<ActsScalar>(bname + "_z", dd4hepElement, 0.);
  transform.pretranslate(Vector3(x, y, z));
  return transform;
}

}  // namespace Acts

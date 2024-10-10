// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Acts {

/// @class BinUtility
///
/// The BinUtility class that translated global and local position into a bins
/// of a BinnedArray, most performant is equidistant binning without a
/// transform,
/// however, optionally a transform can be provided, e.g. for binning on shifted
/// object, the transform is usually shared with the geometric object the Array
/// is
/// defined on, for performance reasons, also the inverse transform is stored.
///
class BinUtility {
 public:
  /// Constructor for equidistant
  BinUtility()
      : m_binningData(),
        m_transform(Transform3::Identity()),
        m_itransform(Transform3::Identity()) {
    m_binningData.reserve(3);
  }

  /// Constructor with only a Transform3
  ///
  /// @param tForm is the local to global transform
  explicit BinUtility(const Transform3& tForm)
      : m_binningData(), m_transform(tForm), m_itransform(tForm.inverse()) {
    m_binningData.reserve(3);
  }

  /// Constructor from BinningData directly
  ///
  /// @param bData is the provided binning data
  /// @param tForm is the (optional) transform
  BinUtility(const BinningData& bData,
             const Transform3& tForm = Transform3::Identity())
      : m_binningData(), m_transform(tForm), m_itransform(tForm.inverse()) {
    m_binningData.reserve(3);
    m_binningData.push_back(bData);
  }

  /// Constructor for equidistant
  ///
  /// @param bins is the number of bins
  /// @param min in the minimal value
  /// @param max is the maximal value
  /// @param opt is the binning option : open, closed
  /// @param value is the binninb value : binX, binY, binZ, etc.
  /// @param tForm is the (optional) transform
  BinUtility(std::size_t bins, float min, float max, BinningOption opt = open,
             BinningValue value = BinningValue::binX,
             const Transform3& tForm = Transform3::Identity())
      : m_binningData(), m_transform(tForm), m_itransform(tForm.inverse()) {
    m_binningData.reserve(3);
    m_binningData.push_back(BinningData(opt, value, bins, min, max));
  }

  /// Constructor for arbitrary
  ///
  /// @param bValues is the boundary values of the binning
  /// @param opt is the binning option : open, closed
  /// @param value is the binninb value : binX, binY, binZ, etc.
  /// @param tForm is the (optional) transform
  BinUtility(std::vector<float>& bValues, BinningOption opt = open,
             BinningValue value = BinningValue::binPhi,
             const Transform3& tForm = Transform3::Identity())
      : m_binningData(), m_transform(tForm), m_itransform(tForm.inverse()) {
    m_binningData.reserve(3);
    m_binningData.push_back(BinningData(opt, value, bValues));
  }

  /// Copy constructor
  ///
  /// @param sbu is the source bin utility
  BinUtility(const BinUtility& sbu) = default;

  BinUtility(BinUtility&& sbu) = default;

  /// Assignment operator
  ///
  /// @param sbu is the source bin utility
  BinUtility& operator=(const BinUtility& sbu) {
    if (this != &sbu) {
      m_binningData = sbu.m_binningData;
      m_transform = sbu.m_transform;
      m_itransform = sbu.m_itransform;
    }
    return (*this);
  }

  BinUtility& operator=(BinUtility&&) = default;

  /// Operator+= to make multidimensional BinUtility
  ///
  /// @param gbu is the additional BinUtility to be chosen
  BinUtility& operator+=(const BinUtility& gbu) {
    const std::vector<BinningData>& bData = gbu.binningData();

    m_transform = m_transform * gbu.transform();
    m_itransform = m_transform.inverse();
    if (m_binningData.size() + bData.size() > 3) {
      throw std::runtime_error{"BinUtility does not support dim > 3"};
    }
    m_binningData.insert(m_binningData.end(), bData.begin(), bData.end());
    return (*this);
  }

  /// Virtual Destructor
  ~BinUtility() = default;

  /// Equality operator
  bool operator==(const BinUtility& other) const {
    return (m_transform.isApprox(other.m_transform) &&
            m_binningData == other.binningData());
  }

  /// Return the binning data vector
  const std::vector<BinningData>& binningData() const { return m_binningData; }

  /// Return the total number of bins
  std::size_t bins() const { return bins(0) * bins(1) * bins(2); }

  /// Bin-triple fast access
  ///
  /// - calculate the bin triple with one transform
  ///
  /// @param position is the 3D position to be evaluated
  ///
  /// @return is the bin value in 3D
  std::array<std::size_t, 3> binTriple(const Vector3& position) const {
    /// transform or not
    const Vector3 bPosition = m_itransform * position;
    // get the dimension
    std::size_t mdim = m_binningData.size();
    /// now get the bins
    std::size_t bin0 = m_binningData[0].searchGlobal(bPosition);
    std::size_t bin1 = mdim > 1 ? m_binningData[1].searchGlobal(bPosition) : 0;
    std::size_t bin2 = mdim > 2 ? m_binningData[2].searchGlobal(bPosition) : 0;
    /// return the triple
    return {{bin0, bin1, bin2}};
  }

  /// Bin from a 3D vector (already in binning frame)
  ///
  /// @param position is the 3D position to be evaluated
  /// @param ba is the bin dimension
  ///
  /// @return is the bin value
  std::size_t bin(const Vector3& position, std::size_t ba = 0) const {
    if (ba >= m_binningData.size()) {
      return 0;
    }
    std::size_t bEval = m_binningData[ba].searchGlobal(m_itransform * position);
    return bEval;
  }

  /// Return the other direction for fast interlinking
  ///
  /// @param position is the global position for the next search
  /// @param direction is the global position for the next search
  /// @param ba is the bin accessor
  ///
  /// @todo the
  ///
  /// @return the next bin
  int nextDirection(const Vector3& position, const Vector3& direction,
                    std::size_t ba = 0) const {
    if (ba >= m_binningData.size()) {
      return 0;
    }
    return m_binningData[ba].nextDirection(position, direction);
  }

  /// Bin from a 2D vector (following local parameters defintitions)
  /// - no optional transform applied
  /// - USE WITH CARE !!
  ///
  /// You need to make sure that the local position is actually in the binning
  /// frame of the BinUtility
  ///
  /// @param lposition is the local position to be set
  /// @param ba is the bin dimension
  ///
  /// @return bin calculated from local
  std::size_t bin(const Vector2& lposition, std::size_t ba = 0) const {
    if (ba >= m_binningData.size()) {
      return 0;
    }
    return m_binningData[ba].searchLocal(lposition);
  }
  /// Check if bin is inside from Vector2 - optional transform applied
  ///
  /// @param position is the global position to be evaluated
  /// @return is a boolean check
  bool inside(const Vector3& position) const {
    /// transform or not
    const Vector3& bPosition = m_itransform * position;
    // loop and break
    for (auto& bData : m_binningData) {
      if (!(bData.inside(bPosition))) {
        return false;
      }
    }
    // survived all the checks
    return true;
  }

  /// First bin maximal value
  /// @return the dimension of the binning data
  std::size_t dimensions() const { return m_binningData.size(); }

  /// First bin maximal value
  ///
  /// @param ba is the binaccessor
  ///
  /// @return std::size_t is the maximal bin of the accessor entry
  std::size_t max(std::size_t ba = 0) const {
    if (ba >= m_binningData.size()) {
      return 0;
    }
    return (m_binningData[ba].bins() - 1);
  }

  /// Number of bins
  ///
  /// @param ba is the binaccessor
  ///
  /// @return std::size_t is the bins of the accessor entry
  std::size_t bins(std::size_t ba) const {
    if (ba >= m_binningData.size()) {
      return 1;
    }
    return (m_binningData[ba].bins());
  }

  /// Transform applied to global positions before lookup
  ///
  /// @return Shared pointer to transform
  const Transform3& transform() const { return m_transform; }

  /// The type/value of the binning
  ///
  /// @param ba is the binaccessor
  ///
  /// @return the binning value of the accessor entry
  BinningValue binningValue(std::size_t ba = 0) const {
    if (ba >= m_binningData.size()) {
      throw std::runtime_error{"Dimension out of bounds"};
    }
    return (m_binningData[ba].binvalue);
  }

  /// Serialize the bin triple
  /// - this creates a simple std::size_t from a triple object
  ///
  /// @param bin is the bin to be serialized
  std::size_t serialize(const std::array<std::size_t, 3>& bin) const {
    std::size_t serializedBin = bin[0];
    if (m_binningData.size() == 2) {
      serializedBin += bin[1] * m_binningData[0].bins();
    } else if (m_binningData.size() == 3) {
      serializedBin +=
          (bin[1] * m_binningData[0].bins() * bin[2] * m_binningData[1].bins());
    }
    return serializedBin;
  }

  /// Output Method for std::ostream, to be overloaded by child classes
  ///
  /// @param sl is the ostream to be dumped into
  /// @param indent the current indentation
  ///
  /// @return the input stream
  std::ostream& toStream(std::ostream& sl,
                         const std::string& indent = "") const {
    sl << indent << "BinUtility for " << m_binningData.size()
       << "- dimensional array:" << std::endl;
    for (auto [ibd, bd] : enumerate(m_binningData)) {
      sl << indent << "dimension     : " << ibd << std::endl;
      sl << bd.toString(indent) << std::endl;
    }
    return sl;
  }

  /// Output into a string
  ///
  /// @param indent the current indentation
  ///
  /// @return a string with the stream information
  std::string toString(const std::string& indent = "") const {
    std::stringstream ss;
    toStream(ss, indent);
    return ss.str();
  }

  /// Overload of << operator for std::ostream for debug output
  friend std::ostream& operator<<(std::ostream& sl, const BinUtility& bgen) {
    return bgen.toStream(sl);
  }

 private:
  std::vector<BinningData> m_binningData;  /// vector of BinningData
  Transform3 m_transform;                  /// shared transform
  Transform3 m_itransform;                 /// unique inverse transform
};

}  // namespace Acts

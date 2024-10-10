// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {
/// @brief A description of a triplet candidate.
/// @tparam external_space_point_t  The external spacepoint type.
template <typename external_space_point_t>
struct TripletCandidate {
  /// @brief Default Constructor
  TripletCandidate() = default;

  /// @brief Default Destructor
  ~TripletCandidate() = default;

  /// @brief constructor
  /// @param b The bottom space point
  /// @param m The middle space point
  /// @param t The top space point
  /// @param w The quality of the candidate
  /// @param z The z coordinate of the origin
  /// @param q Whether the candidate is high or low quality
  TripletCandidate(external_space_point_t& b, external_space_point_t& m,
                   external_space_point_t& t, float w, float z, bool q)
      : bottom(&b), middle(&m), top(&t), weight(w), zOrigin(z), isQuality(q) {}

  /// @brief Copy operations
  TripletCandidate(const TripletCandidate&) = default;
  TripletCandidate& operator=(const TripletCandidate&) = default;

  /// @brief Move operations
  TripletCandidate(TripletCandidate&& other) noexcept
      : bottom(std::exchange(other.bottom, nullptr)),
        middle(std::exchange(other.middle, nullptr)),
        top(std::exchange(other.top, nullptr)),
        weight(other.weight),
        zOrigin(other.zOrigin),
        isQuality(other.isQuality) {}

  TripletCandidate& operator=(TripletCandidate&& other) noexcept {
    bottom = std::exchange(other.bottom, nullptr);
    middle = std::exchange(other.middle, nullptr);
    top = std::exchange(other.top, nullptr);
    weight = other.weight;
    zOrigin = other.zOrigin;
    isQuality = other.isQuality;
    return *this;
  }

  external_space_point_t* bottom{nullptr};
  external_space_point_t* middle{nullptr};
  external_space_point_t* top{nullptr};
  float weight{0.};
  float zOrigin{0.};
  bool isQuality{false};
};

/// @class CandidatesForMiddleSp
/// The CandidatesForMiddleSp collects the triplet candidates given a
/// fixed middle spacepoint. It internally stores the triplet candidates
/// keeping only those with the higher quality.
///
/// @tparam external_space_point_t The external spacepoint type.

template <typename external_space_point_t>
concept SatisfyCandidateConcept = requires(external_space_point_t spacePoint) {
  { spacePoint.x() } -> std::convertible_to<float>;
  { spacePoint.y() } -> std::convertible_to<float>;
  { spacePoint.z() } -> std::convertible_to<float>;
};

template <SatisfyCandidateConcept external_space_point_t>
class CandidatesForMiddleSp {
 public:
  using value_type = TripletCandidate<external_space_point_t>;

  /// @brief Setting maximum number of candidates to keep
  /// @param nLow Maximum number of candidates in the low-quality collection
  /// @param nHigh Maximum number of candidates in the high-quality collection
  void setMaxElements(std::size_t nLow, std::size_t nHigh);

  /// @brief Retrieve the triplet candidates, the resulting vector is already sorted,
  /// elements with higher quality first
  /// @returns Vector of triplet candidates
  std::vector<value_type> storage();

  /// @brief Adding a new triplet candidate to the collection, should it satisfy the
  /// selection criteria
  /// @param SpB Bottom space point
  /// @param SpM Medium space point
  /// @param SpT Top space point
  /// @param weight The quality of the triplet candidate
  /// @param zOrigin The z-coordinate of the origin
  /// @param isQuality Whether the triplet candidate is high or low quality
  /// @returns whether the triplet candidate has been added or not to the collection
  bool push(external_space_point_t& SpB, external_space_point_t& SpM,
            external_space_point_t& SpT, float weight, float zOrigin,
            bool isQuality);

  /// @brief Clear the internal storage
  void clear();

  /// @brief A function for sorting the triplet candidates from higher to lower quality
  /// @param i1 First triplet candidate
  /// @param i2 Second triplet candidate
  /// @returns The comparison result
  static bool descendingByQuality(const value_type& i1, const value_type& i2);

  /// @brief A function for sorting the triplet candidates from lower to higher quality
  /// @param i1 First triplet candidate
  /// @param i2 Second triplet candidate
  /// @returns The comparison result
  static bool ascendingByQuality(const value_type& i1, const value_type& i2);

  /// @brief Retrieve the number of Low quality candidates
  /// @returns The number of Low quality candidates
  std::size_t nLowQualityCandidates() const;

  /// @brief Retrieve the number of High quality candidates
  /// @returns The number of High quality candidates
  std::size_t nHighQualityCandidates() const;

 private:
  /// @brief dding a new triplet candidate to the collection, should it satisfy the
  /// selection criteria
  /// @param indices The collection into which the candidate should be stored
  /// @param n The current number of stored elements in the container
  /// @param nMax The maximum number of elements that can be stored in the container
  /// @param SpB The bottom space point
  /// @param SpM The middle space point
  /// @param SpT The top space point
  /// @param weight The quality of the triplet candidate
  /// @param zOrigin The z-coordinate of the origin
  /// @param isQuality Whether the triplet candidate is high or low quality
  /// @returns whether the triplet candidate has been added or not to the collection
  bool push(std::vector<std::size_t>& indices, std::size_t& n,
            const std::size_t nMax, external_space_point_t& SpB,
            external_space_point_t& SpM, external_space_point_t& SpT,
            float weight, float zOrigin, bool isQuality);

  /// @brief Check if an element exists in the collection. The element to be checked
  /// is supposed to be in the n position of the collection.
  /// @param n Index of the requested element
  /// @param maxSize Number of elements currently stored in the collection
  /// @returns Whether the element exists
  bool exists(const std::size_t n, const std::size_t maxSize) const;

  /// @brief Pop an element from a collection. The removal of the element from the collection
  /// does not imply its destruction. In fact, the number of stored elements is
  /// simply diminished by 1. The popped element is technically still available
  /// at the end of the collection.
  /// @param indices The collection
  /// @param currentSize The current number of element stored in the collection. The function will
  /// diminish this value by 1
  void pop(std::vector<std::size_t>& indices, std::size_t& currentSize);

  /// @brief Return the weight for a candidate
  /// @param indices The collection in which the element is stored
  /// @param n Index of the element in the collection
  /// @returns The weight of the candidate
  float weight(const std::vector<std::size_t>& indices, std::size_t n) const;

  /// @brief Move an element up in the min heap tree. The function checks whether the element's
  /// weight is lower of its parent's weight. If so, it swaps them. Reiterate
  /// the process until the element is in the correct position on the tree
  /// @param indices The collection
  /// @param n The index of the element to place in the correct position
  void bubbleup(std::vector<std::size_t>& indices, std::size_t n);

  /// @brief Move an element down in the min heap tree. The function checks whether the elements's
  /// weight is lower of its child's weights. If so, it swaps the element with
  /// the child with the lowest weight. Reiterate the process until the element
  /// is in the correct position on the tree
  /// @param indices The collection
  /// @param n The index of the element to place in the correct position
  /// @param actualSize The current number of elements stored in the collection
  void bubbledw(std::vector<std::size_t>& indices, std::size_t n,
                std::size_t actualSize);

  /// @brief Adding a new triplet candidate to the collection. The function is called after the candidate has satisfied
  /// all the selection criteria
  /// @param indices The collection
  /// @param n Current number of stored elements in the collection
  /// @param nMax The maximum number of elements that can be stored in the collection
  /// @param element The element that must be added to the collection
  void addToCollection(std::vector<std::size_t>& indices, std::size_t& n,
                       const std::size_t nMax, value_type&& element);

 private:
  // sizes
  // m_maxSize* is the maximum size of the indices collections. These values
  // are set by the user once
  std::size_t m_maxSizeHigh{0};
  std::size_t m_maxSizeLow{0};
  // m_n_* is the current size of the indices collections [0, m_maxSize*).
  // These values are set internally by the class
  std::size_t m_nHigh{0};
  std::size_t m_nLow{0};

  // storage contains the collection of the candidates
  std::vector<value_type> m_storage{};

  // The following vectors store indexes to elements in the storage
  // They are sorted as a min heap tree, in which
  // Each node is lower than its children
  // Thus, it is guaranteed that the lower elements is at the front
  // Sorting criteria is the seed quality
  //
  // This is in effect faster sorted container - implementation with std::set
  // and std::priority_queue were tried and were found to be slower.

  // list of indexes of candidates with high quality in the storage
  std::vector<std::size_t> m_indicesHigh{};
  // list of indexes of candidates with low quality in the storage
  std::vector<std::size_t> m_indicesLow{};
};

}  // namespace Acts

#include "Acts/Seeding/CandidatesForMiddleSp.ipp"

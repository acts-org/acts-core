// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/SpacePointContainer.hpp"
#include "Acts/Utilities/Holders.hpp"

#include <any>

namespace ActsExamples {

template <typename collection_t>
class SpacePointContainer {
 public:
  using CollectionType = collection_t;
  using ValueType = typename CollectionType::value_type;

  friend Acts::SpacePointContainer<
      ActsExamples::SpacePointContainer<collection_t>, Acts::detail::RefHolder>;

  // default constructor is of no use. It cannot be used, so why bother?
  SpacePointContainer() = delete;
  // we never get the ownership. In both read-only and read-and-write mode
  // the memory backend is independetly handled. This is only interfacing it to
  // ACTS
  SpacePointContainer(CollectionType&& container) = delete;
  SpacePointContainer(CollectionType& container) : m_storage(container) {}
  SpacePointContainer(CollectionType* container) : m_storage(container) {}

  // No copy constructor or copy operation allowed
  SpacePointContainer(const SpacePointContainer<collection_t>&) = delete;
  SpacePointContainer<collection_t>& operator=(
      const SpacePointContainer<collection_t>&) = delete;

  // only move operation allowed
  SpacePointContainer(SpacePointContainer<collection_t>&& other) noexcept
      : m_storage(std::exchange(other.m_storage.ptr, nullptr)) {}
  SpacePointContainer<collection_t>& operator=(
      SpacePointContainer<collection_t>&& other) noexcept {
    m_storage = std::exchange(other.m_storage.ptr, nullptr);
    return *this;
  }

  ~SpacePointContainer() = default;

 private:
  std::size_t size_impl() const;

  float x_impl(std::size_t idx) const;
  float y_impl(std::size_t idx) const;
  float z_impl(std::size_t idx) const;
  float varianceR_impl(std::size_t idx) const;
  float varianceZ_impl(std::size_t idx) const;

  const ValueType& get_impl(std::size_t idx) const { return storage()[idx]; }

  std::any component_impl(Acts::HashedString key, std::size_t /*n*/) const {
    using namespace Acts::HashedStringLiteral;
    switch (key) {
      case "TopStripVector"_hash:
      case "BottomStripVector"_hash:
      case "StripCenterDistance"_hash:
      case "TopStripCenterPosition"_hash:
        return Acts::Vector3(0, 0, 0);
      default:
        throw std::runtime_error("no such component " + std::to_string(key));
    }
  }

 private:
  const CollectionType& storage() const;

 private:
  Acts::detail::RefHolder<CollectionType> m_storage;
};

template <typename collection_t>
inline std::size_t SpacePointContainer<collection_t>::size_impl() const {
  return storage().size();
}

// TO-DO
// Be smart here... collection_t can container values or pointers ...

template <typename collection_t>
inline float SpacePointContainer<collection_t>::x_impl(std::size_t idx) const {
  return storage()[idx]->x();
}

template <typename collection_t>
inline float SpacePointContainer<collection_t>::y_impl(std::size_t idx) const {
  return storage()[idx]->y();
}

template <typename collection_t>
inline float SpacePointContainer<collection_t>::z_impl(std::size_t idx) const {
  return storage()[idx]->z();
}

template <typename collection_t>
inline float SpacePointContainer<collection_t>::varianceR_impl(
    std::size_t idx) const {
  return storage()[idx]->varianceR;
}

template <typename collection_t>
inline float SpacePointContainer<collection_t>::varianceZ_impl(
    std::size_t idx) const {
  return storage()[idx]->varianceZ;
}

template <typename collection_t>
const typename SpacePointContainer<collection_t>::CollectionType&
SpacePointContainer<collection_t>::storage() const {
  return *m_storage;
}

}  // namespace ActsExamples

// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/SpacePointProxy.hpp"
#include "Acts/Utilities/Holders.hpp"

namespace Acts {

template <typename container_t>
class SpacePointProxyIterator {
 public:
  using ContainerType = container_t;
  using ProxyType = typename container_t::SpacePointProxyType;

  using iterator_category = std::random_access_iterator_tag;
  using value_type = ProxyType;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;

 public:
  // Constructors
  SpacePointProxyIterator(container_t&& container, std::size_t index) = delete;
  SpacePointProxyIterator(const container_t& container, std::size_t index);

  SpacePointProxyIterator& operator++();
  SpacePointProxyIterator& operator--();
  SpacePointProxyIterator operator++(int);
  SpacePointProxyIterator operator--(int);

  bool operator==(const SpacePointProxyIterator& other) const;
  auto operator<=>(const SpacePointProxyIterator& other) const;

  SpacePointProxyIterator& operator+=(const std::size_t offset);
  SpacePointProxyIterator& operator-=(const std::size_t offset);

  SpacePointProxyIterator operator+(const std::size_t offset) const;
  SpacePointProxyIterator operator-(const std::size_t offset) const;

  difference_type operator-(const SpacePointProxyIterator& other) const;

  const value_type& operator*() const;

 private:
  const container_t* m_container{nullptr};
  std::size_t m_index{0ul};
};

}  // namespace Acts

#include "Acts/EventData/SpacePointProxyIterator.ipp"

// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/HashedString.hpp"
#include "Acts/Utilities/Holders.hpp"

#include <any>
#include <iostream>
#include <string_view>
#include <type_traits>

namespace Acts {

template <typename container_t>
class SpacePointProxy {
 public:
  using ContainerType = container_t;
  using ValueType = typename ContainerType::ValueType;

  // Never take the ownership of the container
  SpacePointProxy(container_t&& container, std::size_t index) = delete;
  // Only get the reference
  SpacePointProxy(const container_t& container, std::size_t index);

  const ValueType& externalSpacePoint() const;
  std::size_t index() const;

  float x() const;
  float y() const;
  float z() const;
  float phi() const;
  float radius() const;
  float varianceR() const;
  float varianceZ() const;

  const Acts::Vector3& topStripVector() const;
  const Acts::Vector3& bottomStripVector() const;
  const Acts::Vector3& stripCenterDistance() const;
  const Acts::Vector3& topStripCenterPosition() const;

 private:
  const container_t& container() const;

  const container_t* m_container{nullptr};
  std::size_t m_index{0ul};
};

}  // namespace Acts

#include "Acts/EventData/SpacePointProxy.ipp"

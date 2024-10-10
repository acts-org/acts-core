// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SpacePointData.hpp"
#include "Acts/EventData/SpacePointProxy.hpp"
#include "Acts/EventData/Utils.hpp"
#include "Acts/Utilities/HashedString.hpp"
#include "Acts/Utilities/Iterator.hpp"

#include <any>
#include <vector>

namespace Acts {

struct SpacePointContainerConfig {
  bool useDetailedDoubleMeasurementInfo = false;
  bool isInInternalUnits = false;

  SpacePointContainerConfig toInternalUnits() const {
    if (isInInternalUnits) {
      throw std::runtime_error(
          "Repeated conversion to internal units for "
          "SpacePointContainerConfig");
    }
    using namespace Acts::UnitLiterals;
    SpacePointContainerConfig config = *this;
    config.isInInternalUnits = true;
    return config;
  };
};

struct SpacePointContainerOptions {
  // location of beam in x,y plane.
  // used as offset for Space Points
  Acts::Vector2 beamPos{0 * Acts::UnitConstants::mm,
                        0 * Acts::UnitConstants::mm};
  bool isInInternalUnits = false;

  SpacePointContainerOptions toInternalUnits() const {
    if (isInInternalUnits) {
      throw std::runtime_error(
          "Repeated conversion to internal units for "
          "SpacePointContainerOptions");
    }
    using namespace Acts::UnitLiterals;
    SpacePointContainerOptions options = *this;
    options.isInInternalUnits = true;
    options.beamPos[0] /= 1_mm;
    options.beamPos[1] /= 1_mm;
    return options;
  }
};

template <typename container_t, template <typename> class holder_t>
class SpacePointContainer {
 public:
  friend class Acts::SpacePointProxy<
      Acts::SpacePointContainer<container_t, holder_t>>;

 public:
  using SpacePointProxyType =
      Acts::SpacePointProxy<Acts::SpacePointContainer<container_t, holder_t>>;

  using iterator =
      ContainerIndexIterator<Acts::SpacePointContainer<container_t, holder_t>,
                             SpacePointProxyType&, false>;
  using const_iterator =
      ContainerIndexIterator<Acts::SpacePointContainer<container_t, holder_t>,
                             const SpacePointProxyType&, true>;

  using ValueType = typename container_t::ValueType;
  using ProxyType = SpacePointProxyType;
  using value_type = ProxyType;
  using size_type = std::size_t;

 public:
  // Constructors
  // It makes sense to support both options of
  // taking or not the ownership

  // Do not take ownership
  // Activate only if holder_t is RefHolder
  template <template <typename> class H = holder_t,
            typename = std::enable_if_t<Acts::detail::is_same_template<
                H, Acts::detail::RefHolder>::value>>
  SpacePointContainer(const Acts::SpacePointContainerConfig& config,
                      const Acts::SpacePointContainerOptions& options,
                      const container_t& container);

  // Take the ownership
  // Activate only if holder_t is ValueHolder
  template <template <typename> class H = holder_t,
            typename = std::enable_if_t<Acts::detail::is_same_template<
                H, Acts::detail::ValueHolder>::value>>
  SpacePointContainer(const Acts::SpacePointContainerConfig& config,
                      const Acts::SpacePointContainerOptions& options,
                      container_t&& container);

  // No copy operations
  SpacePointContainer(SpacePointContainer& other) = delete;
  SpacePointContainer& operator=(SpacePointContainer& other) = delete;

  // move operations
  SpacePointContainer(SpacePointContainer&& other) noexcept = default;
  SpacePointContainer& operator=(SpacePointContainer&& other) noexcept =
      default;

  // Destructor
  ~SpacePointContainer() = default;

  std::size_t size() const;

  iterator begin();
  iterator end();
  const_iterator cbegin() const;
  const_iterator cend() const;
  const_iterator begin() const;
  const_iterator end() const;

  ProxyType& at(const std::size_t n);
  const ProxyType& at(const std::size_t n) const;
  const ValueType& sp(const std::size_t n) const;

 private:
  void initialize();

  const container_t& container() const;
  const ProxyType& proxy(const std::size_t n) const;
  std::vector<ProxyType>& proxies();
  const std::vector<ProxyType>& proxies() const;

  float x(const std::size_t n) const;
  float y(const std::size_t n) const;
  float z(const std::size_t n) const;
  float phi(const std::size_t n) const;
  float radius(const std::size_t n) const;
  float varianceR(const std::size_t n) const;
  float varianceZ(const std::size_t n) const;

  const Acts::Vector3& topStripVector(const std::size_t n) const;
  const Acts::Vector3& bottomStripVector(const std::size_t n) const;
  const Acts::Vector3& stripCenterDistance(const std::size_t n) const;
  const Acts::Vector3& topStripCenterPosition(const std::size_t n) const;

  Acts::SpacePointContainerConfig m_config;
  Acts::SpacePointContainerOptions m_options;
  Acts::SpacePointData m_data;
  holder_t<const container_t> m_container;
  std::vector<ProxyType> m_proxies;
};

}  // namespace Acts

#include "Acts/EventData/SpacePointContainer.ipp"

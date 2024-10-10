// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceError.hpp"
#include "ActsFatras/Digitization/DigitizationData.hpp"

#include <array>
#include <utility>

namespace ActsFatras {

/// Generic implementation of a channel merger, currently only additive
/// channel merging.
///
/// @tparam signal_t The type of signal, needs operator+= to be defined
/// @tparam kSize the dimensonality of the object (cluster)
///
/// @param channels The channels from one cluster
///
/// @return A cluster containing the parameter set and cluster size
template <typename signal_t, std::size_t kSize>
const std::vector<Channel<signal_t, kSize>> mergeChannels(
    const std::vector<Channel<signal_t, kSize>>& channels) {
  using Channel = Channel<signal_t, kSize>;
  using ChannelKey = std::array<unsigned int, kSize>;

  // Fill a channel map - use the channel identification
  auto extractChannelKey = [&](const Channel& ch) -> ChannelKey {
    ChannelKey cKey;
    for (unsigned int ik = 0; ik < kSize; ++ik) {
      cKey[ik] = ch.cellId[ik].first;
    }
    return cKey;
  };

  std::map<ChannelKey, Channel> channelMap;
  for (const auto& ch : channels) {
    ChannelKey key = extractChannelKey(ch);
    auto chItr = channelMap.find(key);
    if (chItr != channelMap.end()) {
      chItr->second.value += ch.value;
      chItr->second.links.insert(ch.links.begin(), ch.links.end());
    } else {
      channelMap.insert(std::pair<ChannelKey, Channel>(key, ch));
    }
  }
  // Unroll the channels after merging
  std::vector<Channel> mergedChannels;
  mergedChannels.reserve(channelMap.size());
  for (auto& [key, value] : channelMap) {
    mergedChannels.push_back(value);
  }
  return mergedChannels;
}

}  // namespace ActsFatras

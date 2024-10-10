// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// TODO: update to C++17 style
// Consider to moving to detail subdirectory
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

namespace Acts {

struct GbtsConnection {
 public:
  GbtsConnection(unsigned int s, unsigned int d);

  unsigned int m_src, m_dst;
  std::vector<int> m_binTable;
};

class GbtsConnector {
 public:
  struct LayerGroup {
    LayerGroup(unsigned int l1Key,
               const std::vector<const Acts::GbtsConnection *> &v)
        : m_dst(l1Key), m_sources(v) {}

    unsigned int m_dst;  // the target layer of the group
    std::vector<const Acts::GbtsConnection *>
        m_sources;  // the source layers of the group
  };

  GbtsConnector(std::ifstream &inFile);

  ~GbtsConnector();

  float m_etaBin{};

  std::map<int, std::vector<struct LayerGroup>> m_layerGroups;
  std::map<int, std::vector<Acts::GbtsConnection *>> m_connMap;
  // TODO: change to std::map<int, std::vector<Acts::GbtsConnection> >
  // m_connMap; or   std::map<int,
  // std::vector<std::unique_ptr<Acts::GbtsConnection>> > m_connMap;
};

}  // namespace Acts

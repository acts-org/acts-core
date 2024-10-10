// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO: update to C++17 style
#include "Acts/TrackFinding/GbtsConnector.hpp"

#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <unordered_map>

namespace Acts {

GbtsConnection::GbtsConnection(unsigned int s, unsigned int d)
    : m_src(s), m_dst(d) {}

GbtsConnector::GbtsConnector(std::ifstream &inFile) {
  m_layerGroups.clear();

  int nLinks{};

  inFile >> nLinks >> m_etaBin;

  for (int l = 0; l < nLinks; l++) {
    unsigned int stage{}, lIdx{}, src{}, dst{}, nEntries{};
    int height{}, width{};

    inFile >> lIdx >> stage >> src >> dst >> height >> width >> nEntries;

    GbtsConnection *pC = new GbtsConnection(src, dst);

    int dummy{};

    for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
        inFile >> dummy;  // pC->m_binTable[j+i*width];
      }
    }

    int vol_id = src / 1000;

    if (vol_id == 13 || vol_id == 12 || vol_id == 14) {
      delete pC;
      continue;
    }

    vol_id = dst / 1000;

    if (vol_id == 13 || vol_id == 12 || vol_id == 14) {
      delete pC;
      continue;
    }

    std::map<int, std::vector<GbtsConnection *>>::iterator it =
        m_connMap.find(stage);

    if (it == m_connMap.end()) {
      std::vector<GbtsConnection *> v(1, pC);
      m_connMap.insert(std::make_pair(stage, v));
    } else {
      (*it).second.push_back(pC);
    }
  }

  // re-arrange the connection stages

  std::list<const GbtsConnection *> lConns;

  std::map<int, std::vector<const GbtsConnection *>> newConnMap;

  for (const auto &conn : m_connMap) {
    std::copy(conn.second.begin(), conn.second.end(),
              std::back_inserter(lConns));
  }

  int stageCounter = 0;

  while (!lConns.empty()) {
    std::unordered_map<unsigned int, std::pair<int, int>>
        mCounter;  // layerKey, nDst, nSrc

    for (const auto &conn : lConns) {
      auto entryIt = mCounter.find(conn->m_dst);
      if (entryIt != mCounter.end()) {
        (*entryIt).second.first++;
      } else {
        int nDst = 1;
        int nSrc = 0;
        mCounter.insert(
            std::make_pair(conn->m_dst, std::make_pair(nDst, nSrc)));
      }

      entryIt = mCounter.find(conn->m_src);
      if (entryIt != mCounter.end()) {
        (*entryIt).second.second++;
      } else {
        int nDst = 0;
        int nSrc = 1;
        mCounter.insert(
            std::make_pair(conn->m_src, std::make_pair(nDst, nSrc)));
      }
    }

    // find layers with nSrc = 0

    std::set<unsigned int> zeroLayers;

    for (const auto &layerCounts : mCounter) {
      if (layerCounts.second.second != 0) {
        continue;
      }

      zeroLayers.insert(layerCounts.first);
    }

    // remove connections which use zeroLayer as destination

    std::vector<const GbtsConnection *> theStage;

    std::list<const GbtsConnection *>::iterator cIt = lConns.begin();

    while (cIt != lConns.end()) {
      if (zeroLayers.contains((*cIt)->m_dst)) {
        theStage.push_back(*cIt);
        cIt = lConns.erase(cIt);
        continue;
      }
      cIt++;
    }
    newConnMap.insert(std::make_pair(stageCounter, theStage));
    stageCounter++;
  }

  // create layer groups

  int currentStage = 0;

  // the doublet making is done using "outside-in" approach hence the reverse
  // iterations

  for (std::map<int, std::vector<const GbtsConnection *>>::reverse_iterator it =
           newConnMap.rbegin();
       it != newConnMap.rend(); ++it, currentStage++) {
    const std::vector<const GbtsConnection *> &vConn = (*it).second;

    // loop over links, extract all connections for the stage, group sources by
    // L1 (dst) index

    std::map<unsigned int, std::vector<const GbtsConnection *>> l1ConnMap;

    for (const auto *conn : vConn) {
      unsigned int dst = conn->m_dst;

      std::map<unsigned int, std::vector<const GbtsConnection *>>::iterator
          l1MapIt = l1ConnMap.find(dst);
      if (l1MapIt != l1ConnMap.end()) {
        (*l1MapIt).second.push_back(conn);
      } else {
        std::vector<const GbtsConnection *> v = {conn};
        l1ConnMap.insert(std::make_pair(dst, v));
      }
    }

    std::vector<LayerGroup> lgv;

    lgv.reserve(l1ConnMap.size());

    for (const auto &l1Group : l1ConnMap) {
      lgv.push_back(LayerGroup(l1Group.first, l1Group.second));
    }

    m_layerGroups.insert(std::make_pair(currentStage, lgv));
  }

  newConnMap.clear();
}

GbtsConnector::~GbtsConnector() {
  m_layerGroups.clear();
  for (std::map<int, std::vector<GbtsConnection *>>::iterator it =
           m_connMap.begin();
       it != m_connMap.end(); ++it) {
    for (std::vector<GbtsConnection *>::iterator cIt = (*it).second.begin();
         cIt != (*it).second.end(); ++cIt) {
      delete (*cIt);
    }
  }
}
}  // namespace Acts

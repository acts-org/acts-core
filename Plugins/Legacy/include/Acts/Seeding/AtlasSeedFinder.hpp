// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

///////////////////////////////////////////////////////////////////
// AtlasSeedFinder.hpp Acts project
///////////////////////////////////////////////////////////////////

#pragma once
#include "Acts/Seeding/LegacyInternalSeed.hpp"
#include "Acts/Seeding/SPForSeed.hpp"

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Legacy {
template <typename SpacePoint>
class AtlasSeedFinder {
  struct Config {
    // UNIT AS RETURNED BY m_fieldService->getField() default value in ATLAS
    // was
    // 5. Unit is kilo-Tesla
    //  double bFieldInZ = 5.;
    double bFieldInZ = 0.00208;

    double SCT_rMin = 200.;

    double beamPosX = 0;
    double beamPosY = 0;
    double beamPosZ = 0;
    double beamTiltX = 0;
    double beamTiltY = 0;
  };
  ///////////////////////////////////////////////////////////////////
  // Public methods:
  ///////////////////////////////////////////////////////////////////

 public:
  ///////////////////////////////////////////////////////////////////
  // Standard tool methods
  ///////////////////////////////////////////////////////////////////

  AtlasSeedFinder();
  virtual ~AtlasSeedFinder() {
    if (r_index != nullptr) {
      delete[] r_index;
    }
    if (r_map != nullptr) {
      delete[] r_map;
    }
    if (r_Sorted != nullptr) {
      delete[] r_Sorted;
    }

    // Delete seeds
    //
    for (i_seed = l_seeds.begin(); i_seed != l_seeds.end(); ++i_seed) {
      delete *i_seed;
    }
    // Delete space points for reconstruction
    //
    i_spforseed = l_spforseed.begin();
    for (; i_spforseed != l_spforseed.end(); ++i_spforseed) {
      delete *i_spforseed;
    }
    if (m_seedOutput != nullptr) {
      delete m_seedOutput;
    }

    if (m_SP != nullptr) {
      delete[] m_SP;
    }
    if (m_R != nullptr) {
      delete[] m_R;
    }
    if (m_Tz != nullptr) {
      delete[] m_Tz;
    }
    if (m_Er != nullptr) {
      delete[] m_Er;
    }
    if (m_U != nullptr) {
      delete[] m_U;
    }
    if (m_V != nullptr) {
      delete[] m_V;
    }
    if (m_Zo != nullptr) {
      delete[] m_Zo;
    }
    if (m_OneSeeds != nullptr) {
      delete[] m_OneSeeds;
    }
  }

  ///////////////////////////////////////////////////////////////////
  // Methods to initialize tool for new event or region
  ///////////////////////////////////////////////////////////////////

  template <class RandIter>
  void newEvent(int /*iteration*/, RandIter /*spBegin*/, RandIter /*spEnd*/);

  //////////////////////////////////////////////////////////////////
  // Method to initialize seeds production
  //////////////////////////////////////////////////////////////////
  void find3Sp();

  ///////////////////////////////////////////////////////////////////
  // Iterator through seeds pseudo collection produced accordingly
  // methods find
  ///////////////////////////////////////////////////////////////////

  const Seed<SpacePoint>* next();

  ///////////////////////////////////////////////////////////////////
  // Configuration
  ///////////////////////////////////////////////////////////////////

  const Config m_config;

 protected:
  /**    @name Disallow default instantiation, copy, assignment */
  //@{
  AtlasSeedFinder(const AtlasSeedFinder<SpacePoint>&) = delete;
  AtlasSeedFinder<SpacePoint>& operator=(const AtlasSeedFinder<SpacePoint>&) =
      delete;
  //@}
  ///////////////////////////////////////////////////////////////////
  // Protected data and methods
  ///////////////////////////////////////////////////////////////////

  bool m_endlist = false;
  bool m_checketa = false;
  bool m_isvertex = false;
  int m_nprint = 0;
  int m_nlist = 0;
  int m_maxsize = 0;
  int m_state = 0;
  // event number since tool init
  int m_iteration = 0;
  float m_etamin = 0, m_etamax = 0;
  float m_drmin = 0, m_drminv = 0;
  float m_drmax = 0;
  float m_dzdrmin0 = 0;
  float m_dzdrmax0 = 0;
  float m_dzdrmin = 0;
  float m_dzdrmax = 0;
  float m_zmin = 0;
  float m_zmax = 0;
  float m_zminU = 0;
  float m_zmaxU = 0;
  float m_zminB = 0;
  float m_zmaxB = 0;
  float m_ftrig = 0;
  float m_ftrigW = 0;
  // maximum radius of outermost detector element
  float r_rmax = 0;
  // size of one r-slice
  float r_rstep = 0;

  float m_dzver = 0;
  float m_dzdrver = 0;
  float m_diver = 0;
  float m_diverpps = 0;
  float m_diversss = 0;
  float m_divermax = 0;
  float m_dazmax = 0;
  float m_ptmin = 0;
  float m_ipt = 0;
  float m_ipt2 = 0;
  float m_COF = 0;
  float m_K = 0;
  float m_ipt2K = 0;
  float m_ipt2C = 0;
  float m_COFK = 0;
  float m_umax = 0;
  // number of r-slices
  int r_size = 0;
  int r_first = 0;
  int rf_size = 0;
  int rfz_size = 0;
  std::list<SPForSeed<SpacePoint>*>* r_Sorted = nullptr;
  std::list<SPForSeed<SpacePoint>*> rfz_Sorted[583];
  std::list<SPForSeed<SpacePoint>*> l_spforseed;
  typename std::list<SPForSeed<SpacePoint>*>::iterator i_spforseed;
  typename std::list<SPForSeed<SpacePoint>*>::iterator m_rMin;

  int m_nsaz = 0, m_nsazv = 0;
  int m_fNmax = 0, m_fvNmax = 0;
  int m_fNmin = 0, m_fvNmin = 0;
  int m_zMin = 0;
  // m_nr: number of bins used in r_Sorted; r_index: index of all used bins in
  // r_Sorted; r_map is number of SP in each bin in r_Sorted
  int m_nr = 0;
  int* r_index = nullptr;
  int* r_map = nullptr;
  int m_nrfz = 0, rfz_index[583] = {}, rfz_map[583] = {};
  int rfz_b[583] = {}, rfz_t[593] = {}, rfz_ib[583][9] = {},
      rfz_it[583][9] = {};
  float m_sF = 0;

  ///////////////////////////////////////////////////////////////////
  // Tables for 3 space points seeds search
  ///////////////////////////////////////////////////////////////////

  int m_maxsizeSP = 0;
  SPForSeed<SpacePoint>** m_SP = nullptr;
  float* m_Zo = nullptr;
  float* m_Tz = nullptr;
  float* m_R = nullptr;
  float* m_U = nullptr;
  float* m_V = nullptr;
  float* m_Er = nullptr;

  Seed<SpacePoint>* m_seedOutput = nullptr;

  std::list<InternalSeed<SpacePoint>*> l_seeds;
  typename std::list<InternalSeed<SpacePoint>*>::iterator i_seed;
  typename std::list<InternalSeed<SpacePoint>*>::iterator i_seede;

  std::multimap<float, InternalSeed<SpacePoint>*> m_seeds;
  typename std::multimap<float, InternalSeed<SpacePoint>*>::iterator m_seed;

  std::multimap<float, InternalSeed<SpacePoint>*> m_mapOneSeeds;
  InternalSeed<SpacePoint>* m_OneSeeds = nullptr;
  int m_maxOneSize = 0;
  int m_nOneSeeds = 0;
  int m_fillOneSeeds = 0;
  std::vector<std::pair<float, SPForSeed<SpacePoint>*>> m_CmSp;

  ///////////////////////////////////////////////////////////////////
  // Beam geometry
  ///////////////////////////////////////////////////////////////////

  float m_xbeam = 0;  // x-center of beam-axis
  float m_ybeam = 0;  // y-center of beam-axis
  float m_zbeam = 0;  // z-center of beam-axis

  ///////////////////////////////////////////////////////////////////
  // Protected methods
  ///////////////////////////////////////////////////////////////////

  void buildFrameWork();
  void buildBeamFrameWork();

  SPForSeed<SpacePoint>* newSpacePoint(SpacePoint* const& /*sp*/);

  void newOneSeed(SPForSeed<SpacePoint>*& /*p1*/,
                  SPForSeed<SpacePoint>*& /*p2*/,
                  SPForSeed<SpacePoint>*& /*p3*/, float /*z*/, float /*q*/);

  void newOneSeedWithCurvaturesComparison(SPForSeed<SpacePoint>*& /*SPb*/,
                                          SPForSeed<SpacePoint>*& /*SP0*/,
                                          float /*Zob*/);

  void fillSeeds();
  void fillLists();
  void erase();
  void production3Sp();
  void production3Sp(
      typename std::list<SPForSeed<SpacePoint>*>::iterator* /*rb*/,
      typename std::list<SPForSeed<SpacePoint>*>::iterator* /*rbe*/,
      typename std::list<SPForSeed<SpacePoint>*>::iterator* /*rt*/,
      typename std::list<SPForSeed<SpacePoint>*>::iterator* /*rte*/, int /*NB*/,
      int /*NT*/, int& /*nseed*/);

  void findNext();
  bool isZCompatible(float& /*Zv*/);
  void convertToBeamFrameWork(SpacePoint* const& /*sp*/, float* /*r*/);
};

///////////////////////////////////////////////////////////////////
// Inline methods
///////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline const Seed<SpacePoint>* AtlasSeedFinder<SpacePoint>::next() {
  do {
    if (i_seed == i_seede) {
      findNext();
      if (i_seed == i_seede) {
        return nullptr;
      }
    }
    ++i_seed;
  } while (!(*m_seed++).second->set3(*m_seedOutput));
  return (m_seedOutput);
}

template <typename SpacePoint>
inline bool AtlasSeedFinder<SpacePoint>::isZCompatible(float& Zv) {
  if (Zv < m_zminU || Zv > m_zmaxU) {
    return false;
  } else {
    return true;
  }
}

///////////////////////////////////////////////////////////////////
// New space point for seeds
///////////////////////////////////////////////////////////////////

template <typename SpacePoint>
inline SPForSeed<SpacePoint>* AtlasSeedFinder<SpacePoint>::newSpacePoint(
    SpacePoint* const& sp) {
  SPForSeed<SpacePoint>* sps = nullptr;

  float r[3];
  convertToBeamFrameWork(sp, r);

  if (m_checketa) {
    // filter SP outside of eta-range
    float z = (fabs(r[2]) + m_zmax);
    float x = r[0] * m_dzdrmin;
    float y = r[1] * m_dzdrmin;
    if ((z * z) < (x * x + y * y)) {
      return nullptr;
    }
  }

  if (i_spforseed != l_spforseed.end()) {
    sps = (*i_spforseed++);
    sps->set(sp, r);
  } else {
    l_spforseed.push_back((sps = new SPForSeed<SpacePoint>(sp, r)));
    i_spforseed = l_spforseed.end();
  }

  return sps;
}

///////////////////////////////////////////////////////////////////
// Object-function for curvature seeds comparison
///////////////////////////////////////////////////////////////////

class comCurvature {
 public:
  template <typename SpacePoint>
  bool operator()(
      const std::pair<float, Acts::Legacy::SPForSeed<SpacePoint>*>& i1,
      const std::pair<float, Acts::Legacy::SPForSeed<SpacePoint>*>& i2) {
    return i1.first < i2.first;
  }
};
}  // namespace Acts::Legacy
#include "Acts/Seeding/AtlasSeedFinder.ipp"

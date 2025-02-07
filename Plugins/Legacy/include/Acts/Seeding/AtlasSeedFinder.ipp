// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

///////////////////////////////////////////////////////////////////
// AtlasSeedFinder.ipp Acts project
///////////////////////////////////////////////////////////////////

#include <algorithm>
#include <numbers>

///////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////

template <typename SpacePoint>
Acts::Legacy::AtlasSeedFinder<SpacePoint>::AtlasSeedFinder() {
  m_checketa = false;
  m_maxsize = 50000;
  m_ptmin = 400.;
  m_etamin = 0.;
  m_etamax = 2.7;
  // delta R minimum & maximum within a seed
  m_drmin = 5.;
  m_drminv = 20.;
  m_drmax = 270.;
  // restrict z coordinate of particle origin
  m_zmin = -250.;
  m_zmax = +250.;
  // radius of detector in mm
  r_rmax = 600.;
  r_rstep = 2.;

  // checking if Z is compatible:
  // m_dzver is related to delta-Z
  // m_dzdrver is related to delta-Z divided by delta-R
  m_dzver = 5.;
  m_dzdrver = .02;

  // shift all spacepoints by this offset such that the beam can be assumed to
  // be at 0,0
  // z shift should not matter as beam is assumed to be parallel to central
  // detector axis,
  // but spacepoints will be shifted by z as well anyway.
  m_xbeam = 0.;
  m_ybeam = 0.;
  m_zbeam = 0.;

  // config
  // max impact parameters
  // m_diver = max ppp impact params
  m_diver = 10.;
  m_diverpps = 1.7;
  m_diversss = 50;
  m_divermax = 20.;
  // delta azimuth (phi)
  m_dazmax = .02;
  // limit for sp compatible with 1 middle space point
  // ridiculously large to be EXTRA safe
  // only actually keep 5 of these max 5000 (m_maxOneSize of m_maxsizeSP)
  m_maxsizeSP = 5000;
  m_maxOneSize = 5;

  // cache: counting if ran already
  m_state = 0;

  m_nlist = 0;
  m_endlist = true;
  r_Sorted = nullptr;
  r_index = nullptr;
  r_map = nullptr;
  m_SP = nullptr;
  m_R = nullptr;
  m_Tz = nullptr;
  m_Er = nullptr;
  m_U = nullptr;
  m_V = nullptr;
  m_Zo = nullptr;
  m_OneSeeds = nullptr;
  m_seedOutput = nullptr;

  // Build framework
  //
  buildFrameWork();
  m_CmSp.reserve(500);
}

///////////////////////////////////////////////////////////////////
// Initialize tool for new event
///////////////////////////////////////////////////////////////////
template <typename SpacePoint>
template <class RandIter>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::newEvent(int iteration,
                                                         RandIter spBegin,
                                                         RandIter spEnd) {
  iteration <= 0 ? m_iteration = 0 : m_iteration = iteration;
  erase();
  m_dzdrmin = m_dzdrmin0;
  m_dzdrmax = m_dzdrmax0;
  m_umax = 100.;
  // if first event
  r_first = 0;
  if (!m_iteration) {
    buildBeamFrameWork();

    // curvature depending on bfield
    m_K = 2. / (300. * m_config.bFieldInZ);
    // curvature of minimum pT track
    m_ipt2K = m_ipt2 / (m_K * m_K);
    // scattering of min pT track
    m_ipt2C = m_ipt2 * m_COF;
    // scattering times curvature (missing: div by pT)
    m_COFK = m_COF * (m_K * m_K);
    i_spforseed = l_spforseed.begin();
  }
  // only if not first event
  else {
    fillLists();
    return;
  }

  float irstep = 1. / r_rstep;
  int irmax = r_size - 1;
  // TODO using 3 member vars to clear r_Sorted
  for (int i = 0; i != m_nr; ++i) {
    int n = r_index[i];
    r_map[n] = 0;
    r_Sorted[n].clear();
  }
  m_nr = 0;

  // convert space-points to SPForSeed and sort into radius-binned array
  // r_Sorted
  // store number of SP per bin in r_map
  RandIter sp = spBegin;
  for (; sp != spEnd; ++sp) {
    Acts::Legacy::SPForSeed<SpacePoint>* sps = newSpacePoint((*sp));
    if (!sps) {
      continue;
    }
    int ir = static_cast<int>(sps->radius() * irstep);
    if (ir > irmax) {
      ir = irmax;
    }
    r_Sorted[ir].push_back(sps);
    // if there is exactly one SP in current bin, add bin nr in r_index (for
    // deletion later)
    // TODO overly complicated
    ++r_map[ir];
    if (r_map[ir] == 1) {
      r_index[m_nr++] = ir;
    }
  }

  fillLists();
}

///////////////////////////////////////////////////////////////////
// Methods to initialize different strategies of seeds production
// with three space points with or without vertex constraint
///////////////////////////////////////////////////////////////////

template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::find3Sp() {
  m_zminU = m_zmin;
  m_zmaxU = m_zmax;

  if ((m_state == 0) || (m_nlist != 0)) {
    i_seede = l_seeds.begin();
    m_state = 1;
    m_nlist = 0;
    m_endlist = true;
    m_fvNmin = 0;
    m_fNmin = 0;
    m_zMin = 0;
    production3Sp();
  }
  i_seed = l_seeds.begin();
  m_seed = m_seeds.begin();
}

///////////////////////////////////////////////////////////////////
// Find next set space points
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::findNext() {
  if (m_endlist) {
    return;
  }

  i_seede = l_seeds.begin();

  production3Sp();

  i_seed = l_seeds.begin();
  m_seed = m_seeds.begin();
  ++m_nlist;
}

///////////////////////////////////////////////////////////////////
// Initiate framework for seed generator
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::buildFrameWork() {
  m_ptmin = std::abs(m_ptmin);

  if (m_ptmin < 100.) {
    m_ptmin = 100.;
  }

  if (m_diversss < m_diver) {
    m_diversss = m_diver;
  }
  if (m_divermax < m_diversss) {
    m_divermax = m_diversss;
  }

  if (std::abs(m_etamin) < 0.1) {
    m_etamin = -m_etamax;
  }
  m_dzdrmax0 = 1. / tan(2. * atan(exp(-m_etamax)));
  m_dzdrmin0 = 1. / tan(2. * atan(exp(-m_etamin)));

  // scattering factor. depends on error, forward direction and distance between
  // SP
  m_COF = 134 * .05 * 9.;
  m_ipt = 1. / std::abs(0.9 * m_ptmin);
  m_ipt2 = m_ipt * m_ipt;
  m_K = 0.;

  // set all counters zero
  m_nsaz = m_nsazv = m_nr = m_nrfz = 0;

  // Build radius sorted containers
  //
  r_size = static_cast<int>((r_rmax + .1) / r_rstep);
  r_Sorted = new std::list<Acts::Legacy::SPForSeed<SpacePoint>*>[r_size];
  r_index = new int[r_size];
  r_map = new int[r_size];
  for (int i = 0; i != r_size; ++i) {
    r_index[i] = 0;
    r_map[i] = 0;
  }

  // Build radius-azimuthal sorted containers
  //
  const float pi2 = 2. * std::numbers::pi;
  const int NFmax = 53;
  const float sFmax = static_cast<float>(NFmax) / pi2;
  const float m_sFmin = 100. / 60.;
  // make phi-slices for 400MeV tracks, unless ptMin is even smaller
  float ptm = 400.;
  if (m_ptmin < ptm) {
    ptm = m_ptmin;
  }

  m_sF = ptm / 60.;
  if (m_sF > sFmax) {
    m_sF = sFmax;
  } else if (m_sF < m_sFmin) {
    m_sF = m_sFmin;
  }
  m_fNmax = static_cast<int>(pi2 * m_sF);
  if (m_fNmax >= NFmax) {
    m_fNmax = NFmax - 1;
  }

  // Build radius-azimuthal-Z sorted containers
  //
  m_nrfz = 0;
  for (int i = 0; i != 583; ++i) {
    rfz_index[i] = 0;
    rfz_map[i] = 0;
  }

  // Build maps for radius-azimuthal-Z sorted collections
  //
  for (int f = 0; f <= m_fNmax; ++f) {
    int fb = f - 1;
    if (fb < 0) {
      fb = m_fNmax;
    }
    int ft = f + 1;
    if (ft > m_fNmax) {
      ft = 0;
    }

    // For each azimuthal region loop through all Z regions
    //
    for (int z = 0; z != 11; ++z) {
      int a = f * 11 + z;
      int b = fb * 11 + z;
      int c = ft * 11 + z;
      rfz_b[a] = 3;
      rfz_t[a] = 3;
      rfz_ib[a][0] = a;
      rfz_it[a][0] = a;
      rfz_ib[a][1] = b;
      rfz_it[a][1] = b;
      rfz_ib[a][2] = c;
      rfz_it[a][2] = c;
      if (z == 5) {
        rfz_t[a] = 9;
        rfz_it[a][3] = a + 1;
        rfz_it[a][4] = b + 1;
        rfz_it[a][5] = c + 1;
        rfz_it[a][6] = a - 1;
        rfz_it[a][7] = b - 1;
        rfz_it[a][8] = c - 1;
      } else if (z > 5) {
        rfz_b[a] = 6;
        rfz_ib[a][3] = a - 1;
        rfz_ib[a][4] = b - 1;
        rfz_ib[a][5] = c - 1;

        if (z < 10) {
          rfz_t[a] = 6;
          rfz_it[a][3] = a + 1;
          rfz_it[a][4] = b + 1;
          rfz_it[a][5] = c + 1;
        }
      } else {
        rfz_b[a] = 6;
        rfz_ib[a][3] = a + 1;
        rfz_ib[a][4] = b + 1;
        rfz_ib[a][5] = c + 1;

        if (z > 0) {
          rfz_t[a] = 6;
          rfz_it[a][3] = a - 1;
          rfz_it[a][4] = b - 1;
          rfz_it[a][5] = c - 1;
        }
      }

      if (z == 3) {
        rfz_b[a] = 9;
        rfz_ib[a][6] = a + 2;
        rfz_ib[a][7] = b + 2;
        rfz_ib[a][8] = c + 2;
      } else if (z == 7) {
        rfz_b[a] = 9;
        rfz_ib[a][6] = a - 2;
        rfz_ib[a][7] = b - 2;
        rfz_ib[a][8] = c - 2;
      }
    }
  }

  if (!m_SP) {
    m_SP = new Acts::Legacy::SPForSeed<SpacePoint>*[m_maxsizeSP];
  }
  if (m_R == nullptr) {
    m_R = new float[m_maxsizeSP];
  }
  if (m_Tz == nullptr) {
    m_Tz = new float[m_maxsizeSP];
  }
  if (m_Er == nullptr) {
    m_Er = new float[m_maxsizeSP];
  }
  if (m_U == nullptr) {
    m_U = new float[m_maxsizeSP];
  }
  if (m_V == nullptr) {
    m_V = new float[m_maxsizeSP];
  }
  if (m_Zo == nullptr) {
    m_Zo = new float[m_maxsizeSP];
  }
  if (!m_OneSeeds) {
    m_OneSeeds = new Acts::Legacy::InternalSeed<SpacePoint>[m_maxOneSize];
  }

  if (!m_seedOutput) {
    m_seedOutput = new Acts::Legacy::Seed<SpacePoint>();
  }

  i_seed = l_seeds.begin();
  i_seede = l_seeds.end();
}

///////////////////////////////////////////////////////////////////
// Initiate beam framework for seed generator
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::buildBeamFrameWork() {
  double bx = m_config.beamPosX;
  double by = m_config.beamPosY;
  double bz = m_config.beamPosZ;

  m_xbeam = static_cast<float>(bx);
  m_ybeam = static_cast<float>(by);
  m_zbeam = static_cast<float>(bz);
}

///////////////////////////////////////////////////////////////////
// Initiate beam framework for seed generator
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::convertToBeamFrameWork(
    SpacePoint* const& sp, float* r) {
  r[0] = static_cast<float>(sp->x) - m_xbeam;
  r[1] = static_cast<float>(sp->y) - m_ybeam;
  r[2] = static_cast<float>(sp->z) - m_zbeam;
}

///////////////////////////////////////////////////////////////////
// Initiate space points seed maker
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::fillLists() {
  const float pi2 = 2. * std::numbers::pi;
  typename std::list<Acts::Legacy::SPForSeed<SpacePoint>*>::iterator r, re;

  int ir0 = 0;
  bool ibl = false;

  r_first = 0;
  if (m_iteration != 0) {
    r_first = static_cast<int>(m_config.SCT_rMin / r_rstep);
  }
  for (int i = r_first; i != r_size; ++i) {
    if (r_map[i] == 0) {
      continue;
    }

    r = r_Sorted[i].begin();
    re = r_Sorted[i].end();

    if (ir0 == 0) {
      ir0 = i;
    }
    // if not 1st event
    if (m_iteration != 0) {
      //
      if (!(*r)->spacepoint->clusterList().second) {
        if (i < 20) {
          ibl = true;
        }
      } else if (ibl) {
        break;
      } else if (i > 175) {
        break;
      }
    }

    for (; r != re; ++r) {
      // Azimuthal (Phi) angle sort
      // bin nr "f" is phi * phi-slices
      float F = (*r)->phi();
      if (F < 0.) {
        F += pi2;
      }

      int f = static_cast<int>(F * m_sF);
      if (f < 0) {
        f = m_fNmax;
      } else if (f > m_fNmax) {
        f = 0;
      }

      int z = 0;
      float Z = (*r)->z();

      // Azimuthal angle and Z-coordinate sort
      // assign z-bin a value between 0 and 10 identifying the z-slice of a
      // space-point
      if (Z > 0.) {
        Z < 250.    ? z = 5
        : Z < 450.  ? z = 6
        : Z < 925.  ? z = 7
        : Z < 1400. ? z = 8
        : Z < 2500. ? z = 9
                    : z = 10;
      } else {
        Z > -250.    ? z = 5
        : Z > -450.  ? z = 4
        : Z > -925.  ? z = 3
        : Z > -1400. ? z = 2
        : Z > -2500. ? z = 1
                     : z = 0;
      }
      // calculate bin nr "n" for self-made r-phi-z sorted 3D array "rfz_Sorted"
      // record number of sp in m_nsaz
      int n = f * 11 + z;
      ++m_nsaz;
      // push back sp into rfz_Sorted, record new number of entries in bin in
      // rfz_map,
      // if 1st entry record non-empty bin in "rfz_index"
      rfz_Sorted[n].push_back(*r);
      if (rfz_map[n]++ == 0) {
        rfz_index[m_nrfz++] = n;
      }
    }
  }
  m_state = 0;
}

///////////////////////////////////////////////////////////////////
// Erase space point information
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::erase() {
  for (int i = 0; i != m_nrfz; ++i) {
    int n = rfz_index[i];
    rfz_map[n] = 0;
    rfz_Sorted[n].clear();
  }

  m_state = 0;
  m_nsaz = 0;
  m_nsazv = 0;
  m_nrfz = 0;
}

///////////////////////////////////////////////////////////////////
// Production 3 space points seeds
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::production3Sp() {
  // if less than 3 sp in total
  if (m_nsaz < 3) {
    return;
  }
  m_seeds.clear();

  // indices for the z-regions array in weird order.
  // ensures creating seeds first for barrel, then left EC, then right EC
  const int ZI[11] = {5, 6, 7, 8, 9, 10, 4, 3, 2, 1, 0};
  typename std::list<Acts::Legacy::SPForSeed<SpacePoint>*>::iterator rt[9],
      rte[9], rb[9], rbe[9];
  int nseed = 0;

  // Loop through all azimuthal regions
  //
  for (int f = m_fNmin; f <= m_fNmax; ++f) {
    // For each azimuthal region loop through all Z regions
    // first for barrel, then left EC, then right EC
    int z = 0;
    if (!m_endlist) {
      z = m_zMin;
    }
    for (; z != 11; ++z) {
      int a = f * 11 + ZI[z];
      if (rfz_map[a] == 0) {
        continue;
      }
      int NB = 0, NT = 0;
      for (int i = 0; i != rfz_b[a]; ++i) {
        int an = rfz_ib[a][i];
        // if bin has no entry: continue
        if (rfz_map[an] == 0) {
          continue;
        }
        // assign begin-pointer and end-pointer of current bin to rb and rbe
        rb[NB] = rfz_Sorted[an].begin();
        rbe[NB++] = rfz_Sorted[an].end();
      }
      for (int i = 0; i != rfz_t[a]; ++i) {
        int an = rfz_it[a][i];
        // if bin has no entry: continue
        if (rfz_map[an] == 0) {
          continue;
        }
        // assign begin-pointer and end-pointer of current bin to rt and rte
        rt[NT] = rfz_Sorted[an].begin();
        rte[NT++] = rfz_Sorted[an].end();
      }
      production3Sp(rb, rbe, rt, rte, NB, NT, nseed);
      if (!m_endlist) {
        m_fNmin = f;
        m_zMin = z;
        return;
      }
    }
  }
  m_endlist = true;
}

///////////////////////////////////////////////////////////////////
// Production 3 space points seeds for full scan
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::production3Sp(
    typename std::list<Acts::Legacy::SPForSeed<SpacePoint>*>::iterator* rb,
    typename std::list<Acts::Legacy::SPForSeed<SpacePoint>*>::iterator* rbe,
    typename std::list<Acts::Legacy::SPForSeed<SpacePoint>*>::iterator* rt,
    typename std::list<Acts::Legacy::SPForSeed<SpacePoint>*>::iterator* rte,
    int NB, int NT, int& nseed) {
  typename std::list<Acts::Legacy::SPForSeed<SpacePoint>*>::iterator r0 = rb[0],
                                                                     r;
  if (!m_endlist) {
    r0 = m_rMin;
    m_endlist = true;
  }

  float ipt2K = m_ipt2K;
  float ipt2C = m_ipt2C;
  float COFK = m_COFK;
  float imaxp = m_diver;
  float imaxs = m_divermax;

  m_CmSp.clear();

  // Loop through all space points
  // first bottom bin used as "current bin" for middle spacepoints
  for (; r0 != rbe[0]; ++r0) {
    m_nOneSeeds = 0;
    m_mapOneSeeds.clear();

    float R = (*r0)->radius();

    const int sur0 = (*r0)->surface();
    float X = (*r0)->x();
    float Y = (*r0)->y();
    float Z = (*r0)->z();
    int Nb = 0;

    // Bottom links production
    //
    for (int i = 0; i != NB; ++i) {
      for (r = rb[i]; r != rbe[i]; ++r) {
        float Rb = (*r)->radius();
        float dR = R - Rb;
        // if deltaR larger than deltaRMax, store spacepoint counter position in
        // rb[i]
        // this is not necessary at all because r is the loop counter (rb never
        // read again)
        if (dR > m_drmax) {
          rb[i] = r;
          continue;
        }
        // if dR is smaller than drmin, stop processing this bottombin
        // because it has no more sp with lower radii
        // OR break because processing PPP and encountered SCT SP
        if (dR < m_drmin ||
            (m_iteration && (*r)->spacepoint->clusterList().second)) {
          break;
        }
        if ((*r)->surface() == sur0) {
          continue;
        }
        // forward direction of SP duplet
        float Tz = (Z - (*r)->z()) / dR;
        float aTz = std::abs(Tz);
        // why also exclude seeds with small pseudorapidity??
        if (aTz < m_dzdrmin || aTz > m_dzdrmax) {
          continue;
        }

        // Comparison with vertices Z coordinates
        // continue if duplet origin not within collision region on z axis
        float Zo = Z - R * Tz;
        if (!isZCompatible(Zo)) {
          continue;
        }
        m_SP[Nb] = (*r);
        if (++Nb == m_maxsizeSP) {
          goto breakb;
        }
      }
    }
  breakb:
    if ((Nb == 0) || Nb == m_maxsizeSP) {
      continue;
    }
    int Nt = Nb;

    // Top links production
    //
    for (int i = 0; i != NT; ++i) {
      for (r = rt[i]; r != rte[i]; ++r) {
        float Rt = (*r)->radius();
        float dR = Rt - R;

        if (dR < m_drmin) {
          rt[i] = r;
          continue;
        }
        if (dR > m_drmax) {
          break;
        }
        //  TODO: is check if in same surface necessary?
        if ((*r)->surface() == sur0) {
          continue;
        }

        float Tz = ((*r)->z() - Z) / dR;
        float aTz = std::abs(Tz);

        if (aTz < m_dzdrmin || aTz > m_dzdrmax) {
          continue;
        }

        // Comparison with vertices Z coordinates
        //
        float Zo = Z - R * Tz;
        if (!isZCompatible(Zo)) {
          continue;
        }
        m_SP[Nt] = (*r);
        if (++Nt == m_maxsizeSP) {
          goto breakt;
        }
      }
    }

  breakt:
    if ((Nt - Nb) == 0) {
      continue;
    }
    float covr0 = (*r0)->covr();
    float covz0 = (*r0)->covz();
    float ax = X / R;
    float ay = Y / R;

    for (int i = 0; i != Nt; ++i) {
      Acts::Legacy::SPForSeed<SpacePoint>* sp = m_SP[i];

      float dx = sp->x() - X;
      float dy = sp->y() - Y;
      float dz = sp->z() - Z;
      // calculate projection fraction of spM->spT vector pointing in same
      // direction as
      // vector origin->spM (x) and projection fraction of spM->spT vector
      // pointing
      // orthogonal to origin->spM (y)
      float x = dx * ax + dy * ay;
      float y = dy * ax - dx * ay;
      // 1/(r*r) = 1/dx*dx+dy*dy = 1/(distance between the two points)^2
      float r2 = 1. / (x * x + y * y);
      // 1/r
      float dr = sqrt(r2);
      float tz = dz * dr;
      if (i < Nb) {
        tz = -tz;
      }

      m_Tz[i] = tz;
      m_Zo[i] = Z - R * tz;
      m_R[i] = dr;
      m_U[i] = x * r2;
      m_V[i] = y * r2;
      m_Er[i] = ((covz0 + sp->covz()) + (tz * tz) * (covr0 + sp->covr())) * r2;
    }
    covr0 *= .5;
    covz0 *= 2.;

    // Three space points comparison
    //
    for (int b = 0; b != Nb; ++b) {
      float Zob = m_Zo[b];
      float Tzb = m_Tz[b];
      float Rb2r = m_R[b] * covr0;
      float Rb2z = m_R[b] * covz0;
      float Erb = m_Er[b];
      float Vb = m_V[b];
      float Ub = m_U[b];
      // Tzb2 = 1/sin^2(theta)
      float Tzb2 = (1. + Tzb * Tzb);
      float sTzb2 = sqrt(Tzb2);
      // CSA  = curvature^2/(pT^2 * sin^2(theta)) * scatteringFactor
      float CSA = Tzb2 * COFK;
      // ICSA =          (1/(pT^2 * sin^2(theta)) * scatteringFactor
      float ICSA = Tzb2 * ipt2C;
      float imax = imaxp;
      if (m_SP[b]->spacepoint->clusterList().second) {
        imax = imaxs;
      }

      for (int t = Nb; t != Nt; ++t) {
        float dT = ((Tzb - m_Tz[t]) * (Tzb - m_Tz[t]) - m_R[t] * Rb2z -
                    (Erb + m_Er[t])) -
                   (m_R[t] * Rb2r) * ((Tzb + m_Tz[t]) * (Tzb + m_Tz[t]));
        if (dT > ICSA) {
          continue;
        }

        float dU = m_U[t] - Ub;
        if (dU == 0.) {
          continue;
        }
        float A = (m_V[t] - Vb) / dU;
        float S2 = 1. + A * A;
        float B = Vb - A * Ub;
        float B2 = B * B;
        // B2/S2=1/helixradius^2
        if (B2 > ipt2K * S2 || dT * S2 > B2 * CSA) {
          continue;
        }

        float Im = std::abs((A - B * R) * R);

        if (Im <= imax) {
          // Add penalty factor dependent on difference between cot(theta) to
          // the quality Im (previously Impact)
          float dr = 0;
          m_R[t] < m_R[b] ? dr = m_R[t] : dr = m_R[b];
          Im += std::abs((Tzb - m_Tz[t]) / (dr * sTzb2));
          // B/sqrt(S2) = 1/helixradius
          m_CmSp.push_back(std::make_pair(B / sqrt(S2), m_SP[t]));
          m_SP[t]->setParam(Im);
        }
      }
      if (!m_CmSp.empty()) {
        newOneSeedWithCurvaturesComparison(m_SP[b], (*r0), Zob);
      }
    }
    fillSeeds();
    nseed += m_fillOneSeeds;
    if (nseed >= m_maxsize) {
      m_endlist = false;
      ++r0;
      m_rMin = r0;
      return;
    }
  }
}

///////////////////////////////////////////////////////////////////
// New 3 space points pro seeds
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::newOneSeed(
    Acts::Legacy::SPForSeed<SpacePoint>*& p1,
    Acts::Legacy::SPForSeed<SpacePoint>*& p2,
    Acts::Legacy::SPForSeed<SpacePoint>*& p3, float z, float q) {
  // if the number of seeds already in m_OneSeeds does not exceed m_maxOneSize
  // then insert the current SP into m_mapOneSeeds and m_OneSeeds.
  if (m_nOneSeeds < m_maxOneSize) {
    m_OneSeeds[m_nOneSeeds].set(p1, p2, p3, z);
    m_mapOneSeeds.insert(std::make_pair(q, m_OneSeeds + m_nOneSeeds));
    ++m_nOneSeeds;
  }
  // if not, check the q(uality) of the LAST seed of m_mapOneSeeds
  // if the quality of the new seed is worse, replace the value this
  // (better) quality-key pointed to with the SP from the new seed.
  // Then, add the new seed a SECOND time to the map with the worse quality as
  // key.
  // Then remove all entries after the newly inserted entry equal to the new
  // seed.
  else {
    typename std::multimap<
        float, Acts::Legacy::InternalSeed<SpacePoint>*>::reverse_iterator l =
        m_mapOneSeeds.rbegin();

    if ((*l).first <= q) {
      return;
    }

    Acts::Legacy::InternalSeed<SpacePoint>* s = (*l).second;
    s->set(p1, p2, p3, z);

    typename std::multimap<
        float, Acts::Legacy::InternalSeed<SpacePoint>*>::iterator i =
        m_mapOneSeeds.insert(std::make_pair(q, s));

    for (++i; i != m_mapOneSeeds.end(); ++i) {
      if ((*i).second == s) {
        m_mapOneSeeds.erase(i);
        return;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////
// New 3 space points pro seeds production
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::
    newOneSeedWithCurvaturesComparison(
        Acts::Legacy::SPForSeed<SpacePoint>*& SPb,
        Acts::Legacy::SPForSeed<SpacePoint>*& SP0, float Zob) {
  // allowed (1/helixradius)-delta between 2 seeds
  const float dC = .00003;

  bool pixb = !SPb->spacepoint->clusterList().second;
  float ub = SPb->quality();
  float u0 = SP0->quality();

  std::sort(m_CmSp.begin(), m_CmSp.end(), Acts::Legacy::comCurvature());
  typename std::vector<
      std::pair<float, Acts::Legacy::SPForSeed<SpacePoint>*>>::iterator j,
      jn, i = m_CmSp.begin(), ie = m_CmSp.end();
  jn = i;

  for (; i != ie; ++i) {
    float u = (*i).second->param();
    float Im = (*i).second->param();

    bool pixt = !(*i).second->spacepoint->clusterList().second;

    const int Sui = (*i).second->surface();
    float Ri = (*i).second->radius();
    float Ci1 = (*i).first - dC;
    float Ci2 = (*i).first + dC;
    float Rmi = 0.;
    float Rma = 0.;
    bool in = false;

    if (!pixb) {
      u -= 400.;
    } else if (pixt) {
      u -= 200.;
    }

    for (j = jn; j != ie; ++j) {
      if (j == i) {
        continue;
      }
      if ((*j).first < Ci1) {
        jn = j;
        ++jn;
        continue;
      }
      if ((*j).first > Ci2) {
        break;
      }
      if ((*j).second->surface() == Sui) {
        continue;
      }
      // Compared seeds should have at least deltaRMin distance
      float Rj = (*j).second->radius();
      if (std::abs(Rj - Ri) < m_drmin) {
        continue;
      }

      if (in) {
        if (Rj > Rma) {
          Rma = Rj;
        } else if (Rj < Rmi) {
          Rmi = Rj;
        } else {
          continue;
        }
        // add 200 to quality if 2 compatible seeds with high deltaR of their
        // spT have been found
        // (i.e. potential track spans 5 surfaces)
        if ((Rma - Rmi) > 20.) {
          u -= 200.;
          break;
        }
      } else {
        // first found compatible seed: add 200 to quality
        in = true;
        Rma = Rmi = Rj;
        u -= 200.;
      }
    }
    // if quality is below threshold, discard seed
    if (u > m_umax) {
      continue;
    }
    // if mixed seed and no compatible seed was found, discard seed
    if (pixb != pixt) {
      if (u > 0. || (u > ub && u > u0 && u > (*i).second->quality())) {
        continue;
      }
    }
    // if sct seed and at least 1 comp seed was found, accept even seeds with
    // larger impact params + cot theta penalty
    // m_diversss < Impact parameters + penalty for cot(theta) difference <
    // m_divermax
    // (if m_diversss set smaller than m_divermax, else m_diversss=m_divermax)
    if (!pixb && Im > m_diversss && u > Im - 500.) {
      continue;
    }

    newOneSeed(SPb, SP0, (*i).second, Zob, u);
  }
  m_CmSp.clear();
}

///////////////////////////////////////////////////////////////////
// Fill seeds
///////////////////////////////////////////////////////////////////
template <class SpacePoint>
void Acts::Legacy::AtlasSeedFinder<SpacePoint>::fillSeeds() {
  m_fillOneSeeds = 0;

  typename std::multimap<float, Acts::Legacy::InternalSeed<SpacePoint>
                                    *>::iterator lf = m_mapOneSeeds.begin(),
                                                 l = m_mapOneSeeds.begin(),
                                                 le = m_mapOneSeeds.end();

  if (l == le) {
    return;
  }

  Acts::Legacy::InternalSeed<SpacePoint>* s = nullptr;

  for (; l != le; ++l) {
    float w = (*l).first;
    s = (*l).second;
    if (l != lf && s->spacepoint0()->radius() < 43. && w > -200.) {
      continue;
    }
    if (!s->setQuality(w)) {
      continue;
    }

    if (i_seede != l_seeds.end()) {
      s = (*i_seede++);
      *s = *(*l).second;
    } else {
      s = new Acts::Legacy::InternalSeed<SpacePoint>(*(*l).second);
      l_seeds.push_back(s);
      i_seede = l_seeds.end();
    }

    if (s->spacepoint0()->spacepoint->clusterList().second) {
      w -= 3000.;
    } else if (s->spacepoint1()->spacepoint->clusterList().second) {
      w -= 2000.;
    } else if (s->spacepoint2()->spacepoint->clusterList().second) {
      w -= 1000.;
    }

    m_seeds.insert(std::make_pair(w, s));
    ++m_fillOneSeeds;
  }
}

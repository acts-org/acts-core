// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO: update to C++17 style
#include "Acts/TrackFinding/RoiDescriptor.hpp"

#include <cmath>
#include <sstream>

namespace Acts {

Acts::RoiDescriptor::RoiDescriptor(double eta, double etaMinus, double etaPlus,
                                   double phi, double phiMinus, double phiPlus,
                                   double zed, double zedMinus, double zedPlus)
    : m_phi(phi),
      m_eta(eta),
      m_zed(zed),
      m_phiMinus(phiMinus),
      m_phiPlus(phiPlus),
      m_etaMinus(etaMinus),  //-4.5
      m_etaPlus(etaPlus),
      m_zedMinus(zedMinus),
      m_zedPlus(zedPlus) {
  // catch in the athena roi code
  //  if ( std::isnan(m_etaPlus)  ) throw std::invalid_argument( "RoiDescriptor:
  //  etaPlus nan" ); if ( std::isnan(m_etaMinus) ) throw std::invalid_argument(
  //  "RoiDescriptor: etaMinus nan" );

  m_drdzMinus = std::tan(2 * std::atan(std::exp(-m_etaMinus)));  //-0.02
  m_drdzPlus = std::tan(2 * std::atan(std::exp(-m_etaPlus)));    // 0.02

  m_dzdrMinus = 1 / m_drdzMinus;  //-45
  m_dzdrPlus = 1 / m_drdzPlus;    // 45
}

Acts::RoiDescriptor::~RoiDescriptor() = default;

}  // namespace Acts

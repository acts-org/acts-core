// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/ParticleData.hpp"

#include "Acts/Definitions/Units.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <ostream>
#include <type_traits>

#include "ParticleDataTable.hpp"

namespace {

// TODO the following functions could be constexpr but we are currently limited
// by `std::find`

static inline std::optional<std::size_t> findIndexByPdg(std::int32_t pdg) {
  auto beg = std::cbegin(kParticlesPdgNumber);
  auto end = std::cend(kParticlesPdgNumber);
  // assumes sorted container of pdg numbers
  auto pos = std::find(beg, end, pdg);
  if (pos == end) {
    return std::nullopt;
  }
  return std::make_optional(std::distance(beg, pos));
}

// Find an element within a data column using sorted pdg numbers as the index.
template <typename ColumnContainer>
static inline auto findByPdg(std::int32_t pdg, const ColumnContainer& column)
    -> std::optional<std::decay_t<decltype(column[0])>> {
  // should be a static_assert, but that seems to fail on LLVM
  assert((std::size(column) == kParticlesCount) && "Inconsistent column size");

  auto index = findIndexByPdg(pdg);
  if (!index) {
    return std::nullopt;
  }
  return column[*index];
}

static constexpr float extractCharge(float value) {
  // convert three charge to regular charge in native units
  return (value / 3.0f) * Acts::UnitConstants::e;
}

static constexpr float extractMass(float value) {
  return value * Acts::UnitConstants::MeV;
}

}  // namespace

std::optional<float> Acts::findCharge(Acts::PdgParticle pdg) {
  const auto charge =
      findByPdg(static_cast<std::int32_t>(pdg), kParticlesThreeCharge);
  if (!charge) {
    return std::nullopt;
  }
  return extractCharge(*charge);
}

std::optional<float> Acts::findMass(Acts::PdgParticle pdg) {
  const auto mass =
      findByPdg(static_cast<std::int32_t>(pdg), kParticlesMassMeV);
  if (!mass) {
    return std::nullopt;
  }
  return extractMass(*mass);
}

std::optional<std::string_view> Acts::findName(Acts::PdgParticle pdg) {
  return findByPdg(static_cast<std::int32_t>(pdg), kParticlesName);
}

std::optional<Acts::ParticleData> Acts::findParticleData(PdgParticle pdg) {
  auto index = findIndexByPdg(pdg);
  if (!index) {
    return std::nullopt;
  }
  ParticleData result;
  result.charge = extractCharge(kParticlesThreeCharge[*index]);
  result.mass = extractMass(kParticlesMassMeV[*index]);
  result.name = kParticlesName[*index];
  return {};
}

std::ostream& Acts::operator<<(std::ostream& os, Acts::PdgParticle pdg) {
  const auto name = Acts::findName(pdg);
  if (name) {
    os << *name;
  } else {
    os << static_cast<std::int32_t>(pdg);
  }
  return os;
}

std::optional<std::string_view> Acts::pdgToShortAbsString(PdgParticle pdg) {
  pdg = makeAbsolutePdgParticle(pdg);
  if (pdg == eElectron) {
    return "e";
  }
  if (pdg == eMuon) {
    return "mu";
  }
  if (pdg == eTau) {
    return "t";
  }
  if (pdg == eGamma) {
    return "g";
  }
  if (pdg == ePionZero) {
    return "pi0";
  }
  if (pdg == ePionPlus) {
    return "pi";
  }
  if (pdg == eKaonPlus) {
    return "K";
  }
  if (pdg == eNeutron) {
    return "n";
  }
  if (pdg == eProton) {
    return "p";
  }
  if (pdg == eLead) {
    return "lead";
  }
  return std::nullopt;
}

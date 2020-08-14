// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ACTFW/Utilities/Options.hpp"
#include "ACTFW/Utilities/OptionsFwd.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Units.hpp"

#include <iostream>

#include "GeantinoRecording.hpp"

namespace po = boost::program_options;
using namespace Acts::UnitLiterals;

namespace FW {

namespace Options {

/// @brief Geant4 specific options
///
/// @param desc The option descrion forward
void addGeant4Options(FW::Options::Description& desc);

/// Read the Geatn4 options and @return a GeantinoRecording::Config
///
/// @tparam vmap_t is the Type of the Parameter map to be read out
///
/// @param variables is the parameter map for the options
///
/// @returns a Config object for the GeantinoRecording
ActsExamples::GeantinoRecording::Config readGeantinoRecordingConfig(
    const Variables& variables);

}  // namespace Options
}  // namespace FW

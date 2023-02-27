// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ExaTrkX/Stages.hpp"

Acts::Loggable::Loggable(const Logger &logger) : m_logger(logger.clone()) {}

const Acts::Logger &Acts::Loggable::logger() const {
  return *m_logger;
}

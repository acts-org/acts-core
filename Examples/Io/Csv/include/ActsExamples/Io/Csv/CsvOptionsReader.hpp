// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Io/Csv/CsvMeasurementReader.hpp"
#include "ActsExamples/Io/Csv/CsvParticleReader.hpp"
#include "ActsExamples/Io/Csv/CsvPlanarClusterReader.hpp"
#include "ActsExamples/Io/Csv/CsvSimHitReader.hpp"
#include "ActsExamples/Io/Csv/CsvSpacePointReader.hpp"
#include "ActsExamples/Utilities/OptionsFwd.hpp"

namespace ActsExamples {
namespace Options {

// There are no additional CSV reader options apart from the
// format-independent, generic input option.

/// Read the CSV particle reader config.
ActsExamples::CsvParticleReader::Config readCsvParticleReaderConfig(
    const Variables& vm);

/// Read the CSV sim hit reader config.
ActsExamples::CsvSimHitReader::Config readCsvSimHitReaderConfig(
    const Variables& vm);

/// Read the CSV space point reader config.
ActsExamples::CsvSpacePointReader::Config readCsvSpacePointReaderConfig(
    const Variables& vm);

/// Read the CSV measurement reader config.
ActsExamples::CsvMeasurementReader::Config readCsvMeasurementReaderConfig(
    const Variables& vm);

/// Read the CSV particle reader config.
ActsExamples::CsvPlanarClusterReader::Config readCsvPlanarClusterReaderConfig(
    const Variables& vm);

}  // namespace Options
}  // namespace ActsExamples

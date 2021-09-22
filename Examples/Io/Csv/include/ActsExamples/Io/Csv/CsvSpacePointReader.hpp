// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/IReader.hpp"

#include <memory>
#include <vector>
#include <string>

namespace ActsExamples {

/// Read in a simhit collection in comma-separated-value format.
///
/// This reads one files per event in the configured input directory. By default
/// it reads files in the current working directory. Files are assumed to be
/// named using the following schema
///
///     event000000001-<stem>.csv
///     event000000002-<stem>.csv
///
/// and each line in the file corresponds to one simhit.
class CsvSpacePointReader final : public IReader {
public:
 struct Config {
   /// Where to read input files from.
   std::string inputDir;
   /// Input filename stem.
   std::string inputStem;
   /// Input space point collection.
   std::string inputCollection;
   /// Output space point collections.
   std::string outputSpacePoints;
 };

 /// Construct the simhit reader.
 ///
 /// @params cfg is the configuration object
 /// @params lvl is the logging level
 CsvSpacePointReader(const Config& cfg, Acts::Logging::Level lvl);

 std::string name() const final override;

 /// Return the available events range.
 std::pair<size_t, size_t> availableEvents() const final override;

 /// Read out data from the input stream.
 ProcessCode read(const ActsExamples::AlgorithmContext& ctx) final override;

private:
 Config m_cfg;
 std::pair<size_t, size_t> m_eventsRange;
 std::unique_ptr<const Acts::Logger> m_logger;

 const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples

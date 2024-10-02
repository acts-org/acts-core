// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"
#include "Acts/Vertexing/Vertex.hpp"

#include <fstream>
#include <iterator>
#include <regex>

namespace Acts::Test {

using namespace Acts::UnitLiterals;
using Covariance = BoundSquareMatrix;

enum VertexCsvData { BeamSpotData, VerticesData, TracksData };

/// @brief Helper struct to store reference vertex related information
struct VertexInfo {
  // The position
  Vector3 position;
  // The covariance
  SquareMatrix3 covariance;
  // Number of tracks
  int nTracks = 0;
  // Weight of first track
  double trk1Weight = 0;
  // Vertex compatibility value of first track
  double trk1Comp = 0;
  // Chi2 of first track
  double trk1Chi2 = 0;
};

inline std::tuple<Vertex, std::vector<VertexInfo>,
                  std::vector<BoundTrackParameters>>
readTracksAndVertexCSV(const std::string& toolString,
                       const std::string& fileBase = "vertexing_event_mu20") {
  const auto beamspotDataPath =
      Acts::Test::getDataPath(fileBase + "_beamspot.csv");
  const auto tracksDataPath = Acts::Test::getDataPath(fileBase + "_tracks.csv");
  const auto verticesDataPath =
      Acts::Test::getDataPath(fileBase + "_vertices_" + toolString + ".csv");

  const std::regex comma(",");

  // Open source files
  std::ifstream beamspotData(beamspotDataPath);
  std::ifstream tracksData(tracksDataPath);
  std::ifstream verticesData(verticesDataPath);

  // String to store the read lines
  std::string line{};

  std::shared_ptr<PerigeeSurface> perigeeSurface;
  std::vector<BoundTrackParameters> tracks;
  std::vector<VertexInfo> vertices;
  Vertex beamspotConstraint;

  // Read in beamspot data
  std::getline(beamspotData, line);  // skip header
  while (beamspotData && std::getline(beamspotData, line)) {
    // Tokenize line and store result in vector
    std::vector<std::string> row{
        std::sregex_token_iterator(line.begin(), line.end(), comma, -1),
        std::sregex_token_iterator()};

    Vector3 beamspotPos;
    SquareMatrix3 beamspotCov;
    beamspotPos << std::stod(row[0]) * (1_mm), std::stod(row[1]) * (1_mm),
        std::stod(row[2]) * (1_mm);
    beamspotCov << std::stod(row[3]), 0, 0, 0, std::stod(row[4]), 0, 0, 0,
        std::stod(row[5]);
    beamspotConstraint.setPosition(beamspotPos);
    beamspotConstraint.setCovariance(beamspotCov);
    perigeeSurface = Surface::makeShared<PerigeeSurface>(beamspotPos);
  }

  // Read in track data
  std::getline(tracksData, line);  // skip header
  while (tracksData && std::getline(tracksData, line)) {
    // Tokenize line and store result in vector
    std::vector<std::string> row{
        std::sregex_token_iterator(line.begin(), line.end(), comma, -1),
        std::sregex_token_iterator()};

    BoundVector params;
    params << std::stod(row[0]), std::stod(row[1]), std::stod(row[2]),
        std::stod(row[3]), std::stod(row[4]) * 1. / (1_MeV), std::stod(row[5]);
    Covariance covMat;
    covMat << std::stod(row[6]), std::stod(row[7]), std::stod(row[8]),
        std::stod(row[9]), std::stod(row[10]) * 1. / (1_MeV),
        std::stod(row[11]), std::stod(row[7]), std::stod(row[12]),
        std::stod(row[13]), std::stod(row[14]),
        std::stod(row[15]) * 1. / (1_MeV), std::stod(row[16]),
        std::stod(row[8]), std::stod(row[13]), std::stod(row[17]),
        std::stod(row[18]), std::stod(row[19]) * 1. / (1_MeV),
        std::stod(row[20]), std::stod(row[9]), std::stod(row[14]),
        std::stod(row[18]), std::stod(row[21]),
        std::stod(row[22]) * 1. / (1_MeV), std::stod(row[23]),
        std::stod(row[10]) * 1. / (1_MeV), std::stod(row[15]) * 1. / (1_MeV),
        std::stod(row[19]) * 1. / (1_MeV), std::stod(row[22]) * 1. / (1_MeV),
        std::stod(row[24]) * 1. / (1_MeV * 1_MeV),
        std::stod(row[25]) * 1. / (1_MeV), std::stod(row[11]),
        std::stod(row[16]), std::stod(row[20]), std::stod(row[23]),
        std::stod(row[25]) * 1. / (1_MeV), std::stod(row[26]);

    // TODO we do not have a hypothesis at hand here. defaulting to pion
    tracks.emplace_back(perigeeSurface, params, std::move(covMat),
                        ParticleHypothesis::pion());
  }

  // Read in reference vertex data
  std::getline(verticesData, line);  // skip header
  while (verticesData && std::getline(verticesData, line)) {
    // Tokenize line and store result in vector
    std::vector<std::string> row{
        std::sregex_token_iterator(line.begin(), line.end(), comma, -1),
        std::sregex_token_iterator()};

    Vector3 pos;
    pos << std::stod(row[0]) * (1_mm), std::stod(row[1]) * (1_mm),
        std::stod(row[2]) * (1_mm);
    SquareMatrix3 cov;
    cov << std::stod(row[3]), std::stod(row[4]), std::stod(row[5]),
        std::stod(row[6]), std::stod(row[7]), std::stod(row[8]),
        std::stod(row[9]), std::stod(row[10]), std::stod(row[11]);
    VertexInfo vertexInfo;
    vertexInfo.position = pos;
    vertexInfo.covariance = cov;
    vertexInfo.nTracks = std::stoi(row[12]);
    vertexInfo.trk1Weight = std::stod(row[13]);
    vertexInfo.trk1Comp = std::stod(row[14]);
    vertexInfo.trk1Chi2 = std::stod(row[15]);
    vertices.push_back(vertexInfo);
  }

  return std::make_tuple(beamspotConstraint, vertices, tracks);
}

}  // namespace Acts::Test

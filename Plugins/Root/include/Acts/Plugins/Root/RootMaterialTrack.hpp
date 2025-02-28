// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Material/MaterialInteraction.hpp"

#include <memory>

namespace ROOT {
namespace Experimental {
class RNTupleModel;
}  // namespace Experimental
}  // namespace ROOT

class TTree;
class TChain;

namespace Acts {

/// Common I/O representation of Acts::RecordedMaterialTrack from ROOT
///
/// This holds the columnar information of the material track which can
/// then be used for reading and writing, either with TTree or with
/// ROOT's next generation RNTuple I/O system.
class RootMaterialTrack {
 public:
  /// @brief Configuration struct for the material reading/writing
  struct Config {
    /// Re-calculate total values from individual steps (for cross-checks)
    bool recalculateTotals = false;
    /// Write aut pre and post step (for G4), otherwise central step position
    bool prePostStep = false;
    /// Write the surface to which the material step correpond
    bool storeSurface = false;
    /// Write the volume to which the material step correpond
    bool storeVolume = false;
    /// Collapse consecutive interactions of a single surface into a single
    /// interaction
    bool collapseInteractions = false;
    /// Read option
    bool readCachedSurfaceInformation = false;
  };

  /// @brief extra information to be written out
  struct Auxiliaries {
    std::uint32_t eventId = 0;
  };

  /// @brief  Internal payload representation
  struct Payload {
    /// Event identifier.
    std::shared_ptr<std::uint32_t> eventId = std::make_shared<std::uint32_t>(0);

    /// start global x
    std::shared_ptr<float> vX = std::make_shared<float>(0.);
    /// start global y
    std::shared_ptr<float> vY = std::make_shared<float>(0.);
    /// start global z
    std::shared_ptr<float> vZ = std::make_shared<float>(0.);
    /// start global momentum x
    std::shared_ptr<float> vPX = std::make_shared<float>(0.);
    /// start global momentum y
    std::shared_ptr<float> vPY = std::make_shared<float>(0.);
    /// start global momentum z
    std::shared_ptr<float> vPZ = std::make_shared<float>(0.);
    /// start phi direction
    std::shared_ptr<float> vPPhi = std::make_shared<float>(0.);
    /// start eta direction
    std::shared_ptr<float> vPEta = std::make_shared<float>(0.);
    /// thickness in X0/L0
    std::shared_ptr<float> tX0 = std::make_shared<float>(0.);
    /// thickness in X0/L0
    std::shared_ptr<float> tL0 = std::make_shared<float>(0.);

    /// step x (start) position (optional)
    std::shared_ptr<std::vector<float>> stepXs =
        std::make_shared<std::vector<float>>();
    /// step y (start) position (optional)
    std::shared_ptr<std::vector<float>> stepYs =
        std::make_shared<std::vector<float>>();
    /// step z (start) position (optional)
    std::shared_ptr<std::vector<float>> stepZs =
        std::make_shared<std::vector<float>>();
    /// step x position
    std::shared_ptr<std::vector<float>> stepX =
        std::make_shared<std::vector<float>>();
    /// step y position
    std::shared_ptr<std::vector<float>> stepY =
        std::make_shared<std::vector<float>>();
    /// step z position
    std::shared_ptr<std::vector<float>> stepZ =
        std::make_shared<std::vector<float>>();
    /// step r position
    std::shared_ptr<std::vector<float>> stepR =
        std::make_shared<std::vector<float>>();
    /// step x (end) position (optional)
    std::shared_ptr<std::vector<float>> stepXe =
        std::make_shared<std::vector<float>>();
    /// step y (end) position (optional)
    std::shared_ptr<std::vector<float>> stepYe =
        std::make_shared<std::vector<float>>();
    /// step z (end) position (optional)
    std::shared_ptr<std::vector<float>> stepZe =
        std::make_shared<std::vector<float>>();
    /// step x direction
    std::shared_ptr<std::vector<float>> stepDX =
        std::make_shared<std::vector<float>>();
    /// step y direction
    std::shared_ptr<std::vector<float>> stepDY =
        std::make_shared<std::vector<float>>();
    /// step z direction
    std::shared_ptr<std::vector<float>> stepDZ =
        std::make_shared<std::vector<float>>();
    /// step length
    std::shared_ptr<std::vector<float>> stepLength =
        std::make_shared<std::vector<float>>();
    /// step material x0
    std::shared_ptr<std::vector<float>> matX0 =
        std::make_shared<std::vector<float>>();
    /// step material l0
    std::shared_ptr<std::vector<float>> matL0 =
        std::make_shared<std::vector<float>>();
    /// step material A
    std::shared_ptr<std::vector<float>> matA =
        std::make_shared<std::vector<float>>();
    /// step material Z
    std::shared_ptr<std::vector<float>> matZ =
        std::make_shared<std::vector<float>>();
    /// step material rho
    std::shared_ptr<std::vector<float>> matRho =
        std::make_shared<std::vector<float>>();

    /// ID of the surface associated with the step
    std::shared_ptr<std::vector<std::uint64_t>> surfaceId =
        std::make_shared<std::vector<std::uint64_t>>();
    /// Type of the surface associated with the step
    std::shared_ptr<std::vector<std::int32_t>> surfaceType =
        std::make_shared<std::vector<std::int32_t>>();
    /// x position of the surface intersection associated with the step
    std::shared_ptr<std::vector<float>> surfaceX =
        std::make_shared<std::vector<float>>();
    /// y position of the surface intersection associated with the step
    std::shared_ptr<std::vector<float>> surfaceY =
        std::make_shared<std::vector<float>>();
    /// z position of the surface intersection associated with the step
    std::shared_ptr<std::vector<float>> surfaceZ =
        std::make_shared<std::vector<float>>();
    /// r of the position of the surface intersection associated with the step
    std::shared_ptr<std::vector<float>> surfaceR =
        std::make_shared<std::vector<float>>();
    /// the distance to the surface associated with the step
    std::shared_ptr<std::vector<float>> surfaceDistance =
        std::make_shared<std::vector<float>>();
    /// path correction when associating material to the given surface
    std::shared_ptr<std::vector<float>> surfacePathCorrection =
        std::make_shared<std::vector<float>>();
    /// Min range of the surface associated with the step
    std::shared_ptr<std::vector<float>> surfaceRangeMin =
        std::make_shared<std::vector<float>>();
    /// Max range of the surface associated with the step
    std::shared_ptr<std::vector<float>> surfaceRangeMax =
        std::make_shared<std::vector<float>>();

    /// ID of the volume associated with the step
    std::shared_ptr<std::vector<std::uint64_t>> volumeId =
        std::make_shared<std::vector<std::uint64_t>>();
  };

  /// Constructor
  ///
  /// @param cfg The configuration struct
  RootMaterialTrack(const Config& cfg);

  /// This method initializes for TTree writing
  ///
  /// @param chain The TChain to wread from
  void initializeRead(TChain& tree);

  /// This method initializes for TTree writing
  ///
  /// @param tree The TTree to write to
  void initializeWrite(TTree& tree);

  // This method initializes for RNTuple writing
  ///
  /// @param rntModel The RNTupleModel to connect to
  void initializeWrite(ROOT::Experimental::RNTupleModel& rntModel);

  /// Fill the payload with the material track information
  ///
  /// @param gctx The geometry context
  /// @param rmTrack The recorded material track to fill
  /// @param aux The auxiliary information to fill
  void fill(const GeometryContext& gctx, const RecordedMaterialTrack& rmTrack,
            const Auxiliaries& aux);

  /// Create a RecordedMaterialTrack from the current payload
  /// @note It is up to the caller to move/update the payload in the reader
  RecordedMaterialTrack read() const;

 private:
  /// Clear the payload
  void clear();

  Config m_cfg;       // Configuration struct
  Payload m_payload;  // Paylod object
};

}  // namespace Acts

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
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <numbers>
#include <optional>
#include <ostream>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {
namespace Test {
struct SurfaceArrayCreatorFixture;
}

using SurfaceMatcher =
    std::function<bool(const GeometryContext& gctx, AxisDirection,
                       const Surface*, const Surface*)>;

using SurfaceVector = std::vector<const Surface*>;
using SurfaceMatrix = std::vector<SurfaceVector>;

using V3Vector = std::vector<Vector3>;
using V3Matrix = std::vector<V3Vector>;

using AxisScalar = Vector3::Scalar;

/// @class SurfaceArrayCreator
///
/// It is designed create sub surface arrays to be ordered on Surfaces
///
/// @todo write more documentation on how this is done
class SurfaceArrayCreator {
 public:
  friend struct Acts::Test::SurfaceArrayCreatorFixture;
  friend class Acts::SurfaceArray;

  struct ProtoAxis {
    BinningType bType = BinningType::equidistant;
    AxisDirection axisDir = AxisDirection::AxisX;
    std::size_t nBins = 0;
    AxisScalar min = 0;
    AxisScalar max = 0;
    std::vector<AxisScalar> binEdges;

    std::size_t getBin(AxisScalar x) const {
      if (binEdges.empty()) {
        // equidistant
        AxisScalar w = (max - min) / nBins;
        return static_cast<std::size_t>(std::floor((x - min) / w));
      } else {
        // variable
        const auto it =
            std::upper_bound(std::begin(binEdges), std::end(binEdges), x);
        return std::distance(std::begin(binEdges), it) - 1;
      }
    }
  };

  // Configuration struct
  struct Config {
    /// Type-erased function which determines whether two surfaces are supposed
    /// to be considered equivalent in terms of the binning
    SurfaceMatcher surfaceMatcher = SurfaceArrayCreator::isSurfaceEquivalent;

    /// Optimize the binning in phi for disc layers. Reduces the number
    /// of bins to the lowest number of non-equivalent phi surfaces
    /// of all r-bins. If false, this step is skipped.
    bool doPhiBinningOptimization = true;
  };

  /// Constructor with default config
  ///
  /// @param logger logging instance
  explicit SurfaceArrayCreator(std::unique_ptr<const Logger> logger =
                                   getDefaultLogger("SurfaceArrayCreator",
                                                    Logging::INFO))
      : m_cfg(Config()), m_logger(std::move(logger)) {}
  /// Constructor with explicit config
  ///
  /// @param cfg Explicit config struct
  /// @param logger logging instance
  explicit SurfaceArrayCreator(const Config& cfg,
                               std::unique_ptr<const Logger> logger =
                                   getDefaultLogger("SurfaceArrayCreator",
                                                    Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(logger)) {}

  /// Destructor
  virtual ~SurfaceArrayCreator() = default;

  /// SurfaceArrayCreator interface method
  ///
  /// - create an array in a cylinder, binned in phi, z when extremas and
  /// bin numbers are known
  /// @warning This function requires the cylinder aligned with the z-axis
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// to be ordered on the cylinder
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @param [in] gctx The gometry context for this building call
  /// @param protoLayerOpt The proto layer containing the layer size
  /// @param binsPhi is the number of bins in phi for the surfaces
  /// @param binsZ is the number of bin in Z for the surfaces
  /// @param transform is the (optional) additional transform applied
  ///
  /// @return a unique pointer to a new SurfaceArray
  std::unique_ptr<SurfaceArray> surfaceArrayOnCylinder(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t binsPhi,
      std::size_t binsZ, std::optional<ProtoLayer> protoLayerOpt = std::nullopt,
      const Transform3& transform = Transform3::Identity()) const;

  /// SurfaceArrayCreator interface method
  ///
  /// - create an array in a cylinder, binned in phi, z when extremas and bin
  /// numbers are unknown - this method goes through the surfaces and finds out
  /// the needed information
  /// @warning This function requires the cylinder aligned with the z-axis
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// to be ordered on the cylinder
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @param [in] gctx The gometry context for this building call
  /// @param protoLayerOpt The proto layer containing the layer size
  /// @param bTypePhi the binning type in phi direction (equidistant/arbitrary)
  /// @param bTypeZ the binning type in z direction (equidistant/arbitrary)
  /// @param transform is the (optional) additional transform applied
  ///
  /// @return a unique pointer a new SurfaceArray
  std::unique_ptr<Acts::SurfaceArray> surfaceArrayOnCylinder(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces,
      BinningType bTypePhi = equidistant, BinningType bTypeZ = equidistant,
      std::optional<ProtoLayer> protoLayerOpt = std::nullopt,
      const Transform3& transform = Transform3::Identity()) const;

  /// SurfaceArrayCreator interface method
  /// - create an array on a disc, binned in r, phi when extremas and
  /// bin numbers are known
  ///
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// to be ordered on the disc
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @warning This function requires the disc aligned with the z-axis
  /// @param [in] gctx The gometry context for this building call
  /// @param protoLayerOpt The proto layer containing the layer size
  /// @param binsPhi is the number of bins in phi for the surfaces
  /// @param binsR is the number of bin in R for the surfaces
  /// @param transform is the (optional) additional transform applied
  ///
  /// @return a unique pointer a new SurfaceArray
  std::unique_ptr<SurfaceArray> surfaceArrayOnDisc(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t binsR,
      std::size_t binsPhi,
      std::optional<ProtoLayer> protoLayerOpt = std::nullopt,
      const Transform3& transform = Transform3::Identity()) const;

  /// SurfaceArrayCreator interface method
  ///
  /// - create an array in a cylinder, binned in phi, r when extremas and bin
  /// numbers are unknown - this method goes through the surfaces and finds out
  /// the needed information
  /// @param surfaces is the vector of pointers to sensitive surfaces
  /// to be ordered on the disc
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @warning This function requires the disc aligned with the z-axis
  /// @param [in] gctx The gometry context for this building call
  /// @param protoLayerOpt The proto layer containing the layer size
  /// @param bTypeR the binning type in r direction (equidistant/arbitrary)
  /// @param bTypePhi the binning type in phi direction (equidistant/arbitrary)
  /// @param transform is the (optional) additional transform applied
  ///
  /// @return a unique pointer a new SurfaceArray
  /// @note If there is more than on R-Ring, number of phi bins
  ///       will be set to lowest number of surfaces of any R-ring.
  ///       This ignores bTypePhi and produces equidistant binning in phi
  std::unique_ptr<Acts::SurfaceArray> surfaceArrayOnDisc(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, BinningType bTypeR,
      BinningType bTypePhi,
      std::optional<ProtoLayer> protoLayerOpt = std::nullopt,
      const Transform3& transform = Transform3::Identity()) const;

  /// SurfaceArrayCreator interface method
  /// - create an array on a plane
  ///
  /// @param [in] gctx The gometry context for this building call
  /// @param [in] surfaces is the vector of pointers to sensitive surfaces
  /// to be ordered on the plane
  /// @pre the pointers to the sensitive surfaces in the surfaces vectors all
  /// need to be valid, since no check is performed
  /// @warning This function requires the plane aligned with either the x-, y-
  /// or z-axis
  /// @param [in] bins1 is the number of bins in the orthogonal direction to @p
  /// aDir
  /// @param [in] bins2 is the number of bins in the orthogonal direction to @p
  /// aDir
  /// @param [in] aDir Direction of the aligned surfaces
  /// @param [in] protoLayerOpt Optional @c ProtoLayer instance
  /// @param [in] transform is the (optional) additional transform applied
  ///
  /// @return a unique pointer a new SurfaceArray
  std::unique_ptr<SurfaceArray> surfaceArrayOnPlane(
      const GeometryContext& gctx,
      std::vector<std::shared_ptr<const Surface>> surfaces, std::size_t bins1,
      std::size_t bins2, AxisDirection aDir,
      std::optional<ProtoLayer> protoLayerOpt = std::nullopt,
      const Transform3& transform = Transform3::Identity()) const;

  /// Static check function for surface equivalent
  ///
  /// @param [in] gctx the geometry context for this check
  /// @param aDir the axis direction for the binning
  /// @param a first surface for checking
  /// @param b second surface for checking
  static bool isSurfaceEquivalent(const GeometryContext& gctx,
                                  AxisDirection aDir, const Surface* a,
                                  const Surface* b) {
    using namespace UnitLiterals;
    using VectorHelpers::perp;

    if (aDir == Acts::AxisDirection::AxisPhi) {
      // Take the two binning positions
      Vector3 pos1 = a->referencePosition(gctx, AxisDirection::AxisR);
      Vector3 pos2 = b->referencePosition(gctx, AxisDirection::AxisR);

      // Project them on the (x, y) plane, where Phi angles are calculated
      auto proj1 = pos1.head<2>(), proj2 = pos2.head<2>();

      // Basic dot and cross products identities give us the cosine and sine
      // of these angles, time the squared vector norm
      auto cos_dPhi_n2 = proj1.dot(proj2);
      auto sin_dPhi_n2 = proj1.x() * proj2.y() - proj2.x() * proj1.y();

      // ...so by injecting them into atan2, we get the angle between them
      auto dPhi = std::atan2(sin_dPhi_n2, cos_dPhi_n2);
      return std::abs(dPhi) < std::numbers::pi / 180.;
    }

    if (aDir == Acts::AxisDirection::AxisZ) {
      return (std::abs(a->referencePosition(gctx, AxisDirection::AxisR).z() -
                       b->referencePosition(gctx, AxisDirection::AxisR).z()) <
              1_um);
    }

    if (aDir == Acts::AxisDirection::AxisR) {
      return (std::abs(perp(a->referencePosition(gctx, AxisDirection::AxisR)) -
                       perp(b->referencePosition(gctx, AxisDirection::AxisR))) <
              1_um);
    }

    return false;
  }

  /// Set logging instance
  /// @param logger is the logging instance to be set
  void setLogger(std::unique_ptr<const Logger> logger) {
    m_logger = std::move(logger);
  }

 private:
  /// configuration object
  Config m_cfg;

  /// Private access to logger
  const Logger& logger() const { return *m_logger; }

  std::vector<const Surface*> findKeySurfaces(
      const std::vector<const Surface*>& surfaces,
      const std::function<bool(const Surface*, const Surface*)>& equal) const;

  std::size_t determineBinCount(const GeometryContext& gctx,
                                const std::vector<const Surface*>& surfaces,
                                AxisDirection aDir) const;

  /// SurfaceArrayCreator internal method
  /// Creates a variable @c ProtoAxis from a vector of (unsorted) surfaces with
  /// PlanarBounds
  /// It loops through the surfaces and finds out the needed information
  /// First the surfaces are sorted in the binning direction and the so called
  /// "key" surfaces (surfaces with different positions in the binning
  /// direction) are extracted. The boundary value between two surfaces is the
  /// mean value of the two center position in the binning direction. The first
  /// and the last boundaries are calculated from the vertices of the first and
  /// last surface.
  /// @note currently implemented for phi, r and z bining
  /// @todo implement for x,y binning
  /// @param [in] gctx the geometry context for this call
  /// @param surfaces are the sensitive surfaces to be
  /// @param aDir the AxisDirection in which direction should be binned
  /// (currently possible: AxisPhi, AxisR, AxisZ)
  /// @param protoLayer Instance of @c ProtoLayer holding generic layer info
  /// @param transform is the (optional) additional transform applied
  /// @return Instance of @c ProtoAxis containing determined properties
  /// @note This only creates the @c ProtoAxis, this needs to be turned
  ///       into an actual @c Axis object to be used
  ProtoAxis createVariableAxis(const GeometryContext& gctx,
                               const std::vector<const Surface*>& surfaces,
                               AxisDirection aDir, const ProtoLayer& protoLayer,
                               Transform3& transform) const;

  /// SurfaceArrayCreator internal method
  /// Creates a equidistant @c ProtoAxis when the extremas and the bin number
  /// are
  /// It loops through the surfaces and finds out the needed information
  /// First the surfaces are sorted in the binning direction and the so called
  /// "key" surfaces (surfaces with different positions in the binning
  /// direction) are extracted. The number of key surfaces euqals the number of
  /// bins. Afterwards the minimum and maximum are calculated by
  /// subtracting/adding half of a bin size to the center position (in the
  /// binning direction) to the first/last surface.
  /// @note currently implemented for phi, r and z bining
  /// @todo implement for x,y binning
  /// @param [in] gctx the geometry context for this call
  /// @param surfaces are the sensitive surfaces to be
  /// @param aDir the AxisDirection in which direction should be binned
  /// (currently possible: AxisPhi, AxisR, AxisZ)
  /// @param protoLayer Instance of @c ProtoLayer holding generic layer info
  /// @param transform is the (optional) additional transform applied
  /// @param nBins Number of bins to use, 0 means determine automatically
  /// @return Instance of @c ProtoAxis containing determined properties
  /// @note This only creates the @c ProtoAxis, this needs to be turned
  ///       into an actual @c Axis object to be used
  ProtoAxis createEquidistantAxis(const GeometryContext& gctx,
                                  const std::vector<const Surface*>& surfaces,
                                  AxisDirection aDir,
                                  const ProtoLayer& protoLayer,
                                  Transform3& transform,
                                  std::size_t nBins = 0) const;

  /// SurfaceArrayCreator internal method
  /// @brief Creates a SurfaceGridLookup instance within an any
  /// This is essentially a factory which absorbs some if/else logic
  /// that is required by the templating.
  /// @tparam bdtA AxisBoundaryType of axis A
  /// @tparam bdtB AxisBoundaryType of axis B
  /// @tparam F1 type-deducted value of g2l lambda
  /// @tparam F2 type-deducted value of l2g lambda
  /// @param globalToLocal transform callable
  /// @param localToGlobal transform callable
  /// @param pAxisA ProtoAxis object for axis A
  /// @param pAxisB ProtoAxis object for axis B
  template <AxisBoundaryType bdtA, AxisBoundaryType bdtB, typename F1,
            typename F2>
  static std::unique_ptr<SurfaceArray::ISurfaceGridLookup>
  makeSurfaceGridLookup2D(F1 globalToLocal, F2 localToGlobal, ProtoAxis pAxisA,
                          ProtoAxis pAxisB) {
    using ISGL = SurfaceArray::ISurfaceGridLookup;
    std::unique_ptr<ISGL> ptr;

    // this becomes completely unreadable otherwise
    // clang-format off
    if (pAxisA.bType == equidistant && pAxisB.bType == equidistant) {

      Axis<AxisType::Equidistant, bdtA> axisA(pAxisA.min, pAxisA.max, pAxisA.nBins);
      Axis<AxisType::Equidistant, bdtB> axisB(pAxisB.min, pAxisB.max, pAxisB.nBins);

      using SGL = SurfaceArray::SurfaceGridLookup<decltype(axisA), decltype(axisB)>;
      ptr = std::make_unique<SGL>(
            globalToLocal, localToGlobal, std::pair{axisA, axisB}, std::vector{pAxisA.axisDir, pAxisB.axisDir});

    } else if (pAxisA.bType == equidistant && pAxisB.bType == arbitrary) {

      Axis<AxisType::Equidistant, bdtA> axisA(pAxisA.min, pAxisA.max, pAxisA.nBins);
      Axis<AxisType::Variable, bdtB> axisB(pAxisB.binEdges);

      using SGL = SurfaceArray::SurfaceGridLookup<decltype(axisA), decltype(axisB)>;
      ptr = std::make_unique<SGL>(
            globalToLocal, localToGlobal, std::pair{axisA, axisB}, std::vector{pAxisA.axisDir, pAxisB.axisDir});

    } else if (pAxisA.bType == arbitrary && pAxisB.bType == equidistant) {

      Axis<AxisType::Variable, bdtA> axisA(pAxisA.binEdges);
      Axis<AxisType::Equidistant, bdtB> axisB(pAxisB.min, pAxisB.max, pAxisB.nBins);

      using SGL = SurfaceArray::SurfaceGridLookup<decltype(axisA), decltype(axisB)>;
      ptr = std::make_unique<SGL>(
            globalToLocal, localToGlobal, std::pair{axisA, axisB}, std::vector{pAxisA.axisDir, pAxisB.axisDir});

    } else /*if (pAxisA.bType == arbitrary && pAxisB.bType == arbitrary)*/ {

      Axis<AxisType::Variable, bdtA> axisA(pAxisA.binEdges);
      Axis<AxisType::Variable, bdtB> axisB(pAxisB.binEdges);

      using SGL = SurfaceArray::SurfaceGridLookup<decltype(axisA), decltype(axisB)>;
      ptr = std::make_unique<SGL>(
            globalToLocal, localToGlobal, std::pair{axisA, axisB}, std::vector{pAxisA.axisDir, pAxisB.axisDir});
    }
    // clang-format on

    return ptr;
  }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private helper method to complete the binning
  ///
  ///
  ///  given a grid point o
  ///    |  0  |  1 |  2  |  3 |  4  |
  ///    ------------------------------
  ///  0 |  x  |    |     |    |  x  |
  ///  1 |     |    |  o  |    |     |
  ///  2 |  x  |    |     |    |  x  |
  ///
  /// This is being called when you chose to use more bins thans surfaces
  /// I.e. to put a finer granularity binning onto your surface
  /// Neighbour bins are then filled to contain pointers as well
  /// This method delegates to SurfaceGridLookup itself.
  /// @param [in] gctx the geometry context for this call
  /// @param sl The @c SurfaceGridLookup
  /// @param surfaces the surfaces
  void completeBinning(const GeometryContext& gctx,
                       SurfaceArray::ISurfaceGridLookup& sl,
                       const std::vector<const Surface*>& surfaces) const {
    ACTS_VERBOSE(
        "Complete binning by filling closest neighbour surfaces into "
        "empty bins.");

    std::size_t binCompleted = sl.completeBinning(gctx, surfaces);

    ACTS_VERBOSE("       filled  : " << binCompleted
                                     << " (includes under/overflow)");
  }

  /// Private helper method to transform the  vertices of surface bounds into
  /// global coordinates
  /// @param [in] gctx the geometry context for this call
  /// @param surface the surface associated with the given vertices
  /// @param locVertices a vector of the vertices in local coordinates
  /// @return a vector of the vertices in global coordinates
  std::vector<Acts::Vector3> makeGlobalVertices(
      const GeometryContext& gctx, const Acts::Surface& surface,
      const std::vector<Acts::Vector2>& locVertices) const;
};

}  // namespace Acts

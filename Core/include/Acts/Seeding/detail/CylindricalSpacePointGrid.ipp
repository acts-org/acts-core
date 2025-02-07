// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <concepts>
#include <numbers>

template <typename external_spacepoint_t>
Acts::CylindricalSpacePointGrid<external_spacepoint_t>
Acts::CylindricalSpacePointGridCreator::createGrid(
    const Acts::CylindricalSpacePointGridConfig& config,
    const Acts::CylindricalSpacePointGridOptions& options,
    const Acts::Logger& logger) {
  if (!config.isInInternalUnits) {
    throw std::runtime_error(
        "CylindricalSpacePointGridConfig not in ACTS internal units in "
        "CylindricalSpacePointGridCreator::createGrid");
  }
  if (!options.isInInternalUnits) {
    throw std::runtime_error(
        "CylindricalSpacePointGridOptions not in ACTS internal units in "
        "CylindricalSpacePointGridCreator::createGrid");
  }
  using AxisScalar = Acts::Vector3::Scalar;
  using namespace Acts::UnitLiterals;

  int phiBins = 0;
  // for no magnetic field, create 100 phi-bins
  if (options.bFieldInZ == 0) {
    phiBins = 100;
    ACTS_VERBOSE(
        "B-Field is 0 (z-coordinate), setting the number of bins in phi to "
        << phiBins);
  } else {
    // calculate circle intersections of helix and max detector radius
    float minHelixRadius =
        config.minPt /
        (1_T * 1e6 *
         options.bFieldInZ);  // in mm -> R[mm] =pT[GeV] / (3·10−4×B[T])
                              // = pT[MeV] / (300 *Bz[kT])

    // sanity check: if yOuter takes the square root of a negative number
    if (minHelixRadius < config.rMax * 0.5) {
      throw std::domain_error(
          "The value of minHelixRadius cannot be smaller than rMax / 2. Please "
          "check the configuration of bFieldInZ and minPt");
    }

    float maxR2 = config.rMax * config.rMax;
    float xOuter = maxR2 / (2 * minHelixRadius);
    float yOuter = std::sqrt(maxR2 - xOuter * xOuter);
    float outerAngle = std::atan(xOuter / yOuter);
    // intersection of helix and max detector radius minus maximum R distance
    // from middle SP to top SP
    float innerAngle = 0;
    float rMin = config.rMax;
    if (config.rMax > config.deltaRMax) {
      rMin = config.rMax - config.deltaRMax;
      float innerCircleR2 =
          (config.rMax - config.deltaRMax) * (config.rMax - config.deltaRMax);
      float xInner = innerCircleR2 / (2 * minHelixRadius);
      float yInner = std::sqrt(innerCircleR2 - xInner * xInner);
      innerAngle = std::atan(xInner / yInner);
    }

    // evaluating the azimutal deflection including the maximum impact parameter
    float deltaAngleWithMaxD0 =
        std::abs(std::asin(config.impactMax / (rMin)) -
                 std::asin(config.impactMax / config.rMax));

    // evaluating delta Phi based on the inner and outer angle, and the azimutal
    // deflection including the maximum impact parameter
    // Divide by config.phiBinDeflectionCoverage since we combine
    // config.phiBinDeflectionCoverage number of consecutive phi bins in the
    // seed making step. So each individual bin should cover
    // 1/config.phiBinDeflectionCoverage of the maximum expected azimutal
    // deflection
    float deltaPhi = (outerAngle - innerAngle + deltaAngleWithMaxD0) /
                     config.phiBinDeflectionCoverage;

    // sanity check: if the delta phi is equal to or less than zero, we'll be
    // creating an infinite or a negative number of bins, which would be bad!
    if (deltaPhi <= 0.f) {
      throw std::domain_error(
          "Delta phi value is equal to or less than zero, leading to an "
          "impossible number of bins (negative or infinite)");
    }

    // divide 2pi by angle delta to get number of phi-bins
    // size is always 2pi even for regions of interest
    phiBins = static_cast<int>(std::ceil(2 * std::numbers::pi / deltaPhi));
    // need to scale the number of phi bins accordingly to the number of
    // consecutive phi bins in the seed making step.
    // Each individual bin should be approximately a fraction (depending on this
    // number) of the maximum expected azimutal deflection.

    // set protection for large number of bins, by default it is large
    phiBins = std::min(phiBins, config.maxPhiBins);
  }

  Acts::Axis<AxisType::Equidistant, AxisBoundaryType::Closed> phiAxis(
      config.phiMin, config.phiMax, phiBins);

  // vector that will store the edges of the bins of z
  std::vector<AxisScalar> zValues{};

  // If zBinEdges is not defined, calculate the edges as zMin + bin * zBinSize
  if (config.zBinEdges.empty()) {
    // TODO: can probably be optimized using smaller z bins
    // and returning (multiple) neighbors only in one z-direction for forward
    // seeds
    // FIXME: zBinSize must include scattering
    float zBinSize = config.cotThetaMax * config.deltaRMax;
    float zBins =
        std::max(1.f, std::floor((config.zMax - config.zMin) / zBinSize));

    zValues.reserve(static_cast<int>(zBins));
    for (int bin = 0; bin <= static_cast<int>(zBins); bin++) {
      AxisScalar edge =
          config.zMin + bin * ((config.zMax - config.zMin) / zBins);
      zValues.push_back(edge);
    }

  } else {
    // Use the zBinEdges defined in the config
    zValues.reserve(config.zBinEdges.size());
    for (float bin : config.zBinEdges) {
      zValues.push_back(bin);
    }
  }

  std::vector<AxisScalar> rValues{};
  rValues.reserve(std::max(2ul, config.rBinEdges.size()));
  if (config.rBinEdges.empty()) {
    rValues = {config.rMin, config.rMax};
  } else {
    rValues.insert(rValues.end(), config.rBinEdges.begin(),
                   config.rBinEdges.end());
  }

  Axis<AxisType::Variable, AxisBoundaryType::Open> zAxis(std::move(zValues));
  Axis<AxisType::Variable, AxisBoundaryType::Open> rAxis(std::move(rValues));

  ACTS_VERBOSE("Defining Grid:");
  ACTS_VERBOSE("- Phi Axis: " << phiAxis);
  ACTS_VERBOSE("- Z axis  : " << zAxis);
  ACTS_VERBOSE("- R axis  : " << rAxis);

  return Acts::CylindricalSpacePointGrid<external_spacepoint_t>(
      std::make_tuple(std::move(phiAxis), std::move(zAxis), std::move(rAxis)));
}

template <typename external_spacepoint_t,
          typename external_spacepoint_iterator_t>
void Acts::CylindricalSpacePointGridCreator::fillGrid(
    const Acts::SeedFinderConfig<external_spacepoint_t>& config,
    const Acts::SeedFinderOptions& options,
    Acts::CylindricalSpacePointGrid<external_spacepoint_t>& grid,
    external_spacepoint_iterator_t spBegin,
    external_spacepoint_iterator_t spEnd, const Acts::Logger& logger) {
  if (!config.isInInternalUnits) {
    throw std::runtime_error(
        "SeedFinderConfig not in ACTS internal units in BinnedSPGroup");
  }
  if (config.seedFilter == nullptr) {
    throw std::runtime_error("SeedFinderConfig has a null SeedFilter object");
  }
  if (!options.isInInternalUnits) {
    throw std::runtime_error(
        "SeedFinderOptions not in ACTS internal units in BinnedSPGroup");
  }

  // Space points are assumed to be ALREADY CORRECTED for beamspot position
  // phi, z and r space point selection comes naturally from the
  // grid axis definition. Calling `isInside` will let us know if we are
  // inside the grid range.
  // If a space point is outside the validity range of these quantities
  // it goes in an over- or under-flow bin. We want to avoid to consider those
  // and skip some computations.
  // Additional cuts can be applied by customizing the space point selector
  // in the config object.

  // keep track of changed bins while sorting
  std::vector<bool> usedBinIndex(grid.size(), false);
  std::vector<std::size_t> rBinsIndex;
  rBinsIndex.reserve(grid.size());

  ACTS_VERBOSE("Fetching " << std::distance(spBegin, spEnd)
                           << " space points to the grid");
  std::size_t counter = 0ul;
  for (external_spacepoint_iterator_t it = spBegin; it != spEnd; ++it) {
    const external_spacepoint_t& sp = *it;

    // remove SPs according to experiment specific cuts
    if (!config.spacePointSelector(sp)) {
      continue;
    }

    // fill rbins into grid
    Acts::Vector3 position(sp.phi(), sp.z(), sp.radius());
    if (!grid.isInside(position)) {
      continue;
    }

    std::size_t globIndex = grid.globalBinFromPosition(position);
    auto& rbin = grid.at(globIndex);
    rbin.push_back(&sp);
    ++counter;

    // keep track of the bins we modify so that we can later sort the SPs in
    // those bins only
    if (rbin.size() > 1 && !usedBinIndex[globIndex]) {
      usedBinIndex[globIndex] = true;
      rBinsIndex.push_back(globIndex);
    }
  }

  /// sort SPs in R for each filled bin
  for (std::size_t binIndex : rBinsIndex) {
    auto& rbin = grid.atPosition(binIndex);
    std::ranges::sort(rbin, {}, [](const auto& rb) { return rb->radius(); });
  }

  ACTS_VERBOSE(
      "Number of space points inserted (within grid range): " << counter);
}

template <typename external_spacepoint_t, typename external_collection_t>
  requires std::ranges::range<external_collection_t> &&
           std::same_as<typename external_collection_t::value_type,
                        external_spacepoint_t>
void Acts::CylindricalSpacePointGridCreator::fillGrid(
    const Acts::SeedFinderConfig<external_spacepoint_t>& config,
    const Acts::SeedFinderOptions& options,
    Acts::CylindricalSpacePointGrid<external_spacepoint_t>& grid,
    const external_collection_t& collection, const Acts::Logger& logger) {
  Acts::CylindricalSpacePointGridCreator::fillGrid<external_spacepoint_t>(
      config, options, grid, std::ranges::begin(collection),
      std::ranges::end(collection), logger);
}

// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/GridPortalLink.hpp"

#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"

namespace Acts {

std::unique_ptr<GridPortalLink> GridPortalLink::make(
    const std::shared_ptr<RegularSurface>& surface, TrackingVolume& volume,
    BinningValue direction) {
  std::unique_ptr<GridPortalLink> grid;

  if (const auto* cylinder =
          dynamic_cast<const CylinderSurface*>(surface.get());
      cylinder != nullptr) {
    if (direction == BinningValue::binRPhi) {
      ActsScalar r = cylinder->bounds().get(CylinderBounds::eR);
      if (cylinder->bounds().coversFullAzimuth()) {
        grid = GridPortalLink::make(surface, direction,
                                    Axis{AxisClosed, -M_PI * r, M_PI * r, 1});
      } else {
        ActsScalar hlPhi =
            cylinder->bounds().get(CylinderBounds::eHalfPhiSector);

        grid = GridPortalLink::make(surface, direction,
                                    Axis{AxisBound, -hlPhi * r, hlPhi * r, 1});
      }
    } else if (direction == BinningValue::binZ) {
      ActsScalar hlZ = cylinder->bounds().get(CylinderBounds::eHalfLengthZ);
      grid = GridPortalLink::make(surface, direction,
                                  Axis{AxisBound, -hlZ, hlZ, 1});
    } else {
      throw std::invalid_argument{"Invalid binning direction"};
    }
  } else if (const auto* disc = dynamic_cast<const DiscSurface*>(surface.get());
             disc != nullptr) {
    const auto& bounds = dynamic_cast<const RadialBounds&>(disc->bounds());
    if (direction == BinningValue::binR) {
      ActsScalar minR = bounds.get(RadialBounds::eMinR);
      ActsScalar maxR = bounds.get(RadialBounds::eMaxR);
      grid = GridPortalLink::make(surface, direction,
                                  Axis{AxisBound, minR, maxR, 1});
    } else if (direction == BinningValue::binPhi) {
      if (bounds.coversFullAzimuth()) {
        grid = GridPortalLink::make(surface, direction,
                                    Axis{AxisClosed, -M_PI, M_PI, 1});
      } else {
        ActsScalar hlPhi = bounds.get(RadialBounds::eHalfPhiSector);
        grid = GridPortalLink::make(surface, direction,
                                    Axis{AxisBound, -hlPhi, hlPhi, 1});
      }
    } else {
      throw std::invalid_argument{"Invalid binning direction"};
    }
  } else if (const auto* plane =
                 dynamic_cast<const PlaneSurface*>(surface.get());
             plane != nullptr) {
    throw std::invalid_argument{"Plane surface is not implemented yet"};

  } else {
    throw std::invalid_argument{"Surface type is not supported"};
  }

  assert(grid != nullptr);
  grid->setVolume(&volume);

  return grid;
}

void GridPortalLink::checkConsistency(const CylinderSurface& cyl) const {
  if (cyl.bounds().get(CylinderBounds::eAveragePhi) != 0) {
    throw std::invalid_argument(
        "GridPortalLink: CylinderBounds: only average phi == 0 is "
        "supported. Rotate the cylinder surface.");
  }

  constexpr auto tolerance = s_onSurfaceTolerance;
  auto same = [](auto a, auto b) { return std::abs(a - b) < tolerance; };

  auto checkZ = [&cyl, same](const IAxis& axis) {
    ActsScalar hlZ = cyl.bounds().get(CylinderBounds::eHalfLengthZ);
    if (!same(axis.getMin(), -hlZ) || !same(axis.getMax(), hlZ)) {
      throw std::invalid_argument(
          "GridPortalLink: CylinderBounds: invalid length setup.");
    }
  };
  auto checkRPhi = [&cyl, same](const IAxis& axis) {
    ActsScalar hlPhi = cyl.bounds().get(CylinderBounds::eHalfPhiSector);
    ActsScalar r = cyl.bounds().get(CylinderBounds::eR);
    if (ActsScalar hlRPhi = r * hlPhi;
        !same(axis.getMin(), -hlRPhi) || !same(axis.getMax(), hlRPhi)) {
      throw std::invalid_argument(
          "GridPortalLink: CylinderBounds: invalid phi sector setup: axes "
          "don't match bounds");
    }

    // If full cylinder, make sure axis wraps around
    if (same(hlPhi, M_PI)) {
      if (axis.getBoundaryType() != AxisBoundaryType::Closed) {
        throw std::invalid_argument(
            "GridPortalLink: CylinderBounds: invalid phi sector setup: "
            "axis is not closed.");
      }
    } else {
      if (axis.getBoundaryType() == AxisBoundaryType::Closed) {
        throw std::invalid_argument(
            "GridPortalLink: CylinderBounds: invalid phi sector setup: "
            "axis is closed.");
      }
    }
  };

  if (dim() == 1) {
    const IAxis& axisLoc0 = *grid().axes().front();
    if (direction() == BinningValue::binRPhi) {
      checkRPhi(axisLoc0);
    } else {
      checkZ(axisLoc0);
    }
  } else {  // DIM == 2
    const auto& axisLoc0 = *grid().axes().front();
    const auto& axisLoc1 = *grid().axes().back();
    checkRPhi(axisLoc0);
    checkZ(axisLoc1);
  }
}

void GridPortalLink::checkConsistency(const DiscSurface& disc) const {
  constexpr auto tolerance = s_onSurfaceTolerance;
  auto same = [](auto a, auto b) { return std::abs(a - b) < tolerance; };

  const auto* bounds = dynamic_cast<const RadialBounds*>(&disc.bounds());
  if (bounds == nullptr) {
    throw std::invalid_argument(
        "GridPortalLink: DiscBounds: invalid bounds type.");
  }

  if (bounds->get(RadialBounds::eAveragePhi) != 0) {
    throw std::invalid_argument(
        "GridPortalLink: DiscBounds: only average phi == 0 is supported. "
        "Rotate the disc surface.");
  }

  auto checkR = [&bounds, same](const IAxis& axis) {
    ActsScalar minR = bounds->get(RadialBounds::eMinR);
    ActsScalar maxR = bounds->get(RadialBounds::eMaxR);
    if (!same(axis.getMin(), minR) || !same(axis.getMax(), maxR)) {
      throw std::invalid_argument(
          "GridPortalLink: DiscBounds: invalid radius setup.");
    }
  };

  auto checkPhi = [&bounds, same](const IAxis& axis) {
    ActsScalar hlPhi = bounds->get(RadialBounds::eHalfPhiSector);
    if (!same(axis.getMin(), -hlPhi) || !same(axis.getMax(), hlPhi)) {
      throw std::invalid_argument(
          "GridPortalLink: DiscBounds: invalid phi sector setup.");
    }
    // If full disc, make sure axis wraps around
    if (same(hlPhi, M_PI)) {
      if (axis.getBoundaryType() != Acts::AxisBoundaryType::Closed) {
        throw std::invalid_argument(
            "GridPortalLink: DiscBounds: invalid phi sector setup: axis is "
            "not closed.");
      }
    } else {
      if (axis.getBoundaryType() == Acts::AxisBoundaryType::Closed) {
        throw std::invalid_argument(
            "GridPortalLink: DiscBounds: invalid phi sector setup: axis "
            "is closed.");
      }
    }
  };

  if (dim() == 1) {
    const IAxis& axisLoc0 = *grid().axes().front();
    if (direction() == BinningValue::binR) {
      checkR(axisLoc0);
    } else {
      checkPhi(axisLoc0);
    }
  } else {  // DIM == 2
    const auto& axisLoc0 = *grid().axes().front();
    const auto& axisLoc1 = *grid().axes().back();
    checkR(axisLoc0);
    checkPhi(axisLoc1);
  }
}

void GridPortalLink::printContents(std::ostream& os) const {
  std::size_t dim = grid().axes().size();
  os << "----- GRID " << dim << "d -----" << std::endl;
  os << grid() << " along " << direction() << std::endl;

  auto lpad = [](const std::string& s, std::size_t n) {
    return s.size() < n ? std::string(n - s.size(), ' ') + s : s;
  };

  auto rpad = [](const std::string& s, std::size_t n) {
    return s.size() < n ? s + std::string(n - s.size(), ' ') : s;
  };

  std::string loc0;
  std::string loc1;

  bool flipped = false;

  if (surface().type() == Surface::Cylinder) {
    loc0 = "rPhi";
    loc1 = "z";
    flipped = direction() != BinningValue::binRPhi;
  } else if (surface().type() == Surface::Disc) {
    loc0 = "r";
    loc1 = "phi";
    flipped = direction() != BinningValue::binR;
  } else if (surface().type() == Surface::Plane) {
    loc0 = "x";
    loc1 = "y";
    flipped = direction() != BinningValue::binX;
  } else {
    throw std::invalid_argument{"Unsupported surface type"};
  }

  if (dim == 1) {
    auto loc = numLocalBins();

    if (flipped) {
      os << lpad(loc1, 4) << " > " << lpad("i=0", 10) << " ";
      for (std::size_t i = 1; i <= loc.at(0) + 1; i++) {
        os << lpad("i=" + std::to_string(i), 13) + " ";
      }
      os << std::endl;

      os << std::string(4, ' ');
      for (std::size_t i = 0; i <= loc.at(0) + 1; i++) {
        std::string name = "0x0";
        if (const auto* v = atLocalBins({i}); v != nullptr) {
          name = v->volumeName();
        }
        name = name.substr(0, std::min(name.size(), std::size_t{13}));
        os << lpad(name, 13) << " ";
      }
      os << std::endl;

    } else {
      os << "v " << loc0 << std::endl;
      for (std::size_t i = 0; i <= loc.at(0) + 1; i++) {
        os << "i=" << i << " ";
        std::string name = "0x0";
        if (const auto* v = atLocalBins({i}); v != nullptr) {
          name = v->volumeName();
        }
        name = name.substr(0, std::min(name.size(), std::size_t{13}));
        os << lpad(name, 13) << " ";
        os << std::endl;
      }
    }

  } else {
    auto loc = numLocalBins();
    os << rpad("v " + loc0 + "|" + loc1 + " >", 14) + "j=0 ";
    for (std::size_t j = 1; j <= loc.at(1) + 1; j++) {
      os << lpad("j=" + std::to_string(j), 13) << " ";
    }
    os << std::endl;
    for (std::size_t i = 0; i <= loc.at(0) + 1; i++) {
      os << "i=" << i << " ";
      for (std::size_t j = 0; j <= loc.at(1) + 1; j++) {
        std::string name = "0x0";
        if (const auto* v = atLocalBins({i, j}); v != nullptr) {
          name = v->volumeName();
        }
        name = name.substr(0, std::min(name.size(), std::size_t{13}));
        os << lpad(name, 13) << " ";
      }
      os << std::endl;
    }
  }
}

void GridPortalLink::fillGrid1dTo2d(FillDirection dir,
                                    const GridPortalLink& grid1d,
                                    GridPortalLink& grid2d) {
  const auto locSource = grid1d.numLocalBins();
  const auto locDest = grid2d.numLocalBins();
  assert(locSource.size() == 1);
  assert(locDest.size() == 2);

  for (std::size_t i = 0; i <= locSource[0] + 1; ++i) {
    TrackingVolume* source = grid1d.atLocalBins({i});

    if (dir == FillDirection::loc1) {
      for (std::size_t j = 0; j <= locDest[1] + 1; ++j) {
        grid2d.atLocalBins({i, j}) = source;
      }
    } else if (dir == FillDirection::loc0) {
      for (std::size_t j = 0; j <= locDest[0] + 1; ++j) {
        grid2d.atLocalBins({j, i}) = source;
      }
    }
  }
}

std::unique_ptr<GridPortalLink> GridPortalLink::extendTo2dImpl(
    const std::shared_ptr<CylinderSurface>& surface, const IAxis* other) const {
  assert(dim() == 1);
  if (direction() == BinningValue::binRPhi) {
    const auto& axisRPhi = *grid().axes().front();
    // 1D direction is binRPhi, so add a Z axis
    ActsScalar hlZ = surface->bounds().get(CylinderBounds::eHalfLengthZ);

    auto grid = axisRPhi.visit([&](const auto& axis0) {
      Axis axisZ{AxisBound, -hlZ, hlZ, 1};
      const IAxis* axis = other != nullptr ? other : &axisZ;
      return axis->visit(
          [&surface,
           &axis0](const auto& axis1) -> std::unique_ptr<GridPortalLink> {
            return GridPortalLink::make(surface, axis0, axis1);
          });
    });

    fillGrid1dTo2d(FillDirection::loc1, *this, *grid);
    return grid;

  } else {
    const auto& axisZ = *grid().axes().front();
    // 1D direction is binZ, so add an rPhi axis
    ActsScalar r = surface->bounds().get(CylinderBounds::eR);
    ActsScalar hlPhi = surface->bounds().get(CylinderBounds::eHalfPhiSector);
    ActsScalar hlRPhi = r * hlPhi;

    auto makeGrid = [&](auto bdt) {
      auto grid = axisZ.visit([&](const auto& axis1) {
        Axis axisRPhi{bdt, -hlRPhi, hlRPhi, 1};
        const IAxis* axis = other != nullptr ? other : &axisRPhi;
        return axis->visit(
            [&](const auto& axis0) -> std::unique_ptr<GridPortalLink> {
              return GridPortalLink::make(surface, axis0, axis1);
            });
      });

      fillGrid1dTo2d(FillDirection::loc0, *this, *grid);
      return grid;
    };

    if (surface->bounds().coversFullAzimuth()) {
      return makeGrid(AxisClosed);
    } else {
      return makeGrid(AxisBound);
    }
  }
}

std::unique_ptr<GridPortalLink> GridPortalLink::extendTo2dImpl(
    const std::shared_ptr<DiscSurface>& surface, const IAxis* other) const {
  assert(dim() == 1);

  const auto* bounds = dynamic_cast<const RadialBounds*>(&surface->bounds());
  if (bounds == nullptr) {
    throw std::invalid_argument(
        "GridPortalLink: DiscBounds: invalid bounds type.");
  }

  if (direction() == BinningValue::binR) {
    const auto& axisR = *grid().axes().front();
    // 1D direction is binR, so add a phi axis
    ActsScalar hlPhi = bounds->get(RadialBounds::eHalfPhiSector);

    auto makeGrid = [&](auto bdt) {
      auto grid = axisR.visit([&](const auto& axis0) {
        Axis axisPhi{bdt, -hlPhi, hlPhi, 1};
        const IAxis* axis = other != nullptr ? other : &axisPhi;
        return axis->visit(
            [&](const auto& axis1) -> std::unique_ptr<GridPortalLink> {
              return GridPortalLink::make(surface, axis0, axis1);
            });
      });

      fillGrid1dTo2d(FillDirection::loc1, *this, *grid);
      return grid;
    };

    if (bounds->coversFullAzimuth()) {
      return makeGrid(AxisClosed);
    } else {
      return makeGrid(AxisBound);
    }
  } else {
    const auto& axisPhi = *grid().axes().front();
    // 1D direction is binPhi, so add an R axis
    ActsScalar rMin = bounds->get(RadialBounds::eMinR);
    ActsScalar rMax = bounds->get(RadialBounds::eMaxR);

    auto grid = axisPhi.visit([&](const auto& axis1) {
      Axis axisR{AxisBound, rMin, rMax, 1};
      const IAxis* axis = other != nullptr ? other : &axisR;
      return axis->visit(
          [&surface,
           &axis1](const auto& axis0) -> std::unique_ptr<GridPortalLink> {
            return GridPortalLink::make(surface, axis0, axis1);
          });
    });
    fillGrid1dTo2d(FillDirection::loc0, *this, *grid);
    return grid;
  }
}

}  // namespace Acts

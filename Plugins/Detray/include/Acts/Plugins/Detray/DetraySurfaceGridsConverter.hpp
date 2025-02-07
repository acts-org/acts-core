// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/IAxis.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/TypeList.hpp"

#include <detray/io/frontend/payloads.hpp>

namespace Acts {

class Surface;
class SurfaceBounds;
class IAxis;

namespace Experimental {
class Detector;
}

namespace DetraySurfaceGridsConverter {

/// Conversion method for axis objects to detray::axis payloads
///
/// @param ia the axis to be converted
///
/// @return the axis_payload(bounds, binning, bins, edges)
detray::io::axis_payload convertAxis(const IAxis& ia);

/// Conversion method for grid objects to detray::grid payloads
///
/// @param grid the grid to be converted
/// @param swapAxis the flag for swapping the axes
///
/// @return the grid_payload(axes, bins)
template <typename grid_type>
detray::io::grid_payload<std::size_t, detray::io::accel_id> convertGrid(
    const grid_type& grid, bool swapAxis = false);

/// Conversion method for index grid objects to detray::grid payloads
///
/// @param indexGrid the index grid to be converted
///
/// @return the grid_payload(axes, bins)
template <typename index_grid>
detray::io::grid_payload<std::size_t, detray::io::accel_id> convertImpl(
    const index_grid& indexGrid);

/// Conversion method for instance objects to detray::grid payloads
///
/// @param delegate the internal navigation delegate
/// @param refInstance the reference instance
///
/// @return the grid_payload(axes, bins)
template <typename instance_type>
std::optional<detray::io::grid_payload<std::size_t, detray::io::accel_id>>
convert(const Experimental::InternalNavigationDelegate& delegate,
        [[maybe_unused]] const instance_type& refInstance);

/// Conversion method for instance objects to detray::grid payloads
///
/// @param delegate the internal navigation delegate
/// @param refInstance the reference instance
///
/// @return the grid_payload(axes, bins)
template <typename... Args>
std::vector<detray::io::grid_payload<std::size_t, detray::io::accel_id>>
unrollConvert(const Acts::Experimental::InternalNavigationDelegate& delegate,
              Acts::TypeList<Args...> /*unused*/);

/// Conversion method for instance objects to detray::grid payloads
///
/// @param delegate the internal navigation delegate
/// @param refInstance the reference instance
///
/// @return the grid_payload(axes, bins)
detray::io::detector_grids_payload<std::size_t, detray::io::accel_id>
convertSurfaceGrids(const Acts::Experimental::Detector& detector);

}  // namespace DetraySurfaceGridsConverter

}  // namespace Acts

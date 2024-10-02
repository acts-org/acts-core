# Third party software

This contains software that is usually not available through the system
package manager.

**Note** Only include software here that is either a header-only library or
can be built as a static library. Do not use any of this software as part of
the public interface and only include it in pure implementation files to avoid
issues with missing files after installation.

`nlohmann_json` is exempted from this rule as it is handled specially.

## nlohmann_json

CMake instructions to build [nlohmann::json](https://github.com/nlohmann/json).

## boost

For convenience, it's possible to use the ACTS build system to build the minimum
required version of [boost](https://www.boost.org/) (currently 1.71.0).  No source is
bundled here, and if requested via "-DACTS_USE_SYSTEM_BOOST=OFF", only the filesystem,
program_options, and test libraries will be built.

Warning: during installation, the built boost libraries will be installed alongside the
ACTS libraries, with a version suffix. This location may be known to the system linker.

## eigen3

For convenience, it's possible to use the ACTS build system to build
the minimum required version of [Eigen](https://eigen.tuxfamily.org)
(currently 3.3.7), with "-DACTS_USE_SYSTEM_EIGEN3=OFF".

## pybind11

CMake instructions to build [Pybind11](https://github.com/pybind/pybind11), which is used to create python bindings for the examples.

## FRNN

CMake instructions to build [FRNN](https://github.com/lxxue/FRNN), which is used by the Exa.TrkX plugin.

## Algebra plugins

CMake instruction to build [algebra-plugins](https://github.com/acts-project/algebra-plugins), which is used by the traccc plugin.

## VecMem

CMake instruction to build [VecMem](https://github.com/acts-project/vecmem), which is used by the traccc plugin.

## Covfie

CMake instruction to build [covfie](https://github.com/acts-project/covfie), which is used by the traccc plugin.

## Detray

CMake instruction to build [detray](https://github.com/acts-project/detray), which is used by the traccc plugin.

## Traccc

CMake instruction to build [traccc](https://github.com/acts-project/traccc), which is used by the traccc plugin.

## Annoy

CMake instructions to build [Annoy](https://github.com/spotify/annoy) which is used by the Hashing plugin.

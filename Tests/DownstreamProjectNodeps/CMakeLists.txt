cmake_minimum_required(VERSION 3.11)

project(ActsDownstreamProjectNodeps)

find_package(Acts CONFIG REQUIRED COMPONENTS Core)

# place artifacts in GNU-like paths, e.g. binaries in `<build>/bin`
include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}"
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}"
)

add_executable(ShowActsVersion ShowActsVersion.cpp)
target_link_libraries(
    ShowActsVersion
    PRIVATE ActsCore Boost::boost Eigen3::Eigen
)

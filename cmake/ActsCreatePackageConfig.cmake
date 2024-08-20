# Create relocatable CMake package config files

include(CMakePackageConfigHelpers)

# use path suggested by
# https://cmake.org/cmake/help/v3.18/manual/cmake-packages.7.html
set(install_package_config_dir "${CMAKE_INSTALL_LIBDIR}/cmake/Acts")

# version is taken automatically from PROJECT_VERSION; no need to specify
write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/ActsConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/ActsConfig.cmake.in
    ${PROJECT_BINARY_DIR}/ActsConfig.cmake
    INSTALL_DESTINATION ${install_package_config_dir}
    PATH_VARS CMAKE_INSTALL_BINDIR CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

# install cmake package configs
install(
    FILES
        ${PROJECT_BINARY_DIR}/ActsConfigVersion.cmake
        ${PROJECT_BINARY_DIR}/ActsConfig.cmake
    DESTINATION ${install_package_config_dir}
)

# install third party FindXXX.cmake files
install(
    FILES ${CMAKE_CURRENT_LIST_DIR}/FindOnnxRuntime.cmake
    DESTINATION ${install_package_config_dir}/Modules
)

# install target configs for all available components
foreach(_component ${_components})
    install(
        EXPORT Acts${_component}Targets
        DESTINATION ${install_package_config_dir}
    )
endforeach()

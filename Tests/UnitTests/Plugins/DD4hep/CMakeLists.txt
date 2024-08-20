add_library(
    ActsTestsDD4hepFactories
    SHARED
    DD4hepTestsHelper.cpp
    LayerFactory.cpp
    PrimitivesFactory.cpp
)

target_include_directories(
    ActsTestsDD4hepFactories
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
    ActsTestsDD4hepFactories
    PUBLIC ActsPluginDD4hep DD4hep::DDCore
)

# Not sure why this needs to be set, but dd4hep fails to compile otherwise
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(
    ActsTestsDD4hepFactories
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
dd4hep_set_version(ActsTestsDD4hepFactories MAJOR 1 MINOR 0 PATCH 0)
dd4hep_generate_rootmap(ActsTestsDD4hepFactories)

find_library(dd4hep_core_library DDCore)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
    set(factory_path
        "$<PATH:GET_PARENT_PATH,$<TARGET_FILE:ActsTestsDD4hepFactories>>"
    )
else()
    set(factory_path "${CMAKE_CURRENT_BINARY_DIR}")
endif()

if(NOT "${dd4hep_core_library}" STREQUAL "dd4hep_core_library-NOTFOUND")
    cmake_path(GET dd4hep_core_library PARENT_PATH DD4HEP_LIBRARY_PATH)
    # set test library dependencies
    set(unittest_extra_libraries ActsTestsDD4hepFactories ActsPluginDD4hep)
    set(dd4hep_tests
        DD4hepDetectorElement
        DD4hepCylinderLayerStructure
        DD4hepDiscLayerStructure
        DD4hepCylindricalDetector
    )
    foreach(_test ${dd4hep_tests})
        add_unittest(${_test} ${_test}Tests.cpp)
        add_dependencies(
            ActsUnitTest${_test}
            Components_ActsTestsDD4hepFactories
        )
        if(APPLE)
            set_property(
                TEST ${_test}
                PROPERTY
                    ENVIRONMENT
                        "DYLD_LIBRARY_PATH=${DD4HEP_LIBRARY_PATH}:${factory_path}:$ENV{DYLD_LIBRARY_PATH}"
            )
        else()
            set_property(
                TEST ${_test}
                PROPERTY
                    ENVIRONMENT
                        "LD_LIBRARY_PATH=${DD4HEP_LIBRARY_PATH}:${factory_path}:$ENV{LD_LIBRARY_PATH}"
            )
        endif()
    endforeach()
else()
    message(
        WARNING
        "DD4hep libraries not found, DD4hep tests will not be built."
    )
endif()

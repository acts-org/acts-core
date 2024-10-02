# get the path of the test data directory
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Data" acts_test_data_dir)

add_library(
    ActsTestsCommonHelpers
    SHARED
    Acts/Tests/CommonHelpers/DataDirectory.cpp
    Acts/Tests/CommonHelpers/CylindricalDetector.cpp
)
target_compile_definitions(
    ActsTestsCommonHelpers
    PRIVATE
        "ACTS_TEST_DATA_DIR=\"${acts_test_data_dir}\""
        BOOST_FILESYSTEM_NO_DEPRECATED
)
target_include_directories(
    ActsTestsCommonHelpers
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
    ActsTestsCommonHelpers
    PUBLIC ActsCore
    PRIVATE std::filesystem
)

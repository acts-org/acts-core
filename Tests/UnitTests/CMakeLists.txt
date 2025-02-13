# add a unittest executable w/ default dependencies and register it.

# the common libraries which are linked to every unittest can be
# extended by setting the `unittest_extra_libraries` variables before
# calling the macro.

add_custom_target(unit_tests)

macro(add_unittest _name _source)
    # automatically prefix the target name
    set(_target "ActsUnitTest${_name}")
    add_executable(${_target} ${_source})
    # define required BOOST_TEST_... macros here to ensure consistent names
    target_compile_definitions(${_target} PRIVATE "-DBOOST_TEST_DYN_LINK")
    set_source_files_properties(
        ${_source}
        PROPERTIES COMPILE_DEFINITIONS "BOOST_TEST_MODULE=${_target}"
    )
    target_include_directories(${_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(
        ${_target}
        PRIVATE
            ActsCore
            ActsTestsCommonHelpers
            Boost::unit_test_framework
            ${unittest_extra_libraries}
    )
    # register as unittest executable
    add_test(NAME ${_name} COMMAND ${_target})
    add_dependencies(unit_tests ${_target})
endmacro()

add_subdirectory(Core)
add_subdirectory_if(Examples ACTS_BUILD_EXAMPLES AND ACTS_BUILD_EXAMPLES_UNITTESTS)
add_subdirectory_if(Benchmarks ACTS_BUILD_BENCHMARKS)
add_subdirectory_if(Fatras ACTS_BUILD_FATRAS)
add_subdirectory(Plugins)
add_subdirectory_if(Alignment ACTS_BUILD_ALIGNMENT)

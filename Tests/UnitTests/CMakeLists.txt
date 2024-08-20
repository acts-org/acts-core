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

# This function adds a non compile test. To this end it
# - Adds a target to process the file at `src`, and converts begin/end macros
#   to `#if defined` in an extra file
# - Adds an executable target for that source file, excludes it from the default build
#   Set a preprocessor define to enable ONE critical section.
# - Adds a test job to ctest which invokes CMake to build this executable target.
#   The test is set to fail if the build succeeds, i.e. testing if something doesn't compile
# - Adds a closure test where it's supposed to actually compile if all of the
#   critical sections are removed
function(add_non_compile_test name src)
    # Don't add anything if the corresponding flag is not set
    if(NOT ACTS_BUILD_NONCOMPILE_TESTS)
        return()
    endif()

    # Figure out where to put the output file
    cmake_path(ABSOLUTE_PATH src)
    get_filename_component(_filename ${src} NAME)
    set(_processed_source "${CMAKE_CURRENT_BINARY_DIR}/${_filename}")

    # Add a build step to generate the source file by invoking a CMake script
    add_custom_command(
        OUTPUT ${_processed_source}
        DEPENDS ${src}
        COMMAND
            "${CMAKE_COMMAND}" -DINPUT_FILE=${src}
            -DOUTPUT_FILE=${_processed_source} -P
            ${CMAKE_SOURCE_DIR}/cmake/ActsGenerateNonCompileTest.cmake
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_target("${name}_generated_source" DEPENDS ${_processed_source})

    # Create the executable target, add the generated source code as a
    # dependencies, so that it's generated when required.
    set(test "ActsNonCompileTest${name}Closure")
    set(target "${test}_Executable")
    add_executable(${target} ${_processed_source})
    target_link_libraries(${target} PUBLIC ActsCore ActsTestsCommonHelpers)
    add_dependencies(${target} "${name}_generated_source")

    # Don't build this target by default
    set_target_properties(
        ${target}
        PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON
    )

    # Add the test that calls into CMake to build the target. This one we expect to succeed
    add_test(
        NAME ${test}
        COMMAND
            ${CMAKE_COMMAND} --build . --target ${target} --config
            $<CONFIGURATION>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    # Loop over critical section markers and add one pair of executable targets and
    # and test jobs. The test jobs are flipped, so success is failure.
    file(READ ${src} content)
    string(
        REGEX MATCHALL
        "ACTS_DOES_NOT_COMPILE_BEGIN\\(([A-Za-z0-9]+)\\)"
        matches
        ${content}
    )
    foreach(match ${matches})
        string(
            REGEX REPLACE
            "ACTS_DOES_NOT_COMPILE_BEGIN\\(([A-Za-z0-9]+)\\)"
            "\\1"
            match
            ${match}
        )

        set(test "ActsNonCompileTest${name}${match}")
        set(target "${test}_Executable")
        add_executable(${target} ${_processed_source})
        target_link_libraries(${target} PUBLIC ActsCore ActsTestsCommonHelpers)
        target_compile_definitions(${target} PRIVATE "-D${match}")
        add_dependencies(${target} "${name}_generated_source")

        set_target_properties(
            ${target}
            PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON
        )

        add_test(
            NAME ${test}
            COMMAND
                ${CMAKE_COMMAND} --build . --target ${target} --config
                $<CONFIGURATION>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        set_tests_properties(${test} PROPERTIES WILL_FAIL ON)
    endforeach()
endfunction()

add_subdirectory(Core)
add_subdirectory_if(Examples ACTS_BUILD_EXAMPLES AND ACTS_BUILD_EXAMPLES_UNITTESTS)
add_subdirectory_if(Benchmarks ACTS_BUILD_BENCHMARKS)
add_subdirectory_if(Fatras ACTS_BUILD_FATRAS)
add_subdirectory(Plugins)
add_subdirectory_if(Alignment ACTS_BUILD_ALIGNMENT)

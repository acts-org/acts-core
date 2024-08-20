# to be compatible with readthedocs.org, the documentation build is always
# in-source; as opposed to the configurable out-of-source build directory for
# the code. when running running on readthedocs.org, the build is fully driven
# by Sphinx, including running Doxygen.
#
# this CMake-based build is only intended for local development.

set(sphinx_build ${CMAKE_CURRENT_SOURCE_DIR}/_build)
set(sphinx_doctrees ${CMAKE_CURRENT_SOURCE_DIR}/_build/doctrees)
set(sphinx_html ${CMAKE_CURRENT_SOURCE_DIR}/_build/html)

set(sphinx_command)
list(APPEND sphinx_command "-b;html")
list(APPEND sphinx_command "-d;${sphinx_doctrees}")
list(APPEND sphinx_command "-j;auto")
list(APPEND sphinx_command "-W;--keep-going")
list(APPEND sphinx_command "-t;run_doxygen;-t;lazy_autodoc")

if(DEFINED ENV{CI})
    list(APPEND sphinx_command "-b;linkcheck")
endif()

list(APPEND sphinx_command "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND sphinx_command "${sphinx_html}")

message(${sphinx_command})

# standard target to build the documentation without automatic API generation
add_custom_target(
    docs
    COMMAND ${Sphinx_EXECUTABLE} ${sphinx_command}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Build documentation"
)

install(
    DIRECTORY ${sphinx_html}/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/Acts
    OPTIONAL
)
